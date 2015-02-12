/****************************************************************************
**
** Copyright (C) 2015 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>
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
