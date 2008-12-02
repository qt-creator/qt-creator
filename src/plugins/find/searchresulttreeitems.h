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

    itemType getItemType(void) const;
    const SearchResultTreeItem *getParent(void) const;
    const SearchResultTreeItem *getChild(int index) const;
    void appendChild(SearchResultTreeItem *child);
    int getChildrenCount(void) const;
    int getRowOfItem(void) const;
    void clearChildren(void);

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
    QString getFileName(void) const;
    void appendResultLine(int index, int lineNumber, const QString &rowText, int searchTermStart,
        int searchTermLength);

private:
    QString m_fileName;
};

} //Internal
} //Find

#endif
