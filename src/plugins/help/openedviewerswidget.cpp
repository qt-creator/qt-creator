/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "openedviewerswidget.h"

#include "centralwidget.h"
#include "helpviewer.h"

#include <QtGui/QApplication>
#include <QtGui/QKeyEvent>
#include <QtGui/QMenu>
#include <QtGui/QPainter>

#include <coreplugin/coreconstants.h>

using namespace Help::Internal;

OpenedViewersDelegate::OpenedViewersDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void OpenedViewersDelegate::paint(QPainter *painter,
    const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (option.state & QStyle::State_MouseOver) {
        if ((QApplication::mouseButtons() & Qt::LeftButton) == 0)
            pressedIndex = QModelIndex();

        QBrush brush = option.palette.alternateBase();
        if (index == pressedIndex)
            brush = option.palette.dark();

        painter->fillRect(option.rect, brush);
    }

    QStyledItemDelegate::paint(painter, option, index);

    if (index.column() == 1 && option.state & QStyle::State_MouseOver) {
        QIcon icon((option.state & QStyle::State_Selected)
            ? ":/core/images/closebutton.png" : ":/core/images/darkclosebutton.png");

        QRect iconRect(option.rect.right() - option.rect.height(),
            option.rect.top(), option.rect.height(), option.rect.height());

        icon.paint(painter, iconRect, Qt::AlignRight | Qt::AlignVCenter);
    }
}


// OpenedViewersWidget


OpenedViewersWidget::OpenedViewersWidget(QWidget *parent)
    : QWidget(parent)
    , m_delegate(new OpenedViewersDelegate(this))
{
    m_ui.setupUi(this);

    setFocusProxy(m_ui.viewerList);
    setWindowTitle(tr("Open Documents"));
    setWindowIcon(QIcon(Core::Constants::ICON_DIR));

    m_ui.viewerList->installEventFilter(this);
    m_ui.viewerList->setItemDelegate(m_delegate);
    m_ui.viewerList->viewport()->setAttribute(Qt::WA_Hover);
    m_ui.viewerList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_ui.viewerList->setAttribute(Qt::WA_MacShowFocusRect, false);

    m_model = new QStandardItemModel(0, 2, this);
    m_ui.viewerList->setModel(m_model);

    m_ui.viewerList->header()->setResizeMode(1, QHeaderView::Fixed);
    m_ui.viewerList->header()->setResizeMode(0, QHeaderView::Stretch);
    m_ui.viewerList->header()->resizeSection(1, 16);

    CentralWidget *widget = CentralWidget::instance();
    connect(widget, SIGNAL(sourceChanged(QUrl)), this,
        SLOT(resetWidgetModel()));
    connect(widget, SIGNAL(currentViewerChanged(int)), this,
        SLOT(updateViewerWidgetModel(int)));
    connect(widget, SIGNAL(viewerAboutToBeRemoved(int)), this,
        SLOT(removeViewerFromWidgetModel(int)));

    connect(m_ui.viewerList, SIGNAL(clicked(QModelIndex)), this,
        SLOT(handleClicked(QModelIndex)));
    connect(m_ui.viewerList, SIGNAL(pressed(QModelIndex)), this,
        SLOT(handlePressed(QModelIndex)));
    connect(m_ui.viewerList, SIGNAL(customContextMenuRequested(QPoint)), this,
        SLOT(contextMenuRequested(QPoint)));
}

OpenedViewersWidget::~OpenedViewersWidget()
{
}

bool OpenedViewersWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_ui.viewerList && event->type() == QEvent::KeyPress
            && m_ui.viewerList->currentIndex().isValid()) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        const int key = ke->key();
        if ((key == Qt::Key_Return || key == Qt::Key_Enter)
                && ke->modifiers() == 0) {
            handlePressed(m_ui.viewerList->currentIndex());
            return true;
        } else if ((key == Qt::Key_Delete || key == Qt::Key_Backspace)
                && ke->modifiers() == 0) {
            handleClicked(m_ui.viewerList->currentIndex());
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void OpenedViewersWidget::resetWidgetModel()
{
    m_model->clear();

    int i = 0;
    CentralWidget *widget = CentralWidget::instance();
    QStandardItem *parentItem = m_model->invisibleRootItem();
    while (HelpViewer *viewer = widget->helpViewerAtIndex(i++)) {
        QList<QStandardItem*> list;
        list.append(new QStandardItem(viewer->documentTitle()));
        list.append(new QStandardItem());
        parentItem->appendRow(list);
    }

    m_ui.viewerList->header()->setStretchLastSection(false);
    m_ui.viewerList->header()->setResizeMode(0, QHeaderView::Stretch);
    m_ui.viewerList->header()->setResizeMode(1, QHeaderView::Fixed);
    m_ui.viewerList->header()->resizeSection(1, 16);
    m_ui.viewerList->setEditTriggers(QAbstractItemView::NoEditTriggers);

    int row = widget->indexOf(widget->currentHelpViewer());
    if (row >= 0)
        m_ui.viewerList->setCurrentIndex(m_model->index(row, 0));
}

void OpenedViewersWidget::updateViewerWidgetModel(int index)
{
     if (index >= 0)
        m_ui.viewerList->setCurrentIndex(m_model->index(index, 0));
}

void OpenedViewersWidget::removeViewerFromWidgetModel(int index)
{
    if (index >= 0)
        m_model->removeRow(index);
}

void OpenedViewersWidget::handleClicked(const QModelIndex &index)
{
    if (index.isValid() && index.column() == 1) {
            // the funky close button
        CentralWidget::instance()->closeTab(index.row());

        // work around a bug in itemviews where the delegate wouldn't get the
        // QStyle::State_MouseOver
        QPoint cursorPos = QCursor::pos();
        QWidget *vp = m_ui.viewerList->viewport();
        QMouseEvent e(QEvent::MouseMove, vp->mapFromGlobal(cursorPos),
            cursorPos, Qt::NoButton, 0, 0);
        QCoreApplication::sendEvent(vp, &e);
    }
}

void OpenedViewersWidget::handlePressed(const QModelIndex &index)
{
    if (index.isValid()) {
        switch (index.column()) {
            case 0:
                CentralWidget::instance()->activateTab(index.row());
                break;

            case 1:
                m_delegate->pressedIndex = index;
                break;
        }
    }
}

void OpenedViewersWidget::contextMenuRequested(const QPoint &pos)
{
    const QModelIndex &index = m_ui.viewerList->indexAt(pos);
    if (!index.isValid())
        return;

    CentralWidget *widget = CentralWidget::instance();
    HelpViewer *viewer = widget->helpViewerAtIndex(index.row());
    if (widget->count() <= 1 || !viewer)
        return;

    QMenu contextMenu;
    const QString &title = viewer->documentTitle();
    QAction *close = contextMenu.addAction(tr("Close %1").arg(title));
    QAction *closeOther = contextMenu.addAction(tr("Close All Except %1").arg(title));

    if (QAction *action = contextMenu.exec(m_ui.viewerList->mapToGlobal(pos))) {
        if (action == closeOther) {
            int currentPage = widget->indexOf(viewer);
            for (int i = widget->count() - 1; i >= 0; --i) {
                viewer = widget->helpViewerAtIndex(i);
                if (i != currentPage && viewer) {
                    widget->closeTab(i);
                    if (i < currentPage)
                        --currentPage;
                }
            }
        }

        if (action == close)
            widget->closeTab(index.row());
    }
}
