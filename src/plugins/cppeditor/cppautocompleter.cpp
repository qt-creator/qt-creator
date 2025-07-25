// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppautocompleter.h"

#include <cplusplus/MatchingText.h>

#include <texteditor/tabsettings.h>

#include <QTextBlock>
#include <QTextCursor>

#ifdef WITH_TESTS
#include "cppeditorconstants.h"
#include "cppeditorwidget.h"

#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/icodestylepreferences.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>

#include <QScopeGuard>
#include <QTest>

#endif // WITH_TESTS

namespace CppEditor::Internal {

bool CppAutoCompleter::contextAllowsAutoBrackets(const QTextCursor &cursor,
                                                 const QString &textToInsert) const
{
    const CPlusPlus::MatchingText::IsNextBlockDeeperIndented isIndented
            = [this](const QTextBlock &b) { return isNextBlockIndented(b); };
    return CPlusPlus::MatchingText::contextAllowsAutoParentheses(cursor, textToInsert, isIndented);
}

bool CppAutoCompleter::contextAllowsAutoQuotes(const QTextCursor &cursor,
                                               const QString &textToInsert) const
{
    return CPlusPlus::MatchingText::contextAllowsAutoQuotes(cursor, textToInsert);
}

bool CppAutoCompleter::contextAllowsElectricCharacters(const QTextCursor &cursor) const
{
    return CPlusPlus::MatchingText::contextAllowsElectricCharacters(cursor);
}

bool CppAutoCompleter::isInComment(const QTextCursor &cursor) const
{
    return CPlusPlus::MatchingText::isInCommentHelper(cursor);
}

bool CppAutoCompleter::isInString(const QTextCursor &cursor) const
{
    return CPlusPlus::MatchingText::stringKindAtCursor(cursor) != CPlusPlus::T_EOF_SYMBOL;
}

QString CppAutoCompleter::insertMatchingBrace(const QTextCursor &cursor, const QString &text,
                                              QChar lookAhead, bool skipChars, int *skippedChars) const
{
    return CPlusPlus::MatchingText::insertMatchingBrace(cursor, text, lookAhead,
                                                        skipChars, skippedChars);
}

QString CppAutoCompleter::insertMatchingQuote(const QTextCursor &cursor, const QString &text,
                                              QChar lookAhead, bool skipChars, int *skippedChars) const
{
    return CPlusPlus::MatchingText::insertMatchingQuote(cursor, text, lookAhead,
                                                        skipChars, skippedChars);
}

QString CppAutoCompleter::insertParagraphSeparator(const QTextCursor &cursor) const
{
    return CPlusPlus::MatchingText::insertParagraphSeparator(cursor);
}

#ifdef WITH_TESTS
namespace Tests {

enum FileContent {
    EmptyFile,
    InCComment,
    InCPPComment,
    InString,
    InBetween,
    InUnbalanced,
    NumberOfItems
};

static QString fileContent(int fileContent, QChar charToInsert)
{
    switch (fileContent) {
    case EmptyFile:    return QLatin1String("|");
    case InCComment:   return QLatin1String("/*|*/");
    case InCPPComment: return QLatin1String("// |");
    case InString:     return QLatin1String("\"|\"");
    case InBetween:
        switch (charToInsert.toLatin1()) {
        case '"': case '\'':    return charToInsert + QLatin1Char('|') + charToInsert;
        case '(': case ')':     return QLatin1String("(|)");
        case '{': case '}':     return QLatin1String("{|}");
        case '[': case ']':     return QLatin1String("[|]");
        default:                return QString();
        }
    case InUnbalanced:
        switch (charToInsert.toLatin1()) {
        case '"': case '\'':    return charToInsert + QLatin1Char('|') + charToInsert;
        case '(':               return QLatin1String("(|))");
        case ')':               return QLatin1String("((|)");
        case '{':               return QLatin1String("{|}}");
        case '}':               return QLatin1String("{{|}");
        case '[':               return QLatin1String("[|]]");
        case ']':               return QLatin1String("[[|]");
        default:                return QString();
        }
    default:                    break;
    }
    return QString();
}

static QString fileContentTestName(int fileContent)
{
    switch (fileContent) {
    case EmptyFile:     return QLatin1String("Empty File");
    case InCComment:    return QLatin1String("C Comment");
    case InCPPComment:  return QLatin1String("Cpp Comment");
    case InString:      return QLatin1String("String");
    case InBetween:     return QLatin1String("The Completing Chars");
    case InUnbalanced:  return QLatin1String("Unbalanced Matching Chars");
    }
    return QString();
}

static QString charTestName(QChar c)
{
    switch (c.toLatin1()) {
    case '\'': return QLatin1String("Quote");
    case '"' : return QLatin1String("Double Quote");
    case '(' : return QLatin1String("Open Round Brackets");
    case ')' : return QLatin1String("Closing Round Brackets");
    case '{' : return QLatin1String("Open Curly Brackets");
    case '}' : return QLatin1String("Closing Curly Brackets");
    case '[' : return QLatin1String("Open Square Brackets");
    case ']' : return QLatin1String("Closing Square Brackets");
    }
    return QString();
}

static QString charGroupTestName(QChar c)
{
    switch (c.toLatin1()) {
    case '\'':              return QLatin1String("Quotes");
    case '"' :              return QLatin1String("Double Quotes");
    case '(' : case ')' :   return QLatin1String("Round Brackets");
    case '{' : case '}' :   return QLatin1String("Curly Brackets");
    case '[' : case ']' :   return QLatin1String("Square Brackets");
    }
    return QString();
}

static bool isOpeningChar(QChar c)
{
    return QString(QLatin1String("\"'({[")).contains(c);
}

static bool isClosingChar(QChar c)
{
    return QString(QLatin1String("\"')}]")).contains(c);
}

static QChar closingChar(QChar c)
{
    switch (c.toLatin1()) {
    case '\'': return QLatin1Char('\'');
    case '"' : return QLatin1Char('"');
    case '(' : return QLatin1Char(')');
    case '{' : return QLatin1Char('}');
    case '[' : return QLatin1Char(']');
    }
    return QChar();
}

static TextEditor::BaseTextEditor *creteCppEditor(const QString &text)
{
    QString name(QLatin1String("auto_complete_test"));
    Core::IEditor *editor =  Core::EditorManager::openEditorWithContents(
        Constants::CPPEDITOR_ID, &name, text.toLocal8Bit());

    return qobject_cast<TextEditor::BaseTextEditor *>(editor);
}

static QTextCursor openEditor(TextEditor::BaseTextEditor *cppEditor)
{
    QTextCursor tc;

    if (cppEditor == 0)
        return tc;
    tc = cppEditor->editorWidget()->textCursor();
    tc.movePosition(QTextCursor::Start);
    tc = tc.document()->find(QLatin1String("|"), tc);
    if (tc.isNull())
        return tc;
    tc.removeSelectedText();
    int position = tc.position();
    tc = tc.document()->find(QLatin1String("|"), tc);
    if (!tc.isNull()) {
        tc.removeSelectedText();
        tc.setPosition(position, QTextCursor::KeepAnchor);
    } else {
        tc = cppEditor->editorWidget()->textCursor();
        tc.setPosition(position);
    }
    return tc;
}

void AutoCompleterTest::testAutoComplete_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("textToInsert");
    QTest::addColumn<QString>("expectedText");
    QTest::addColumn<int>("expectedSkippedChars");

    const QString charsToInsert(QLatin1String("'\"(){}[]"));
    for (int i = 0; i < charsToInsert.length(); ++i) {
        for (int fc = EmptyFile; fc < NumberOfItems; ++fc) {
            const QChar c = charsToInsert.at(i);
            const QString testName = QLatin1String("Insert ") + charTestName(c)
                    + QLatin1String(" Into ") + fileContentTestName(fc);
            QString expectedText;
            int skippedChar = 0;

            if (fc == EmptyFile && isOpeningChar(c) && c != QLatin1Char('{'))
                expectedText = closingChar(c);

            if (fc == InBetween) {
                // When we are inside the matching chars and a closing char is inserted we want
                // to skip the already present closing char instead of adding an additional one.
                if (isClosingChar(c))
                    ++skippedChar;
                // If another opening char is inserted we
                // expect the same behavior as in an empty file
                else if (isOpeningChar(c))
                    expectedText = closingChar(c);
            }

            // Inserting a double quote into a string should have the same behavior as inserting
            // it into the matching char. For all other chars we do not expect a closing char
            // to be inserted.
            if (fc == InString && c == QLatin1Char('"'))
                ++skippedChar;

            if (fc == InUnbalanced && QString(QLatin1String("\"'")).contains(c))
                ++skippedChar;

            QTest::newRow(testName.toLatin1().data())
                    << fileContent(fc, c)
                    << QString(c)
                    << expectedText
                    << skippedChar;
        }
    }
}

void AutoCompleterTest::testAutoComplete()
{
    QFETCH(QString, text);
    QFETCH(QString, textToInsert);
    QFETCH(QString, expectedText);
    QFETCH(int, expectedSkippedChars);

    QVERIFY(text.contains(QLatin1Char('|')));

    const QScopeGuard cleanup([] { Core::EditorManager::closeAllEditors(false); });

    TextEditor::BaseTextEditor *cppEditor = creteCppEditor(text);
    QVERIFY(cppEditor);
    QTRY_VERIFY(cppEditor->textDocument()->syntaxHighlighter()->syntaxHighlighterUpToDate());
    QTextCursor tc = openEditor(cppEditor);
    QTRY_VERIFY(cppEditor->textDocument()->syntaxHighlighter()->syntaxHighlighterUpToDate());

    QVERIFY(!tc.isNull());

    const QString &matchingText = CppAutoCompleter().autoComplete(tc, textToInsert,
                                                                  true /*skipChars*/);

    int skippedChars = tc.selectedText().size();

    QCOMPARE(matchingText, expectedText);
    QCOMPARE(skippedChars, expectedSkippedChars);
}

void AutoCompleterTest::testSurroundWithSelection_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("textToInsert");
    QTest::addColumn<QString>("expectedText");

    const QString charsToInsert(QLatin1String("'\"(){}[]"));
    const QString selection(QLatin1String("arg;"));
    const QString text(QLatin1String("L|%1|;"));
    for (int i = 0; i < charsToInsert.length(); ++i) {
        const QChar c = charsToInsert.at(i);
        QTest::newRow(charTestName(c).toLatin1().data())
                << text.arg(selection)
                << QString(c)
                << (isOpeningChar(c) ? selection + closingChar(c) : QString());
    }
    const QChar c(QLatin1Char('{'));
    QTest::newRow((QLatin1String("Surround Line with ") + charTestName(c)).toLatin1().data())
            << QString(QLatin1String("|%1\n|")).arg(selection)
            << QString(c)
            << QString(QChar::ParagraphSeparator
                       + selection
                       + QChar::ParagraphSeparator
                       + closingChar(c)
                       + QChar::ParagraphSeparator);

    QTest::newRow(("Surround Line Parts with " + charTestName(c)).toLatin1().data())
            << QString(QLatin1String("if (true)|%1\n%1| true;\n")).arg(selection)
            << QString(c)
            << QString(QChar::ParagraphSeparator
                       + selection
                       + QChar::ParagraphSeparator
                       + selection
                       + QChar::ParagraphSeparator
                       + closingChar(c));
}

void AutoCompleterTest::testSurroundWithSelection()
{
    QFETCH(QString, text);
    QFETCH(QString, textToInsert);
    QFETCH(QString, expectedText);

    QVERIFY(text.count(QLatin1Char('|')) == 2);

    const QScopeGuard cleanup([] { Core::EditorManager::closeAllEditors(false); });
    TextEditor::BaseTextEditor *cppEditor = creteCppEditor(text);
    QVERIFY(cppEditor);
    QTRY_VERIFY(cppEditor->textDocument()->syntaxHighlighter()->syntaxHighlighterUpToDate());
    QTextCursor tc = openEditor(cppEditor);
    QTRY_VERIFY(cppEditor->textDocument()->syntaxHighlighter()->syntaxHighlighterUpToDate());

    QVERIFY(!tc.isNull());

    const QString &matchingText = CppAutoCompleter().autoComplete(tc, textToInsert,
                                                                  true /*skipChars*/);

    QCOMPARE(matchingText, expectedText);
}

void AutoCompleterTest::testAutoBackspace_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("expectedStopHandling");

    const QString charsToInsert(QLatin1String("'\"({["));
    for (int i = 0; i < charsToInsert.length(); ++i) {
        const QChar c = charsToInsert.at(i);

        QTest::newRow((QLatin1String("Inside ") + charGroupTestName(c)).toLatin1().data())
                << fileContent(InBetween, c)
                << QString("({['\"").contains(c);
    }
}

void AutoCompleterTest::testAutoBackspace()
{
    QFETCH(QString, text);
    QFETCH(bool, expectedStopHandling);

    QVERIFY(text.contains(QLatin1Char('|')));

    const QScopeGuard cleanup([] { Core::EditorManager::closeAllEditors(false); });
    TextEditor::BaseTextEditor *cppEditor = creteCppEditor(text);
    QVERIFY(cppEditor);
    QTRY_VERIFY(cppEditor->textDocument()->syntaxHighlighter()->syntaxHighlighterUpToDate());
    QTextCursor tc = openEditor(cppEditor);
    QTRY_VERIFY(cppEditor->textDocument()->syntaxHighlighter()->syntaxHighlighterUpToDate());

    QVERIFY(!tc.isNull());

    const bool stopHandling = CppAutoCompleter().autoBackspace(tc);

    QCOMPARE(stopHandling, expectedStopHandling);
}

void AutoCompleterTest::testInsertParagraph_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("expectedBlockCount");

    QTest::newRow("After Opening Curly Braces")
            << QString(QLatin1String("{|"))
            << 1;
    QTest::newRow("Between Curly Braces")
            << QString(QLatin1String("{|}"))
            << 1;

    QString indentation(TextEditor::TextEditorSettings::codeStyle()->tabSettings().m_indentSize,
                        QChar::Space);

    QTest::newRow("Before Indented Block")
            << QString(QLatin1String("if (true) {|\n") + indentation + QLatin1String("arg;\n"))
            << 0;
    QTest::newRow("Before Unindented Block")
            << QString(QLatin1String("if (true) {|\narg;\n"))
            << 1;
}

void AutoCompleterTest::testInsertParagraph()
{
    QFETCH(QString, text);
    QFETCH(int, expectedBlockCount);

    QVERIFY(text.contains(QLatin1Char('|')));

    const QScopeGuard cleanup([] { Core::EditorManager::closeAllEditors(false); });
    TextEditor::BaseTextEditor *cppEditor = creteCppEditor(text);
    QVERIFY(cppEditor);
    QTRY_VERIFY(cppEditor->textDocument()->syntaxHighlighter()->syntaxHighlighterUpToDate());
    QTextCursor tc = openEditor(cppEditor);
    QTRY_VERIFY(cppEditor->textDocument()->syntaxHighlighter()->syntaxHighlighterUpToDate());

    QVERIFY(!tc.isNull());

    CppAutoCompleter completer = CppAutoCompleter();
    completer.setTabSettings(TextEditor::TextEditorSettings::codeStyle()->tabSettings());

    const int blockCount = completer.paragraphSeparatorAboutToBeInserted(tc);

    QCOMPARE(blockCount, expectedBlockCount);
}

} // namespace Tests
#endif // WITH_TESTS

} // namespace CppEditor::Internal
