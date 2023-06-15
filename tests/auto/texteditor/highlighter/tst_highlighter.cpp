// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef WITH_TESTS
#define WITH_TESTS
#endif

#include <texteditor/semantichighlighter.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/texteditorsettings.h>

#include <QObject>
#include <QTextBlock>
#include <QTextDocument>
#include <QtTest>

namespace TextEditor {

class tst_highlighter: public QObject
{
    Q_OBJECT

private slots:
    void init_testCase();
    void init();
    void test_setExtraAdditionalFormats();
    void test_clearExtraFormats();
    void test_incrementalApplyAdditionalFormats();
    void test_preeditText();
    void cleanup();

private:
    QTextDocument *doc = nullptr;
    SyntaxHighlighter *highlighter = nullptr;
    FontSettings fontsettings;
    QHash<int, QTextCharFormat> formatHash;
    QTextCharFormat whitespaceFormat;
};

void tst_highlighter::init_testCase()
{
    QTextCharFormat c0;
    c0.setFontItalic(true);
    formatHash[0] = c0;
    QTextCharFormat c1;
    c1.setFontOverline(true);
    formatHash[1] = c1;
}

void tst_highlighter::init()
{
    const QString text =
R"(First
Second with spaces

Last)";


    doc = new QTextDocument();
    doc->setPlainText(text);

    highlighter = new SyntaxHighlighter(doc, fontsettings);
}

static const HighlightingResults &highlightingResults()
{
    static HighlightingResults results{HighlightingResult(),
                                       HighlightingResult(1, 1, 5, 0),
                                       HighlightingResult(2, 4, 7, 0),
                                       HighlightingResult(2, 17, 5, 1),
                                       HighlightingResult(4, 1, 8, 0),
                                       HighlightingResult(6, 1, 8, 1)};
    return results;
}

void tst_highlighter::test_setExtraAdditionalFormats()
{
    QCOMPARE(doc->blockCount(), 4);

    SemanticHighlighter::setExtraAdditionalFormats(highlighter, highlightingResults(), formatHash);

    for (auto block = doc->firstBlock(); block.isValid(); block = block.next()) {
        QVERIFY(block.blockNumber() < 4);
        QVERIFY(block.layout());
        auto formats = block.layout()->formats();
        switch (block.blockNumber()) {
        case 0: // First
            QCOMPARE(formats.size(), 1);
            QCOMPARE(formats.at(0).format.fontItalic(), true);
            QCOMPARE(formats.at(0).format.fontOverline(), false);
            QCOMPARE(formats.at(0).start, 0);
            QCOMPARE(formats.at(0).length, 5);
            break;
        case 1: // Second with spaces
            QCOMPARE(formats.size(), 4);
            QCOMPARE(formats.at(0).format.fontItalic(), false);
            QCOMPARE(formats.at(0).format.fontOverline(), false);
            QCOMPARE(formats.at(0).start, 6);
            QCOMPARE(formats.at(0).length, 1);

            QCOMPARE(formats.at(1).format.fontItalic(), false);
            QCOMPARE(formats.at(1).format.fontOverline(), false);
            QCOMPARE(formats.at(1).start, 11);
            QCOMPARE(formats.at(1).length, 1);

            QCOMPARE(formats.at(2).format.fontItalic(), true);
            QCOMPARE(formats.at(2).format.fontOverline(), false);
            QCOMPARE(formats.at(2).start, 3);
            QCOMPARE(formats.at(2).length, 7);

            QCOMPARE(formats.at(3).format.fontItalic(), false);
            QCOMPARE(formats.at(3).format.fontOverline(), true);
            QCOMPARE(formats.at(3).start, 16);
            QCOMPARE(formats.at(3).length, 3);
            break;
        case 2: //
            QCOMPARE(formats.size(), 1);
            QCOMPARE(formats.at(0).format.fontItalic(), false);
            QCOMPARE(formats.at(0).format.fontOverline(), true);
            QCOMPARE(formats.at(0).start, 0);
            QCOMPARE(formats.at(0).length, 1);
            break;
        case 3: // Last
            QCOMPARE(formats.size(), 2);
            QCOMPARE(formats.at(0).format.fontItalic(), false);
            QCOMPARE(formats.at(0).format.fontOverline(), true);
            QCOMPARE(formats.at(0).start, 0);
            QCOMPARE(formats.at(0).length, 1);

            QCOMPARE(formats.at(1).format.fontItalic(), true);
            QCOMPARE(formats.at(1).format.fontOverline(), false);
            QCOMPARE(formats.at(1).start, 0);
            QCOMPARE(formats.at(1).length, 5);
            break;
        }
    }
}

void tst_highlighter::test_clearExtraFormats()
{
    QCOMPARE(doc->blockCount(), 4);

    SemanticHighlighter::setExtraAdditionalFormats(highlighter, highlightingResults(), formatHash);

    QTextBlock firstBlock = doc->findBlockByNumber(0);
    QTextBlock spacesLineBlock = doc->findBlockByNumber(1);
    QTextBlock emptyLineBlock = doc->findBlockByNumber(2);
    QTextBlock lastBlock = doc->findBlockByNumber(3);

    highlighter->clearExtraFormats(emptyLineBlock);
    QVERIFY(emptyLineBlock.layout()->formats().isEmpty());

    highlighter->clearExtraFormats(spacesLineBlock);

    auto formats = spacesLineBlock.layout()->formats();
    // the spaces are not extra formats and should remain when clearing extra formats
    QCOMPARE(formats.size(), 2);
    QCOMPARE(formats.at(0).format.fontItalic(), false);
    QCOMPARE(formats.at(0).format.fontOverline(), false);
    QCOMPARE(formats.at(0).start, 6);
    QCOMPARE(formats.at(0).length, 1);

    QCOMPARE(formats.at(1).format.fontItalic(), false);
    QCOMPARE(formats.at(1).format.fontOverline(), false);
    QCOMPARE(formats.at(1).start, 11);
    QCOMPARE(formats.at(1).length, 1);

    // first and last should be untouched
    formats = firstBlock.layout()->formats();
    QCOMPARE(formats.size(), 1);
    QCOMPARE(formats.at(0).format.fontItalic(), true);
    QCOMPARE(formats.at(0).format.fontOverline(), false);
    QCOMPARE(formats.at(0).start, 0);
    QCOMPARE(formats.at(0).length, 5);

    formats = lastBlock.layout()->formats();
    QCOMPARE(formats.size(), 2);
    QCOMPARE(formats.at(0).format.fontItalic(), false);
    QCOMPARE(formats.at(0).format.fontOverline(), true);
    QCOMPARE(formats.at(0).start, 0);
    QCOMPARE(formats.at(0).length, 1);

    QCOMPARE(formats.at(1).format.fontItalic(), true);
    QCOMPARE(formats.at(1).format.fontOverline(), false);
    QCOMPARE(formats.at(1).start, 0);
    QCOMPARE(formats.at(1).length, 5);

    highlighter->clearAllExtraFormats();

    QVERIFY(firstBlock.layout()->formats().isEmpty());
    QVERIFY(emptyLineBlock.layout()->formats().isEmpty());
    formats = spacesLineBlock.layout()->formats();

    QCOMPARE(formats.size(), 2);
    QCOMPARE(formats.at(0).format.fontItalic(), false);
    QCOMPARE(formats.at(0).format.fontOverline(), false);
    QCOMPARE(formats.at(0).start, 6);
    QCOMPARE(formats.at(0).length, 1);

    QCOMPARE(formats.at(1).format.fontItalic(), false);
    QCOMPARE(formats.at(1).format.fontOverline(), false);
    QCOMPARE(formats.at(1).start, 11);
    QCOMPARE(formats.at(1).length, 1);

    QVERIFY(lastBlock.layout()->formats().isEmpty());
}

void tst_highlighter::test_incrementalApplyAdditionalFormats()
{
    const HighlightingResults newResults{HighlightingResult(),
                                         HighlightingResult(1, 1, 5, 0),
                                         HighlightingResult(2, 4, 7, 0),
                                         HighlightingResult(4, 1, 8, 0),
                                         HighlightingResult(6, 1, 8, 1)};

    QCOMPARE(doc->blockCount(), 4);
    QTextBlock firstBlock = doc->findBlockByNumber(0);
    QTextBlock spacesLineBlock = doc->findBlockByNumber(1);
    QTextBlock emptyLineBlock = doc->findBlockByNumber(2);
    QTextBlock lastBlock = doc->findBlockByNumber(3);

    QFutureInterface<HighlightingResult> fiOld;
    fiOld.reportResults(highlightingResults());
    SemanticHighlighter::incrementalApplyExtraAdditionalFormats(highlighter,
                                                                fiOld.future(),
                                                                2,
                                                                0,
                                                                formatHash);
    auto formats = firstBlock.layout()->formats();
    QVERIFY(formats.isEmpty());

    SemanticHighlighter::incrementalApplyExtraAdditionalFormats(highlighter,
                                                                fiOld.future(),
                                                                0,
                                                                2,
                                                                formatHash);

    formats = firstBlock.layout()->formats();
    QCOMPARE(formats.size(), 1);
    QCOMPARE(formats.at(0).format.fontItalic(), true);
    QCOMPARE(formats.at(0).format.fontOverline(), false);
    QCOMPARE(formats.at(0).start, 0);
    QCOMPARE(formats.at(0).length, 5);

    formats = spacesLineBlock.layout()->formats();
    QCOMPARE(formats.size(), 2);
    QCOMPARE(formats.at(0).format.fontItalic(), false);
    QCOMPARE(formats.at(0).format.fontOverline(), false);
    QCOMPARE(formats.at(0).start, 6);
    QCOMPARE(formats.at(0).length, 1);

    QCOMPARE(formats.at(1).format.fontItalic(), false);
    QCOMPARE(formats.at(1).format.fontOverline(), false);
    QCOMPARE(formats.at(1).start, 11);
    QCOMPARE(formats.at(1).length, 1);

    SemanticHighlighter::incrementalApplyExtraAdditionalFormats(highlighter,
                                                                fiOld.future(),
                                                                3,
                                                                6,
                                                                formatHash);

    formats = firstBlock.layout()->formats();
    QCOMPARE(formats.at(0).format.fontItalic(), true);
    QCOMPARE(formats.at(0).format.fontOverline(), false);
    QCOMPARE(formats.at(0).start, 0);
    QCOMPARE(formats.at(0).length, 5);

    formats = lastBlock.layout()->formats();
    QCOMPARE(formats.size(), 2);
    QCOMPARE(formats.at(0).format.fontItalic(), false);
    QCOMPARE(formats.at(0).format.fontOverline(), true);
    QCOMPARE(formats.at(0).start, 0);
    QCOMPARE(formats.at(0).length, 1);

    QCOMPARE(formats.at(1).format.fontItalic(), true);
    QCOMPARE(formats.at(1).format.fontOverline(), false);
    QCOMPARE(formats.at(1).start, 0);
    QCOMPARE(formats.at(1).length, 5);

    QFutureInterface<HighlightingResult> fiNew;
    fiNew.reportResults(newResults);
    SemanticHighlighter::incrementalApplyExtraAdditionalFormats(highlighter,
                                                                fiNew.future(),
                                                                0,
                                                                3,
                                                                formatHash);

    // should still contain the highlight from oldResults
    formats = emptyLineBlock.layout()->formats();
    QCOMPARE(formats.size(), 1);
    QCOMPARE(formats.at(0).format.fontItalic(), false);
    QCOMPARE(formats.at(0).format.fontOverline(), true);
    QCOMPARE(formats.at(0).start, 0);
    QCOMPARE(formats.at(0).length, 1);

    SemanticHighlighter::incrementalApplyExtraAdditionalFormats(highlighter,
                                                                fiNew.future(),
                                                                3,
                                                                4,
                                                                formatHash);

    // should have no results since the new results do not contain a highlight at that position
    formats = emptyLineBlock.layout()->formats();
    QVERIFY(formats.isEmpty());

    // QTCREATORBUG-29218
    highlighter->clearAllExtraFormats();
    const HighlightingResults bug29218Results{HighlightingResult(1, 1, 2, 0),
                                              HighlightingResult(1, 3, 2, 1)};
    QFutureInterface<HighlightingResult> fi29218;
    fi29218.reportResults(bug29218Results);
    formats = firstBlock.layout()->formats();
    QVERIFY(formats.isEmpty());
    SemanticHighlighter::incrementalApplyExtraAdditionalFormats(highlighter,
                                                                fi29218.future(),
                                                                0,
                                                                1,
                                                                formatHash);
    formats = firstBlock.layout()->formats();
    QCOMPARE(formats.size(), 1);
    QCOMPARE(formats.at(0).format.fontItalic(), true);
    QCOMPARE(formats.at(0).format.fontOverline(), false);
    QCOMPARE(formats.at(0).start, 0);
    QCOMPARE(formats.at(0).length, 2);
    SemanticHighlighter::incrementalApplyExtraAdditionalFormats(highlighter,
                                                                fi29218.future(),
                                                                1,
                                                                2,
                                                                formatHash);
    formats = firstBlock.layout()->formats();
    QCOMPARE(formats.size(), 2);
    QCOMPARE(formats.at(0).format.fontItalic(), true);
    QCOMPARE(formats.at(0).format.fontOverline(), false);
    QCOMPARE(formats.at(0).start, 0);
    QCOMPARE(formats.at(0).length, 2);
    QCOMPARE(formats.at(1).format.fontItalic(), false);
    QCOMPARE(formats.at(1).format.fontOverline(), true);
    QCOMPARE(formats.at(1).start, 2);
    QCOMPARE(formats.at(1).length, 2);
}

void tst_highlighter::test_preeditText()
{
    QCOMPARE(doc->blockCount(), 4);

    QTextBlock firstBlock = doc->findBlockByNumber(0);
    firstBlock.layout()->setPreeditArea(2, "uaf");

    SemanticHighlighter::setExtraAdditionalFormats(highlighter, highlightingResults(), formatHash);

    auto formats = firstBlock.layout()->formats();
    QCOMPARE(formats.size(), 2);
    QCOMPARE(formats.at(0).format.fontItalic(), true);
    QCOMPARE(formats.at(0).format.fontOverline(), false);
    QCOMPARE(formats.at(0).start, 0);
    QCOMPARE(formats.at(0).length, 2);

    QCOMPARE(formats.at(1).format.fontItalic(), true);
    QCOMPARE(formats.at(1).format.fontOverline(), false);
    QCOMPARE(formats.at(1).start, 5);
    QCOMPARE(formats.at(1).length, 3);
}

void tst_highlighter::cleanup()
{
    delete doc;
    doc = nullptr;
    highlighter = nullptr;
}

} // namespace TextEditor

QTEST_MAIN(TextEditor::tst_highlighter)

#include "tst_highlighter.moc"
