/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "openeditorsmodel.h"
#include "ieditor.h"
#include "idocument.h"

#include <QDir>
#include <QIcon>

namespace Core {

struct OpenEditorsModelPrivate
{
    OpenEditorsModelPrivate();

    const QIcon m_lockedIcon;
    const QIcon m_unlockedIcon;

    QList<OpenEditorsModel::Entry> m_editors;
    QList<IEditor *> m_duplicateEditors;
};

OpenEditorsModelPrivate::OpenEditorsModelPrivate() :
    m_lockedIcon(QLatin1String(":/core/images/locked.png")),
    m_unlockedIcon(QLatin1String(":/core/images/unlocked.png"))
{
}

OpenEditorsModel::Entry::Entry() :
    editor(0)
{
}

OpenEditorsModel::OpenEditorsModel(QObject *parent) :
    QAbstractItemModel(parent), d(new OpenEditorsModelPrivate)
{
}

OpenEditorsModel::~OpenEditorsModel()
{
    delete d;
}

QIcon OpenEditorsModel::lockedIcon() const
{
    return d->m_lockedIcon;
}

QIcon OpenEditorsModel::unlockedIcon() const
{
    return d->m_unlockedIcon;
}

QString OpenEditorsModel::Entry::fileName() const {
    return editor ? editor->document()->fileName() : m_fileName;
}

QString OpenEditorsModel::Entry::displayName() const {
    return editor ? editor->displayName() : m_displayName;
}

Id OpenEditorsModel::Entry::id() const
{
    return editor ? editor->id() : m_id;
}

int OpenEditorsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 2;
}

int OpenEditorsModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return d->m_editors.count();
    return 0;
}

QList<IEditor *> OpenEditorsModel::editors() const
{
    QList<IEditor *> result;
    foreach (const Entry &entry, d->m_editors)
        if (entry.editor)
            result += entry.editor;
    return result;
}

void OpenEditorsModel::addEditor(IEditor *editor, bool isDuplicate)
{
    if (!editor)
        return;

    if (isDuplicate) {
        d->m_duplicateEditors.append(editor);
        return;
    }

    Entry entry;
    entry.editor = editor;
    addEntry(entry);
}

void OpenEditorsModel::addRestoredEditor(const QString &fileName, const QString &displayName, const Id &id)
{
    Entry entry;
    entry.m_fileName = fileName;
    entry.m_displayName = displayName;
    entry.m_id = id;
    addEntry(entry);
}

QModelIndex OpenEditorsModel::firstRestoredEditor() const
{
    for (int i = 0; i < d->m_editors.count(); ++i)
        if (!d->m_editors.at(i).editor)
            return createIndex(i, 0);
    return QModelIndex();
}

void OpenEditorsModel::addEntry(const Entry &entry)
{
    QString fileName = entry.fileName();

    int previousIndex = findFileName(fileName);
    if (previousIndex >= 0) {
        if (entry.editor && d->m_editors.at(previousIndex).editor == 0) {
            d->m_editors[previousIndex] = entry;
            connect(entry.editor, SIGNAL(changed()), this, SLOT(itemChanged()));
        }
        return;
    }

    int index;
    QString displayName = entry.displayName();
    for (index = 0; index < d->m_editors.count(); ++index) {
        if (displayName < d->m_editors.at(index).displayName())
            break;
    }

    beginInsertRows(QModelIndex(), index, index);
    d->m_editors.insert(index, entry);
    if (entry.editor)
        connect(entry.editor, SIGNAL(changed()), this, SLOT(itemChanged()));
    endInsertRows();
}


int OpenEditorsModel::findEditor(IEditor *editor) const
{
    for (int i = 0; i < d->m_editors.count(); ++i)
        if (d->m_editors.at(i).editor == editor)
            return i;
    return -1;
}

int OpenEditorsModel::findFileName(const QString &filename) const
{
    if (filename.isEmpty())
        return -1;
    for (int i = 0; i < d->m_editors.count(); ++i) {
        if (d->m_editors.at(i).fileName() == filename)
            return i;
    }
    return -1;
}

void OpenEditorsModel::removeEditor(IEditor *editor)
{
    d->m_duplicateEditors.removeAll(editor);
    removeEditor(findEditor(editor));
}

void OpenEditorsModel::removeEditor(const QModelIndex &index)
{
    removeEditor(index.row());
}

void OpenEditorsModel::removeEditor(const QString &fileName)
{
    removeEditor(findFileName(fileName));
}

void OpenEditorsModel::removeEditor(int idx)
{
    if (idx < 0)
        return;
    IEditor *editor= d->m_editors.at(idx).editor;
    beginRemoveRows(QModelIndex(), idx, idx);
    d->m_editors.removeAt(idx);
    endRemoveRows();
    if (editor)
        disconnect(editor, SIGNAL(changed()), this, SLOT(itemChanged()));
}

void OpenEditorsModel::removeAllRestoredEditors()
{
    for (int i = d->m_editors.count()-1; i >= 0; --i) {
        if (!d->m_editors.at(i).editor) {
            beginRemoveRows(QModelIndex(), i, i);
            d->m_editors.removeAt(i);
            endRemoveRows();
        }
    }
}

QList<OpenEditorsModel::Entry> OpenEditorsModel::restoredEditors() const
{
    QList<Entry> result;
    for (int i = d->m_editors.count()-1; i >= 0; --i) {
        if (!d->m_editors.at(i).editor)
            result.append(d->m_editors.at(i));
    }
    return result;
}

bool OpenEditorsModel::isDuplicate(IEditor *editor) const
{
    return editor && d->m_duplicateEditors.contains(editor);
}

IEditor *OpenEditorsModel::originalForDuplicate(IEditor *duplicate) const
{
    IDocument *document = duplicate->document();
    foreach (const Entry &e, d->m_editors)
        if (e.editor && e.editor->document() == document)
            return e.editor;
    return 0;
}

QList<IEditor *> OpenEditorsModel::duplicatesFor(IEditor *editor) const
{
    QList<IEditor *> result;
    IDocument *document = editor->document();
    foreach (IEditor *e, d->m_duplicateEditors)
        if (e->document() == document)
            result += e;
    return result;
}

void OpenEditorsModel::makeOriginal(IEditor *duplicate)
{
    Q_ASSERT(duplicate && isDuplicate(duplicate));
    IEditor *original = originalForDuplicate(duplicate);
    Q_ASSERT(original);
    int i = findEditor(original);
    d->m_editors[i].editor = duplicate;
    d->m_duplicateEditors.removeOne(duplicate);
    d->m_duplicateEditors.append(original);
    disconnect(original, SIGNAL(changed()), this, SLOT(itemChanged()));
    connect(duplicate, SIGNAL(changed()), this, SLOT(itemChanged()));
}

void OpenEditorsModel::emitDataChanged(IEditor *editor)
{
    int idx = findEditor(editor);
    if (idx < 0)
        return;
    QModelIndex mindex = index(idx, 0);
    emit dataChanged(mindex, mindex);
}

QModelIndex OpenEditorsModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    if (column < 0 || column > 1 || row < 0 || row >= d->m_editors.count())
        return QModelIndex();
    return createIndex(row, column);
}

QVariant OpenEditorsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || (index.column() != 0 && role < Qt::UserRole))
        return QVariant();
    Entry e = d->m_editors.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return (e.editor && e.editor->document()->isModified())
                ? e.displayName() + QLatin1Char('*')
                : e.displayName();
    case Qt::DecorationRole:
    {
        bool showLock = false;
        if (e.editor) {
            showLock = e.editor->document()->fileName().isEmpty()
                    ? false
                    : e.editor->document()->isFileReadOnly();
        } else {
            showLock = !QFileInfo(e.m_fileName).isWritable();
        }
        return showLock ? d->m_lockedIcon : QIcon();
    }
    case Qt::ToolTipRole:
        return e.fileName().isEmpty()
                ? e.displayName()
                : QDir::toNativeSeparators(e.fileName());
    case Qt::UserRole:
        return qVariantFromValue(e.editor);
    case Qt::UserRole + 1:
        return e.fileName();
    case Qt::UserRole + 2:
        return QVariant::fromValue(e.editor ? Core::Id(e.editor->id()) : e.id());
    default:
        return QVariant();
    }
    return QVariant();
}

QModelIndex OpenEditorsModel::indexOf(IEditor *editor) const
{
    int idx = findEditor(originalForDuplicate(editor));
    return createIndex(idx, 0);
}

QString OpenEditorsModel::displayNameForDocument(IDocument *document) const
{
    for (int i = 0; i < d->m_editors.count(); ++i)
        if (d->m_editors.at(i).editor && d->m_editors.at(i).editor->document() == document)
            return d->m_editors.at(i).editor->displayName();
    return QString();
}

void OpenEditorsModel::itemChanged()
{
    emitDataChanged(qobject_cast<IEditor*>(sender()));
}

QList<OpenEditorsModel::Entry> OpenEditorsModel::entries() const
{
    return d->m_editors;
}

IEditor *OpenEditorsModel::editorAt(int row) const
{
    return d->m_editors.at(row).editor;
}

} // namespace Core
