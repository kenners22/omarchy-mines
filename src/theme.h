#pragma once

#include <QColor>
#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QVariantList>

// Follows the Omarchy theme live: the accent, the window's background and
// foreground, light or dark, the palette the numbers are drawn in, and
// Hyprland's window rounding. Every other color the board needs is mixed from
// those, so a theme switch recolors the whole game with nothing to configure.
class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString accent READ accent NOTIFY changed)
    Q_PROPERTY(QString accentForeground READ accentForeground NOTIFY changed)
    Q_PROPERTY(int radius READ radius NOTIFY changed)
    Q_PROPERTY(bool dark READ dark NOTIFY changed)
    Q_PROPERTY(QColor background READ background NOTIFY changed)
    Q_PROPERTY(QColor foreground READ foreground NOTIFY changed)
    Q_PROPERTY(QColor dim READ dim NOTIFY changed)
    Q_PROPERTY(QColor faint READ faint NOTIFY changed)
    Q_PROPERTY(QColor panel READ panel NOTIFY changed)
    Q_PROPERTY(QColor grout READ grout NOTIFY changed)
    Q_PROPERTY(QColor tile READ tile NOTIFY changed)
    Q_PROPERTY(QColor tilePressed READ tilePressed NOTIFY changed)
    Q_PROPERTY(QColor well READ well NOTIFY changed)
    Q_PROPERTY(QColor danger READ danger NOTIFY changed)
    Q_PROPERTY(QVariantList numbers READ numbers NOTIFY changed)
    Q_PROPERTY(QString fontFamily READ fontFamily CONSTANT)

public:
    explicit Theme(const QString &currentDirectory = {}, QObject *parent = nullptr, const QString &hyprctl = {});
    ~Theme() override;

    QString accent() const { return m_accent; }
    QString accentForeground() const;
    int radius() const { return m_radius; }

    bool dark() const { return m_dark; }
    QColor background() const { return m_background; }
    QColor foreground() const { return m_foreground; }

    // Secondary text, and the hairlines and disabled marks fainter still.
    QColor dim() const { return mix(m_foreground, m_background, .38); }
    QColor faint() const { return mix(m_foreground, m_background, .72); }

    // Cards that sit over the window: the shortcuts, best times, and so on.
    QColor panel() const;

    // The board: covered tiles stand out from the window, opened wells sink
    // into it, and the grout is the thin line left between cells.
    QColor grout() const;
    QColor tile() const { return mix(m_background, m_foreground, m_dark ? .17 : .16); }
    QColor tilePressed() const { return mix(m_background, m_foreground, m_dark ? .08 : .07); }
    QColor well() const;
    QColor danger() const { return m_danger; }

    // The colors of the numbers 1 to 8, each readable on a well.
    QVariantList numbers() const;

    QString fontFamily() const { return m_fontFamily; }

    static QString readAccent(const QString &path);
    static QColor mix(const QColor &from, const QColor &to, double amount);
    static double contrast(const QColor &a, const QColor &b);

signals:
    void changed();

private:
    void reload();
    void refreshRounding();
    QColor readable(const QColor &color, const QColor &on, double ratio) const;

    QString m_directory;
    QString m_accent = "#FFD60A";
    bool m_dark = true;
    QColor m_background{"#0e0e10"};
    QColor m_foreground{"#f5f5f7"};
    QColor m_danger{"#ff6b6b"};
    QVector<QColor> m_palette;
    QString m_fontFamily;
    QString m_signature;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    QString m_hyprctl;
    int m_radius = 0;
    QProcess m_roundingProcess;
    QTimer m_roundingPoll, m_roundingTimeout;
};
