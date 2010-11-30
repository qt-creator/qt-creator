/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef AUTOCOMPLETER_H
#define AUTOCOMPLETER_H

#include "texteditor_global.h"

#include <QtCore/QChar>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QTextCursor;
QT_END_NAMESPACE

namespace TextEditor {

class TEXTEDITOR_EXPORT AutoCompleter
{
public:
    AutoCompleter();
    virtual ~AutoCompleter();

    void setAutoParenthesesEnabled(bool b);
    bool isAutoParenthesesEnabled() const;

    void setSurroundWithEnabled(bool b);
    bool isSurroundWithEnabled() const;

    // Returns the text to complete at the cursor position, or an empty string
    virtual QString autoComplete(QTextCursor &cursor, const QString &text) const;

    // Handles backspace. When returning true, backspace processing is stopped
    virtual bool autoBackspace(QTextCursor &cursor);

    // Hook to insert special characters on enter. Returns the number of extra blocks inserted.
    virtual int paragraphSeparatorAboutToBeInserted(QTextCursor &cursor);

    virtual bool contextAllowsAutoParentheses(const QTextCursor &cursor,
                                              const QString &textToInsert = QString()) const;
    virtual bool contextAllowsElectricCharacters(const QTextCursor &cursor) const;

    // Returns true if the cursor is inside a comment.
    virtual bool isInComment(const QTextCursor &cursor) const;

    virtual QString insertMatchingBrace(const QTextCursor &cursor, const
                                        QString &text,
                                        QChar la,
                                        int *skippedChars) const;

    // Returns the text that needs to be inserted
    virtual QString insertParagraphSeparator(const QTextCursor &cursor) const;

protected:
    static void countBracket(QChar open, QChar close, QChar c, int *errors, int *stillopen);
    static void countBrackets(QTextCursor cursor, int from, int end, QChar open, QChar close,
                              int *errors, int *stillopen);

private:
    mutable bool m_allowSkippingOfBlockEnd;
    bool m_surroundWithEnabled;
    bool m_autoParenthesesEnabled;
};

} // TextEditor

#endif // AUTOCOMPLETER_H
