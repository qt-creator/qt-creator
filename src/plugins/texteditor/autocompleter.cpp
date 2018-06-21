/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "autocompleter.h"
#include "textdocumentlayout.h"
#include "tabsettings.h"

#include <QDebug>
#include <QTextCursor>

using namespace TextEditor;

AutoCompleter::AutoCompleter() :
    m_allowSkippingOfBlockEnd(false),
    m_autoInsertBrackets(true),
    m_surroundWithBrackets(true),
    m_autoInsertQuotes(true),
    m_surroundWithQuotes(true)
{}

AutoCompleter::~AutoCompleter()
{}

static void countBracket(QChar open, QChar close, QChar c, int *errors, int *stillopen)
{
    if (c == open)
        ++*stillopen;
    else if (c == close)
        --*stillopen;

    if (*stillopen < 0) {
        *errors += -1 * (*stillopen);
        *stillopen = 0;
    }
}

static void countBrackets(QTextCursor cursor, int from, int end, QChar open, QChar close,
                          int *errors, int *stillopen)
{
    cursor.setPosition(from);
    QTextBlock block = cursor.block();
    while (block.isValid() && block.position() < end) {
        Parentheses parenList = TextDocumentLayout::parentheses(block);
        if (!parenList.isEmpty() && !TextDocumentLayout::ifdefedOut(block)) {
            for (int i = 0; i < parenList.count(); ++i) {
                Parenthesis paren = parenList.at(i);
                int position = block.position() + paren.pos;
                if (position < from || position >= end)
                    continue;
                countBracket(open, close, paren.chr, errors, stillopen);
            }
        }
        block = block.next();
    }
}

enum class CharType { OpenChar, CloseChar };
static QChar charType(const QChar &c, CharType type)
{
    switch (c.unicode()) {
    case '(': case ')':
        return type == CharType::OpenChar ? QLatin1Char('(') : QLatin1Char(')');
    case '[': case ']':
        return type == CharType::OpenChar ? QLatin1Char('[') : QLatin1Char(']');
    case '{': case '}':
        return type == CharType::OpenChar ? QLatin1Char('{') : QLatin1Char('}');
    }
    return QChar();
}

static bool fixesBracketsError(const QString &textToInsert, const QTextCursor &cursor)
{
    const QChar character = textToInsert.at(0);
    const QString allParentheses = QLatin1String("()[]{}");
    if (!allParentheses.contains(character))
        return false;

    QTextCursor tmp = cursor;
    bool foundBlockStart = TextBlockUserData::findPreviousBlockOpenParenthesis(&tmp);
    int blockStart = foundBlockStart ? tmp.position() : 0;
    tmp = cursor;
    bool foundBlockEnd = TextBlockUserData::findNextBlockClosingParenthesis(&tmp);
    int blockEnd = foundBlockEnd ? tmp.position() : (cursor.document()->characterCount() - 1);
    const QChar openChar = charType(character, CharType::OpenChar);
    const QChar closeChar = charType(character, CharType::CloseChar);

    int errors = 0;
    int stillopen = 0;
    countBrackets(cursor, blockStart, blockEnd, openChar, closeChar, &errors, &stillopen);
    int errorsBeforeInsertion = errors + stillopen;
    errors = 0;
    stillopen = 0;
    countBrackets(cursor, blockStart, cursor.position(), openChar, closeChar, &errors, &stillopen);
    countBracket(openChar, closeChar, character, &errors, &stillopen);
    countBrackets(cursor, cursor.position(), blockEnd, openChar, closeChar, &errors, &stillopen);
    int errorsAfterInsertion = errors + stillopen;
    return errorsAfterInsertion < errorsBeforeInsertion;
}

static QString surroundSelectionWithBrackets(const QString &textToInsert, const QString &selection)
{
    QString replacement;
    if (textToInsert == QLatin1String("(")) {
        replacement = selection + QLatin1Char(')');
    } else if (textToInsert == QLatin1String("[")) {
        replacement = selection + QLatin1Char(']');
    } else if (textToInsert == QLatin1String("{")) {
        //If the text spans multiple lines, insert on different lines
        replacement = selection;
        if (selection.contains(QChar::ParagraphSeparator)) {
            //Also, try to simulate auto-indent
            replacement = (selection.startsWith(QChar::ParagraphSeparator)
                   ? QString()
                   : QString(QChar::ParagraphSeparator)) + selection;
            if (replacement.endsWith(QChar::ParagraphSeparator))
                replacement += QLatin1Char('}') + QString(QChar::ParagraphSeparator);
            else
                replacement += QString(QChar::ParagraphSeparator) + QLatin1Char('}');
        } else {
            replacement += QLatin1Char('}');
        }
    }
    return replacement;
}

bool AutoCompleter::isQuote(const QString &text)
{
    return text == QLatin1String("\"") || text == QLatin1String("'");
}

bool AutoCompleter::isNextBlockIndented(const QTextBlock &currentBlock) const
{
    QTextBlock block = currentBlock;
    int indentation = m_tabSettings.indentationColumn(block.text());

    if (block.next().isValid()) { // not the last block
        block = block.next();
        //skip all empty blocks
        while (block.isValid() && m_tabSettings.onlySpace(block.text()))
            block = block.next();
        if (block.isValid()
                && m_tabSettings.indentationColumn(block.text()) > indentation)
            return true;
    }

    return false;
}

QString AutoCompleter::replaceSelection(QTextCursor &cursor, const QString &textToInsert) const
{
    if (!cursor.hasSelection())
        return QString();
    if (isQuote(textToInsert) && m_surroundWithQuotes)
        return cursor.selectedText() + textToInsert;
    if (m_surroundWithBrackets)
        return surroundSelectionWithBrackets(textToInsert, cursor.selectedText());
    return QString();
}

QString AutoCompleter::autoComplete(QTextCursor &cursor, const QString &textToInsert,
                                    bool skipChars) const
{
    const bool checkBlockEnd = m_allowSkippingOfBlockEnd;
    m_allowSkippingOfBlockEnd = false; // consume blockEnd.

    QString autoText = replaceSelection(cursor, textToInsert);
    if (!autoText.isEmpty())
        return autoText;

    QTextDocument *doc = cursor.document();
    const QChar lookAhead = doc->characterAt(cursor.selectionEnd());

    int skippedChars = 0;

    if (isQuote(textToInsert) && m_autoInsertQuotes
            && contextAllowsAutoQuotes(cursor, textToInsert)) {
        autoText = insertMatchingQuote(cursor, textToInsert, lookAhead, skipChars, &skippedChars);
    } else if (m_autoInsertBrackets && contextAllowsAutoBrackets(cursor, textToInsert)) {
        if (fixesBracketsError(textToInsert, cursor))
            return QString();

        autoText = insertMatchingBrace(cursor, textToInsert, lookAhead, skipChars, &skippedChars);

        if (checkBlockEnd && textToInsert.at(0) == QLatin1Char('}')) {
            if (textToInsert.length() > 1)
                qWarning() << "*** handle event compression";

            int startPos = cursor.selectionEnd(), pos = startPos;
            while (doc->characterAt(pos).isSpace())
                ++pos;

            if (doc->characterAt(pos) == QLatin1Char('}') && skipChars)
                skippedChars += (pos - startPos) + 1;
        }
    } else {
        return QString();
    }

    if (skipChars && skippedChars) {
        const int pos = cursor.position();
        cursor.setPosition(pos + skippedChars);
        cursor.setPosition(pos, QTextCursor::KeepAnchor);
    }

    return autoText;
}

bool AutoCompleter::autoBackspace(QTextCursor &cursor)
{
    m_allowSkippingOfBlockEnd = false;

    if (!m_autoInsertBrackets)
        return false;

    int pos = cursor.position();
    if (pos == 0)
        return false;
    QTextCursor c = cursor;
    c.setPosition(pos - 1);

    QTextDocument *doc = cursor.document();
    const QChar lookAhead = doc->characterAt(pos);
    const QChar lookBehind = doc->characterAt(pos - 1);
    const QChar lookFurtherBehind = doc->characterAt(pos - 2);

    const QChar character = lookBehind;
    if (character == QLatin1Char('(')
            || character == QLatin1Char('[')
            || character == QLatin1Char('{')) {
        QTextCursor tmp = cursor;
        TextBlockUserData::findPreviousBlockOpenParenthesis(&tmp);
        int blockStart = tmp.isNull() ? 0 : tmp.position();
        tmp = cursor;
        TextBlockUserData::findNextBlockClosingParenthesis(&tmp);
        int blockEnd = tmp.isNull() ? (cursor.document()->characterCount()-1) : tmp.position();
        QChar openChar = character;
        QChar closeChar = charType(character, CharType::CloseChar);

        int errors = 0;
        int stillopen = 0;
        countBrackets(cursor, blockStart, blockEnd, openChar, closeChar, &errors, &stillopen);
        int errorsBeforeDeletion = errors + stillopen;
        errors = 0;
        stillopen = 0;
        countBrackets(cursor, blockStart, pos - 1, openChar, closeChar, &errors, &stillopen);
        countBrackets(cursor, pos, blockEnd, openChar, closeChar, &errors, &stillopen);
        int errorsAfterDeletion = errors + stillopen;

        if (errorsAfterDeletion < errorsBeforeDeletion)
            return false; // insertion fixes parentheses or bracket errors, do not auto complete
    }

    // ### this code needs to be generalized
    if    ((lookBehind == QLatin1Char('(') && lookAhead == QLatin1Char(')'))
        || (lookBehind == QLatin1Char('[') && lookAhead == QLatin1Char(']'))
        || (lookBehind == QLatin1Char('{') && lookAhead == QLatin1Char('}'))
        || (lookBehind == QLatin1Char('"') && lookAhead == QLatin1Char('"')
            && lookFurtherBehind != QLatin1Char('\\'))
        || (lookBehind == QLatin1Char('\'') && lookAhead == QLatin1Char('\'')
            && lookFurtherBehind != QLatin1Char('\\'))) {
        if (! isInComment(c)) {
            cursor.beginEditBlock();
            cursor.deleteChar();
            cursor.deletePreviousChar();
            cursor.endEditBlock();
            return true;
        }
    }
    return false;
}

int AutoCompleter::paragraphSeparatorAboutToBeInserted(QTextCursor &cursor)
{
    if (!m_autoInsertBrackets)
        return 0;

    QTextDocument *doc = cursor.document();
    if (doc->characterAt(cursor.position() - 1) != QLatin1Char('{'))
        return 0;

    if (!contextAllowsAutoBrackets(cursor))
        return 0;

    // verify that we indeed do have an extra opening brace in the document
    QTextBlock block = cursor.block();
    const QString textFromCusror = block.text().mid(cursor.positionInBlock()).trimmed();
    int braceDepth = TextDocumentLayout::braceDepth(doc->lastBlock());

    if (braceDepth <= 0 && (textFromCusror.isEmpty() || textFromCusror.at(0) != QLatin1Char('}')))
        return 0; // braces are all balanced or worse, no need to do anything and separator inserted not between '{' and '}'

    // we have an extra brace , let's see if we should close it

    /* verify that the next block is not further intended compared to the current block.
       This covers the following case:

            if (condition) {|
                statement;
    */
    if (isNextBlockIndented(block))
        return 0;

    const QString &textToInsert = insertParagraphSeparator(cursor);
    int pos = cursor.position();
    cursor.insertBlock();
    cursor.insertText(textToInsert);
    cursor.setPosition(pos);

    // if we actually insert a separator, allow it to be overwritten if
    // user types it
    if (!textToInsert.isEmpty())
        m_allowSkippingOfBlockEnd = true;

    return 1;
}

bool AutoCompleter::contextAllowsAutoBrackets(const QTextCursor &cursor,
                                              const QString &textToInsert) const
{
    Q_UNUSED(cursor);
    Q_UNUSED(textToInsert);
    return false;
}

bool AutoCompleter::contextAllowsAutoQuotes(const QTextCursor &cursor, const QString &textToInsert) const
{
    Q_UNUSED(cursor);
    Q_UNUSED(textToInsert);
    return false;
}

bool AutoCompleter::contextAllowsElectricCharacters(const QTextCursor &cursor) const
{
    return contextAllowsAutoBrackets(cursor);
}

bool AutoCompleter::isInComment(const QTextCursor &cursor) const
{
    Q_UNUSED(cursor);
    return false;
}

bool AutoCompleter::isInString(const QTextCursor &cursor) const
{
    Q_UNUSED(cursor);
    return false;
}

QString AutoCompleter::insertMatchingBrace(const QTextCursor &cursor,
                                           const QString &text,
                                           QChar lookAhead,
                                           bool skipChars,
                                           int *skippedChars) const
{
    Q_UNUSED(cursor);
    Q_UNUSED(text);
    Q_UNUSED(lookAhead);
    Q_UNUSED(skipChars);
    Q_UNUSED(skippedChars);
    return QString();
}

QString AutoCompleter::insertMatchingQuote(const QTextCursor &cursor,
                                           const QString &text,
                                           QChar lookAhead,
                                           bool skipChars,
                                           int *skippedChars) const
{
    Q_UNUSED(cursor);
    Q_UNUSED(text);
    Q_UNUSED(lookAhead);
    Q_UNUSED(skipChars);
    Q_UNUSED(skippedChars);
    return QString();
}

QString AutoCompleter::insertParagraphSeparator(const QTextCursor &cursor) const
{
    Q_UNUSED(cursor);
    return QString();
}
