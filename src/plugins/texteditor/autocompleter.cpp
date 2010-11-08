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

#include "autocompleter.h"

#include <QtGui/QTextCursor>

using namespace TextEditor;

AutoCompleter::AutoCompleter()
{}

AutoCompleter::~AutoCompleter()
{}

bool AutoCompleter::contextAllowsAutoParentheses(const QTextCursor &cursor,
                                                 const QString &textToInsert) const
{
    return doContextAllowsAutoParentheses(cursor, textToInsert);
}

bool AutoCompleter::contextAllowsElectricCharacters(const QTextCursor &cursor) const
{
    return doContextAllowsElectricCharacters(cursor);
}

bool AutoCompleter::isInComment(const QTextCursor &cursor) const
{
    return doIsInComment(cursor);
}

QString AutoCompleter::insertMatchingBrace(const QTextCursor &cursor, const
                                           QString &text,
                                           QChar la,
                                           int *skippedChars) const
{
    return doInsertMatchingBrace(cursor, text, la, skippedChars);
}

QString AutoCompleter::insertParagraphSeparator(const QTextCursor &cursor) const
{
    return doInsertParagraphSeparator(cursor);
}

bool AutoCompleter::doContextAllowsAutoParentheses(const QTextCursor &cursor,
                                                   const QString &textToInsert) const
{
    Q_UNUSED(cursor);
    Q_UNUSED(textToInsert);
    return false;
}

bool AutoCompleter::doContextAllowsElectricCharacters(const QTextCursor &cursor) const
{
    return doContextAllowsAutoParentheses(cursor);
}

bool AutoCompleter::doIsInComment(const QTextCursor &cursor) const
{
    Q_UNUSED(cursor);
    return false;
}

QString AutoCompleter::doInsertMatchingBrace(const QTextCursor &cursor,
                                             const QString &text,
                                             QChar la,
                                             int *skippedChars) const
{
    Q_UNUSED(cursor);
    Q_UNUSED(text);
    Q_UNUSED(la);
    Q_UNUSED(skippedChars);
    return QString();
}

QString AutoCompleter::doInsertParagraphSeparator(const QTextCursor &cursor) const
{
    Q_UNUSED(cursor);
    return QString();
}
