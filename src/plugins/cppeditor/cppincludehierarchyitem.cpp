/****************************************************************************
**
** Copyright (C) 2016 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>
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

#include "cppincludehierarchyitem.h"

namespace CppEditor {
namespace Internal {

CppIncludeHierarchyItem::CppIncludeHierarchyItem(const QString &filePath,
                                                 CppIncludeHierarchyItem *parent,
                                                 bool isCyclic)
    : m_fileName(filePath.mid(filePath.lastIndexOf(QLatin1Char('/')) + 1))
    , m_filePath(filePath)
    , m_parentItem(parent)
    , m_isCyclic(isCyclic)
    , m_hasChildren(false)
    , m_line(0)
{
}

CppIncludeHierarchyItem::~CppIncludeHierarchyItem()
{
    removeChildren();
}

const QString &CppIncludeHierarchyItem::fileName() const
{
    return m_fileName;
}

const QString &CppIncludeHierarchyItem::filePath() const
{
    return m_filePath;
}

CppIncludeHierarchyItem *CppIncludeHierarchyItem::parent() const
{
    return m_parentItem;
}

bool CppIncludeHierarchyItem::isCyclic() const
{
    return m_isCyclic;
}

void CppIncludeHierarchyItem::appendChild(CppIncludeHierarchyItem *childItem)
{
    m_childItems.append(childItem);
}

CppIncludeHierarchyItem *CppIncludeHierarchyItem::child(int row)
{
    return m_childItems.at(row);
}

int CppIncludeHierarchyItem::row() const
{
    if (m_parentItem)
        return m_parentItem->m_childItems.indexOf(const_cast<CppIncludeHierarchyItem*>(this));

    return 0;
}

int CppIncludeHierarchyItem::childCount() const
{
    return m_childItems.size();
}

void CppIncludeHierarchyItem::removeChildren()
{
    qDeleteAll(m_childItems);
    m_childItems.clear();
}

bool CppIncludeHierarchyItem::needChildrenPopulate() const
{
    return m_hasChildren && m_childItems.isEmpty();
}

bool CppIncludeHierarchyItem::hasChildren() const
{
    return m_hasChildren;
}

void CppIncludeHierarchyItem::setHasChildren(bool hasChildren)
{
    m_hasChildren = hasChildren;
}

int CppIncludeHierarchyItem::line() const
{
    return m_line;
}

void CppIncludeHierarchyItem::setLine(int line)
{
    m_line = line;
}

} // namespace Internal
} // namespace CppEditor
