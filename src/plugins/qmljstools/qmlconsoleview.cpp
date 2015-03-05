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

#include "qmlconsoleview.h"
#include "qmlconsoleitemdelegate.h"
#include "qmlconsoleitemmodel.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/manhattanstyle.h>
#include <utils/hostosinfo.h>

#include <QAction>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QApplication>
#include <QClipboard>
#include <QAbstractProxyModel>
#include <QFileInfo>
#include <QScrollBar>
#include <QStyleFactory>
#include <QString>
#include <QUrl>

using namespace QmlJS;

namespace QmlJSTools {
namespace Internal {

class QmlConsoleViewStyle : public ManhattanStyle
{
public:
    QmlConsoleViewStyle(const QString &baseStyleName) : ManhattanStyle(baseStyleName) {}

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                       const QWidget *widget = 0) const
    {
        if (element != QStyle::PE_PanelItemViewRow)
            ManhattanStyle::drawPrimitive(element, option, painter, widget);
    }

    int styleHint(StyleHint hint, const QStyleOption *option = 0, const QWidget *widget = 0,
                  QStyleHintReturn *returnData = 0) const {
        if (hint == SH_ItemView_ShowDecorationSelected)
            return 0;
        else
            return ManhattanStyle::styleHint(hint, option, widget, returnData);
    }
};

///////////////////////////////////////////////////////////////////////
//
// QmlConsoleView
//
///////////////////////////////////////////////////////////////////////

QmlConsoleView::QmlConsoleView(QWidget *parent) :
    Utils::TreeView(parent)
{
    setFrameStyle(QFrame::NoFrame);
    setHeaderHidden(true);
    setRootIsDecorated(false);
    setUniformRowHeights(true);
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

    QString baseName = QApplication::style()->objectName();
    if (Utils::HostOsInfo::isAnyUnixHost() && !Utils::HostOsInfo::isMacHost()
            && baseName == QLatin1String("windows")) {
        // Sometimes we get the standard windows 95 style as a fallback
        if (QStyleFactory::keys().contains(QLatin1String("Fusion"))) {
            baseName = QLatin1String("fusion"); // Qt5
        } else { // Qt4
            // e.g. if we are running on a KDE4 desktop
            QByteArray desktopEnvironment = qgetenv("DESKTOP_SESSION");
            if (desktopEnvironment == "kde")
                baseName = QLatin1String("plastique");
            else
                baseName = QLatin1String("cleanlooks");
        }
    }
    QmlConsoleViewStyle *style = new QmlConsoleViewStyle(baseName);
    setStyle(style);
    style->setParent(this);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    horizontalScrollBar()->setSingleStep(20);
    verticalScrollBar()->setSingleStep(20);

    connect(this, &QmlConsoleView::activated, this, &QmlConsoleView::onRowActivated);
}

void QmlConsoleView::onScrollToBottom()
{
    // Keep scrolling to bottom if scroll bar is not at maximum()
    if (verticalScrollBar()->value() != verticalScrollBar()->maximum())
        scrollToBottom();
}

void QmlConsoleView::mousePressEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    QModelIndex index = indexAt(pos);
    if (index.isValid()) {
        ConsoleItem::ItemType type = (ConsoleItem::ItemType)index.data(
                    QmlConsoleItemModel::TypeRole).toInt();
        bool handled = false;
        if (type == ConsoleItem::UndefinedType) {
            bool showTypeIcon = index.parent() == QModelIndex();
            ConsoleItemPositions positions(visualRect(index), viewOptions().font, showTypeIcon,
                                           true);

            if (positions.expandCollapseIcon().contains(pos)) {
                if (isExpanded(index))
                    setExpanded(index, false);
                else
                    setExpanded(index, true);
                handled = true;
            }
        }
        if (!handled)
            Utils::TreeView::mousePressEvent(event);
    } else {
        selectionModel()->setCurrentIndex(model()->index(model()->rowCount() - 1, 0),
                                          QItemSelectionModel::ClearAndSelect);
    }
}

void QmlConsoleView::resizeEvent(QResizeEvent *e)
{
    static_cast<QmlConsoleItemDelegate *>(itemDelegate())->emitSizeHintChanged(
                selectionModel()->currentIndex());
    Utils::TreeView::resizeEvent(e);
}

void QmlConsoleView::drawBranches(QPainter *painter, const QRect &rect,
                                  const QModelIndex &index) const
{
    static_cast<QmlConsoleItemDelegate *>(itemDelegate())->drawBackground(painter, rect, index,
                                                                            false);
    Utils::TreeView::drawBranches(painter, rect, index);
}

void QmlConsoleView::contextMenuEvent(QContextMenuEvent *event)
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

    if (a == copy) {
        copyToClipboard(itemIndex);
    } else if (a == show) {
        onRowActivated(itemIndex);
    } else if (a == clear) {
        QAbstractProxyModel *proxyModel = qobject_cast<QAbstractProxyModel *>(model());
        QmlConsoleItemModel *handler = qobject_cast<QmlConsoleItemModel *>(
                    proxyModel->sourceModel());
        handler->clear();
    }
}

void QmlConsoleView::onRowActivated(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    // See if we have file and line Info
    QString filePath = model()->data(index, QmlConsoleItemModel::FileRole).toString();
    const QUrl fileUrl = QUrl(filePath);
    if (fileUrl.isLocalFile())
        filePath = fileUrl.toLocalFile();
    if (!filePath.isEmpty()) {
        QFileInfo fi(filePath);
        if (fi.exists() && fi.isFile() && fi.isReadable()) {
            int line = model()->data(index, QmlConsoleItemModel::LineRole).toInt();
            Core::EditorManager::openEditorAt(fi.canonicalFilePath(), line);
        }
    }
}

void QmlConsoleView::copyToClipboard(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    QString contents = model()->data(index, QmlConsoleItemModel::ExpressionRole).toString();
    // See if we have file and line Info
    QString filePath = model()->data(index, QmlConsoleItemModel::FileRole).toString();
    const QUrl fileUrl = QUrl(filePath);
    if (fileUrl.isLocalFile())
        filePath = fileUrl.toLocalFile();
    if (!filePath.isEmpty()) {
        contents = QString::fromLatin1("%1 %2: %3").arg(contents).arg(filePath).arg(
                    model()->data(index, QmlConsoleItemModel::LineRole).toString());
    }
    QClipboard *cb = QApplication::clipboard();
    cb->setText(contents);
}

bool QmlConsoleView::canShowItemInTextEditor(const QModelIndex &index)
{
    if (!index.isValid())
        return false;

    // See if we have file and line Info
    QString filePath = model()->data(index, QmlConsoleItemModel::FileRole).toString();
    const QUrl fileUrl = QUrl(filePath);
    if (fileUrl.isLocalFile())
        filePath = fileUrl.toLocalFile();
    if (!filePath.isEmpty()) {
        QFileInfo fi(filePath);
        if (fi.exists() && fi.isFile() && fi.isReadable())
            return true;
    }
    return false;
}

} // Internal
} // QmlJSTools
