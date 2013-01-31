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

#include "qdeclarativeviewinspector.h"
#include "qdeclarativeviewinspector_p.h"
#include "qdeclarativeinspectorservice.h"
#include "editor/liveselectiontool.h"
#include "editor/zoomtool.h"
#include "editor/colorpickertool.h"
#include "editor/livelayeritem.h"
#include "editor/boundingrecthighlighter.h"

#include "qt_private/qdeclarativedebughelper_p.h"

#include <QDeclarativeItem>
#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#include <QDeclarativeExpression>
#include <QWidget>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QGraphicsObject>
#include <QApplication>

#include "qt_private/qdeclarativestate_p.h"

namespace QmlJSDebugger {

QDeclarativeViewInspectorPrivate::QDeclarativeViewInspectorPrivate(QDeclarativeViewInspector *q) :
    q(q),
    designModeBehavior(false),
    showAppOnTop(false),
    animationPaused(false),
    slowDownFactor(1.0f)
{
}

QDeclarativeViewInspectorPrivate::~QDeclarativeViewInspectorPrivate()
{
}

QDeclarativeViewInspector::QDeclarativeViewInspector(QDeclarativeView *view, QObject *parent) :
    QObject(parent), data(new QDeclarativeViewInspectorPrivate(this))
{
    data->view = view;
    data->manipulatorLayer = new LiveLayerItem(view->scene());
    data->selectionTool = new LiveSelectionTool(this);
    data->zoomTool = new ZoomTool(this);
    data->colorPickerTool = new ColorPickerTool(this);
    data->boundingRectHighlighter = new BoundingRectHighlighter(this);
    data->currentTool = data->selectionTool;

    // to capture ChildRemoved event when viewport changes
    data->view->installEventFilter(this);

    data->setViewport(data->view->viewport());

    data->debugService = QDeclarativeInspectorService::instance();

    connect(data->debugService, SIGNAL(designModeBehaviorChanged(bool)),
            SLOT(setDesignModeBehavior(bool)));
    connect(data->debugService, SIGNAL(showAppOnTopChanged(bool)),
            SLOT(setShowAppOnTop(bool)));
    connect(data->debugService, SIGNAL(reloadRequested()), data.data(), SLOT(_q_reloadView()));
    connect(data->debugService, SIGNAL(currentObjectsChanged(QList<QObject*>)),
            data.data(), SLOT(_q_onCurrentObjectsChanged(QList<QObject*>)));
    connect(data->debugService, SIGNAL(animationSpeedChangeRequested(qreal)),
            SLOT(animationSpeedChangeRequested(qreal)));
    connect(data->debugService, SIGNAL(executionPauseChangeRequested(bool)),
            SLOT(animationPausedChangeRequested(bool)));
    connect(data->debugService, SIGNAL(colorPickerToolRequested()),
            data.data(), SLOT(_q_changeToColorPickerTool()));
    connect(data->debugService, SIGNAL(selectMarqueeToolRequested()),
            data.data(), SLOT(_q_changeToMarqueeSelectTool()));
    connect(data->debugService, SIGNAL(selectToolRequested()), data.data(), SLOT(_q_changeToSingleSelectTool()));
    connect(data->debugService, SIGNAL(zoomToolRequested()), data.data(), SLOT(_q_changeToZoomTool()));
    connect(data->debugService,
            SIGNAL(objectCreationRequested(QString,QObject*,QStringList,QString,int)),
            data.data(), SLOT(_q_createQmlObject(QString,QObject*,QStringList,QString,int)));
    connect(data->debugService,
            SIGNAL(objectDeletionRequested(QObject*)), data.data(), SLOT(_q_deleteQmlObject(QObject*)));
    connect(data->debugService,
            SIGNAL(objectReparentRequested(QObject*,QObject*)),
            data.data(), SLOT(_q_reparentQmlObject(QObject*,QObject*)));
    connect(data->debugService, SIGNAL(clearComponentCacheRequested()),
            data.data(), SLOT(_q_clearComponentCache()));
    connect(data->view, SIGNAL(statusChanged(QDeclarativeView::Status)),
            data.data(), SLOT(_q_onStatusChanged(QDeclarativeView::Status)));

    connect(data->colorPickerTool, SIGNAL(selectedColorChanged(QColor)),
            SIGNAL(selectedColorChanged(QColor)));
    connect(data->colorPickerTool, SIGNAL(selectedColorChanged(QColor)),
            data->debugService, SLOT(selectedColorChanged(QColor)));

    data->_q_changeToSingleSelectTool();
}

QDeclarativeViewInspector::~QDeclarativeViewInspector()
{
}

void QDeclarativeViewInspectorPrivate::_q_reloadView()
{
    clearHighlight();
    emit q->reloadRequested();
}

void QDeclarativeViewInspectorPrivate::setViewport(QWidget *widget)
{
    if (viewport.data() == widget)
        return;

    if (viewport)
        viewport.data()->removeEventFilter(q);

    viewport = widget;
    if (viewport) {
        // make sure we get mouse move events
        viewport.data()->setMouseTracking(true);
        viewport.data()->installEventFilter(q);
    }
}

void QDeclarativeViewInspectorPrivate::clearEditorItems()
{
    clearHighlight();
    setSelectedItems(QList<QGraphicsItem*>());
}

bool QDeclarativeViewInspector::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == data->view) {
        // Event from view
        if (event->type() == QEvent::ChildRemoved) {
            // Might mean that viewport has changed
            if (data->view->viewport() != data->viewport.data())
                data->setViewport(data->view->viewport());
        }
        return QObject::eventFilter(obj, event);
    }

    // Event from viewport
    switch (event->type()) {
    case QEvent::Leave: {
        if (leaveEvent(event))
            return true;
        break;
    }
    case QEvent::MouseButtonPress: {
        if (mousePressEvent(static_cast<QMouseEvent*>(event)))
            return true;
        break;
    }
    case QEvent::MouseMove: {
        if (mouseMoveEvent(static_cast<QMouseEvent*>(event)))
            return true;
        break;
    }
    case QEvent::MouseButtonRelease: {
        if (mouseReleaseEvent(static_cast<QMouseEvent*>(event)))
            return true;
        break;
    }
    case QEvent::KeyPress: {
        if (keyPressEvent(static_cast<QKeyEvent*>(event)))
            return true;
        break;
    }
    case QEvent::KeyRelease: {
        if (keyReleaseEvent(static_cast<QKeyEvent*>(event)))
            return true;
        break;
    }
    case QEvent::MouseButtonDblClick: {
        if (mouseDoubleClickEvent(static_cast<QMouseEvent*>(event)))
            return true;
        break;
    }
    case QEvent::Wheel: {
        if (wheelEvent(static_cast<QWheelEvent*>(event)))
            return true;
        break;
    }
    default: {
        break;
    }
    } //switch

    // standard event processing
    return QObject::eventFilter(obj, event);
}

bool QDeclarativeViewInspector::leaveEvent(QEvent * /*event*/)
{
    if (!data->designModeBehavior)
        return false;
    data->clearHighlight();
    return true;
}

bool QDeclarativeViewInspector::mousePressEvent(QMouseEvent *event)
{
    if (!data->designModeBehavior)
        return false;
    data->cursorPos = event->pos();
    data->currentTool->mousePressEvent(event);
    return true;
}

bool QDeclarativeViewInspector::mouseMoveEvent(QMouseEvent *event)
{
    if (!data->designModeBehavior) {
        data->clearEditorItems();
        return false;
    }
    data->cursorPos = event->pos();

    QList<QGraphicsItem*> selItems = data->selectableItems(event->pos());
    if (!selItems.isEmpty()) {
        declarativeView()->setToolTip(AbstractLiveEditTool::titleForItem(selItems.first()));
    } else {
        declarativeView()->setToolTip(QString());
    }
    if (event->buttons()) {
        data->currentTool->mouseMoveEvent(event);
    } else {
        data->currentTool->hoverMoveEvent(event);
    }
    return true;
}

bool QDeclarativeViewInspector::mouseReleaseEvent(QMouseEvent *event)
{
    if (!data->designModeBehavior)
        return false;

    data->cursorPos = event->pos();
    data->currentTool->mouseReleaseEvent(event);
    return true;
}

bool QDeclarativeViewInspector::keyPressEvent(QKeyEvent *event)
{
    if (!data->designModeBehavior)
        return false;

    data->currentTool->keyPressEvent(event);
    return true;
}

bool QDeclarativeViewInspector::keyReleaseEvent(QKeyEvent *event)
{
    if (!data->designModeBehavior)
        return false;

    switch(event->key()) {
    case Qt::Key_V:
        data->_q_changeToSingleSelectTool();
        break;
// disabled because multiselection does not do anything useful without design mode
//    case Qt::Key_M:
//        data->_q_changeToMarqueeSelectTool();
//        break;
    case Qt::Key_I:
        data->_q_changeToColorPickerTool();
        break;
    case Qt::Key_Z:
        data->_q_changeToZoomTool();
        break;
    case Qt::Key_Space:
        setAnimationPaused(!data->animationPaused);
        break;
    default:
        break;
    }

    data->currentTool->keyReleaseEvent(event);
    return true;
}

bool insertObjectInListProperty(QDeclarativeListReference &fromList, int position, QObject *object)
{
    QList<QObject *> tmpList;
    int i;

    if (!(fromList.canCount() && fromList.canAt() && fromList.canAppend() && fromList.canClear()))
        return false;

    if (position == fromList.count()) {
        fromList.append(object);
        return true;
    }

    for (i=0; i<fromList.count(); ++i)
        tmpList << fromList.at(i);

    fromList.clear();
    for (i=0; i<position; ++i)
        fromList.append(tmpList.at(i));

    fromList.append(object);
    for (; i<tmpList.count(); ++i)
        fromList.append(tmpList.at(i));

    return true;
}

bool removeObjectFromListProperty(QDeclarativeListReference &fromList, QObject *object)
{
    QList<QObject *> tmpList;
    int i;

    if (!(fromList.canCount() && fromList.canAt() && fromList.canAppend() && fromList.canClear()))
        return false;

    for (i=0; i<fromList.count(); ++i)
        if (object != fromList.at(i))
            tmpList << fromList.at(i);

    fromList.clear();

    foreach (QObject *item, tmpList)
        fromList.append(item);

    return true;
}

void QDeclarativeViewInspectorPrivate::_q_createQmlObject(const QString &qml, QObject *parent,
                                                         const QStringList &importList,
                                                         const QString &filename, int order)
{
    if (!parent)
        return;

    QString imports;
    foreach (const QString &s, importList) {
        imports += s;
        imports += QLatin1Char('\n');
    }

    QDeclarativeContext *parentContext = view->engine()->contextForObject(parent);
    QDeclarativeComponent component(view->engine(), q);
    QByteArray constructedQml = QString(imports + qml).toLatin1();

    component.setData(constructedQml, filename);
    QObject *newObject = component.create(parentContext);
    if (newObject) {
        newObject->setParent(parent);
        do {
            // add child item
            QDeclarativeItem *parentItem = qobject_cast<QDeclarativeItem*>(parent);
            QDeclarativeItem *newItem    = qobject_cast<QDeclarativeItem*>(newObject);
            if (parentItem && newItem) {
                newItem->setParentItem(parentItem);
                break;
            }

            // add property change
            QDeclarativeState *parentState = qobject_cast<QDeclarativeState*>(parent);
            QDeclarativeStateOperation *newPropertyChanges = qobject_cast<QDeclarativeStateOperation *>(newObject);
            if (parentState && newPropertyChanges) {
                (*parentState) << newPropertyChanges;
                break;
            }

            // add states
            QDeclarativeState *newState = qobject_cast<QDeclarativeState*>(newObject);
            if (parentItem && newState) {
                QDeclarativeListReference statesList(parentItem, "states");
                statesList.append(newObject);
                break;
            }

            // add animation to transition
            if (parent->inherits("QDeclarativeTransition") &&
                    newObject->inherits("QDeclarativeAbstractAnimation")) {
                QDeclarativeListReference animationsList(parent, "animations");
                animationsList.append(newObject);
                break;
            }

            // add animation to animation
            if (parent->inherits("QDeclarativeAnimationGroup") &&
                    newObject->inherits("QDeclarativeAbstractAnimation")) {
                QDeclarativeListReference animationsList(parent, "animations");
                if (order==-1) {
                    animationsList.append(newObject);
                } else {
                    if (!insertObjectInListProperty(animationsList, order, newObject)) {
                        animationsList.append(newObject);
                    }
                }
                break;
            }

            // add transition
            if (parentItem && newObject->inherits("QDeclarativeTransition")) {
                QDeclarativeListReference transitionsList(parentItem,"transitions");
                if (transitionsList.count() == 1 && transitionsList.at(0) == 0) {
                    transitionsList.clear();
                }
                transitionsList.append(newObject);
                break;
            }

        } while (false);
    }
}

void QDeclarativeViewInspectorPrivate::_q_reparentQmlObject(QObject *object, QObject *newParent)
{
    if (!newParent)
        return;

    object->setParent(newParent);
    QDeclarativeItem *newParentItem = qobject_cast<QDeclarativeItem*>(newParent);
    QDeclarativeItem *item    = qobject_cast<QDeclarativeItem*>(object);
    if (newParentItem && item)
        item->setParentItem(newParentItem);
}

void QDeclarativeViewInspectorPrivate::_q_deleteQmlObject(QObject *object)
{
    // special cases for transitions/animations
    if (object->inherits("QDeclarativeAbstractAnimation")) {
        if (object->parent()) {
            QDeclarativeListReference animationsList(object->parent(), "animations");
            if (removeObjectFromListProperty(animationsList, object))
                object->deleteLater();
            return;
        }
    }

    if (object->inherits("QDeclarativeTransition")) {
        QDeclarativeListReference transitionsList(object->parent(), "transitions");
        if (removeObjectFromListProperty(transitionsList, object))
            object->deleteLater();
        return;
    }
}

void QDeclarativeViewInspectorPrivate::_q_clearComponentCache()
{
    view->engine()->clearComponentCache();
}

void QDeclarativeViewInspectorPrivate::_q_removeFromSelection(QObject *obj)
{
    QList<QGraphicsItem*> items = selectedItems();
    if (QGraphicsItem *item = qobject_cast<QGraphicsObject*>(obj))
        items.removeOne(item);
    setSelectedItems(items);
}

bool QDeclarativeViewInspector::mouseDoubleClickEvent(QMouseEvent * /*event*/)
{
    if (!data->designModeBehavior)
        return false;

    return true;
}

bool QDeclarativeViewInspector::wheelEvent(QWheelEvent *event)
{
    if (!data->designModeBehavior)
        return false;
    data->currentTool->wheelEvent(event);
    return true;
}

void QDeclarativeViewInspector::setDesignModeBehavior(bool value)
{
    emit designModeBehaviorChanged(value);

    data->debugService->setDesignModeBehavior(value);

    data->designModeBehavior = value;

    if (!data->designModeBehavior)
        data->clearEditorItems();
}

bool QDeclarativeViewInspector::designModeBehavior()
{
    return data->designModeBehavior;
}

bool QDeclarativeViewInspector::showAppOnTop() const
{
    return data->showAppOnTop;
}

void QDeclarativeViewInspector::setShowAppOnTop(bool appOnTop)
{
    if (data->view) {
        QWidget *window = data->view->window();
        Qt::WindowFlags flags = window->windowFlags();
        if (appOnTop)
            flags |= Qt::WindowStaysOnTopHint;
        else
            flags &= ~Qt::WindowStaysOnTopHint;

        window->setWindowFlags(flags);
        window->show();
    }

    data->showAppOnTop = appOnTop;
    data->debugService->setShowAppOnTop(appOnTop);

    emit showAppOnTopChanged(appOnTop);
}

void QDeclarativeViewInspectorPrivate::changeTool(Constants::DesignTool tool,
                                                 Constants::ToolFlags /*flags*/)
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

void QDeclarativeViewInspectorPrivate::setSelectedItemsForTools(QList<QGraphicsItem *> items)
{
    foreach (const QWeakPointer<QGraphicsObject> &obj, currentSelection) {
        if (QGraphicsItem *item = obj.data()) {
            if (!items.contains(item)) {
                QObject::disconnect(obj.data(), SIGNAL(destroyed(QObject*)),
                                    this, SLOT(_q_removeFromSelection(QObject*)));
                currentSelection.removeOne(obj);
            }
        }
    }

    foreach (QGraphicsItem *item, items) {
        if (QGraphicsObject *obj = item->toGraphicsObject()) {
            if (!currentSelection.contains(obj)) {
                QObject::connect(obj, SIGNAL(destroyed(QObject*)),
                                 this, SLOT(_q_removeFromSelection(QObject*)));
                currentSelection.append(obj);
            }
        }
    }

    currentTool->updateSelectedItems();
}

void QDeclarativeViewInspectorPrivate::setSelectedItems(QList<QGraphicsItem *> items)
{
    QList<QWeakPointer<QGraphicsObject> > oldList = currentSelection;
    setSelectedItemsForTools(items);
    if (oldList != currentSelection) {
        QList<QObject*> objectList;
        foreach (const QWeakPointer<QGraphicsObject> &graphicsObject, currentSelection) {
            if (graphicsObject)
                objectList << graphicsObject.data();
        }

        debugService->setCurrentObjects(objectList);
    }
}

QList<QGraphicsItem *> QDeclarativeViewInspectorPrivate::selectedItems()
{
    QList<QGraphicsItem *> selection;
    foreach (const QWeakPointer<QGraphicsObject> &selectedObject, currentSelection) {
        if (selectedObject.data())
            selection << selectedObject.data();
    }

    return selection;
}

void QDeclarativeViewInspector::setSelectedItems(QList<QGraphicsItem *> items)
{
    data->setSelectedItems(items);
}

QList<QGraphicsItem *> QDeclarativeViewInspector::selectedItems()
{
    return data->selectedItems();
}

QDeclarativeView *QDeclarativeViewInspector::declarativeView()
{
    return data->view;
}

void QDeclarativeViewInspectorPrivate::clearHighlight()
{
    boundingRectHighlighter->clear();
}

void QDeclarativeViewInspectorPrivate::highlight(const QList<QGraphicsObject *> &items)
{
    if (items.isEmpty())
        return;

    QList<QGraphicsObject*> objectList;
    foreach (QGraphicsItem *item, items) {
        QGraphicsItem *child = item;

        if (child) {
            QGraphicsObject *childObject = child->toGraphicsObject();
            if (childObject)
                objectList << childObject;
        }
    }

    boundingRectHighlighter->highlight(objectList);
}

QList<QGraphicsItem*> QDeclarativeViewInspectorPrivate::selectableItems(
    const QPointF &scenePos) const
{
    QList<QGraphicsItem*> itemlist = view->scene()->items(scenePos);
    return filterForSelection(itemlist);
}

QList<QGraphicsItem*> QDeclarativeViewInspectorPrivate::selectableItems(const QPoint &pos) const
{
    QList<QGraphicsItem*> itemlist = view->items(pos);
    return filterForSelection(itemlist);
}

QList<QGraphicsItem*> QDeclarativeViewInspectorPrivate::selectableItems(
    const QRectF &sceneRect, Qt::ItemSelectionMode selectionMode) const
{
    QList<QGraphicsItem*> itemlist = view->scene()->items(sceneRect, selectionMode);
    return filterForSelection(itemlist);
}

void QDeclarativeViewInspectorPrivate::_q_changeToSingleSelectTool()
{
    currentToolMode = Constants::SelectionToolMode;
    selectionTool->setRubberbandSelectionMode(false);

    changeToSelectTool();

    emit q->selectToolActivated();
    debugService->setCurrentTool(Constants::SelectionToolMode);
}

void QDeclarativeViewInspectorPrivate::changeToSelectTool()
{
    if (currentTool == selectionTool)
        return;

    currentTool->clear();
    currentTool = selectionTool;
    currentTool->clear();
    currentTool->updateSelectedItems();
}

void QDeclarativeViewInspectorPrivate::_q_changeToMarqueeSelectTool()
{
    changeToSelectTool();
    currentToolMode = Constants::MarqueeSelectionToolMode;
    selectionTool->setRubberbandSelectionMode(true);

    emit q->marqueeSelectToolActivated();
    debugService->setCurrentTool(Constants::MarqueeSelectionToolMode);
}

void QDeclarativeViewInspectorPrivate::_q_changeToZoomTool()
{
    currentToolMode = Constants::ZoomMode;
    currentTool->clear();
    currentTool = zoomTool;
    currentTool->clear();

    emit q->zoomToolActivated();
    debugService->setCurrentTool(Constants::ZoomMode);
}

void QDeclarativeViewInspectorPrivate::_q_changeToColorPickerTool()
{
    if (currentTool == colorPickerTool)
        return;

    currentToolMode = Constants::ColorPickerMode;
    currentTool->clear();
    currentTool = colorPickerTool;
    currentTool->clear();

    emit q->colorPickerActivated();
    debugService->setCurrentTool(Constants::ColorPickerMode);
}

void QDeclarativeViewInspector::setAnimationSpeed(qreal slowDownFactor)
{
    Q_ASSERT(slowDownFactor > 0);
    if (data->slowDownFactor == slowDownFactor)
        return;

    animationSpeedChangeRequested(slowDownFactor);
    data->debugService->setAnimationSpeed(slowDownFactor);
}

void QDeclarativeViewInspector::setAnimationPaused(bool paused)
{
    if (data->animationPaused == paused)
        return;

    animationPausedChangeRequested(paused);
    data->debugService->setAnimationPaused(paused);
}

void QDeclarativeViewInspector::animationSpeedChangeRequested(qreal factor)
{
    if (data->slowDownFactor != factor) {
        data->slowDownFactor = factor;
        emit animationSpeedChanged(factor);
    }

    const float effectiveFactor = data->animationPaused ? 0 : factor;
    QDeclarativeDebugHelper::setAnimationSlowDownFactor(effectiveFactor);
}

void QDeclarativeViewInspector::animationPausedChangeRequested(bool paused)
{
    if (data->animationPaused != paused) {
        data->animationPaused = paused;
        emit animationPausedChanged(paused);
    }

    const float effectiveFactor = paused ? 0 : data->slowDownFactor;
    QDeclarativeDebugHelper::setAnimationSlowDownFactor(effectiveFactor);
}


void QDeclarativeViewInspectorPrivate::_q_applyChangesFromClient()
{
}


QList<QGraphicsItem*> QDeclarativeViewInspectorPrivate::filterForSelection(
    QList<QGraphicsItem*> &itemlist) const
{
    foreach (QGraphicsItem *item, itemlist) {
        if (isEditorItem(item))
            itemlist.removeOne(item);
    }

    return itemlist;
}

bool QDeclarativeViewInspectorPrivate::isEditorItem(QGraphicsItem *item) const
{
    return (item->type() == Constants::EditorItemType
            || item->type() == Constants::ResizeHandleItemType
            || item->data(Constants::EditorItemDataKey).toBool());
}

void QDeclarativeViewInspectorPrivate::_q_onStatusChanged(QDeclarativeView::Status status)
{
    if (status == QDeclarativeView::Ready)
        debugService->reloaded();
}

void QDeclarativeViewInspectorPrivate::_q_onCurrentObjectsChanged(QList<QObject*> objects)
{
    QList<QGraphicsItem*> items;
    QList<QGraphicsObject*> gfxObjects;
    foreach (QObject *obj, objects) {
        if (QDeclarativeItem *declarativeItem = qobject_cast<QDeclarativeItem*>(obj)) {
            items << declarativeItem;
            gfxObjects << declarativeItem;
        }
    }
    if (designModeBehavior) {
        setSelectedItemsForTools(items);
        clearHighlight();
        highlight(gfxObjects);
    }
}

QString QDeclarativeViewInspector::idStringForObject(QObject *obj)
{
    return QDeclarativeInspectorService::instance()->idStringForObject(obj);
}

// adjusts bounding boxes on edges of screen to be visible
QRectF QDeclarativeViewInspector::adjustToScreenBoundaries(const QRectF &boundingRectInSceneSpace)
{
    int marginFromEdge = 1;
    QRectF boundingRect(boundingRectInSceneSpace);
    if (qAbs(boundingRect.left()) - 1 < 2)
        boundingRect.setLeft(marginFromEdge);

    QRect rect = data->view->rect();

    if (boundingRect.right() >= rect.right())
        boundingRect.setRight(rect.right() - marginFromEdge);

    if (qAbs(boundingRect.top()) - 1 < 2)
        boundingRect.setTop(marginFromEdge);

    if (boundingRect.bottom() >= rect.bottom())
        boundingRect.setBottom(rect.bottom() - marginFromEdge);

    return boundingRect;
}

} // namespace QmlJSDebugger
