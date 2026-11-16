#include "board.h"

#include <algorithm>
#include <cstdlib>

Board::Board(int columns, int rows, int mines)
    : m_random(QRandomGenerator::securelySeeded()) {
    reset(columns, rows, mines);
}

void Board::reset(int columns, int rows, int mines) {
    m_columns = std::clamp(columns, MinSide, MaxColumns);
    m_rows = std::clamp(rows, MinSide, MaxRows);
    m_mines = std::clamp(mines, 1, maxMines(m_columns, m_rows));
    m_cells.fill(Cell(), m_columns * m_rows);
    m_flags = 0;
    m_open = 0;
    m_laid = false;
    m_state = Ready;
}

void Board::layMines(const QVector<int> &cells) {
    reset(m_columns, m_rows, m_mines);
    int count = 0;
    for (int index : cells) {
        if (contains(index) && !m_cells[index].mine) {
            m_cells[index].mine = true;
            ++count;
        }
    }
    m_mines = count;
    m_laid = true;
    countAdjacent();
}

QVector<int> Board::neighbours(int index) const {
    QVector<int> result;
    if (!contains(index))
        return result;
    const int column = index % m_columns, row = index / m_columns;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            const int c = column + dx, r = row + dy;
            if ((dx != 0 || dy != 0) && c >= 0 && c < m_columns && r >= 0 && r < m_rows)
                result.append(r * m_columns + c);
        }
    }
    return result;
}

Board::Face Board::face(int index) const {
    if (!contains(index))
        return Hidden;
    const Cell &cell = m_cells[index];
    if (cell.open)
        return cell.mine ? Blast : Open;
    if (m_state == Lost) {
        if (cell.flagged)
            return cell.mine ? Flag : Misflag;
        return cell.mine ? Mine : Hidden;
    }
    // A cleared board flags whatever mines were left unmarked.
    if (m_state == Won)
        return cell.mine ? Flag : Hidden;
    return cell.flagged ? Flag : Hidden;
}

// Mines go anywhere except the first cell opened and the cells around it, so
// the first move always opens an area rather than a lone number. maxMines()
// keeps enough room for that on every board.
void Board::scatterMines(int safeIndex) {
    QVector<bool> clear(m_cells.size(), false);
    clear[safeIndex] = true;
    const auto around = neighbours(safeIndex);
    if (m_cells.size() - around.size() - 1 >= m_mines)
        for (int index : around)
            clear[index] = true;

    QVector<int> candidates;
    candidates.reserve(m_cells.size());
    for (int index = 0; index < m_cells.size(); ++index)
        if (!clear[index])
            candidates.append(index);

    // A partial Fisher-Yates shuffle: each mine is a uniform pick from the
    // candidates not yet taken.
    for (int placed = 0; placed < m_mines; ++placed) {
        const int pick = placed + int(m_random.bounded(quint32(candidates.size() - placed)));
        std::swap(candidates[placed], candidates[pick]);
        m_cells[candidates[placed]].mine = true;
    }
    m_laid = true;
    countAdjacent();
}

void Board::countAdjacent() {
    for (int index = 0; index < m_cells.size(); ++index) {
        int count = 0;
        for (int neighbour : neighbours(index))
            count += m_cells[neighbour].mine ? 1 : 0;
        m_cells[index].adjacent = quint8(count);
    }
}

int Board::distance(int from, int to) const {
    return std::max(std::abs(from % m_columns - to % m_columns), std::abs(from / m_columns - to / m_columns));
}

// Opens a safe cell and, when nothing around it is mined, everything it
// touches, and so on outward. Flags hold: a flagged cell is never opened.
void Board::open(int index, int origin, Changes &changes) {
    QVector<int> pending{index};
    while (!pending.isEmpty()) {
        const int current = pending.takeLast();
        Cell &cell = m_cells[current];
        if (cell.open || cell.flagged)
            continue;
        cell.open = true;
        ++m_open;
        changes.append({current, distance(origin, current)});
        if (cell.adjacent == 0)
            for (int neighbour : neighbours(current))
                if (!m_cells[neighbour].open && !m_cells[neighbour].flagged)
                    pending.append(neighbour);
    }
}

// Settles the game after a move: lost if a mine was opened, won once every
// safe cell is open. Either way the remaining mines change face.
void Board::finish(int origin, Changes &changes) {
    bool blast = false;
    for (const Change &change : changes)
        blast = blast || m_cells[change.index].mine;

    if (blast)
        m_state = Lost;
    else if (m_open == m_cells.size() - m_mines)
        m_state = Won;
    else
        return;

    for (int index = 0; index < m_cells.size(); ++index) {
        const Cell &cell = m_cells[index];
        if (cell.open)
            continue;
        const bool reveals = m_state == Lost ? cell.mine != cell.flagged : cell.mine && !cell.flagged;
        if (reveals)
            changes.append({index, distance(origin, index)});
    }
    if (m_state == Won)
        m_flags = m_mines;
}

Board::Changes Board::reveal(int index) {
    Changes changes;
    if (!contains(index) || m_state == Won || m_state == Lost)
        return changes;
    if (m_cells[index].open || m_cells[index].flagged)
        return changes;

    if (!m_laid)
        scatterMines(index);
    m_state = Playing;

    if (m_cells[index].mine) {
        m_cells[index].open = true;
        changes.append({index, 0});
    } else {
        open(index, index, changes);
    }
    finish(index, changes);
    return changes;
}

Board::Changes Board::toggleFlag(int index) {
    Changes changes;
    if (!contains(index) || m_state == Won || m_state == Lost || m_cells[index].open)
        return changes;
    Cell &cell = m_cells[index];
    cell.flagged = !cell.flagged;
    m_flags += cell.flagged ? 1 : -1;
    changes.append({index, 0});
    return changes;
}

// On an open number with that many flags around it, opens the rest of its
// neighbours in one move. A wrong flag means a mine gets opened.
Board::Changes Board::chord(int index) {
    Changes changes;
    if (!contains(index) || m_state != Playing)
        return changes;
    const Cell &centre = m_cells[index];
    if (!centre.open || centre.mine || centre.adjacent == 0)
        return changes;

    const auto around = neighbours(index);
    int flags = 0;
    for (int neighbour : around)
        flags += m_cells[neighbour].flagged ? 1 : 0;
    if (flags != centre.adjacent)
        return changes;

    for (int neighbour : around) {
        Cell &cell = m_cells[neighbour];
        if (cell.open || cell.flagged)
            continue;
        if (cell.mine) {
            cell.open = true;
            changes.append({neighbour, distance(index, neighbour)});
        } else {
            open(neighbour, index, changes);
        }
    }
    if (!changes.isEmpty())
        finish(index, changes);
    return changes;
}
