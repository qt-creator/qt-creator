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

#include "undocommands_p.h"

#include <QModelIndex>

using namespace ResourceEditor;
using namespace ResourceEditor::Internal;

ViewCommand::ViewCommand(ResourceView *view)
        : m_view(view)
{ }

ViewCommand::~ViewCommand()
{ }

ModelIndexViewCommand::ModelIndexViewCommand(ResourceView *view)
        : ViewCommand(view)
{ }

ModelIndexViewCommand::~ModelIndexViewCommand()
{ }

void ModelIndexViewCommand::storeIndex(const QModelIndex &index)
{
    if (m_view->isPrefix(index)) {
        m_prefixArrayIndex = index.row();
        m_fileArrayIndex = -1;
    } else {
        m_fileArrayIndex = index.row();
        m_prefixArrayIndex = m_view->model()->parent(index).row();
    }
}

QModelIndex ModelIndexViewCommand::makeIndex() const
{
    const QModelIndex prefixModelIndex
            = m_view->model()->index(m_prefixArrayIndex, 0, QModelIndex());
    if (m_fileArrayIndex != -1) {
        // File node
        const QModelIndex fileModelIndex
                = m_view->model()->index(m_fileArrayIndex, 0, prefixModelIndex);
        return fileModelIndex;
    } else {
        // Prefix node
        return prefixModelIndex;
    }
}



ModifyPropertyCommand::ModifyPropertyCommand(ResourceView *view, const QModelIndex &nodeIndex,
        ResourceView::NodeProperty property, const int mergeId, const QString &before,
        const QString &after)
        : ModelIndexViewCommand(view), m_property(property), m_before(before), m_after(after),
        m_mergeId(mergeId)
{
    storeIndex(nodeIndex);
}

bool ModifyPropertyCommand::mergeWith(const QUndoCommand * command)
{
    if (command->id() != id() || m_property != static_cast<const ModifyPropertyCommand *>(command)->m_property)
        return false;
    // Choose older command (this) and forgot the other
    return true;
}

void ModifyPropertyCommand::undo()
{
    Q_ASSERT(m_view);

    // Save current text in m_after for redo()
    m_after = m_view->getCurrentValue(m_property);

    // Reset text to m_before
    m_view->changeValue(makeIndex(), m_property, m_before);
}

void ModifyPropertyCommand::redo()
{
    // Prevent execution from within QUndoStack::push
    if (m_after.isNull())
        return;

    // Bring back text before undo
    Q_ASSERT(m_view);
    m_view->changeValue(makeIndex(), m_property, m_after);
}

RemoveEntryCommand::RemoveEntryCommand(ResourceView *view, const QModelIndex &index)
        : ModelIndexViewCommand(view), m_entry(0), m_isExpanded(true)
{
    storeIndex(index);
}

RemoveEntryCommand::~RemoveEntryCommand()
{
    freeEntry();
}

void RemoveEntryCommand::redo()
{
    freeEntry();
    const QModelIndex index = makeIndex();
    m_isExpanded = m_view->isExpanded(index);
    m_entry = m_view->removeEntry(index);
}

void RemoveEntryCommand::undo()
{
    if (m_entry != 0) {
        m_entry->restore();
        Q_ASSERT(m_view != 0);
        const QModelIndex index = makeIndex();
        m_view->setExpanded(index, m_isExpanded);
        m_view->setCurrentIndex(index);
        freeEntry();
    }
}

void RemoveEntryCommand::freeEntry()
{
    delete m_entry;
    m_entry = 0;
}

RemoveMultipleEntryCommand::RemoveMultipleEntryCommand(ResourceView *view, const QList<QModelIndex> &list)
{
    m_subCommands.reserve(list.size());
    for (const QModelIndex &index : list)
        m_subCommands.push_back(new RemoveEntryCommand(view, index));
}

RemoveMultipleEntryCommand::~RemoveMultipleEntryCommand()
{
    qDeleteAll(m_subCommands);
}

void RemoveMultipleEntryCommand::redo()
{
    auto it = m_subCommands.rbegin();
    auto end = m_subCommands.rend();

    for (; it != end; ++it)
        (*it)->redo();
}

void RemoveMultipleEntryCommand::undo()
{
    auto it = m_subCommands.begin();
    auto end = m_subCommands.end();

    for (; it != end; ++it)
        (*it)->undo();
}

AddFilesCommand::AddFilesCommand(ResourceView *view, int prefixIndex, int cursorFileIndex,
        const QStringList &fileNames)
        : ViewCommand(view), m_prefixIndex(prefixIndex), m_cursorFileIndex(cursorFileIndex),
        m_fileNames(fileNames)
{ }

void AddFilesCommand::redo()
{
    m_view->addFiles(m_prefixIndex, m_fileNames, m_cursorFileIndex, m_firstFile, m_lastFile);
}

void AddFilesCommand::undo()
{
    m_view->removeFiles(m_prefixIndex, m_firstFile, m_lastFile);
}

AddEmptyPrefixCommand::AddEmptyPrefixCommand(ResourceView *view)
        : ViewCommand(view)
{ }

void AddEmptyPrefixCommand::redo()
{
    m_prefixArrayIndex = m_view->addPrefix().row();
}

void AddEmptyPrefixCommand::undo()
{
    const QModelIndex prefixModelIndex = m_view->model()->index(
            m_prefixArrayIndex, 0, QModelIndex());
    delete m_view->removeEntry(prefixModelIndex);
}
