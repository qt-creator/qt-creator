/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "searchresulttreeitems.h"

using namespace Find::Internal;

SearchResultTreeItem::SearchResultTreeItem(const Find::SearchResultItem &item,
                                           const SearchResultTreeItem *parent)
  : item(item),
  m_parent(parent),
  m_isUserCheckable(false),
  m_checkState(Qt::Unchecked),
  m_isGenerated(false)
{
}

SearchResultTreeItem::~SearchResultTreeItem()
{
    clearChildren();
}

bool SearchResultTreeItem::isLeaf() const
{
    return childrenCount() == 0 && parent() != 0;
}

bool SearchResultTreeItem::isUserCheckable() const
{
    return m_isUserCheckable;
}

void SearchResultTreeItem::setIsUserCheckable(bool isUserCheckable)
{
    m_isUserCheckable = isUserCheckable;
}

Qt::CheckState SearchResultTreeItem::checkState() const
{
    return m_checkState;
}

void SearchResultTreeItem::setCheckState(Qt::CheckState checkState)
{
    m_checkState = checkState;
}

void SearchResultTreeItem::clearChildren()
{
    qDeleteAll(m_children);
    m_children.clear();
}

int SearchResultTreeItem::childrenCount() const
{
    return m_children.count();
}

int SearchResultTreeItem::rowOfItem() const
{
    return (m_parent ? m_parent->m_children.indexOf(const_cast<SearchResultTreeItem*>(this)):0);
}

SearchResultTreeItem* SearchResultTreeItem::childAt(int index) const
{
    return m_children.at(index);
}

const SearchResultTreeItem *SearchResultTreeItem::parent() const
{
    return m_parent;
}

static bool lessThanByText(SearchResultTreeItem *a, const QString &b)
{
    return a->item.text < b;
}

int SearchResultTreeItem::insertionIndex(const QString &text, SearchResultTreeItem **existingItem) const
{
    QList<SearchResultTreeItem *>::const_iterator insertionPosition =
            qLowerBound(m_children.begin(), m_children.end(), text, lessThanByText);
    if (existingItem) {
        if (insertionPosition != m_children.end() && (*insertionPosition)->item.text == text) {
            (*existingItem) = (*insertionPosition);
        } else {
            *existingItem = 0;
        }
    }
    return insertionPosition - m_children.begin();
}

int SearchResultTreeItem::insertionIndex(const Find::SearchResultItem &item, SearchResultTreeItem **existingItem) const
{
    return insertionIndex(item.text, existingItem);
}

void SearchResultTreeItem::insertChild(int index, SearchResultTreeItem *child)
{
    m_children.insert(index, child);
}

void SearchResultTreeItem::insertChild(int index, const Find::SearchResultItem &item)
{
    SearchResultTreeItem *child = new SearchResultTreeItem(item, this);
    if (isUserCheckable()) {
        child->setIsUserCheckable(true);
        child->setCheckState(Qt::Checked);
    }
    insertChild(index, child);
}

void SearchResultTreeItem::appendChild(const Find::SearchResultItem &item)
{
    insertChild(m_children.count(), item);
}
