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

#include "resourceview.h"

#include "undocommands_p.h"

#include <coreplugin/fileutils.h>
#include <coreplugin/icore.h>
#include <coreplugin/removefiledialog.h>

#include <QDebug>

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenu>
#include <QMouseEvent>
#include <QUndoStack>

namespace ResourceEditor {
namespace Internal {

/*!
    \class FileEntryBackup

    Backups a file node.
*/
class FileEntryBackup : public EntryBackup
{
private:
    int m_fileIndex;
    QString m_alias;

public:
    FileEntryBackup(ResourceModel &model, int prefixIndex, int fileIndex,
            const QString &fileName, const QString &alias)
            : EntryBackup(model, prefixIndex, fileName), m_fileIndex(fileIndex),
            m_alias(alias) { }
    void restore() const;
};

void FileEntryBackup::restore() const
{
    m_model->insertFile(m_prefixIndex, m_fileIndex, m_name, m_alias);
}

/*!
    \class PrefixEntryBackup

    Backups a prefix node including children.
*/
class PrefixEntryBackup : public EntryBackup
{
private:
    QString m_language;
    QList<FileEntryBackup> m_files;

public:
    PrefixEntryBackup(ResourceModel &model, int prefixIndex, const QString &prefix,
            const QString &language, const QList<FileEntryBackup> &files)
            : EntryBackup(model, prefixIndex, prefix), m_language(language), m_files(files) { }
    void restore() const;
};

void PrefixEntryBackup::restore() const
{
    m_model->insertPrefix(m_prefixIndex, m_name, m_language);
    foreach (const FileEntryBackup &entry, m_files) {
        entry.restore();
    }
}

class RelativeResourceModel : public ResourceModel
{
public:
    RelativeResourceModel(const ResourceFile &resource_file, QObject *parent = 0);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
    {
        if (!index.isValid())
            return QVariant();
/*
        void const * const internalPointer = index.internalPointer();

        if ((role == Qt::DisplayRole) && (internalPointer != NULL))
            return ResourceModel::data(index, Qt::ToolTipRole);
*/
        return ResourceModel::data(index, role);
    }

    void setResourceDragEnabled(bool e) { m_resourceDragEnabled = e; }
    bool resourceDragEnabled() const { return m_resourceDragEnabled; }

    virtual Qt::ItemFlags flags(const QModelIndex &index) const;

    EntryBackup * removeEntry(const QModelIndex &index);

private:
    bool m_resourceDragEnabled;
};

RelativeResourceModel::RelativeResourceModel(const ResourceFile &resource_file, QObject *parent)  :
    ResourceModel(resource_file, parent),
    m_resourceDragEnabled(false)
{
}

Qt::ItemFlags RelativeResourceModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags rc = ResourceModel::flags(index);
    if ((rc & Qt::ItemIsEnabled) && m_resourceDragEnabled)
        rc |= Qt::ItemIsDragEnabled;
    return rc;
}

EntryBackup * RelativeResourceModel::removeEntry(const QModelIndex &index)
{
    const QModelIndex prefixIndex = this->prefixIndex(index);
    const bool isPrefixNode = (prefixIndex == index);

    // Create backup, remove, return backup
    if (isPrefixNode) {
        QString dummy;
        QString prefixBackup;
        getItem(index, prefixBackup, dummy);
        const QString languageBackup = lang(index);
        const int childCount = rowCount(index);
        QList<FileEntryBackup> filesBackup;
        for (int i = 0; i < childCount; i++) {
            const QModelIndex childIndex = this->index(i, 0, index);
            const QString fileNameBackup = file(childIndex);
            const QString aliasBackup = alias(childIndex);
            FileEntryBackup entry(*this, index.row(), i, fileNameBackup, aliasBackup);
            filesBackup << entry;
        }
        deleteItem(index);
        return new PrefixEntryBackup(*this, index.row(), prefixBackup, languageBackup, filesBackup);
    } else {
        const QString fileNameBackup = file(index);
        const QString aliasBackup = alias(index);
        if (!QFile::exists(fileNameBackup)) {
            deleteItem(index);
            return new FileEntryBackup(*this, prefixIndex.row(), index.row(), fileNameBackup, aliasBackup);
        }
        Core::RemoveFileDialog removeFileDialog(fileNameBackup, Core::ICore::mainWindow());
        if (removeFileDialog.exec() == QDialog::Accepted) {
            deleteItem(index);
            Core::FileUtils::removeFile(fileNameBackup, removeFileDialog.isDeleteFileChecked());
            return new FileEntryBackup(*this, prefixIndex.row(), index.row(), fileNameBackup, aliasBackup);
        }
        return 0;
    }
}

} // namespace Internal
} // namespace ResourceEditor

using namespace ResourceEditor;
using namespace ResourceEditor::Internal;

ResourceView::ResourceView(QUndoStack *history, QWidget *parent) :
    QTreeView(parent),
    m_qrcModel(new RelativeResourceModel(m_qrcFile, this)),
    m_history(history),
    m_mergeId(-1)
{
    advanceMergeId();
    setModel(m_qrcModel);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setEditTriggers(EditKeyPressed);

    header()->hide();

    connect(m_qrcModel, SIGNAL(dirtyChanged(bool)),
        this, SIGNAL(dirtyChanged(bool)));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(showContextMenu(QPoint)));
    connect(this, SIGNAL(activated(QModelIndex)),
            this, SLOT(itemActivated(QModelIndex)));
}

ResourceView::~ResourceView()
{
}

bool ResourceView::isDirty() const
{
    return m_qrcModel->dirty();
}

void ResourceView::setDirty(bool dirty)
{
    m_qrcModel->setDirty(dirty);
}

void ResourceView::findSamePlacePostDeletionModelIndex(int &row, QModelIndex &parent) const
{
    // Concept:
    // - Make selection stay on same Y level
    // - Enable user to hit delete several times in row
    const bool hasLowerBrother = m_qrcModel->hasIndex(row + 1,
            0, parent);
    if (hasLowerBrother) {
        // First or mid child -> lower brother
        //  o
        //  +--o
        //  +-[o]  <-- deleted
        //  +--o   <-- chosen
        //  o
        // --> return unmodified
    } else {
        if (parent == QModelIndex()) {
            // Last prefix node
            if (row == 0) {
                // Last and only prefix node
                // [o]  <-- deleted
                //  +--o
                //  +--o
                row = -1;
                parent = QModelIndex();
            } else {
                const QModelIndex upperBrother = m_qrcModel->index(row - 1,
                        0, parent);
                if (m_qrcModel->hasChildren(upperBrother)) {
                    //  o
                    //  +--o  <-- selected
                    // [o]    <-- deleted
                    row = m_qrcModel->rowCount(upperBrother) - 1;
                    parent = upperBrother;
                } else {
                    //  o
                    //  o  <-- selected
                    // [o] <-- deleted
                    row--;
                }
            }
        } else {
            // Last file node
            const bool hasPrefixBelow = m_qrcModel->hasIndex(parent.row() + 1,
                    parent.column(), QModelIndex());
            if (hasPrefixBelow) {
                // Last child or parent with lower brother -> lower brother of parent
                //  o
                //  +--o
                //  +-[o]  <-- deleted
                //  o      <-- chosen
                row = parent.row() + 1;
                parent = QModelIndex();
            } else {
                const bool onlyChild = row == 0;
                if (onlyChild) {
                    // Last and only child of last parent -> parent
                    //  o      <-- chosen
                    //  +-[o]  <-- deleted
                    row = parent.row();
                    parent = m_qrcModel->parent(parent);
                } else {
                    // Last child of last parent -> upper brother
                    //  o
                    //  +--o   <-- chosen
                    //  +-[o]  <-- deleted
                    row--;
                }
            }
        }
    }
}

EntryBackup * ResourceView::removeEntry(const QModelIndex &index)
{
    Q_ASSERT(m_qrcModel);
    return m_qrcModel->removeEntry(index);
}

QStringList ResourceView::existingFilesSubtracted(int prefixIndex, const QStringList &fileNames) const
{
    return m_qrcModel->existingFilesSubtracted(prefixIndex, fileNames);
}

void ResourceView::addFiles(int prefixIndex, const QStringList &fileNames, int cursorFile,
        int &firstFile, int &lastFile)
{
    Q_ASSERT(m_qrcModel);
    m_qrcModel->addFiles(prefixIndex, fileNames, cursorFile, firstFile, lastFile);

    // Expand prefix node
    const QModelIndex prefixModelIndex = m_qrcModel->index(prefixIndex, 0, QModelIndex());
    if (prefixModelIndex.isValid())
        this->setExpanded(prefixModelIndex, true);
}

void ResourceView::removeFiles(int prefixIndex, int firstFileIndex, int lastFileIndex)
{
    Q_ASSERT(prefixIndex >= 0 && prefixIndex < m_qrcModel->rowCount(QModelIndex()));
    const QModelIndex prefixModelIndex = m_qrcModel->index(prefixIndex, 0, QModelIndex());
    Q_ASSERT(prefixModelIndex != QModelIndex());
    Q_ASSERT(firstFileIndex >= 0 && firstFileIndex < m_qrcModel->rowCount(prefixModelIndex));
    Q_ASSERT(lastFileIndex >= 0 && lastFileIndex < m_qrcModel->rowCount(prefixModelIndex));

    for (int i = lastFileIndex; i >= firstFileIndex; i--) {
        const QModelIndex index = m_qrcModel->index(i, 0, prefixModelIndex);
        delete removeEntry(index);
    }
}

void ResourceView::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Delete)
        removeItem();
    else
        QTreeView::keyPressEvent(e);
}

QModelIndex ResourceView::addPrefix()
{
    const QModelIndex idx = m_qrcModel->addNewPrefix();
    selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect);
    return idx;
}

void ResourceView::refresh()
{
    m_qrcModel->refresh();
    QModelIndex idx = currentIndex();
    setModel(0);
    setModel(m_qrcModel);
    setCurrentIndex(idx);
    expandAll();
}

QStringList ResourceView::fileNamesToAdd()
{
    return QFileDialog::getOpenFileNames(this, tr("Open File"),
            m_qrcModel->absolutePath(QString()),
            tr("All files (*)"));
}

void ResourceView::addFiles(QStringList fileList, const QModelIndex &index)
{
    if (fileList.isEmpty())
        return;
    QModelIndex idx = index;
    if (!m_qrcModel->hasChildren(QModelIndex())) {
        idx = addPrefix();
        expand(idx);
    }

    idx = m_qrcModel->addFiles(idx, fileList);

    if (idx.isValid()) {
        const QModelIndex preindex = m_qrcModel->prefixIndex(index);
        setExpanded(preindex, true);
        selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect);
        QString prefix, file;
        m_qrcModel->getItem(preindex, prefix, file);
// XXX        emit filesAdded(prefix, fileList);
    }
}

void ResourceView::addFile(const QString &prefix, const QString &file)
{
    const QModelIndex preindex = m_qrcModel->getIndex(prefix, QString());
    addFiles(QStringList(file), preindex);
}

bool ResourceView::load(const QString &fileName)
{
    const QFileInfo fi(fileName);
    m_qrcModel->setFileName(fi.absoluteFilePath());

    if (!fi.exists())
        return false;

    return m_qrcModel->reload();
}

bool ResourceView::save()
{
    return m_qrcModel->save();
}

QString ResourceView::currentAlias() const
{
    const QModelIndex current = currentIndex();
    if (!current.isValid())
        return QString();
    return m_qrcModel->alias(current);
}

QString ResourceView::currentPrefix() const
{
    const QModelIndex current = currentIndex();
    if (!current.isValid())
        return QString();
    const QModelIndex preindex = m_qrcModel->prefixIndex(current);
    QString prefix, file;
    m_qrcModel->getItem(preindex, prefix, file);
    return prefix;
}

QString ResourceView::currentLanguage() const
{
    const QModelIndex current = currentIndex();
    if (!current.isValid())
        return QString();
    const QModelIndex preindex = m_qrcModel->prefixIndex(current);
    return m_qrcModel->lang(preindex);
}

QString ResourceView::currentResourcePath() const
{
    const QModelIndex current = currentIndex();
    if (!current.isValid())
        return QString();

    const QString alias = m_qrcModel->alias(current);
    if (!alias.isEmpty())
        return QLatin1Char(':') + currentPrefix() + QLatin1Char('/') + alias;

    return QLatin1Char(':') + currentPrefix() + QLatin1Char('/') + m_qrcModel->relativePath(m_qrcModel->file(current));
}

QString ResourceView::getCurrentValue(NodeProperty property) const
{
    switch (property) {
    case AliasProperty: return currentAlias();
    case PrefixProperty: return currentPrefix();
    case LanguageProperty: return currentLanguage();
    default: Q_ASSERT(false); return QString(); // Kill warning
    }
}

void ResourceView::changeValue(const QModelIndex &nodeIndex, NodeProperty property,
        const QString &value)
{
    switch (property) {
    case AliasProperty: m_qrcModel->changeAlias(nodeIndex, value); return;
    case PrefixProperty: m_qrcModel->changePrefix(nodeIndex, value); return;
    case LanguageProperty: m_qrcModel->changeLang(nodeIndex, value); return;
    default: Q_ASSERT(false);
    }
}

void ResourceView::itemActivated(const QModelIndex &index)
{
    const QString fileName = m_qrcModel->file(index);
    if (fileName.isEmpty())
        return;
    emit itemActivated(fileName);
}

void ResourceView::showContextMenu(const QPoint &pos)
{
    const QModelIndex index = indexAt(pos);
    const QString fileName = m_qrcModel->file(index);
    if (fileName.isEmpty())
        return;
    emit showContextMenu(mapToGlobal(pos), fileName);
}

void ResourceView::advanceMergeId()
{
    m_mergeId++;
    if (m_mergeId < 0)
        m_mergeId = 0;
}

void ResourceView::addUndoCommand(const QModelIndex &nodeIndex, NodeProperty property,
        const QString &before, const QString &after)
{
    QUndoCommand * const command = new ModifyPropertyCommand(this, nodeIndex, property,
            m_mergeId, before, after);
    m_history->push(command);
}

void ResourceView::setCurrentAlias(const QString &before, const QString &after)
{
    const QModelIndex current = currentIndex();
    if (!current.isValid())
        return;

    addUndoCommand(current, AliasProperty, before, after);
}

void ResourceView::setCurrentPrefix(const QString &before, const QString &after)
{
    const QModelIndex current = currentIndex();
    if (!current.isValid())
        return;
    const QModelIndex preindex = m_qrcModel->prefixIndex(current);

    addUndoCommand(preindex, PrefixProperty, before, after);
}

void ResourceView::setCurrentLanguage(const QString &before, const QString &after)
{
    const QModelIndex current = currentIndex();
    if (!current.isValid())
        return;
    const QModelIndex preindex = m_qrcModel->prefixIndex(current);

    addUndoCommand(preindex, LanguageProperty, before, after);
}

bool ResourceView::isPrefix(const QModelIndex &index) const
{
    if (!index.isValid())
        return false;
    const QModelIndex preindex = m_qrcModel->prefixIndex(index);
    if (preindex == index)
        return true;
    return false;
}

QString ResourceView::fileName() const
{
    return m_qrcModel->fileName();
}

void ResourceView::setFileName(const QString &fileName)
{
    m_qrcModel->setFileName(fileName);
}

void ResourceView::setResourceDragEnabled(bool e)
{
    setDragEnabled(e);
    m_qrcModel->setResourceDragEnabled(e);
}

bool ResourceView::resourceDragEnabled() const
{
    return m_qrcModel->resourceDragEnabled();
}
