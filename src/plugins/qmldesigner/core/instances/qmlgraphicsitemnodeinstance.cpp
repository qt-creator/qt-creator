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

#include "qmlgraphicsitemnodeinstance.h"
#include "qmlviewnodeinstance.h"
#include "graphicsscenenodeinstance.h"


#include <invalidnodeinstanceexception.h>
#include <propertymetainfo.h>

#include "bindingproperty.h"
#include "variantproperty.h"

#include <QDeclarativeExpression>

#include <private/qdeclarativeanchors_p.h>
#include <private/qdeclarativeanchors_p_p.h>
#include <private/qdeclarativeproperty_p.h>
#include <private/qdeclarativerectangle_p.h>

#include <cmath>

#include <QHash>

namespace QmlDesigner {
namespace Internal {

QmlGraphicsItemNodeInstance::QmlGraphicsItemNodeInstance(QDeclarativeItem *item, bool hasContent)
   : GraphicsObjectNodeInstance(item, hasContent),
     m_hasHeight(false),
     m_hasWidth(false)
{
}

QmlGraphicsItemNodeInstance::~QmlGraphicsItemNodeInstance()
{
}

QmlGraphicsItemNodeInstance::Pointer QmlGraphicsItemNodeInstance::create(const NodeMetaInfo &metaInfo, QDeclarativeContext *context, QObject *objectToBeWrapped)
{
    QPair<QGraphicsObject*, bool> objectPair;

    if (objectToBeWrapped)
        objectPair = qMakePair(qobject_cast<QGraphicsObject*>(objectToBeWrapped), false);
    else
        objectPair = GraphicsObjectNodeInstance::createGraphicsObject(metaInfo, context);

    QDeclarativeItem *qmlGraphicsItem = dynamic_cast<QDeclarativeItem*>(objectPair.first);

    if (qmlGraphicsItem == 0)
        throw InvalidNodeInstanceException(__LINE__, __FUNCTION__, __FILE__);

    Pointer instance(new QmlGraphicsItemNodeInstance(qmlGraphicsItem, objectPair.second));

    if (objectToBeWrapped)
        instance->setDeleteHeldInstance(false); // the object isn't owned

    instance->populateResetValueHash();

    return instance;
}

bool QmlGraphicsItemNodeInstance::isQmlGraphicsItem() const
{
    return true;
}

QSizeF QmlGraphicsItemNodeInstance::size() const
{
    if (modelNode().isValid()) {
        double implicitWidth = qmlGraphicsItem()->implicitWidth();
        if (!m_hasWidth
            && implicitWidth // WORKAROUND
            && implicitWidth != qmlGraphicsItem()->width()
            && !modelNode().hasBindingProperty("width")) {
            qmlGraphicsItem()->blockSignals(true);
            qmlGraphicsItem()->setWidth(implicitWidth);
            qmlGraphicsItem()->blockSignals(false);
        }

        double implicitHeight = qmlGraphicsItem()->implicitHeight();
        if (!m_hasHeight
            && implicitWidth // WORKAROUND
            && implicitHeight != qmlGraphicsItem()->height()
            && !modelNode().hasBindingProperty("height")) {
            qmlGraphicsItem()->blockSignals(true);
            qmlGraphicsItem()->setHeight(implicitHeight);
            qmlGraphicsItem()->blockSignals(false);
        }

    }

    if (modelNode().isRootNode()) {
        if (!m_hasWidth) {
            qmlGraphicsItem()->blockSignals(true);
            qmlGraphicsItem()->setWidth(100.);
            qmlGraphicsItem()->blockSignals(false);
        }

        if (!m_hasHeight) {
            qmlGraphicsItem()->blockSignals(true);
            qmlGraphicsItem()->setHeight(100.);
            qmlGraphicsItem()->blockSignals(false);
        }
    }

    return QSizeF(qmlGraphicsItem()->width(), qmlGraphicsItem()->height());
}

QRectF QmlGraphicsItemNodeInstance::boundingRect() const
{
    if (modelNode().isValid()) {
        double implicitWidth = qmlGraphicsItem()->implicitWidth();
        if (!m_hasWidth
            && implicitWidth // WORKAROUND
            && implicitWidth != qmlGraphicsItem()->width()
            && !modelNode().hasBindingProperty("width")) {
            qmlGraphicsItem()->blockSignals(true);
            qmlGraphicsItem()->setWidth(implicitWidth);
            qmlGraphicsItem()->blockSignals(false);
        }

        double implicitHeight = qmlGraphicsItem()->implicitHeight();
        if (!m_hasHeight
            && implicitWidth // WORKAROUND
            && implicitHeight != qmlGraphicsItem()->height()
            && !modelNode().hasBindingProperty("height")) {
            qmlGraphicsItem()->blockSignals(true);
            qmlGraphicsItem()->setHeight(implicitHeight);
            qmlGraphicsItem()->blockSignals(false);
        }

    }

    if (modelNode().isRootNode()) {
        if (!m_hasWidth) {
            qmlGraphicsItem()->blockSignals(true);
            qmlGraphicsItem()->setWidth(100.);
            qmlGraphicsItem()->blockSignals(false);
        }

        if (!m_hasHeight) {
            qmlGraphicsItem()->blockSignals(true);
            qmlGraphicsItem()->setHeight(100.);
            qmlGraphicsItem()->blockSignals(false);
        }
    }

    return qmlGraphicsItem()->boundingRect();
}

//QVariant anchorLineFor(QDeclarativeItem *item, const AnchorLine &anchorLine)
//{
//    switch(anchorLine.type()) {
//        case AnchorLine::Top : return item->property("top");
//        case AnchorLine::Bottom : return item->property("bottom");
//        case AnchorLine::Left : return item->property("left");
//        case AnchorLine::Right : return item->property("right");
//        case AnchorLine::HorizontalCenter : return item->property("horizontalCenter");
//        case AnchorLine::VerticalCenter : return item->property("verticalCenter");
//        case AnchorLine::Baseline : return item->property("baseline");
//        default: QVariant();
//        }
//
//    Q_ASSERT_X(false, Q_FUNC_INFO, QString::number(anchorLine.type()).toLatin1());
//    return QVariant();
//}

void QmlGraphicsItemNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    if (name == "height") {
       if (value.isValid())
           m_hasHeight = true;
       else
           m_hasHeight = false;
    }

    if (name == "width") {
       if (value.isValid())
           m_hasWidth = true;
       else
           m_hasWidth = false;
    }

    GraphicsObjectNodeInstance::setPropertyVariant(name, value);
}

void QmlGraphicsItemNodeInstance::setPropertyBinding(const QString &name, const QString &expression)
{
    GraphicsObjectNodeInstance::setPropertyBinding(name, expression);
}

QVariant QmlGraphicsItemNodeInstance::property(const QString &name) const
{
    if (name == "width" && modelNode().isValid() && !modelNode().hasBindingProperty("width")) {
        double implicitWidth = qmlGraphicsItem()->implicitWidth();
        if (!m_hasWidth
            && implicitWidth // WORKAROUND
            && implicitWidth != qmlGraphicsItem()->width())
            qmlGraphicsItem()->setWidth(implicitWidth);
    }

    if (name == "height" && modelNode().isValid() && !modelNode().hasBindingProperty("height")) {
        double implicitHeight = qmlGraphicsItem()->implicitHeight();
        if (!m_hasHeight
            && implicitHeight // WORKAROUND
            && implicitHeight != qmlGraphicsItem()->height())
            qmlGraphicsItem()->setHeight(implicitHeight);
    }

    return GraphicsObjectNodeInstance::property(name);
}

void QmlGraphicsItemNodeInstance::resetHorizontal()
 {
    if (modelNode().hasBindingProperty("x"))
       setPropertyBinding("x", modelNode().bindingProperty("x").expression());
    else if (modelNode().hasVariantProperty("x"))
       setPropertyVariant("x", modelNode().variantProperty("x").value());
    else
       setPropertyVariant("x", 0.0);

    if (modelNode().hasBindingProperty("width"))
       setPropertyBinding("width", modelNode().bindingProperty("width").expression());
    else if (modelNode().hasVariantProperty("width"))
       setPropertyVariant("width", modelNode().variantProperty("width").value());
    else
       setPropertyVariant("width", qmlGraphicsItem()->implicitWidth());
}

void QmlGraphicsItemNodeInstance::resetVertical()
 {
    if (modelNode().hasBindingProperty("y"))
       setPropertyBinding("y", modelNode().bindingProperty("y").expression());
    else if (modelNode().hasVariantProperty("y"))
       setPropertyVariant("y", modelNode().variantProperty("y").value());
    else
       setPropertyVariant("y", 0.0);

    if (modelNode().hasBindingProperty("height"))
       setPropertyBinding("height", modelNode().bindingProperty("height").expression());
    else if (modelNode().hasVariantProperty("height"))
       setPropertyVariant("height", modelNode().variantProperty("height").value());
    else
       setPropertyVariant("height", qmlGraphicsItem()->implicitHeight());
}

int QmlGraphicsItemNodeInstance::penWidth() const
{
    QDeclarativeRectangle *rectangle = qobject_cast<QDeclarativeRectangle*>(object());
    if (rectangle)
        return rectangle->border()->width();

    return GraphicsObjectNodeInstance::penWidth();
}

void QmlGraphicsItemNodeInstance::resetProperty(const QString &name)
{
    if (name == "height")
        m_hasHeight = false;

    if (name == "width")
        m_hasWidth = false;


    GraphicsObjectNodeInstance::resetProperty(name);
    if (name == "anchors.fill") {
        qmlGraphicsItem()->anchors()->resetFill();
        resetHorizontal();
        resetVertical();
    } else if (name == "anchors.centerIn") {
        qmlGraphicsItem()->anchors()->resetCenterIn();
        resetHorizontal();
        resetVertical();
    } else if (name == "anchors.top") {
        qmlGraphicsItem()->anchors()->resetTop();
        resetVertical();
    } else if (name == "anchors.left") {
        qmlGraphicsItem()->anchors()->resetLeft();
        resetHorizontal();
    } else if (name == "anchors.right") {
        qmlGraphicsItem()->anchors()->resetRight();
        resetHorizontal();
    } else if (name == "anchors.bottom") {
        qmlGraphicsItem()->anchors()->resetBottom();
        resetVertical();
    } else if (name == "anchors.horizontalCenter") {
        qmlGraphicsItem()->anchors()->resetHorizontalCenter();
        resetHorizontal();
    } else if (name == "anchors.verticalCenter") {
        qmlGraphicsItem()->anchors()->resetVerticalCenter();
        resetVertical();
    } else if (name == "anchors.baseline") {
        qmlGraphicsItem()->anchors()->resetBaseline();
        resetVertical();
    }
}

//void  QmlGraphicsItemNodeInstance::updateAnchors()
//{
//    NodeAnchors anchors(modelNode());
//
//    if (anchors.hasAnchor(AnchorLine::Top)) {
//        AnchorLine anchorLine(anchors.anchor(AnchorLine::Top));
//        NodeInstance instance(nodeInstanceView()->instanceForNode(anchorLine.modelNode()));
//
//        if (instance.isQmlGraphicsItem()) {
//            Pointer qmlGraphicsItemInstance(instance.QmlGraphicsItemNodeInstance());
//            qmlGraphicsItem()->anchors()->setProperty("top", anchorLineFor(qmlGraphicsItemInstance->qmlGraphicsItem(), anchorLine));
//        }
//    } else {
//        if (qmlGraphicsItem()->anchors()->usedAnchors().testFlag(QDeclarativeAnchors::HasTopAnchor)) {
//            qmlGraphicsItem()->anchors()->resetTop();
//            setPropertyValue("y", modelNode().property("y").value());
//            setPropertyValue("height", modelNode().property("height").value());
//        }
//    }
//
//
//    if (anchors.hasAnchor(AnchorLine::Left)) {
//        AnchorLine anchorLine(anchors.anchor(AnchorLine::Left));
//        NodeInstance instance(nodeInstanceView()->instanceForNode(anchorLine.modelNode()));
//
//        if (instance.isQmlGraphicsItem()) {
//            Pointer qmlGraphicsItemInstance(instance.QmlGraphicsItemNodeInstance());
//            qmlGraphicsItem()->anchors()->setProperty("left", anchorLineFor(qmlGraphicsItemInstance->qmlGraphicsItem(), anchorLine));
//        }
//    } else {
//        if (qmlGraphicsItem()->anchors()->usedAnchors().testFlag(QDeclarativeAnchors::HasLeftAnchor)) {
//            qmlGraphicsItem()->anchors()->resetLeft();
//            setPropertyValue("x", modelNode().property("x").value());
//            setPropertyValue("width", modelNode().property("width").value());
//        }
//    }
//
//
//    if (anchors.hasAnchor(AnchorLine::Right)) {
//        AnchorLine anchorLine(anchors.anchor(AnchorLine::Right));
//        NodeInstance instance(nodeInstanceView()->instanceForNode(anchorLine.modelNode()));
//
//        if (instance.isQmlGraphicsItem()) {
//            Pointer qmlGraphicsItemInstance(instance.QmlGraphicsItemNodeInstance());
//            qmlGraphicsItem()->anchors()->setProperty("right", anchorLineFor(qmlGraphicsItemInstance->qmlGraphicsItem(), anchorLine));
//        }
//    } else {
//        if (qmlGraphicsItem()->anchors()->usedAnchors().testFlag(QDeclarativeAnchors::HasRightAnchor)) {
//            qmlGraphicsItem()->anchors()->resetRight();
//            setPropertyValue("x", modelNode().property("x").value());
//            setPropertyValue("width", modelNode().property("width").value());
//        }
//    }
//
//
//    if (anchors.hasAnchor(AnchorLine::Bottom)) {
//        AnchorLine anchorLine(anchors.anchor(AnchorLine::Bottom));
//        NodeInstance instance(nodeInstanceView()->instanceForNode(anchorLine.modelNode()));
//
//        if (instance.isQmlGraphicsItem()) {
//            Pointer qmlGraphicsItemInstance(instance.QmlGraphicsItemNodeInstance());
//            qmlGraphicsItem()->anchors()->setProperty("bottom", anchorLineFor(qmlGraphicsItemInstance->qmlGraphicsItem(), anchorLine));
//        }
//    } else {
//        if (qmlGraphicsItem()->anchors()->usedAnchors().testFlag(QDeclarativeAnchors::HasBottomAnchor)) {
//            qmlGraphicsItem()->anchors()->resetBottom();
//            setPropertyValue("y", modelNode().property("y").value());
//            setPropertyValue("height", modelNode().property("height").value());
//        }
//    }
//
//
//    if (anchors.hasAnchor(AnchorLine::HorizontalCenter)) {
//        AnchorLine anchorLine(anchors.anchor(AnchorLine::HorizontalCenter));
//        NodeInstance instance(nodeInstanceView()->instanceForNode(anchorLine.modelNode()));
//
//        if (instance.isQmlGraphicsItem()) {
//            Pointer qmlGraphicsItemInstance(instance.QmlGraphicsItemNodeInstance());
//            qmlGraphicsItem()->anchors()->setProperty("horizontalCenter", anchorLineFor(qmlGraphicsItemInstance->qmlGraphicsItem(), anchorLine));
//        }
//    } else {
//        if (qmlGraphicsItem()->anchors()->usedAnchors().testFlag(QDeclarativeAnchors::HasHCenterAnchor)) {
//            qmlGraphicsItem()->anchors()->resetHorizontalCenter();
//            setPropertyValue("x", modelNode().property("x").value());
//            setPropertyValue("width", modelNode().property("width").value());
//        }
//    }
//
//
//    if (anchors.hasAnchor(AnchorLine::VerticalCenter)) {
//        AnchorLine anchorLine(anchors.anchor(AnchorLine::VerticalCenter));
//        NodeInstance instance(nodeInstanceView()->instanceForNode(anchorLine.modelNode()));
//
//        if (instance.isQmlGraphicsItem()) {
//            Pointer qmlGraphicsItemInstance(instance.QmlGraphicsItemNodeInstance());
//            qmlGraphicsItem()->anchors()->setProperty("verticalCenter",anchorLineFor(qmlGraphicsItemInstance->qmlGraphicsItem(), anchorLine));
//        }
//    } else {
//        if (qmlGraphicsItem()->anchors()->usedAnchors().testFlag(QDeclarativeAnchors::HasVCenterAnchor)) {
//            qmlGraphicsItem()->anchors()->resetVerticalCenter();
//            setPropertyValue("y", modelNode().property("y").value());
//            setPropertyValue("height", modelNode().property("height").value());
//        }
//    }
//
//
//    qmlGraphicsItem()->anchors()->setTopMargin(anchors.margin(AnchorLine::Top));
//    qmlGraphicsItem()->anchors()->setLeftMargin(anchors.margin(AnchorLine::Left));
//    qmlGraphicsItem()->anchors()->setBottomMargin(anchors.margin(AnchorLine::Bottom));
//    qmlGraphicsItem()->anchors()->setRightMargin(anchors.margin(AnchorLine::Right));
//    qmlGraphicsItem()->anchors()->setHorizontalCenterOffset(anchors.margin(AnchorLine::HorizontalCenter));
//    qmlGraphicsItem()->anchors()->setVerticalCenterOffset(anchors.margin(AnchorLine::VerticalCenter));
//}

QDeclarativeAnchors::UsedAnchor anchorLineFlagForName(const QString &name)
{
    if (name == "anchors.top")
        return QDeclarativeAnchors::HasTopAnchor;

    if (name == "anchors.left")
        return QDeclarativeAnchors::HasLeftAnchor;

    if (name == "anchors.bottom")
         return QDeclarativeAnchors::HasBottomAnchor;

    if (name == "anchors.right")
        return QDeclarativeAnchors::HasRightAnchor;

    if (name == "anchors.horizontalCenter")
        return QDeclarativeAnchors::HasHCenterAnchor;

    if (name == "anchors.verticalCenter")
         return QDeclarativeAnchors::HasVCenterAnchor;

    if (name == "anchors.baseline")
         return QDeclarativeAnchors::HasBaselineAnchor;


    Q_ASSERT_X(false, Q_FUNC_INFO, "wrong anchor name - this should never happen");
    return QDeclarativeAnchors::HasLeftAnchor;
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

bool isValidAnchorName(const QString &name)
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

QPair<QString, NodeInstance> QmlGraphicsItemNodeInstance::anchor(const QString &name) const
{
    if (!isValidAnchorName(name) || !hasAnchor(name))
        return GraphicsObjectNodeInstance::anchor(name);

    QObject *targetObject = 0;
    QString targetName;

    if (name == "anchors.fill") {
        targetObject = qmlGraphicsItem()->anchors()->fill();
    } else if (name == "anchors.centerIn") {
        targetObject = qmlGraphicsItem()->anchors()->centerIn();
    } else {
        QDeclarativeProperty metaProperty(object(), name, context());
        QDeclarativeAnchorLine anchorLine = metaProperty.read().value<QDeclarativeAnchorLine>();
        if (anchorLine.anchorLine != QDeclarativeAnchorLine::Invalid) {
            targetObject = anchorLine.item;
            targetName = propertyNameForAnchorLine(anchorLine.anchorLine);
        }

    }

    if (targetObject && nodeInstanceView()->hasInstanceForObject(targetObject)) {
        return qMakePair(targetName, nodeInstanceView()->instanceForObject(targetObject));
    } else {
        return GraphicsObjectNodeInstance::anchor(name);
    }
}

bool QmlGraphicsItemNodeInstance::hasAnchor(const QString &name) const
{
    if (!isValidAnchorName(name))
        return false;

    if (name == "anchors.fill")
        return qmlGraphicsItem()->anchors()->fill() != 0;

    if (name == "anchors.centerIn")
        return qmlGraphicsItem()->anchors()->centerIn() != 0;

    if (name == "anchors.right")
        return qmlGraphicsItem()->anchors()->right().item != 0;

    if (name == "anchors.top")
        return qmlGraphicsItem()->anchors()->top().item != 0;

    if (name == "anchors.left")
        return qmlGraphicsItem()->anchors()->left().item != 0;

    if (name == "anchors.bottom")
        return qmlGraphicsItem()->anchors()->bottom().item != 0;

    if (name == "anchors.horizontalCenter")
        return qmlGraphicsItem()->anchors()->horizontalCenter().item != 0;

    if (name == "anchors.verticalCenter")
        return qmlGraphicsItem()->anchors()->verticalCenter().item != 0;

    if (name == "anchors.baseline")
        return qmlGraphicsItem()->anchors()->baseline().item != 0;

    return qmlGraphicsItem()->anchors()->usedAnchors().testFlag(anchorLineFlagForName(name));
}

bool isAnchoredTo(QDeclarativeItem *fromItem, QDeclarativeItem *toItem)
{
    return fromItem->anchors()->fill() == toItem
            || fromItem->anchors()->centerIn() == toItem
            || fromItem->anchors()->bottom().item == toItem
            || fromItem->anchors()->top().item == toItem
            || fromItem->anchors()->left().item == toItem
            || fromItem->anchors()->right().item == toItem
            || fromItem->anchors()->verticalCenter().item == toItem
            || fromItem->anchors()->horizontalCenter().item == toItem
            || fromItem->anchors()->baseline().item == toItem;
}

bool areChildrenAnchoredTo(QDeclarativeItem *fromItem, QDeclarativeItem *toItem)
{
    foreach(QObject *childObject, fromItem->children()) {
        QDeclarativeItem *childItem = qobject_cast<QDeclarativeItem*>(childObject);
        if (childItem) {
            if (isAnchoredTo(childItem, toItem))
                return true;

            if (areChildrenAnchoredTo(childItem, toItem))
                return true;
        }
    }

    return false;
}

bool QmlGraphicsItemNodeInstance::isAnchoredBy() const
{
    if (areChildrenAnchoredTo(qmlGraphicsItem(), qmlGraphicsItem())) // search in children for a anchor to this item
        return true;

    if (qmlGraphicsItem()->parent()) {
        foreach(QObject *siblingObject, qmlGraphicsItem()->parent()->children()) { // search in siblings for a anchor to this item
            QDeclarativeItem *siblingItem = qobject_cast<QDeclarativeItem*>(siblingObject);
            if (siblingItem) {
                if (isAnchoredTo(siblingItem, qmlGraphicsItem()))
                    return true;

                if (areChildrenAnchoredTo(siblingItem, qmlGraphicsItem()))
                    return true;
            }
        }
    }

    return false;
}



QDeclarativeItem *QmlGraphicsItemNodeInstance::qmlGraphicsItem() const
{
    if (object() == 0)
        return 0;

    Q_ASSERT(qobject_cast<QDeclarativeItem*>(object()));
    return static_cast<QDeclarativeItem*>(object());
}
}
}
