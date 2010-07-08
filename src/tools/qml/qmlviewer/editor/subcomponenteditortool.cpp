#include "subcomponenteditortool.h"
#include "qdeclarativedesignview.h"
#include "subcomponentmasklayeritem.h"
#include "layeritem.h"

#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QTimer>
#include <QMouseEvent>
#include <QKeyEvent>

#include <QDebug>


namespace QmlViewer {

SubcomponentEditorTool::SubcomponentEditorTool(QDeclarativeDesignView *view)
    : AbstractFormEditorTool(view),
    m_animIncrement(0.05f),
    m_animTimer(new QTimer(this))
{
    m_mask = new SubcomponentMaskLayerItem(view, view->manipulatorLayer());
    connect(m_animTimer, SIGNAL(timeout()), SLOT(animate()));
    m_animTimer->setInterval(20);
}

SubcomponentEditorTool::~SubcomponentEditorTool()
{

}

void SubcomponentEditorTool::mousePressEvent(QMouseEvent */*event*/)
{

}

void SubcomponentEditorTool::mouseMoveEvent(QMouseEvent */*event*/)
{

}

bool SubcomponentEditorTool::containsCursor(const QPoint &mousePos) const
{
    if (!m_currentContext.size())
        return false;

    qDebug() << __FUNCTION__ << m_currentContext.top();

    QPointF scenePos = view()->mapToScene(mousePos);
    QRectF itemRect = m_currentContext.top()->boundingRect() | m_currentContext.top()->childrenBoundingRect();
    QRectF polyRect = m_currentContext.top()->mapToScene(itemRect).boundingRect();

    return polyRect.contains(scenePos);
}

void SubcomponentEditorTool::mouseReleaseEvent(QMouseEvent */*event*/)
{

}

void SubcomponentEditorTool::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton
        && !containsCursor(event->pos())
        && m_currentContext.size() > 1)
    {
        aboutToPopContext();
    }
}

void SubcomponentEditorTool::hoverMoveEvent(QMouseEvent *event)
{
    if (!containsCursor(event->pos()) && m_currentContext.size() > 1) {
        view()->clearHighlightBoundingRect();
    }
}

void SubcomponentEditorTool::wheelEvent(QWheelEvent */*event*/)
{

}

void SubcomponentEditorTool::keyPressEvent(QKeyEvent */*event*/)
{

}

void SubcomponentEditorTool::keyReleaseEvent(QKeyEvent */*keyEvent*/)
{

}

void SubcomponentEditorTool::itemsAboutToRemoved(const QList<QGraphicsItem*> &/*itemList*/)
{

}

void SubcomponentEditorTool::animate()
{
    if (m_animIncrement > 0) {
        if (m_mask->opacity() + m_animIncrement < 0.5f) {
            m_mask->setOpacity(m_mask->opacity() + m_animIncrement);
        } else {
            m_animTimer->stop();
            m_mask->setOpacity(0.5f);
        }
    } else {
        if (m_mask->opacity() + m_animIncrement > 0) {
            m_mask->setOpacity(m_mask->opacity() + m_animIncrement);
        } else {
            m_animTimer->stop();
            m_mask->setOpacity(0);
            popContext();
        }
    }

}

void SubcomponentEditorTool::clear()
{
    m_currentContext.clear();
    m_mask->setCurrentItem(0);
    m_animTimer->stop();
    m_mask->hide();
}

void SubcomponentEditorTool::graphicsObjectsChanged(const QList<QGraphicsObject*> &/*itemList*/)
{

}

void SubcomponentEditorTool::selectedItemsChanged(const QList<QGraphicsItem*> &/*itemList*/)
{

}

void SubcomponentEditorTool::setCurrentItem(QGraphicsItem* contextItem)
{
    if (!contextItem)
        return;

    QGraphicsObject *gfxObject = contextItem->toGraphicsObject();
    if (!gfxObject)
        return;

    //QString parentClassName = gfxObject->metaObject()->className();
    //if (parentClassName.contains(QRegExp("_QMLTYPE_\\d+")))

    bool containsSelectableItems = false;
    foreach(QGraphicsItem *item, gfxObject->childItems()) {
        if (item->type() == Constants::EditorItemType
         || item->type() == Constants::ResizeHandleItemType)
        {
            continue;
        }
        containsSelectableItems = true;
        break;
    }

    if (containsSelectableItems) {
        m_mask->setCurrentItem(gfxObject);
        m_mask->setOpacity(0);
        m_mask->show();
        m_animIncrement = 0.05f;
        m_animTimer->start();

        view()->clearHighlightBoundingRect();
        view()->setSelectedItems(QList<QGraphicsItem*>());

        pushContext(gfxObject);
    }
}

bool SubcomponentEditorTool::isDirectChildOfContext(QGraphicsItem *item) const
{
    return (item->parentItem() == m_currentContext.top());
}

bool SubcomponentEditorTool::itemIsChildOfQmlSubComponent(QGraphicsItem *item) const
{
    if (item->parentItem() && item->parentItem() != m_currentContext.top()) {
        QGraphicsObject *parent = item->parentItem()->toGraphicsObject();
        QString parentClassName = parent->metaObject()->className();

        if (parentClassName.contains(QRegExp("_QMLTYPE_\\d+"))) {
            return true;
        } else {
            return itemIsChildOfQmlSubComponent(parent);
        }
    }

    return false;
}

void SubcomponentEditorTool::pushContext(QGraphicsObject *contextItem)
{
    m_currentContext.push(contextItem);
}

void SubcomponentEditorTool::aboutToPopContext()
{
    if (m_currentContext.size() > 2) {
        popContext();
    } else {
        m_animIncrement = -0.05f;
        m_animTimer->start();
    }
}

QGraphicsObject *SubcomponentEditorTool::popContext()
{
    QGraphicsObject *popped = m_currentContext.pop();
    if (m_currentContext.size() > 1) {
        m_mask->setCurrentItem(m_currentContext.top());
        m_mask->setOpacity(0.5f);
        m_mask->setVisible(true);
    }

    return popped;
}

QGraphicsObject *SubcomponentEditorTool::currentRootItem() const
{
    return m_currentContext.top();
}

} // namespace QmlViewer
