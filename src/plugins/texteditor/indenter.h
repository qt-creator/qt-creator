/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef INDENTER_H
#define INDENTER_H

#include "texteditor_global.h"

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
