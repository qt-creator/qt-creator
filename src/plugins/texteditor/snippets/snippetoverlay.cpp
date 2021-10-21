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

#include "snippetoverlay.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace TextEditor {
namespace Internal {

void SnippetOverlay::clear()
{
    TextEditorOverlay::clear();
    m_selections.clear();
    m_variables.clear();
}

void SnippetOverlay::addSnippetSelection(const QTextCursor &cursor,
                                         const QColor &color,
                                         NameMangler *mangler,
                                         int variableIndex)
{
    m_variables[variableIndex] << selections().size();
    SnippetSelection selection;
    selection.variableIndex = variableIndex;
    selection.mangler = mangler;
    m_selections << selection;
    addOverlaySelection(cursor, color, color, TextEditorOverlay::ExpandBegin);
}

void SnippetOverlay::setFinalSelection(const QTextCursor &cursor, const QColor &color)
{
    m_finalSelectionIndex = selections().size();
    addOverlaySelection(cursor, color, color, TextEditorOverlay::ExpandBegin);
}

void SnippetOverlay::updateEquivalentSelections(const QTextCursor &cursor)
{
    const int &currentIndex = indexForCursor(cursor);
    if (currentIndex < 0)
        return;
    const QString &currentText = cursorForIndex(currentIndex).selectedText();
    const QList<int> &equivalents = m_variables.value(m_selections[currentIndex].variableIndex);
    for (int i : equivalents) {
        if (i == currentIndex)
            continue;
        QTextCursor cursor = cursorForIndex(i);
        const QString &equivalentText = cursor.selectedText();
        if (currentText != equivalentText) {
            cursor.joinPreviousEditBlock();
            cursor.insertText(currentText);
            cursor.endEditBlock();
        }
    }
}

void SnippetOverlay::accept()
{
    hide();
    for (int i = 0; i < m_selections.size(); ++i) {
        if (NameMangler *mangler = m_selections[i].mangler) {
            QTextCursor cursor = cursorForIndex(i);
            const QString current = cursor.selectedText();
            const QString result = mangler->mangle(current);
            if (result != current) {
                cursor.joinPreviousEditBlock();
                cursor.insertText(result);
                cursor.endEditBlock();
            }
        }
    }
    clear();
}

bool SnippetOverlay::hasCursorInSelection(const QTextCursor &cursor) const
{
    return indexForCursor(cursor) >= 0;
}

QTextCursor SnippetOverlay::firstSelectionCursor() const
{
    const QList<OverlaySelection> selections = TextEditorOverlay::selections();
    return selections.isEmpty() ? QTextCursor() : cursorForSelection(selections.first());
}

QTextCursor SnippetOverlay::nextSelectionCursor(const QTextCursor &cursor) const
{
    const QList<OverlaySelection> selections = TextEditorOverlay::selections();
    if (selections.isEmpty())
        return {};
    const SnippetSelection &currentSelection = selectionForCursor(cursor);
    if (currentSelection.variableIndex >= 0) {
        int nextVariableIndex = currentSelection.variableIndex + 1;
        if (!m_variables.contains(nextVariableIndex)) {
            if (m_finalSelectionIndex >= 0)
                return cursorForIndex(m_finalSelectionIndex);
            nextVariableIndex = m_variables.firstKey();
        }

        for (int selectionIndex : m_variables[nextVariableIndex]) {
            if (selections[selectionIndex].m_cursor_begin.position() > cursor.position())
                return cursorForIndex(selectionIndex);
        }
        return cursorForIndex(m_variables[nextVariableIndex].first());
    }
    // currently not over a variable simply select the next available one
    for (const OverlaySelection &candidate : selections) {
        if (candidate.m_cursor_begin.position() > cursor.position())
            return cursorForSelection(candidate);
    }
    return cursorForSelection(selections.first());
}

QTextCursor SnippetOverlay::previousSelectionCursor(const QTextCursor &cursor) const
{
    const QList<OverlaySelection> selections = TextEditorOverlay::selections();
    if (selections.isEmpty())
        return {};
    const SnippetSelection &currentSelection = selectionForCursor(cursor);
    if (currentSelection.variableIndex >= 0) {
        int previousVariableIndex = currentSelection.variableIndex - 1;
        if (!m_variables.contains(previousVariableIndex))
            previousVariableIndex = m_variables.lastKey();

        const QList<int> &equivalents = m_variables[previousVariableIndex];
        for (int i = equivalents.size() - 1; i >= 0; --i) {
            if (selections.at(equivalents.at(i)).m_cursor_end.position() < cursor.position())
                return cursorForIndex(equivalents.at(i));
        }
        return cursorForIndex(m_variables[previousVariableIndex].last());
    }
    // currently not over a variable simply select the previous available one
    for (int i = selections.size() - 1; i >= 0; --i) {
        if (selections.at(i).m_cursor_end.position() < cursor.position())
            return cursorForIndex(i);
    }
    return cursorForSelection(selections.last());
}

bool SnippetOverlay::isFinalSelection(const QTextCursor &cursor) const
{
    return m_finalSelectionIndex >= 0 ? cursor == cursorForIndex(m_finalSelectionIndex) : false;
}

int SnippetOverlay::indexForCursor(const QTextCursor &cursor) const
{
    return Utils::indexOf(selections(),
                          [pos = cursor.position()](const OverlaySelection &selection) {
                              return selection.m_cursor_begin.position() <= pos
                                     && selection.m_cursor_end.position() >= pos;
                          });
}

SnippetOverlay::SnippetSelection SnippetOverlay::selectionForCursor(const QTextCursor &cursor) const
{
    return m_selections.value(indexForCursor(cursor));
}

} // namespace Internal
} // namespace TextEditor
