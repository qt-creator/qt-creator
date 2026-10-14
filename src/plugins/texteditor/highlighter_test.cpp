// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "highlighter_test.h"

#include "fontsettings.h"
#include "fontsettingspage.h"
#include "highlighter.h"
#include "highlighterhelper.h"
#include "syntaxhighlighter.h"
#include "textdocument.h"
#include "texteditor.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/fileutils.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>

#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>

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

class GenerigHighlighterTests final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testHighlight_data();
    void testHighlight();
    void testChange();
    void testPreeditText();
    void testMarkdownColors_data();
    void testMarkdownColors();
    void testMarkdownColorScheme();
    void cleanupTestCase();

private:
    BaseTextEditor *m_editor = nullptr;
};

void GenerigHighlighterTests::initTestCase()
{
    QString title = "test.json";

    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, json);
    QVERIFY(editor);
    m_editor = qobject_cast<BaseTextEditor *>(editor);
    m_editor->editorWidget()->configureGenericHighlighter(
        Utils::mimeTypeForName(Utils::Constants::JSON_MIMETYPE));
    QVERIFY(m_editor);

    // A misspelling in the test data would add a mark of its own to the formats this
    // compares, so the test checks no spelling whatever the settings say.
    m_editor->textDocument()->syntaxHighlighter()->setSpellCheckLanguage({});
    m_editor->textDocument()->syntaxHighlighter()->rehighlight();
}

using FormatRanges = QList<QTextLayout::FormatRange>;

static QTextCharFormat toFormat(const TextStyle &style)
{
    const static FontSettingsData fontSettings = globalFontSettings().data();
    auto format = fontSettings.toTextCharFormat(style);
    if (style == C_FUNCTION)
        format.setFontWeight(QFont::Bold); // is explicitly set by the ksyntax format definition
    if (style == C_TEXT) {
        format = QTextCharFormat();
        format.setFontWeight(QFont::Bold); // is explicitly set by the ksyntax format definition
    }
    if (style == C_INFO_CONTEXT) {
        format.setUnderlineStyle(QTextCharFormat::DotLine); // explicitly set by ksyntax format
        format.setFontWeight(QFont::Bold);
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

static void compareFormats(
    const QTextLayout::FormatRange &actual, const QTextLayout::FormatRange &expected)
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

    QTRY_COMPARE(block.layout()->formats().size(), formatRanges.size());
    const QList<QTextLayout::FormatRange> actualFormats = block.layout()->formats();
    // full hash calculation for QTextCharFormat fails so just check the important entries of format
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
    QTRY_COMPARE(block.layout()->formats().size(), formatRanges.size());
    const QList<QTextLayout::FormatRange> actualFormats = block.layout()->formats();
    // full hash calculation for QTextCharFormat fails so just check the important entries of format
    for (int i = 0; i < formatRanges.size(); ++i)
        compareFormats(actualFormats.at(i), formatRanges.at(i));
}

void GenerigHighlighterTests::testPreeditText()
{
    QTextBlock block = m_editor->textDocument()->document()->findBlockByNumber(2);
    QVERIFY(block.isValid());

    QTextCursor c(block);
    c.beginEditBlock();
    block.layout()->setPreeditArea(7, "uaf");
    c.endEditBlock();

    m_editor->textDocument()->syntaxHighlighter()->rehighlight();

    const FormatRanges formatRanges = {{0, 4, toFormat(C_VISUAL_WHITESPACE)},
                                       {4, 3, toFormat(C_TYPE)},
                                       {10, 3, toFormat(C_TYPE)},
                                       {13, 1, toFormat(C_FUNCTION)},
                                       {14, 1, toFormat(C_VISUAL_WHITESPACE)},
                                       {15, 6, toFormat(C_STRING)},
                                       {21, 1, toFormat(C_FUNCTION)}};
    QTRY_COMPARE(block.layout()->formats().size(), formatRanges.size());
    const QList<QTextLayout::FormatRange> actualFormats = block.layout()->formats();
    // full hash calculation for QTextCharFormat fails so just check the important entries of format
    for (int i = 0; i < formatRanges.size(); ++i)
        compareFormats(actualFormats.at(i), formatRanges.at(i));
}

void GenerigHighlighterTests::testMarkdownColors_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("token");
    QTest::addColumn<int>("style");

    for (int level = 1; level <= 6; ++level) {
        QTest::addRow("heading-%d", level)
            << QString(level, '#') + " Heading" << QString("Heading") << int(C_MARKDOWN_HEADING);
    }
    QTest::addRow("setext-heading") << QString("Heading\n=======") << QString("=======")
                                  << int(C_MARKDOWN_HEADING);
    QTest::addRow("emphasis") << QString("*emphasis*") << QString("emphasis")
                             << int(C_MARKDOWN_EMPHASIS);
    QTest::addRow("strong") << QString("**strong**") << QString("strong") << int(C_MARKDOWN_STRONG);
    QTest::addRow("strong-emphasis") << QString("***strong***") << QString("strong")
                                    << int(C_MARKDOWN_STRONG);
    QTest::addRow("strikethrough") << QString("~~removed~~") << QString("removed")
                                  << int(C_MARKDOWN_STRIKETHROUGH);
    QTest::addRow("link-label") << QString("[label](https://example.org)") << QString("label")
                               << int(C_MARKDOWN_LINK);
    QTest::addRow("link-target") << QString("[label](https://example.org)")
                                << QString("https://example.org") << int(C_MARKDOWN_LINK);
    QTest::addRow("autolink") << QString("<https://example.org>") << QString("https://example.org")
                             << int(C_MARKDOWN_LINK);
    QTest::addRow("inline-code") << QString("`code`") << QString("code") << int(C_MARKDOWN_CODE);
    QTest::addRow("fenced-code") << QString("```\ncode\n```") << QString("code")
                                << int(C_MARKDOWN_CODE);
    QTest::addRow("blockquote") << QString("> quoted") << QString("quoted")
                               << int(C_MARKDOWN_BLOCKQUOTE);
    QTest::addRow("blockquote-highlight") << QString("> ==marked==") << QString("marked")
                                           << int(C_ERROR_CONTEXT);
    QTest::addRow("blockquote-emphasis") << QString("> *emphasis*") << QString("emphasis")
                                        << int(C_MARKDOWN_EMPHASIS);
    QTest::addRow("list-marker") << QString("- item") << QString("-") << int(C_MARKDOWN_LIST);
    QTest::addRow("numbered-list") << QString("1. item") << QString("1.") << int(C_MARKDOWN_LIST);
    QTest::addRow("horizontal-rule") << QString("\n---") << QString("---") << int(C_MARKDOWN_RULE);
    QTest::addRow("table") << QString("| first | second |\n| --- | --- |") << QString("|")
                          << int(C_MARKDOWN_TABLE);
    QTest::addRow("embedded-code") << QString("```cpp\nint value;\n```") << QString("int")
                                  << int(C_TYPE);
    QTest::addRow("plain-text") << QString("ordinary text") << QString("ordinary") << int(C_TEXT);
}

void GenerigHighlighterTests::testMarkdownColors()
{
    QFETCH(QString, text);
    QFETCH(QString, token);
    QFETCH(int, style);

    FontSettingsData settings = globalFontSettings().data();
    for (int category = C_MARKDOWN_HEADING; category <= C_MARKDOWN_TABLE; ++category) {
        Format &format = settings.formatFor(TextStyle(category));
        format.setForeground(QColor(40 + 15 * (category - C_MARKDOWN_HEADING), 40, 150));
        format.setBackground(QColor(232, 238, 240));
    }
    const ColorScheme customScheme = settings.colorScheme();
    settings.setColorScheme(customScheme);
    QTextDocument document;
    Highlighter highlighter;
    highlighter.setDocument(&document);
    highlighter.setDefinition(HighlighterHelper::definitionForName("Markdown"));
    highlighter.setFontSettings(settings);
    highlighter.setSpellCheckLanguage({});
    document.setPlainText(text);
    highlighter.rehighlight();

    const int position = text.indexOf(token);
    QVERIFY(position >= 0);
    const QTextBlock block = document.findBlock(position);
    const int offset = position - block.position();
    QTextCharFormat actual;
    for (const QTextLayout::FormatRange &range : block.layout()->formats()) {
        if (range.start <= offset && offset < range.start + range.length) {
            actual = range.format;
            break;
        }
    }
    const QTextCharFormat expected = settings.toTextCharFormat(TextStyle(style));
    QCOMPARE(actual.foreground(), style == C_TEXT ? QBrush() : expected.foreground());
    if (style == C_TEXT)
        return;
    QCOMPARE(actual.background(), expected.background());
    if (style == C_MARKDOWN_HEADING || style == C_MARKDOWN_STRONG)
        QCOMPARE(actual.fontWeight(), int(QFont::Bold));
    if (style == C_MARKDOWN_EMPHASIS || text.contains("***"))
        QVERIFY(actual.fontItalic());
    if (style == C_MARKDOWN_STRIKETHROUGH)
        QVERIFY(actual.fontStrikeOut());
    if (style == C_MARKDOWN_LINK && !text.startsWith('<'))
        QVERIFY(actual.fontUnderline());

    ColorScheme changed = settings.colorScheme();
    changed.formatFor(TextStyle(style)).setForeground(QColor(40, 150, 80));
    settings.setColorScheme(changed);
    highlighter.setFontSettings(settings);
    highlighter.rehighlight();
    for (const QTextLayout::FormatRange &range : block.layout()->formats()) {
        if (range.start <= offset && offset < range.start + range.length) {
            QCOMPARE(range.format.foreground().color(), QColor(40, 150, 80));
            return;
        }
    }
    QFAIL("No format covers the token after changing the color settings.");
}

void GenerigHighlighterTests::testMarkdownColorScheme()
{
    const bool atomicSaveDisabled = Utils::FileUtils::atomicSaveDisabled();
    const auto restoreAtomicSave = qScopeGuard([atomicSaveDisabled] {
        Utils::FileUtils::setAtomicSaveDisabled(atomicSaveDisabled);
    });
    // Test scheme serialization independently of atomic file replacement.
    Utils::FileUtils::setAtomicSaveDisabled(true);
    const FormatDescriptions descriptions = initialFormats();
    for (const FormatDescription &description : descriptions) {
        if (description.id() < C_MARKDOWN_HEADING || description.id() > C_MARKDOWN_TABLE)
            continue;
        QVERIFY(description.showControl(FormatDescription::ShowForegroundControl));
        QVERIFY(description.showControl(FormatDescription::ShowBackgroundControl));
        QVERIFY(!description.showControl(FormatDescription::ShowFontControls));
        QVERIFY(!description.showControl(FormatDescription::ShowUnderlineControl));
    }

    for (const QString &name : QStringList{"default.xml", "light-2024.xml", "dark-2024.xml",
                                          "grayscale.xml"}) {
        FontSettingsData settings;
        QVERIFY(settings.loadColorScheme(FontSettingsData::defaultSchemeFileName(name), descriptions));
        for (int style = C_MARKDOWN_HEADING; style <= C_MARKDOWN_TABLE; ++style)
            QVERIFY(settings.colorScheme().contains(TextStyle(style)));
        QCOMPARE(settings.formatFor(C_MARKDOWN_HEADING).foreground(),
                 settings.formatFor(C_TYPE).foreground());
        QCOMPARE(settings.formatFor(C_MARKDOWN_CODE).foreground(),
                 settings.formatFor(C_STRING).foreground());

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const Utils::FilePath file = Utils::FilePath::fromString(directory.filePath("markdown.xml"));
        settings.formatFor(C_MARKDOWN_HEADING).setForeground(QColor(40, 150, 80));
        QVERIFY(settings.saveColorScheme(file));
        FontSettingsData reloaded;
        QVERIFY(reloaded.loadColorScheme(file, descriptions));
        QCOMPARE(reloaded.formatFor(C_MARKDOWN_HEADING), settings.formatFor(C_MARKDOWN_HEADING));
    }
}

void GenerigHighlighterTests::cleanupTestCase()
{
    if (m_editor)
        Core::EditorManager::closeEditors({m_editor});
    QVERIFY(Core::EditorManager::currentEditor() == nullptr);
}

QObject *createGenericHighlighterTests()
{
    return new GenerigHighlighterTests;
}

} // namespace TextEditor::Internal

#include "highlighter_test.moc"
