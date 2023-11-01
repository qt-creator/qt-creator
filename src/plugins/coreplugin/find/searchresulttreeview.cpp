// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "searchresulttreeview.h"
#include "searchresulttreeitemroles.h"
#include "searchresulttreemodel.h"
#include "searchresulttreeitemdelegate.h"

#include <utils/qtcassert.h>
#include <utils/searchresultitem.h>

#include <QHeaderView>
#include <QKeyEvent>
#include <QVBoxLayout>

using namespace Utils;

namespace Core {
namespace Internal {

class FilterWidget : public QWidget
{
public:
    FilterWidget(QWidget *parent, QWidget *content) : QWidget(parent, Qt::Popup)
    {
        setAttribute(Qt::WA_DeleteOnClose);
        const auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(2);
        layout->addWidget(content);
        setLayout(layout);
        move(parent->mapToGlobal(QPoint(0, -sizeHint().height())));
    }
};

SearchResultTreeView::SearchResultTreeView(QWidget *parent)
    : TreeView(parent)
    , m_model(new SearchResultFilterModel(this))
    , m_autoExpandResults(false)
{
    setModel(m_model);
    connect(m_model, &SearchResultFilterModel::filterInvalidated,
            this, &SearchResultTreeView::filterInvalidated);

    setItemDelegate(new SearchResultTreeItemDelegate(8, this));
    setIndentation(14);
    setExpandsOnDoubleClick(true);
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    header()->setStretchLastSection(false);
    header()->hide();

    connect(this, &SearchResultTreeView::activated,
            this, &SearchResultTreeView::emitJumpToSearchResult);
}

void SearchResultTreeView::setAutoExpandResults(bool expand)
{
    m_autoExpandResults = expand;
}

void SearchResultTreeView::setTextEditorFont(const QFont &font, const SearchResultColors &colors)
{
    m_model->setTextEditorFont(font, colors);

    QPalette p;
    p.setColor(QPalette::Base, colors.value(SearchResultColor::Style::Default).textBackground);
    setPalette(p);
}

void SearchResultTreeView::clear()
{
    m_model->clear();
}

void SearchResultTreeView::addResults(const SearchResultItems &items, SearchResult::AddMode mode)
{
    const QList<QModelIndex> addedParents = m_model->addResults(items, mode);
    if (m_autoExpandResults && !addedParents.isEmpty()) {
        for (const QModelIndex &index : addedParents)
            setExpanded(index, true);
    }
}

void SearchResultTreeView::setFilter(SearchResultFilter *filter)
{
    m_filter = filter;
    if (m_filter)
        m_filter->setParent(this);
    m_model->setFilter(filter);
    emit filterChanged();
}

bool SearchResultTreeView::hasFilter() const
{
    return m_filter;
}

void SearchResultTreeView::showFilterWidget(QWidget *parent)
{
    QTC_ASSERT(hasFilter(), return);
    const auto optionsWidget = new FilterWidget(parent, m_filter->createWidget());
    optionsWidget->show();
}

void SearchResultTreeView::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Return
            || event->key() == Qt::Key_Enter)
            && event->modifiers() == 0
            && currentIndex().isValid()
            && state() != QAbstractItemView::EditingState) {
        const SearchResultItem item
            = model()->data(currentIndex(), ItemDataRoles::ResultItemRole).value<SearchResultItem>();
        emit jumpToSearchResult(item);
        return;
    }
    TreeView::keyPressEvent(event);
}

bool SearchResultTreeView::event(QEvent *e)
{
    if (e->type() == QEvent::Resize)
        header()->setMinimumSectionSize(width());
    return TreeView::event(e);
}

void SearchResultTreeView::emitJumpToSearchResult(const QModelIndex &index)
{
    if (model()->data(index, ItemDataRoles::IsGeneratedRole).toBool())
        return;
    SearchResultItem item = model()->data(index, ItemDataRoles::ResultItemRole).value<SearchResultItem>();

    emit jumpToSearchResult(item);
}

void SearchResultTreeView::setTabWidth(int tabWidth)
{
    auto delegate = static_cast<SearchResultTreeItemDelegate *>(itemDelegate());
    delegate->setTabWidth(tabWidth);
    doItemsLayout();
}

SearchResultFilterModel *SearchResultTreeView::model() const
{
    return m_model;
}

} // namespace Internal
} // namespace Core
