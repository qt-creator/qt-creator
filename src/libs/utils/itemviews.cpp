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

static Internal::ViewSearchCallback &viewSearchCallback()
{
    static Internal::ViewSearchCallback theViewSearchCallback;
    return theViewSearchCallback;
}

static void makeViewSearchable(QAbstractItemView *view, int role)
{
    if (viewSearchCallback())
        viewSearchCallback()(view, role);
}

/*!
    \internal

    \note Only use once from Core initialization.
*/
void Internal::setViewSearchCallback(const ViewSearchCallback &cb)
{
    viewSearchCallback() = cb;
}

TreeView::TreeView(QWidget *parent)
    : View<QTreeView>(parent)
{
    setUniformRowHeights(true);
}

void TreeView::setSearchRole(int role)
{
    makeViewSearchable(this, role);
}

ListView::ListView(QWidget *parent)
    : View<QListView>(parent)
{}

ListWidget::ListWidget(QWidget *parent)
    : View<QListWidget>(parent)
{}

} // Utils
