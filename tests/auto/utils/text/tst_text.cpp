// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/textutils.h>

#include <QTextCursor>
#include <QTextDocument>
#include <QtTest>

using namespace Utils::Text;

class tst_Text : public QObject
{
    Q_OBJECT

private slots:
    void testPositionFromFileName_data();
    void testPositionFromFileName();

    void testPositionFromPositionInDocument_data();
    void testPositionFromPositionInDocument();

    void testPositionFromCursor_data();
    void testPositionFromCursor();

    void testRangeLength_data();
    void testRangeLength();
};

void tst_Text::testPositionFromFileName_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<Position>("pos");
    QTest::addColumn<int>("expectedPostfixPos");

    const QString file("/foo/bar");
    const QString fileWin("C:\\foo\\bar");

    QTest::newRow("no pos") << file << Position() << -1;
    QTest::newRow("no pos win") << fileWin << Position() << -1;

    QTest::newRow("empty:") << file + ":" << Position() << 8;
    QTest::newRow("empty: win") << fileWin + ":" << Position() << 10;

    QTest::newRow("empty+") << file + "+" << Position() << 8;
    QTest::newRow("empty+ win") << fileWin + "+" << Position() << 10;

    QTest::newRow("empty::") << file + "::" << Position() << 8;
    QTest::newRow("empty:: win") << fileWin + "::" << Position() << 10;

    QTest::newRow("empty++") << file + "++" << Position() << 8;
    QTest::newRow("empty++ win") << fileWin + "++" << Position() << 10;

    QTest::newRow("line:") << file + ":1" << Position{1, 0} << 8;
    QTest::newRow("line: win") << fileWin + ":1" << Position{1, 0} << 10;

    QTest::newRow("line+") << file + "+8" << Position{8, 0} << 8;
    QTest::newRow("line+ win") << fileWin + "+8" << Position{8, 0} << 10;

    QTest::newRow("multi digit line:") << file + ":42" << Position{42, 0} << 8;
    QTest::newRow("multi digit line: win") << fileWin + ":42" << Position{42, 0} << 10;

    QTest::newRow("multi digit line+") << file + "+1234567890" << Position{1234567890, 0} << 8;
    QTest::newRow("multi digit line+ win")
        << fileWin + "+1234567890" << Position{1234567890, 0} << 10;

    QTest::newRow("line: empty column:") << file + ":1:" << Position{1, 0} << 8;
    QTest::newRow("line: empty column: win") << fileWin + ":1:" << Position{1, 0} << 10;

    QTest::newRow("line+ empty column+") << file + "+8+" << Position{8, 0} << 8;
    QTest::newRow("line+ empty column+ win") << fileWin + "+8+" << Position{8, 0} << 10;

    QTest::newRow("line: column:") << file + ":1:2" << Position{1, 1} << 8;
    QTest::newRow("line: column: win") << fileWin + ":1:2" << Position{1, 1} << 10;

    QTest::newRow("line+ column+") << file + "+8+3" << Position{8, 2} << 8;
    QTest::newRow("line+ column+ win") << fileWin + "+8+3" << Position{8, 2} << 10;

    QTest::newRow("mixed:+") << file + ":1+2" << Position{1, 1} << 8;
    QTest::newRow("mixed:+ win") << fileWin + ":1+2" << Position{1, 1} << 10;

    QTest::newRow("mixed+:") << file + "+8:3" << Position{8, 2} << 8;
    QTest::newRow("mixed+: win") << fileWin + "+8:3" << Position{8, 2} << 10;

    QTest::newRow("garbage:") << file + ":foo" << Position() << -1;
    QTest::newRow("garbage: win") << fileWin + ":bar" << Position() << -1;

    QTest::newRow("garbage+") << file + "+snu" << Position() << -1;
    QTest::newRow("garbage+ win") << fileWin + "+snu" << Position() << -1;

    QTest::newRow("msvc(") << file + "(" << Position() << 8;
    QTest::newRow("msvc( win") << fileWin + "(" << Position() << 10;

    QTest::newRow("msvc(empty)") << file + "()" << Position() << -1;
    QTest::newRow("msvc(empty) win") << fileWin + "()" << Position() << -1;

    QTest::newRow("msvc(line") << file + "(1" << Position{1, 0} << 8;
    QTest::newRow("msvc(line win") << fileWin + "(4569871" << Position{4569871, 0} << 10;

    QTest::newRow("msvc(line)") << file + "(1)" << Position{1, 0} << 8;
    QTest::newRow("msvc(line) win") << fileWin + "(4569871)" << Position{4569871, 0} << 10;

    QTest::newRow("msvc(garbage)") << file + "(foo)" << Position() << -1;
    QTest::newRow("msvc(garbage) win") << fileWin + "(bar)" << Position() << -1;
}

void tst_Text::testPositionFromFileName()
{
    QFETCH(QString, fileName);
    QFETCH(Position, pos);
    QFETCH(int, expectedPostfixPos);

    int postfixPos = -1;
    QCOMPARE(Position::fromFileName(fileName, postfixPos), pos);
    QCOMPARE(postfixPos, expectedPostfixPos);
}

void tst_Text::testPositionFromPositionInDocument_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("documentPosition");
    QTest::addColumn<Position>("position");

    QTest::newRow("0") << QString() << 0 << Position{1, 0};
    QTest::newRow("-1") << QString() << -1 << Position();
    QTest::newRow("mid line") << QString("foo\n") << 1 << Position{1, 1};
    QTest::newRow("second line") << QString("foo\n") << 4 << Position{2, 0};
    QTest::newRow("second mid line") << QString("foo\nbar") << 5 << Position{2, 1};
    QTest::newRow("behind content") << QString("foo\nbar") << 8 << Position();
}

void tst_Text::testPositionFromPositionInDocument()
{
    QFETCH(QString, text);
    QFETCH(int, documentPosition);
    QFETCH(Position, position);

    const QTextDocument doc(text);
    QCOMPARE(Position::fromPositionInDocument(&doc, documentPosition), position);
}

void tst_Text::testPositionFromCursor_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("documentPosition");
    QTest::addColumn<Position>("position");

    QTest::newRow("0") << QString() << 0 << Position{1, 0};
    QTest::newRow("-1") << QString() << -1 << Position{};
    QTest::newRow("mid line") << QString("foo\n") << 1 << Position{1, 1};
    QTest::newRow("second line") << QString("foo\n") << 4 << Position{2, 0};
    QTest::newRow("second mid line") << QString("foo\nbar") << 5 << Position{2, 1};
    QTest::newRow("behind content") << QString("foo\nbar") << 8 << Position{1, 0};
}

void tst_Text::testPositionFromCursor()
{
    QFETCH(QString, text);
    QFETCH(int, documentPosition);
    QFETCH(Position, position);

    if (documentPosition < 0) {// test invalid Cursor {
        QCOMPARE(Position::fromCursor(QTextCursor()), position);
    } else {
        QTextDocument doc(text);
        QTextCursor cursor(&doc);
        cursor.setPosition(documentPosition);
        QCOMPARE(Position::fromCursor(cursor), position);
    }
}

void tst_Text::testRangeLength_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<Range>("range");
    QTest::addColumn<int>("length");

    QTest::newRow("empty range") << QString() << Range{{1, 0}, {1, 0}} << 0;

    QTest::newRow("range on same line") << QString() << Range{{1, 0}, {1, 1}} << 1;

    QTest::newRow("range spanning lines") << QString("foo\nbar") << Range{{1, 0}, {2, 0}} << 4;

    QTest::newRow("range spanning lines and column offset")
        << QString("foo\nbar") << Range{{1, 1}, {2, 1}} << 4;

    QTest::newRow("range spanning lines and begin column offset")
        << QString("foo\nbar") << Range{{1, 1}, {2, 0}} << 3;

    QTest::newRow("range spanning lines and end column offset")
        << QString("foo\nbar") << Range{{1, 0}, {2, 1}} << 5;

    QTest::newRow("hyper range") << QString("foo\nbar\nfoobar") << Range{{2, 1}, {3, 1}} << 4;

    QTest::newRow("flipped range") << QString() << Range{{2, 0}, {1, 0}} << -1;

    QTest::newRow("out of range") << QString() << Range{{1, 0}, {2, 0}} << -1;
}

void tst_Text::testRangeLength()
{
    QFETCH(QString, text);
    QFETCH(Range, range);
    QFETCH(int, length);

    QCOMPARE(range.length(text), length);
}

QTEST_GUILESS_MAIN(tst_Text)

#include "tst_text.moc"
