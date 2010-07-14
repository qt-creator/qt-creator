#include "qdeclarativedesignview.h"
#include "qdeclarativedesigndebugserver.h"
#include "selectiontool.h"
#include "zoomtool.h"
#include "colorpickertool.h"
#include "layeritem.h"
#include "boundingrecthighlighter.h"
#include "subcomponenteditortool.h"
#include "qmltoolbar.h"

#include <QMouseEvent>
#include <QDeclarativeItem>
#include <QWidget>
#include <QGraphicsObject>
#include <QApplication>

#include <QAbstractAnimation>
#include <private/qabstractanimation_p.h>

namespace QmlViewer {

Q_GLOBAL_STATIC(QDeclarativeDesignDebugServer, qmlDesignDebugServer)

QDeclarativeDesignView::QDeclarativeDesignView(QWidget *parent) :
    QDeclarativeView(parent),
    m_designModeBehavior(false),
    m_executionPaused(false),
    m_slowdownFactor(1.0f),
    m_toolbar(0)
{
    m_manipulatorLayer = new LayerItem(scene());
    m_selectionTool = new SelectionTool(this);
    m_zoomTool = new ZoomTool(this);
    m_colorPickerTool = new ColorPickerTool(this);
    m_boundingRectHighlighter = new BoundingRectHighlighter(this);
    m_subcomponentEditorTool = new SubcomponentEditorTool(this);
    m_currentTool = m_selectionTool;

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

    connect(this, SIGNAL(statusChanged(QDeclarativeView::Status)), SLOT(onStatusChanged(QDeclarativeView::Status)));

    connect(m_colorPickerTool, SIGNAL(selectedColorChanged(QColor)), SIGNAL(selectedColorChanged(QColor)));

    createToolbar();
}

QDeclarativeDesignView::~QDeclarativeDesignView()
{
}

void QDeclarativeDesignView::reloadView()
{
    m_subcomponentEditorTool->clear();
    clearHighlight();
    emit reloadRequested();
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
    m_cursorPos = event->pos();
    m_currentTool->mousePressEvent(event);
}

void QDeclarativeDesignView::mouseMoveEvent(QMouseEvent *event)
{
    if (!designModeBehavior()) {
        QDeclarativeView::mouseMoveEvent(event);
        return;
    }
    m_cursorPos = event->pos();

    QList<QGraphicsItem*> selItems = selectableItems(event->pos());
    if (!selItems.isEmpty()) {
        setToolTip(AbstractFormEditorTool::titleForItem(selItems.first()));
    } else {
        setToolTip(QString());
    }
    if (event->buttons()) {
        m_subcomponentEditorTool->mouseMoveEvent(event);
        m_currentTool->mouseMoveEvent(event);
    } else {
        m_subcomponentEditorTool->hoverMoveEvent(event);
        m_currentTool->hoverMoveEvent(event);
    }
}

void QDeclarativeDesignView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!designModeBehavior()) {
        QDeclarativeView::mouseReleaseEvent(event);
        return;
    }
    m_subcomponentEditorTool->mouseReleaseEvent(event);

    m_cursorPos = event->pos();
    m_currentTool->mouseReleaseEvent(event);

    qmlDesignDebugServer()->setCurrentObjects(AbstractFormEditorTool::toObjectList(selectedItems()));
}

void QDeclarativeDesignView::keyPressEvent(QKeyEvent *event)
{
    if (!designModeBehavior()) {
        QDeclarativeView::keyPressEvent(event);
        return;
    }
    m_currentTool->keyPressEvent(event);
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
            m_subcomponentEditorTool->setCurrentItem(selectedItems().first());
        break;
    case Qt::Key_Space:
        if (m_executionPaused) {
            continueExecution(m_slowdownFactor);
        } else {
            pauseExecution();
        }
        break;
    default:
        break;
    }

    m_currentTool->keyReleaseEvent(event);
}

QGraphicsItem *QDeclarativeDesignView::currentRootItem() const
{
    return m_subcomponentEditorTool->currentRootItem();
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
        itemToEnter = m_subcomponentEditorTool->firstChildOfContext(itemToEnter);

    m_subcomponentEditorTool->setCurrentItem(itemToEnter);
    m_subcomponentEditorTool->mouseDoubleClickEvent(event);

    if ((event->buttons() & Qt::LeftButton) && itemToEnter) {
        QGraphicsObject *objectToEnter = itemToEnter->toGraphicsObject();
        if (objectToEnter)
            qmlDesignDebugServer()->setCurrentObjects(QList<QObject*>() << objectToEnter);
    }

}
void QDeclarativeDesignView::wheelEvent(QWheelEvent *event)
{
    if (!m_designModeBehavior) {
        QDeclarativeView::wheelEvent(event);
        return;
    }
    m_currentTool->wheelEvent(event);
}

void QDeclarativeDesignView::setDesignModeBehavior(bool value)
{
    emit designModeBehaviorChanged(value);

    m_toolbar->setDesignModeBehavior(value);
    qmlDesignDebugServer()->setDesignModeBehavior(value);

    m_designModeBehavior = value;
    if (m_subcomponentEditorTool) {
        m_subcomponentEditorTool->clear();
        clearHighlight();
        setSelectedItems(QList<QGraphicsItem*>());

        if (rootObject())
            m_subcomponentEditorTool->pushContext(rootObject());
    }
}

bool QDeclarativeDesignView::designModeBehavior() const
{
    return m_designModeBehavior;
}

void QDeclarativeDesignView::changeTool(Constants::DesignTool tool, Constants::ToolFlags /*flags*/)
{
    switch(tool) {
    case Constants::SelectionToolMode:
        changeToSingleSelectTool();
        break;
    case Constants::NoTool:
    default:
        m_currentTool = 0;
        break;
    }
}

void QDeclarativeDesignView::setSelectedItems(QList<QGraphicsItem *> items)
{
    m_currentSelection = items;
    m_currentTool->setItems(items);
}

QList<QGraphicsItem *> QDeclarativeDesignView::selectedItems() const
{
    return m_currentSelection;
}

AbstractFormEditorTool *QDeclarativeDesignView::currentTool() const
{
    return m_currentTool;
}

void QDeclarativeDesignView::clearHighlight()
{
    m_boundingRectHighlighter->clear();
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
            child = m_subcomponentEditorTool->firstChildOfContext(item);

        if (child) {
            QGraphicsObject *childObject = child->toGraphicsObject();
            if (childObject)
                objectList << childObject;
        }
    }

    m_boundingRectHighlighter->highlight(objectList);
}

bool QDeclarativeDesignView::mouseInsideContextItem() const
{
    return m_subcomponentEditorTool->containsCursor(m_cursorPos.toPoint());
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
    //qDebug() << "changing to selection tool";

    m_currentToolMode = Constants::SelectionToolMode;
    m_selectionTool->setRubberbandSelectionMode(false);

    changeToSelectTool();

    emit selectToolActivated();
    qmlDesignDebugServer()->setCurrentTool(Constants::SelectionToolMode);
}

void QDeclarativeDesignView::changeToSelectTool()
{
    if (m_currentTool == m_selectionTool)
        return;

    m_currentTool->clear();
    m_currentTool = m_selectionTool;
    m_currentTool->clear();
    m_currentTool->setItems(m_currentSelection);
}

void QDeclarativeDesignView::changeToMarqueeSelectTool()
{
    changeToSelectTool();
    m_currentToolMode = Constants::MarqueeSelectionToolMode;
    m_selectionTool->setRubberbandSelectionMode(true);

    emit marqueeSelectToolActivated();
    qmlDesignDebugServer()->setCurrentTool(Constants::MarqueeSelectionToolMode);
}

void QDeclarativeDesignView::changeToZoomTool()
{
    m_currentToolMode = Constants::ZoomMode;
    m_currentTool->clear();
    m_currentTool = m_zoomTool;
    m_currentTool->clear();

    emit zoomToolActivated();
    qmlDesignDebugServer()->setCurrentTool(Constants::ZoomMode);
}

void QDeclarativeDesignView::changeToColorPickerTool()
{
    if (m_currentTool == m_colorPickerTool)
        return;

    m_currentToolMode = Constants::ColorPickerMode;
    m_currentTool->clear();
    m_currentTool = m_colorPickerTool;
    m_currentTool->clear();

    emit colorPickerActivated();
    qmlDesignDebugServer()->setCurrentTool(Constants::ColorPickerMode);
}

void QDeclarativeDesignView::changeAnimationSpeed(qreal slowdownFactor)
{
    m_slowdownFactor = slowdownFactor;

    if (m_slowdownFactor != 0) {
        continueExecution(m_slowdownFactor);
    } else {
        pauseExecution();
    }
}

void QDeclarativeDesignView::continueExecution(qreal slowdownFactor)
{
    Q_ASSERT(slowdownFactor > 0);

    m_slowdownFactor = slowdownFactor;
    static const qreal animSpeedSnapDelta = 0.01f;
    bool useStandardSpeed = (qAbs(1.0f - m_slowdownFactor) < animSpeedSnapDelta);

    QUnifiedTimer *timer = QUnifiedTimer::instance();
    timer->setSlowdownFactor(m_slowdownFactor);
    timer->setSlowModeEnabled(!useStandardSpeed);
    m_executionPaused = false;

    emit executionStarted(m_slowdownFactor);
    qmlDesignDebugServer()->setAnimationSpeed(m_slowdownFactor);
}

void QDeclarativeDesignView::pauseExecution()
{
    QUnifiedTimer *timer = QUnifiedTimer::instance();
    timer->setSlowdownFactor(0);
    timer->setSlowModeEnabled(true);
    m_executionPaused = true;

    emit executionPaused();
    qmlDesignDebugServer()->setAnimationSpeed(0);
}

void QDeclarativeDesignView::applyChangesFromClient()
{

}

LayerItem *QDeclarativeDesignView::manipulatorLayer() const
{
    return m_manipulatorLayer;
}

QList<QGraphicsItem*> QDeclarativeDesignView::filterForSelection(QList<QGraphicsItem*> &itemlist) const
{
    foreach(QGraphicsItem *item, itemlist) {
        if (isEditorItem(item) || !m_subcomponentEditorTool->isChildOfContext(item))
            itemlist.removeOne(item);
    }

    return itemlist;
}

QList<QGraphicsItem*> QDeclarativeDesignView::filterForCurrentContext(QList<QGraphicsItem*> &itemlist) const
{
    foreach(QGraphicsItem *item, itemlist) {

        if (isEditorItem(item) || !m_subcomponentEditorTool->isDirectChildOfContext(item)) {

            // if we're a child, but not directly, replace with the parent that is directly in context.
            if (QGraphicsItem *contextParent = m_subcomponentEditorTool->firstChildOfContext(item)) {
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
            m_subcomponentEditorTool->pushContext(rootObject());
            emit executionStarted(1.0f);
        }
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

QToolBar *QDeclarativeDesignView::toolbar() const
{
    return m_toolbar;
}

void QDeclarativeDesignView::createToolbar()
{
    m_toolbar = new QmlToolbar(this);
    connect(this, SIGNAL(selectedColorChanged(QColor)), m_toolbar, SLOT(setColorBoxColor(QColor)));

    connect(this, SIGNAL(designModeBehaviorChanged(bool)), m_toolbar, SLOT(setDesignModeBehavior(bool)));

    connect(m_toolbar, SIGNAL(designModeBehaviorChanged(bool)), this, SLOT(setDesignModeBehavior(bool)));
    connect(m_toolbar, SIGNAL(executionStarted()), this, SLOT(continueExecution()));
    connect(m_toolbar, SIGNAL(executionPaused()), this, SLOT(pauseExecution()));
    connect(m_toolbar, SIGNAL(colorPickerSelected()), this, SLOT(changeToColorPickerTool()));
    connect(m_toolbar, SIGNAL(zoomToolSelected()), this, SLOT(changeToZoomTool()));
    connect(m_toolbar, SIGNAL(selectToolSelected()), this, SLOT(changeToSingleSelectTool()));
    connect(m_toolbar, SIGNAL(marqueeSelectToolSelected()), this, SLOT(changeToMarqueeSelectTool()));

    connect(m_toolbar, SIGNAL(applyChangesFromQmlFileSelected()), SLOT(applyChangesFromClient()));

    connect(this, SIGNAL(executionStarted(qreal)), m_toolbar, SLOT(startExecution()));
    connect(this, SIGNAL(executionPaused()), m_toolbar, SLOT(pauseExecution()));

    connect(this, SIGNAL(selectToolActivated()), m_toolbar, SLOT(activateSelectTool()));

    // disabled features
    //connect(m_toolbar, SIGNAL(applyChangesToQmlFileSelected()), SLOT(applyChangesToClient()));
    //connect(this, SIGNAL(resizeToolActivated()), m_toolbar, SLOT(activateSelectTool()));
    //connect(this, SIGNAL(moveToolActivated()),   m_toolbar, SLOT(activateSelectTool()));

    connect(this, SIGNAL(colorPickerActivated()), m_toolbar, SLOT(activateColorPicker()));
    connect(this, SIGNAL(zoomToolActivated()), m_toolbar, SLOT(activateZoom()));
    connect(this, SIGNAL(marqueeSelectToolActivated()), m_toolbar, SLOT(activateMarqueeSelectTool()));
}

} //namespace QmlViewer
