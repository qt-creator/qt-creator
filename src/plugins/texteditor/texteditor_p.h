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

#include <QList>

namespace TextEditor {

class TextDocument;

namespace Internal {
class TextEditorOverlay;
class ClipboardAssistProvider;

class TEXTEDITOR_EXPORT TextBlockSelection
{
public:
    TextBlockSelection()
        : positionBlock(0), positionColumn(0)
        , anchorBlock(0) , anchorColumn(0){}
    TextBlockSelection(const TextBlockSelection &other);

    void clear();
    QTextCursor selection(const TextDocument *baseTextDocument) const;
    QTextCursor cursor(const TextDocument *baseTextDocument) const;
    void fromPostition(int positionBlock, int positionColumn, int anchorBlock, int anchorColumn);
    bool hasSelection() { return !(positionBlock == anchorBlock && positionColumn == anchorColumn); }

    // defines the first block
    inline int firstBlockNumber() const { return qMin(positionBlock, anchorBlock); }
     // defines the last block
    inline int lastBlockNumber() const { return qMax(positionBlock, anchorBlock); }
    // defines the first visual column of the selection
    inline int firstVisualColumn() const { return qMin(positionColumn, anchorColumn); }
    // defines the last visual column of the selection
    inline int lastVisualColumn() const { return qMax(positionColumn, anchorColumn); }

public:
    int positionBlock;
    int positionColumn;
    int anchorBlock;
    int anchorColumn;

private:
    QTextCursor cursor(const TextDocument *baseTextDocument, bool fullSelection) const;
};

//
// TextEditorPrivate
//

struct TextEditorPrivateHighlightBlocks
{
    QList<int> open;
    QList<int> close;
    QList<int> visualIndent;
    inline int count() const { return visualIndent.size(); }
    inline bool isEmpty() const { return open.isEmpty() || close.isEmpty() || visualIndent.isEmpty(); }
    inline bool operator==(const TextEditorPrivateHighlightBlocks &o) const {
        return (open == o.open && close == o.close && visualIndent == o.visualIndent);
    }
    inline bool operator!=(const TextEditorPrivateHighlightBlocks &o) const { return !(*this == o); }
};

} // namespace Internal
} // namespace TextEditor
