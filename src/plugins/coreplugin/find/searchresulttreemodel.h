/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

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

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

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
