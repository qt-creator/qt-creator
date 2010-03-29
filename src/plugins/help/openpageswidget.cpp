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

#include "openpageswidget.h"
#include "openpagesmodel.h"

#include <QtGui/QApplication>
#include <QtGui/QPainter>

#include <QtGui/QHeaderView>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QMenu>

#ifdef Q_WS_MAC
#include <qmacstyle_mac.h>
#endif

using namespace Help::Internal;

// -- OpenPagesDelegate

OpenPagesDelegate::OpenPagesDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void OpenPagesDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
           const QModelIndex &index) const
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

        const QRect iconRect(option.rect.right() - option.rect.height(),
            option.rect.top(), option.rect.height(), option.rect.height());

        icon.paint(painter, iconRect, Qt::AlignRight | Qt::AlignVCenter);
    }

}

// -- OpenPagesWidget

OpenPagesWidget::OpenPagesWidget(OpenPagesModel *model)
{
    setModel(model);
    setIndentation(0);
    setItemDelegate((m_delegate = new OpenPagesDelegate(this)));

    setFrameStyle(QFrame::NoFrame);
    setTextElideMode(Qt::ElideMiddle);
    setAttribute(Qt::WA_MacShowFocusRect, false);

    viewport()->setAttribute(Qt::WA_Hover);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);

    header()->hide();
    header()->setStretchLastSection(false);
    header()->setResizeMode(0, QHeaderView::Stretch);
    header()->setResizeMode(1, QHeaderView::Fixed);
    header()->resizeSection(1, 16);

    installEventFilter(this);
    setUniformRowHeights(true);
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, SIGNAL(clicked(QModelIndex)), this,
        SLOT(handleClicked(QModelIndex)));
    connect(this, SIGNAL(pressed(QModelIndex)), this,
        SLOT(handlePressed(QModelIndex)));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this,
        SLOT(contextMenuRequested(QPoint)));
}

OpenPagesWidget::~OpenPagesWidget()
{
}

// -- private slots

void OpenPagesWidget::contextMenuRequested(QPoint pos)
{
    const QModelIndex &index = indexAt(pos);
    if (!index.isValid())
        return;

    QMenu contextMenu;
    QAction *closeEditor = contextMenu.addAction(tr("Close %1").arg(index.data()
        .toString()));
    QAction *closeOtherEditors = contextMenu.addAction(tr("Close All Except %1")
        .arg(index.data().toString()));

    if (model()->rowCount() == 1) {
        closeEditor->setEnabled(false);
        closeOtherEditors->setEnabled(false);
    }

    QAction *action = contextMenu.exec(mapToGlobal(pos));
    if (action == closeEditor)
        emit closePage(index);
    else if (action == closeOtherEditors)
        emit closePagesExcept(index);
}

void OpenPagesWidget::handlePressed(const QModelIndex &index)
{
    if (index.column() == 0)
        emit setCurrentPage(index);

    if (index.column() == 1)
        m_delegate->pressedIndex = index;
}

void OpenPagesWidget::handleClicked(const QModelIndex &index)
{
    // implemented here to handle the funky close button and to  work around a
    // bug in item views where the delegate wouldn't get the QStyle::State_MouseOver
    if (index.column() == 1) {
        if (model()->rowCount() > 1)
            emit closePage(index);

        QWidget *vp = viewport();
        const QPoint &cursorPos = QCursor::pos();
        QMouseEvent e(QEvent::MouseMove, vp->mapFromGlobal(cursorPos), cursorPos,
            Qt::NoButton, 0, 0);
        QCoreApplication::sendEvent(vp, &e);
    }
}

// -- private

bool OpenPagesWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == this && event->type() == QEvent::KeyPress) {
        if (currentIndex().isValid()) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(event);
            const int key = ke->key();
            if ((key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space)
                && ke->modifiers() == 0) {
                emit setCurrentPage(currentIndex());
            } else if ((key == Qt::Key_Delete || key == Qt::Key_Backspace)
                && ke->modifiers() == 0 && model()->rowCount() > 1) {
                emit closePage(currentIndex());
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
