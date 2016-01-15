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

#include "contentwindow.h"

#include <centralwidget.h>
#include <helpviewer.h>
#include <localhelpmanager.h>
#include <openpagesmanager.h>

#include <utils/navigationtreeview.h>

#include <QLayout>
#include <QFocusEvent>
#include <QMenu>

#include <QHelpEngine>
#include <QHelpContentModel>

using namespace Help::Internal;

ContentWindow::ContentWindow()
    : m_contentWidget(0)
    , m_expandDepth(-2)
    , m_isOpenInNewPageActionVisible(true)
{
    m_contentModel = (&LocalHelpManager::helpEngine())->contentModel();
    m_contentWidget = new Utils::NavigationTreeView;
    m_contentWidget->setModel(m_contentModel);
    m_contentWidget->setActivationMode(Utils::SingleClickActivation);
    m_contentWidget->installEventFilter(this);
    m_contentWidget->viewport()->installEventFilter(this);
    m_contentWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    setFocusProxy(m_contentWidget);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_contentWidget);

    connect(m_contentWidget, &QWidget::customContextMenuRequested,
            this, &ContentWindow::showContextMenu);
    connect(m_contentWidget, &QTreeView::activated,
            this, &ContentWindow::itemActivated);

    connect(m_contentModel, &QHelpContentModel::contentsCreated,
            this, &ContentWindow::expandTOC);
}

ContentWindow::~ContentWindow()
{
}

void ContentWindow::setOpenInNewPageActionVisible(bool visible)
{
    m_isOpenInNewPageActionVisible = visible;
}

void ContentWindow::expandTOC()
{
    if (m_expandDepth > -2) {
        expandToDepth(m_expandDepth);
        m_expandDepth = -2;
    }
}

void ContentWindow::expandToDepth(int depth)
{
    m_expandDepth = depth;
    if (depth == -1)
        m_contentWidget->expandAll();
    else
        m_contentWidget->expandToDepth(depth);
}

bool ContentWindow::eventFilter(QObject *o, QEvent *e)
{
    if (m_isOpenInNewPageActionVisible && m_contentWidget && o == m_contentWidget->viewport()
        && e->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *me = static_cast<QMouseEvent*>(e);
        QItemSelectionModel *sm = m_contentWidget->selectionModel();
        if (!sm)
            return QWidget::eventFilter(o, e);

        Qt::MouseButtons button = me->button();
        const QModelIndex &index = m_contentWidget->indexAt(me->pos());

        if (index.isValid() && sm->isSelected(index)) {
            if ((button == Qt::LeftButton && (me->modifiers() & Qt::ControlModifier))
                    || (button == Qt::MidButton)) {
                QHelpContentItem *itm = m_contentModel->contentItemAt(index);
                if (itm)
                    emit linkActivated(itm->url(), true/*newPage*/);
            }
        }
    }
    return QWidget::eventFilter(o, e);
}

void ContentWindow::showContextMenu(const QPoint &pos)
{
    if (!m_contentWidget->indexAt(pos).isValid())
        return;

    QHelpContentModel *contentModel =
        qobject_cast<QHelpContentModel*>(m_contentWidget->model());
    QHelpContentItem *itm =
        contentModel->contentItemAt(m_contentWidget->currentIndex());

    QMenu menu;
    QAction *curTab = menu.addAction(tr("Open Link"));
    QAction *newTab = 0;
    if (m_isOpenInNewPageActionVisible)
        newTab = menu.addAction(tr("Open Link as New Page"));

    QAction *action = menu.exec(m_contentWidget->mapToGlobal(pos));
    if (curTab == action)
        emit linkActivated(itm->url(), false/*newPage*/);
    else if (newTab && newTab == action)
        emit linkActivated(itm->url(), true/*newPage*/);
}

void ContentWindow::itemActivated(const QModelIndex &index)
{
    if (QHelpContentItem *itm = m_contentModel->contentItemAt(index))
        emit linkActivated(itm->url(), false/*newPage*/);
}
