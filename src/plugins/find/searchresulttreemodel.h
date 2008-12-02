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
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#ifndef SEARCHRESULTTREEMODEL_H
#define SEARCHRESULTTREEMODEL_H

#include <QtCore/QAbstractItemModel>

namespace Find{
namespace Internal {

class SearchResultTreeItem;
class SearchResultTextRow;
class SearchResultFile;

class SearchResultTreeModel: public QAbstractItemModel
{
    Q_OBJECT

public:
    SearchResultTreeModel(QObject *parent = 0);
    ~SearchResultTreeModel();

    QModelIndex index(int row, int column,
                              const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

signals:
    void jumpToSearchResult(const QString &fileName, int lineNumber,
        int searchTermStart, int searchTermLength);
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

public slots:
    void clear();
    void appendResultLine(int index, int lineNumber, const QString &rowText, int searchTermStart,
        int searchTermLength);
    void appendResultLine(int index, const QString &fileName, int lineNumber, const QString &rowText, int searchTermStart,
        int searchTermLength);

private:
    void appendResultFile(const QString &fileName);
    QVariant data(const SearchResultTextRow *row, int role) const;
    QVariant data(const SearchResultFile *file, int role) const;
    void initializeData(void);
    void disposeData(void);

    SearchResultTreeItem *m_rootItem;
    SearchResultFile *m_lastAppendedResultFile;
};

} //Internal
} //Find

#endif
