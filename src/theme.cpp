// Theme watching follows Omacut (MIT, David Heinemeier Hansson).
#include "theme.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <algorithm>
#include <cmath>

namespace {

// colors.toml is flat `name = "value"` lines, so a pattern is all it takes.
QHash<QString, QString> readColors(const QString &path) {
    QHash<QString, QString> values;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return values;
    static const QRegularExpression line(R"re(^\s*([a-z_]+)\s*=\s*["']([^"']*)["'])re",
                                         QRegularExpression::MultilineOption);
    auto matches = line.globalMatch(QString::fromUtf8(file.readAll()));
    while (matches.hasNext()) {
        const auto match = matches.next();
        values.insert(match.captured(1), match.captured(2));
    }
    return values;
}

// A color the theme gives as "#rrggbb", or the fallback when it gives none.
QString hexOr(const QHash<QString, QString> &values, const QString &name, const QString &fallback) {
    static const QRegularExpression hex("^#[0-9a-fA-F]{6}$");
    const QString value = values.value(name);
    return hex.match(value).hasMatch() ? value : fallback;
}

QColor colorOr(const QHash<QString, QString> &values, const QString &name, const QColor &fallback) {
    return QColor(hexOr(values, name, fallback.name()));
}

// WCAG relative luminance.
double luminance(const QColor &color) {
    auto linear = [](double v) { return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4); };
    return .2126 * linear(color.redF()) + .7152 * linear(color.greenF()) + .0722 * linear(color.blueF());
}

} // namespace

Theme::Theme(const QString &directory, QObject *parent, const QString &hyprctl)
    : QObject(parent),
      m_directory(directory.isEmpty() ? QDir::homePath() + "/.local/state/omarchy/current" : directory) {
    // Omarchy is drawn in the system's monospace font; so is the game.
    m_fontFamily = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();

    m_debounce.setSingleShot(true);
    m_debounce.setInterval(80);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_debounce.start(); });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_debounce.start(); });
    connect(&m_debounce, &QTimer::timeout, this, &Theme::reload);

    // Corners follow Hyprland's effective rounding, which themes and personal
    // overrides can both set, so ask hyprctl rather than reading its config.
    // Without Hyprland the corners stay sharp, like Omarchy's default.
    m_hyprctl = !hyprctl.isEmpty() ? hyprctl
              : directory.isEmpty() && !qEnvironmentVariableIsEmpty("HYPRLAND_INSTANCE_SIGNATURE")
                  ? QStandardPaths::findExecutable("hyprctl") : QString();
    m_roundingTimeout.setSingleShot(true);
    m_roundingTimeout.setInterval(500);
    connect(&m_roundingTimeout, &QTimer::timeout, &m_roundingProcess, &QProcess::kill);
    connect(&m_roundingProcess, &QProcess::errorOccurred, this, [this] { m_roundingTimeout.stop(); });
    connect(&m_roundingProcess, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        m_roundingTimeout.stop();
        const auto value = QJsonDocument::fromJson(m_roundingProcess.readAllStandardOutput()).object().value("int");
        if (code != 0 || status != QProcess::NormalExit || !value.isDouble() || value.toInt(-1) < 0)
            return;
        if (value.toInt() != m_radius) {
            m_radius = value.toInt();
            emit changed();
        }
    });
    m_roundingPoll.setInterval(1000);
    connect(&m_roundingPoll, &QTimer::timeout, this, &Theme::refreshRounding);
    if (!m_hyprctl.isEmpty())
        m_roundingPoll.start();
    reload();
}

Theme::~Theme() {
    if (m_roundingProcess.state() != QProcess::NotRunning) {
        m_roundingProcess.kill();
        m_roundingProcess.waitForFinished(1000);
    }
}

void Theme::refreshRounding() {
    if (m_hyprctl.isEmpty() || m_roundingProcess.state() != QProcess::NotRunning)
        return;
    m_roundingProcess.start(m_hyprctl, {"-j", "getoption", "decoration:rounding"});
    m_roundingTimeout.start();
}

QString Theme::readAccent(const QString &path) {
    return hexOr(readColors(path), "accent", "#FFD60A");
}

QColor Theme::mix(const QColor &from, const QColor &to, double amount) {
    const double t = std::clamp(amount, 0.0, 1.0);
    return QColor::fromRgbF(float(from.redF() + (to.redF() - from.redF()) * t),
                            float(from.greenF() + (to.greenF() - from.greenF()) * t),
                            float(from.blueF() + (to.blueF() - from.blueF()) * t));
}

double Theme::contrast(const QColor &a, const QColor &b) {
    const double la = luminance(a), lb = luminance(b);
    return (std::max(la, lb) + .05) / (std::min(la, lb) + .05);
}

// Black or white, whichever reads on the accent.
QString Theme::accentForeground() const {
    return luminance(QColor(m_accent)) > .179 ? "black" : "white";
}

QColor Theme::panel() const {
    return mix(m_background, m_foreground, m_dark ? .055 : .03);
}

QColor Theme::well() const {
    return mix(m_background, m_dark ? QColor(Qt::black) : QColor(Qt::white), m_dark ? .30 : .55);
}

QColor Theme::grout() const {
    return mix(well(), m_foreground, m_dark ? .085 : .10);
}

// A theme's palette is its own business (some are all greys, some all
// greens), so a color that would be hard to read on a well is pulled toward
// the foreground until it isn't, and otherwise left alone.
QColor Theme::readable(const QColor &color, const QColor &on, double ratio) const {
    for (int step = 0; step <= 10; ++step) {
        const QColor candidate = mix(color, m_foreground, step / 10.0);
        if (contrast(candidate, on) >= ratio)
            return candidate;
    }
    return m_foreground;
}

QVariantList Theme::numbers() const {
    QVariantList colors;
    for (const QColor &color : m_palette)
        colors.append(readable(color, well(), 3.2));
    return colors;
}

void Theme::reload() {
    refreshRounding();
    const auto paths = m_watcher.files() + m_watcher.directories();
    if (!paths.isEmpty())
        m_watcher.removePaths(paths);

    // A theme switch replaces symlinks and files, so watch the parents too and
    // re-arm after every change. Watching the nearest existing ancestor means
    // installing Omarchy later works without restarting the app.
    QString ancestor = m_directory;
    while (!QFileInfo::exists(ancestor) && ancestor != "/")
        ancestor = QFileInfo(ancestor).absolutePath();
    QStringList candidates{ancestor, QFileInfo(ancestor).absolutePath(), m_directory,
                           m_directory + "/theme", m_directory + "/theme/colors.toml"};
    candidates.removeDuplicates();
    for (const auto &path : candidates)
        if (QFileInfo::exists(path))
            m_watcher.addPath(path);

    // Anything the theme doesn't say falls back to a dark default, one key at
    // a time, so a sparse colors.toml still themes what it can.
    const auto values = readColors(m_directory + "/theme/colors.toml");
    const QString accent = hexOr(values, "accent", "#FFD60A");
    m_background = colorOr(values, "background", QColor("#0e0e10"));
    m_foreground = colorOr(values, "foreground", QColor("#f5f5f7"));
    const QString mode = values.value("mode");
    m_dark = mode == "light" ? false : mode == "dark" ? true : luminance(m_background) < .4;

    const QColor red = colorOr(values, "red", QColor("#ff6b6b"));
    const QColor yellow = colorOr(values, "yellow", QColor("#ffd866"));
    m_palette = {
        colorOr(values, "blue", QColor("#6cb6ff")),
        colorOr(values, "green", QColor("#7ee787")),
        red,
        colorOr(values, "magenta", QColor("#d2a8ff")),
        yellow,
        colorOr(values, "cyan", QColor("#76e3ea")),
        colorOr(values, "orange", mix(red, yellow, .5)),
        dim(),
    };
    m_danger = readable(red, well(), 3.2);

    // Compared as a whole, so an unrelated change to the directory doesn't
    // repaint the window.
    QString signature = accent + m_background.name() + m_foreground.name() + (m_dark ? "d" : "l");
    for (const QColor &color : std::as_const(m_palette))
        signature += color.name();
    if (signature != m_signature) {
        m_signature = signature;
        m_accent = accent;
        emit changed();
    }
}
