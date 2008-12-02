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
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#include "searchresulttreeview.h"
#include "searchresulttreeitemroles.h"
#include "searchresulttreemodel.h"
#include "searchresulttreeitemdelegate.h"

#include <QtGui/QHeaderView>

using namespace Find::Internal;

SearchResultTreeView::SearchResultTreeView(QWidget *parent)
:   QTreeView(parent)
,   m_autoExpandResults(false)
{
    m_model = new SearchResultTreeModel(this);
    setModel(m_model);
    setItemDelegate(new SearchResultTreeItemDelegate(this));

    setIndentation(14);
    header()->hide();

    connect (this, SIGNAL(activated(const QModelIndex&)), this, SLOT(emitJumpToSearchResult(const QModelIndex&)));
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
        setExpanded(model()->index(model()->rowCount()-1, 0), true);
}

void SearchResultTreeView::emitJumpToSearchResult(const QModelIndex &index)
{
    if (model()->data(index, ItemDataRoles::TypeRole).toString().compare("row") != 0)
        return;

    QString fileName(model()->data(index, ItemDataRoles::FileNameRole).toString());
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
