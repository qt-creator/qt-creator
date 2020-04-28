/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "transitiontool.h"

#include <formeditorscene.h>
#include <formeditorview.h>
#include <formeditorwidget.h>
#include <itemutilfunctions.h>
#include <formeditoritem.h>
#include <layeritem.h>

#include <resizehandleitem.h>

#include <bindingproperty.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <qmlitemnode.h>
#include <qmldesignerplugin.h>
#include <abstractaction.h>
#include <designeractionmanager.h>
#include <variantproperty.h>
#include <rewritingexception.h>
#include <rewritertransaction.h>

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QAction>
#include <QMessageBox>
#include <QPair>
#include <QGraphicsSceneMouseEvent>

namespace QmlDesigner {

static bool isTransitionSource(const ModelNode &node)
{
    return QmlFlowTargetNode::isFlowEditorTarget(node);
}

static bool isTransitionTarget(const QmlItemNode &node)
{
    return QmlFlowTargetNode::isFlowEditorTarget(node)
            && !node.isFlowActionArea()
            && !node.isFlowWildcard();
}

class TransitionToolAction : public AbstractAction
{
public:
    TransitionToolAction(const QString &name) : AbstractAction(name) {}

    QByteArray category() const override
    {
        return QByteArray();
    }

    QByteArray menuId() const override
    {
        return "TransitionTool";
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
            return isTransitionSource(selectionContext.currentSingleSelectedNode());

        return false;
    }

    bool isEnabled(const SelectionContext &selectionContext) const override
    {
        return isVisible(selectionContext);
    }
};

class TransitionCustomAction : public TransitionToolAction
{
public:
    TransitionCustomAction(const QString &name) : TransitionToolAction(name) {}

    QByteArray category() const override
    {
        return ComponentCoreConstants::flowCategory;
    }

    SelectionContext selectionContext() const
    {
        return AbstractAction::selectionContext();
    }

};

static QRectF paintedBoundingRect(FormEditorItem *item)
{
    QRectF boundingRect = item->qmlItemNode().instanceBoundingRect();
    if (boundingRect.width() < 4)
        boundingRect = item->boundingRect();
    return boundingRect;
}

static QPointF centerPoint(FormEditorItem *item)
{
     QRectF boundingRect = paintedBoundingRect(item);
     return QPointF(item->scenePos().x() + boundingRect.width() / 2,
                    item->scenePos().y() + boundingRect.height() / 2);
}

void static setToBoundingRect(QGraphicsRectItem *rect, FormEditorItem *item)
{
    QPolygonF boundingRectInSceneSpace(item->mapToScene(paintedBoundingRect(item)));
    rect->setRect(boundingRectInSceneSpace.boundingRect());
}

TransitionTool::TransitionTool()
    : QObject(), AbstractCustomTool()
{

    TransitionToolAction *transitionToolAction = new TransitionToolAction(tr("Add Transition"));
    QmlDesignerPlugin::instance()->designerActionManager().addDesignerAction(transitionToolAction);

    connect(transitionToolAction->action(), &QAction::triggered,
            this, &TransitionTool::activateTool);

    TransitionCustomAction *removeAction = new TransitionCustomAction(tr("Remove Transitions"));
    QmlDesignerPlugin::instance()->designerActionManager().addDesignerAction(removeAction);

    connect(removeAction->action(), &QAction::triggered,
            this, [removeAction](){

        SelectionContext context = removeAction->selectionContext();
        QmlFlowTargetNode node = QmlFlowTargetNode(context.currentSingleSelectedNode());

        context.view()->executeInTransaction("Remove Transitions", [&node](){
            if (node.isValid())
                node.removeTransitions();
        });
    });

    TransitionCustomAction *removeAllTransitionsAction = new TransitionCustomAction(tr("Remove All Transitions"));
    QmlDesignerPlugin::instance()->designerActionManager().addDesignerAction(removeAllTransitionsAction);

    connect(removeAllTransitionsAction->action(), &QAction::triggered,
            this, [removeAllTransitionsAction](){

        if (QMessageBox::question(Core::ICore::dialogParent(),
                                  tr("Remove All Transitions"),
                                  tr("Do you really want to remove all transitions?"),
                                  QMessageBox::Yes | QMessageBox::No) !=  QMessageBox::Yes)
            return;

        SelectionContext context = removeAllTransitionsAction->selectionContext();
        QmlFlowTargetNode node = QmlFlowTargetNode(context.currentSingleSelectedNode());

        context.view()->executeInTransaction("Remove All Transitions", [&node](){
            if (node.isValid() && node.flowView().isValid())
                node.flowView().removeAllTransitions();
        });
    });

    TransitionCustomAction *removeDanglingTransitionAction = new TransitionCustomAction(tr("Remove Dangling Transitions"));
    QmlDesignerPlugin::instance()->designerActionManager().addDesignerAction(removeDanglingTransitionAction);

    connect(removeDanglingTransitionAction->action(), &QAction::triggered,
            this, [removeDanglingTransitionAction](){

        SelectionContext context = removeDanglingTransitionAction->selectionContext();
        QmlFlowTargetNode node = QmlFlowTargetNode(context.currentSingleSelectedNode());

        context.view()->executeInTransaction("Remove Dangling Transitions", [&node](){
            if (node.isValid() && node.flowView().isValid())
                node.flowView().removeDanglingTransitions();
        });
    });
}

TransitionTool::~TransitionTool()
{
}

void TransitionTool::clear()
{
    m_lineItem.reset(nullptr);
    m_rectangleItem1.reset(nullptr);
    m_rectangleItem2.reset(nullptr);

    AbstractFormEditorTool::clear();
}

void TransitionTool::mousePressEvent(const QList<QGraphicsItem*> &itemList,
                                            QGraphicsSceneMouseEvent *event)
{
    if (m_blockEvents)
        return;

    if (event->button() != Qt::LeftButton)
        return;

    AbstractFormEditorTool::mousePressEvent(itemList, event);
    TransitionTool::mouseMoveEvent(itemList, event);
}

void TransitionTool::mouseMoveEvent(const QList<QGraphicsItem*> & itemList,
                              QGraphicsSceneMouseEvent * event)
{
    if (!m_lineItem)
        return;

    QTC_ASSERT(currentFormEditorItem(), return);

    const QPointF pos = centerPoint(m_formEditorItem);
    lineItem()->setLine(pos.x(),
                        pos.y(),
                       event->scenePos().x(),
                       event->scenePos().y());

    FormEditorItem *formEditorItem = nearestFormEditorItem(event->scenePos(), itemList);

    if (formEditorItem
            && formEditorItem->qmlItemNode().isValid()
            && isTransitionTarget(formEditorItem->qmlItemNode().modelNode())) {
        rectangleItem2()->setVisible(true);
        setToBoundingRect(rectangleItem2(), formEditorItem);
    } else {
        rectangleItem2()->setVisible(false);
    }
}

void TransitionTool::hoverMoveEvent(const QList<QGraphicsItem*> & itemList,
                        QGraphicsSceneMouseEvent *event)
{
    mouseMoveEvent(itemList, event);
}

void TransitionTool::keyPressEvent(QKeyEvent * /*keyEvent*/)
{
}

void TransitionTool::keyReleaseEvent(QKeyEvent * /*keyEvent*/)
{
    view()->changeToSelectionTool();
}

void  TransitionTool::dragLeaveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{
}

void  TransitionTool::dragMoveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{
}

void TransitionTool::mouseReleaseEvent(const QList<QGraphicsItem*> &itemList,
                                 QGraphicsSceneMouseEvent *event)
{
    if (m_blockEvents)
        return;

    if (event->button() == Qt::LeftButton) {
        FormEditorItem *formEditorItem = nearestFormEditorItem(event->scenePos(), itemList);

        if (formEditorItem
                && QmlFlowTargetNode(formEditorItem->qmlItemNode().modelNode()).isValid())
            createTransition(m_formEditorItem, formEditorItem);
    }

    view()->changeToSelectionTool();
}


void TransitionTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event)
{
    AbstractFormEditorTool::mouseDoubleClickEvent(itemList, event);
}

void TransitionTool::itemsAboutToRemoved(const QList<FormEditorItem*> &)
{
    view()->changeCurrentToolTo(this);
}

void TransitionTool::selectedItemsChanged(const QList<FormEditorItem*> &itemList)
{
    if (!itemList.isEmpty()) {
        createItems();

        m_formEditorItem = itemList.first();
        setToBoundingRect(rectangleItem1(), m_formEditorItem);
    }
}

void TransitionTool::instancesCompleted(const QList<FormEditorItem*> & /*itemList*/)
{
}

void  TransitionTool::instancesParentChanged(const QList<FormEditorItem *> & /*itemList*/)
{
}

void TransitionTool::instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > & /*propertyList*/)
{
}

void TransitionTool::formEditorItemsChanged(const QList<FormEditorItem*> & /*itemList*/)
{
}

int TransitionTool::wantHandleItem(const ModelNode &modelNode) const
{
    if (isTransitionSource(modelNode))
        return 10;

    return 0;
}

QString TransitionTool::name() const
{
    return tr("Transition Tool");
}

void TransitionTool::activateTool()
{
    view()->changeToCustomTool();
}

void TransitionTool::unblock()
{
    m_blockEvents = false;
}

QGraphicsLineItem *TransitionTool::lineItem()
{
    return m_lineItem.get();
}

QGraphicsRectItem *TransitionTool::rectangleItem1()
{
    return m_rectangleItem1.get();
}

QGraphicsRectItem *TransitionTool::rectangleItem2()
{
    return m_rectangleItem2.get();
}

FormEditorItem *TransitionTool::currentFormEditorItem() const
{
    if (scene()->items().contains(m_formEditorItem))
        return m_formEditorItem;

    return nullptr;
}

void TransitionTool::createItems() {
    m_blockEvents = true;
    QTimer::singleShot(200, this, [this](){ unblock(); });

    if (!lineItem())
        m_lineItem.reset(new QGraphicsLineItem(scene()->manipulatorLayerItem()));

    if (!rectangleItem1())
        m_rectangleItem1.reset(new QGraphicsRectItem(scene()->manipulatorLayerItem()));

    if (!rectangleItem2())
        m_rectangleItem2.reset(new QGraphicsRectItem(scene()->manipulatorLayerItem()));

    m_rectangleItem2->setVisible(false);

    QPen pen;
    pen.setColor(QColor(Qt::lightGray));
    pen.setStyle(Qt::DashLine);
    pen.setWidth(0);
    m_lineItem->setPen(pen);

    pen.setColor(QColor(108, 141, 221));
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(4);
    pen.setCosmetic(true);
    m_rectangleItem1->setPen(pen);

    m_rectangleItem2->setPen(pen);
}

void TransitionTool::createTransition(FormEditorItem *source, FormEditorItem *target)
{
    QmlFlowTargetNode sourceNode(source->qmlItemNode().modelNode());
    QmlFlowTargetNode targetNode(target->qmlItemNode().modelNode());

    if (sourceNode.isValid() && targetNode.isValid()
            && sourceNode != targetNode
            && !targetNode.isFlowActionArea()
            && !targetNode.isFlowWildcard()) {
        view()->executeInTransaction("create transition", [&sourceNode, targetNode](){
            sourceNode.assignTargetItem(targetNode);
        });
    } else {
        qWarning() << Q_FUNC_INFO << "nodes invalid";
    }
}

}
