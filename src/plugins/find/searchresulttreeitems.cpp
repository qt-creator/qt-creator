/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#include "searchresulttreeitems.h"

using namespace Find::Internal;

SearchResultTreeItem::SearchResultTreeItem(SearchResultTreeItem::itemType type, const SearchResultTreeItem *parent)
:   m_type(type)
,   m_parent(parent)
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
:   SearchResultTreeItem(resultFile, parent)
,   m_fileName(fileName)
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
