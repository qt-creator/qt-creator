// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "view3dtool.h"

#include "designmodewidget.h"
#include "formeditorview.h"
#include "qmldesignerplugin.h"

#include <QGraphicsSceneMouseEvent>

namespace QmlDesigner {

View3DTool::View3DTool()
    : QObject(), AbstractCustomTool()
{
}

View3DTool::~View3DTool()
{
}

void View3DTool::clear()
{
    m_view3dNode = {};
    AbstractFormEditorTool::clear();
}

void View3DTool::mouseMoveEvent(const QList<QGraphicsItem *> &, QGraphicsSceneMouseEvent *)
{
}

void View3DTool::hoverMoveEvent(const QList<QGraphicsItem *> &, QGraphicsSceneMouseEvent *)
{
}

void View3DTool::keyPressEvent(QKeyEvent *)
{
}

void View3DTool::keyReleaseEvent(QKeyEvent *)
{
}

void  View3DTool::dragLeaveEvent(const QList<QGraphicsItem *> &, QGraphicsSceneDragDropEvent *)
{
}

void  View3DTool::dragMoveEvent(const QList<QGraphicsItem *> &, QGraphicsSceneDragDropEvent *)
{
}

void View3DTool::mouseReleaseEvent(const QList<QGraphicsItem *> &,
                                   QGraphicsSceneMouseEvent *event)
{
    if (m_view3dNode.isValid()) {
        QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("Editor3D", true);
        view()->emitCustomNotification("pick_3d_node_from_2d_scene",
                                       {m_view3dNode}, {event->scenePos()});
    }

    view()->changeToSelectionTool();
}

void View3DTool::itemsAboutToRemoved(const QList<FormEditorItem *> &)
{
}

void View3DTool::instancesCompleted(const QList<FormEditorItem *> &)
{
}

void  View3DTool::instancesParentChanged(const QList<FormEditorItem *> &)
{
}

void View3DTool::instancePropertyChange(const QList<QPair<ModelNode, PropertyName>> &)
{
}

void View3DTool::formEditorItemsChanged(const QList<FormEditorItem *> &)
{
}

int View3DTool::wantHandleItem(const ModelNode &modelNode) const
{
    if (modelNode.metaInfo().isQtQuick3DView3D())
        return 30;

    return 0;
}


QString View3DTool::name() const
{
    return tr("View3D Tool");
}

void View3DTool::selectedItemsChanged(const QList<FormEditorItem *> &itemList)
{
    if (itemList.size() == 1) {
        if (itemList[0]) {
            ModelNode node = itemList[0]->qmlItemNode().modelNode();
            if (node.metaInfo().isQtQuick3DView3D()) {
                m_view3dNode = node;
                return;
            }
        }
    }

    view()->changeToSelectionTool();
}

} // namespace QmlDesigner
