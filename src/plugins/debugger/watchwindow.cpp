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

#include "watchwindow.h"

#include "debuggeractions.h"
#include "debuggerinternalconstants.h"
#include "debuggercore.h"
#include "watchhandler.h"

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QHeaderView>
#include <QScrollBar>
#include <QTimer>

namespace Debugger {
namespace Internal {

WatchTreeView::WatchTreeView(WatchType type)
  : m_type(type), m_sliderPosition(0)
{
    setObjectName("WatchWindow");
    setWindowTitle(tr("Locals and Expressions"));
    setIndentation(indentation() * 9/10);
    setUniformRowHeights(true);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);

    connect(this, &QTreeView::expanded, this, &WatchTreeView::expandNode);
    connect(this, &QTreeView::collapsed, this, &WatchTreeView::collapseNode);
}

void WatchTreeView::expandNode(const QModelIndex &idx)
{
    model()->setData(idx, true, LocalsExpandedRole);
}

void WatchTreeView::collapseNode(const QModelIndex &idx)
{
    model()->setData(idx, false, LocalsExpandedRole);
}

void WatchTreeView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    emit currentIndexChanged(current);
    BaseTreeView::currentChanged(current, previous);
}

void WatchTreeView::setModel(QAbstractItemModel *model)
{
    BaseTreeView::setModel(model);
    setRootIndex(model->index(m_type, 0, QModelIndex()));
    setRootIsDecorated(true);
    if (header()) {
        header()->setDefaultAlignment(Qt::AlignLeft);
        if (m_type != LocalsType && m_type != InspectType)
            header()->hide();
    }

    auto watchModel = qobject_cast<WatchModelBase *>(model);
    QTC_ASSERT(watchModel, return);
    connect(model, &QAbstractItemModel::layoutChanged,
            this, &WatchTreeView::resetHelper);
    connect(watchModel, &WatchModelBase::currentIndexRequested,
            this, &QAbstractItemView::setCurrentIndex);
    connect(watchModel, &WatchModelBase::itemIsExpanded,
            this, &WatchTreeView::handleItemIsExpanded);
    if (m_type == LocalsType) {
        connect(watchModel, &WatchModelBase::updateStarted,
                this, &WatchTreeView::showProgressIndicator);
        connect(watchModel, &WatchModelBase::updateFinished,
                this, &WatchTreeView::hideProgressIndicator);
    }
}

void WatchTreeView::handleItemIsExpanded(const QModelIndex &idx)
{
    bool on = idx.data(LocalsExpandedRole).toBool();
    QTC_ASSERT(on, return);
    if (!isExpanded(idx))
        expand(idx);
}

void WatchTreeView::reexpand(QTreeView *view, const QModelIndex &idx)
{
    if (idx.data(LocalsExpandedRole).toBool()) {
        //qDebug() << "EXPANDING " << view->model()->data(idx, LocalsINameRole);
        if (!view->isExpanded(idx)) {
            view->expand(idx);
            for (int i = 0, n = view->model()->rowCount(idx); i != n; ++i) {
                QModelIndex idx1 = view->model()->index(i, 0, idx);
                reexpand(view, idx1);
            }
        }
    } else {
        //qDebug() << "COLLAPSING " << view->model()->data(idx, LocalsINameRole);
        if (view->isExpanded(idx))
            view->collapse(idx);
    }
}

void WatchTreeView::resetHelper()
{
    QModelIndex idx = model()->index(m_type, 0);
    reexpand(this, idx);
    expand(idx);
}

void WatchTreeView::reset()
{
    BaseTreeView::reset();
    if (model()) {
        setRootIndex(model()->index(m_type, 0));
        resetHelper();
    }
}

void WatchTreeView::doItemsLayout()
{
    if (m_sliderPosition == 0)
        m_sliderPosition = verticalScrollBar()->sliderPosition();
    Utils::BaseTreeView::doItemsLayout();
    if (m_sliderPosition)
        QTimer::singleShot(0, this, &WatchTreeView::adjustSlider);
}

void WatchTreeView::adjustSlider()
{
    if (m_sliderPosition) {
        verticalScrollBar()->setSliderPosition(m_sliderPosition);
        m_sliderPosition = 0;
    }
}

} // namespace Internal
} // namespace Debugger
