#pragma once

#include <QAbstractListModel>
#include <QElapsedTimer>
#include <QTimer>
#include <QVariantMap>

#include "board.h"
#include "scores.h"

// The game as the window sees it: the board as a list model (one row per
// cell, left to right, top to bottom), the clock, the level, and the best
// times. QML calls the invokables and draws whatever the properties say.
class Game : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int columns READ columns NOTIFY boardChanged)
    Q_PROPERTY(int rows READ rows NOTIFY boardChanged)
    Q_PROPERTY(int mineCount READ mineCount NOTIFY boardChanged)
    Q_PROPERTY(QString level READ level NOTIFY boardChanged)
    Q_PROPERTY(int customColumns READ customColumns NOTIFY customChanged)
    Q_PROPERTY(int customRows READ customRows NOTIFY customChanged)
    Q_PROPERTY(int customMines READ customMines NOTIFY customChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY stateChanged)
    Q_PROPERTY(bool over READ over NOTIFY stateChanged)
    Q_PROPERTY(bool won READ won NOTIFY stateChanged)
    Q_PROPERTY(bool lost READ lost NOTIFY stateChanged)
    Q_PROPERTY(bool newBest READ newBest NOTIFY stateChanged)
    Q_PROPERTY(int minesLeft READ minesLeft NOTIFY minesLeftChanged)
    Q_PROPERTY(qint64 elapsed READ elapsed NOTIFY elapsedChanged)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool suspended READ suspended WRITE setSuspended NOTIFY pausedChanged)
    Q_PROPERTY(bool covered READ covered NOTIFY pausedChanged)
    Q_PROPERTY(qint64 best READ best NOTIFY scoresChanged)
    Q_PROPERTY(int played READ played NOTIFY scoresChanged)
    Q_PROPERTY(int wins READ wins NOTIFY scoresChanged)
    Q_PROPERTY(QString issue READ issue NOTIFY issueChanged)
    Q_PROPERTY(int minSide READ minSide CONSTANT)
    Q_PROPERTY(int maxColumns READ maxColumns CONSTANT)
    Q_PROPERTY(int maxRows READ maxRows CONSTANT)

public:
    // Board::Face, named again here so QML can say Game.Flag.
    enum Face { Hidden, Flag, Open, Mine, Blast, Misflag };
    Q_ENUM(Face)

    enum Role { FaceRole = Qt::UserRole + 1, AdjacentRole, DelayRole };

    // An empty directory means the app's own (~/.local/share/mines); tests
    // pass a temporary one.
    explicit Game(const QString &dataDirectory = {}, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int columns() const { return m_board.columns(); }
    int rows() const { return m_board.rows(); }
    int mineCount() const { return m_board.mineCount(); }
    QString level() const { return m_level; }
    int customColumns() const { return m_customColumns; }
    int customRows() const { return m_customRows; }
    int customMines() const { return m_customMines; }

    bool playing() const { return m_board.state() == Board::Playing; }
    bool over() const { return won() || lost(); }
    bool won() const { return m_board.state() == Board::Won; }
    bool lost() const { return m_board.state() == Board::Lost; }
    bool newBest() const { return m_newBest; }
    int minesLeft() const { return m_board.minesLeft(); }
    qint64 elapsed() const;

    // Paused is the player's own (P); suspended is the window's, set while
    // it is out of focus or showing a dialog. Either stops the clock, and a
    // game in play is covered meanwhile so a pause is no free look.
    bool paused() const { return m_paused; }
    void setPaused(bool paused);
    bool suspended() const { return m_suspended; }
    void setSuspended(bool suspended);
    bool covered() const { return playing() && (m_paused || m_suspended); }

    qint64 best() const { return m_scores.best(scoreKey()); }
    int played() const { return m_scores.level(scoreKey()).played; }
    int wins() const { return m_scores.level(scoreKey()).won; }
    QString issue() const;

    // The limits of a custom board.
    int minSide() const { return Board::MinSide; }
    int maxColumns() const { return Board::MaxColumns; }
    int maxRows() const { return Board::MaxRows; }

    // Tests and screenshots pin the mine layout.
    Board &board() { return m_board; }

    Q_INVOKABLE void newGame();
    Q_INVOKABLE void setLevel(const QString &level);
    Q_INVOKABLE void setCustom(int columns, int rows, int mines);
    Q_INVOKABLE int maxMines(int columns, int rows) const;

    // The three moves, and act(): what a plain click or Space does, which is
    // open a covered cell or chord an open number.
    Q_INVOKABLE void reveal(int index);
    Q_INVOKABLE void flag(int index);
    Q_INVOKABLE void chord(int index);
    Q_INVOKABLE void act(int index);
    Q_INVOKABLE int faceAt(int index) const { return int(m_board.face(index)); }

    // "1:07", or "1:07.4" with tenths, in the language's digits.
    Q_INVOKABLE QString formatTime(qint64 ms, bool tenths) const;

    // A level's record for the best times card: played, won, and times as
    // a list of {time, date}. "custom" means the custom board as set now.
    Q_INVOKABLE QVariantMap record(const QString &level) const;

signals:
    void boardChanged();
    void customChanged();
    void stateChanged();
    void minesLeftChanged();
    void elapsedChanged();
    void pausedChanged();
    void scoresChanged();
    void issueChanged();

private:
    QString scoreKey(const QString &level = {}) const;
    void start(const QString &level);
    void settle(Board::State before, const Board::Changes &changes);
    void syncClock();
    void loadSettings();
    void saveSettings() const;

    QString m_directory;
    Board m_board;
    Scores m_scores;
    QString m_level = "beginner";
    int m_customColumns = 20;
    int m_customRows = 14;
    int m_customMines = 45;
    QVector<int> m_delay;
    bool m_newBest = false;
    bool m_paused = false;
    bool m_suspended = false;
    int m_shownMinesLeft = 0;
    qint64 m_banked = 0;
    QElapsedTimer m_clock;
    QTimer m_tick;
};
