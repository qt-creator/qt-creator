/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef INDENTER_H
#define INDENTER_H

#include "texteditor_global.h"

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QTextDocument;
class QTextCursor;
class QTextBlock;
class QChar;
QT_END_NAMESPACE

namespace TextEditor {

class ICodeStylePreferences;
class TabSettings;

class TEXTEDITOR_EXPORT Indenter
{
public:
    Indenter();
    virtual ~Indenter();

    // Returns true if key triggers an indent.
    virtual bool isElectricCharacter(const QChar &ch) const;

    // Indent a text block based on previous line. Default does nothing
    virtual void indentBlock(QTextDocument *doc,
                             const QTextBlock &block,
                             const QChar &typedChar,
                             const TabSettings &tabSettings);

    // Indent at cursor. Calls indentBlock for selection or current line.
    virtual void indent(QTextDocument *doc,
                        const QTextCursor &cursor,
                        const QChar &typedChar,
                        const TabSettings &tabSettings);

    // Reindent at cursor. Selection will be adjusted according to the indentation
    // change of the first block.
    virtual void reindent(QTextDocument *doc, const QTextCursor &cursor, const TabSettings &tabSettings);

    virtual void setCodeStylePreferences(ICodeStylePreferences *preferences);

    virtual void invalidateCache(QTextDocument *doc);
};

} // namespace TextEditor

#endif // INDENTER_H
