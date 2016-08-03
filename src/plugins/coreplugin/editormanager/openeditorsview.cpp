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

#include "openeditorsview.h"
#include "editormanager.h"
#include "ieditor.h"
#include "documentmodel.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>

#include <utils/utilsicons.h>

#include <QApplication>
#include <QMenu>

using namespace Core;
using namespace Core::Internal;

////
// OpenEditorsWidget
////

OpenEditorsWidget::OpenEditorsWidget()
{
    setWindowTitle(tr("Open Documents"));
    setWindowIcon(Utils::Icons::DIR.icon());
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);

    m_model = new ProxyModel(this);
    m_model->setSourceModel(DocumentModel::model());
    setModel(m_model);

    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, &OpenEditorsWidget::updateCurrentItem);
    connect(this, &OpenDocumentsTreeView::activated,
            this, &OpenEditorsWidget::handleActivated);
    connect(this, &OpenDocumentsTreeView::closeActivated,
            this, &OpenEditorsWidget::closeDocument);

    connect(this, &OpenDocumentsTreeView::customContextMenuRequested,
            this, &OpenEditorsWidget::contextMenuRequested);
}

OpenEditorsWidget::~OpenEditorsWidget()
{
}

void OpenEditorsWidget::updateCurrentItem(IEditor *editor)
{
    IDocument *document = editor ? editor->document() : 0;
    QModelIndex index = m_model->index(DocumentModel::indexOfDocument(document), 0);
    if (!index.isValid()) {
        clearSelection();
        return;
    }
    setCurrentIndex(index);
    selectionModel()->select(currentIndex(),
                                              QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    scrollTo(currentIndex());
}

void OpenEditorsWidget::handleActivated(const QModelIndex &index)
{
    if (index.column() == 0) {
        activateEditor(index);
    } else if (index.column() == 1) { // the funky close button
        closeDocument(index);

        // work around a bug in itemviews where the delegate wouldn't get the QStyle::State_MouseOver
        QPoint cursorPos = QCursor::pos();
        QWidget *vp = viewport();
        QMouseEvent e(QEvent::MouseMove, vp->mapFromGlobal(cursorPos), cursorPos, Qt::NoButton, 0, 0);
        QCoreApplication::sendEvent(vp, &e);
    }
}

void OpenEditorsWidget::activateEditor(const QModelIndex &index)
{
    selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    EditorManager::activateEditorForEntry(
                DocumentModel::entryAtRow(m_model->mapToSource(index).row()));
}

void OpenEditorsWidget::closeDocument(const QModelIndex &index)
{
    EditorManager::closeDocument(
                DocumentModel::entryAtRow(m_model->mapToSource(index).row()));
    // work around selection changes
    updateCurrentItem(EditorManager::currentEditor());
}

void OpenEditorsWidget::contextMenuRequested(QPoint pos)
{
    QMenu contextMenu;
    QModelIndex editorIndex = indexAt(pos);
    DocumentModel::Entry *entry = DocumentModel::entryAtRow(
                m_model->mapToSource(editorIndex).row());
    EditorManager::addSaveAndCloseEditorActions(&contextMenu, entry);
    contextMenu.addSeparator();
    EditorManager::addNativeDirAndOpenWithActions(&contextMenu, entry);
    contextMenu.exec(mapToGlobal(pos));
}

///
// OpenEditorsViewFactory
///

OpenEditorsViewFactory::OpenEditorsViewFactory()
{
    setId("Open Documents");
    setDisplayName(OpenEditorsWidget::tr("Open Documents"));
    setActivationSequence(QKeySequence(UseMacShortcuts ? tr("Meta+O") : tr("Alt+O")));
    setPriority(200);
}

NavigationView OpenEditorsViewFactory::createWidget()
{
    return NavigationView(new OpenEditorsWidget());
}

ProxyModel::ProxyModel(QObject *parent) : QAbstractProxyModel(parent)
{
}

QModelIndex ProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    // root
    if (!sourceIndex.isValid())
        return QModelIndex();
    // hide the <no document>
    int row = sourceIndex.row() - 1;
    if (row < 0)
        return QModelIndex();
    return createIndex(row, sourceIndex.column());
}

QModelIndex ProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid())
        return QModelIndex();
    // handle missing <no document>
    return sourceModel()->index(proxyIndex.row() + 1, proxyIndex.column());
}

QModelIndex ProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || row < 0 || row >= sourceModel()->rowCount(mapToSource(parent)) - 1
            || column < 0 || column > 1)
        return QModelIndex();
    return createIndex(row, column);
}

QModelIndex ProxyModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)
    return QModelIndex();
}

int ProxyModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return sourceModel()->rowCount(mapToSource(parent)) - 1;
    return 0;
}

int ProxyModel::columnCount(const QModelIndex &parent) const
{
    return sourceModel()->columnCount(mapToSource(parent));
}

void ProxyModel::setSourceModel(QAbstractItemModel *sm)
{
    QAbstractItemModel *previousModel = sourceModel();
    if (previousModel) {
        disconnect(previousModel, &QAbstractItemModel::dataChanged,
                   this, &ProxyModel::sourceDataChanged);
        disconnect(previousModel, &QAbstractItemModel::rowsInserted,
                   this, &ProxyModel::sourceRowsInserted);
        disconnect(previousModel, &QAbstractItemModel::rowsRemoved,
                   this, &ProxyModel::sourceRowsRemoved);
        disconnect(previousModel, &QAbstractItemModel::rowsAboutToBeInserted,
                   this, &ProxyModel::sourceRowsAboutToBeInserted);
        disconnect(previousModel, &QAbstractItemModel::rowsAboutToBeRemoved,
                   this, &ProxyModel::sourceRowsAboutToBeRemoved);
    }
    QAbstractProxyModel::setSourceModel(sm);
    if (sm) {
        connect(sm, &QAbstractItemModel::dataChanged,
                this, &ProxyModel::sourceDataChanged);
        connect(sm, &QAbstractItemModel::rowsInserted,
                this, &ProxyModel::sourceRowsInserted);
        connect(sm, &QAbstractItemModel::rowsRemoved,
                this, &ProxyModel::sourceRowsRemoved);
        connect(sm, &QAbstractItemModel::rowsAboutToBeInserted,
                this, &ProxyModel::sourceRowsAboutToBeInserted);
        connect(sm, &QAbstractItemModel::rowsAboutToBeRemoved,
                this, &ProxyModel::sourceRowsAboutToBeRemoved);
    }
}

QModelIndex ProxyModel::sibling(int row, int column, const QModelIndex &idx) const
{
    return QAbstractItemModel::sibling(row, column, idx);
}

Qt::DropActions ProxyModel::supportedDragActions() const
{
    return sourceModel()->supportedDragActions();
}

void ProxyModel::sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    QModelIndex topLeftIndex = mapFromSource(topLeft);
    if (!topLeftIndex.isValid())
        topLeftIndex = index(0, topLeft.column());
    QModelIndex bottomRightIndex = mapFromSource(bottomRight);
    if (!bottomRightIndex.isValid())
        bottomRightIndex = index(0, bottomRight.column());
    emit dataChanged(topLeftIndex, bottomRightIndex);
}

void ProxyModel::sourceRowsRemoved(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    endRemoveRows();
}

void ProxyModel::sourceRowsInserted(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    endInsertRows();
}

void ProxyModel::sourceRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    int realStart = parent.isValid() || start == 0 ? start : start - 1;
    int realEnd = parent.isValid() || end == 0 ? end : end - 1;
    beginRemoveRows(parent, realStart, realEnd);
}

void ProxyModel::sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
    int realStart = parent.isValid() || start == 0 ? start : start - 1;
    int realEnd = parent.isValid() || end == 0 ? end : end - 1;
    beginInsertRows(parent, realStart, realEnd);
}
