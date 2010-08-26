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

#ifndef SEARCHRESULTTREEITEMS_H
#define SEARCHRESULTTREEITEMS_H

#include "searchresultwindow.h"

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/qnamespace.h>
#include <QtGui/QIcon>

namespace Find {
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

    bool isUserCheckable() const;
    void setIsUserCheckable(bool isUserCheckable);

    Qt::CheckState checkState() const;
    void setCheckState(Qt::CheckState checkState);

    bool isGenerated() const { return m_isGenerated; }
    void setGenerated(bool value) { m_isGenerated = value; }

    SearchResultItem item;

private:
    SearchResultTreeItem *m_parent;
    QList<SearchResultTreeItem *> m_children;
    bool m_isUserCheckable;
    Qt::CheckState m_checkState;
    bool m_isGenerated;
};

} // namespace Internal
} // namespace Find

#endif // SEARCHRESULTTREEITEMS_H
