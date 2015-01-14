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

#ifndef CPPINCLUDEHIERARCHYITEM_H
#define CPPINCLUDEHIERARCHYITEM_H

#include <QList>
#include <QString>

namespace CppEditor {
namespace Internal {

class CppIncludeHierarchyItem
{
public:
    CppIncludeHierarchyItem(const QString &filePath, CppIncludeHierarchyItem *parent = 0,
                            bool isCyclic = false);
    virtual ~CppIncludeHierarchyItem();

    CppIncludeHierarchyItem *parent() const;
    CppIncludeHierarchyItem *child(int row);
    int childCount() const;
    void appendChild(CppIncludeHierarchyItem *childItem);
    void removeChildren();
    bool needChildrenPopulate() const;
    int row() const;
    bool isCyclic() const;

    const QString &fileName() const;
    const QString &filePath() const;
    bool hasChildren() const;
    void setHasChildren(bool hasChildren);
    int line() const;
    void setLine(int line);

private:
    QString m_fileName;
    QString m_filePath;
    QList<CppIncludeHierarchyItem *> m_childItems;
    CppIncludeHierarchyItem *m_parentItem;
    bool m_isCyclic;
    bool m_hasChildren;
    int m_line;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPINCLUDEHIERARCHYITEM_H
