/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "openpageswidget.h"

#include "centralwidget.h"
#include "openpagesmodel.h"

#include <coreplugin/coreconstants.h>

#include <QAbstractItemModel>
#include <QApplication>
#include <QMenu>

using namespace Help::Internal;

// -- OpenPagesWidget

OpenPagesWidget::OpenPagesWidget(OpenPagesModel *sourceModel, QWidget *parent)
    : OpenDocumentsTreeView(parent)
    , m_allowContextMenu(true)
{
    setModel(sourceModel);

    setContextMenuPolicy(Qt::CustomContextMenu);
    updateCloseButtonVisibility();

    connect(this, &OpenDocumentsTreeView::activated,
            this, &OpenPagesWidget::handleActivated);
    connect(this, &OpenDocumentsTreeView::closeActivated,
            this, &OpenPagesWidget::handleCloseActivated);
    connect(this, &OpenDocumentsTreeView::customContextMenuRequested,
            this, &OpenPagesWidget::contextMenuRequested);
    connect(model(), SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(updateCloseButtonVisibility()));
    connect(model(), SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(updateCloseButtonVisibility()));
}

OpenPagesWidget::~OpenPagesWidget()
{
}

void OpenPagesWidget::selectCurrentPage()
{
    QItemSelectionModel * const selModel = selectionModel();
    selModel->clearSelection();
    selModel->select(model()->index(CentralWidget::instance()->currentIndex(), 0),
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    scrollTo(currentIndex());
}

void OpenPagesWidget::allowContextMenu(bool ok)
{
    m_allowContextMenu = ok;
}

// -- private slots

void OpenPagesWidget::contextMenuRequested(QPoint pos)
{
    QModelIndex index = indexAt(pos);
    if (!index.isValid() || !m_allowContextMenu)
        return;

    if (index.column() == 1)
        index = index.sibling(index.row(), 0);
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

void OpenPagesWidget::handleActivated(const QModelIndex &index)
{
    if (index.column() == 0) {
        emit setCurrentPage(index);
    } else if (index.column() == 1) { // the funky close button
        if (model()->rowCount() > 1)
            emit closePage(index);

        // work around a bug in itemviews where the delegate wouldn't get the QStyle::State_MouseOver
        QWidget *vp = viewport();
        const QPoint &cursorPos = QCursor::pos();
        QMouseEvent e(QEvent::MouseMove, vp->mapFromGlobal(cursorPos), cursorPos, Qt::NoButton, 0, 0);
        QCoreApplication::sendEvent(vp, &e);
    }
}

void OpenPagesWidget::handleCloseActivated(const QModelIndex &index)
{
    if (model()->rowCount() > 1)
        emit closePage(index);
}

void OpenPagesWidget::updateCloseButtonVisibility()
{
    setCloseButtonVisible(model() && model()->rowCount() > 1);
}
