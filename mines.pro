QT += core gui qml quick quickcontrols2

CONFIG += c++17 release
TARGET = mines
TEMPLATE = app

HEADERS += \
    src/board.h \
    src/game.h \
    src/scores.h \
    src/theme.h \
    src/translator.h

SOURCES += \
    src/main.cpp \
    src/board.cpp \
    src/game.cpp \
    src/scores.cpp \
    src/theme.cpp \
    src/translator.cpp

RESOURCES += src/resources.qrc
