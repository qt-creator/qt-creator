// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "silversearcherparser.h"
#include "silversearcherparser_test.h"

#include "findinfilessilversearcher.h"

#include <utils/environment.h>
#include <utils/filesearch.h>
#include <utils/qtcprocess.h>

#include <QTemporaryDir>
#include <QTest>

using namespace TextEditor;
using namespace Utils;

namespace SilverSearcher::Internal {

static SearchResultItem searchResult(const FilePath &fileName, const QString &matchingLine,
                                     int lineNumber, int matchStart, int matchLength)
{
    SearchResultItem result;
    result.setFilePath(fileName);
    result.setLineText(matchingLine);
    result.setMainRange(lineNumber, matchStart, matchLength);
    result.setUseTextEditorFont(true);
    return result;
}

// What the parser gets from Utils::Process with a Latin-1 codec: a character for each byte
static QString agOutput(const char *bytes)
{
    return QString::fromLatin1(bytes);
}

static SearchResultItem multiLineSearchResult(const FilePath &fileName, const QString &matchingLines,
                                              const Text::Range &range)
{
    SearchResultItem result;
    result.setFilePath(fileName);
    result.setLineText(matchingLines);
    result.setMainRange(range);
    result.setUseTextEditorFont(true);
    return result;
}

void OutputParserTest::test_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<SearchResultItems>("results");

    QTest::addRow("nothing") << QString("\n") << SearchResultItems();
    QTest::addRow("oneFileOneMatch")
        << QString(":/file/path/to/filename.h\n"
                   "1;1 5: match\n")
        << SearchResultItems{searchResult("/file/path/to/filename.h", " match", 1, 1, 5)};
    QTest::addRow("multipleFilesWithOneMatch")
        << QString(":/file/path/to/filename1.h\n"
                   "1;1 5: match\n"
                   "\n"
                   ":/file/path/to/filename2.h\n"
                   "2;2 5:  match\n")
        << SearchResultItems{searchResult("/file/path/to/filename1.h", " match", 1, 1, 5),
                             searchResult("/file/path/to/filename2.h", "  match", 2, 2, 5)};
    QTest::addRow("oneFileMultipleMatches")
        << QString(":/file/path/to/filename.h\n"
                   "1;1 5,7 5: match match\n")
        << SearchResultItems{searchResult("/file/path/to/filename.h", " match match", 1, 1, 5),
                             searchResult("/file/path/to/filename.h", " match match", 1, 7, 5)};
    QTest::addRow("multipleFilesWithMultipleMatches")
        << QString(":/file/path/to/filename1.h\n"
                   "1;1 5,7 5: match match\n"
                   "\n"
                   ":/file/path/to/filename2.h\n"
                   "2;2 5,8 5:  match match\n")
        << SearchResultItems{searchResult("/file/path/to/filename1.h", " match match", 1, 1, 5),
                             searchResult("/file/path/to/filename1.h", " match match", 1, 7, 5),
                             searchResult("/file/path/to/filename2.h", "  match match", 2, 2, 5),
                             searchResult("/file/path/to/filename2.h", "  match match", 2, 8, 5)};
    // The following outputs were produced by ag 2.2.0 with --parallel --ackmate -s
    QTest::addRow("matchAcrossLines") // ab\ncd
        << QString(":/f/c.txt\n"
                   "2;bbb ab\n"
                   "3;4 5:cd ee\n"
                   "5;ggg ab\n"
                   "6;4 5:cd\n"
                   "8;iii ab\n"
                   "9;4 5:cd ab\n"
                   "10;10 5:cd\n")
        << SearchResultItems{multiLineSearchResult("/f/c.txt", "bbb ab\ncd ee", {{2, 4}, {3, 2}}),
                             multiLineSearchResult("/f/c.txt", "ggg ab\ncd", {{5, 4}, {6, 2}}),
                             multiLineSearchResult("/f/c.txt", "iii ab\ncd ab", {{8, 4}, {9, 2}}),
                             multiLineSearchResult("/f/c.txt", "cd ab\ncd", {{9, 3}, {10, 2}})};
    QTest::addRow("matchAcrossThreeLines") // x ab\ncd\nef
        << QString(":/f/a.txt\n"
                   "1;x ab\n"
                   "2;cd\n"
                   "3;0 10:ef g\n")
        << SearchResultItems{multiLineSearchResult("/f/a.txt", "x ab\ncd\nef g", {{1, 0}, {3, 2}})};
    QTest::addRow("matchEndingWithLineBreak") // two\nthree\n
        << QString(":/f/b.txt\n"
                   "2;two\n"
                   "3;three\n"
                   "4;0 10:four\n")
        << SearchResultItems{
               multiLineSearchResult("/f/b.txt", "two\nthree\nfour", {{2, 0}, {4, 0}})};
    QTest::addRow("overlappingMatchesAcrossLines") // b\nc
        << QString(":/f/d.txt\n"
                   "1;a b\n"
                   "2;2 3:c b\n"
                   "3;6 3:c b\n"
                   "4;10 3:c q\n"
                   "6;foo b\n"
                   "7;4 3:c zz\n")
        << SearchResultItems{multiLineSearchResult("/f/d.txt", "a b\nc b", {{1, 2}, {2, 1}}),
                             multiLineSearchResult("/f/d.txt", "c b\nc b", {{2, 2}, {3, 1}}),
                             multiLineSearchResult("/f/d.txt", "c b\nc q", {{3, 2}, {4, 1}}),
                             multiLineSearchResult("/f/d.txt", "foo b\nc zz", {{6, 4}, {7, 1}})};
    QTest::addRow("singleLineMatchesOnConsecutiveLines") // b
        << QString(":/f/d.txt\n"
                   "1;2 1:a b\n"
                   "2;2 1:c b\n"
                   "3;2 1:c b\n"
                   "6;4 1:foo b\n")
        << SearchResultItems{searchResult("/f/d.txt", "a b", 1, 2, 1),
                             searchResult("/f/d.txt", "c b", 2, 2, 1),
                             searchResult("/f/d.txt", "c b", 3, 2, 1),
                             searchResult("/f/d.txt", "foo b", 6, 4, 1)};
    QTest::addRow("singleAndMultiLineMatchOnTheSameLine") // o b\nc|zz
        << QString(":/f/d.txt\n"
                   "6;foo b\n"
                   "7;2 5,8 2:c zz\n")
        << SearchResultItems{multiLineSearchResult("/f/d.txt", "foo b\nc zz", {{6, 2}, {7, 1}}),
                             searchResult("/f/d.txt", "c zz", 7, 2, 2)};
    QTest::addRow("singleLineMatchThenMatchAcrossLines") // c b\nc q|a
        << QString(":/f/d.txt\n"
                   "1;0 1:a b\n"
                   "3;c b\n"
                   "4;0 7:c q\n"
                   "5;1 1:last\n")
        << SearchResultItems{searchResult("/f/d.txt", "a b", 1, 0, 1),
                             multiLineSearchResult("/f/d.txt", "c b\nc q", {{3, 0}, {4, 3}}),
                             searchResult("/f/d.txt", "last", 5, 1, 1)};
    QTest::addRow("matchAcrossLinesWithCarriageReturns") // ab\r\ncd
        << QString(":/f/e.txt\n"
                   "1;ab\r\n"
                   "2;0 6:cd\r\n")
        << SearchResultItems{multiLineSearchResult("/f/e.txt", "ab\r\ncd\r", {{1, 0}, {2, 2}})};
    QTest::addRow("matchAcrossLinesWithNonAscii") // \xc3\xa9 ab\ncd
        << agOutput(":/f/f.txt\n"
                    "1;\xc3\xa9 ab\n"
                    "2;3 5:cd\n")
        << SearchResultItems{multiLineSearchResult("/f/f.txt", QString::fromUtf8("\xc3\xa9 ab\ncd"),
                                                   {{1, 2}, {2, 2}})};
    // A character outside the BMP is four bytes in UTF-8 and two characters in a QString
    QTest::addRow("matchAcrossLinesWithFourByteCharacters")
        << agOutput(":/f/f.txt\n"
                    "1;\xf0\x9f\x98\x80 ab\n"
                    "2;5 5:cd\n")
        << SearchResultItems{multiLineSearchResult("/f/f.txt",
                                                   QString::fromUtf8("\xf0\x9f\x98\x80 ab\ncd"),
                                                   {{1, 3}, {2, 2}})};
    // A file that is not UTF-8, such as one in Latin-1. Each byte that is not valid UTF-8 is
    // decoded as a replacement character.
    QTest::addRow("invalidUtf8") // foo
        << agOutput(":/f/l.txt\n"
                    "1;5 3:// \xe9 foo\n")
        << SearchResultItems{searchResult("/f/l.txt", QString::fromUtf8("// \xe9 foo"), 1, 5, 3)};
    // A replacement character in a UTF-8 file is three bytes, unlike the one for a Latin-1 byte
    QTest::addRow("replacementCharacter") // foo
        << agOutput(":/f/r.txt\n"
                    "1;7 3:// \xef\xbf\xbd foo\n")
        << SearchResultItems{searchResult("/f/r.txt", QString::fromUtf8("// \xef\xbf\xbd foo"),
                                          1, 5, 3)};
    QTest::addRow("nonAsciiFilePath")
        << agOutput(":/f/\xc3\xa9.txt\n"
                    "1;1 5: match\n")
        << SearchResultItems{
               searchResult(FilePath::fromString(QString::fromUtf8("/f/\xc3\xa9.txt")),
                            " match", 1, 1, 5)};
    QTest::addRow("matchAcrossLinesWithInvalidUtf8") // ab\ncd
        << agOutput(":/f/l.txt\n"
                    "1;\xe9\xe9 ab\n"
                    "2;3 5:cd\n")
        << SearchResultItems{multiLineSearchResult("/f/l.txt", QString::fromUtf8("\xe9\xe9 ab\ncd"),
                                                   {{1, 3}, {2, 2}})};
    QTest::addRow("hitBeyondTheEndOfItsLine")
        << QString(":/f/a.txt\n"
                   "1;2 50:ab\n"
                   "2;0 50:cd\n")
        << SearchResultItems{searchResult("/f/a.txt", "ab", 1, 2, 0),
                             searchResult("/f/a.txt", "cd", 2, 0, 2)};
    QTest::addRow("matchAcrossLinesInTwoFiles")
        << QString(":/f/a.txt\n"
                   "1;ab\n"
                   "2;0 5:cd\n"
                   "\n"
                   ":/f/b.txt\n"
                   "5;xab\n"
                   "6;1 5:cd\n")
        << SearchResultItems{multiLineSearchResult("/f/a.txt", "ab\ncd", {{1, 0}, {2, 2}}),
                             multiLineSearchResult("/f/b.txt", "xab\ncd", {{5, 1}, {6, 2}})};
    QTest::addRow("matchAcrossLinesWithoutTheFollowingLines")
        << QString(":/file/path/to/filename.h\n"
                   "1;1 8:xab\n")
        << SearchResultItems{searchResult("/file/path/to/filename.h", "xab", 1, 1, 2)};
    QTest::addRow("followingLineIsNotConsecutive")
        << QString(":/file/path/to/filename.h\n"
                   "1;1 8:xab\n"
                   "5:cd\n")
        << SearchResultItems{searchResult("/file/path/to/filename.h", "xab", 1, 1, 2)};
}

void OutputParserTest::test()
{
    QFETCH(QString, input);
    QFETCH(SearchResultItems, results);
    const SearchResultItems items = SilverSearcher::parse(input);
    QCOMPARE(items, results);
}

static SearchResultItems parseInChunks(const QString &input, const QList<qsizetype> &splits)
{
    QPromise<void> promise;
    promise.start();
    ParserState state;
    SearchResultItems items;
    qsizetype begin = 0;
    for (const qsizetype split : splits) {
        items.append(SilverSearcher::parse(promise.future(), input.mid(begin, split - begin), {},
                                           &state));
        begin = split;
    }
    items.append(SilverSearcher::parse(promise.future(), input.mid(begin), {}, &state));
    return items;
}

void OutputParserTest::testChunksEndingInsideALine()
{
    const QStringList inputs{
        QString(":/f/c.txt\n2;bbb ab\n3;4 5:cd ee\n5;ggg ab\n6;4 5:cd\n8;iii ab\n9;4 5:cd ab\n"
                "10;10 5:cd\n\n:/f/d.txt\n1;0 3:a b\n"),
        QString(":/f/e.txt\n1;ab\n2;cd\n3;0 9:x\n"),
        agOutput(":/f/f.txt\n1;\xc3\xa9 ab\n2;3 5:cd\n")};
    for (const QString &input : inputs) {
        const SearchResultItems expected = SilverSearcher::parse(input);
        QVERIFY(expected.size() >= 1);
        // The process hands over its output wherever the pipe happened to end
        for (qsizetype split = 0; split <= input.size(); ++split) {
            QVERIFY2(parseInChunks(input, {split}) == expected,
                     qPrintable(QString("split at %1 of %2").arg(split).arg(input.left(12))));
        }
        for (qsizetype split = 0; split < input.size(); ++split)
            QVERIFY(parseInChunks(input, {split, split + 1, split + 2}) == expected);
    }
}

static SearchResultItems parseSearchWithUserData(const QString &input, const QString &regExp,
                                                 const QString &plainText = {})
{
    QPromise<void> promise;
    promise.start();
    ParserState state;
    std::optional<QRegularExpression> regularExpression;
    if (!regExp.isEmpty())
        regularExpression = QRegularExpression(regExp);
    else
        state.plainText = plainText;
    return SilverSearcher::parse(promise.future(), input, regularExpression, &state);
}

static SearchResultItems parseSearch(const QString &input, const QString &regExp,
                                     const QString &plainText = {})
{
    SearchResultItems items = parseSearchWithUserData(input, regExp, plainText);
    for (SearchResultItem &item : items)
        item.setUserData({});
    return items;
}

// Utils::Process turns CRLF into LF but ag counts two bytes for it in the length of a hit. The
// outputs are those of ag 2.2.0 on "ab\r\ncdx\r\n" and "ab\r\ncd\r\nxyz\r\n".
void OutputParserTest::testCarriageReturns()
{
    const FilePath file = "/f/g.txt";
    const QString normalized = ":/f/g.txt\n1;ab\n2;0 6:cdx\n";
    const SearchResultItems cdx{multiLineSearchResult(file, "ab\ncdx", {{1, 0}, {2, 2}})};
    QCOMPARE(parseSearch(normalized, "ab\\Rcd"), cdx);
    QCOMPARE(parseSearch(normalized, "ab\\s+cd"), cdx);
    QCOMPARE(parseSearch(normalized, "ab\r\ncd"), cdx); // The pattern names the CR
    QCOMPARE(parseSearch(normalized, {}, "ab\r\ncd"), cdx);
    // Not what Utils::Process delivers, but what parse() can be fed
    QCOMPARE(parseSearch(":/f/g.txt\n1;ab\r\n2;0 6:cdx\r\n", "ab\\Rcd"),
             (SearchResultItems{multiLineSearchResult(file, "ab\r\ncdx\r", {{1, 0}, {2, 2}})}));

    // The regular expression is matched against the text with the CR, but the captured texts
    // have the line feeds of the document they are replaced in
    const SearchResultItems captured = parseSearchWithUserData(normalized, "(ab)\r\n(cd)");
    QCOMPARE(captured.size(), 1);
    QCOMPARE(captured.first().mainRange(), cdx.first().mainRange());
    QCOMPARE(captured.first().userData().toStringList(), (QStringList{"ab\ncd", "ab", "cd"}));
    for (const QString &pattern : {QString("(ab\\s+cd)"), QString("(ab\\Rcd)")}) {
        const SearchResultItems acrossBreak = parseSearchWithUserData(normalized, pattern);
        QCOMPARE(acrossBreak.size(), 1);
        QCOMPARE(acrossBreak.first().mainRange(), cdx.first().mainRange());
        QCOMPARE(acrossBreak.first().userData().toStringList(), (QStringList{"ab\ncd", "ab\ncd"}));
    }

    QCOMPARE(parseSearch(":/f/h.txt\n1;ab\n2;cd\n3;0 9:xyz\n", "(?s)ab.*x"),
             (SearchResultItems{multiLineSearchResult("/f/h.txt", "ab\ncd\nxyz",
                                                      {{1, 0}, {3, 1}})}));

    // A file with LF line breaks is not mistaken for one with CRLF
    QCOMPARE(parseSearch(":/f/g.txt\n1;ab\n2;0 5:cdx\n", "ab\\Rcd"), cdx);
    QCOMPARE(parseSearch(":/f/g.txt\n1;ab\n2;0 5:cdx\n", {}, "ab\ncd"), cdx);
    QCOMPARE(parseSearch(":/f/g.txt\n1;AB\n2;0 5:cdx\n", {}, "ab\ncd"),
             (SearchResultItems{multiLineSearchResult(file, "AB\ncdx", {{1, 0}, {2, 2}})}));
}

void OutputParserTest::testOnlyHitOnItsLineIsNotConfirmed()
{
    QPromise<void> promise;
    promise.start();
    ParserState state;
    const QRegularExpression regExp("ab");
    const SearchResultItems items = SilverSearcher::parse(promise.future(),
                                                          ":/f/a.txt\n1;0 2:ab\n", regExp, &state);
    QCOMPARE(items.size(), 1);
    QVERIFY(!state.wholeMatch); // Not built, so not used for every hit

    SilverSearcher::parse(promise.future(), "3;ab\n4;0 5:cd\n5;ab\n6;0 5:cd\n", regExp, &state);
    QVERIFY(state.wholeMatch);
}

void OutputParserTest::testWholeMatchIsReused()
{
    QPromise<void> promise;
    promise.start();
    ParserState state;
    const QRegularExpression regExp("ab\\ncd");
    SilverSearcher::parse(promise.future(), ":/f/a.txt\n1;ab\n2;0 5:cd\n", regExp, &state);
    QVERIFY(state.wholeMatch);

    // Equivalent to the one that was built, but told apart by the comment
    const QRegularExpression built("\\A(?:ab\\ncd)\\z(?#built)");
    state.wholeMatch = built;
    const SearchResultItems items
        = SilverSearcher::parse(promise.future(), "4;ab\n5;0 5:cd\n", regExp, &state);
    QCOMPARE(items.size(), 1);
    QCOMPARE(items.first().mainRange(), Text::Range({{4, 0}, {5, 2}}));
    QVERIFY(state.wholeMatch);
    QCOMPARE(state.wholeMatch->pattern(), built.pattern());
}

void OutputParserTest::testHitCountedFromEarlierLine()
{
    // The column of the hit on line 10 counts from line 8. It also fits in line 10, which is long.
    const SearchResultItems items = parseSearch(":/f/c.txt\n"
                                                "8;iii ab\n"
                                                "9;4 5:cd ab\n"
                                                "10;10 5:cd and more text\n",
                                                {}, "ab\ncd");
    QCOMPARE(items,
             (SearchResultItems{
                 multiLineSearchResult("/f/c.txt", "iii ab\ncd ab", {{8, 4}, {9, 2}}),
                 multiLineSearchResult("/f/c.txt", "cd ab\ncd and more text", {{9, 3}, {10, 2}})}));

    // The same in a file with CRLF line breaks
    QCOMPARE(parseSearch(":/f/c.txt\n8;iii ab\n9;4 6:cd ab\n10;11 6:cd and more text\n", {},
                         "ab\r\ncd"),
             items);
}

void OutputParserTest::testRunIsBounded()
{
    QPromise<void> promise;
    promise.start();
    ParserState state;
    QString input = ":/f/a.txt\n";
    const int lineCount = 5000;
    for (int i = 1; i <= lineCount; ++i)
        input += QString("%1;0 1:x\n").arg(i);
    const SearchResultItems items = SilverSearcher::parse(promise.future(), input, {}, &state);
    QCOMPARE(items.size(), lineCount);
    QVERIFY(state.run.size() <= 100);
    QVERIFY(state.starts.size() <= 100);
}

// Hits that cannot be resolved do not leave the lines that they might count from behind
void OutputParserTest::testUnresolvedHitsAreBounded()
{
    QPromise<void> promise;
    promise.start();
    ParserState state;
    QString input = ":/f/a.txt\n";
    const int lineCount = 5000;
    for (int i = 1; i <= lineCount; ++i)
        input += QString("%1;0 50:ab\n").arg(i);
    const SearchResultItems items = SilverSearcher::parse(promise.future(), input, {}, &state);
    QCOMPARE(items.size(), lineCount);
    QCOMPARE(items.last().mainRange(), Text::Range({{lineCount, 0}, {lineCount, 2}}));
    QVERIFY(state.starts.size() <= 100);
}

// Every line break is inside a match, so the columns of all hits count from the first line,
// which is no longer needed for the hits themselves.
static QString overlappingMatches(int lineCount, int lineBreakSize)
{
    QString input = ":/f/d.txt\n1;a b\n";
    for (int line = 2; line <= lineCount; ++line) {
        input += QString("%1;%2 %3:c %4\n")
                     .arg(line)
                     .arg(2 + (line - 2) * (3 + lineBreakSize))
                     .arg(2 + lineBreakSize)
                     .arg(line < lineCount ? "b" : "q");
    }
    return input;
}

void OutputParserTest::testLongRunOfMatchesAcrossLines()
{
    const int lineCount = 250;
    const QList<SearchResultItems> results{
        SilverSearcher::parse(overlappingMatches(lineCount, 1)),
        parseSearch(overlappingMatches(lineCount, 1), {}, "b\nc"),
        parseSearch(overlappingMatches(lineCount, 2), {}, "b\r\nc")};
    for (const SearchResultItems &items : results) {
        QCOMPARE(items.size(), lineCount - 1);
        QCOMPARE(items.at(1).mainRange(), Text::Range({{2, 2}, {3, 1}}));
        QCOMPARE(items.last().mainRange(), Text::Range({{lineCount - 1, 2}, {lineCount, 1}}));
        QCOMPARE(items.last().lineText(), QString("c b\nc q"));
    }
}

void OutputParserTest::testLongMatch()
{
    const int lineCount = 250;
    QString input = ":/f/a.txt\n1;/* x\n";
    for (int line = 2; line < lineCount; ++line)
        input += QString("%1;x\n").arg(line);
    const int length = 5 + (lineCount - 2) * 2 + 4;
    input += QString("%1;0 %2:x */ y\n").arg(lineCount).arg(length);

    const SearchResultItems items = parseSearch(input, "(?s)/\\*.*?\\*/");
    QCOMPARE(items.size(), 1);
    QCOMPARE(items.first().mainRange(), Text::Range({{1, 0}, {lineCount, 4}}));
    QCOMPARE(items.first().lineText().count('\n'), lineCount - 1);
}

void OutputParserTest::testRegularExpressionUserData()
{
    const QRegularExpression regExp("(a(b))\\n(c)");
    const SearchResultItems items = SilverSearcher::parse(QString(":/file/path/to/filename.h\n"
                                                                   "1;xab\n"
                                                                   "2;1 5:cd\n"),
                                                          regExp);
    QCOMPARE(items.size(), 1);
    QCOMPARE(items.first().userData().toStringList(), (QStringList{"ab\nc", "ab", "b", "c"}));
}

// A term in a search that is not for a regular expression has the line feeds of the document it
// was taken from. The outputs are those of ag 2.2.0 for "ab\r?\ncd" on "ab\r\ncdx\r\n" and
// "iii ab\r\ncd ab\r\ncd and more text\r\n", from Utils::Process, which turns CRLF into LF.
void OutputParserTest::testPlainTextAcrossLinesInCrlfFile()
{
    const QString output = ":/f/g.txt\n1;ab\n2;0 6:cdx\n";
    const SearchResultItems cdx{multiLineSearchResult("/f/g.txt", "ab\ncdx", {{1, 0}, {2, 2}})};
    QCOMPARE(parseSearch(output, {}, "ab\ncd"), cdx);

    QPromise<void> promise;
    promise.start();
    ParserState state;
    state.plainText = "AB\nCD";
    state.plainTextCaseSensitivity = Qt::CaseInsensitive;
    QCOMPARE(SilverSearcher::parse(promise.future(), output, {}, &state), cdx);

    QCOMPARE(parseSearch(":/f/c.txt\n1;iii ab\n2;4 6:cd ab\n3;11 6:cd and more text\n", {},
                         "ab\ncd"),
             (SearchResultItems{
                 multiLineSearchResult("/f/c.txt", "iii ab\ncd ab", {{1, 4}, {2, 2}}),
                 multiLineSearchResult("/f/c.txt", "cd ab\ncd and more text", {{2, 3}, {3, 2}})}));
}

static FileFindParameters plainTextParameters(const QString &text, FindFlags flags = {})
{
    FileFindParameters parameters;
    parameters.text = text;
    parameters.flags = flags;
    parameters.nameFilters = {"*"};
    parameters.searchDir = FilePath::fromString("/dir");
    return parameters;
}

// The search term, which follows the "--"
static QString termArgument(const QStringList &arguments)
{
    const qsizetype separator = arguments.indexOf("--");
    return separator >= 0 ? arguments.value(separator + 1) : QString();
}

void OutputParserTest::testArguments()
{
    // A line break of a plain text term matches the CRLF of a file with Windows line endings
    const QStringList acrossLines = silverSearcherArguments(plainTextParameters("a.b(\n)c"));
    QVERIFY2(!acrossLines.contains("-Q"), qPrintable(acrossLines.join(' ')));
    QCOMPARE(termArgument(acrossLines), QString("a\\.b\\(\\r?\\n\\)c"));
    QCOMPARE(silverSearcherArguments(plainTextParameters("a.b(\r\n)c")), acrossLines);

    const QStringList oneLine = silverSearcherArguments(plainTextParameters("a.b("));
    QVERIFY2(oneLine.contains("-Q"), qPrintable(oneLine.join(' ')));
    QCOMPARE(termArgument(oneLine), QString("a.b("));

    // A regular expression is given as it is
    const QStringList regExp = silverSearcherArguments(
        plainTextParameters("a.\\nb", FindRegularExpression));
    QVERIFY2(!regExp.contains("-Q"), qPrintable(regExp.join(' ')));
    QCOMPARE(termArgument(regExp), QString("a.\\nb"));
}

// "file:begin line:column-end line:column|line text", the files being in one directory
static QStringList hits(const SearchResultItems &items)
{
    QStringList result;
    for (const SearchResultItem &item : items) {
        const Text::Range range = item.mainRange();
        result << QString("%1:%2:%3-%4:%5|%6")
                      .arg(item.filePath()->fileName())
                      .arg(range.begin.line).arg(range.begin.column)
                      .arg(range.end.line).arg(range.end.column)
                      .arg(item.lineText());
    }
    result.sort();
    return result;
}

void OutputParserTest::testSearchAcrossLinesInCrlfFile()
{
    Process version;
    version.setCommand({"ag", {"--version"}});
    version.setEnvironment(Environment::systemEnvironment());
    version.start();
    using namespace std::chrono_literals;
    if (!version.waitForFinished(5s) || version.result() != ProcessResult::FinishedWithSuccess)
        QSKIP("The Silver Searcher is not available.");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto write = [&dir](const QString &name, const QByteArray &contents) {
        QFile file(dir.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(contents);
    };
    write("lf.txt", "x (a\nb) y\n");
    write("crlf.txt", "x (a\r\nb) y\r\n");
    if (QTest::currentTestFailed())
        return;

    FileFindParameters parameters = plainTextParameters("(a\nB)");
    parameters.searchDir = FilePath::fromString(dir.path());
    const QFuture<SearchResultItems> future = findWithSilverSearcher(parameters);
    QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(), 20000);
    SearchResultItems items;
    for (const SearchResultItems &chunk : future.results())
        items.append(chunk);
    QCOMPARE(hits(items),
             (QStringList{"crlf.txt:1:2-2:2|x (a\nb) y", "lf.txt:1:2-2:2|x (a\nb) y"}));
}

} // namespace SilverSearcher::Internal
