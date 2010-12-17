/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SEARCHRESULTTREEMODEL_H
#define SEARCHRESULTTREEMODEL_H

#include "searchresultwindow.h"

#include <QtCore/QAbstractItemModel>
#include <QtCore/QRegExp>
#include <QtGui/QFont>
#include <QtGui/QTextDocument>

namespace Find {
namespace Internal {

class SearchResultTreeItem;

class SearchResultTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    SearchResultTreeModel(QObject *parent = 0);
    ~SearchResultTreeModel();

    void setShowReplaceUI(bool show);
    void setTextEditorFont(const QFont &font);

    Qt::ItemFlags flags(const QModelIndex &index) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    QModelIndex next(const QModelIndex &idx, bool includeGenerated = false) const;
    QModelIndex prev(const QModelIndex &idx, bool includeGenerated = false) const;

    QList<QModelIndex> addResults(const QList<SearchResultItem> &items, SearchResultWindow::AddMode mode);

    QModelIndex find(const QRegExp &expr, const QModelIndex &index, QTextDocument::FindFlags flags);
    QModelIndex find(const QString &term, const QModelIndex &index, QTextDocument::FindFlags flags);

signals:
    void jumpToSearchResult(const QString &fileName, int lineNumber,
                            int searchTermStart, int searchTermLength);

public slots:
    void clear();

private:
    QModelIndex index(SearchResultTreeItem *item) const;
    void addResultsToCurrentParent(const QList<SearchResultItem> &items, SearchResultWindow::AddMode mode);
    QSet<SearchResultTreeItem *> addPath(const QStringList &path);
    QVariant data(const SearchResultTreeItem *row, int role) const;
    bool setCheckState(const QModelIndex &idx, Qt::CheckState checkState, bool firstCall = true);
    QModelIndex nextIndex(const QModelIndex &idx) const;
    QModelIndex prevIndex(const QModelIndex &idx) const;
    SearchResultTreeItem *treeItemAtIndex(const QModelIndex &idx) const;

    SearchResultTreeItem *m_rootItem;
    SearchResultTreeItem *m_currentParent;
    QModelIndex m_currentIndex;
    QStringList m_currentPath; // the path that belongs to the current parent
    QFont m_textEditorFont;
    bool m_showReplaceUI;
};

} // namespace Internal
} // namespace Find

#endif // SEARCHRESULTTREEMODEL_H
