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

#pragma once

#include "searchresultwindow.h"

#include <QString>
#include <QList>

namespace Core {
namespace Internal {

class SearchResultTreeItem
{
public:
    explicit SearchResultTreeItem(const SearchResultItem &item = SearchResultItem(),
                                  SearchResultTreeItem *parent = NULL);
    virtual ~SearchResultTreeItem();

    bool isLeaf() const;
    SearchResultTreeItem *parent() const;
    SearchResultTreeItem *childAt(int index) const;
    int insertionIndex(const QString &text, SearchResultTreeItem **existingItem) const;
    int insertionIndex(const SearchResultItem &item, SearchResultTreeItem **existingItem) const;
    void insertChild(int index, SearchResultTreeItem *child);
    void insertChild(int index, const SearchResultItem &item);
    void appendChild(const SearchResultItem &item);
    int childrenCount() const;
    int rowOfItem() const;
    void clearChildren();

    Qt::CheckState checkState() const;
    void setCheckState(Qt::CheckState checkState);

    bool isGenerated() const { return m_isGenerated; }
    void setGenerated(bool value) { m_isGenerated = value; }

    SearchResultItem item;

private:
    SearchResultTreeItem *m_parent;
    QList<SearchResultTreeItem *> m_children;
    bool m_isGenerated;
    Qt::CheckState m_checkState;
};

} // namespace Internal
} // namespace Core
