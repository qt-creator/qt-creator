/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qdeclarativedesignview.h"
#include "qdeclarativedesignview_p.h"
#include "qdeclarativedesigndebugserver.h"
#include "selectiontool.h"
#include "zoomtool.h"
#include "colorpickertool.h"
#include "layeritem.h"
#include "boundingrecthighlighter.h"
#include "subcomponenteditortool.h"
#include "qmltoolbar.h"

#include <QDeclarativeItem>
#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#include <QDeclarativeExpression>
#include <QWidget>
#include <QMouseEvent>
#include <QGraphicsObject>
#include <QApplication>

#include <QAbstractAnimation>
#include <private/qabstractanimation_p.h>

namespace QmlViewer {

const int SceneChangeUpdateInterval = 5000;

Q_GLOBAL_STATIC(QDeclarativeDesignDebugServer, qmlDesignDebugServer)

QDeclarativeDesignViewPrivate::QDeclarativeDesignViewPrivate(QDeclarativeDesignView *q) :
    q(q),
    designModeBehavior(false),
    executionPaused(false),
    slowdownFactor(1.0f),
    toolbar(0)
{
    sceneChangedTimer.setInterval(SceneChangeUpdateInterval);
    sceneChangedTimer.setSingleShot(true);
}

QDeclarativeDesignViewPrivate::~QDeclarativeDesignViewPrivate()
{
}

QDeclarativeDesignView::QDeclarativeDesignView(QWidget *parent) :
    QDeclarativeView(parent), data(new QDeclarativeDesignViewPrivate(this))
{
    data->manipulatorLayer = new LayerItem(scene());
    data->selectionTool = new SelectionTool(this);
    data->zoomTool = new ZoomTool(this);
    data->colorPickerTool = new ColorPickerTool(this);
    data->boundingRectHighlighter = new BoundingRectHighlighter(this);
    data->subcomponentEditorTool = new SubcomponentEditorTool(this);
    data->currentTool = data->selectionTool;

    connect(scene(), SIGNAL(changed(QList<QRectF>)), SLOT(_q_sceneChanged(QList<QRectF>)));
    setMouseTracking(true);

    connect(qmlDesignDebugServer(), SIGNAL(designModeBehaviorChanged(bool)), SLOT(setDesignModeBehavior(bool)));
    connect(qmlDesignDebugServer(), SIGNAL(reloadRequested()), SLOT(_q_reloadView()));
    connect(qmlDesignDebugServer(),
            SIGNAL(currentObjectsChanged(QList<QObject*>)),
            SLOT(_q_onCurrentObjectsChanged(QList<QObject*>)));
    connect(qmlDesignDebugServer(), SIGNAL(animationSpeedChangeRequested(qreal)), SLOT(changeAnimationSpeed(qreal)));
    connect(qmlDesignDebugServer(), SIGNAL(colorPickerToolRequested()), SLOT(_q_changeToColorPickerTool()));
    connect(qmlDesignDebugServer(), SIGNAL(selectMarqueeToolRequested()), SLOT(_q_changeToMarqueeSelectTool()));
    connect(qmlDesignDebugServer(), SIGNAL(selectToolRequested()), SLOT(_q_changeToSingleSelectTool()));
    connect(qmlDesignDebugServer(), SIGNAL(zoomToolRequested()), SLOT(_q_changeToZoomTool()));
    connect(qmlDesignDebugServer(),
            SIGNAL(objectCreationRequested(QString,QObject*,QStringList,QString)),
            SLOT(_q_createQmlObject(QString,QObject*,QStringList,QString)));
    connect(qmlDesignDebugServer(), SIGNAL(contextPathIndexChanged(int)), SLOT(_q_changeContextPathIndex(int)));
    connect(qmlDesignDebugServer(), SIGNAL(clearComponentCacheRequested()), SLOT(_q_clearComponentCache()));
    connect(this, SIGNAL(statusChanged(QDeclarativeView::Status)), SLOT(_q_onStatusChanged(QDeclarativeView::Status)));

    connect(data->colorPickerTool, SIGNAL(selectedColorChanged(QColor)), SIGNAL(selectedColorChanged(QColor)));
    connect(data->colorPickerTool, SIGNAL(selectedColorChanged(QColor)),
            qmlDesignDebugServer(), SLOT(selectedColorChanged(QColor)));

    connect(data->subcomponentEditorTool, SIGNAL(cleared()), SIGNAL(inspectorContextCleared()));
    connect(data->subcomponentEditorTool, SIGNAL(contextPushed(QString)), SIGNAL(inspectorContextPushed(QString)));
    connect(data->subcomponentEditorTool, SIGNAL(contextPopped()), SIGNAL(inspectorContextPopped()));
    connect(data->subcomponentEditorTool, SIGNAL(contextPathChanged(QStringList)), qmlDesignDebugServer(), SLOT(contextPathUpdated(QStringList)));

    connect(&(data->sceneChangedTimer), SIGNAL(timeout()), SLOT(_q_checkSceneItemCount()));

    data->createToolbar();

    data->_q_changeToSingleSelectTool();
}

QDeclarativeDesignView::~QDeclarativeDesignView()
{
}

void QDeclarativeDesignView::setInspectorContext(int contextIndex)
{
    if (data->subcomponentEditorTool->contextIndex() != contextIndex) {
        QGraphicsObject *object = data->subcomponentEditorTool->setContext(contextIndex);
        if (object)
            qmlDesignDebugServer()->setCurrentObjects(QList<QObject*>() << object);
    }
}

void QDeclarativeDesignViewPrivate::_q_reloadView()
{
    subcomponentEditorTool->clear();
    clearHighlight();
    emit q->reloadRequested();
}

void QDeclarativeDesignViewPrivate::clearEditorItems()
{
    clearHighlight();
    setSelectedItems(QList<QGraphicsItem*>());
}

void QDeclarativeDesignView::leaveEvent(QEvent *event)
{
    if (!data->designModeBehavior) {
        QDeclarativeView::leaveEvent(event);
        return;
    }
    data->clearHighlight();
}

void QDeclarativeDesignView::mousePressEvent(QMouseEvent *event)
{
    if (!data->designModeBehavior) {
        QDeclarativeView::mousePressEvent(event);
        return;
    }
    data->cursorPos = event->pos();
    data->currentTool->mousePressEvent(event);
}

void QDeclarativeDesignView::mouseMoveEvent(QMouseEvent *event)
{
    if (!data->designModeBehavior) {
        data->clearEditorItems();
        QDeclarativeView::mouseMoveEvent(event);
        return;
    }
    data->cursorPos = event->pos();

    QList<QGraphicsItem*> selItems = data->selectableItems(event->pos());
    if (!selItems.isEmpty()) {
        setToolTip(AbstractFormEditorTool::titleForItem(selItems.first()));
    } else {
        setToolTip(QString());
    }
    if (event->buttons()) {
        data->subcomponentEditorTool->mouseMoveEvent(event);
        data->currentTool->mouseMoveEvent(event);
    } else {
        data->subcomponentEditorTool->hoverMoveEvent(event);
        data->currentTool->hoverMoveEvent(event);
    }
}

void QDeclarativeDesignView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!data->designModeBehavior) {
        QDeclarativeView::mouseReleaseEvent(event);
        return;
    }
    data->subcomponentEditorTool->mouseReleaseEvent(event);

    data->cursorPos = event->pos();
    data->currentTool->mouseReleaseEvent(event);

    qmlDesignDebugServer()->setCurrentObjects(AbstractFormEditorTool::toObjectList(selectedItems()));
}

void QDeclarativeDesignView::keyPressEvent(QKeyEvent *event)
{
    if (!data->designModeBehavior) {
        QDeclarativeView::keyPressEvent(event);
        return;
    }
    data->currentTool->keyPressEvent(event);
}

void QDeclarativeDesignView::keyReleaseEvent(QKeyEvent *event)
{
    if (!data->designModeBehavior) {
        QDeclarativeView::keyReleaseEvent(event);
        return;
    }

    switch(event->key()) {
    case Qt::Key_V:
        data->_q_changeToSingleSelectTool();
        break;
    case Qt::Key_M:
        data->_q_changeToMarqueeSelectTool();
        break;
    case Qt::Key_I:
        data->_q_changeToColorPickerTool();
        break;
    case Qt::Key_Z:
        data->_q_changeToZoomTool();
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (!data->selectedItems().isEmpty())
            data->subcomponentEditorTool->setCurrentItem(data->selectedItems().first());
        break;
    case Qt::Key_Space:
        if (data->executionPaused) {
            continueExecution(data->slowdownFactor);
        } else {
            pauseExecution();
        }
        break;
    default:
        break;
    }

    data->currentTool->keyReleaseEvent(event);
}

void QDeclarativeDesignViewPrivate::_q_createQmlObject(const QString &qml, QObject *parent, const QStringList &importList, const QString &filename)
{
    if (!parent)
        return;

    QString imports;
    foreach(const QString &s, importList) {
        imports += s + "\n";
    }

    QDeclarativeContext *parentContext = q->engine()->contextForObject(parent);
    QDeclarativeComponent component(q->engine(), q);
    QByteArray constructedQml = QString(imports + qml).toLatin1();

    component.setData(constructedQml, filename);
    QObject *newObject = component.create(parentContext);
    if (newObject) {
        newObject->setParent(parent);
        QDeclarativeItem *parentItem = qobject_cast<QDeclarativeItem*>(parent);
        QDeclarativeItem *newItem    = qobject_cast<QDeclarativeItem*>(newObject);
        if (parentItem && newItem) {
            newItem->setParentItem(parentItem);
        }
    }
}

void QDeclarativeDesignViewPrivate::_q_clearComponentCache()
{
    q->engine()->clearComponentCache();
}

QGraphicsItem *QDeclarativeDesignViewPrivate::currentRootItem() const
{
    return subcomponentEditorTool->currentRootItem();
}

void QDeclarativeDesignView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!data->designModeBehavior) {
        QDeclarativeView::mouseDoubleClickEvent(event);
        return;
    }

    if (data->currentToolMode != Constants::SelectionToolMode
     && data->currentToolMode != Constants::MarqueeSelectionToolMode)
    {
        return;
    }

    QGraphicsItem *itemToEnter = 0;
    QList<QGraphicsItem*> itemList = items(event->pos());
    data->filterForSelection(itemList);

    if (data->selectedItems().isEmpty() && !itemList.isEmpty()) {
        itemToEnter = itemList.first();
    } else if (!data->selectedItems().isEmpty() && !itemList.isEmpty()) {
        itemToEnter = itemList.first();
    }

    if (itemToEnter)
        itemToEnter = data->subcomponentEditorTool->firstChildOfContext(itemToEnter);

    data->subcomponentEditorTool->setCurrentItem(itemToEnter);
    data->subcomponentEditorTool->mouseDoubleClickEvent(event);

    if ((event->buttons() & Qt::LeftButton) && itemToEnter) {
        QGraphicsObject *objectToEnter = itemToEnter->toGraphicsObject();
        if (objectToEnter)
            qmlDesignDebugServer()->setCurrentObjects(QList<QObject*>() << objectToEnter);
    }

}
void QDeclarativeDesignView::wheelEvent(QWheelEvent *event)
{
    if (!data->designModeBehavior) {
        QDeclarativeView::wheelEvent(event);
        return;
    }
    data->currentTool->wheelEvent(event);
}

void QDeclarativeDesignView::setDesignModeBehavior(bool value)
{
    emit designModeBehaviorChanged(value);

    data->toolbar->setDesignModeBehavior(value);
    qmlDesignDebugServer()->setDesignModeBehavior(value);

    data->designModeBehavior = value;
    if (data->subcomponentEditorTool) {
        data->subcomponentEditorTool->clear();
        data->clearHighlight();
        data->setSelectedItems(QList<QGraphicsItem*>());

        if (rootObject())
            data->subcomponentEditorTool->pushContext(rootObject());
    }

    if (!data->designModeBehavior)
        data->clearEditorItems();
}

bool QDeclarativeDesignView::designModeBehavior()
{
    return data->designModeBehavior;
}

void QDeclarativeDesignViewPrivate::changeTool(Constants::DesignTool tool, Constants::ToolFlags /*flags*/)
{
    switch(tool) {
    case Constants::SelectionToolMode:
        _q_changeToSingleSelectTool();
        break;
    case Constants::NoTool:
    default:
        currentTool = 0;
        break;
    }
}

void QDeclarativeDesignViewPrivate::setSelectedItemsForTools(QList<QGraphicsItem *> items)
{
    currentSelection.clear();
    foreach(QGraphicsItem *item, items) {
        if (item) {
            QGraphicsObject *obj = item->toGraphicsObject();
            if (obj)
                currentSelection << obj;
        }
    }
    currentTool->updateSelectedItems();
}

void QDeclarativeDesignViewPrivate::setSelectedItems(QList<QGraphicsItem *> items)
{
    setSelectedItemsForTools(items);
    qmlDesignDebugServer()->setCurrentObjects(AbstractFormEditorTool::toObjectList(items));
}

QList<QGraphicsItem *> QDeclarativeDesignViewPrivate::selectedItems()
{
    QList<QGraphicsItem *> selection;
    foreach(const QWeakPointer<QGraphicsObject> &selectedObject, currentSelection) {
        if (selectedObject.isNull()) {
            currentSelection.removeOne(selectedObject);
        } else {
            selection << selectedObject.data();
        }
    }

    return selection;
}

void QDeclarativeDesignView::setSelectedItems(QList<QGraphicsItem *> items)
{
    data->setSelectedItems(items);
}

QList<QGraphicsItem *> QDeclarativeDesignView::selectedItems()
{
    return data->selectedItems();
}

void QDeclarativeDesignViewPrivate::clearHighlight()
{
    boundingRectHighlighter->clear();
}

void QDeclarativeDesignViewPrivate::highlight(QGraphicsObject * item, ContextFlags flags)
{
    highlight(QList<QGraphicsObject*>() << item, flags);
}

void QDeclarativeDesignViewPrivate::highlight(QList<QGraphicsObject *> items, ContextFlags flags)
{
    if (items.isEmpty())
        return;

    QList<QGraphicsObject*> objectList;
    foreach(QGraphicsItem *item, items) {
        QGraphicsItem *child = item;
        if (flags & ContextSensitive)
            child = subcomponentEditorTool->firstChildOfContext(item);

        if (child) {
            QGraphicsObject *childObject = child->toGraphicsObject();
            if (childObject)
                objectList << childObject;
        }
    }

    boundingRectHighlighter->highlight(objectList);
}

bool QDeclarativeDesignViewPrivate::mouseInsideContextItem() const
{
    return subcomponentEditorTool->containsCursor(cursorPos.toPoint());
}

QList<QGraphicsItem*> QDeclarativeDesignViewPrivate::selectableItems(const QPointF &scenePos) const
{
    QList<QGraphicsItem*> itemlist = q->scene()->items(scenePos);
    return filterForCurrentContext(itemlist);
}

QList<QGraphicsItem*> QDeclarativeDesignViewPrivate::selectableItems(const QPoint &pos) const
{
    QList<QGraphicsItem*> itemlist = q->items(pos);
    return filterForCurrentContext(itemlist);
}

QList<QGraphicsItem*> QDeclarativeDesignViewPrivate::selectableItems(const QRectF &sceneRect, Qt::ItemSelectionMode selectionMode) const
{
    QList<QGraphicsItem*> itemlist = q->scene()->items(sceneRect, selectionMode);

    return filterForCurrentContext(itemlist);
}

void QDeclarativeDesignViewPrivate::_q_changeToSingleSelectTool()
{
    currentToolMode = Constants::SelectionToolMode;
    selectionTool->setRubberbandSelectionMode(false);

    changeToSelectTool();

    emit q->selectToolActivated();
    qmlDesignDebugServer()->setCurrentTool(Constants::SelectionToolMode);
}

void QDeclarativeDesignViewPrivate::changeToSelectTool()
{
    if (currentTool == selectionTool)
        return;

    currentTool->clear();
    currentTool = selectionTool;
    currentTool->clear();
    currentTool->updateSelectedItems();
}

void QDeclarativeDesignViewPrivate::_q_changeToMarqueeSelectTool()
{
    changeToSelectTool();
    currentToolMode = Constants::MarqueeSelectionToolMode;
    selectionTool->setRubberbandSelectionMode(true);

    emit q->marqueeSelectToolActivated();
    qmlDesignDebugServer()->setCurrentTool(Constants::MarqueeSelectionToolMode);
}

void QDeclarativeDesignViewPrivate::_q_changeToZoomTool()
{
    currentToolMode = Constants::ZoomMode;
    currentTool->clear();
    currentTool = zoomTool;
    currentTool->clear();

    emit q->zoomToolActivated();
    qmlDesignDebugServer()->setCurrentTool(Constants::ZoomMode);
}

void QDeclarativeDesignViewPrivate::_q_changeToColorPickerTool()
{
    if (currentTool == colorPickerTool)
        return;

    currentToolMode = Constants::ColorPickerMode;
    currentTool->clear();
    currentTool = colorPickerTool;
    currentTool->clear();

    emit q->colorPickerActivated();
    qmlDesignDebugServer()->setCurrentTool(Constants::ColorPickerMode);
}

void QDeclarativeDesignViewPrivate::_q_changeContextPathIndex(int index)
{
    subcomponentEditorTool->setContext(index);
}

void QDeclarativeDesignViewPrivate::_q_sceneChanged(const QList<QRectF> & /*areas*/)
{
    if (designModeBehavior)
        return;

    if (!sceneChangedTimer.isActive())
        sceneChangedTimer.start();
}

void QDeclarativeDesignViewPrivate::_q_checkSceneItemCount()
{
    bool hasNewItems = hasNewGraphicsObjects(q->rootObject());

    if (hasNewItems) {
        qmlDesignDebugServer()->sceneItemCountChanged();
    }
}

static bool hasNewGraphicsObjectsRecur(QGraphicsObject *object,
                                  QSet<QGraphicsObject *> &newItems,
                                  const QSet<QGraphicsObject *> &previousItems)
{
    bool hasNew = false;

    newItems << object;

    foreach(QGraphicsItem *item, object->childItems()) {
        QGraphicsObject *gfxObject = item->toGraphicsObject();
        if (gfxObject) {
            newItems << gfxObject;

            hasNew = hasNewGraphicsObjectsRecur(gfxObject, newItems, previousItems) || hasNew;
            if (!previousItems.contains(gfxObject))
                hasNew = true;
        }
    }

    return hasNew;
}

bool QDeclarativeDesignViewPrivate::hasNewGraphicsObjects(QGraphicsObject *object)
{
    QSet<QGraphicsObject *> newItems;
    bool ret = hasNewGraphicsObjectsRecur(object, newItems, sceneGraphicsObjects);
    sceneGraphicsObjects = newItems;

    return ret;
}

void QDeclarativeDesignView::changeAnimationSpeed(qreal slowdownFactor)
{
    data->slowdownFactor = slowdownFactor;

    if (data->slowdownFactor != 0) {
        continueExecution(data->slowdownFactor);
    } else {
        pauseExecution();
    }
}

void QDeclarativeDesignView::continueExecution(qreal slowdownFactor)
{
    Q_ASSERT(slowdownFactor > 0);

    data->slowdownFactor = slowdownFactor;
    static const qreal animSpeedSnapDelta = 0.01f;
    bool useStandardSpeed = (qAbs(1.0f - data->slowdownFactor) < animSpeedSnapDelta);

    QUnifiedTimer *timer = QUnifiedTimer::instance();
    timer->setSlowdownFactor(data->slowdownFactor);
    timer->setSlowModeEnabled(!useStandardSpeed);
    data->executionPaused = false;

    emit executionStarted(data->slowdownFactor);
    qmlDesignDebugServer()->setAnimationSpeed(data->slowdownFactor);
}

void QDeclarativeDesignView::pauseExecution()
{
    QUnifiedTimer *timer = QUnifiedTimer::instance();
    timer->setSlowdownFactor(0);
    timer->setSlowModeEnabled(true);
    data->executionPaused = true;

    emit executionPaused();
    qmlDesignDebugServer()->setAnimationSpeed(0);
}

void QDeclarativeDesignViewPrivate::_q_applyChangesFromClient()
{

}


QList<QGraphicsItem*> QDeclarativeDesignViewPrivate::filterForSelection(QList<QGraphicsItem*> &itemlist) const
{
    foreach(QGraphicsItem *item, itemlist) {
        if (isEditorItem(item) || !subcomponentEditorTool->isChildOfContext(item))
            itemlist.removeOne(item);
    }

    return itemlist;
}

QList<QGraphicsItem*> QDeclarativeDesignViewPrivate::filterForCurrentContext(QList<QGraphicsItem*> &itemlist) const
{
    foreach(QGraphicsItem *item, itemlist) {

        if (isEditorItem(item) || !subcomponentEditorTool->isDirectChildOfContext(item)) {

            // if we're a child, but not directly, replace with the parent that is directly in context.
            if (QGraphicsItem *contextParent = subcomponentEditorTool->firstChildOfContext(item)) {
                if (contextParent != item) {
                    if (itemlist.contains(contextParent)) {
                        itemlist.removeOne(item);
                    } else {
                        itemlist.replace(itemlist.indexOf(item), contextParent);
                    }
                }
            } else {
                itemlist.removeOne(item);
            }
        }
    }

    return itemlist;
}

bool QDeclarativeDesignViewPrivate::isEditorItem(QGraphicsItem *item) const
{
    return (item->type() == Constants::EditorItemType
         || item->type() == Constants::ResizeHandleItemType
         || item->data(Constants::EditorItemDataKey).toBool());
}

void QDeclarativeDesignViewPrivate::_q_onStatusChanged(QDeclarativeView::Status status)
{
    if (status == QDeclarativeView::Ready) {
        if (q->rootObject()) {
            hasNewGraphicsObjects(q->rootObject());
            if (subcomponentEditorTool->contextIndex() != -1)
                subcomponentEditorTool->clear();
            subcomponentEditorTool->pushContext(q->rootObject());
            emit q->executionStarted(1.0f);

        }
        qmlDesignDebugServer()->reloaded();
    }
}

void QDeclarativeDesignViewPrivate::_q_onCurrentObjectsChanged(QList<QObject*> objects)
{
    QList<QGraphicsItem*> items;
    QList<QGraphicsObject*> gfxObjects;
    foreach(QObject *obj, objects) {
        QDeclarativeItem* declarativeItem = qobject_cast<QDeclarativeItem*>(obj);
        if (declarativeItem) {
            items << declarativeItem;
            QGraphicsObject *gfxObj = declarativeItem->toGraphicsObject();
            if (gfxObj)
                gfxObjects << gfxObj;
        }
    }
    setSelectedItemsForTools(items);
    clearHighlight();
    highlight(gfxObjects, QDeclarativeDesignViewPrivate::IgnoreContext);
}

QString QDeclarativeDesignView::idStringForObject(QObject *obj)
{
    return qmlDesignDebugServer()->idStringForObject(obj);
}

// adjusts bounding boxes on edges of screen to be visible
QRectF QDeclarativeDesignView::adjustToScreenBoundaries(const QRectF &boundingRectInSceneSpace)
{
    int marginFromEdge = 1;
    QRectF boundingRect(boundingRectInSceneSpace);
    if (qAbs(boundingRect.left()) - 1 < 2) {
        boundingRect.setLeft(marginFromEdge);
    }

    if (boundingRect.right() >= rect().right() ) {
        boundingRect.setRight(rect().right() - marginFromEdge);
    }

    if (qAbs(boundingRect.top()) - 1 < 2) {
        boundingRect.setTop(marginFromEdge);
    }

    if (boundingRect.bottom() >= rect().bottom() ) {
        boundingRect.setBottom(rect().bottom() - marginFromEdge);
    }

    return boundingRect;
}

QToolBar *QDeclarativeDesignView::toolbar() const
{
    return data->toolbar;
}

void QDeclarativeDesignViewPrivate::createToolbar()
{
    toolbar = new QmlToolbar(q);
    QObject::connect(q, SIGNAL(selectedColorChanged(QColor)), toolbar, SLOT(setColorBoxColor(QColor)));

    QObject::connect(q, SIGNAL(designModeBehaviorChanged(bool)), toolbar, SLOT(setDesignModeBehavior(bool)));

    QObject::connect(toolbar, SIGNAL(designModeBehaviorChanged(bool)), q, SLOT(setDesignModeBehavior(bool)));
    QObject::connect(toolbar, SIGNAL(executionStarted()), q, SLOT(continueExecution()));
    QObject::connect(toolbar, SIGNAL(executionPaused()), q, SLOT(pauseExecution()));
    QObject::connect(toolbar, SIGNAL(colorPickerSelected()), q, SLOT(_q_changeToColorPickerTool()));
    QObject::connect(toolbar, SIGNAL(zoomToolSelected()), q, SLOT(_q_changeToZoomTool()));
    QObject::connect(toolbar, SIGNAL(selectToolSelected()), q, SLOT(_q_changeToSingleSelectTool()));
    QObject::connect(toolbar, SIGNAL(marqueeSelectToolSelected()), q, SLOT(_q_changeToMarqueeSelectTool()));

    QObject::connect(toolbar, SIGNAL(applyChangesFromQmlFileSelected()), q, SLOT(_q_applyChangesFromClient()));

    QObject::connect(q, SIGNAL(executionStarted(qreal)), toolbar, SLOT(startExecution()));
    QObject::connect(q, SIGNAL(executionPaused()), toolbar, SLOT(pauseExecution()));

    QObject::connect(q, SIGNAL(selectToolActivated()), toolbar, SLOT(activateSelectTool()));

    // disabled features
    //connect(d->m_toolbar, SIGNAL(applyChangesToQmlFileSelected()), SLOT(applyChangesToClient()));
    //connect(q, SIGNAL(resizeToolActivated()), d->m_toolbar, SLOT(activateSelectTool()));
    //connect(q, SIGNAL(moveToolActivated()),   d->m_toolbar, SLOT(activateSelectTool()));

    QObject::connect(q, SIGNAL(colorPickerActivated()), toolbar, SLOT(activateColorPicker()));
    QObject::connect(q, SIGNAL(zoomToolActivated()), toolbar, SLOT(activateZoom()));
    QObject::connect(q, SIGNAL(marqueeSelectToolActivated()), toolbar, SLOT(activateMarqueeSelectTool()));
}

} //namespace QmlViewer

#include <moc_qdeclarativedesignview.cpp>
