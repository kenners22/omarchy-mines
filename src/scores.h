#pragma once

#include <QDateTime>
#include <QHash>
#include <QString>
#include <QVector>

// Best times and win counts, one record per level, kept as JSON in the
// app's data directory. A level is "beginner", "intermediate", "expert", or
// a custom board's own key ("custom-20x12-40"), so every board size keeps
// its own list.
class Scores {
public:
    struct Time {
        qint64 ms = 0;
        QDateTime at;
    };

    struct Level {
        int played = 0;
        int won = 0;
        QVector<Time> times; // fastest first, at most Kept
    };

    enum Problem { None, Unreadable, Unwritable };

    static constexpr int Kept = 10;

    explicit Scores(const QString &path);

    QString path() const { return m_path; }
    Problem problem() const { return m_problem; }

    // Where an unreadable file was moved so its times aren't lost.
    QString setAside() const { return m_setAside; }

    Level level(const QString &key) const { return m_levels.value(key); }
    qint64 best(const QString &key) const;

    // A game counts as played from its first opened cell, so one abandoned
    // halfway still shows in the win rate.
    void started(const QString &key);

    // Records a win. True when it is the fastest time yet for the level.
    bool won(const QString &key, qint64 ms, const QDateTime &at);

private:
    void load();
    void save();

    QString m_path;
    QString m_setAside;
    Problem m_problem = None;
    QHash<QString, Level> m_levels;
};
