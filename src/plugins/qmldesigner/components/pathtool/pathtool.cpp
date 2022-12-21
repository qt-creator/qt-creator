// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pathtool.h"

#include <formeditorscene.h>
#include <formeditorview.h>
#include <formeditorwidget.h>
#include <itemutilfunctions.h>
#include <formeditoritem.h>

#include "pathitem.h"

#include <nodemetainfo.h>
#include <qmlitemnode.h>
#include <nodeproperty.h>
#include <nodelistproperty.h>
#include <qmldesignerplugin.h>

#include <abstractaction.h>
#include <designeractionmanager.h>

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QAction>
#include <QDebug>
#include <QPair>

namespace QmlDesigner {

static bool isNonSupportedPathElement(const ModelNode &pathElement)
{
    if (pathElement.type() == "QtQuick.PathCubic")
        return false;

    if (pathElement.type() == "QtQuick.PathAttribute")
        return false;

    if (pathElement.type() == "QtQuick.PathPercent")
        return false;

    if (pathElement.type() == "QtQuick.PathAttribute")
        return false;

    if (pathElement.type() == "QtQuick.PathQuad")
        return false;

    if (pathElement.type() == "QtQuick.PathLine")
        return false;

    return true;
}


static int pathRankForModelNode(const ModelNode &modelNode) {
    if (modelNode.metaInfo().hasProperty("path")) {
        if (modelNode.hasNodeProperty("path")) {
            ModelNode pathNode = modelNode.nodeProperty("path").modelNode();
            if (pathNode.metaInfo().isQtQuickPath() && pathNode.hasNodeListProperty("pathElements")) {
                const QList<ModelNode> pathElements = pathNode.nodeListProperty("pathElements")
                                                          .toModelNodeList();
                if (pathElements.isEmpty())
                    return 0;

                for (const ModelNode &pathElement : pathElements) {
                    if (isNonSupportedPathElement(pathElement))
                        return 0;
                }
            }
            return 20;
        }
    }

    return 0;
}

class PathToolAction : public AbstractAction
{
public:
    PathToolAction() : AbstractAction(QCoreApplication::translate("PathToolAction","Edit Path")) {}

    QByteArray category() const override
    {
        return QByteArray();
    }

    QByteArray menuId() const override
    {
        return "PathTool";
    }

    int priority() const override
    {
        return CustomActionsPriority;
    }

    Type type() const override
    {
        return ContextMenuAction;
    }

protected:
    bool isVisible(const SelectionContext &selectionContext) const override
    {
        if (selectionContext.scenePosition().isNull())
            return false;

        if (selectionContext.singleNodeIsSelected())
            return pathRankForModelNode(selectionContext.currentSingleSelectedNode()) > 0;

        return false;
    }

    bool isEnabled(const SelectionContext &selectionContext) const override
    {
        return isVisible(selectionContext);
    }
};

PathTool::PathTool(ExternalDependenciesInterface &externalDepoendencies)
    : m_pathToolView(this, externalDepoendencies)
{
    auto textToolAction = new PathToolAction;
    QmlDesignerPlugin::instance()->designerActionManager().addDesignerAction(textToolAction);
    connect(textToolAction->action(), &QAction::triggered, [=]() {
        if (m_pathToolView.model())
            m_pathToolView.model()->detachView(&m_pathToolView);
        view()->changeCurrentToolTo(this);
    });


}

PathTool::~PathTool() = default;

void PathTool::clear()
{
    if (m_pathItem)
        m_pathItem->deleteLater();

    AbstractFormEditorTool::clear();
}

void PathTool::mousePressEvent(const QList<QGraphicsItem*> & /*itemList*/,
                                            QGraphicsSceneMouseEvent * event)
{
    event->setPos(m_pathItem->mapFromScene(event->scenePos()));
    event->setLastPos(m_pathItem->mapFromScene(event->lastScenePos()));
    scene()->sendEvent(m_pathItem.data(), event);
}

void PathTool::mouseMoveEvent(const QList<QGraphicsItem*> & /*itemList*/,
                              QGraphicsSceneMouseEvent *event)
{
    event->setPos(m_pathItem->mapFromScene(event->scenePos()));
    event->setLastPos(m_pathItem->mapFromScene(event->lastScenePos()));
    scene()->sendEvent(m_pathItem.data(), event);
}

void PathTool::hoverMoveEvent(const QList<QGraphicsItem*> & /*itemList*/,
                        QGraphicsSceneMouseEvent * event)
{
    event->setPos(m_pathItem->mapFromScene(event->scenePos()));
    event->setLastPos(m_pathItem->mapFromScene(event->lastScenePos()));
    scene()->sendEvent(m_pathItem.data(), event);
}

void PathTool::keyPressEvent(QKeyEvent *keyEvent)
{
    if (keyEvent->key() == Qt::Key_Escape) {
        m_pathItem->writePathToProperty();
        keyEvent->accept();
    }
}

void PathTool::keyReleaseEvent(QKeyEvent * keyEvent)
{
    if (keyEvent->key() == Qt::Key_Escape) {
        keyEvent->accept();
        if (m_pathToolView.model())
            m_pathToolView.model()->detachView(&m_pathToolView);
        view()->changeToSelectionTool();
    }
}

void  PathTool::dragLeaveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{

}

void  PathTool::dragMoveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{

}

void PathTool::mouseReleaseEvent(const QList<QGraphicsItem*> & /*itemList*/,
                                 QGraphicsSceneMouseEvent *event)
{
    event->setPos(m_pathItem->mapFromScene(event->scenePos()));
    event->setLastPos(m_pathItem->mapFromScene(event->lastScenePos()));
    scene()->sendEvent(m_pathItem.data(), event);
}


void PathTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> & /*itemList*/, QGraphicsSceneMouseEvent *event)
{
    if (m_pathItem.data() && !m_pathItem->boundingRect().contains(m_pathItem->mapFromScene(event->scenePos()))) {
        m_pathItem->writePathToProperty();
        view()->changeToSelectionTool();
        event->accept();
    }
}

void PathTool::itemsAboutToRemoved(const QList<FormEditorItem*> &removedItemList)
{
    if (m_pathItem == nullptr)
        return;

    if (removedItemList.contains(m_pathItem->formEditorItem()))
        view()->changeToSelectionTool();
}

static bool hasPathProperty(FormEditorItem *formEditorItem)
{
    return formEditorItem->qmlItemNode().modelNode().metaInfo().hasProperty("path");
}

void PathTool::selectedItemsChanged(const QList<FormEditorItem*> &itemList)
{
    if (m_pathItem.data() && itemList.contains(m_pathItem->formEditorItem()))
        m_pathItem->writePathToProperty();

    delete m_pathItem.data();
    if (!itemList.isEmpty() && hasPathProperty(itemList.constFirst())) {
        FormEditorItem *formEditorItem = itemList.constFirst();
        m_pathItem = new PathItem(scene());
        m_pathItem->setParentItem(scene()->manipulatorLayerItem());
        m_pathItem->setFormEditorItem(formEditorItem);
        formEditorItem->qmlItemNode().modelNode().model()->attachView(&m_pathToolView);
    } else {
        if (m_pathToolView.model())
            m_pathToolView.model()->detachView(&m_pathToolView);
        view()->changeToSelectionTool();
    }
}

void PathTool::instancesCompleted(const QList<FormEditorItem*> & /*itemList*/)
{
}

void  PathTool::instancesParentChanged(const QList<FormEditorItem *> & /*itemList*/)
{
}

void PathTool::instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList)
{
    using ModelNodePropertyNamePair = QPair<ModelNode, PropertyName>;
    for (const ModelNodePropertyNamePair &propertyPair : propertyList) {
        if (propertyPair.first == m_pathItem->formEditorItem()->qmlItemNode().modelNode()
                && propertyPair.second == "path")
            m_pathItem->updatePath();
    }
}

void PathTool::formEditorItemsChanged(const QList<FormEditorItem*> & /*itemList*/)
{
}

int PathTool::wantHandleItem(const ModelNode &modelNode) const
{
    return pathRankForModelNode(modelNode);
}

QString PathTool::name() const
{
    return QCoreApplication::translate("PathTool", "Path Tool");
}

ModelNode PathTool::editingPathViewModelNode() const
{
    if (m_pathItem)
        return m_pathItem->formEditorItem()->qmlItemNode().modelNode();

    return ModelNode();
}

void PathTool::pathChanged()
{
    if (m_pathItem)
        m_pathItem->updatePath();
}

}
