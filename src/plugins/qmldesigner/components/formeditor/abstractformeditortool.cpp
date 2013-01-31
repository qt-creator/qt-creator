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

#include "abstractformeditortool.h"
#include "formeditorview.h"
#include "formeditorview.h"

#include <modelnodecontextmenu.h>

#include <coreplugin/editormanager/editormanager.h>

#include <QDebug>
#include <QGraphicsSceneDragDropEvent>
#include <nodemetainfo.h>
#include <nodeproperty.h>

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
            && formEditorItem->qmlItemNode().instanceIsMovable()
            && !formEditorItem->qmlItemNode().instanceIsInPositioner()
            && (formEditorItem->qmlItemNode().hasShowContent()))
            return true;
    }

    foreach (QGraphicsItem *item, itemList) {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
        if (formEditorItem
            && formEditorItem->qmlItemNode().isValid()
            && formEditorItem->qmlItemNode().instanceIsMovable()
            && !formEditorItem->qmlItemNode().instanceIsInPositioner()
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

static inline bool checkIfNodeIsAView(const ModelNode &node)
{
    return node.metaInfo().isValid() &&
            (node.metaInfo().isSubclassOf("QtQuick.ListView", -1, -1) ||
             node.metaInfo().isSubclassOf("QtQuick.GridView", -1, -1) ||
             node.metaInfo().isSubclassOf("QtQuick.PathView", -1, -1));
}

void AbstractFormEditorTool::mousePressEvent(const QList<QGraphicsItem*> & /*itemList*/, QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::RightButton)
        event->accept();
}

void AbstractFormEditorTool::mouseReleaseEvent(const QList<QGraphicsItem*> & /*itemList*/, QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        showContextMenu(event);
        event->accept();
    }
}

void AbstractFormEditorTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> &, QGraphicsSceneMouseEvent *)
{
}

void AbstractFormEditorTool::showContextMenu(QGraphicsSceneMouseEvent *event)
{
    ModelNodeContextMenu::showContextMenu(view(), event->screenPos(), event->scenePos().toPoint(), true);
}

void AbstractFormEditorTool::clear()
{
    m_itemList.clear();
}
}
