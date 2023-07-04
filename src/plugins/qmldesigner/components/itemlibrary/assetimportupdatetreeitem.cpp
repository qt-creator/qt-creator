// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "assetimportupdatetreeitem.h"

namespace QmlDesigner {
namespace Internal {

AssetImportUpdateTreeItem::AssetImportUpdateTreeItem(const QFileInfo &info,
                                                     AssetImportUpdateTreeItem *parent)
  : m_parent(parent)
  , m_fileInfo(info)
{
    if (parent)
        parent->appendChild(this);
}

AssetImportUpdateTreeItem::~AssetImportUpdateTreeItem()
{
    if (m_parent)
        m_parent->removeChild(this);
    clear();
}

void AssetImportUpdateTreeItem::clear()
{
    qDeleteAll(m_children);
    m_children.clear();
    m_fileInfo = {};
    m_parent = nullptr;
}

int AssetImportUpdateTreeItem::childCount() const
{
    return m_children.size();
}

int AssetImportUpdateTreeItem::rowOfItem() const
{
    return m_parent ? m_parent->m_children.indexOf(const_cast<AssetImportUpdateTreeItem *>(this))
                    : 0;
}

AssetImportUpdateTreeItem *AssetImportUpdateTreeItem::childAt(int index) const
{
    return m_children.at(index);
}

AssetImportUpdateTreeItem *AssetImportUpdateTreeItem::parent() const
{
    return m_parent;
}

void AssetImportUpdateTreeItem::removeChild(AssetImportUpdateTreeItem *item)
{
    m_children.removeOne(item);
}

void AssetImportUpdateTreeItem::appendChild(AssetImportUpdateTreeItem *item)
{
    m_children.append(item);
}

} // namespace Internal
} // namespace QmlDesigner
