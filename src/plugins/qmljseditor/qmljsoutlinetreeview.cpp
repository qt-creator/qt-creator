// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljseditortr.h"
#include "qmljsoutlinetreeview.h"
#include "qmloutlinemodel.h"

#include <utils/delegates.h>
#include <QMenu>

namespace QmlJSEditor {
namespace Internal {

QmlJSOutlineTreeView::QmlJSOutlineTreeView(QWidget *parent) :
    Utils::NavigationTreeView(parent)
{
    setExpandsOnDoubleClick(false);

    setDragEnabled(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(InternalMove);

    setRootIsDecorated(false);

    auto itemDelegate = new Utils::AnnotatedItemDelegate(this);
    itemDelegate->setDelimiter(QLatin1String(" "));
    itemDelegate->setAnnotationRole(QmlOutlineModel::AnnotationRole);
    setItemDelegateForColumn(0, itemDelegate);
}

void QmlJSOutlineTreeView::contextMenuEvent(QContextMenuEvent *event)
{
    if (!event)
        return;

    QMenu contextMenu;

    connect(contextMenu.addAction(Tr::tr("Expand All")), &QAction::triggered,
            this, &QmlJSOutlineTreeView::expandAll);
    connect(contextMenu.addAction(Tr::tr("Collapse All")), &QAction::triggered,
            this, &QmlJSOutlineTreeView::collapseAllExceptRoot);

    contextMenu.exec(event->globalPos());

    event->accept();
}

void QmlJSOutlineTreeView::collapseAllExceptRoot()
{
    if (!model())
        return;
    const QModelIndex rootElementIndex = model()->index(0, 0, rootIndex());
    int rowCount = model()->rowCount(rootElementIndex);
    for (int i = 0; i < rowCount; ++i) {
        collapse(model()->index(i, 0, rootElementIndex));
    }
}

} // namespace Internal
} // namespace QmlJSEditor
