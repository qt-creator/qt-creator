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

#include "openeditorsview.h"
#include "editormanager.h"
#include "openeditorsmodel.h"

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
    EditorManager *em = EditorManager::instance();
    setModel(em->openedEditorsModel());
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    header()->setStretchLastSection(false);
    header()->setResizeMode(0, QHeaderView::Stretch);
    header()->setResizeMode(1, QHeaderView::Fixed);
    header()->resizeSection(1, 16);
    setContextMenuPolicy(Qt::CustomContextMenu);
    installEventFilter(this);
    viewport()->installEventFilter(this);

    connect(em, SIGNAL(currentEditorChanged(Core::IEditor*)),
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
    if (!editor) {
        clearSelection();
        return;
    }
    EditorManager *em = EditorManager::instance();
    setCurrentIndex(em->openedEditorsModel()->indexOf(editor));
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
    EditorManager::instance()->activateEditorForIndex(index, EditorManager::ModeSwitch);
}

void OpenEditorsWidget::closeEditor(const QModelIndex &index)
{
    EditorManager::instance()->closeEditor(index);
    // work around selection changes
    updateCurrentItem(EditorManager::currentEditor());
}

void OpenEditorsWidget::contextMenuRequested(QPoint pos)
{
    QMenu contextMenu;
    QModelIndex editorIndex = indexAt(pos);
    EditorManager::instance()->addSaveAndCloseEditorActions(&contextMenu, editorIndex);
    contextMenu.addSeparator();
    EditorManager::instance()->addNativeDirActions(&contextMenu, editorIndex);
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

Core::Id OpenEditorsViewFactory::id() const
{
    return Core::Id("Open Documents");
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
