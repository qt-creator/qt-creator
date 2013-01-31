/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "contentwindow.h"

#include "centralwidget.h"
#include "helpviewer.h"
#include "localhelpmanager.h"
#include "openpagesmanager.h"

#include <QLayout>
#include <QFocusEvent>
#include <QMenu>

#include <QHelpEngine>
#include <QHelpContentWidget>

using namespace Help::Internal;

ContentWindow::ContentWindow()
    : m_contentWidget(0)
    , m_expandDepth(-2)
{
    m_contentWidget = (&LocalHelpManager::helpEngine())->contentWidget();
    m_contentWidget->installEventFilter(this);
    m_contentWidget->viewport()->installEventFilter(this);
    m_contentWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    setFocusProxy(m_contentWidget);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_contentWidget);

    connect(m_contentWidget, SIGNAL(customContextMenuRequested(QPoint)), this,
        SLOT(showContextMenu(QPoint)));
    connect(m_contentWidget, SIGNAL(linkActivated(QUrl)), this,
        SIGNAL(linkActivated(QUrl)));

    QHelpContentModel *contentModel =
        qobject_cast<QHelpContentModel*>(m_contentWidget->model());
    connect(contentModel, SIGNAL(contentsCreated()), this, SLOT(expandTOC()));

    m_contentWidget->setFrameStyle(QFrame::NoFrame);
}

ContentWindow::~ContentWindow()
{
}

bool ContentWindow::syncToContent(const QUrl& url)
{
    QModelIndex idx = m_contentWidget->indexOf(url);
    if (!idx.isValid())
        return false;
    m_contentWidget->setCurrentIndex(idx);
    return true;
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
    if (m_contentWidget && o == m_contentWidget->viewport()
        && e->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *me = static_cast<QMouseEvent*>(e);
        QItemSelectionModel *sm = m_contentWidget->selectionModel();
        if (!me || !sm)
            return QWidget::eventFilter(o, e);

        Qt::MouseButtons button = me->button();
        const QModelIndex &index = m_contentWidget->indexAt(me->pos());

        if (index.isValid() && sm->isSelected(index)) {
            if ((button == Qt::LeftButton && (me->modifiers() & Qt::ControlModifier))
                || (button == Qt::MidButton)) {
                QHelpContentModel *contentModel =
                    qobject_cast<QHelpContentModel*>(m_contentWidget->model());
                if (contentModel) {
                    QHelpContentItem *itm = contentModel->contentItemAt(index);
                    if (itm && HelpViewer::canOpenPage(itm->url().path()))
                        OpenPagesManager::instance().createPage(itm->url());
                }
            } else if (button == Qt::LeftButton) {
                itemClicked(index);
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
    QAction *newTab = menu.addAction(tr("Open Link as New Page"));
    if (!HelpViewer::canOpenPage(itm->url().path()))
        newTab->setEnabled(false);

    menu.move(m_contentWidget->mapToGlobal(pos));

    QAction *action = menu.exec();
    if (curTab == action)
        emit linkActivated(itm->url());
    else if (newTab == action)
        OpenPagesManager::instance().createPage(itm->url());
}

void ContentWindow::itemClicked(const QModelIndex &index)
{
    QHelpContentModel *contentModel =
        qobject_cast<QHelpContentModel*>(m_contentWidget->model());

    if (contentModel) {
        if (QHelpContentItem *itm = contentModel->contentItemAt(index)) {
            const QUrl &url = itm->url();
            if (url != CentralWidget::instance()->currentHelpViewer()->source())
                emit linkActivated(itm->url());
        }
    }
}
