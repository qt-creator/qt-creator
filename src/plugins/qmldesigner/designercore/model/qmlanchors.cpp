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

#include "qmlanchors.h"
#include "bindingproperty.h"
#include "nodeabstractproperty.h"
#include "rewritertransaction.h"
#include "qmlmodelview.h"

namespace QmlDesigner {


static PropertyName lineTypeToString(AnchorLine::Type lineType)
{
    switch (lineType) {
        case AnchorLine::Left:             return PropertyName("left");
        case AnchorLine::Top:              return PropertyName("top");
        case AnchorLine::Right:            return PropertyName("right");
        case AnchorLine::Bottom:           return PropertyName("bottom");
        case AnchorLine::HorizontalCenter: return PropertyName("horizontalCenter");
        case AnchorLine::VerticalCenter:   return PropertyName("verticalCenter");
        case AnchorLine::Baseline:         return PropertyName("baseline");
        case AnchorLine::Fill:             return PropertyName("fill");
        case AnchorLine::Center:           return PropertyName("centerIn");
        default:                           return PropertyName();
    }
}

bool AnchorLine::isHorizontalAnchorLine(Type anchorline)
{
    return anchorline & HorizontalMask;
}

bool AnchorLine::isVerticalAnchorLine(Type anchorline)
{
     return anchorline & VerticalMask;
}

static AnchorLine::Type propertyNameToLineType(const QString & string)
{
    if (string == QLatin1String("left"))
        return AnchorLine::Left;
    else if (string == QLatin1String("top"))
        return AnchorLine::Top;
    else if (string == QLatin1String("right"))
        return AnchorLine::Right;
    else if (string == QLatin1String("bottom"))
        return AnchorLine::Bottom;
    else if (string == QLatin1String("horizontalCenter"))
        return AnchorLine::HorizontalCenter;
    else if (string == QLatin1String("verticalCenter"))
        return AnchorLine::VerticalCenter;
    else if (string == QLatin1String("baseline"))
        return AnchorLine::VerticalCenter;
    else if (string == QLatin1String("centerIn"))
        return AnchorLine::Center;
    else if (string == QLatin1String("fill"))
        return AnchorLine::Fill;

    return AnchorLine::Invalid;
}

static PropertyName marginPropertyName(AnchorLine::Type lineType)
{
    switch (lineType) {
        case AnchorLine::Left:             return PropertyName("anchors.leftMargin");
        case AnchorLine::Top:              return PropertyName("anchors.topMargin");
        case AnchorLine::Right:            return PropertyName("anchors.rightMargin");
        case AnchorLine::Bottom:           return PropertyName("anchors.bottomMargin");
        case AnchorLine::HorizontalCenter: return PropertyName("anchors.horizontalCenterOffset");
        case AnchorLine::VerticalCenter:   return PropertyName("anchors.verticalCenterOffset");
        default:                           return PropertyName();
    }
}

static PropertyName anchorPropertyName(AnchorLine::Type lineType)
{
    const PropertyName typeString = lineTypeToString(lineType);

    if (typeString.isEmpty())
        return PropertyName();
    else
        return PropertyName("anchors.") + typeString;
}


QmlAnchors::QmlAnchors(const QmlItemNode &fxItemNode) : m_qmlItemNode(fxItemNode)
{
}

QmlItemNode QmlAnchors::qmlItemNode() const
{
    return m_qmlItemNode;
}

bool QmlAnchors::isValid() const
{
    return m_qmlItemNode.isValid();
}

void QmlAnchors::setAnchor(AnchorLine::Type sourceAnchorLine,
                          const QmlItemNode &targetQmlItemNode,
                          AnchorLine::Type targetAnchorLine)
{
    RewriterTransaction transaction = qmlItemNode().qmlModelView()->beginRewriterTransaction();
    if (qmlItemNode().isInBaseState()) {
        if ((qmlItemNode().nodeInstance().hasAnchor("anchors.fill") && (sourceAnchorLine & AnchorLine::Fill))
             || ((qmlItemNode().nodeInstance().hasAnchor("anchors.centerIn") && (sourceAnchorLine & AnchorLine::Center)))) {
            removeAnchor(sourceAnchorLine);
        }

        const PropertyName propertyName = anchorPropertyName(sourceAnchorLine);
        QString targetExpression = targetQmlItemNode.modelNode().validId();
        if (targetQmlItemNode.modelNode() == qmlItemNode().modelNode().parentProperty().parentModelNode())
            targetExpression = "parent";
        if (sourceAnchorLine != AnchorLine::Center && sourceAnchorLine != AnchorLine::Fill)
            targetExpression = targetExpression + QLatin1Char('.') + lineTypeToString(targetAnchorLine);
        qmlItemNode().modelNode().bindingProperty(propertyName).setExpression(targetExpression);
    }
}

bool detectHorizontalCycle(const ModelNode &node, QList<ModelNode> knownNodeList)
{
    if (knownNodeList.contains(node))
        return true;

    knownNodeList.append(node);

    static PropertyNameList validAnchorLines(PropertyNameList() << "right" << "left" << "horizontalCenter");
    static PropertyNameList anchorNames(PropertyNameList() << "anchors.right" << "anchors.left" << "anchors.horizontalCenter");

    foreach (const PropertyName &anchorName, anchorNames) {
        if (node.hasBindingProperty(anchorName)) {
            AbstractProperty targetProperty = node.bindingProperty(anchorName).resolveToProperty();
            if (targetProperty.isValid()) {
                if (!validAnchorLines.contains(targetProperty.name()))
                    return true;

                if (detectHorizontalCycle(targetProperty.parentModelNode(), knownNodeList))
                    return true;
            }
        }

    }

    static PropertyNameList anchorShortcutNames(PropertyNameList() << "anchors.fill" << "anchors.centerIn");
    foreach (const PropertyName &anchorName, anchorShortcutNames) {
        if (node.hasBindingProperty(anchorName)) {
            ModelNode targetNode = node.bindingProperty(anchorName).resolveToModelNode();

            if (targetNode.isValid() && detectHorizontalCycle(targetNode, knownNodeList))
                return true;
        }
    }

    return false;
}

bool detectVerticalCycle(const ModelNode &node, QList<ModelNode> knownNodeList)
{
    if (!node.isValid())
        return false;

    if (knownNodeList.contains(node))
        return true;

    knownNodeList.append(node);

    static PropertyNameList validAnchorLines(PropertyNameList() << "top" << "bottom" << "verticalCenter" << "baseline");
    static PropertyNameList anchorNames(PropertyNameList() << "anchors.top" << "anchors.bottom" << "anchors.verticalCenter" << "anchors.baseline");

    foreach (const PropertyName &anchorName, anchorNames) {
        if (node.hasBindingProperty(anchorName)) {
            AbstractProperty targetProperty = node.bindingProperty(anchorName).resolveToProperty();
            if (targetProperty.isValid()) {
                if (!validAnchorLines.contains(targetProperty.name()))
                    return true;

                if (detectVerticalCycle(targetProperty.parentModelNode(), knownNodeList))
                    return true;
            }
        }

    }

    static PropertyNameList anchorShortcutNames(PropertyNameList() << "anchors.fill" << "anchors.centerIn");
    foreach (const PropertyName &anchorName, anchorShortcutNames) {
        if (node.hasBindingProperty(anchorName)) {
            ModelNode targetNode = node.bindingProperty(anchorName).resolveToModelNode();

            if (targetNode.isValid() && detectVerticalCycle(targetNode, knownNodeList))
                return true;
        }
    }

    return false;
}

bool QmlAnchors::canAnchor(const QmlItemNode &targetModelNode) const
{
    if (!qmlItemNode().isInBaseState())
        return false;

    if (targetModelNode == qmlItemNode().instanceParent())
        return true;

    if (qmlItemNode().instanceParent() == targetModelNode.instanceParent())
        return true;

    return false;
}

AnchorLine::Type QmlAnchors::possibleAnchorLines(AnchorLine::Type sourceAnchorLineType,
                                                const QmlItemNode &targetQmlItemNode) const
{
    if (!canAnchor(targetQmlItemNode))
        return AnchorLine::Invalid;

    if (AnchorLine::isHorizontalAnchorLine(sourceAnchorLineType)) {
        if (!detectHorizontalCycle(targetQmlItemNode, QList<ModelNode>() << qmlItemNode().modelNode()))
            return AnchorLine::HorizontalMask;
    }

    if (AnchorLine::isVerticalAnchorLine(sourceAnchorLineType)) {
        if (!detectVerticalCycle(targetQmlItemNode, QList<ModelNode>() << qmlItemNode().modelNode()))
            return AnchorLine::VerticalMask;
    }

    return AnchorLine::Invalid;
}

AnchorLine QmlAnchors::instanceAnchor(AnchorLine::Type sourceAnchorLine) const
{
    QPair<PropertyName, qint32> targetAnchorLinePair;
    if (qmlItemNode().nodeInstance().hasAnchor("anchors.fill") && (sourceAnchorLine & AnchorLine::Fill)) {
        targetAnchorLinePair = qmlItemNode().nodeInstance().anchor("anchors.fill");
        targetAnchorLinePair.first = lineTypeToString(sourceAnchorLine); // TODO: looks wrong
    } else if (qmlItemNode().nodeInstance().hasAnchor("anchors.centerIn") && (sourceAnchorLine & AnchorLine::Center)) {
        targetAnchorLinePair = qmlItemNode().nodeInstance().anchor("anchors.centerIn");
        targetAnchorLinePair.first = lineTypeToString(sourceAnchorLine);
    } else {
        targetAnchorLinePair = qmlItemNode().nodeInstance().anchor(anchorPropertyName(sourceAnchorLine));
    }

    AnchorLine::Type targetAnchorLine = propertyNameToLineType(targetAnchorLinePair.first);

    if (targetAnchorLine == AnchorLine::Invalid )
        return AnchorLine();

    if (targetAnchorLinePair.second < 0) //there might be no node instance for the parent
        return AnchorLine();

    return AnchorLine(QmlItemNode(qmlItemNode().nodeForInstance(qmlItemNode().qmlModelView()->nodeInstanceView()->instanceForId(targetAnchorLinePair.second))), targetAnchorLine);
}

void QmlAnchors::removeAnchor(AnchorLine::Type sourceAnchorLine)
{
   RewriterTransaction transaction = qmlItemNode().qmlModelView()->beginRewriterTransaction();
    if (qmlItemNode().isInBaseState()) {
        const PropertyName propertyName = anchorPropertyName(sourceAnchorLine);
        if (qmlItemNode().nodeInstance().hasAnchor("anchors.fill") && (sourceAnchorLine & AnchorLine::Fill)) {
            qmlItemNode().modelNode().removeProperty("anchors.fill");
            qmlItemNode().modelNode().bindingProperty("anchors.top").setExpression("parent.top");
            qmlItemNode().modelNode().bindingProperty("anchors.left").setExpression("parent.left");
            qmlItemNode().modelNode().bindingProperty("anchors.bottom").setExpression("parent.bottom");
            qmlItemNode().modelNode().bindingProperty("anchors.right").setExpression("parent.right");

        } else if (qmlItemNode().nodeInstance().hasAnchor("anchors.centerIn") && (sourceAnchorLine & AnchorLine::Center)) {
            qmlItemNode().modelNode().removeProperty("anchors.centerIn");
            qmlItemNode().modelNode().bindingProperty("anchors.horizontalCenter").setExpression("parent.horizontalCenter");
            qmlItemNode().modelNode().bindingProperty("anchors.verticalCenter").setExpression("parent.verticalCenter");
        }

        qmlItemNode().modelNode().removeProperty(propertyName);
    }
}

void QmlAnchors::removeAnchors()
{
    RewriterTransaction transaction = qmlItemNode().qmlModelView()->beginRewriterTransaction();
    if (qmlItemNode().nodeInstance().hasAnchor("anchors.fill"))
        qmlItemNode().modelNode().removeProperty("anchors.fill");
    if (qmlItemNode().nodeInstance().hasAnchor("anchors.centerIn"))
        qmlItemNode().modelNode().removeProperty("anchors.centerIn");
    if (qmlItemNode().nodeInstance().hasAnchor("anchors.top"))
        qmlItemNode().modelNode().removeProperty("anchors.top");
    if (qmlItemNode().nodeInstance().hasAnchor("anchors.left"))
        qmlItemNode().modelNode().removeProperty("anchors.left");
    if (qmlItemNode().nodeInstance().hasAnchor("anchors.right"))
        qmlItemNode().modelNode().removeProperty("anchors.right");
    if (qmlItemNode().nodeInstance().hasAnchor("anchors.bottom"))
        qmlItemNode().modelNode().removeProperty("anchors.bottom");
    if (qmlItemNode().nodeInstance().hasAnchor("anchors.horizontalCenter"))
        qmlItemNode().modelNode().removeProperty("anchors.horizontalCenter");
    if (qmlItemNode().nodeInstance().hasAnchor("anchors.verticalCenter"))
        qmlItemNode().modelNode().removeProperty("anchors.verticalCenter");
    if (qmlItemNode().nodeInstance().hasAnchor("anchors.baseline"))
        qmlItemNode().modelNode().removeProperty("anchors.baseline");
}

bool QmlAnchors::instanceHasAnchor(AnchorLine::Type sourceAnchorLine) const
{
    const PropertyName propertyName = anchorPropertyName(sourceAnchorLine);

    if (sourceAnchorLine & AnchorLine::Fill)
        return qmlItemNode().nodeInstance().hasAnchor(propertyName) || qmlItemNode().nodeInstance().hasAnchor("anchors.fill");

    if (sourceAnchorLine & AnchorLine::Center)
        return qmlItemNode().nodeInstance().hasAnchor(propertyName) || qmlItemNode().nodeInstance().hasAnchor("anchors.centerIn");


    return qmlItemNode().nodeInstance().hasAnchor(propertyName);
}

bool QmlAnchors::instanceHasAnchors() const
{
    return instanceHasAnchor(AnchorLine::Left) ||
           instanceHasAnchor(AnchorLine::Right) ||
           instanceHasAnchor(AnchorLine::Top) ||
           instanceHasAnchor(AnchorLine::Bottom) ||
           instanceHasAnchor(AnchorLine::HorizontalCenter) ||
           instanceHasAnchor(AnchorLine::VerticalCenter) ||
            instanceHasAnchor(AnchorLine::Baseline);
}

QRectF contentRect(const NodeInstance &nodeInstance)
{
    QRectF contentRect(nodeInstance.position(), nodeInstance.size());
    return nodeInstance.contentTransform().mapRect(contentRect);
}

double QmlAnchors::instanceLeftAnchorLine() const
{
    return contentRect(qmlItemNode().nodeInstance()).x();
}

double QmlAnchors::instanceTopAnchorLine() const
{
    return contentRect(qmlItemNode().nodeInstance()).y();
}

double QmlAnchors::instanceRightAnchorLine() const
{
    return contentRect(qmlItemNode().nodeInstance()).x() + contentRect(qmlItemNode().nodeInstance()).width();
}

double QmlAnchors::instanceBottomAnchorLine() const
{
    return contentRect(qmlItemNode().nodeInstance()).y() + contentRect(qmlItemNode().nodeInstance()).height();
}

double QmlAnchors::instanceHorizontalCenterAnchorLine() const
{
    return (instanceLeftAnchorLine() + instanceRightAnchorLine()) / 2.0;
}

double QmlAnchors::instanceVerticalCenterAnchorLine() const
{
    return (instanceBottomAnchorLine() + instanceTopAnchorLine()) / 2.0;
}

double QmlAnchors::instanceAnchorLine(AnchorLine::Type anchorLine) const
{
    switch (anchorLine) {
    case AnchorLine::Left: return instanceLeftAnchorLine();
    case AnchorLine::Top: return instanceTopAnchorLine();
    case AnchorLine::Bottom: return instanceBottomAnchorLine();
    case AnchorLine::Right: return instanceRightAnchorLine();
    case AnchorLine::HorizontalCenter: return instanceHorizontalCenterAnchorLine();
    case AnchorLine::VerticalCenter: return instanceVerticalCenterAnchorLine();
    default: return 0;
    }

    return 0.0;
}

void QmlAnchors::setMargin(AnchorLine::Type sourceAnchorLineType, double margin) const
{
    PropertyName propertyName = marginPropertyName(sourceAnchorLineType);
    qmlItemNode().setVariantProperty(propertyName, qRound(margin));
}

bool QmlAnchors::instanceHasMargin(AnchorLine::Type sourceAnchorLineType) const
{
    return !qIsNull(instanceMargin(sourceAnchorLineType));
}

static bool checkForHorizontalCycleRecusive(const QmlAnchors &anchors, QList<QmlItemNode> &visitedItems)
{
    visitedItems.append(anchors.qmlItemNode());
    if (anchors.instanceHasAnchor(AnchorLine::Left)) {
        AnchorLine leftAnchorLine = anchors.instanceAnchor(AnchorLine::Left);
        if (visitedItems.contains(leftAnchorLine.qmlItemNode()) || checkForHorizontalCycleRecusive(leftAnchorLine.qmlItemNode().anchors(), visitedItems))
            return true;
    }

    if (anchors.instanceHasAnchor(AnchorLine::Right)) {
        AnchorLine rightAnchorLine = anchors.instanceAnchor(AnchorLine::Right);
        if (visitedItems.contains(rightAnchorLine.qmlItemNode()) || checkForHorizontalCycleRecusive(rightAnchorLine.qmlItemNode().anchors(), visitedItems))
            return true;
    }

    if (anchors.instanceHasAnchor(AnchorLine::HorizontalCenter)) {
        AnchorLine horizontalCenterAnchorLine = anchors.instanceAnchor(AnchorLine::HorizontalCenter);
        if (visitedItems.contains(horizontalCenterAnchorLine.qmlItemNode()) || checkForHorizontalCycleRecusive(horizontalCenterAnchorLine.qmlItemNode().anchors(), visitedItems))
            return true;
    }

    return false;
}

static bool checkForVerticalCycleRecusive(const QmlAnchors &anchors, QList<QmlItemNode> &visitedItems)
{
    visitedItems.append(anchors.qmlItemNode());

    if (anchors.instanceHasAnchor(AnchorLine::Top)) {
        AnchorLine topAnchorLine = anchors.instanceAnchor(AnchorLine::Top);
        if (visitedItems.contains(topAnchorLine.qmlItemNode()) || checkForVerticalCycleRecusive(topAnchorLine.qmlItemNode().anchors(), visitedItems))
            return true;
    }

    if (anchors.instanceHasAnchor(AnchorLine::Bottom)) {
        AnchorLine bottomAnchorLine = anchors.instanceAnchor(AnchorLine::Bottom);
        if (visitedItems.contains(bottomAnchorLine.qmlItemNode()) || checkForVerticalCycleRecusive(bottomAnchorLine.qmlItemNode().anchors(), visitedItems))
            return true;
    }

    if (anchors.instanceHasAnchor(AnchorLine::VerticalCenter)) {
        AnchorLine verticalCenterAnchorLine = anchors.instanceAnchor(AnchorLine::VerticalCenter);
        if (visitedItems.contains(verticalCenterAnchorLine.qmlItemNode()) || checkForVerticalCycleRecusive(verticalCenterAnchorLine.qmlItemNode().anchors(), visitedItems))
            return true;
    }

    return false;
}

bool QmlAnchors::checkForHorizontalCycle(const QmlItemNode &sourceItem) const
{
    QList<QmlItemNode> visitedItems;
    visitedItems.append(sourceItem);

    return checkForHorizontalCycleRecusive(*this, visitedItems);
}

bool QmlAnchors::checkForVerticalCycle(const QmlItemNode &sourceItem) const
{
    QList<QmlItemNode> visitedItems;
    visitedItems.append(sourceItem);

    return checkForVerticalCycleRecusive(*this, visitedItems);
}

double QmlAnchors::instanceMargin(AnchorLine::Type sourceAnchorLineType) const
{
    return qmlItemNode().nodeInstance().property(marginPropertyName(sourceAnchorLineType)).toDouble();
}

void QmlAnchors::removeMargin(AnchorLine::Type sourceAnchorLineType)
{
    if (qmlItemNode().isInBaseState()) {
        PropertyName propertyName = marginPropertyName(sourceAnchorLineType);
        qmlItemNode().modelNode().removeProperty(propertyName);
    }
}

void QmlAnchors::removeMargins()
{
    RewriterTransaction transaction = qmlItemNode().qmlModelView()->beginRewriterTransaction();
    removeMargin(AnchorLine::Left);
    removeMargin(AnchorLine::Right);
    removeMargin(AnchorLine::Top);
    removeMargin(AnchorLine::Bottom);
    removeMargin(AnchorLine::HorizontalCenter);
    removeMargin(AnchorLine::VerticalCenter);
}

QmlItemNode AnchorLine::qmlItemNode() const
{
    return m_qmlItemNode;
}

void QmlAnchors::fill()
{
    if (instanceHasAnchors())
        removeAnchors();

    qmlItemNode().modelNode().bindingProperty("anchors.fill").setExpression("parent");
}

void QmlAnchors::centerIn()
{
    if (instanceHasAnchors())
        removeAnchors();

    qmlItemNode().modelNode().bindingProperty("anchors.centerIn").setExpression("parent");
}

bool QmlAnchors::checkForCycle(AnchorLine::Type anchorLineTyp, const QmlItemNode &sourceItem) const
{
    if (anchorLineTyp & AnchorLine::HorizontalMask)
        return checkForHorizontalCycle(sourceItem);
    else
        return checkForVerticalCycle(sourceItem);
}

} //QmlDesigner
