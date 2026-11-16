QT += core gui qml quick quickcontrols2 testlib
CONFIG += c++17 testcase
TARGET = mines_tests
TEMPLATE = app

INCLUDEPATH += ../src

HEADERS += ../src/board.h ../src/game.h ../src/scores.h ../src/theme.h ../src/translator.h
SOURCES += mines_tests.cpp ../src/board.cpp ../src/game.cpp ../src/scores.cpp ../src/theme.cpp ../src/translator.cpp
