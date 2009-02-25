/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "searchresulttreeview.h"
#include "searchresulttreeitemroles.h"
#include "searchresulttreemodel.h"
#include "searchresulttreeitemdelegate.h"

#include <QtGui/QHeaderView>

using namespace Find::Internal;

SearchResultTreeView::SearchResultTreeView(QWidget *parent)
  : QTreeView(parent), m_autoExpandResults(false)
{
    m_model = new SearchResultTreeModel(this);
    setModel(m_model);
    setItemDelegate(new SearchResultTreeItemDelegate(this));

    setIndentation(14);
    header()->hide();

    connect (this, SIGNAL(activated(QModelIndex)), this, SLOT(emitJumpToSearchResult(QModelIndex)));
}

void SearchResultTreeView::setAutoExpandResults(bool expand)
{
    m_autoExpandResults = expand;
}

void SearchResultTreeView::clear(void)
{
    m_model->clear();
}

void SearchResultTreeView::appendResultLine(int index, const QString &fileName, int lineNumber, const QString &rowText,
    int searchTermStart, int searchTermLength)
{
    int rowsBefore = m_model->rowCount();
    m_model->appendResultLine(index, fileName, lineNumber, rowText, searchTermStart, searchTermLength);
    int rowsAfter = m_model->rowCount();

    if (m_autoExpandResults && (rowsAfter > rowsBefore))
        setExpanded(model()->index(model()->rowCount() - 1, 0), true);
}

void SearchResultTreeView::emitJumpToSearchResult(const QModelIndex &index)
{
    if (model()->data(index, ItemDataRoles::TypeRole).toString().compare("row") != 0)
        return;

    QString fileName = model()->data(index, ItemDataRoles::FileNameRole).toString();
    int position = model()->data(index, ItemDataRoles::ResultIndexRole).toInt();
    int lineNumber = model()->data(index, ItemDataRoles::ResultLineNumberRole).toInt();
    int searchTermStart = model()->data(index, ItemDataRoles::SearchTermStartRole).toInt();
    int searchTermLength = model()->data(index, ItemDataRoles::SearchTermLengthRole).toInt();

    emit jumpToSearchResult(position, fileName, lineNumber, searchTermStart, searchTermLength);
}

void SearchResultTreeView::keyPressEvent(QKeyEvent *e)
{
    if (!e->modifiers() && e->key() == Qt::Key_Return) {
        emit activated(currentIndex());
        e->accept();
        return;
    }
    QTreeView::keyPressEvent(e);
}
