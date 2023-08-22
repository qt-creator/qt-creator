// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemviews.h"

namespace Utils {

/*!
   \class Utils::TreeView
   \inmodule QtCreator

    \brief The TreeView adds setActivationMode to QTreeView
    to allow for single click/double click behavior on
    platforms where the default is different. Use with care.

    Also adds sane keyboard navigation for mac.

    Note: This uses setUniformRowHeights(true) by default.
 */

/*!
   \class Utils::TreeWidget
   \inmodule QtCreator

    \brief The TreeWidget adds setActivationMode to QTreeWidget
    to allow for single click/double click behavior on
    platforms where the default is different. Use with care.

    Also adds sane keyboard navigation for mac.

    Note: This uses setUniformRowHeights(true) by default.
 */

/*!
   \class Utils::ListView
   \inmodule QtCreator

    \brief The ListView adds setActivationMode to QListView
    to allow for single click/double click behavior on
    platforms where the default is different. Use with care.

    Also adds sane keyboard navigation for mac.
 */

/*!
   \class Utils::ListWidget
   \inmodule QtCreator

    \brief The ListWidget adds setActivationMode to QListWidget
    to allow for single click/double click behavior on
    platforms where the default is different. Use with care.

    Also adds sane keyboard navigation for mac.
 */

TreeView::TreeView(QWidget *parent)
    : View<QTreeView>(parent)
{
    setUniformRowHeights(true);
}

TreeWidget::TreeWidget(QWidget *parent)
    : View<QTreeWidget>(parent)
{
    setUniformRowHeights(true);
}

ListView::ListView(QWidget *parent)
    : View<QListView>(parent)
{}

ListWidget::ListWidget(QWidget *parent)
    : View<QListWidget>(parent)
{}

} // Utils
