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

#ifndef UNDO_COMMANDS_H
#define UNDO_COMMANDS_H

#include "resourceview.h"

#include <QString>
#include <QUndoCommand>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace ResourceEditor {
namespace Internal {

/*!
    \class ViewCommand

    Provides a base for \l ResourceView-related commands.
*/
class ViewCommand : public QUndoCommand
{
protected:
    ResourceView *m_view;

    ViewCommand(ResourceView *view);
    virtual ~ViewCommand();
};

/*!
    \class ModelIndexViewCommand

    Provides a mean to store/restore a \l QModelIndex as it cannot
    be stored safely in most cases. This is an abstract class.
*/
class ModelIndexViewCommand : public ViewCommand
{
    int m_prefixArrayIndex;
    int m_fileArrayIndex;

protected:
    ModelIndexViewCommand(ResourceView *view);
    virtual ~ModelIndexViewCommand();
    void storeIndex(const QModelIndex &index);
    QModelIndex makeIndex() const;
};

/*!
    \class ModifyPropertyCommand

    Modifies the name/prefix/language property of a prefix/file node.
*/
class ModifyPropertyCommand : public ModelIndexViewCommand
{
    ResourceView::NodeProperty m_property;
    QString m_before;
    QString m_after;
    int m_mergeId;

public:
    ModifyPropertyCommand(ResourceView *view, const QModelIndex &nodeIndex,
            ResourceView::NodeProperty property, const int mergeId, const QString &before,
            const QString &after = QString());

private:
    int id() const { return m_mergeId; }
    bool mergeWith(const QUndoCommand * command);
    void undo();
    void redo();
};

/*!
    \class RemoveEntryCommand

    Removes a \l QModelIndex including all children from a \l ResourceView.
*/
class RemoveEntryCommand : public ModelIndexViewCommand
{
    EntryBackup *m_entry;
    bool m_isExpanded;

public:
    RemoveEntryCommand(ResourceView *view, const QModelIndex &index);
    ~RemoveEntryCommand();

private:
    void redo();
    void undo();
    void freeEntry();
};

/*!
    \class RemoveMultipleEntryCommand

    Removes multiple \l QModelIndex including all children from a \l ResourceView.
*/
class RemoveMultipleEntryCommand : public QUndoCommand
{
    std::vector<QUndoCommand *> m_subCommands;
public:
    // list must be in view order
    RemoveMultipleEntryCommand(ResourceView *view, const QList<QModelIndex> &list);
    ~RemoveMultipleEntryCommand();
private:
    void redo() override;
    void undo() override;
};


/*!
    \class AddFilesCommand

    Adds a list of files to a given prefix node.
*/
class AddFilesCommand : public ViewCommand
{
    int m_prefixIndex;
    int m_cursorFileIndex;
    int m_firstFile;
    int m_lastFile;
    const QStringList m_fileNames;

public:
    AddFilesCommand(ResourceView *view, int prefixIndex, int cursorFileIndex,
            const QStringList &fileNames);

private:
    void redo();
    void undo();
};

/*!
    \class AddEmptyPrefixCommand

    Adds a new, empty prefix node.
*/
class AddEmptyPrefixCommand : public ViewCommand
{
    int m_prefixArrayIndex;

public:
    AddEmptyPrefixCommand(ResourceView *view);

private:
    void redo();
    void undo();
};

} // namespace Internal
} // namespace ResourceEditor

#endif // UNDO_COMMANDS_H
