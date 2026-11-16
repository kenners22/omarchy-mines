#include "scores.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <algorithm>

Scores::Scores(const QString &path) : m_path(path) {
    load();
}

qint64 Scores::best(const QString &key) const {
    const auto found = m_levels.constFind(key);
    return found == m_levels.constEnd() || found->times.isEmpty() ? 0 : found->times.first().ms;
}

void Scores::load() {
    QFile file(m_path);
    if (!file.exists())
        return;

    QJsonParseError error{};
    QJsonDocument document;
    if (file.open(QIODevice::ReadOnly))
        document = QJsonDocument::fromJson(file.readAll(), &error);
    else
        error.error = QJsonParseError::UnterminatedObject;
    file.close();

    // A file that can't be read is the player's history all the same: move
    // it aside rather than write a fresh one over it, and say so.
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        m_problem = Unreadable;
        const QString aside = m_path + ".unreadable-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
        if (QFile::rename(m_path, aside))
            m_setAside = aside;
        return;
    }

    const QJsonObject levels = document.object().value("levels").toObject();
    for (auto it = levels.begin(); it != levels.end(); ++it) {
        const QJsonObject object = it.value().toObject();
        Level level;
        level.played = std::max(0, object.value("played").toInt());
        level.won = std::clamp(object.value("won").toInt(), 0, level.played);
        for (const QJsonValue &value : object.value("times").toArray()) {
            const QJsonObject entry = value.toObject();
            const qint64 ms = qint64(entry.value("ms").toDouble());
            if (ms > 0)
                level.times.append({ms, QDateTime::fromString(entry.value("at").toString(), Qt::ISODate)});
        }
        std::stable_sort(level.times.begin(), level.times.end(),
                         [](const Time &a, const Time &b) { return a.ms < b.ms; });
        if (level.times.size() > Kept)
            level.times.resize(Kept);
        m_levels.insert(it.key(), level);
    }
}

// Written beside the file and renamed over it, so a crash or a full disk
// mid-write can't leave half a file where the best times were.
void Scores::save() {
    QJsonObject levels;
    for (auto it = m_levels.constBegin(); it != m_levels.constEnd(); ++it) {
        QJsonArray times;
        for (const Time &time : it->times)
            times.append(QJsonObject{{"ms", double(time.ms)}, {"at", time.at.toString(Qt::ISODate)}});
        levels.insert(it.key(), QJsonObject{{"played", it->played}, {"won", it->won}, {"times", times}});
    }
    const QByteArray data = QJsonDocument(QJsonObject{{"version", 1}, {"levels", levels}}).toJson();

    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile file(m_path);
    const bool saved = file.open(QIODevice::WriteOnly) && file.write(data) == data.size() && file.commit();
    if (!saved)
        m_problem = Unwritable;
    else if (m_problem == Unwritable)
        m_problem = None;
}

void Scores::started(const QString &key) {
    ++m_levels[key].played;
    save();
}

bool Scores::won(const QString &key, qint64 ms, const QDateTime &at) {
    Level &level = m_levels[key];
    level.won = std::min(level.won + 1, level.played);
    const bool fastest = level.times.isEmpty() || ms < level.times.first().ms;

    // After any equal time, so the earlier of two identical runs stays ahead.
    const auto position = std::upper_bound(level.times.begin(), level.times.end(), ms,
                                           [](qint64 value, const Time &time) { return value < time.ms; });
    level.times.insert(position, {ms, at});
    if (level.times.size() > Kept)
        level.times.resize(Kept);
    save();
    return fastest;
}
