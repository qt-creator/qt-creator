// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "formeditorscene.h"
#include "formeditoritem.h"
#include "formeditortracing.h"
#include "formeditorview.h"
#include "formeditorwidget.h"

#include <designeralgorithm.h>
#include <designersettings.h>
#include <nodehints.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <QGraphicsSceneDragDropEvent>

#include <QEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsView>

#include <QDebug>
#include <QElapsedTimer>
#include <QList>
#include <QTime>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/ranges.h>

namespace QmlDesigner {

using NanotraceHR::keyValue;

using FormEditorTracing::category;

FormEditorScene::FormEditorScene(FormEditorWidget *view, FormEditorView *editorView)
        : QGraphicsScene()
        , m_editorView(editorView)
        , m_showBoundingRects(false)
        , m_annotationVisibility(false)
{
    NanotraceHR::Tracer tracer{"form editor scene constructor", category()};

    setupScene();
    view->setScene(this);
    setItemIndexMethod(QGraphicsScene::NoIndex);
}

FormEditorScene::~FormEditorScene()
{
    NanotraceHR::Tracer tracer{"form editor scene destructor", category()};

    clear(); //FormEditorItems have to be cleared before destruction
             //Reason: FormEditorItems accesses FormEditorScene in destructor
}


void FormEditorScene::setupScene()
{
    NanotraceHR::Tracer tracer{"form editor scene setup scene", category()};

    m_formLayerItem = new LayerItem(this);
    m_manipulatorLayerItem = new LayerItem(this);
    m_manipulatorLayerItem->setZValue(1.0);
    m_manipulatorLayerItem->setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
    m_formLayerItem->setZValue(0.0);
    m_formLayerItem->setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
}

void FormEditorScene::resetScene()
{
    NanotraceHR::Tracer tracer{"form editor scene reset scene", category()};

    const QList<QGraphicsItem *> items = m_manipulatorLayerItem->childItems();
    for (QGraphicsItem *item : items) {
       removeItem(item);
       delete item;
    }

    setSceneRect(-canvasWidth()/2., -canvasHeight()/2., canvasWidth(), canvasHeight());
}

FormEditorItem* FormEditorScene::itemForQmlItemNode(const QmlItemNode &qmlItemNode) const
{
    NanotraceHR::Tracer tracer{"form editor scene item for qml item node", category()};

    return m_qmlItemNodeItemHash.value(qmlItemNode);
}

double FormEditorScene::canvasWidth() const
{
    NanotraceHR::Tracer tracer{"form editor scene canvas width", category()};

    return designerSettings().canvasWidth();
}

double FormEditorScene::canvasHeight() const
{
    NanotraceHR::Tracer tracer{"form editor scene canvas height", category()};

    return designerSettings().canvasHeight();
}

QList<FormEditorItem *> FormEditorScene::itemsForQmlItemNodes(
    const Utils::span<const QmlItemNode> &nodeList) const
{
    NanotraceHR::Tracer tracer{"form editor scene items for qml item nodes", category()};

    return CoreUtils::to<QList>(nodeList
                                    | std::views::transform(
                                        std::bind_front(&FormEditorScene::itemForQmlItemNode, this))
                                    | Utils::views::cache_latest | std::views::filter(std::identity{}),
                                nodeList.size());
}

QList<FormEditorItem*> FormEditorScene::allFormEditorItems() const
{
    NanotraceHR::Tracer tracer{"form editor scene all form editor items", category()};

    return m_qmlItemNodeItemHash.values();
}

void FormEditorScene::updateAllFormEditorItems()
{
    NanotraceHR::Tracer tracer{"form editor scene update all form editor items", category()};

    const QList<FormEditorItem *> items = allFormEditorItems();
    for (FormEditorItem *item : items)
        item->update();
}

void FormEditorScene::removeItemFromHash(FormEditorItem *item)
{
    NanotraceHR::Tracer tracer{"form editor scene remove item from hash", category()};

    m_qmlItemNodeItemHash.remove(item->qmlItemNode());
}

AbstractFormEditorTool* FormEditorScene::currentTool() const
{
    NanotraceHR::Tracer tracer{"form editor scene current tool", category()};

    return m_editorView->currentTool();
}

//This function calculates the possible parent for reparent
FormEditorItem* FormEditorScene::calulateNewParent(FormEditorItem *formEditorItem)
{
    NanotraceHR::Tracer tracer{"form editor scene calculate new parent",
                               category(),
                               keyValue("form editor item", formEditorItem)};

    if (formEditorItem->qmlItemNode().isValid()) {
        const QList<QGraphicsItem *> list = items(formEditorItem->qmlItemNode().instanceBoundingRect().center());
        for (QGraphicsItem *graphicsItem : list) {
            if (qgraphicsitem_cast<FormEditorItem*>(graphicsItem) &&
                graphicsItem->collidesWithItem(formEditorItem, Qt::ContainsItemShape))
                return qgraphicsitem_cast<FormEditorItem*>(graphicsItem);
        }
    }

    return nullptr;
}

void FormEditorScene::synchronizeTransformation(FormEditorItem *item)
{
    NanotraceHR::Tracer tracer{"form editor scene synchronize transformation",
                               category(),
                               keyValue("form editor item", item)};

    item->updateGeometry();
    item->update();

    if (item->qmlItemNode().isRootNode()) {
        formLayerItem()->update();
        manipulatorLayerItem()->update();
    }
}

void FormEditorScene::synchronizeParent(const QmlItemNode &qmlItemNode)
{
    NanotraceHR::Tracer tracer{"form editor scene synchronize parent",
                               category(),
                               keyValue("model node", qmlItemNode)};

    QmlItemNode parentNode = qmlItemNode.instanceParent().toQmlItemNode();
    reparentItem(qmlItemNode, parentNode);
}

void FormEditorScene::synchronizeOtherProperty(FormEditorItem *item, PropertyNameView propertyName)
{
    NanotraceHR::Tracer tracer{"form editor scene synchronize other property",
                               category(),
                               keyValue("form editor item", item),
                               keyValue("property name", propertyName)};

    Q_ASSERT(item);

    item->synchronizeOtherProperty(propertyName);
}

FormEditorItem *FormEditorScene::addFormEditorItem(const QmlItemNode &qmlItemNode, ItemType type)
{
    NanotraceHR::Tracer tracer{"form editor scene add form editor item",
                               category(),
                               keyValue("model node", qmlItemNode)};

    FormEditorItem *formEditorItem = nullptr;

    if (type == Preview3d)
        formEditorItem = new FormEditor3dPreview(qmlItemNode, this);
    else
        formEditorItem = new FormEditorItem(qmlItemNode, this);

    QTC_ASSERT(!m_qmlItemNodeItemHash.contains(qmlItemNode), ;);

    m_qmlItemNodeItemHash.insert(qmlItemNode, formEditorItem);
    if (qmlItemNode.isRootNode()) {
        setSceneRect(-canvasWidth()/2., -canvasHeight()/2., canvasWidth(), canvasHeight());
        formLayerItem()->update();
        manipulatorLayerItem()->update();
    }

    return formEditorItem;
}

void FormEditorScene::dropEvent(QGraphicsSceneDragDropEvent * event)
{
    NanotraceHR::Tracer tracer{"form editor scene drop event", category()};

    currentTool()->dropEvent(removeLayerItems(itemsAt(event->scenePos())), event);

    if (views().constFirst())
        views().constFirst()->setFocus();
}

void FormEditorScene::dragEnterEvent(QGraphicsSceneDragDropEvent * event)
{
    NanotraceHR::Tracer tracer{"form editor scene drag enter event", category()};

    currentTool()->dragEnterEvent(removeLayerItems(itemsAt(event->scenePos())), event);
}

void FormEditorScene::dragLeaveEvent(QGraphicsSceneDragDropEvent * event)
{
    NanotraceHR::Tracer tracer{"form editor scene drag leave event", category()};

    currentTool()->dragLeaveEvent(removeLayerItems(itemsAt(event->scenePos())), event);

    return;
}

void FormEditorScene::dragMoveEvent(QGraphicsSceneDragDropEvent * event)
{
    NanotraceHR::Tracer tracer{"form editor scene drag move event", category()};

    currentTool()->dragMoveEvent(removeLayerItems(itemsAt(event->scenePos())), event);
}

QList<QGraphicsItem *> FormEditorScene::removeLayerItems(const QList<QGraphicsItem *> &itemList)
{
    NanotraceHR::Tracer tracer{"form editor scene remove layer items", category()};

    QList<QGraphicsItem *> itemListWithoutLayerItems;

    for (QGraphicsItem *item : itemList)
        if (item != manipulatorLayerItem() && item != formLayerItem())
            itemListWithoutLayerItems.append(item);

    return itemListWithoutLayerItems;
}

QList<QGraphicsItem *> FormEditorScene::itemsAt(const QPointF &pos)
{
    NanotraceHR::Tracer tracer{"form editor scene items at", category()};

    QTransform transform;

    if (!views().isEmpty())
        transform = views().constFirst()->transform();

    return items(pos,
                 Qt::IntersectsItemShape,
                 Qt::DescendingOrder,
                 transform);
}

void FormEditorScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    NanotraceHR::Tracer tracer{"form editor scene mouse press event", category()};

    event->ignore();
    QGraphicsScene::mousePressEvent(event);
    if (event->isAccepted())
        return;

    if (editorView() && editorView()->model())
        currentTool()->mousePressEvent(removeLayerItems(itemsAt(event->scenePos())), event);
}

static QElapsedTimer staticTimer()
{
    QElapsedTimer timer;
    timer.start();
    return timer;
}

void FormEditorScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    NanotraceHR::Tracer tracer{"form editor scene mouse move event", category()};

    static QElapsedTimer time = staticTimer();

    QGraphicsScene::mouseMoveEvent(event);

    if (time.elapsed() > 30) {
        time.restart();
        if (event->buttons())
            currentTool()->mouseMoveEvent(removeLayerItems(itemsAt(event->scenePos())), event);
        else
            currentTool()->hoverMoveEvent(removeLayerItems(itemsAt(event->scenePos())), event);

        event->accept();
    }
}

void FormEditorScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    NanotraceHR::Tracer tracer{"form editor scene mouse release event", category()};

    event->ignore();
    QGraphicsScene::mouseReleaseEvent(event);
    if (event->isAccepted())
        return;

    if (editorView() && editorView()->model()) {
        currentTool()->mouseReleaseEvent(removeLayerItems(itemsAt(event->scenePos())), event);

        event->accept();
    }
 }

void FormEditorScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    NanotraceHR::Tracer tracer{"form editor scene mouse double click event", category()};

    event->ignore();
    QGraphicsScene::mousePressEvent(event);
    if (event->isAccepted())
        return;

    if (editorView() && editorView()->model()) {
        currentTool()->mouseDoubleClickEvent(removeLayerItems(itemsAt(event->scenePos())), event);

        event->accept();
    }
}

void FormEditorScene::keyPressEvent(QKeyEvent *keyEvent)
{
    NanotraceHR::Tracer tracer{"form editor scene key press event", category()};

    if (editorView() && editorView()->model())
        currentTool()->keyPressEvent(keyEvent);
}

void FormEditorScene::keyReleaseEvent(QKeyEvent *keyEvent)
{
    NanotraceHR::Tracer tracer{"form editor scene key release event", category()};

    if (editorView() && editorView()->model())
        currentTool()->keyReleaseEvent(keyEvent);
}

void FormEditorScene::focusOutEvent(QFocusEvent *focusEvent)
{
    NanotraceHR::Tracer tracer{"form editor scene focus out event", category()};

    if (currentTool())
        currentTool()->focusLost();

    QmlDesignerPlugin::emitUsageStatisticsTime(Constants::EVENT_FORMEDITOR_TIME,
                                               m_usageTimer.elapsed());

    QGraphicsScene::focusOutEvent(focusEvent);
}

void FormEditorScene::focusInEvent(QFocusEvent *focusEvent)
{
    NanotraceHR::Tracer tracer{"form editor scene focus in event", category()};

    m_usageTimer.restart();
    QGraphicsScene::focusInEvent(focusEvent);
}

FormEditorView *FormEditorScene::editorView() const
{
    return m_editorView;
}

LayerItem* FormEditorScene::manipulatorLayerItem() const
{
    return m_manipulatorLayerItem.data();
}

LayerItem* FormEditorScene::formLayerItem() const
{
    return m_formLayerItem.data();
}

bool FormEditorScene::event(QEvent * event)
{
    NanotraceHR::Tracer tracer{"form editor scene event", category()};

    switch (event->type()) {
    case QEvent::GraphicsSceneHoverEnter:
        hoverEnterEvent(static_cast<QGraphicsSceneHoverEvent *>(event));
        return QGraphicsScene::event(event);
    case QEvent::GraphicsSceneHoverMove:
        hoverMoveEvent(static_cast<QGraphicsSceneHoverEvent *>(event));
        return QGraphicsScene::event(event);
    case QEvent::GraphicsSceneHoverLeave:
        hoverLeaveEvent(static_cast<QGraphicsSceneHoverEvent *>(event));
        return QGraphicsScene::event(event);
    case QEvent::ShortcutOverride:
        if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape) {
            currentTool()->keyPressEvent(static_cast<QKeyEvent *>(event));
            return true;
        }
        Q_FALLTHROUGH();
    default:
        return QGraphicsScene::event(event);
    }
}

void FormEditorScene::hoverEnterEvent(QGraphicsSceneHoverEvent * /*event*/)
{
}

void FormEditorScene::hoverMoveEvent(QGraphicsSceneHoverEvent * /*event*/)
{
}

void FormEditorScene::hoverLeaveEvent(QGraphicsSceneHoverEvent * /*event*/)
{
}

void FormEditorScene::reparentItem(const QmlItemNode &node, const QmlItemNode &newParent)
{
    NanotraceHR::Tracer tracer{"form editor scene reparent item",
                               category(),
                               keyValue("model node", node),
                               keyValue("new parent", newParent)};

    if (FormEditorItem *item = itemForQmlItemNode(node)) {
        item->setParentItem(nullptr);
        if (newParent.isValid()) {
            if (FormEditorItem *parentItem = itemForQmlItemNode(newParent))
                item->setParentItem(parentItem);
        }
    } else {
        Q_ASSERT(itemForQmlItemNode(node));
    }
}

FormEditorItem* FormEditorScene::rootFormEditorItem() const
{
    NanotraceHR::Tracer tracer{"form editor scene root form editor item", category()};

    return itemForQmlItemNode(editorView()->rootModelNode());
}

void FormEditorScene::clearFormEditorItems()
{
    NanotraceHR::Tracer tracer{"form editor scene clear form editor items", category()};

    const QList<QGraphicsItem *> itemList(items());

    auto cast = [](QGraphicsItem *item) { return qgraphicsitem_cast<FormEditorItem *>(item); };

    auto formEditorItems = Utils::span{itemList} | std::views::transform(cast)
                           | std::views::filter(std::identity{});

    for (FormEditorItem *item : formEditorItems)
        item->setParentItem(nullptr);

    for (FormEditorItem *item : formEditorItems)
            delete item;
}

void FormEditorScene::highlightBoundingRect(FormEditorItem *highlighItem)
{
    NanotraceHR::Tracer tracer{"form editor scene highlight bounding rect",
                               category(),
                               keyValue("highlight item", highlighItem)};

    QList<FormEditorItem *> items = allFormEditorItems();
    for (FormEditorItem *item : items) {
        if (item == highlighItem)
            item->setHighlightBoundingRect(true);
        else
            item->setHighlightBoundingRect(false);
    }
}

void FormEditorScene::setShowBoundingRects(bool show)
{
    NanotraceHR::Tracer tracer{"form editor scene set show bounding rects",
                               category(),
                               keyValue("show", show)};

    m_showBoundingRects = show;
    updateAllFormEditorItems();
}

bool FormEditorScene::showBoundingRects() const
{
    NanotraceHR::Tracer tracer{"form editor scene show bounding rects", category()};

    return m_showBoundingRects;
}

bool FormEditorScene::annotationVisibility() const
{
    NanotraceHR::Tracer tracer{"form editor scene annotation visibility", category()};

    return m_annotationVisibility;
}

void FormEditorScene::setAnnotationVisibility(bool status)
{
    NanotraceHR::Tracer tracer{"form editor scene set annotation visibility",
                               category(),
                               keyValue("status", status)};

    m_annotationVisibility = status;
}

}

