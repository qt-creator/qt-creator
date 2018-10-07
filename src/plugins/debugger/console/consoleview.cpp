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

#include "consoleview.h"
#include "consoleitemdelegate.h"
#include "consoleitemmodel.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/manhattanstyle.h>
#include <qtsupport/baseqtversion.h>
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

namespace Debugger {
namespace Internal {

class ConsoleViewStyle : public ManhattanStyle
{
public:
    ConsoleViewStyle(const QString &baseStyleName) : ManhattanStyle(baseStyleName) {}

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                       const QWidget *widget = nullptr) const final
    {
        if (element != QStyle::PE_PanelItemViewRow)
            ManhattanStyle::drawPrimitive(element, option, painter, widget);
    }

    int styleHint(StyleHint hint, const QStyleOption *option = nullptr,
                  const QWidget *widget = nullptr,
                  QStyleHintReturn *returnData = nullptr) const final
    {
        if (hint == SH_ItemView_ShowDecorationSelected)
            return 0;
        else
            return ManhattanStyle::styleHint(hint, option, widget, returnData);
    }
};

///////////////////////////////////////////////////////////////////////
//
// ConsoleView
//
///////////////////////////////////////////////////////////////////////

ConsoleView::ConsoleView(ConsoleItemModel *model, QWidget *parent) :
    Utils::TreeView(parent), m_model(model)
{
    setFrameStyle(QFrame::NoFrame);
    setHeaderHidden(true);
    setRootIsDecorated(false);
    setEditTriggers(QAbstractItemView::AllEditTriggers);
    setStyleSheet("QTreeView::branch:has-siblings:!adjoins-item {"
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
                  "image: none; }");

    QString baseName = QApplication::style()->objectName();
    if (Utils::HostOsInfo::isAnyUnixHost() && !Utils::HostOsInfo::isMacHost()
            && baseName == "windows") {
        // Sometimes we get the standard windows 95 style as a fallback
        if (QStyleFactory::keys().contains("Fusion")) {
            baseName = "fusion"; // Qt5
        }
    }
    auto style = new ConsoleViewStyle(baseName);
    setStyle(style);
    style->setParent(this);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    horizontalScrollBar()->setSingleStep(20);
    verticalScrollBar()->setSingleStep(20);

    connect(this, &ConsoleView::activated, this, &ConsoleView::onRowActivated);
}

void ConsoleView::onScrollToBottom()
{
    // Keep scrolling to bottom if scroll bar is not at maximum()
    if (verticalScrollBar()->value() != verticalScrollBar()->maximum())
        scrollToBottom();
}

void ConsoleView::populateFileFinder()
{
    QtSupport::BaseQtVersion::populateQmlFileFinder(&m_finder, nullptr);
}

void ConsoleView::mousePressEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    QModelIndex index = indexAt(pos);
    if (index.isValid()) {
        ConsoleItem::ItemType type = (ConsoleItem::ItemType)index.data(
                    ConsoleItem::TypeRole).toInt();
        bool handled = false;
        if (type == ConsoleItem::DefaultType) {
            bool showTypeIcon = index.parent() == QModelIndex();
            ConsoleItemPositions positions(m_model, visualRect(index), viewOptions().font, showTypeIcon,
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
    }
}

void ConsoleView::resizeEvent(QResizeEvent *e)
{
    static_cast<ConsoleItemDelegate *>(itemDelegate())->emitSizeHintChanged(
                selectionModel()->currentIndex());
    Utils::TreeView::resizeEvent(e);
}

void ConsoleView::drawBranches(QPainter *painter, const QRect &rect,
                                  const QModelIndex &index) const
{
    static_cast<ConsoleItemDelegate *>(itemDelegate())->drawBackground(painter, rect, index,
                                                                            false);
    Utils::TreeView::drawBranches(painter, rect, index);
}

void ConsoleView::contextMenuEvent(QContextMenuEvent *event)
{
    QModelIndex itemIndex = indexAt(event->pos());
    QMenu menu;

    auto copy = new QAction(tr("&Copy"), this);
    copy->setEnabled(itemIndex.isValid());
    menu.addAction(copy);
    auto show = new QAction(tr("&Show in Editor"), this);
    show->setEnabled(canShowItemInTextEditor(itemIndex));
    menu.addAction(show);
    menu.addSeparator();
    auto clear = new QAction(tr("C&lear"), this);
    menu.addAction(clear);

    QAction *a = menu.exec(event->globalPos());
    if (a == nullptr)
        return;

    if (a == copy) {
        copyToClipboard(itemIndex);
    } else if (a == show) {
        onRowActivated(itemIndex);
    } else if (a == clear) {
        auto proxyModel = qobject_cast<QAbstractProxyModel*>(model());
        auto handler = qobject_cast<ConsoleItemModel*>(
                    proxyModel->sourceModel());
        handler->clear();
    }
}

void ConsoleView::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    selectionModel()->setCurrentIndex(model()->index(model()->rowCount() - 1, 0),
                                      QItemSelectionModel::ClearAndSelect);
}

void ConsoleView::onRowActivated(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    const QFileInfo fi(m_finder.findFile(model()->data(index, ConsoleItem::FileRole).toString()));
    if (fi.exists() && fi.isFile() && fi.isReadable()) {
        Core::EditorManager::openEditorAt(fi.canonicalFilePath(),
                                          model()->data(index, ConsoleItem::LineRole).toInt());
    }
}

void ConsoleView::copyToClipboard(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    QString contents = model()->data(index, ConsoleItem::ExpressionRole).toString();
    // See if we have file and line Info
    QString filePath = model()->data(index, ConsoleItem::FileRole).toString();
    const QUrl fileUrl = QUrl(filePath);
    if (fileUrl.isLocalFile())
        filePath = fileUrl.toLocalFile();
    if (!filePath.isEmpty()) {
        contents = QString::fromLatin1("%1 %2: %3").arg(contents).arg(filePath).arg(
                    model()->data(index, ConsoleItem::LineRole).toString());
    }
    QClipboard *cb = QApplication::clipboard();
    cb->setText(contents);
}

bool ConsoleView::canShowItemInTextEditor(const QModelIndex &index)
{
    if (!index.isValid())
        return false;

    bool success = false;
    m_finder.findFile(model()->data(index, ConsoleItem::FileRole).toString(), &success);
    return success;
}

} // Internal
} // Debugger
