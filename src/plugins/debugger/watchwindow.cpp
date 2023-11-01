// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "watchwindow.h"

#include "debuggeractions.h"
#include "debuggerinternalconstants.h"
#include "debuggertr.h"
#include "watchhandler.h"

#include <utils/aspects.h>
#include <utils/qtcassert.h>

#include <QHeaderView>
#include <QScrollBar>
#include <QTimer>

namespace Debugger {
namespace Internal {

WatchTreeView::WatchTreeView(WatchType type)
  : m_type(type)
{
    setObjectName("WatchWindow");
    setWindowTitle(Tr::tr("Locals and Expressions"));
    setIndentation(indentation() * 9/10);
    setUniformRowHeights(true);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);

    connect(this, &QTreeView::expanded, this, &WatchTreeView::expandNode);
    connect(this, &QTreeView::collapsed, this, &WatchTreeView::collapseNode);

    connect(&settings().logTimeStamps, &Utils::BaseAspect::changed,
            this, &WatchTreeView::updateTimeColumn);
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
        if (m_type == ReturnType || m_type == TooltipType)
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

    updateTimeColumn();
}

void WatchTreeView::updateTimeColumn()
{
    if (header())
        header()->setSectionHidden(WatchModelBase::TimeColumn, !settings().logTimeStamps());
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
