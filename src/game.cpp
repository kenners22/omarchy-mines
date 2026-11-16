#include "game.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <algorithm>
#include <climits>

namespace {

struct Preset {
    const char *level;
    int columns, rows, mines;
};

const Preset presets[] = {
    {"beginner", 9, 9, 10},
    {"intermediate", 16, 16, 40},
    {"expert", 30, 16, 99},
};

QString dataDirectory(const QString &given) {
    return given.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) : given;
}

// Paths are shown to the player the way they'd type them.
QString homeRelative(const QString &path) {
    const QString home = QDir::homePath();
    return path.startsWith(home + "/") ? "~" + path.mid(home.size()) : path;
}

} // namespace

Game::Game(const QString &directory, QObject *parent)
    : QAbstractListModel(parent),
      m_directory(dataDirectory(directory)),
      m_scores(m_directory + "/scores.json") {
    // Ten times a second is enough for a clock that shows whole seconds,
    // and it only runs while a game does.
    m_tick.setInterval(100);
    connect(&m_tick, &QTimer::timeout, this, &Game::elapsedChanged);

    loadSettings();
    start(m_level);
}

int Game::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_board.cellCount();
}

QVariant Game::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || !m_board.contains(index.row()))
        return {};
    switch (role) {
    case FaceRole: return int(m_board.face(index.row()));
    case AdjacentRole: return m_board.adjacent(index.row());
    case DelayRole: return m_delay.value(index.row());
    }
    return {};
}

QHash<int, QByteArray> Game::roleNames() const {
    return {{FaceRole, "face"}, {AdjacentRole, "adjacent"}, {DelayRole, "delay"}};
}

qint64 Game::elapsed() const {
    return m_banked + (m_clock.isValid() ? m_clock.elapsed() : 0);
}

// The clock runs only while a game is in play and nothing has paused it.
void Game::syncClock() {
    const bool run = playing() && !m_paused && !m_suspended;
    if (run && !m_clock.isValid()) {
        m_clock.start();
        m_tick.start();
    } else if (!run && m_clock.isValid()) {
        m_banked += m_clock.elapsed();
        m_clock.invalidate();
        m_tick.stop();
        emit elapsedChanged();
    }
}

void Game::setPaused(bool paused) {
    // There is nothing to pause before the first move or after the last.
    paused = paused && playing();
    if (paused == m_paused)
        return;
    m_paused = paused;
    syncClock();
    emit pausedChanged();
}

void Game::setSuspended(bool suspended) {
    if (suspended == m_suspended)
        return;
    m_suspended = suspended;
    syncClock();
    emit pausedChanged();
}

QString Game::scoreKey(const QString &level) const {
    const QString which = level.isEmpty() ? m_level : level;
    if (which != "custom")
        return which;
    return QString("custom-%1x%2-%3").arg(m_customColumns).arg(m_customRows).arg(m_customMines);
}

QString Game::issue() const {
    switch (m_scores.problem()) {
    case Scores::Unreadable:
        if (!m_scores.setAside().isEmpty())
            return tr("Best times couldn't be read. The old file is kept as %1.")
                .arg(QFileInfo(m_scores.setAside()).fileName());
        return tr("Best times couldn't be read from %1.").arg(homeRelative(m_scores.path()));
    case Scores::Unwritable:
        return tr("Best times can't be saved to %1.").arg(homeRelative(m_scores.path()));
    case Scores::None:
        break;
    }
    return {};
}

// Lays out a fresh board for the level. The model only resets when the grid
// changes shape; a new game on the same grid just repaints its cells.
void Game::start(const QString &level) {
    int columns = m_customColumns, rows = m_customRows, mines = m_customMines;
    QString known = "custom";
    for (const Preset &preset : presets) {
        if (level == QLatin1String(preset.level)) {
            known = level;
            columns = preset.columns;
            rows = preset.rows;
            mines = preset.mines;
        }
    }

    const bool reshaped = columns != m_board.columns() || rows != m_board.rows() || m_delay.isEmpty();
    const bool changed = reshaped || mines != m_board.mineCount() || known != m_level;
    if (reshaped)
        beginResetModel();
    m_level = known;
    m_board.reset(columns, rows, mines);
    m_delay.fill(0, m_board.cellCount());
    if (reshaped)
        endResetModel();
    else
        emit dataChanged(index(0), index(m_board.cellCount() - 1));

    m_banked = 0;
    m_clock.invalidate();
    m_tick.stop();
    m_newBest = false;
    m_paused = false;
    m_shownMinesLeft = m_board.minesLeft();

    if (changed)
        emit boardChanged();
    emit stateChanged();
    emit minesLeftChanged();
    emit elapsedChanged();
    emit scoresChanged();
    emit pausedChanged();
}

void Game::newGame() {
    start(m_level);
}

void Game::setLevel(const QString &level) {
    start(level);
    saveSettings();
}

void Game::setCustom(int columns, int rows, int mines) {
    m_customColumns = std::clamp(columns, Board::MinSide, Board::MaxColumns);
    m_customRows = std::clamp(rows, Board::MinSide, Board::MaxRows);
    m_customMines = std::clamp(mines, 1, Board::maxMines(m_customColumns, m_customRows));
    emit customChanged();
    setLevel("custom");
}

int Game::maxMines(int columns, int rows) const {
    return Board::maxMines(std::clamp(columns, Board::MinSide, Board::MaxColumns),
                           std::clamp(rows, Board::MinSide, Board::MaxRows));
}

// Everything that follows a move: repaint the cells it changed, start or
// stop the clock, and write down a game begun or won.
void Game::settle(Board::State before, const Board::Changes &changes) {
    if (changes.isEmpty())
        return;
    const Board::State now = m_board.state();

    // Changes ripple outward from the move. The whole ripple takes about the
    // same short time however far it travels, a little longer for the mines
    // at the end of a game, and never holds up the next click.
    int farthest = 0, first = INT_MAX, last = -1;
    for (const Board::Change &change : changes) {
        farthest = std::max(farthest, change.distance);
        first = std::min(first, change.index);
        last = std::max(last, change.index);
    }
    const bool finished = now == Board::Won || now == Board::Lost;
    const double step = farthest == 0 ? 0 : std::min(finished ? 22.0 : 12.0, (finished ? 480.0 : 220.0) / farthest);
    for (const Board::Change &change : changes)
        m_delay[change.index] = int(change.distance * step);

    // The delay goes out first so a cell knows how long to wait by the time
    // it hears its face changed.
    emit dataChanged(index(first), index(last), {DelayRole});
    emit dataChanged(index(first), index(last), {FaceRole, AdjacentRole});

    if (before == Board::Ready && now != Board::Ready) {
        m_scores.started(scoreKey());
        emit scoresChanged();
    }
    if (now != before) {
        // The clock stops first, so the time written down is the time shown.
        syncClock();
        if (now == Board::Won) {
            m_newBest = m_scores.won(scoreKey(), std::max<qint64>(1, elapsed()), QDateTime::currentDateTime());
            emit scoresChanged();
        }
        emit stateChanged();
        emit pausedChanged();
        emit issueChanged();
    }
    if (m_board.minesLeft() != m_shownMinesLeft) {
        m_shownMinesLeft = m_board.minesLeft();
        emit minesLeftChanged();
    }
}

void Game::reveal(int index) {
    if (covered())
        return;
    const Board::State before = m_board.state();
    settle(before, m_board.reveal(index));
}

void Game::flag(int index) {
    if (covered())
        return;
    const Board::State before = m_board.state();
    settle(before, m_board.toggleFlag(index));
}

void Game::chord(int index) {
    if (covered())
        return;
    const Board::State before = m_board.state();
    settle(before, m_board.chord(index));
}

void Game::act(int index) {
    if (m_board.face(index) == Board::Open)
        chord(index);
    else
        reveal(index);
}

QString Game::formatTime(qint64 ms, bool tenths) const {
    const qint64 seconds = std::max<qint64>(0, ms) / 1000;
    QString text = seconds >= 3600
        ? QString("%1:%2:%3").arg(seconds / 3600).arg(seconds / 60 % 60, 2, 10, QChar('0')).arg(seconds % 60, 2, 10, QChar('0'))
        : QString("%1:%2").arg(seconds / 60).arg(seconds % 60, 2, 10, QChar('0'));
    const QLocale locale;
    if (tenths)
        text += locale.decimalPoint() + QString::number(std::max<qint64>(0, ms) % 1000 / 100);

    // Languages that count in their own digits get them here too.
    const QString zero = locale.zeroDigit();
    if (zero.size() == 1 && zero != "0")
        for (QChar &character : text)
            if (character >= QChar('0') && character <= QChar('9'))
                character = QChar(zero.at(0).unicode() + (character.unicode() - '0'));
    return text;
}

QVariantMap Game::record(const QString &level) const {
    const Scores::Level stored = m_scores.level(scoreKey(level));

    // The language's own long date, without the weekday and with the month
    // cut short: "Mar 2, 2026" or "2. Mär. 2026", never an ambiguous 3/2/26.
    const QLocale locale;
    QString format = locale.dateFormat(QLocale::LongFormat);
    static const QRegularExpression weekday("dddd[,\\s]*|[,\\s]*dddd");
    format.remove(weekday);
    format.replace("MMMM", "MMM");

    QVariantList times;
    for (const Scores::Time &time : stored.times)
        times.append(QVariantMap{{"time", formatTime(time.ms, true)},
                                 {"date", locale.toString(time.at.toLocalTime().date(), format)}});
    return {{"played", stored.played}, {"won", stored.won}, {"times", times}};
}

void Game::loadSettings() {
    QFile file(m_directory + "/settings.json");
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonObject settings = QJsonDocument::fromJson(file.readAll()).object();
    const QJsonObject custom = settings.value("custom").toObject();
    m_customColumns = std::clamp(custom.value("columns").toInt(m_customColumns), Board::MinSide, Board::MaxColumns);
    m_customRows = std::clamp(custom.value("rows").toInt(m_customRows), Board::MinSide, Board::MaxRows);
    m_customMines = std::clamp(custom.value("mines").toInt(m_customMines), 1,
                               Board::maxMines(m_customColumns, m_customRows));
    m_level = settings.value("level").toString(m_level);
}

// Which level to open on next time. Losing this is no loss, so a failed
// write is left alone; the best times report their own.
void Game::saveSettings() const {
    const QJsonObject custom{{"columns", m_customColumns}, {"rows", m_customRows}, {"mines", m_customMines}};
    const QByteArray data = QJsonDocument(QJsonObject{{"level", m_level}, {"custom", custom}}).toJson();
    QDir().mkpath(m_directory);
    QSaveFile file(m_directory + "/settings.json");
    if (file.open(QIODevice::WriteOnly) && file.write(data) == data.size())
        file.commit();
}
