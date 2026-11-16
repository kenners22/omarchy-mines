// mines — a mine-clearing puzzle for Omarchy.

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>
#include <QtQml>

#include "game.h"
#include "theme.h"
#include "translator.h"

int main(int argc, char *argv[]) {
    // --help and --version print and leave, so they need no display.
    for (int i = 1; i < argc; ++i) {
        const QByteArray argument(argv[i]);
        if (argument == "-h" || argument == "--help" || argument == "--help-all" || argument == "-v" || argument == "--version")
            qputenv("QT_QPA_PLATFORM", "offscreen");
    }

    QGuiApplication app(argc, argv);
    app.setApplicationName("mines");
    app.setApplicationVersion("0.1.0");

    // Matches the window to mines.desktop, so the compositor (Wayland app_id)
    // and the launcher pick up the installed icon.
    app.setDesktopFileName("mines");
    app.setWindowIcon(QIcon::fromTheme("mines"));

    QQuickStyle::setStyle("Material");

    // Installed before anything is translated, and every qsTr() binding is
    // re-evaluated when the Omarchy language changes.
    Translator translator;
    app.installTranslator(&translator);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "A mine-clearing puzzle for Omarchy."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    Theme theme;
    Game game;

    // QML reads the cell faces by name (Game.Flag); the game itself is the
    // one object below.
    qmlRegisterUncreatableType<Game>("Mines", 1, 0, "Game", "The game is the context property `game`.");

    QQmlApplicationEngine engine;
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    engine.rootContext()->setContextProperty("translator", &translator);
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("game", &game);
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return 1;

    return app.exec();
}
