/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "abstractformeditortool.h"
#include "formeditorview.h"
#include "formeditorview.h"

#include <QtDebug>
#include <QGraphicsSceneDragDropEvent>
#include <nodemetainfo.h>

namespace QmlDesigner {

AbstractFormEditorTool::AbstractFormEditorTool(FormEditorView *editorView) : m_view(editorView)
{
}


AbstractFormEditorTool::~AbstractFormEditorTool()
{

}

FormEditorView* AbstractFormEditorTool::view() const
{
    return m_view;
}

FormEditorScene* AbstractFormEditorTool::scene() const
{
    return view()->scene();
}

void AbstractFormEditorTool::setItems(const QList<FormEditorItem*> &itemList)
{
    m_itemList = itemList;
    selectedItemsChanged(m_itemList);
}

QList<FormEditorItem*> AbstractFormEditorTool::items() const
{
    return m_itemList;
}

bool AbstractFormEditorTool::topItemIsMovable(const QList<QGraphicsItem*> & itemList)
{
    QGraphicsItem *firstSelectableItem = topMovableGraphicsItem(itemList);
    if (firstSelectableItem == 0)
        return false;

    FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(firstSelectableItem);
    QList<QmlItemNode> selectedNodes = view()->selectedQmlItemNodes();

    if (formEditorItem != 0
       && selectedNodes.contains(formEditorItem->qmlItemNode()))
        return true;

    return false;

}

bool AbstractFormEditorTool::topSelectedItemIsMovable(const QList<QGraphicsItem*> &itemList)
{
    QList<QmlItemNode> selectedNodes = view()->selectedQmlItemNodes();

    foreach (QGraphicsItem *item, itemList) {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
        if (formEditorItem
            && selectedNodes.contains(formEditorItem->qmlItemNode())
            && !formEditorItem->qmlItemNode().isRootNode()
            && (formEditorItem->qmlItemNode().hasShowContent()))
            return true;
    }

    foreach (QGraphicsItem *item, itemList) {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
        if (formEditorItem
            && !formEditorItem->qmlItemNode().isRootNode()
            && selectedNodes.contains(formEditorItem->qmlItemNode()))
            return true;
    }

    return false;

}


bool AbstractFormEditorTool::topItemIsResizeHandle(const QList<QGraphicsItem*> &/*itemList*/)
{
    return false;
}

QGraphicsItem *AbstractFormEditorTool::topMovableGraphicsItem(const QList<QGraphicsItem*> &itemList)
{
    foreach (QGraphicsItem *item, itemList) {
        if (item->flags().testFlag(QGraphicsItem::ItemIsMovable))
            return item;
    }

    return 0;
}
FormEditorItem *AbstractFormEditorTool::topMovableFormEditorItem(const QList<QGraphicsItem*> &itemList)
{
    foreach (QGraphicsItem *item, itemList) {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
        if (formEditorItem
           && (formEditorItem->qmlItemNode().hasShowContent()))
            return formEditorItem;
    }

    return 0;
}

FormEditorItem* AbstractFormEditorTool::topFormEditorItem(const QList<QGraphicsItem*> & itemList)
{
    foreach (QGraphicsItem *item, itemList) {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
        if (formEditorItem && !formEditorItem->qmlItemNode().isRootNode())
            return formEditorItem;
    }

    return 0;
}

FormEditorItem* AbstractFormEditorTool::topFormEditorItemWithRootItem(const QList<QGraphicsItem*> & itemList)
{
    foreach (QGraphicsItem *item, itemList) {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
        if (formEditorItem)
            return formEditorItem;
    }

    return 0;
}

void AbstractFormEditorTool::dropEvent(QGraphicsSceneDragDropEvent * /* event */)
{
    Q_ASSERT(false);
}

void AbstractFormEditorTool::dragEnterEvent(QGraphicsSceneDragDropEvent * event)
{
    if (event->mimeData()->hasFormat("application/vnd.bauhaus.itemlibraryinfo") ||
        event->mimeData()->hasFormat("application/vnd.bauhaus.libraryresource")) {
        event->accept();
        view()->changeToDragTool();
        view()->currentTool()->dragEnterEvent(event);
    } else {
        event->ignore();
    }
}

void AbstractFormEditorTool::dragLeaveEvent(QGraphicsSceneDragDropEvent * /* event */)
{
    Q_ASSERT(false);
}

void AbstractFormEditorTool::dragMoveEvent(QGraphicsSceneDragDropEvent * /* event */)
{
    Q_ASSERT(false);
}


}
