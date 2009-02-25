/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "searchresulttreeitems.h"

using namespace Find::Internal;

SearchResultTreeItem::SearchResultTreeItem(SearchResultTreeItem::itemType type, const SearchResultTreeItem *parent)
  : m_type(type), m_parent(parent)
{
}

SearchResultTreeItem::~SearchResultTreeItem()
{
    clearChildren();
}

void SearchResultTreeItem::clearChildren(void)
{
    qDeleteAll(m_children);
    m_children.clear();
}

SearchResultTreeItem::itemType SearchResultTreeItem::getItemType(void) const
{
    return m_type;
}

int SearchResultTreeItem::getChildrenCount(void) const
{
    return m_children.count();
}

int SearchResultTreeItem::getRowOfItem(void) const
{
    return (m_parent?m_parent->m_children.indexOf(const_cast<SearchResultTreeItem*>(this)):0);
}

const SearchResultTreeItem* SearchResultTreeItem::getChild(int index) const
{
    return m_children.at(index);
}

const SearchResultTreeItem *SearchResultTreeItem::getParent(void) const
{
    return m_parent;
}

void SearchResultTreeItem::appendChild(SearchResultTreeItem *child)
{
    m_children.append(child);
}

SearchResultTextRow::SearchResultTextRow(int index, int lineNumber,
    const QString &rowText, int searchTermStart, int searchTermLength, const SearchResultTreeItem *parent)
:   SearchResultTreeItem(resultRow, parent),
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

SearchResultFile::SearchResultFile(const QString &fileName, const SearchResultTreeItem *parent)
  : SearchResultTreeItem(resultFile, parent), m_fileName(fileName)
{
}

QString SearchResultFile::getFileName(void) const
{
    return m_fileName;
}

void SearchResultFile::appendResultLine(int index, int lineNumber, const QString &rowText, int searchTermStart,
        int searchTermLength)
{
    SearchResultTreeItem *child = new SearchResultTextRow(index, lineNumber, rowText, searchTermStart, searchTermLength, this);
    appendChild(child);
}
