// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "assetimportupdatetreemodel.h"
#include "assetimportupdatetreeitem.h"

#include <QSize>

namespace QmlDesigner {
namespace Internal {

AssetImportUpdateTreeModel::AssetImportUpdateTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_rootItem = new AssetImportUpdateTreeItem {{}};
}

AssetImportUpdateTreeModel::~AssetImportUpdateTreeModel()
{
    delete m_rootItem;
}

Qt::ItemFlags AssetImportUpdateTreeModel::flags(const QModelIndex &idx) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(idx);

    if (idx.isValid())
        flags |= Qt::ItemIsUserCheckable;

    return flags;
}

QModelIndex AssetImportUpdateTreeModel::index(int row, int column,
                                              const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    const AssetImportUpdateTreeItem *parentItem;

    parentItem = parent.isValid() ? treeItemAtIndex(parent) : m_rootItem;

    const AssetImportUpdateTreeItem *childItem = parentItem->childAt(row);
    if (childItem)
        return createIndex(row, column, const_cast<AssetImportUpdateTreeItem *>(childItem));
    else
        return QModelIndex();
}

QModelIndex AssetImportUpdateTreeModel::index(AssetImportUpdateTreeItem *item) const
{
    return createIndex(item->rowOfItem(), 0, item);
}

QVariant AssetImportUpdateTreeModel::data(const AssetImportUpdateTreeItem *row, int role) const
{
    if (role == Qt::DisplayRole)
        return row->fileInfo().fileName();
    if (role == Qt::CheckStateRole)
        return row->checkState();
    if (role == Qt::ToolTipRole)
        return row->fileInfo().absoluteFilePath();
    return {};
}

QModelIndex AssetImportUpdateTreeModel::parent(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QModelIndex();

    const AssetImportUpdateTreeItem *childItem = treeItemAtIndex(idx);
    const AssetImportUpdateTreeItem *parentItem = childItem->parent();

    if (parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->rowOfItem(), 0, const_cast<AssetImportUpdateTreeItem *>(parentItem));
}

int AssetImportUpdateTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    return parent.isValid() ? treeItemAtIndex(parent)->childCount()
                            : m_rootItem->childCount();
}

int AssetImportUpdateTreeModel::columnCount(const QModelIndex &) const
{
    return 1;
}

AssetImportUpdateTreeItem *AssetImportUpdateTreeModel::treeItemAtIndex(const QModelIndex &idx)
{
    return static_cast<AssetImportUpdateTreeItem*>(idx.internalPointer());
}

QVariant AssetImportUpdateTreeModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid())
        return {};

    if (role == Qt::SizeHintRole)
        return QSize(0, 20);

    return data(treeItemAtIndex(idx), role);
}

bool AssetImportUpdateTreeModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole) {
        auto checkState = static_cast<Qt::CheckState>(value.toInt());
        return setCheckState(idx, checkState);
    }
    return QAbstractItemModel::setData(idx, value, role);
}

bool AssetImportUpdateTreeModel::setCheckState(const QModelIndex &idx, Qt::CheckState checkState,
                                               bool firstCall)
{
    AssetImportUpdateTreeItem *item = treeItemAtIndex(idx);
    if (item->checkState() == checkState)
        return false;
    item->setCheckState(checkState);
    if (firstCall) {
        emit dataChanged(idx, idx);
        // check parents
        AssetImportUpdateTreeItem *parent = item->parent();
        QModelIndex parentIdx = idx.parent();
        while (parent) {
            bool hasChecked = false;
            bool hasUnchecked = false;
            for (const auto child : parent->children()) {
                if (child->checkState() == Qt::Checked) {
                    hasChecked = true;
                } else if (child->checkState() == Qt::Unchecked) {
                    hasUnchecked = true;
                } else if (child->checkState() == Qt::PartiallyChecked) {
                    hasChecked = true;
                    hasUnchecked = true;
                }
            }
            if (hasChecked && hasUnchecked)
                parent->setCheckState(Qt::PartiallyChecked);
            else if (hasChecked)
                parent->setCheckState(Qt::Checked);
            else
                parent->setCheckState(Qt::Unchecked);
            emit dataChanged(parentIdx, parentIdx);
            parent = parent->parent();
            parentIdx = parentIdx.parent();
        }
    }
    // check children
    if (int children = item->childCount()) {
        for (int i = 0; i < children; ++i)
            setCheckState(index(i, 0, idx), checkState, false);
        emit dataChanged(index(0, 0, idx), index(children - 1, 0, idx));
    }
    return true;
}

void AssetImportUpdateTreeModel::createItems(const QList<QFileInfo> &infos,
                                             const QSet<QString> &preselectedFiles)
{
    beginResetModel();
    if (!infos.isEmpty()) {
        QHash<QString, AssetImportUpdateTreeItem *> dirItems;
        for (const auto &info : infos) {
            auto parent = dirItems.value(info.absolutePath());
            if (!parent)
                parent = m_rootItem;
            auto item = new AssetImportUpdateTreeItem(info, parent);
            if (info.isDir()) {
                dirItems.insert(info.absoluteFilePath(), item);
            } else {
                m_fileItems.append(item);
                if (preselectedFiles.contains(info.absoluteFilePath()))
                    item->setCheckState(Qt::Checked);
            }
        }
        // Remove dir items that have no children from the model
        for (auto dirItem : std::as_const(dirItems)) {
            if (dirItem->childCount() == 0)
                delete dirItem;
        }
        std::function<Qt::CheckState (AssetImportUpdateTreeItem *)> updateDirCheckStatesRecursive;
        updateDirCheckStatesRecursive = [&](AssetImportUpdateTreeItem *item) -> Qt::CheckState {
            bool hasChecked = false;
            bool hasUnchecked = false;
            for (const auto child : item->children()) {
                Qt::CheckState childState = child->childCount() > 0
                        ? updateDirCheckStatesRecursive(child)
                        : child->checkState();
                if (childState == Qt::Checked) {
                    hasChecked = true;
                } else if (childState == Qt::Unchecked) {
                    hasUnchecked = true;
                } else {
                    hasChecked = true;
                    hasUnchecked = true;
                    break;
                }
            }
            Qt::CheckState retval = Qt::Unchecked;
            if (hasChecked && hasUnchecked)
                retval = Qt::PartiallyChecked;
            else if (hasChecked)
                retval = Qt::Checked;
            item->setCheckState(retval);
            return retval;
        };
        m_rootItem->setCheckState(updateDirCheckStatesRecursive(m_rootItem));
    }
    endResetModel();
}

QStringList AssetImportUpdateTreeModel::checkedFiles() const
{
    QStringList retList;

    for (const auto item : std::as_const(m_fileItems)) {
        if (item->checkState() == Qt::Checked)
            retList.append(item->fileInfo().absoluteFilePath());
    }

    return retList;
}

void AssetImportUpdateTreeModel::clear()
{
    beginResetModel();
    m_fileItems.clear();
    m_rootItem->clear(); // Deletes all children
    endResetModel();
}

} // namespace Internal
} // namespace QmlDesigner
