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

#include "indexwindow.h"

#include "topicchooser.h"

#include <centralwidget.h>
#include <helpviewer.h>
#include <localhelpmanager.h>
#include <openpagesmanager.h>

#include <utils/fancylineedit.h>
#include <utils/hostosinfo.h>
#include <utils/navigationtreeview.h>
#include <utils/styledbar.h>

#include <QAbstractItemModel>
#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QKeyEvent>
#include <QMenu>
#include <QContextMenuEvent>

#include <QHelpEngine>
#include <QHelpIndexModel>

using namespace Help::Internal;

IndexWindow::IndexWindow()
    : m_searchLineEdit(0)
    , m_indexWidget(0)
    , m_isOpenInNewPageActionVisible(true)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    m_searchLineEdit = new Utils::FancyLineEdit();
    m_searchLineEdit->setPlaceholderText(QString());
    m_searchLineEdit->setFiltering(true);
    setFocusProxy(m_searchLineEdit);
    connect(m_searchLineEdit, &QLineEdit::textChanged,
            this, &IndexWindow::filterIndices);
    m_searchLineEdit->installEventFilter(this);
    m_searchLineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);

    QLabel *l = new QLabel(tr("&Look for:"));
    l->setBuddy(m_searchLineEdit);
    layout->addWidget(l);
    layout->setMargin(0);
    layout->setSpacing(0);

    Utils::StyledBar *toolbar = new Utils::StyledBar(this);
    toolbar->setSingleRow(false);
    QLayout *tbLayout = new QHBoxLayout();
    tbLayout->setSpacing(6);
    tbLayout->setMargin(4);
    tbLayout->addWidget(l);
    tbLayout->addWidget(m_searchLineEdit);
    toolbar->setLayout(tbLayout);
    layout->addWidget(toolbar);

    QHelpIndexModel *indexModel = LocalHelpManager::helpEngine().indexModel();
    m_filteredIndexModel = new IndexFilterModel(this);
    m_filteredIndexModel->setSourceModel(indexModel);
    m_indexWidget = new Utils::NavigationTreeView(this);
    m_indexWidget->setModel(m_filteredIndexModel);
    m_indexWidget->setRootIsDecorated(false);
    m_indexWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_indexWidget->installEventFilter(this);
    connect(indexModel, &QHelpIndexModel::indexCreationStarted,
            this, &IndexWindow::disableSearchLineEdit);
    connect(indexModel, &QHelpIndexModel::indexCreated,
            this, &IndexWindow::enableSearchLineEdit);
    connect(m_indexWidget, &Utils::NavigationTreeView::activated,
            this, [this](const QModelIndex &index) { open(index); });
    connect(m_searchLineEdit, &QLineEdit::returnPressed,
            m_indexWidget, [this]() { open(m_indexWidget->currentIndex()); });
    layout->addWidget(m_indexWidget);

    m_indexWidget->viewport()->installEventFilter(this);
}

IndexWindow::~IndexWindow()
{
}

void IndexWindow::setOpenInNewPageActionVisible(bool visible)
{
    m_isOpenInNewPageActionVisible = visible;
}

void IndexWindow::filterIndices(const QString &filter)
{
    QModelIndex bestMatch;
    if (filter.contains(QLatin1Char('*')))
        bestMatch = m_filteredIndexModel->filter(filter, filter);
    else
        bestMatch = m_filteredIndexModel->filter(filter, QString());
    if (bestMatch.isValid()) {
        m_indexWidget->setCurrentIndex(bestMatch);
        m_indexWidget->scrollTo(bestMatch);
    }
}

bool IndexWindow::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == m_searchLineEdit && e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(e);
        QModelIndex idx = m_indexWidget->currentIndex();
        switch (ke->key()) {
        case Qt::Key_Up:
            idx = m_indexWidget->model()->index(idx.row()-1,
                idx.column(), idx.parent());
            if (idx.isValid())
                m_indexWidget->setCurrentIndex(idx);
            break;
        case Qt::Key_Down:
            idx = m_indexWidget->model()->index(idx.row()+1,
                idx.column(), idx.parent());
            if (idx.isValid())
                m_indexWidget->setCurrentIndex(idx);
            break;
        default: ; // stop complaining
        }
    } else if (obj == m_searchLineEdit
            && e->type() == QEvent::FocusIn
            && static_cast<QFocusEvent *>(e)->reason() != Qt::MouseFocusReason) {
        m_searchLineEdit->selectAll();
        m_searchLineEdit->setFocus();
    } else if (obj == m_indexWidget && e->type() == QEvent::ContextMenu) {
        QContextMenuEvent *ctxtEvent = static_cast<QContextMenuEvent*>(e);
        QModelIndex idx = m_indexWidget->indexAt(ctxtEvent->pos());
        if (idx.isValid()) {
            QMenu menu;
            QAction *curTab = menu.addAction(tr("Open Link"));
            QAction *newTab = 0;
            if (m_isOpenInNewPageActionVisible)
                newTab = menu.addAction(tr("Open Link as New Page"));
            menu.move(m_indexWidget->mapToGlobal(ctxtEvent->pos()));

            QAction *action = menu.exec();
            if (curTab == action)
                open(idx);
            else if (newTab && newTab == action)
                open(idx, true/*newPage*/);
        }
    } else if (m_indexWidget && obj == m_indexWidget->viewport()
        && e->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(e);
        QModelIndex idx = m_indexWidget->indexAt(mouseEvent->pos());
        if (idx.isValid()) {
            Qt::MouseButtons button = mouseEvent->button();
            if (((button == Qt::LeftButton) && (mouseEvent->modifiers() & Qt::ControlModifier))
                || (button == Qt::MidButton)) {
                open(idx);
            }
        }
    }

    return QWidget::eventFilter(obj, e);
}

void IndexWindow::enableSearchLineEdit()
{
    m_searchLineEdit->setDisabled(false);
    filterIndices(m_searchLineEdit->text());
}

void IndexWindow::disableSearchLineEdit()
{
    m_searchLineEdit->setDisabled(true);
}

void IndexWindow::open(const QModelIndex &index, bool newPage)
{
    QString keyword = m_filteredIndexModel->data(index, Qt::DisplayRole).toString();
    QMap<QString, QUrl> links = LocalHelpManager::helpEngine().indexModel()->linksForKeyword(keyword);

    if (links.size() == 1) {
        emit linkActivated(links.first(), newPage);
    } else if (links.size() > 1) {
        emit linksActivated(links, keyword, newPage);
    }
}

Qt::DropActions IndexFilterModel::supportedDragActions() const
{
    return sourceModel()->supportedDragActions();
}

QModelIndex IndexFilterModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return createIndex(row, column);
}

QModelIndex IndexFilterModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)
    return QModelIndex();
}

int IndexFilterModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_toSource.size();
}

int IndexFilterModel::columnCount(const QModelIndex &parent) const
{
    return sourceModel()->columnCount(mapToSource(parent));
}

void IndexFilterModel::setSourceModel(QAbstractItemModel *sm)
{
    QAbstractItemModel *previousModel = sourceModel();
    if (previousModel) {
        disconnect(previousModel, &QAbstractItemModel::dataChanged,
                   this, &IndexFilterModel::sourceDataChanged);
        disconnect(previousModel, &QAbstractItemModel::rowsInserted,
                   this, &IndexFilterModel::sourceRowsInserted);
        disconnect(previousModel, &QAbstractItemModel::rowsRemoved,
                   this, &IndexFilterModel::sourceRowsRemoved);
        disconnect(previousModel, &QAbstractItemModel::modelReset,
                   this, &IndexFilterModel::sourceModelReset);
    }
    QAbstractProxyModel::setSourceModel(sm);
    if (sm) {
        connect(sm, &QAbstractItemModel::dataChanged,
                this, &IndexFilterModel::sourceDataChanged);
        connect(sm, &QAbstractItemModel::rowsInserted,
                this, &IndexFilterModel::sourceRowsInserted);
        connect(sm, &QAbstractItemModel::rowsRemoved,
                this, &IndexFilterModel::sourceRowsRemoved);
        connect(sm, &QAbstractItemModel::modelReset,
                this, &IndexFilterModel::sourceModelReset);
    }
    filter(m_filter, m_wildcard);
}

QModelIndex IndexFilterModel::sibling(int row, int column, const QModelIndex &idx) const
{
    return QAbstractItemModel::sibling(row, column, idx);
}

Qt::ItemFlags IndexFilterModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

IndexFilterModel::IndexFilterModel(QObject *parent)
    : QAbstractProxyModel(parent)
{
}

QModelIndex IndexFilterModel::filter(const QString &filter, const QString &wildcard)
{
    beginResetModel();

    m_filter = filter;
    m_wildcard = wildcard;
    m_toSource.clear();

    // adapted copy from QHelpIndexModel

    if (filter.isEmpty() && wildcard.isEmpty()) {
        int count = sourceModel()->rowCount();
        m_toSource.reserve(count);
        for (int i = 0; i < count; ++i)
            m_toSource.append(i);
        endResetModel();
        return index(0, 0);
    }

    QHelpIndexModel *indexModel = qobject_cast<QHelpIndexModel *>(sourceModel());
    const QStringList indices = indexModel->stringList();
    int goodMatch = -1;
    int perfectMatch = -1;

    if (!wildcard.isEmpty()) {
        QRegExp regExp(wildcard, Qt::CaseInsensitive);
        regExp.setPatternSyntax(QRegExp::Wildcard);
        int i = 0;
        foreach (const QString &index, indices) {
            if (index.contains(regExp)) {
                m_toSource.append(i);
                if (perfectMatch == -1 && index.startsWith(filter, Qt::CaseInsensitive)) {
                    if (goodMatch == -1)
                        goodMatch = m_toSource.size() - 1;
                    if (filter.length() == index.length()){
                        perfectMatch = m_toSource.size() - 1;
                    }
                } else if (perfectMatch > -1 && index == filter) {
                    perfectMatch = m_toSource.size() - 1;
                }
            }
            ++i;
        }
    } else {
        int i = 0;
        foreach (const QString &index, indices) {
            if (index.contains(filter, Qt::CaseInsensitive)) {
                m_toSource.append(i);
                if (perfectMatch == -1 && index.startsWith(filter, Qt::CaseInsensitive)) {
                    if (goodMatch == -1)
                        goodMatch = m_toSource.size() - 1;
                    if (filter.length() == index.length()){
                        perfectMatch = m_toSource.size() - 1;
                    }
                } else if (perfectMatch > -1 && index == filter) {
                    perfectMatch = m_toSource.size() - 1;
                }
            }
            ++i;
        }
    }

    if (perfectMatch == -1)
        perfectMatch = qMax(0, goodMatch);

    endResetModel();
    return index(perfectMatch, 0, QModelIndex());
}

QModelIndex IndexFilterModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid() || proxyIndex.parent().isValid() || proxyIndex.row() >= m_toSource.size())
        return QModelIndex();
    return index(m_toSource.at(proxyIndex.row()), proxyIndex.column());
}

QModelIndex IndexFilterModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (!sourceIndex.isValid() || sourceIndex.parent().isValid())
        return QModelIndex();
    int i = m_toSource.indexOf(sourceIndex.row());
    if (i < 0)
        return QModelIndex();
    return index(i, sourceIndex.column());
}

void IndexFilterModel::sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    QModelIndex topLeftIndex = mapFromSource(topLeft);
    if (!topLeftIndex.isValid())
        topLeftIndex = index(0, topLeft.column());
    QModelIndex bottomRightIndex = mapFromSource(bottomRight);
    if (!bottomRightIndex.isValid())
        bottomRightIndex = index(0, bottomRight.column());
    emit dataChanged(topLeftIndex, bottomRightIndex);
}

void IndexFilterModel::sourceRowsRemoved(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    filter(m_filter, m_wildcard);
}

void IndexFilterModel::sourceRowsInserted(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    filter(m_filter, m_wildcard);
}
void IndexFilterModel::sourceModelReset()
{
    filter(m_filter, m_wildcard);
}
