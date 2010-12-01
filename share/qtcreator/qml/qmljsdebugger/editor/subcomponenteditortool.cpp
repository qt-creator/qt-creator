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

#include "subcomponenteditortool.h"
#include "../qdeclarativeviewobserver_p.h"
#include "subcomponentmasklayeritem.h"
#include "layeritem.h"

#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QTimer>
#include <QMouseEvent>
#include <QKeyEvent>

#include <QDebug>


namespace QmlJSDebugger {

const qreal MaxOpacity = 0.5f;

SubcomponentEditorTool::SubcomponentEditorTool(QDeclarativeViewObserver *view)
    : AbstractFormEditorTool(view),
      m_animIncrement(0.05f),
      m_animTimer(new QTimer(this))
{
    QDeclarativeViewObserverPrivate *observerPrivate =
            QDeclarativeViewObserverPrivate::get(view);
    m_mask = new SubcomponentMaskLayerItem(view, observerPrivate->manipulatorLayer);
    connect(m_animTimer, SIGNAL(timeout()), SLOT(animate()));
    m_animTimer->setInterval(20);
}

SubcomponentEditorTool::~SubcomponentEditorTool()
{

}

void SubcomponentEditorTool::mousePressEvent(QMouseEvent * /*event*/)
{

}

void SubcomponentEditorTool::mouseMoveEvent(QMouseEvent * /*event*/)
{

}

bool SubcomponentEditorTool::containsCursor(const QPoint &mousePos) const
{
    if (!m_currentContext.size())
        return false;

    QPointF scenePos = view()->mapToScene(mousePos);
    QRectF itemRect = m_currentContext.top()->boundingRect()
            | m_currentContext.top()->childrenBoundingRect();
    QRectF polyRect = m_currentContext.top()->mapToScene(itemRect).boundingRect();

    return polyRect.contains(scenePos);
}

void SubcomponentEditorTool::mouseReleaseEvent(QMouseEvent * /*event*/)
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
        QDeclarativeViewObserverPrivate::get(observer())->clearHighlight();
    }
}

void SubcomponentEditorTool::wheelEvent(QWheelEvent * /*event*/)
{

}

void SubcomponentEditorTool::keyPressEvent(QKeyEvent * /*event*/)
{

}

void SubcomponentEditorTool::keyReleaseEvent(QKeyEvent * /*keyEvent*/)
{

}

void SubcomponentEditorTool::itemsAboutToRemoved(const QList<QGraphicsItem*> &/*itemList*/)
{

}

void SubcomponentEditorTool::animate()
{
    if (m_animIncrement > 0) {
        if (m_mask->opacity() + m_animIncrement < MaxOpacity) {
            m_mask->setOpacity(m_mask->opacity() + m_animIncrement);
        } else {
            m_animTimer->stop();
            m_mask->setOpacity(MaxOpacity);
        }
    } else {
        if (m_mask->opacity() + m_animIncrement > 0) {
            m_mask->setOpacity(m_mask->opacity() + m_animIncrement);
        } else {
            m_animTimer->stop();
            m_mask->setOpacity(0);
            popContext();
            emit contextPathChanged(m_path);
        }
    }

}

void SubcomponentEditorTool::clear()
{
    m_currentContext.clear();
    m_mask->setCurrentItem(0);
    m_animTimer->stop();
    m_mask->hide();
    m_path.clear();

    emit contextPathChanged(m_path);
    emit cleared();
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

        QDeclarativeViewObserverPrivate::get(observer())->clearHighlight();
        observer()->setSelectedItems(QList<QGraphicsItem*>());

        pushContext(gfxObject);
    }
}

QGraphicsItem *SubcomponentEditorTool::firstChildOfContext(QGraphicsItem *item) const
{
    if (!item)
        return 0;

    if (isDirectChildOfContext(item))
        return item;

    QGraphicsItem *parent = item->parentItem();
    while (parent) {
        if (isDirectChildOfContext(parent))
            return parent;
        parent = parent->parentItem();
    }

    return 0;
}

bool SubcomponentEditorTool::isChildOfContext(QGraphicsItem *item) const
{
    return (firstChildOfContext(item) != 0);
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
    connect(contextItem, SIGNAL(destroyed(QObject*)), this, SLOT(contextDestroyed(QObject*)));
    connect(contextItem, SIGNAL(xChanged()), this, SLOT(resizeMask()));
    connect(contextItem, SIGNAL(yChanged()), this, SLOT(resizeMask()));
    connect(contextItem, SIGNAL(widthChanged()), this, SLOT(resizeMask()));
    connect(contextItem, SIGNAL(heightChanged()), this, SLOT(resizeMask()));
    connect(contextItem, SIGNAL(rotationChanged()), this, SLOT(resizeMask()));

    m_currentContext.push(contextItem);
    QString title = titleForItem(contextItem);
    emit contextPushed(title);

    m_path << title;
    emit contextPathChanged(m_path);
}

void SubcomponentEditorTool::aboutToPopContext()
{
    if (m_currentContext.size() > 2) {
        popContext();
        emit contextPathChanged(m_path);
    } else {
        m_animIncrement = -0.05f;
        m_animTimer->start();
    }
}

QGraphicsObject *SubcomponentEditorTool::popContext()
{
    QGraphicsObject *popped = m_currentContext.pop();
    m_path.removeLast();

    emit contextPopped();

    disconnect(popped, SIGNAL(xChanged()), this, SLOT(resizeMask()));
    disconnect(popped, SIGNAL(yChanged()), this, SLOT(resizeMask()));
    disconnect(popped, SIGNAL(scaleChanged()), this, SLOT(resizeMask()));
    disconnect(popped, SIGNAL(widthChanged()), this, SLOT(resizeMask()));
    disconnect(popped, SIGNAL(heightChanged()), this, SLOT(resizeMask()));

    if (m_currentContext.size() > 1) {
        QGraphicsObject *item = m_currentContext.top();
        m_mask->setCurrentItem(item);
        m_mask->setOpacity(MaxOpacity);
        m_mask->setVisible(true);
    } else {
        m_mask->setVisible(false);
    }

    return popped;
}

void SubcomponentEditorTool::resizeMask()
{
    QGraphicsObject *item = m_currentContext.top();
    m_mask->setCurrentItem(item);
}

QGraphicsObject *SubcomponentEditorTool::currentRootItem() const
{
    return m_currentContext.top();
}

void SubcomponentEditorTool::contextDestroyed(QObject *contextToDestroy)
{
    disconnect(contextToDestroy, SIGNAL(destroyed(QObject*)),
               this, SLOT(contextDestroyed(QObject*)));

    // pop out the whole context - it might not be safe anymore.
    while (m_currentContext.size() > 1) {
        m_currentContext.pop();
        m_path.removeLast();
        emit contextPopped();
    }
    m_mask->setVisible(false);

    emit contextPathChanged(m_path);
}

QGraphicsObject *SubcomponentEditorTool::setContext(int contextIndex)
{
    Q_ASSERT(contextIndex >= 0);

    // sometimes we have to delete the context while user was still clicking around,
    // so just bail out.
    if (contextIndex >= m_currentContext.size() -1)
        return 0;

    while (m_currentContext.size() - 1 > contextIndex) {
        popContext();
    }
    emit contextPathChanged(m_path);

    return m_currentContext.top();
}

int SubcomponentEditorTool::contextIndex() const
{
    return m_currentContext.size() - 1;
}

} // namespace QmlJSDebugger
