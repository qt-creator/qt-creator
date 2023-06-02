// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "searchresultwindow.h"

#include <utils/itemviews.h>
#include <utils/searchresultitem.h>

namespace Core {

namespace Internal {

class SearchResultFilterModel;

class SearchResultTreeView : public Utils::TreeView
{
    Q_OBJECT

public:
    explicit SearchResultTreeView(QWidget *parent = nullptr);

    void setAutoExpandResults(bool expand);
    void setTextEditorFont(const QFont &font, const Utils::SearchResultColors &colors);
    void setTabWidth(int tabWidth);

    SearchResultFilterModel *model() const;
    void addResults(const Utils::SearchResultItems &items, SearchResult::AddMode mode);
    void setFilter(SearchResultFilter *filter);
    bool hasFilter() const;
    void showFilterWidget(QWidget *parent);

    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *e) override;

signals:
    void jumpToSearchResult(const Utils::SearchResultItem &item);
    void filterInvalidated();
    void filterChanged();

public slots:
    void clear();
    void emitJumpToSearchResult(const QModelIndex &index);

protected:
    SearchResultFilterModel *m_model;
    SearchResultFilter *m_filter = nullptr;
    bool m_autoExpandResults;
};

} // namespace Internal
} // namespace Core
