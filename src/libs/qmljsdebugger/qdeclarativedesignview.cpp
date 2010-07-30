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

Q_GLOBAL_STATIC(QDeclarativeDesignDebugServer, qmlDesignDebugServer)

QDeclarativeDesignViewPrivate::QDeclarativeDesignViewPrivate() :
    designModeBehavior(false),
    executionPaused(false),
    slowdownFactor(1.0f),
    toolbar(0)
{

}

QDeclarativeDesignViewPrivate::~QDeclarativeDesignViewPrivate()
{
}

QDeclarativeDesignView::QDeclarativeDesignView(QWidget *parent) :
    QDeclarativeView(parent)
{
    data = new QDeclarativeDesignViewPrivate;

    data->manipulatorLayer = new LayerItem(scene());
    data->selectionTool = new SelectionTool(this);
    data->zoomTool = new ZoomTool(this);
    data->colorPickerTool = new ColorPickerTool(this);
    data->boundingRectHighlighter = new BoundingRectHighlighter(this);
    data->subcomponentEditorTool = new SubcomponentEditorTool(this);
    data->currentTool = data->selectionTool;

    setMouseTracking(true);

    connect(qmlDesignDebugServer(), SIGNAL(designModeBehaviorChanged(bool)), SLOT(setDesignModeBehavior(bool)));
    connect(qmlDesignDebugServer(), SIGNAL(reloadRequested()), SLOT(reloadView()));
    connect(qmlDesignDebugServer(),
            SIGNAL(currentObjectsChanged(QList<QObject*>)),
            SLOT(onCurrentObjectsChanged(QList<QObject*>)));
    connect(qmlDesignDebugServer(), SIGNAL(animationSpeedChangeRequested(qreal)), SLOT(changeAnimationSpeed(qreal)));
    connect(qmlDesignDebugServer(), SIGNAL(colorPickerToolRequested()), SLOT(changeToColorPickerTool()));
    connect(qmlDesignDebugServer(), SIGNAL(selectMarqueeToolRequested()), SLOT(changeToMarqueeSelectTool()));
    connect(qmlDesignDebugServer(), SIGNAL(selectToolRequested()), SLOT(changeToSingleSelectTool()));
    connect(qmlDesignDebugServer(), SIGNAL(zoomToolRequested()), SLOT(changeToZoomTool()));
    connect(qmlDesignDebugServer(),
            SIGNAL(objectCreationRequested(QString,QObject*,QStringList,QString)),
            SLOT(createQmlObject(QString,QObject*,QStringList,QString)));

    connect(this, SIGNAL(statusChanged(QDeclarativeView::Status)), SLOT(onStatusChanged(QDeclarativeView::Status)));

    connect(data->colorPickerTool, SIGNAL(selectedColorChanged(QColor)), SIGNAL(selectedColorChanged(QColor)));
    connect(data->colorPickerTool, SIGNAL(selectedColorChanged(QColor)),
            qmlDesignDebugServer(), SLOT(selectedColorChanged(QColor)));

    connect(data->subcomponentEditorTool, SIGNAL(cleared()), SIGNAL(inspectorContextCleared()));
    connect(data->subcomponentEditorTool, SIGNAL(contextPushed(QString)), SIGNAL(inspectorContextPushed(QString)));
    connect(data->subcomponentEditorTool, SIGNAL(contextPopped()), SIGNAL(inspectorContextPopped()));

    createToolbar();
}

QDeclarativeDesignView::~QDeclarativeDesignView()
{
}

void QDeclarativeDesignView::setInspectorContext(int contextIndex)
{
    data->subcomponentEditorTool->setContext(contextIndex);
}

void QDeclarativeDesignView::reloadView()
{
    data->subcomponentEditorTool->clear();
    clearHighlight();
    emit reloadRequested();
}

void QDeclarativeDesignView::clearEditorItems()
{
    clearHighlight();
    setSelectedItems(QList<QGraphicsItem*>());
}

void QDeclarativeDesignView::leaveEvent(QEvent *event)
{
    if (!designModeBehavior()) {
        QDeclarativeView::leaveEvent(event);
        return;
    }
    clearHighlight();
}

void QDeclarativeDesignView::mousePressEvent(QMouseEvent *event)
{
    if (!designModeBehavior()) {
        QDeclarativeView::mousePressEvent(event);
        return;
    }
    data->cursorPos = event->pos();
    data->currentTool->mousePressEvent(event);
}

void QDeclarativeDesignView::mouseMoveEvent(QMouseEvent *event)
{
    if (!designModeBehavior()) {
        clearEditorItems();
        QDeclarativeView::mouseMoveEvent(event);
        return;
    }
    data->cursorPos = event->pos();

    QList<QGraphicsItem*> selItems = selectableItems(event->pos());
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
    if (!designModeBehavior()) {
        QDeclarativeView::mouseReleaseEvent(event);
        return;
    }
    data->subcomponentEditorTool->mouseReleaseEvent(event);

    data->cursorPos = event->pos();
    data->currentTool->mouseReleaseEvent(event);

    //qmlDesignDebugServer()->setCurrentObjects(AbstractFormEditorTool::toObjectList(selectedItems()));
}

void QDeclarativeDesignView::keyPressEvent(QKeyEvent *event)
{
    if (!designModeBehavior()) {
        QDeclarativeView::keyPressEvent(event);
        return;
    }
    data->currentTool->keyPressEvent(event);
}

void QDeclarativeDesignView::keyReleaseEvent(QKeyEvent *event)
{
    if (!designModeBehavior()) {
        QDeclarativeView::keyReleaseEvent(event);
        return;
    }

    switch(event->key()) {
    case Qt::Key_V:
        changeToSingleSelectTool();
        break;
    case Qt::Key_M:
        changeToMarqueeSelectTool();
        break;
    case Qt::Key_I:
        changeToColorPickerTool();
        break;
    case Qt::Key_Z:
        changeToZoomTool();
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (!selectedItems().isEmpty())
            data->subcomponentEditorTool->setCurrentItem(selectedItems().first());
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

void QDeclarativeDesignView::createQmlObject(const QString &qml, QObject *parent, const QStringList &importList, const QString &filename)
{
    if (!parent)
        return;

    QString imports;
    foreach(const QString &s, importList) {
        imports += s + "\n";
    }

    QDeclarativeContext *parentContext = engine()->contextForObject(parent);
    QDeclarativeComponent component(engine(), this);
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

QGraphicsItem *QDeclarativeDesignView::currentRootItem() const
{
    return data->subcomponentEditorTool->currentRootItem();
}

void QDeclarativeDesignView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!designModeBehavior()) {
        QDeclarativeView::mouseDoubleClickEvent(event);
        return;
    }
    QGraphicsItem *itemToEnter = 0;
    QList<QGraphicsItem*> itemList = items(event->pos());
    filterForSelection(itemList);

    if (selectedItems().isEmpty() && !itemList.isEmpty()) {
        itemToEnter = itemList.first();
    } else if (!selectedItems().isEmpty() && !itemList.isEmpty()) {
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
        clearHighlight();
        setSelectedItems(QList<QGraphicsItem*>());

        if (rootObject())
            data->subcomponentEditorTool->pushContext(rootObject());
    }

    if (!data->designModeBehavior)
        clearEditorItems();
}

bool QDeclarativeDesignView::designModeBehavior() const
{
    return data->designModeBehavior;
}

void QDeclarativeDesignView::changeTool(Constants::DesignTool tool, Constants::ToolFlags /*flags*/)
{
    switch(tool) {
    case Constants::SelectionToolMode:
        changeToSingleSelectTool();
        break;
    case Constants::NoTool:
    default:
        data->currentTool = 0;
        break;
    }
}

void QDeclarativeDesignView::setSelectedItems(QList<QGraphicsItem *> items)
{
    data->currentSelection.clear();
    foreach(QGraphicsItem *item, items) {
        if (item) {
            QGraphicsObject *obj = item->toGraphicsObject();
            if (obj)
                data->currentSelection << obj;
        }
    }
    data->currentTool->updateSelectedItems();
}

QList<QGraphicsItem *> QDeclarativeDesignView::selectedItems()
{
    QList<QGraphicsItem *> selection;
    foreach(const QWeakPointer<QGraphicsObject> &selectedObject, data->currentSelection) {
        if (selectedObject.isNull()) {
            data->currentSelection.removeOne(selectedObject);
        } else {
            selection << selectedObject.data();
        }
    }

    return selection;
}

void QDeclarativeDesignView::clearHighlight()
{
    data->boundingRectHighlighter->clear();
}

void QDeclarativeDesignView::highlight(QGraphicsItem * item, ContextFlags flags)
{
    highlight(QList<QGraphicsItem*>() << item, flags);
}

void QDeclarativeDesignView::highlight(QList<QGraphicsItem *> items, ContextFlags flags)
{
    if (items.isEmpty())
        return;

    QList<QGraphicsObject*> objectList;
    foreach(QGraphicsItem *item, items) {
        QGraphicsItem *child = item;
        if (flags & ContextSensitive)
            child = data->subcomponentEditorTool->firstChildOfContext(item);

        if (child) {
            QGraphicsObject *childObject = child->toGraphicsObject();
            if (childObject)
                objectList << childObject;
        }
    }

    data->boundingRectHighlighter->highlight(objectList);
}

bool QDeclarativeDesignView::mouseInsideContextItem() const
{
    return data->subcomponentEditorTool->containsCursor(data->cursorPos.toPoint());
}

QList<QGraphicsItem*> QDeclarativeDesignView::selectableItems(const QPointF &scenePos) const
{
    QList<QGraphicsItem*> itemlist = scene()->items(scenePos);
    return filterForCurrentContext(itemlist);
}

QList<QGraphicsItem*> QDeclarativeDesignView::selectableItems(const QPoint &pos) const
{
    QList<QGraphicsItem*> itemlist = items(pos);
    return filterForCurrentContext(itemlist);
}

QList<QGraphicsItem*> QDeclarativeDesignView::selectableItems(const QRectF &sceneRect, Qt::ItemSelectionMode selectionMode) const
{
    QList<QGraphicsItem*> itemlist = scene()->items(sceneRect, selectionMode);

    return filterForCurrentContext(itemlist);
}

void QDeclarativeDesignView::changeToSingleSelectTool()
{
    data->currentToolMode = Constants::SelectionToolMode;
    data->selectionTool->setRubberbandSelectionMode(false);

    changeToSelectTool();

    emit selectToolActivated();
    qmlDesignDebugServer()->setCurrentTool(Constants::SelectionToolMode);
}

void QDeclarativeDesignView::changeToSelectTool()
{
    if (data->currentTool == data->selectionTool)
        return;

    data->currentTool->clear();
    data->currentTool = data->selectionTool;
    data->currentTool->clear();
    data->currentTool->updateSelectedItems();
}

void QDeclarativeDesignView::changeToMarqueeSelectTool()
{
    changeToSelectTool();
    data->currentToolMode = Constants::MarqueeSelectionToolMode;
    data->selectionTool->setRubberbandSelectionMode(true);

    emit marqueeSelectToolActivated();
    qmlDesignDebugServer()->setCurrentTool(Constants::MarqueeSelectionToolMode);
}

void QDeclarativeDesignView::changeToZoomTool()
{
    data->currentToolMode = Constants::ZoomMode;
    data->currentTool->clear();
    data->currentTool = data->zoomTool;
    data->currentTool->clear();

    emit zoomToolActivated();
    qmlDesignDebugServer()->setCurrentTool(Constants::ZoomMode);
}

void QDeclarativeDesignView::changeToColorPickerTool()
{
    if (data->currentTool == data->colorPickerTool)
        return;

    data->currentToolMode = Constants::ColorPickerMode;
    data->currentTool->clear();
    data->currentTool = data->colorPickerTool;
    data->currentTool->clear();

    emit colorPickerActivated();
    qmlDesignDebugServer()->setCurrentTool(Constants::ColorPickerMode);
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

void QDeclarativeDesignView::applyChangesFromClient()
{

}

QGraphicsObject *QDeclarativeDesignView::manipulatorLayer() const
{
    return data->manipulatorLayer;
}

QList<QGraphicsItem*> QDeclarativeDesignView::filterForSelection(QList<QGraphicsItem*> &itemlist) const
{
    foreach(QGraphicsItem *item, itemlist) {
        if (isEditorItem(item) || !data->subcomponentEditorTool->isChildOfContext(item))
            itemlist.removeOne(item);
    }

    return itemlist;
}

QList<QGraphicsItem*> QDeclarativeDesignView::filterForCurrentContext(QList<QGraphicsItem*> &itemlist) const
{
    foreach(QGraphicsItem *item, itemlist) {

        if (isEditorItem(item) || !data->subcomponentEditorTool->isDirectChildOfContext(item)) {

            // if we're a child, but not directly, replace with the parent that is directly in context.
            if (QGraphicsItem *contextParent = data->subcomponentEditorTool->firstChildOfContext(item)) {
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

bool QDeclarativeDesignView::isEditorItem(QGraphicsItem *item) const
{
    return (item->type() == Constants::EditorItemType
         || item->type() == Constants::ResizeHandleItemType
         || item->data(Constants::EditorItemDataKey).toBool());
}

void QDeclarativeDesignView::onStatusChanged(QDeclarativeView::Status status)
{
    if (status == QDeclarativeView::Ready) {
        if (rootObject()) {
            data->subcomponentEditorTool->pushContext(rootObject());
            emit executionStarted(1.0f);
        }
        qmlDesignDebugServer()->reloaded();
    }
}

void QDeclarativeDesignView::onCurrentObjectsChanged(QList<QObject*> objects)
{
    QList<QGraphicsItem*> items;
    foreach(QObject *obj, objects) {
        QDeclarativeItem* declarativeItem = qobject_cast<QDeclarativeItem*>(obj);
        if (declarativeItem)
            items << declarativeItem;
    }

    setSelectedItems(items);
    clearHighlight();
    highlight(items, IgnoreContext);
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

void QDeclarativeDesignView::createToolbar()
{
    data->toolbar = new QmlToolbar(this);
    connect(this, SIGNAL(selectedColorChanged(QColor)), data->toolbar, SLOT(setColorBoxColor(QColor)));

    connect(this, SIGNAL(designModeBehaviorChanged(bool)), data->toolbar, SLOT(setDesignModeBehavior(bool)));

    connect(data->toolbar, SIGNAL(designModeBehaviorChanged(bool)), this, SLOT(setDesignModeBehavior(bool)));
    connect(data->toolbar, SIGNAL(executionStarted()), this, SLOT(continueExecution()));
    connect(data->toolbar, SIGNAL(executionPaused()), this, SLOT(pauseExecution()));
    connect(data->toolbar, SIGNAL(colorPickerSelected()), this, SLOT(changeToColorPickerTool()));
    connect(data->toolbar, SIGNAL(zoomToolSelected()), this, SLOT(changeToZoomTool()));
    connect(data->toolbar, SIGNAL(selectToolSelected()), this, SLOT(changeToSingleSelectTool()));
    connect(data->toolbar, SIGNAL(marqueeSelectToolSelected()), this, SLOT(changeToMarqueeSelectTool()));

    connect(data->toolbar, SIGNAL(applyChangesFromQmlFileSelected()), SLOT(applyChangesFromClient()));

    connect(this, SIGNAL(executionStarted(qreal)), data->toolbar, SLOT(startExecution()));
    connect(this, SIGNAL(executionPaused()), data->toolbar, SLOT(pauseExecution()));

    connect(this, SIGNAL(selectToolActivated()), data->toolbar, SLOT(activateSelectTool()));

    // disabled features
    //connect(d->m_toolbar, SIGNAL(applyChangesToQmlFileSelected()), SLOT(applyChangesToClient()));
    //connect(this, SIGNAL(resizeToolActivated()), d->m_toolbar, SLOT(activateSelectTool()));
    //connect(this, SIGNAL(moveToolActivated()),   d->m_toolbar, SLOT(activateSelectTool()));

    connect(this, SIGNAL(colorPickerActivated()), data->toolbar, SLOT(activateColorPicker()));
    connect(this, SIGNAL(zoomToolActivated()), data->toolbar, SLOT(activateZoom()));
    connect(this, SIGNAL(marqueeSelectToolActivated()), data->toolbar, SLOT(activateMarqueeSelectTool()));
}

} //namespace QmlViewer
