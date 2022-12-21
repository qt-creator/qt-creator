// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"
#include "tabsettings.h"

#include <QString>

QT_BEGIN_NAMESPACE
class QTextBlock;
class QTextCursor;
QT_END_NAMESPACE

namespace TextEditor {

class TEXTEDITOR_EXPORT AutoCompleter
{
public:
    AutoCompleter();
    virtual ~AutoCompleter();

    void setAutoInsertBracketsEnabled(bool b) { m_autoInsertBrackets = b; }
    bool isAutoInsertBracketsEnabled() const { return m_autoInsertBrackets; }
    void setSurroundWithBracketsEnabled(bool b) { m_surroundWithBrackets = b; }
    bool isSurroundWithBracketsEnabled() const { return m_surroundWithBrackets; }

    void setAutoInsertQuotesEnabled(bool b) { m_autoInsertQuotes = b; }
    bool isAutoInsertQuotesEnabled() const { return m_autoInsertQuotes; }
    void setSurroundWithQuotesEnabled(bool b) { m_surroundWithQuotes = b; }
    bool isSurroundWithQuotesEnabled() const { return m_surroundWithQuotes; }

    void setOverwriteClosingCharsEnabled(bool b) { m_overwriteClosingChars = b; }
    bool isOverwriteClosingCharsEnabled() const { return m_overwriteClosingChars; }

    void setTabSettings(const TabSettings &tabSettings) { m_tabSettings = tabSettings; }
    const TabSettings &tabSettings() const { return m_tabSettings; }

    // Returns the text to complete at the cursor position, or an empty string
    virtual QString autoComplete(QTextCursor &cursor, const QString &text, bool skipChars) const;

    // Handles backspace. When returning true, backspace processing is stopped
    virtual bool autoBackspace(QTextCursor &cursor);

    // Hook to insert special characters on enter. Returns the number of extra blocks inserted.
    virtual int paragraphSeparatorAboutToBeInserted(QTextCursor &cursor);

    virtual bool contextAllowsAutoBrackets(const QTextCursor &cursor,
                                              const QString &textToInsert = QString()) const;
    virtual bool contextAllowsAutoQuotes(const QTextCursor &cursor,
                                         const QString &textToInsert = QString()) const;
    virtual bool contextAllowsElectricCharacters(const QTextCursor &cursor) const;

    // Returns true if the cursor is inside a comment.
    virtual bool isInComment(const QTextCursor &cursor) const;

    // Returns true if the cursor is inside a string.
    virtual bool isInString(const QTextCursor &cursor) const;

    virtual QString insertMatchingBrace(const QTextCursor &cursor, const
                                        QString &text,
                                        QChar lookAhead, bool skipChars,
                                        int *skippedChars) const;

    virtual QString insertMatchingQuote(const QTextCursor &cursor, const
                                        QString &text,
                                        QChar lookAhead, bool skipChars,
                                        int *skippedChars) const;

    // Returns the text that needs to be inserted
    virtual QString insertParagraphSeparator(const QTextCursor &cursor) const;

    static bool isQuote(const QString &text);
    bool isNextBlockIndented(const QTextBlock &currentBlock) const;

private:
    QString replaceSelection(QTextCursor &cursor, const QString &textToInsert) const;

private:
    TabSettings m_tabSettings;
    mutable bool m_allowSkippingOfBlockEnd;
    bool m_autoInsertBrackets;
    bool m_surroundWithBrackets;
    bool m_autoInsertQuotes;
    bool m_surroundWithQuotes;
    bool m_overwriteClosingChars;
};

} // TextEditor
