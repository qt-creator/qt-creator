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

#include "searchresulttreeview.h"
#include "searchresulttreeitemroles.h"
#include "searchresulttreemodel.h"
#include "searchresulttreeitemdelegate.h"

#include <QHeaderView>
#include <QKeyEvent>

namespace Core {
namespace Internal {

SearchResultTreeView::SearchResultTreeView(QWidget *parent)
    : Utils::TreeView(parent)
    , m_model(new SearchResultTreeModel(this))
    , m_autoExpandResults(false)
{
    setModel(m_model);
    setItemDelegate(new SearchResultTreeItemDelegate(8, this));
    setIndentation(14);
    setUniformRowHeights(true);
    setExpandsOnDoubleClick(true);
    header()->hide();

    connect(this, &SearchResultTreeView::activated,
            this, &SearchResultTreeView::emitJumpToSearchResult);
}

void SearchResultTreeView::setAutoExpandResults(bool expand)
{
    m_autoExpandResults = expand;
}

void SearchResultTreeView::setTextEditorFont(const QFont &font, const SearchResultColor &color)
{
    m_model->setTextEditorFont(font, color);

    QPalette p;
    p.setColor(QPalette::Base, color.textBackground);
    setPalette(p);
}

void SearchResultTreeView::clear()
{
    m_model->clear();
}

void SearchResultTreeView::addResults(const QList<SearchResultItem> &items, SearchResult::AddMode mode)
{
    QList<QModelIndex> addedParents = m_model->addResults(items, mode);
    if (m_autoExpandResults && !addedParents.isEmpty()) {
        foreach (const QModelIndex &index, addedParents)
            setExpanded(index, true);
    }
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
    SearchResultTreeItemDelegate *delegate = static_cast<SearchResultTreeItemDelegate *>(itemDelegate());
    delegate->setTabWidth(tabWidth);
    doItemsLayout();
}

SearchResultTreeModel *SearchResultTreeView::model() const
{
    return m_model;
}

} // namespace Internal
} // namespace Core
