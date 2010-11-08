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

    bool contextAllowsAutoParentheses(const QTextCursor &cursor,
                                      const QString &textToInsert = QString()) const;
    bool contextAllowsElectricCharacters(const QTextCursor &cursor) const;

    // Returns true if the cursor is inside a comment.
    bool isInComment(const QTextCursor &cursor) const;

    QString insertMatchingBrace(const QTextCursor &cursor, const
                                QString &text,
                                QChar la,
                                int *skippedChars) const;
    // Returns the text that needs to be inserted
    QString insertParagraphSeparator(const QTextCursor &cursor) const;

private:
    virtual bool doContextAllowsAutoParentheses(const QTextCursor &cursor,
                                                const QString &textToInsert = QString()) const;
    virtual bool doContextAllowsElectricCharacters(const QTextCursor &cursor) const;
    virtual bool doIsInComment(const QTextCursor &cursor) const;
    virtual QString doInsertMatchingBrace(const QTextCursor &cursor,
                                          const QString &text,
                                          QChar la,
                                          int *skippedChars) const;
    virtual QString doInsertParagraphSeparator(const QTextCursor &cursor) const;
};

} // TextEditor

#endif // AUTOCOMPLETER_H
