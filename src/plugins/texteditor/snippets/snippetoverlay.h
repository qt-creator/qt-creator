/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "snippet.h"
#include "texteditor/texteditoroverlay.h"

#include <QTextEdit>

namespace TextEditor {
class NameMangler;

namespace Internal {

class SnippetOverlay : public TextEditorOverlay
{
public:
    using TextEditorOverlay::TextEditorOverlay;

    void clear() override;

    void addSnippetSelection(const QTextCursor &cursor,
                             const QColor &color,
                             NameMangler *mangler,
                             int variableGoup);
    void setFinalSelection(const QTextCursor &cursor, const QColor &color);

    void updateEquivalentSelections(const QTextCursor &cursor);
    void accept();

    bool hasCursorInSelection(const QTextCursor &cursor) const;

    QTextCursor firstSelectionCursor() const;
    QTextCursor nextSelectionCursor(const QTextCursor &cursor) const;
    QTextCursor previousSelectionCursor(const QTextCursor &cursor) const;
    bool isFinalSelection(const QTextCursor &cursor) const;

private:
    struct SnippetSelection
    {
        int variableIndex = -1;
        NameMangler *mangler;
    };

    int indexForCursor(const QTextCursor &cursor) const;
    SnippetSelection selectionForCursor(const QTextCursor &cursor) const;

    QList<SnippetSelection> m_selections;
    int m_finalSelectionIndex = -1;
    QMap<int, QList<int>> m_variables;
};

} // namespace Internal
} // namespace TextEditor

