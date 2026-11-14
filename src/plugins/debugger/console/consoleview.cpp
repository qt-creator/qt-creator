// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "consoleview.h"

#include "consoleitemdelegate.h"
#include "consoleitemmodel.h"
#include "../debuggertr.h"

#include <coreplugin/editormanager/editormanager.h>
#include <qtsupport/baseqtversion.h>

#include <utils/hostosinfo.h>
#include <utils/stringutils.h>

#include <QAction>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QApplication>
#include <QAbstractProxyModel>
#include <QScrollBar>
#include <QStyleFactory>
#include <QString>
#include <QUrl>

namespace Debugger::Internal {

ConsoleView::ConsoleView(ConsoleItemModel *model, QWidget *parent) :
    Utils::TreeView(parent), m_model(model)
{
    setUniformRowHeights(false);
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
    QtSupport::QtVersion::populateQmlFileFinder(&m_finder, nullptr);
}

void ConsoleView::mousePressEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    QModelIndex index = indexAt(pos);
    if (index.isValid()) {
        ConsoleItem::ItemType type = (ConsoleItem::ItemType)index.data(
                    ConsoleItem::TypeRole).toInt();
        const bool showTypeIcon = index.parent() == QModelIndex();
        const bool showExpandableIcon = type == ConsoleItem::DefaultType;
        QStyleOptionViewItem option;
        initViewItemOption(&option);
        ConsoleItemPositions positions(m_model, visualRect(index), option.font,
                                       showTypeIcon, showExpandableIcon);

        if ((positions.fileArea().contains(pos) || positions.lineArea().contains(pos))
            && canShowItemInTextEditor(index)) {
            onRowActivated(index);
            return;
        }

        bool handled = false;
        if (showExpandableIcon && positions.expandCollapseIcon().contains(pos)) {
            if (isExpanded(index))
                setExpanded(index, false);
            else
                setExpanded(index, true);
            handled = true;
        }
        if (!handled) {
            Utils::TreeView::mousePressEvent(event);
            if (index.flags().testFlag(Qt::ItemIsEditable))
                edit(index);
        }
    } else {
        Utils::TreeView::mousePressEvent(event);
        selectEditableInput();
        edit(currentIndex());
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
    static_cast<ConsoleItemDelegate *>(itemDelegate())->drawBackground(painter, rect, index, {});
    Utils::TreeView::drawBranches(painter, rect, index);
}

void ConsoleView::contextMenuEvent(QContextMenuEvent *event)
{
    QModelIndex itemIndex = indexAt(event->pos());
    QMenu menu;

    auto copy = new QAction(Tr::tr("&Copy"), this);
    copy->setEnabled(itemIndex.isValid());
    menu.addAction(copy);
    auto show = new QAction(Tr::tr("&Show in Editor"), this);
    show->setEnabled(canShowItemInTextEditor(itemIndex));
    menu.addAction(show);
    menu.addSeparator();
    auto clear = new QAction(Tr::tr("C&lear"), this);
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
    Utils::TreeView::focusInEvent(event);
}

void ConsoleView::focusInput()
{
    selectEditableInput();
    edit(currentIndex());
}

void ConsoleView::selectEditableInput()
{
    const QModelIndex input = model()->index(model()->rowCount() - 1, 0);
    selectionModel()->setCurrentIndex(input, QItemSelectionModel::ClearAndSelect);
}

void ConsoleView::onRowActivated(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    const Utils::FilePaths files
        = m_finder.findFile(model()->data(index, ConsoleItem::FileRole).toString());
    if (!files.isEmpty() && files.constFirst().isReadableFile())
        Core::EditorManager::openEditorAt(
            {files.constFirst(), model()->data(index, ConsoleItem::LineRole).toInt()});
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
    Utils::setClipboardAndSelection(contents);
}

bool ConsoleView::canShowItemInTextEditor(const QModelIndex &index)
{
    if (!index.isValid())
        return false;

    const Utils::FilePaths files
        = m_finder.findFile(model()->data(index, ConsoleItem::FileRole).toString());
    return !files.isEmpty() && files.constFirst().isReadableFile();
}

} // Debugger::Internal
