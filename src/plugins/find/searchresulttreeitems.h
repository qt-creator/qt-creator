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

#ifndef SEARCHRESULTTREEITEMS_H
#define SEARCHRESULTTREEITEMS_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QList>

namespace Find {
namespace Internal {

class SearchResultTreeItem;

class SearchResultTreeItem
{
public:
    enum itemType
    {
        root,
        resultRow,
        resultFile
    };

    SearchResultTreeItem(itemType type = root, const SearchResultTreeItem *parent = NULL);
    virtual ~SearchResultTreeItem();

    itemType getItemType() const;
    const SearchResultTreeItem *getParent() const;
    const SearchResultTreeItem *getChild(int index) const;
    void appendChild(SearchResultTreeItem *child);
    int getChildrenCount() const;
    int getRowOfItem() const;
    void clearChildren();

private:
    itemType m_type;
    const SearchResultTreeItem *m_parent;
    QList<SearchResultTreeItem *> m_children;
};

class SearchResultTextRow: public SearchResultTreeItem
{
public:
    SearchResultTextRow(int index, int lineNumber, const QString &rowText, int searchTermStart,
        int searchTermLength, const SearchResultTreeItem *parent);
    int index() const;
    QString rowText() const;
    int lineNumber() const;
    int searchTermStart() const;
    int searchTermLength() const;

private:
    int m_index;
    int m_lineNumber;
    QString m_rowText;
    int m_searchTermStart;
    int m_searchTermLength;
};

class SearchResultFile: public SearchResultTreeItem
{
public:
    SearchResultFile(const QString &fileName, const SearchResultTreeItem *parent);
    QString getFileName() const;
    void appendResultLine(int index, int lineNumber, const QString &rowText, int searchTermStart,
        int searchTermLength);

private:
    QString m_fileName;
};

} // namespace Internal
} // namespace Find

#endif // SEARCHRESULTTREEITEMS_H
