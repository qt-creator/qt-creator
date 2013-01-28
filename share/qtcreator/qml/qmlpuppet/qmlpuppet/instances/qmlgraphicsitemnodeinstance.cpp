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

#include "qmlgraphicsitemnodeinstance.h"

#include <QDeclarativeExpression>

#include <private/qdeclarativeanchors_p.h>
#include <private/qdeclarativeanchors_p_p.h>
#include <private/qdeclarativeitem_p.h>
#include <private/qdeclarativeproperty_p.h>
#include <private/qdeclarativerectangle_p.h>
#include <private/qdeclarativepositioners_p.h>
#include <private/qdeclarativestategroup_p.h>


#include <cmath>

#include <QHash>

namespace QmlDesigner {
namespace Internal {

QmlGraphicsItemNodeInstance::QmlGraphicsItemNodeInstance(QDeclarativeItem *item)
   : GraphicsObjectNodeInstance(item),
     m_hasHeight(false),
     m_hasWidth(false),
     m_isResizable(true),
     m_x(0.0),
     m_y(0.0),
     m_width(0.0),
     m_height(0.0)
{
}

QmlGraphicsItemNodeInstance::~QmlGraphicsItemNodeInstance()
{
}

bool anyItemHasContent(QGraphicsItem *graphicsItem)
{
    if (!graphicsItem->flags().testFlag(QGraphicsItem::ItemHasNoContents))
        return true;

    foreach (QGraphicsItem *childItem, graphicsItem->childItems()) {
        if (anyItemHasContent(childItem))
            return true;
    }

    return false;
}

QmlGraphicsItemNodeInstance::Pointer QmlGraphicsItemNodeInstance::create(QObject *object)
{
    QDeclarativeItem *qmlGraphicsItem = dynamic_cast<QDeclarativeItem*>(object);

    Q_ASSERT(qmlGraphicsItem);

    Pointer instance(new QmlGraphicsItemNodeInstance(qmlGraphicsItem));

    instance->setHasContent(anyItemHasContent(qmlGraphicsItem));
    qmlGraphicsItem->setFlag(QGraphicsItem::ItemHasNoContents, false);

    static_cast<QDeclarativeParserStatus*>(qmlGraphicsItem)->classBegin();

    instance->populateResetHashes();

    return instance;
}

bool QmlGraphicsItemNodeInstance::isQmlGraphicsItem() const
{
    return true;
}

QSizeF QmlGraphicsItemNodeInstance::size() const
{
    double width;

    if (QDeclarativeItemPrivate::get(qmlGraphicsItem())->widthValid) {
        width = qmlGraphicsItem()->width();
    } else {
        width = qmlGraphicsItem()->implicitWidth();
    }

    double height;

    if (QDeclarativeItemPrivate::get(qmlGraphicsItem())->heightValid) {
        height = qmlGraphicsItem()->height();
    } else {
        height = qmlGraphicsItem()->implicitHeight();
    }


    return QSizeF(width, height);
}

void QmlGraphicsItemNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    if (name == "state")
        return; // states are only set by us

    if (name == "height") {
        m_height = value.toDouble();
       if (value.isValid())
           m_hasHeight = true;
       else
           m_hasHeight = false;
    }

    if (name == "width") {
       m_width = value.toDouble();
       if (value.isValid())
           m_hasWidth = true;
       else
           m_hasWidth = false;
    }

    if (name == "x")
        m_x = value.toDouble();

    if (name == "y")
        m_y = value.toDouble();

    GraphicsObjectNodeInstance::setPropertyVariant(name, value);

    refresh();
    if (isInPositioner())
        parentInstance()->refreshPositioner();
}

void QmlGraphicsItemNodeInstance::setPropertyBinding(const QString &name, const QString &expression)
{
    if (name == "state")
        return; // states are only set by us

    GraphicsObjectNodeInstance::setPropertyBinding(name, expression);
}

QVariant QmlGraphicsItemNodeInstance::property(const QString &name) const
{
    return GraphicsObjectNodeInstance::property(name);
}

void QmlGraphicsItemNodeInstance::resetHorizontal()
 {
    setPropertyVariant("x", m_x);
    if (m_width > 0.0) {
        setPropertyVariant("width", m_width);
    } else {
        setPropertyVariant("width", qmlGraphicsItem()->implicitWidth());
    }
}

void QmlGraphicsItemNodeInstance::resetVertical()
 {
    setPropertyVariant("y", m_y);
    if (m_height > 0.0) {
        setPropertyVariant("height", m_height);
    } else {
        setPropertyVariant("height", qmlGraphicsItem()->implicitWidth());
    }
}

static void repositioning(QDeclarativeItem *item)
{
    if (!item)
        return;

//    QDeclarativeBasePositioner *positioner = qobject_cast<QDeclarativeBasePositioner*>(item);
//    if (positioner)
//        positioner->rePositioning();

    if (item->parentObject())
        repositioning(qobject_cast<QDeclarativeItem*>(item->parentObject()));
}

void QmlGraphicsItemNodeInstance::refresh()
{
    repositioning(qmlGraphicsItem());
}

void QmlGraphicsItemNodeInstance::recursiveDoComponentComplete(QDeclarativeItem *declarativeItem)
{
    if (declarativeItem) {
        if (QDeclarativeItemPrivate::get(declarativeItem)->componentComplete)
            return;
        static_cast<QDeclarativeParserStatus*>(declarativeItem)->componentComplete();

        foreach (QGraphicsItem *childItem, declarativeItem->childItems()) {
            QGraphicsObject *childGraphicsObject = childItem->toGraphicsObject();
            QDeclarativeItem *childDeclarativeItem = qobject_cast<QDeclarativeItem*>(childGraphicsObject);
            if (childDeclarativeItem && !nodeInstanceServer()->hasInstanceForObject(childDeclarativeItem))
                recursiveDoComponentComplete(childDeclarativeItem);
        }
    }
}

void QmlGraphicsItemNodeInstance::doComponentComplete()
{
    if (qmlGraphicsItem()) {
        recursiveDoComponentComplete(qmlGraphicsItem());
    }

    graphicsObject()->update();
}

bool QmlGraphicsItemNodeInstance::isResizable() const
{
    if (isRootNodeInstance())
        return false;

    return m_isResizable && qmlGraphicsItem() && qmlGraphicsItem()->parentItem();
}

void QmlGraphicsItemNodeInstance::setResizable(bool resizeable)
{
    m_isResizable = resizeable;
}

int QmlGraphicsItemNodeInstance::penWidth() const
{
    QDeclarativeRectangle *rectangle = qobject_cast<QDeclarativeRectangle*>(object());
    if (rectangle)
        return rectangle->border()->width();

    return GraphicsObjectNodeInstance::penWidth();
}

void QmlGraphicsItemNodeInstance::initialize(const ObjectNodeInstance::Pointer &objectNodeInstance)
{
    GraphicsObjectNodeInstance::initialize(objectNodeInstance);

    if (objectNodeInstance->instanceId() == 0 && objectNodeInstance->isQmlGraphicsItem()) { // is root item
        objectNodeInstance.staticCast<QmlGraphicsItemNodeInstance>()->setVisible(true);
        objectNodeInstance->setResetValue("visible", true);
    }
}

void QmlGraphicsItemNodeInstance::setVisible(bool isVisible)
{
    qmlGraphicsItem()->setVisible(isVisible);
}

bool QmlGraphicsItemNodeInstance::isVisible() const
{
    return qmlGraphicsItem()->isVisible();
}

void QmlGraphicsItemNodeInstance::resetProperty(const QString &name)
{
    if (name == "height") {
        m_hasHeight = false;
        m_height = 0.0;
    }

    if (name == "width") {
        m_hasWidth = false;
        m_width = 0.0;
    }

    if (name == "x")
        m_x = 0.0;

    if (name == "y")
        m_y = 0.0;


    if (name == "anchors.fill") {
        anchors()->resetFill();
        resetHorizontal();
        resetVertical();
    } else if (name == "anchors.centerIn") {
        anchors()->resetCenterIn();
        resetHorizontal();
        resetVertical();
    } else if (name == "anchors.top") {
        anchors()->resetTop();
        resetVertical();
    } else if (name == "anchors.left") {
        anchors()->resetLeft();
        resetHorizontal();
    } else if (name == "anchors.right") {
        anchors()->resetRight();
        resetHorizontal();
    } else if (name == "anchors.bottom") {
        anchors()->resetBottom();
        resetVertical();
    } else if (name == "anchors.horizontalCenter") {
        anchors()->resetHorizontalCenter();
        resetHorizontal();
    } else if (name == "anchors.verticalCenter") {
        anchors()->resetVerticalCenter();
        resetVertical();
    } else if (name == "anchors.baseline") {
        anchors()->resetBaseline();
        resetVertical();
    }

    GraphicsObjectNodeInstance::resetProperty(name);

    if (isInPositioner())
        parentInstance()->refreshPositioner();
}

void QmlGraphicsItemNodeInstance::reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const QString &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const QString &newParentProperty)
{
    if (oldParentInstance && oldParentInstance->isPositioner()) {
        setInPositioner(false);
        setMovable(true);
    }

    bool componentComplete = QDeclarativeItemPrivate::get(qmlGraphicsItem())->componentComplete;
    QDeclarativeItemPrivate::get(qmlGraphicsItem())->componentComplete = 1;
    GraphicsObjectNodeInstance::reparent(oldParentInstance, oldParentProperty, newParentInstance, newParentProperty);
    QDeclarativeItemPrivate::get(qmlGraphicsItem())->componentComplete = componentComplete;

    if (newParentInstance && newParentInstance->isPositioner()) {
        setInPositioner(true);
        setMovable(false);
    }

    if (oldParentInstance && oldParentInstance->isPositioner() && !(newParentInstance && newParentInstance->isPositioner())) {
        if (!hasBindingForProperty("x"))
            setPropertyVariant("x", m_x);

        if (!hasBindingForProperty("y"))
            setPropertyVariant("y", m_y);
    }

    refresh();
    if (isInPositioner())
        parentInstance()->refreshPositioner();
}

QDeclarativeAnchors::Anchor anchorLineFlagForName(const QString &name)
{
    if (name == "anchors.top")
        return QDeclarativeAnchors::TopAnchor;

    if (name == "anchors.left")
        return QDeclarativeAnchors::LeftAnchor;

    if (name == "anchors.bottom")
         return QDeclarativeAnchors::BottomAnchor;

    if (name == "anchors.right")
        return QDeclarativeAnchors::RightAnchor;

    if (name == "anchors.horizontalCenter")
        return QDeclarativeAnchors::HCenterAnchor;

    if (name == "anchors.verticalCenter")
         return QDeclarativeAnchors::VCenterAnchor;

    if (name == "anchors.baseline")
         return QDeclarativeAnchors::BaselineAnchor;


    Q_ASSERT_X(false, Q_FUNC_INFO, "wrong anchor name - this should never happen");
    return QDeclarativeAnchors::LeftAnchor;
}

QString propertyNameForAnchorLine(const QDeclarativeAnchorLine::AnchorLine &anchorLine)
{
    switch(anchorLine) {
        case QDeclarativeAnchorLine::Left: return "left";
        case QDeclarativeAnchorLine::Right: return "right";
        case QDeclarativeAnchorLine::Top: return "top";
        case QDeclarativeAnchorLine::Bottom: return "bottom";
        case QDeclarativeAnchorLine::HCenter: return "horizontalCenter";
        case QDeclarativeAnchorLine::VCenter: return "verticalCenter";
        case QDeclarativeAnchorLine::Baseline: return "baseline";
        case QDeclarativeAnchorLine::Invalid:
        default: return QString();
    }
}

static bool isValidAnchorName(const QString &name)
{
    static QStringList anchorNameList(QStringList() << "anchors.top"
                                                    << "anchors.left"
                                                    << "anchors.right"
                                                    << "anchors.bottom"
                                                    << "anchors.verticalCenter"
                                                    << "anchors.horizontalCenter"
                                                    << "anchors.fill"
                                                    << "anchors.centerIn"
                                                    << "anchors.baseline");

    return anchorNameList.contains(name);
}

QPair<QString, ServerNodeInstance> QmlGraphicsItemNodeInstance::anchor(const QString &name) const
{
    if (!isValidAnchorName(name) || !hasAnchor(name))
        return GraphicsObjectNodeInstance::anchor(name);

    QObject *targetObject = 0;
    QString targetName;

    if (name == "anchors.fill") {
        targetObject = anchors()->fill();
    } else if (name == "anchors.centerIn") {
        targetObject = anchors()->centerIn();
    } else {
        QDeclarativeProperty metaProperty(object(), name, context());
        if (!metaProperty.isValid())
            return GraphicsObjectNodeInstance::anchor(name);

        QDeclarativeAnchorLine anchorLine = metaProperty.read().value<QDeclarativeAnchorLine>();
        if (anchorLine.anchorLine != QDeclarativeAnchorLine::Invalid) {
            targetObject = anchorLine.item;
            targetName = propertyNameForAnchorLine(anchorLine.anchorLine);
        }

    }

    if (targetObject && nodeInstanceServer()->hasInstanceForObject(targetObject)) {
        return qMakePair(targetName, nodeInstanceServer()->instanceForObject(targetObject));
    } else {
        return GraphicsObjectNodeInstance::anchor(name);
    }
}

QList<ServerNodeInstance> QmlGraphicsItemNodeInstance::stateInstances() const
{
    QList<ServerNodeInstance> instanceList;
    QList<QDeclarativeState *> stateList = QDeclarativeItemPrivate::get(qmlGraphicsItem())->_states()->states();
    foreach(QDeclarativeState *state, stateList)
    {
        if (state && nodeInstanceServer()->hasInstanceForObject(state))
            instanceList.append(nodeInstanceServer()->instanceForObject(state));
    }

    return instanceList;
}

bool QmlGraphicsItemNodeInstance::hasAnchor(const QString &name) const
{
    if (!isValidAnchorName(name))
        return false;

    if (name == "anchors.fill")
        return anchors()->fill() != 0;

    if (name == "anchors.centerIn")
        return anchors()->centerIn() != 0;

    if (name == "anchors.right")
        return anchors()->right().item != 0;

    if (name == "anchors.top")
        return anchors()->top().item != 0;

    if (name == "anchors.left")
        return anchors()->left().item != 0;

    if (name == "anchors.bottom")
        return anchors()->bottom().item != 0;

    if (name == "anchors.horizontalCenter")
        return anchors()->horizontalCenter().item != 0;

    if (name == "anchors.verticalCenter")
        return anchors()->verticalCenter().item != 0;

    if (name == "anchors.baseline")
        return anchors()->baseline().item != 0;

    return anchors()->usedAnchors().testFlag(anchorLineFlagForName(name));
}

bool isAnchoredTo(QDeclarativeItem *fromItem, QDeclarativeItem *toItem)
{
    Q_ASSERT(dynamic_cast<QDeclarativeItemPrivate*>(QGraphicsItemPrivate::get(fromItem)));
    QDeclarativeItemPrivate *fromItemPrivate = static_cast<QDeclarativeItemPrivate*>(QGraphicsItemPrivate::get(fromItem));
    QDeclarativeAnchors *anchors = fromItemPrivate->anchors();
    return anchors->fill() == toItem
            || anchors->centerIn() == toItem
            || anchors->bottom().item == toItem
            || anchors->top().item == toItem
            || anchors->left().item == toItem
            || anchors->right().item == toItem
            || anchors->verticalCenter().item == toItem
            || anchors->horizontalCenter().item == toItem
            || anchors->baseline().item == toItem;
}

bool areChildrenAnchoredTo(QDeclarativeItem *fromItem, QDeclarativeItem *toItem)
{
    foreach(QGraphicsItem *childGraphicsItem, fromItem->childItems()) {
        QDeclarativeItem *childItem = qobject_cast<QDeclarativeItem*>(childGraphicsItem->toGraphicsObject());
        if (childItem) {
            if (isAnchoredTo(childItem, toItem))
                return true;

            if (areChildrenAnchoredTo(childItem, toItem))
                return true;
        }
    }

    return false;
}

bool QmlGraphicsItemNodeInstance::isAnchoredBySibling() const
{
    if (qmlGraphicsItem()->parentItem()) {
        foreach(QGraphicsItem *siblingGraphicsItem, qmlGraphicsItem()->parentItem()->childItems()) { // search in siblings for a anchor to this item
            QDeclarativeItem *siblingItem = qobject_cast<QDeclarativeItem*>(siblingGraphicsItem->toGraphicsObject());
            if (siblingItem) {
                if (isAnchoredTo(siblingItem, qmlGraphicsItem()))
                    return true;
            }
        }
    }

    return false;
}

bool QmlGraphicsItemNodeInstance::isAnchoredByChildren() const
{
    if (areChildrenAnchoredTo(qmlGraphicsItem(), qmlGraphicsItem())) // search in children for a anchor to this item
        return true;

    return false;
}

QDeclarativeItem *QmlGraphicsItemNodeInstance::qmlGraphicsItem() const
{
    if (object() == 0)
        return 0;

    Q_ASSERT(qobject_cast<QDeclarativeItem*>(object()));
    return static_cast<QDeclarativeItem*>(object());
}

QDeclarativeAnchors *QmlGraphicsItemNodeInstance::anchors() const
{
    Q_ASSERT(dynamic_cast<QDeclarativeItemPrivate*>(QGraphicsItemPrivate::get(qmlGraphicsItem())));
    QDeclarativeItemPrivate *itemPrivate = static_cast<QDeclarativeItemPrivate*>(QGraphicsItemPrivate::get(qmlGraphicsItem()));
    return itemPrivate->anchors();
}

} // namespace Internal
} // namespace QmlDesigner
