#pragma once

#include <QRandomGenerator>
#include <QVector>

// The rules of the game, with no window attached: a grid of cells, some of
// them mined, and the three things a player can do to one (open it, flag it,
// or chord a number). Game wraps it for QML; the tests drive it directly.
class Board {
public:
    enum State { Ready, Playing, Won, Lost };

    // What a cell looks like to the player right now. Mine, Blast, and
    // Misflag only appear once the game is lost.
    enum Face { Hidden, Flag, Open, Mine, Blast, Misflag };

    // A cell whose face changed, and how far it is from the cell the player
    // acted on, so the window can ripple the change outward.
    struct Change {
        int index;
        int distance;
    };
    using Changes = QVector<Change>;

    static constexpr int MinSide = 5;
    static constexpr int MaxColumns = 50;
    static constexpr int MaxRows = 30;

    // Nine cells always stay clear so the first click can open an area.
    static int maxMines(int columns, int rows) { return columns * rows - 9; }

    explicit Board(int columns = 9, int rows = 9, int mines = 10);

    // Out-of-range sizes and mine counts are pulled into range.
    void reset(int columns, int rows, int mines);
    void restart() { reset(m_columns, m_rows, m_mines); }

    // Tests pin the layout: a seed for the random one, or the exact cells.
    void seed(quint32 value) { m_random.seed(value); }
    void layMines(const QVector<int> &cells);

    int columns() const { return m_columns; }
    int rows() const { return m_rows; }
    int mineCount() const { return m_mines; }
    int cellCount() const { return m_cells.size(); }
    State state() const { return m_state; }
    int flagCount() const { return m_flags; }
    int openCount() const { return m_open; }

    // Mines minus flags, so it goes negative when too many flags are down.
    int minesLeft() const { return m_state == Won ? 0 : m_mines - m_flags; }

    bool contains(int index) const { return index >= 0 && index < m_cells.size(); }
    Face face(int index) const;
    int adjacent(int index) const { return contains(index) ? m_cells[index].adjacent : 0; }
    bool isMine(int index) const { return contains(index) && m_cells[index].mine; }
    QVector<int> neighbours(int index) const;

    // Each returns the cells whose face changed; nothing when the move
    // doesn't apply. The first reveal lays the mines around the chosen cell.
    Changes reveal(int index);
    Changes toggleFlag(int index);
    Changes chord(int index);

private:
    struct Cell {
        bool mine = false;
        bool open = false;
        bool flagged = false;
        quint8 adjacent = 0;
    };

    void scatterMines(int safeIndex);
    void countAdjacent();
    int distance(int from, int to) const;
    void open(int index, int origin, Changes &changes);
    void finish(int origin, Changes &changes);

    int m_columns = 0;
    int m_rows = 0;
    int m_mines = 0;
    int m_flags = 0;
    int m_open = 0;
    bool m_laid = false;
    State m_state = Ready;
    QVector<Cell> m_cells;
    QRandomGenerator m_random;
};
