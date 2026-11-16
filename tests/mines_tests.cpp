#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "board.h"
#include "game.h"
#include "scores.h"
#include "theme.h"
#include "translator.h"

class MinesTests : public QObject {
    Q_OBJECT

private slots:
    // The board

    void boardStartsCovered()
    {
        Board board(9, 9, 10);
        QCOMPARE(board.state(), Board::Ready);
        QCOMPARE(board.cellCount(), 81);
        QCOMPARE(board.minesLeft(), 10);
        for (int index = 0; index < board.cellCount(); ++index)
            QCOMPARE(board.face(index), Board::Hidden);
    }

    void boardPullsSizesIntoRange()
    {
        Board board(2, 400, 100000);
        QCOMPARE(board.columns(), Board::MinSide);
        QCOMPARE(board.rows(), Board::MaxRows);
        QCOMPARE(board.mineCount(), Board::maxMines(Board::MinSide, Board::MaxRows));

        board.reset(9, 9, 0);
        QCOMPARE(board.mineCount(), 1);
    }

    // Every first move, on every level and from every cell, opens a cell
    // with no mine on it or beside it, and lays exactly the right number.
    void firstMoveIsAlwaysSafe_data()
    {
        QTest::addColumn<int>("columns");
        QTest::addColumn<int>("rows");
        QTest::addColumn<int>("mines");
        QTest::newRow("beginner") << 9 << 9 << 10;
        QTest::newRow("intermediate") << 16 << 16 << 40;
        QTest::newRow("expert") << 30 << 16 << 99;
        QTest::newRow("packed") << 8 << 8 << Board::maxMines(8, 8);
        QTest::newRow("smallest") << 5 << 5 << Board::maxMines(5, 5);
    }

    void firstMoveIsAlwaysSafe()
    {
        QFETCH(int, columns);
        QFETCH(int, rows);
        QFETCH(int, mines);

        Board board(columns, rows, mines);
        for (int first = 0; first < board.cellCount(); ++first) {
            board.restart();
            board.seed(quint32(first * 7919 + 1));
            const auto changes = board.reveal(first);
            QVERIFY(!changes.isEmpty());
            QVERIFY(board.state() != Board::Lost);
            QVERIFY(!board.isMine(first));
            QCOMPARE(board.adjacent(first), 0);
            for (int neighbour : board.neighbours(first))
                QVERIFY(!board.isMine(neighbour));

            int laid = 0;
            for (int index = 0; index < board.cellCount(); ++index)
                laid += board.isMine(index) ? 1 : 0;
            QCOMPARE(laid, mines);
        }
    }

    void minesLandEverywhereElse()
    {
        // Over many games every cell outside the safe area gets its turn.
        Board board(9, 9, 10);
        QSet<int> mined;
        for (int game = 0; game < 400; ++game) {
            board.restart();
            board.seed(quint32(game + 1));
            board.reveal(40);
            for (int index = 0; index < board.cellCount(); ++index)
                if (board.isMine(index))
                    mined.insert(index);
        }
        QCOMPARE(mined.size(), 81 - 9);
    }

    void adjacentCountsTheMinesAround()
    {
        Board board(5, 5, 1);
        board.layMines({0, 1, 2, 5, 7, 10, 11, 12});
        QCOMPARE(board.mineCount(), 8);
        QCOMPARE(board.adjacent(6), 8);
        QCOMPARE(board.adjacent(3), 2);
        QCOMPARE(board.adjacent(24), 0);
        QCOMPARE(board.neighbours(0).size(), 3);
        QCOMPARE(board.neighbours(2).size(), 5);
        QCOMPARE(board.neighbours(12).size(), 8);
    }

    void openingAnEmptyCellOpensItsArea()
    {
        // One mine in the far corner: a click anywhere else opens all the
        // rest, which also clears the board.
        Board board(5, 5, 1);
        board.layMines({24});
        const auto changes = board.reveal(0);
        QCOMPARE(board.openCount(), 24);
        QCOMPARE(board.state(), Board::Won);
        QCOMPARE(board.face(24), Board::Flag);
        QCOMPARE(board.minesLeft(), 0);
        // The 24 opened cells and the mine that got its flag.
        QCOMPARE(changes.size(), 25);
    }

    void openingStopsAtNumbers()
    {
        // A wall of mines down the middle column keeps the two halves apart.
        Board board(5, 5, 1);
        board.layMines({2, 7, 12, 17, 22});
        board.reveal(0);
        QCOMPARE(board.state(), Board::Playing);
        QCOMPARE(board.openCount(), 10);
        QCOMPARE(board.face(1), Board::Open);
        QCOMPARE(board.adjacent(1), 2);
        QCOMPARE(board.face(3), Board::Hidden);
        QCOMPARE(board.face(4), Board::Hidden);
    }

    void changesCarryTheirDistance()
    {
        Board board(5, 5, 1);
        board.layMines({24});
        const auto changes = board.reveal(0);
        for (const Board::Change &change : changes)
            QCOMPARE(change.distance, std::max(change.index % 5, change.index / 5));
    }

    void flagsHoldACellShut()
    {
        Board board(5, 5, 1);
        board.layMines({24});
        QCOMPARE(board.toggleFlag(12).size(), 1);
        QCOMPARE(board.face(12), Board::Flag);
        QCOMPARE(board.minesLeft(), 0);

        // A flagged cell can't be opened directly, and the flood goes round it.
        QVERIFY(board.reveal(12).isEmpty());
        board.reveal(0);
        QCOMPARE(board.face(12), Board::Flag);
        QCOMPARE(board.state(), Board::Playing);

        board.toggleFlag(12);
        QCOMPARE(board.face(12), Board::Hidden);
        board.reveal(12);
        QCOMPARE(board.state(), Board::Won);
    }

    void flagsCanOutnumberMines()
    {
        Board board(5, 5, 1);
        board.toggleFlag(0);
        board.toggleFlag(1);
        QCOMPARE(board.minesLeft(), -1);
        QCOMPARE(board.flagCount(), 2);
    }

    void openCellsTakeNoFlag()
    {
        Board board(5, 5, 1);
        board.layMines({2, 7, 12, 17, 22});
        board.reveal(0);
        QVERIFY(board.toggleFlag(0).isEmpty());
        QCOMPARE(board.flagCount(), 0);
    }

    void openingAMineLosesTheGame()
    {
        Board board(5, 5, 1);
        board.layMines({2, 7, 12, 17, 22});
        board.reveal(0);
        board.toggleFlag(7);   // right
        board.toggleFlag(3);   // wrong
        const auto changes = board.reveal(12);

        QCOMPARE(board.state(), Board::Lost);
        QCOMPARE(board.face(12), Board::Blast);
        QCOMPARE(board.face(7), Board::Flag);
        QCOMPARE(board.face(3), Board::Misflag);
        QCOMPARE(board.face(2), Board::Mine);
        QCOMPARE(board.face(22), Board::Mine);
        QCOMPARE(board.face(4), Board::Hidden);
        // The blast, three mines shown, and the wrong flag.
        QCOMPARE(changes.size(), 5);

        // Nothing moves once the game is over.
        QVERIFY(board.reveal(4).isEmpty());
        QVERIFY(board.toggleFlag(4).isEmpty());
        QVERIFY(board.chord(1).isEmpty());
    }

    void chordOpensAroundASatisfiedNumber()
    {
        // . . . . .
        // . * . . .
        // . . . . .
        // . . . * .
        // . . . . .
        Board board(5, 5, 1);
        board.layMines({6, 18});
        board.reveal(12);
        QCOMPARE(board.face(12), Board::Open);
        QCOMPARE(board.adjacent(12), 2);
        QCOMPARE(board.openCount(), 1);

        // Not until both flags are down.
        QVERIFY(board.chord(12).isEmpty());
        board.toggleFlag(6);
        QVERIFY(board.chord(12).isEmpty());
        board.toggleFlag(18);

        const auto changes = board.chord(12);
        QVERIFY(!changes.isEmpty());
        for (int neighbour : board.neighbours(12))
            QCOMPARE(board.face(neighbour), neighbour == 6 || neighbour == 18 ? Board::Flag : Board::Open);
        // The empty areas past the two free corners open with them; the
        // cells tucked behind each mine stay covered.
        QCOMPARE(board.face(4), Board::Open);
        QCOMPARE(board.face(20), Board::Open);
        QCOMPARE(board.face(0), Board::Hidden);
        QCOMPARE(board.face(24), Board::Hidden);
        QCOMPARE(board.state(), Board::Playing);
    }

    void chordOnAWrongFlagOpensTheMine()
    {
        Board board(5, 5, 1);
        board.layMines({6, 18});
        board.reveal(12);
        board.toggleFlag(6);
        board.toggleFlag(7);   // wrong: the mine is at 18

        board.chord(12);
        QCOMPARE(board.state(), Board::Lost);
        QCOMPARE(board.face(18), Board::Blast);
        QCOMPARE(board.face(7), Board::Misflag);
        QCOMPARE(board.face(6), Board::Flag);
        // The safe cells around the number were opened all the same.
        QCOMPARE(board.face(8), Board::Open);
    }

    void chordNeedsAnOpenNumber()
    {
        Board board(5, 5, 1);
        board.layMines({24});
        QVERIFY(board.chord(0).isEmpty());   // nothing open yet
        board.layMines({2, 7, 12, 17, 22});
        board.reveal(0);
        QVERIFY(board.chord(0).isEmpty());   // an empty cell, not a number
        QVERIFY(board.chord(3).isEmpty());   // still covered
        QVERIFY(board.chord(-1).isEmpty());
        QVERIFY(board.chord(99).isEmpty());
    }

    void winningFlagsWhatWasLeft()
    {
        Board board(5, 5, 1);
        board.layMines({2, 7, 12, 17, 22});
        board.reveal(0);
        board.reveal(4);
        QCOMPARE(board.state(), Board::Won);
        QCOMPARE(board.minesLeft(), 0);
        for (int mine : {2, 7, 12, 17, 22})
            QCOMPARE(board.face(mine), Board::Flag);
    }

    void restartClearsTheBoard()
    {
        Board board(5, 5, 3);
        board.reveal(12);
        board.toggleFlag(0);
        board.restart();
        QCOMPARE(board.state(), Board::Ready);
        QCOMPARE(board.openCount(), 0);
        QCOMPARE(board.flagCount(), 0);
        for (int index = 0; index < board.cellCount(); ++index) {
            QCOMPARE(board.face(index), Board::Hidden);
            QVERIFY(!board.isMine(index));
        }
    }

    // The scores

    void scoresKeepTheFastestFirst()
    {
        QTemporaryDir dir;
        Scores scores(dir.filePath("scores.json"));
        QCOMPARE(scores.best("beginner"), 0);

        const QDateTime now = QDateTime::currentDateTime();
        scores.started("beginner");
        QVERIFY(scores.won("beginner", 9000, now));
        scores.started("beginner");
        QVERIFY(!scores.won("beginner", 12000, now));
        scores.started("beginner");
        QVERIFY(scores.won("beginner", 7000, now));
        scores.started("beginner");
        QVERIFY(!scores.won("beginner", 7000, now));
        scores.started("beginner");

        const Scores::Level level = scores.level("beginner");
        QCOMPARE(level.played, 5);
        QCOMPARE(level.won, 4);
        QCOMPARE(level.times.size(), 4);
        QCOMPARE(level.times.at(0).ms, 7000);
        QCOMPARE(level.times.at(1).ms, 7000);
        QCOMPARE(level.times.at(2).ms, 9000);
        QCOMPARE(level.times.at(3).ms, 12000);
        QCOMPARE(scores.best("beginner"), 7000);
        QCOMPARE(scores.best("expert"), 0);
    }

    void scoresSurviveARestart()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("nested/scores.json");
        {
            Scores scores(path);
            scores.started("expert");
            scores.won("expert", 123456, QDateTime::currentDateTime());
            scores.started("custom-20x12-40");
            QCOMPARE(scores.problem(), Scores::None);
        }
        Scores scores(path);
        QCOMPARE(scores.problem(), Scores::None);
        QCOMPARE(scores.best("expert"), 123456);
        QCOMPARE(scores.level("expert").won, 1);
        QVERIFY(scores.level("expert").times.first().at.isValid());
        QCOMPARE(scores.level("custom-20x12-40").played, 1);
        QCOMPARE(scores.level("custom-20x12-40").won, 0);

        // Nothing is left behind by the atomic write.
        QCOMPARE(QDir(dir.filePath("nested")).entryList(QDir::Files), QStringList{"scores.json"});
    }

    void scoresKeepOnlyTheTopTen()
    {
        QTemporaryDir dir;
        Scores scores(dir.filePath("scores.json"));
        for (int game = 0; game < 15; ++game) {
            scores.started("beginner");
            scores.won("beginner", 1000 * (15 - game), QDateTime::currentDateTime());
        }
        const Scores::Level level = scores.level("beginner");
        QCOMPARE(level.times.size(), int(Scores::Kept));
        QCOMPARE(level.times.first().ms, 1000);
        QCOMPARE(level.times.last().ms, 10000);
        QCOMPARE(level.won, 15);
    }

    void unreadableScoresAreSetAside()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("scores.json");
        write(path, "{ this is not json");

        Scores scores(path);
        QCOMPARE(scores.problem(), Scores::Unreadable);
        QVERIFY(!scores.setAside().isEmpty());
        QFile aside(scores.setAside());
        QVERIFY(aside.open(QIODevice::ReadOnly));
        QCOMPARE(aside.readAll(), QByteArray("{ this is not json"));

        // Play carries on in a fresh file.
        scores.started("beginner");
        QVERIFY(scores.won("beginner", 5000, QDateTime::currentDateTime()));
        QCOMPARE(Scores(path).best("beginner"), 5000);
    }

    void unwritableScoresSaySo()
    {
        QTemporaryDir dir;
        // A file where the directory should be.
        write(dir.filePath("blocked"), "x");
        Scores scores(dir.filePath("blocked/scores.json"));
        QCOMPARE(scores.problem(), Scores::None);
        scores.started("beginner");
        QCOMPARE(scores.problem(), Scores::Unwritable);
        // The game still counts it for this session.
        QCOMPARE(scores.level("beginner").played, 1);
    }

    // The game

    void gameOpensOnBeginner()
    {
        QTemporaryDir dir;
        Game game(dir.path());
        QCOMPARE(game.level(), QString("beginner"));
        QCOMPARE(game.columns(), 9);
        QCOMPARE(game.rows(), 9);
        QCOMPARE(game.mineCount(), 10);
        QCOMPARE(game.rowCount(), 81);
        QCOMPARE(game.minesLeft(), 10);
        QVERIFY(!game.playing());
        QVERIFY(!game.over());
        QCOMPARE(game.elapsed(), 0);
        QCOMPARE(game.issue(), QString());
    }

    void gameLevels_data()
    {
        QTest::addColumn<QString>("level");
        QTest::addColumn<int>("columns");
        QTest::addColumn<int>("rows");
        QTest::addColumn<int>("mines");
        QTest::newRow("beginner") << "beginner" << 9 << 9 << 10;
        QTest::newRow("intermediate") << "intermediate" << 16 << 16 << 40;
        QTest::newRow("expert") << "expert" << 30 << 16 << 99;
        QTest::newRow("custom") << "custom" << 20 << 14 << 45;
    }

    void gameLevels()
    {
        QFETCH(QString, level);
        QFETCH(int, columns);
        QFETCH(int, rows);
        QFETCH(int, mines);

        QTemporaryDir dir;
        Game game(dir.path());
        game.setLevel(level);
        QCOMPARE(game.level(), level);
        QCOMPARE(game.columns(), columns);
        QCOMPARE(game.rows(), rows);
        QCOMPARE(game.mineCount(), mines);
        QCOMPARE(game.rowCount(), columns * rows);
    }

    void gameRemembersItsLevel()
    {
        QTemporaryDir dir;
        {
            Game game(dir.path());
            game.setCustom(12, 7, 9);
            QCOMPARE(game.level(), QString("custom"));
        }
        {
            Game game(dir.path());
            QCOMPARE(game.level(), QString("custom"));
            QCOMPARE(game.columns(), 12);
            QCOMPARE(game.rows(), 7);
            QCOMPARE(game.mineCount(), 9);
            game.setLevel("expert");
        }
        Game game(dir.path());
        QCOMPARE(game.level(), QString("expert"));
        QCOMPARE(game.customColumns(), 12);
    }

    void gamePullsCustomIntoRange()
    {
        QTemporaryDir dir;
        Game game(dir.path());
        game.setCustom(500, 1, 100000);
        QCOMPARE(game.columns(), Board::MaxColumns);
        QCOMPARE(game.rows(), Board::MinSide);
        QCOMPARE(game.mineCount(), Board::maxMines(Board::MaxColumns, Board::MinSide));
        QCOMPARE(game.maxMines(9, 9), 72);
    }

    void gameModelFollowsTheBoard()
    {
        QTemporaryDir dir;
        Game game(dir.path());
        game.setCustom(5, 5, 5);
        game.board().layMines({2, 7, 12, 17, 22});

        QSignalSpy changed(&game, &QAbstractItemModel::dataChanged);
        game.reveal(0);
        // The delays go out before the faces, so a cell knows how long to wait.
        QCOMPARE(changed.size(), 2);
        QCOMPARE(changed.at(0).at(2).value<QList<int>>(), QList<int>{Game::DelayRole});
        QVERIFY(changed.at(1).at(2).value<QList<int>>().contains(Game::FaceRole));

        QCOMPARE(game.data(game.index(0), Game::FaceRole).toInt(), int(Game::Open));
        QCOMPARE(game.data(game.index(1), Game::AdjacentRole).toInt(), 2);
        QCOMPARE(game.data(game.index(3), Game::FaceRole).toInt(), int(Game::Hidden));
        // Cells further from the click wait longer.
        QVERIFY(game.data(game.index(21), Game::DelayRole).toInt() > game.data(game.index(5), Game::DelayRole).toInt());
        QCOMPARE(game.data(game.index(0), Game::DelayRole).toInt(), 0);

        game.flag(3);
        QCOMPARE(game.data(game.index(3), Game::FaceRole).toInt(), int(Game::Flag));
        QCOMPARE(game.minesLeft(), 4);
        QCOMPARE(game.faceAt(3), int(Game::Flag));
    }

    void gameActOpensOrChords()
    {
        QTemporaryDir dir;
        Game game(dir.path());
        game.setCustom(5, 5, 2);
        game.board().layMines({6, 18});
        game.act(12);
        QCOMPARE(game.faceAt(12), int(Game::Open));
        QCOMPARE(game.faceAt(8), int(Game::Hidden));
        game.flag(6);
        game.flag(18);
        game.act(12);
        QCOMPARE(game.faceAt(8), int(Game::Open));
        QCOMPARE(game.faceAt(4), int(Game::Open));

        // On a covered cell it opens; on a flag it does nothing.
        game.act(6);
        QVERIFY(game.playing());
        game.act(0);
        QCOMPARE(game.faceAt(0), int(Game::Open));
    }

    void gameCountsPlayedAndWon()
    {
        QTemporaryDir dir;
        Game game(dir.path());
        game.setCustom(5, 5, 5);
        QCOMPARE(game.played(), 0);

        // Flags alone don't start a game.
        game.flag(3);
        QCOMPARE(game.played(), 0);
        QVERIFY(!game.playing());

        game.board().layMines({2, 7, 12, 17, 22});
        game.reveal(0);
        QVERIFY(game.playing());
        QCOMPARE(game.played(), 1);
        QCOMPARE(game.wins(), 0);
        QCOMPARE(game.best(), 0);

        QSignalSpy state(&game, &Game::stateChanged);
        game.reveal(4);
        QVERIFY(game.won());
        QVERIFY(game.over());
        QVERIFY(game.newBest());
        QCOMPARE(state.size(), 1);
        QCOMPARE(game.wins(), 1);
        // A win is never written down as no time at all.
        QCOMPARE(game.best(), std::max<qint64>(1, game.elapsed()));
        QCOMPARE(game.minesLeft(), 0);

        // An abandoned game still counts as played; a lost one isn't a win.
        game.newGame();
        QVERIFY(!game.over());
        QVERIFY(!game.newBest());
        game.board().layMines({2, 7, 12, 17, 22});
        game.reveal(0);
        game.newGame();
        game.board().layMines({2, 7, 12, 17, 22});
        game.reveal(0);
        game.reveal(2);
        QVERIFY(game.lost());
        QCOMPARE(game.played(), 3);
        QCOMPARE(game.wins(), 1);

        // And it is all still there next time.
        Game again(dir.path());
        QCOMPARE(again.played(), 3);
        QCOMPARE(again.wins(), 1);
        QCOMPARE(again.best(), game.best());
        const QVariantMap record = again.record("custom");
        QCOMPARE(record.value("played").toInt(), 3);
        QCOMPARE(record.value("times").toList().size(), 1);
        QVERIFY(!record.value("times").toList().first().toMap().value("date").toString().isEmpty());
    }

    void everyCustomBoardKeepsItsOwnTimes()
    {
        QTemporaryDir dir;
        Game game(dir.path());
        game.setCustom(5, 5, 5);
        game.board().layMines({2, 7, 12, 17, 22});
        game.reveal(0);
        game.reveal(4);
        QVERIFY(game.best() > 0);

        game.setCustom(6, 5, 5);
        QCOMPARE(game.best(), 0);
        QCOMPARE(game.played(), 0);
        game.setCustom(5, 5, 5);
        QVERIFY(game.best() > 0);
        game.setLevel("beginner");
        QCOMPARE(game.best(), 0);
    }

    void gameClockRunsOnlyInPlay()
    {
        QTemporaryDir dir;
        Game game(dir.path());
        game.setCustom(5, 5, 5);
        QTest::qWait(30);
        QCOMPARE(game.elapsed(), 0);

        game.board().layMines({2, 7, 12, 17, 22});
        game.reveal(0);
        QTest::qWait(60);
        const qint64 running = game.elapsed();
        QVERIFY(running >= 50);

        // Paused: the clock holds, the board is covered, and moves wait.
        game.setPaused(true);
        QVERIFY(game.paused());
        QVERIFY(game.covered());
        const qint64 held = game.elapsed();
        QTest::qWait(60);
        QCOMPARE(game.elapsed(), held);
        game.reveal(4);
        QVERIFY(game.playing());
        game.flag(3);
        QCOMPARE(game.minesLeft(), 5);

        game.setPaused(false);
        QVERIFY(!game.covered());
        QTest::qWait(30);
        QVERIFY(game.elapsed() > held);

        // The window going out of focus does the same.
        game.setSuspended(true);
        QVERIFY(game.covered());
        const qint64 suspended = game.elapsed();
        QTest::qWait(40);
        QCOMPARE(game.elapsed(), suspended);
        game.setSuspended(false);

        game.reveal(4);
        QVERIFY(game.won());
        const qint64 finished = game.elapsed();
        QTest::qWait(40);
        QCOMPARE(game.elapsed(), finished);
        QCOMPARE(game.best(), finished);
    }

    void pauseNeedsAGameInPlay()
    {
        QTemporaryDir dir;
        Game game(dir.path());
        game.setPaused(true);
        QVERIFY(!game.paused());
        game.setSuspended(true);
        QVERIFY(!game.covered());
    }

    void newGameClearsAPause()
    {
        QTemporaryDir dir;
        Game game(dir.path());
        game.reveal(40);
        game.setPaused(true);
        QVERIFY(game.paused());
        game.newGame();
        QVERIFY(!game.paused());
        QVERIFY(!game.playing());
        QCOMPARE(game.elapsed(), 0);
        QCOMPARE(game.faceAt(40), int(Game::Hidden));
    }

    void gameFormatsTime()
    {
        QTemporaryDir dir;
        Game game(dir.path());
        QLocale::setDefault(QLocale("en-GB"));
        QCOMPARE(game.formatTime(0, false), QString("0:00"));
        QCOMPARE(game.formatTime(59999, false), QString("0:59"));
        QCOMPARE(game.formatTime(60000, false), QString("1:00"));
        QCOMPARE(game.formatTime(67480, true), QString("1:07.4"));
        QCOMPARE(game.formatTime(3723000, false), QString("1:02:03"));
        QLocale::setDefault(QLocale("da-DK"));
        QCOMPARE(game.formatTime(67480, true), QString("1:07,4"));
        QLocale::setDefault(QLocale("en-GB"));
    }

    void gameReportsScoreTrouble()
    {
        QTemporaryDir dir;
        write(dir.filePath("scores.json"), "nope");
        Game game(dir.path());
        QVERIFY(game.issue().contains("scores.json.unreadable-"));

        QTemporaryDir other;
        write(other.filePath("blocked"), "x");
        Game stuck(other.filePath("blocked"));
        QCOMPARE(stuck.issue(), QString());
        QSignalSpy issue(&stuck, &Game::issueChanged);
        stuck.reveal(40);
        QVERIFY(stuck.issue().contains("scores.json"));
        QVERIFY(issue.size() >= 1);
    }

    // The theme

    void themeFallsBack()
    {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#FFD60A"));
        QCOMPARE(theme.accentForeground(), QString("black"));
        QVERIFY(theme.dark());
        QCOMPARE(theme.background(), QColor("#0e0e10"));
        QCOMPARE(theme.numbers().size(), 8);
    }

    void themeReadsAccent()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        write(dir.path() + "/theme/colors.toml", "accent = \"#112233\"\n");

        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#112233"));
        QCOMPARE(theme.accentForeground(), QString("white"));
        // What the file doesn't say keeps its default.
        QCOMPARE(theme.background(), QColor("#0e0e10"));
    }

    void themeReadsThePalette()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        write(dir.path() + "/theme/colors.toml",
              "mode = \"light\"\naccent = \"#1e66f5\"\nbackground = \"#eff1f5\"\nforeground = \"#4c4f69\"\n"
              "red = \"#d20f39\"\nblue = \"#1e66f5\"\n");

        Theme theme(dir.path());
        QVERIFY(!theme.dark());
        QCOMPARE(theme.background(), QColor("#eff1f5"));
        QCOMPARE(theme.foreground(), QColor("#4c4f69"));
        QCOMPARE(theme.numbers().at(0).value<QColor>(), QColor("#1e66f5"));
        QCOMPARE(theme.danger(), QColor("#d20f39"));
        // On a light theme the covered tiles are the darker ones.
        QVERIFY(theme.tile().lightnessF() < theme.well().lightnessF());
    }

    // Whatever a theme's palette, every number can be read on an open cell.
    void themeKeepsNumbersReadable_data()
    {
        QTest::addColumn<QByteArray>("colors");
        QTest::newRow("all black") << QByteArray(
            "background = \"#000000\"\nforeground = \"#ffffff\"\nblue = \"#000000\"\ngreen = \"#050505\"\n"
            "red = \"#0a0a0a\"\nmagenta = \"#000000\"\nyellow = \"#000000\"\ncyan = \"#000000\"\n");
        QTest::newRow("all white") << QByteArray(
            "mode = \"light\"\nbackground = \"#ffffff\"\nforeground = \"#000000\"\nblue = \"#ffffff\"\n"
            "green = \"#fafafa\"\nred = \"#f5f5f5\"\nmagenta = \"#ffffff\"\nyellow = \"#ffffff\"\ncyan = \"#ffffff\"\n");
        QTest::newRow("nothing given") << QByteArray("");
    }

    void themeKeepsNumbersReadable()
    {
        QFETCH(QByteArray, colors);
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        write(dir.path() + "/theme/colors.toml", colors);

        Theme theme(dir.path());
        const QVariantList numbers = theme.numbers();
        QCOMPARE(numbers.size(), 8);
        for (const QVariant &number : numbers)
            QVERIFY(Theme::contrast(number.value<QColor>(), theme.well()) >= 3.2);
        QVERIFY(Theme::contrast(theme.danger(), theme.well()) >= 3.2);
    }

    void themeFollowsAChangeOfTheme()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        write(dir.path() + "/theme/colors.toml", "accent = \"#112233\"\n");

        Theme theme(dir.path());
        QSignalSpy changed(&theme, &Theme::changed);
        write(dir.path() + "/theme/colors.toml", "accent = \"#445566\"\nbackground = \"#101010\"\n");
        QTRY_COMPARE(theme.accent(), QString("#445566"));
        QCOMPARE(theme.background(), QColor("#101010"));
        QVERIFY(changed.size() >= 1);
    }

    void themeFollowsHyprlandRounding()
    {
        QTemporaryDir dir;
        const QString response = dir.filePath("response.json"), command = dir.filePath("hyprctl");
        write(command, "#!/bin/sh\ncat '" + response.toUtf8() + "'\n");
        QVERIFY(QFile::setPermissions(command, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        write(response, "{\"int\":0}");

        Theme theme(dir.path(), nullptr, command);
        QCOMPARE(theme.radius(), 0);
        write(response, "{\"int\":6}");
        QTRY_COMPARE(theme.radius(), 6);
    }

    // The translator

    void translatorFallsBackToEnglish()
    {
        QTemporaryDir dir;
        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("en"));
        QVERIFY(translator.translate("Main", "Save").isNull());
    }

    void translatorFollowsTheLanguageFile()
    {
        QTemporaryDir dir;
        write(dir.path() + "/language", "pt-BR\n");
        write(dir.path() + "/pt.json", R"({"Save": "Guardar", "Open": "Abrir"})");
        write(dir.path() + "/pt-BR.json", R"({"Save": "Salvar"})");

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("pt-BR"));
        QCOMPARE(translator.translate("Main", "Save"), QString("Salvar"));
        QCOMPARE(translator.translate("Main", "Open"), QString("Abrir"));
        QVERIFY(translator.translate("Main", "Quit").isNull());
    }

    void translatorPicksPluralForms()
    {
        QTemporaryDir dir;
        write(dir.path() + "/language", "ru\n");
        write(dir.path() + "/en.json", R"({"%n clips": {"one": "%n clip", "other": "%n clips"}})");
        write(dir.path() + "/ru.json", R"({"%n clips": {"one": "%n клип", "few": "%n клипа", "many": "%n клипов", "other": "%n клипа"}})");

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 21), QString("%n клип"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 22), QString("%n клипа"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 25), QString("%n клипов"));
        QCOMPARE(Translator::pluralCategory("ar", 11), QString("many"));
        QCOMPARE(Translator::pluralCategory("ja", 1), QString("other"));
    }

    // Every catalog shipped in src/i18n is sound: it parses, it translates
    // only strings the app really has, and it keeps their placeholders.
    void catalogsMatchTheSources()
    {
        const QDir source(QFINDTESTDATA("../src"));
        QVERIFY(source.exists());
        QString code;
        for (const QString &name : source.entryList({"*.qml", "*.cpp"}, QDir::Files)) {
            QFile file(source.filePath(name));
            QVERIFY(file.open(QIODevice::ReadOnly));
            code += QString::fromUtf8(file.readAll());
        }

        const QDir catalogs(source.filePath("i18n"));
        const QStringList files = catalogs.entryList({"*.json"}, QDir::Files);
        QVERIFY(files.contains("en.json"));
        static const QRegularExpression placeholder("%(n|\\d)");
        for (const QString &name : files) {
            QFile file(catalogs.filePath(name));
            QVERIFY(file.open(QIODevice::ReadOnly));
            QJsonParseError error{};
            const QJsonObject catalog = QJsonDocument::fromJson(file.readAll(), &error).object();
            QVERIFY2(error.error == QJsonParseError::NoError, qPrintable(name + ": " + error.errorString()));
            QVERIFY2(!catalog.isEmpty(), qPrintable(name));

            for (auto it = catalog.begin(); it != catalog.end(); ++it) {
                const QString key = it.key();
                QString quoted = key;
                quoted.replace("\\", "\\\\").replace("\"", "\\\"");
                QVERIFY2(code.contains("\"" + quoted + "\""), qPrintable(name + " translates a string the app doesn't have: " + key));

                QStringList wanted;
                auto found = placeholder.globalMatch(key);
                while (found.hasNext())
                    wanted << found.next().captured(0);
                QStringList forms;
                if (it.value().isObject()) {
                    QVERIFY2(it.value().toObject().contains("other"), qPrintable(name + " has no \"other\" form for: " + key));
                    for (const QJsonValue &form : it.value().toObject())
                        forms << form.toString();
                } else {
                    forms << it.value().toString();
                }
                for (const QString &form : forms) {
                    QVERIFY2(!form.isEmpty(), qPrintable(name + " has an empty translation for: " + key));
                    for (const QString &token : wanted)
                        QVERIFY2(form.contains(token), qPrintable(name + " drops " + token + " from: " + key));
                }
            }
        }
    }

private:
    static void write(const QString &path, const QByteArray &contents)
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write(contents);
    }
};

QTEST_MAIN(MinesTests)
#include "mines_tests.moc"
