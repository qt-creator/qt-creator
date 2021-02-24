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
    return m_children.count();
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
