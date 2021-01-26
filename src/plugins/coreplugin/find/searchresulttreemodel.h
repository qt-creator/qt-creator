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

#include <QFont>
#include <QSortFilterProxyModel>

#include <functional>

namespace Core {
namespace Internal {

class SearchResultTreeItem;
class SearchResultTreeModel;

class SearchResultFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    SearchResultFilterModel(QObject *parent = nullptr);

    void setFilter(SearchResultFilter *filter);
    void setShowReplaceUI(bool show);
    void setTextEditorFont(const QFont &font, const SearchResultColors &colors);
    QList<QModelIndex> addResults(const QList<SearchResultItem> &items, SearchResult::AddMode mode);
    void clear();
    QModelIndex next(const QModelIndex &idx, bool includeGenerated = false,
                     bool *wrapped = nullptr) const;
    QModelIndex prev(const QModelIndex &idx, bool includeGenerated = false,
                     bool *wrapped = nullptr) const;

    SearchResultTreeItem *itemForIndex(const QModelIndex &index) const;

signals:
    void filterInvalidated();

private:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

    QModelIndex nextOrPrev(const QModelIndex &idx, bool *wrapped,
                           const std::function<QModelIndex(const QModelIndex &)> &func) const;
    SearchResultTreeModel *sourceModel() const;

    SearchResultFilter *m_filter = nullptr;
};

} // namespace Internal
} // namespace Core
