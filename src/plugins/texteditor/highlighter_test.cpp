// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "syntaxhighlighter.h"

#include "highlighter_test.h"

#include "fontsettings.h"
#include "textdocument.h"
#include "texteditor.h"
#include "texteditorsettings.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <utils/mimeutils.h>

#include <QtTest/QtTest>

namespace TextEditor::Internal {

constexpr auto json = R"(
{
    "name": "Test",
    "scope": "source.test",
    "uuid": "test",
    "patterns": [
        {
            "match": "a",
            "name": "keyword.test"
        }
    ]
}
)";

void GenerigHighlighterTests::initTestCase()
{
    QString title = "test.json";

    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, json);
    QVERIFY(editor);
    m_editor = qobject_cast<BaseTextEditor *>(editor);
    m_editor->editorWidget()->configureGenericHighlighter(Utils::mimeTypeForName("application/json"));
    QVERIFY(m_editor);
    m_editor->textDocument()->syntaxHighlighter()->rehighlight();
}

using FormatRanges = QList<QTextLayout::FormatRange>;

QTextCharFormat toFormat(const TextStyle &style)
{
    const static FontSettings &fontSettings = TextEditorSettings::fontSettings();
    auto format = fontSettings.toTextCharFormat(style);
    if (style == C_FUNCTION)
        format.setFontWeight(QFont::Bold); // is explicitly set by the ksyntax format definition
    if (style == C_TEXT) {
        format = QTextCharFormat();
        format.setFontWeight(QFont::Bold); // is explicitly set by the ksyntax format definition
    }
    return format;
};

void GenerigHighlighterTests::testHighlight_data()
{
    QTest::addColumn<int>("blockNumber");
    QTest::addColumn<QList<QTextLayout::FormatRange>>("formatRanges");

    // clang-format off
    QTest::addRow("0:<empty block>")
        << 0
        << FormatRanges();
    QTest::addRow("1:{")
        << 1
        << FormatRanges{{0, 1, toFormat(C_FUNCTION)}};
    QTest::addRow("2:    \"name\": \"Test\",")
        << 2
        << FormatRanges{{0, 4, toFormat(C_VISUAL_WHITESPACE)},
                        {4, 6, toFormat(C_TYPE)},
                        {10, 1, toFormat(C_FUNCTION)},
                        {11, 1, toFormat(C_VISUAL_WHITESPACE)},
                        {12, 6, toFormat(C_STRING)},
                        {18, 1, toFormat(C_FUNCTION)}};
    QTest::addRow("3:    \"scope\": \"source.test\",")
        << 3
        << FormatRanges{{0, 4, toFormat(C_VISUAL_WHITESPACE)},
                        {4, 7, toFormat(C_TYPE)},
                        {11, 1, toFormat(C_FUNCTION)},
                        {12, 1, toFormat(C_VISUAL_WHITESPACE)},
                        {13, 13, toFormat(C_STRING)},
                        {26, 1, toFormat(C_FUNCTION)}};
    QTest::addRow("4:    \"uuid\": \"test\",")
        << 4
        << FormatRanges{{0, 4, toFormat(C_VISUAL_WHITESPACE)},
                        {4, 6, toFormat(C_TYPE)},
                        {10, 1, toFormat(C_FUNCTION)},
                        {11, 1, toFormat(C_VISUAL_WHITESPACE)},
                        {12, 6, toFormat(C_STRING)},
                        {18, 1, toFormat(C_FUNCTION)}};
    QTest::addRow("5:    \"patterns\": [")
        << 5
        << FormatRanges{{0, 4, toFormat(C_VISUAL_WHITESPACE)},
                        {4, 10, toFormat(C_TYPE)},
                        {14, 1, toFormat(C_FUNCTION)},
                        {15, 1, toFormat(C_VISUAL_WHITESPACE)},
                        {16, 1, toFormat(C_TEXT)}};
    QTest::addRow("6:        {")
        << 6
        << FormatRanges{{0, 8, toFormat(C_VISUAL_WHITESPACE)},
                        {8, 1, toFormat(C_FUNCTION)}};
    QTest::addRow("7:            \"match\": \"a\",")
        << 7
        << FormatRanges{{0, 12, toFormat(C_VISUAL_WHITESPACE)},
                        {12, 7, toFormat(C_TYPE)},
                        {19, 1, toFormat(C_FUNCTION)},
                        {20, 1, toFormat(C_VISUAL_WHITESPACE)},
                        {21, 3, toFormat(C_STRING)},
                        {24, 1, toFormat(C_FUNCTION)}};
    QTest::addRow("8:            \"name\": \"keyword.test\"")
        << 8
        << FormatRanges{{0, 12, toFormat(C_VISUAL_WHITESPACE)},
                        {12, 6, toFormat(C_TYPE)},
                        {18, 1, toFormat(C_FUNCTION)},
                        {19, 1, toFormat(C_VISUAL_WHITESPACE)},
                        {20, 14, toFormat(C_STRING)}};
    QTest::addRow("9:        }")
        << 9
        << FormatRanges{{0, 8, toFormat(C_VISUAL_WHITESPACE)},
                        {8, 1, toFormat(C_FUNCTION)}};
    QTest::addRow("10:    ]")
        << 10
        << FormatRanges{{0, 4, toFormat(C_VISUAL_WHITESPACE)},
                        {4, 1, toFormat(C_TEXT)}};
    QTest::addRow("11:}")
        << 11
        << FormatRanges{{0, 1, toFormat(C_FUNCTION)}};
    // clang-format on
}

void compareFormats(const QTextLayout::FormatRange &actual, const QTextLayout::FormatRange &expected)
{
    QCOMPARE(actual.start, expected.start);
    QCOMPARE(actual.length, expected.length);
    QCOMPARE(actual.format.foreground(), expected.format.foreground());
    QCOMPARE(actual.format.background(), expected.format.background());
    QCOMPARE(actual.format.fontWeight(), expected.format.fontWeight());
    QCOMPARE(actual.format.fontItalic(), expected.format.fontItalic());
    QCOMPARE(actual.format.underlineStyle(), expected.format.underlineStyle());
    QCOMPARE(actual.format.underlineColor(), expected.format.underlineColor());
}

void GenerigHighlighterTests::testHighlight()
{
    QFETCH(int, blockNumber);
    QFETCH(FormatRanges, formatRanges);

    QTextBlock block = m_editor->textDocument()->document()->findBlockByNumber(blockNumber);
    QVERIFY(block.isValid());

    const QList<QTextLayout::FormatRange> actualFormats = block.layout()->formats();
    // full hash calculation for QTextCharFormat fails so just check the important entries of format
    QCOMPARE(actualFormats.size(), formatRanges.size());
    for (int i = 0; i < formatRanges.size(); ++i)
        compareFormats(actualFormats.at(i), formatRanges.at(i));
}

void GenerigHighlighterTests::testChange()
{
    QTextBlock block = m_editor->textDocument()->document()->findBlockByNumber(10);
    QVERIFY(block.isValid());

    QTextCursor c(block);
    c.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    c.removeSelectedText();
    m_editor->textDocument()->document()->undo();

    block = m_editor->textDocument()->document()->findBlockByNumber(10);
    QVERIFY(block.isValid());

    const FormatRanges formatRanges = {{0, 4, toFormat(C_VISUAL_WHITESPACE)},
                                       {4, 1, toFormat(C_TEXT)}};
    const QList<QTextLayout::FormatRange> actualFormats = block.layout()->formats();
    // full hash calculation for QTextCharFormat fails so just check the important entries of format
    QCOMPARE(actualFormats.size(), formatRanges.size());
    for (int i = 0; i < formatRanges.size(); ++i)
        compareFormats(actualFormats.at(i), formatRanges.at(i));
}

void GenerigHighlighterTests::testPreeditText()
{
    QTextBlock block = m_editor->textDocument()->document()->findBlockByNumber(2);
    QVERIFY(block.isValid());

    block.layout()->setPreeditArea(7, "uaf");
    m_editor->textDocument()->syntaxHighlighter()->rehighlight();

    const FormatRanges formatRanges = {{0, 4, toFormat(C_VISUAL_WHITESPACE)},
                                       {4, 3, toFormat(C_TYPE)},
                                       {10, 3, toFormat(C_TYPE)},
                                       {13, 1, toFormat(C_FUNCTION)},
                                       {14, 1, toFormat(C_VISUAL_WHITESPACE)},
                                       {15, 6, toFormat(C_STRING)},
                                       {21, 1, toFormat(C_FUNCTION)}};
    const QList<QTextLayout::FormatRange> actualFormats = block.layout()->formats();
    // full hash calculation for QTextCharFormat fails so just check the important entries of format
    QCOMPARE(actualFormats.size(), formatRanges.size());
    for (int i = 0; i < formatRanges.size(); ++i)
        compareFormats(actualFormats.at(i), formatRanges.at(i));
}

void GenerigHighlighterTests::cleanupTestCase()
{
    if (m_editor)
        Core::EditorManager::closeEditors({m_editor});
    QVERIFY(Core::EditorManager::currentEditor() == nullptr);
}

} // namespace TextEditor::Internal
