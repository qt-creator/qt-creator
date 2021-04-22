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

#include "snippet.h"

namespace TextEditor {
namespace Internal {


void SnippetOverlay::clear()
{
    TextEditorOverlay::clear();
    m_equivalentSelections.clear();
    m_manglers.clear();
}

void SnippetOverlay::mapEquivalentSelections()
{
    m_equivalentSelections.clear();
    m_equivalentSelections.resize(selections().size());

    QMultiMap<QString, int> all;
    for (int i = 0; i < selections().size(); ++i)
        all.insert(selectionText(i).toLower(), i);

    const QList<QString> &uniqueKeys = all.uniqueKeys();
    foreach (const QString &key, uniqueKeys) {
        QList<int> indexes;
        const auto cAll = all;
        QMultiMap<QString, int>::const_iterator lbit = cAll.lowerBound(key);
        QMultiMap<QString, int>::const_iterator ubit = cAll.upperBound(key);
        while (lbit != ubit) {
            indexes.append(lbit.value());
            ++lbit;
        }

        foreach (int index, indexes)
            m_equivalentSelections[index] = indexes;
    }
}

void SnippetOverlay::updateEquivalentSelections(const QTextCursor &cursor)
{
    int selectionIndex = selectionIndexForCursor(cursor);
    if (selectionIndex == -1)
        return;

    const QString &currentText = selectionText(selectionIndex);
    const QList<int> &equivalents = m_equivalentSelections.at(selectionIndex);
    foreach (int i, equivalents) {
        if (i == selectionIndex)
            continue;
        const QString &equivalentText = selectionText(i);
        if (currentText != equivalentText) {
            QTextCursor selectionCursor = assembleCursorForSelection(i);
            selectionCursor.joinPreviousEditBlock();
            selectionCursor.removeSelectedText();
            selectionCursor.insertText(currentText);
            selectionCursor.endEditBlock();
        }
    }
}

void SnippetOverlay::setNameMangler(const QList<NameMangler *> &manglers)
{
    m_manglers = manglers;
}

void SnippetOverlay::mangle()
{
    for (int i = 0; i < m_manglers.count(); ++i) {
        if (!m_manglers.at(i))
            continue;

        const QString current = selectionText(i);
        const QString result = m_manglers.at(i)->mangle(current);
        if (result != current) {
            QTextCursor selectionCursor = assembleCursorForSelection(i);
            selectionCursor.joinPreviousEditBlock();
            selectionCursor.removeSelectedText();
            selectionCursor.insertText(result);
            selectionCursor.endEditBlock();
        }
    }
}

} // namespace Internal
} // namespace TextEditor
