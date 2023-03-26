// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "openpageswidget.h"

#include "helptr.h"

#include <coreplugin/coreconstants.h>
#include <utils/stringutils.h>

#include <QAbstractItemModel>
#include <QApplication>
#include <QMenu>

using namespace Help::Internal;

// -- OpenPagesWidget

OpenPagesWidget::OpenPagesWidget(QAbstractItemModel *sourceModel, QWidget *parent)
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
    connect(model(), &QAbstractItemModel::rowsInserted,
            this, &OpenPagesWidget::updateCloseButtonVisibility);
    connect(model(), &QAbstractItemModel::rowsRemoved,
            this, &OpenPagesWidget::updateCloseButtonVisibility);
}

OpenPagesWidget::~OpenPagesWidget() = default;

void OpenPagesWidget::selectCurrentPage(int index)
{
    QItemSelectionModel * const selModel = selectionModel();
    selModel->clearSelection();
    selModel->select(model()->index(index, 0),
                     QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    scrollTo(currentIndex());
}

void OpenPagesWidget::allowContextMenu(bool ok)
{
    m_allowContextMenu = ok;
}

// -- private

void OpenPagesWidget::contextMenuRequested(QPoint pos)
{
    QModelIndex index = indexAt(pos);
    if (!index.isValid() || !m_allowContextMenu)
        return;

    if (index.column() == 1)
        index = index.sibling(index.row(), 0);
    QMenu contextMenu;
    const QString displayString = Utils::quoteAmpersands(index.data().toString());
    QAction *closeEditor = contextMenu.addAction(Tr::tr("Close %1").arg(displayString));
    QAction *closeOtherEditors = contextMenu.addAction(
        Tr::tr("Close All Except %1").arg(displayString));

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
        QMouseEvent e(QEvent::MouseMove, vp->mapFromGlobal(cursorPos), cursorPos, Qt::NoButton, {}, {});
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
