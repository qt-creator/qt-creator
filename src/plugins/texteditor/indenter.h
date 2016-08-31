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

#pragma once

#include "texteditor_global.h"

#include <QMap>

QT_BEGIN_NAMESPACE
class QTextDocument;
class QTextCursor;
class QTextBlock;
class QChar;
QT_END_NAMESPACE

namespace TextEditor {

class ICodeStylePreferences;
class TabSettings;

using IndentationForBlock = QMap<int, int>;

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

    virtual int indentFor(const QTextBlock &block, const TextEditor::TabSettings &tabSettings);

    // Expects a list of blocks in order of occurrence in the document.
    virtual IndentationForBlock indentationForBlocks(const QVector<QTextBlock> &blocks,
                                                     const TextEditor::TabSettings &tabSettings);
};

} // namespace TextEditor
