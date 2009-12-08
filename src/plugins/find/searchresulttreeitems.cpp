/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

SearchResultTreeItem::SearchResultTreeItem(SearchResultTreeItem::ItemType type, const SearchResultTreeItem *parent)
  : m_type(type), m_parent(parent), m_isUserCheckable(false), m_checkState(Qt::Unchecked)
{
}

SearchResultTreeItem::~SearchResultTreeItem()
{
    clearChildren();
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

SearchResultTreeItem::ItemType SearchResultTreeItem::itemType() const
{
    return m_type;
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

static bool compareResultFiles(SearchResultTreeItem *a, SearchResultTreeItem *b)
{
    return static_cast<SearchResultFile *>(a)->fileName() <
            static_cast<SearchResultFile *>(b)->fileName();
}

int SearchResultTreeItem::insertionIndex(SearchResultFile *child) const
{
    Q_ASSERT(m_type == Root);
    return qLowerBound(m_children.begin(), m_children.end(), child, compareResultFiles) - m_children.begin();
}

void SearchResultTreeItem::insertChild(int index, SearchResultTreeItem *child)
{
    m_children.insert(index, child);
}

void SearchResultTreeItem::appendChild(SearchResultTreeItem *child)
{
    m_children.append(child);
}

SearchResultTextRow::SearchResultTextRow(int index, int lineNumber,
                                         const QString &rowText,
                                         int searchTermStart, int searchTermLength,
                                         const SearchResultTreeItem *parent):
    SearchResultTreeItem(ResultRow, parent),
    m_index(index),
    m_lineNumber(lineNumber),
    m_rowText(rowText),
    m_searchTermStart(searchTermStart),
    m_searchTermLength(searchTermLength)
{
}

int SearchResultTextRow::index() const
{
    return m_index;
}

QString SearchResultTextRow::rowText() const
{
    return m_rowText;
}

int SearchResultTextRow::lineNumber() const
{
    return m_lineNumber;
}

int SearchResultTextRow::searchTermStart() const
{
    return m_searchTermStart;
}

int SearchResultTextRow::searchTermLength() const
{
    return m_searchTermLength;
}

SearchResultFile::SearchResultFile(const QString &fileName, const SearchResultTreeItem *parent):
    SearchResultTreeItem(ResultFile, parent),
    m_fileName(fileName)
{
}

QString SearchResultFile::fileName() const
{
    return m_fileName;
}

void SearchResultFile::appendResultLine(int index, int lineNumber, const QString &rowText, int searchTermStart,
                                        int searchTermLength)
{
    SearchResultTreeItem *child = new SearchResultTextRow(index, lineNumber, rowText,
                                                          searchTermStart, searchTermLength, this);
    if (isUserCheckable()) {
        child->setIsUserCheckable(true);
        child->setCheckState(Qt::Checked);
    }
    appendChild(child);
}
