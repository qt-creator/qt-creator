// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

