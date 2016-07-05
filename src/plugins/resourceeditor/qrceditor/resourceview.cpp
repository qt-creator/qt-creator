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

#include "resourceview.h"

#include "undocommands_p.h"

#include <coreplugin/fileutils.h>
#include <coreplugin/icore.h>

#include <QDebug>

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenu>
#include <QMouseEvent>
#include <QUndoStack>

using namespace ResourceEditor;
using namespace ResourceEditor::Internal;

ResourceView::ResourceView(RelativeResourceModel *model, QUndoStack *history, QWidget *parent) :
    Utils::TreeView(parent),
    m_qrcModel(model),
    m_history(history),
    m_mergeId(-1)
{
    advanceMergeId();
    setModel(m_qrcModel);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setEditTriggers(EditKeyPressed);

    header()->hide();

    connect(this, &QWidget::customContextMenuRequested, this, &ResourceView::showContextMenu);
    connect(this, &QAbstractItemView::activated, this, &ResourceView::onItemActivated);
}

ResourceView::~ResourceView()
{
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
        Utils::TreeView::keyPressEvent(e);
}

QModelIndex ResourceView::addPrefix()
{
    const QModelIndex idx = m_qrcModel->addNewPrefix();
    selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect);
    return idx;
}

QList<QModelIndex> ResourceView::nonExistingFiles()
{
    return m_qrcModel->nonExistingFiles();
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

void ResourceView::onItemActivated(const QModelIndex &index)
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
    emit contextMenuShown(mapToGlobal(pos), fileName);
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

void ResourceView::setResourceDragEnabled(bool e)
{
    setDragEnabled(e);
    m_qrcModel->setResourceDragEnabled(e);
}

bool ResourceView::resourceDragEnabled() const
{
    return m_qrcModel->resourceDragEnabled();
}
