/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SEARCHRESULTTREEMODEL_H
#define SEARCHRESULTTREEMODEL_H

#include "searchresultwindow.h"
#include "searchresultcolor.h"

#include <QAbstractItemModel>
#include <QFont>

namespace Core {
namespace Internal {

class SearchResultTreeItem;

class SearchResultTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    SearchResultTreeModel(QObject *parent = 0);
    ~SearchResultTreeModel();

    void setShowReplaceUI(bool show);
    void setTextEditorFont(const QFont &font, const SearchResultColor &color);

    Qt::ItemFlags flags(const QModelIndex &index) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    QModelIndex next(const QModelIndex &idx, bool includeGenerated = false, bool *wrapped = 0) const;
    QModelIndex prev(const QModelIndex &idx, bool includeGenerated = false, bool *wrapped = 0) const;

    QList<QModelIndex> addResults(const QList<SearchResultItem> &items, SearchResult::AddMode mode);

signals:
    void jumpToSearchResult(const QString &fileName, int lineNumber,
                            int searchTermStart, int searchTermLength);

public slots:
    void clear();

private:
    QModelIndex index(SearchResultTreeItem *item) const;
    void addResultsToCurrentParent(const QList<SearchResultItem> &items, SearchResult::AddMode mode);
    QSet<SearchResultTreeItem *> addPath(const QStringList &path);
    QVariant data(const SearchResultTreeItem *row, int role) const;
    bool setCheckState(const QModelIndex &idx, Qt::CheckState checkState, bool firstCall = true);
    QModelIndex nextIndex(const QModelIndex &idx, bool *wrapped = 0) const;
    QModelIndex prevIndex(const QModelIndex &idx, bool *wrapped = 0) const;
    SearchResultTreeItem *treeItemAtIndex(const QModelIndex &idx) const;

    SearchResultTreeItem *m_rootItem;
    SearchResultTreeItem *m_currentParent;
    SearchResultColor m_color;
    QModelIndex m_currentIndex;
    QStringList m_currentPath; // the path that belongs to the current parent
    QFont m_textEditorFont;
    bool m_showReplaceUI;
    bool m_editorFontIsUsed;
};

} // namespace Internal
} // namespace Core

#endif // SEARCHRESULTTREEMODEL_H
