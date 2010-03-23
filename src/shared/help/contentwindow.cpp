/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "contentwindow.h"

#include "centralwidget.h"
#include "helpmanager.h"

#include <QtGui/QLayout>
#include <QtGui/QFocusEvent>
#include <QtGui/QMenu>

#include <QtHelp/QHelpEngine>
#include <QtHelp/QHelpContentWidget>

ContentWindow::ContentWindow()
    : m_contentWidget(0)
    , m_expandDepth(-2)
{
    m_contentWidget = (&Help::HelpManager::helpEngine())->contentWidget();
    m_contentWidget->installEventFilter(this);
    m_contentWidget->viewport()->installEventFilter(this);
    m_contentWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    setFocusProxy(m_contentWidget);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(4);
    layout->addWidget(m_contentWidget);

    connect(m_contentWidget, SIGNAL(customContextMenuRequested(QPoint)), this,
        SLOT(showContextMenu(QPoint)));
    connect(m_contentWidget, SIGNAL(linkActivated(QUrl)), this,
        SIGNAL(linkActivated(QUrl)));

    QHelpContentModel *contentModel =
        qobject_cast<QHelpContentModel*>(m_contentWidget->model());
    connect(contentModel, SIGNAL(contentsCreated()), this, SLOT(expandTOC()));
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
        QModelIndex index = m_contentWidget->indexAt(me->pos());
        QItemSelectionModel *sm = m_contentWidget->selectionModel();

        Qt::MouseButtons button = me->button();
        if (index.isValid() && (sm && sm->isSelected(index))) {
            if ((button == Qt::LeftButton && (me->modifiers() & Qt::ControlModifier))
                || (button == Qt::MidButton)) {
                QHelpContentModel *contentModel =
                    qobject_cast<QHelpContentModel*>(m_contentWidget->model());
                if (contentModel) {
                    QHelpContentItem *itm = contentModel->contentItemAt(index);
                    if (itm && !isPdfFile(itm))
                        Help::Internal::CentralWidget::instance()->setSourceInNewTab(itm->url());
                }
            } else if (button == Qt::LeftButton) {
                itemClicked(index);
            }
        }
    } else if (o == m_contentWidget && e->type() == QEvent::KeyPress) {
        if (static_cast<QKeyEvent *>(e)->key() == Qt::Key_Escape)
            emit escapePressed();
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
    QAction *newTab = menu.addAction(tr("Open Link in New Tab"));
    if (isPdfFile(itm))
        newTab->setEnabled(false);

    menu.move(m_contentWidget->mapToGlobal(pos));

    QAction *action = menu.exec();
    if (curTab == action)
        emit linkActivated(itm->url());
    else if (newTab == action)
        Help::Internal::CentralWidget::instance()->setSourceInNewTab(itm->url());
}

void ContentWindow::itemClicked(const QModelIndex &index)
{
    QHelpContentModel *contentModel =
        qobject_cast<QHelpContentModel*>(m_contentWidget->model());

    if (contentModel) {
        QHelpContentItem *itm = contentModel->contentItemAt(index);
        if (itm)
            emit linkActivated(itm->url());
    }
}

bool ContentWindow::isPdfFile(QHelpContentItem *item) const
{
    const QString &path = item->url().path();
    return path.endsWith(QLatin1String(".pdf"), Qt::CaseInsensitive);
}
