/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "qtmessagelogview.h"
#include "qtmessagelogitemdelegate.h"
#include "qtmessageloghandler.h"
#include "debuggerstringutils.h"
#include "debuggercore.h"
#include "debuggerengine.h"

#include <texteditor/basetexteditor.h>

#include <QMouseEvent>
#include <QProxyStyle>
#include <QPainter>
#include <QApplication>
#include <QClipboard>
#include <QAbstractProxyModel>
#include <QFileInfo>
#include <QUrl>
#include <QScrollBar>

namespace Debugger {
namespace Internal {

class QtMessageLogViewViewStyle : public QProxyStyle
{
public:
    void drawPrimitive(PrimitiveElement element,
                       const QStyleOption *option,
                       QPainter *painter,
                       const QWidget *widget = 0) const
    {
        if (element != QStyle::PE_PanelItemViewRow)
            QProxyStyle::drawPrimitive(element, option, painter, widget);
    }

    int styleHint(StyleHint hint,
                  const QStyleOption *option = 0,
                  const QWidget *widget = 0,
                  QStyleHintReturn *returnData = 0) const {
        if (hint == SH_ItemView_ShowDecorationSelected)
            return 0;
        else
            return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

///////////////////////////////////////////////////////////////////////
//
// QtMessageLogView
//
///////////////////////////////////////////////////////////////////////

QtMessageLogView::QtMessageLogView(QWidget *parent) :
    QTreeView(parent)
{
    setFrameStyle(QFrame::NoFrame);
    setHeaderHidden(true);
    setRootIsDecorated(false);
    setEditTriggers(QAbstractItemView::AllEditTriggers);
    setStyleSheet(QLatin1String("QTreeView::branch:has-siblings:!adjoins-item {"
                                "border-image: none;"
                                "image: none; }"
                                "QTreeView::branch:has-siblings:adjoins-item {"
                                "border-image: none;"
                                "image: none; }"
                                "QTreeView::branch:!has-children:!has-siblings:adjoins-item {"
                                "border-image: none;"
                                "image: none; }"
                                "QTreeView::branch:has-children:!has-siblings:closed,"
                                "QTreeView::branch:closed:has-children:has-siblings {"
                                "border-image: none;"
                                "image: none; }"
                                "QTreeView::branch:open:has-children:!has-siblings,"
                                "QTreeView::branch:open:has-children:has-siblings  {"
                                "border-image: none;"
                                "image: none; }"));
    QtMessageLogViewViewStyle *style = new QtMessageLogViewViewStyle;
    setStyle(style);
    style->setParent(this);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    connect(this, SIGNAL(activated(QModelIndex)),
            SLOT(onRowActivated(QModelIndex)));
}

void QtMessageLogView::onScrollToBottom()
{
    //Keep scrolling to bottom if scroll bar is at maximum()
    if (verticalScrollBar()->value() == verticalScrollBar()->maximum())
        scrollToBottom();
}

void QtMessageLogView::mousePressEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    QModelIndex index = indexAt(pos);
    if (index.isValid()) {
        QtMessageLogHandler::ItemType type = (QtMessageLogHandler::ItemType)index.data(
                    QtMessageLogHandler::TypeRole).toInt();
        bool handled = false;
        if (type == QtMessageLogHandler::UndefinedType) {
            bool showTypeIcon = index.parent() == QModelIndex();
            ConsoleItemPositions positions(visualRect(index), viewOptions().font,
                                           showTypeIcon, true);

            if (positions.expandCollapseIcon().contains(pos)) {
                if (isExpanded(index))
                    setExpanded(index, false);
                else
                    setExpanded(index, true);
                handled = true;
            }
        }
        if (!handled)
            QTreeView::mousePressEvent(event);
    } else {
        selectionModel()->setCurrentIndex(model()->index(
                                              model()->rowCount() - 1, 0),
                                 QItemSelectionModel::ClearAndSelect);
    }
}

void QtMessageLogView::keyPressEvent(QKeyEvent *e)
{
    if (!e->modifiers() && e->key() == Qt::Key_Return) {
        emit activated(currentIndex());
        e->accept();
        return;
    }
    QTreeView::keyPressEvent(e);
}

void QtMessageLogView::resizeEvent(QResizeEvent *e)
{
    static_cast<QtMessageLogItemDelegate *>(itemDelegate())->emitSizeHintChanged(
                selectionModel()->currentIndex());
    QTreeView::resizeEvent(e);
}

void QtMessageLogView::drawBranches(QPainter *painter, const QRect &rect,
                             const QModelIndex &index) const
{
    static_cast<QtMessageLogItemDelegate *>(itemDelegate())->drawBackground(
                painter, rect, index, false);
    QTreeView::drawBranches(painter, rect, index);
}

void QtMessageLogView::contextMenuEvent(QContextMenuEvent *event)
{
    QModelIndex itemIndex = indexAt(event->pos());
    QMenu menu;

    QAction *copy = new QAction(tr("&Copy"), this);
    copy->setEnabled(itemIndex.isValid());
    menu.addAction(copy);
    QAction *show = new QAction(tr("&Show in Editor"), this);
    show->setEnabled(canShowItemInTextEditor(itemIndex));
    menu.addAction(show);
    menu.addSeparator();
    QAction *clear = new QAction(tr("C&lear"), this);
    menu.addAction(clear);

    QAction *a = menu.exec(event->globalPos());
    if (a == 0)
        return;

    if (a == copy)
        copyToClipboard(itemIndex);
    else if (a == show)
        onRowActivated(itemIndex);
    else if (a == clear) {
        QAbstractProxyModel *proxyModel =
                qobject_cast<QAbstractProxyModel *>(model());
        QtMessageLogHandler *handler =
                qobject_cast<QtMessageLogHandler *>(proxyModel->sourceModel());
        handler->clear();
    }
}

void QtMessageLogView::onRowActivated(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    //See if we have file and line Info
    QString filePath = model()->data(index,
                                     QtMessageLogHandler::FileRole).toString();
    if (!filePath.isEmpty()) {
        filePath = debuggerCore()->currentEngine()->toFileInProject(
                    QUrl(filePath));
        QFileInfo fi(filePath);
        if (fi.exists() && fi.isFile() && fi.isReadable()) {
            int line = model()->data(index,
                                     QtMessageLogHandler::LineRole).toInt();
            TextEditor::BaseTextEditorWidget::openEditorAt(
                        fi.canonicalFilePath(), line);
        }
    }
}

void QtMessageLogView::copyToClipboard(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    QString contents = model()->data(index).toString();
    //See if we have file and line Info
    QString filePath = model()->data(index,
                                     QtMessageLogHandler::FileRole).toString();
    if (!filePath.isEmpty()) {
        contents = QString(_("%1 %2: %3")).arg(contents).arg(filePath).arg(
                    model()->data(index,
                                  QtMessageLogHandler::LineRole).toString());
    }
    QClipboard *cb = QApplication::clipboard();
    cb->setText(contents);
}

bool QtMessageLogView::canShowItemInTextEditor(const QModelIndex &index)
{
    if (!index.isValid())
        return false;

    //See if we have file and line Info
    QString filePath = model()->data(index,
                                     QtMessageLogHandler::FileRole).toString();
    if (!filePath.isEmpty()) {
        filePath = debuggerCore()->currentEngine()->toFileInProject(
                    QUrl(filePath));
        QFileInfo fi(filePath);
        if (fi.exists() && fi.isFile() && fi.isReadable()) {
            return true;
        }
    }
    return false;
}

} //Internal
} //Debugger
