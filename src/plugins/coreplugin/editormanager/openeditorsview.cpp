/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "openeditorsview.h"
#include "editormanager.h"
#include "ieditor.h"
#include "documentmodel.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <QApplication>
#include <QMenu>
#include <QPainter>
#include <QStyle>
#include <QHeaderView>
#include <QKeyEvent>

using namespace Core;
using namespace Core::Internal;


OpenEditorsDelegate::OpenEditorsDelegate(QObject *parent)
 : QStyledItemDelegate(parent)
{
}

void OpenEditorsDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
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
        const QIcon icon(QLatin1String((option.state & QStyle::State_Selected) ?
                                       Constants::ICON_CLOSE : Constants::ICON_CLOSE_DARK));

        QRect iconRect(option.rect.right() - option.rect.height(),
                       option.rect.top(),
                       option.rect.height(),
                       option.rect.height());

        icon.paint(painter, iconRect, Qt::AlignRight | Qt::AlignVCenter);
    }

}

////
// OpenEditorsWidget
////

OpenEditorsWidget::OpenEditorsWidget()
{
    setWindowTitle(tr("Open Documents"));
    setWindowIcon(QIcon(QLatin1String(Constants::ICON_DIR)));
    setUniformRowHeights(true);
    viewport()->setAttribute(Qt::WA_Hover);
    setItemDelegate((m_delegate = new OpenEditorsDelegate(this)));
    header()->hide();
    setIndentation(0);
    setTextElideMode(Qt::ElideMiddle);
    setFrameStyle(QFrame::NoFrame);
    setAttribute(Qt::WA_MacShowFocusRect, false);
    m_model = new ProxyModel(this);
    m_model->setSourceModel(EditorManager::documentModel());
    setModel(m_model);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    header()->setStretchLastSection(false);
    header()->setResizeMode(0, QHeaderView::Stretch);
    header()->setResizeMode(1, QHeaderView::Fixed);
    header()->resizeSection(1, 16);
    setContextMenuPolicy(Qt::CustomContextMenu);
    installEventFilter(this);
    viewport()->installEventFilter(this);

    connect(EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(updateCurrentItem(Core::IEditor*)));
    connect(this, SIGNAL(clicked(QModelIndex)),
            this, SLOT(handleClicked(QModelIndex)));
    connect(this, SIGNAL(pressed(QModelIndex)),
            this, SLOT(handlePressed(QModelIndex)));

    connect(this, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(contextMenuRequested(QPoint)));
}

OpenEditorsWidget::~OpenEditorsWidget()
{
}

void OpenEditorsWidget::updateCurrentItem(Core::IEditor *editor)
{
    IDocument *document = editor ? editor->document() : 0;
    QModelIndex index = m_model->index(EditorManager::documentModel()->indexOfDocument(document), 0);
    if (!index.isValid()) {
        clearSelection();
        return;
    }
    setCurrentIndex(index);
    selectionModel()->select(currentIndex(),
                                              QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    scrollTo(currentIndex());
}

bool OpenEditorsWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == this && event->type() == QEvent::KeyPress
            && currentIndex().isValid()) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        if ((ke->key() == Qt::Key_Return
                || ke->key() == Qt::Key_Enter)
                && ke->modifiers() == 0) {
            activateEditor(currentIndex());
            return true;
        } else if ((ke->key() == Qt::Key_Delete
                   || ke->key() == Qt::Key_Backspace)
                && ke->modifiers() == 0) {
            closeEditor(currentIndex());
        }
    } else if (obj == viewport()
             && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent * me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::MiddleButton
                && me->modifiers() == Qt::NoModifier) {
            QModelIndex index = indexAt(me->pos());
            if (index.isValid()) {
                closeEditor(index);
                return true;
            }
        }
    }
    return false;
}

void OpenEditorsWidget::handlePressed(const QModelIndex &index)
{
    if (index.column() == 0)
        activateEditor(index);
    else if (index.column() == 1)
        m_delegate->pressedIndex = index;
}

void OpenEditorsWidget::handleClicked(const QModelIndex &index)
{
    if (index.column() == 1) { // the funky close button
        closeEditor(index);

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
                EditorManager::documentModel()->documentAtRow(m_model->mapToSource(index).row()));
}

void OpenEditorsWidget::closeEditor(const QModelIndex &index)
{
    EditorManager::closeEditor(
                EditorManager::documentModel()->documentAtRow(m_model->mapToSource(index).row()));
    // work around selection changes
    updateCurrentItem(EditorManager::currentEditor());
}

void OpenEditorsWidget::contextMenuRequested(QPoint pos)
{
    QMenu contextMenu;
    QModelIndex editorIndex = indexAt(pos);
    DocumentModel::Entry *entry = EditorManager::documentModel()->documentAtRow(
                m_model->mapToSource(editorIndex).row());
    EditorManager::addSaveAndCloseEditorActions(&contextMenu, entry);
    contextMenu.addSeparator();
    EditorManager::addNativeDirActions(&contextMenu, entry);
    contextMenu.exec(mapToGlobal(pos));
}

///
// OpenEditorsViewFactory
///

NavigationView OpenEditorsViewFactory::createWidget()
{
    NavigationView n;
    n.widget = new OpenEditorsWidget();
    return n;
}

QString OpenEditorsViewFactory::displayName() const
{
    return OpenEditorsWidget::tr("Open Documents");
}

int OpenEditorsViewFactory::priority() const
{
    return 200;
}

Id OpenEditorsViewFactory::id() const
{
    return "Open Documents";
}

QKeySequence OpenEditorsViewFactory::activationSequence() const
{
    return QKeySequence(Core::UseMacShortcuts ? tr("Meta+O") : tr("Alt+O"));
}

OpenEditorsViewFactory::OpenEditorsViewFactory()
{
}

OpenEditorsViewFactory::~OpenEditorsViewFactory()
{
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
        disconnect(previousModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                   this, SLOT(sourceDataChanged(QModelIndex,QModelIndex)));
        disconnect(previousModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                   this, SLOT(sourceRowsInserted(QModelIndex,int,int)));
        disconnect(previousModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                   this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));
        disconnect(previousModel, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
                   this, SLOT(sourceRowsAboutToBeInserted(QModelIndex,int,int)));
        disconnect(previousModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                   this, SLOT(sourceRowsAboutToBeRemoved(QModelIndex,int,int)));
    }
    QAbstractProxyModel::setSourceModel(sm);
    if (sm) {
        connect(sm, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                this, SLOT(sourceDataChanged(QModelIndex,QModelIndex)));
        connect(sm, SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(sourceRowsInserted(QModelIndex,int,int)));
        connect(sm, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));
        connect(sm, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
                this, SLOT(sourceRowsAboutToBeInserted(QModelIndex,int,int)));
        connect(sm, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                this, SLOT(sourceRowsAboutToBeRemoved(QModelIndex,int,int)));
    }
}

#if QT_VERSION >= 0x050000
QModelIndex ProxyModel::sibling(int row, int column, const QModelIndex &idx) const
{
    return QAbstractItemModel::sibling(row, column, idx);
}
#endif

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
