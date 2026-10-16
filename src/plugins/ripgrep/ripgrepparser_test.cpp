// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ripgrepparser.h"
#include "ripgrepparser_test.h"

#include <QFuture>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

using namespace Utils;

namespace Ripgrep::Internal {

static SearchResultItem searchResult(const QString &fileName, const QString &lineText,
                                     const Text::Range &range)
{
    SearchResultItem result;
    result.setFilePath(FilePath::fromUserInput(fileName));
    result.setLineText(lineText);
    result.setMainRange(range);
    result.setUseTextEditorFont(true);
    return result;
}

// What "rg --json" printed for the searches below, in a directory with the files
//   one.cpp:   "int a;\nint b;\nint c;\n"
//   crlf.txt:  "x\r\ny\r\nx\r\ny\r\n"
//   utf.txt:   "caf\xc3\xa9 ab\ncd \xc3\xa9\n"
// The timings in the statistics are the ones of that run.
static const char beginOne[] =
    R"({"type":"begin","data":{"path":{"text":"one.cpp"}}})" "\n";
static const char endOne[] =
    R"({"type":"end","data":{"path":{"text":"one.cpp"},"binary_offset":null,"stats":{"elapsed":{"secs":0,"nanos":374459,"human":"0.000374s"},"searches":1,"searches_with_match":1,"bytes_searched":21,"bytes_printed":228,"matched_lines":1,"matches":1}}})" "\n"
    R"({"data":{"elapsed_total":{"human":"0.004044s","nanos":4044000,"secs":0},"stats":{"bytes_printed":228,"bytes_searched":21,"elapsed":{"human":"0.000374s","nanos":374459,"secs":0},"matched_lines":1,"matches":1,"searches":1,"searches_with_match":1}},"type":"summary"})" "\n";

// rg --json -n 'int b' one.cpp
static const char singleLine[] =
    R"({"type":"match","data":{"path":{"text":"one.cpp"},"lines":{"text":"int b;\n"},"line_number":2,"absolute_offset":7,"submatches":[{"match":{"text":"int b"},"start":0,"end":5}]}})" "\n";
// rg --json --multiline 'a;\nint b' one.cpp
static const char twoLines[] =
    R"({"type":"match","data":{"path":{"text":"one.cpp"},"lines":{"text":"int a;\nint b;\n"},"line_number":1,"absolute_offset":0,"submatches":[{"match":{"text":"a;\nint b"},"start":4,"end":12}]}})" "\n";
// rg --json --multiline 'x\r?\ny' crlf.txt
static const char carriageReturns[] =
    R"({"type":"match","data":{"path":{"text":"crlf.txt"},"lines":{"text":"x\r\ny\r\nx\r\ny\r\n"},"line_number":1,"absolute_offset":0,"submatches":[{"match":{"text":"x\r\ny"},"start":0,"end":4},{"match":{"text":"x\r\ny"},"start":6,"end":10}]}})" "\n";
// rg --json --multiline 'ab\ncd' utf.txt
static const char nonAscii[] =
    "{\"type\":\"match\",\"data\":{\"path\":{\"text\":\"utf.txt\"},\"lines\":{\"text\":\"caf\xc3\xa9 ab\\ncd \xc3\xa9\\n\"},\"line_number\":1,\"absolute_offset\":0,\"submatches\":[{\"match\":{\"text\":\"ab\\ncd\"},\"start\":6,\"end\":11}]}}\n";
// rg --json --multiline 'c;\n' one.cpp
static const char endingWithLineBreak[] =
    R"({"type":"match","data":{"path":{"text":"one.cpp"},"lines":{"text":"int c;\n"},"line_number":3,"absolute_offset":14,"submatches":[{"match":{"text":"c;\n"},"start":4,"end":7}]}})" "\n";

// rg --json 'ab' latin1.txt, in a file with the line "caf\xe9 ab\n" that is not valid UTF-8
static const char notUtf8[] =
    R"({"type":"match","data":{"path":{"text":"latin1.txt"},"lines":{"bytes":"Y2Fm6SBhYgo="},"line_number":1,"absolute_offset":0,"submatches":[{"match":{"text":"ab"},"start":5,"end":7}]}})" "\n";

void OutputParserTest::test_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<SearchResultItems>("results");

    QTest::addRow("nothing") << QString() << SearchResultItems();
    QTest::addRow("singleLine")
        << QString(beginOne) + singleLine + endOne
        << SearchResultItems{searchResult("one.cpp", "int b;", Text::Range{{2, 0}, {2, 5}})};
    QTest::addRow("matchAcrossLines")
        << QString(beginOne) + twoLines + endOne
        << SearchResultItems{searchResult("one.cpp", "int a;\nint b;",
                                          Text::Range{{1, 4}, {2, 5}})};
    QTest::addRow("matchAcrossLinesWithCarriageReturns")
        << QString(carriageReturns)
        << SearchResultItems{searchResult("crlf.txt", "x\ny", Text::Range{{1, 0}, {2, 1}}),
                             searchResult("crlf.txt", "x\ny", Text::Range{{3, 0}, {4, 1}})};
    QTest::addRow("matchAcrossLinesWithNonAscii")
        << QString::fromUtf8(nonAscii)
        << SearchResultItems{searchResult("utf.txt", QString::fromUtf8("caf\xc3\xa9 ab\ncd \xc3\xa9"),
                                          Text::Range{{1, 5}, {2, 2}})};
    QTest::addRow("matchEndingWithLineBreak")
        << QString(endingWithLineBreak)
        << SearchResultItems{searchResult("one.cpp", "int c;\n", Text::Range{{3, 4}, {4, 0}})};
    QTest::addRow("linesThatAreNotUtf8")
        << QString(notUtf8)
        << SearchResultItems{searchResult("latin1.txt", QString("caf") + QChar(0xfffd) + " ab",
                                          Text::Range{{1, 5}, {1, 7}})};
    QTest::addRow("severalFiles")
        << QString(singleLine) + carriageReturns
        << SearchResultItems{searchResult("one.cpp", "int b;", Text::Range{{2, 0}, {2, 5}}),
                             searchResult("crlf.txt", "x\ny", Text::Range{{1, 0}, {2, 1}}),
                             searchResult("crlf.txt", "x\ny", Text::Range{{3, 0}, {4, 1}})};
}

void OutputParserTest::test()
{
    QFETCH(QString, input);
    QFETCH(SearchResultItems, results);

    const SearchResultItems items = Ripgrep::parse(input);
    QCOMPARE(items.size(), results.size());
    for (qsizetype i = 0; i < results.size(); ++i)
        QCOMPARE(items.at(i), results.at(i));
}

void OutputParserTest::testChunksEndingInsideALine()
{
    const QString input = QString(beginOne) + twoLines + singleLine + endOne;
    const SearchResultItems expected = Ripgrep::parse(input);
    QCOMPARE(expected.size(), 2);

    // The process hands over its output wherever the pipe happened to end
    for (qsizetype split : {1, 20, 77, 140, 150, 220, 300, 333}) {
        QPromise<void> promise;
        promise.start();
        QString remainder;
        SearchResultItems items = Ripgrep::parse(promise.future(), input.left(split), {}, &remainder);
        items.append(Ripgrep::parse(promise.future(), input.mid(split), {}, &remainder));
        QVERIFY2(items == expected, qPrintable(QString("split at %1").arg(split)));
        QVERIFY(remainder.isEmpty());
    }
}

void OutputParserTest::testRegularExpressionUserData()
{
    // Replacing a result expands the captured texts of a regular expression, so a plain text
    // search must not report any.
    QVERIFY(Ripgrep::parse(twoLines).constFirst().userData().toStringList().isEmpty());

    const QRegularExpression regExp("(a);\\nint b");
    const SearchResultItems items = Ripgrep::parse(twoLines, regExp);
    QCOMPARE(items.size(), 1);
    QCOMPARE(items.constFirst().userData().toStringList(), (QStringList{"a;\nint b", "a"}));

    // The match of a file with CRLF line endings has them in its text
    const SearchResultItems crlfItems = Ripgrep::parse(carriageReturns, QRegularExpression("(x)\\ny"));
    QCOMPARE(crlfItems.size(), 2);
    QCOMPARE(crlfItems.constFirst().userData().toStringList(), (QStringList{"x\ny", "x"}));
}

// rg --json '= ([^,]*)' a.txt b.txt, with "x = 5\n" in a.txt and "x = 5\r\n" in b.txt. ripgrep
// does not let the match go past the end of the line, where a QRegularExpression would match the
// line break, but it does take the CR.
void OutputParserTest::testCapturesStayInTheRange()
{
    const QRegularExpression regExp("= ([^,]*)");
    const QStringList inputs{
        R"({"type":"match","data":{"path":{"text":"a.txt"},"lines":{"text":"x = 5\n"},)"
        R"("line_number":1,"absolute_offset":0,)"
        R"("submatches":[{"match":{"text":"= 5"},"start":2,"end":5}]}})" "\n",
        R"({"type":"match","data":{"path":{"text":"b.txt"},"lines":{"text":"x = 5\r\n"},)"
        R"("line_number":1,"absolute_offset":0,)"
        R"("submatches":[{"match":{"text":"= 5\r"},"start":2,"end":6}]}})" "\n"};
    for (const QString &input : inputs) {
        const SearchResultItems items = Ripgrep::parse(input, regExp);
        QCOMPARE(items.size(), 1);
        QCOMPARE(items.constFirst().lineText(), QString("x = 5"));
        QCOMPARE(items.constFirst().mainRange(), (Text::Range{{1, 2}, {1, 5}}));
        QCOMPARE(items.constFirst().userData().toStringList(), (QStringList{"= 5", "5"}));
    }

    // A look-behind still sees the text before the match
    const SearchResultItems lookBehind = Ripgrep::parse(
        R"({"type":"match","data":{"path":{"text":"a.txt"},"lines":{"text":"x = 5\n"},)"
        R"("line_number":1,"absolute_offset":0,)"
        R"("submatches":[{"match":{"text":"= 5"},"start":2,"end":5}]}})" "\n",
        QRegularExpression("(?<=x )= (\\s*\\d*\\s*)"));
    QCOMPARE(lookBehind.size(), 1);
    QCOMPARE(lookBehind.constFirst().userData().toStringList(), (QStringList{"= 5", "5"}));

    // rg --json '$' b.txt matches between the CR and the LF, which is the end of the line in a
    // document
    const SearchResultItems endOfLine = Ripgrep::parse(
        R"({"type":"match","data":{"path":{"text":"b.txt"},"lines":{"text":"x = 5\r\n"},)"
        R"("line_number":1,"absolute_offset":0,)"
        R"("submatches":[{"match":{"text":""},"start":6,"end":6}]}})" "\n",
        QRegularExpression("$"));
    QCOMPARE(endOfLine.size(), 1);
    QCOMPARE(endOfLine.constFirst().lineText(), QString("x = 5"));
    QCOMPARE(endOfLine.constFirst().mainRange(), (Text::Range{{1, 5}, {1, 5}}));
    QCOMPARE(endOfLine.constFirst().userData().toStringList(), QStringList{QString()});
}

void OutputParserTest::testManyMatchesOnALongLine()
{
    // A minified file: a line of 1 MB with a match "ab" in every 50th piece "\u00e9ab", which
    // has 4 bytes and 3 characters.
    const QByteArray piece = QString("\u00e9ab").toUtf8();
    QCOMPARE(piece.size(), 4);
    const int pieceCount = 1 << 18;
    const QByteArray line = piece.repeated(pieceCount) + '\n';
    QJsonArray submatches;
    for (int i = 0; i < pieceCount; i += 50)
        submatches.append(QJsonObject{{"match", QJsonObject{{"text", "ab"}}},
                                      {"start", i * 4 + 2}, {"end", i * 4 + 4}});
    const QJsonObject data{{"path", QJsonObject{{"text", "min.js"}}},
                           {"lines", QJsonObject{{"text", QString::fromUtf8(line)}}},
                           {"line_number", 7},
                           {"absolute_offset", 0},
                           {"submatches", submatches}};
    const QJsonObject message{{"type", "match"}, {"data", data}};
    const QString input = QString::fromUtf8(QJsonDocument(message).toJson(QJsonDocument::Compact))
                          + '\n';

    const SearchResultItems items = Ripgrep::parse(input);
    QCOMPARE(items.size(), submatches.size());
    // The text of the line is clipped like the built-in engine does it, and shared by the items
    const QString lineText = items.constFirst().lineText();
    QCOMPARE(lineText.size(), 401);
    QVERIFY(lineText.endsWith(QChar(0x2026)));
    QVERIFY(QString::fromUtf8(line).startsWith(lineText.chopped(1)));
    for (qsizetype i = 0; i < items.size(); ++i) {
        const SearchResultItem &item = items.at(i);
        const int column = int(i) * 50 * 3 + 1;
        QCOMPARE(item.mainRange(), (Text::Range{{7, column}, {7, column + 2}}));
        QCOMPARE(item.lineText().constData(), lineText.constData());
    }
}

} // namespace Ripgrep::Internal
