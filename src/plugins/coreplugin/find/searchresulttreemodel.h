// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "searchresultwindow.h"

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
    void setTextEditorFont(const QFont &font, const Utils::SearchResultColors &colors);
    QList<QModelIndex> addResults(const Utils::SearchResultItems &items,
                                  SearchResult::AddMode mode);
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
