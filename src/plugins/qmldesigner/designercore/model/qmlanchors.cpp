/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlanchors.h"
#include "bindingproperty.h"
#include "nodeabstractproperty.h"
#include "rewritertransaction.h"
#include "nodeinstanceview.h"

namespace QmlDesigner {


static PropertyName lineTypeToString(AnchorLineType lineType)
{
    switch (lineType) {
        case AnchorLineLeft:             return PropertyName("left");
        case AnchorLineTop:              return PropertyName("top");
        case AnchorLineRight:            return PropertyName("right");
        case AnchorLineBottom:           return PropertyName("bottom");
        case AnchorLineHorizontalCenter: return PropertyName("horizontalCenter");
        case AnchorLineVerticalCenter:   return PropertyName("verticalCenter");
        case AnchorLineBaseline:         return PropertyName("baseline");
        case AnchorLineFill:             return PropertyName("fill");
        case AnchorLineCenter:           return PropertyName("centerIn");
        default:                           return PropertyName();
    }
}

static AnchorLineType propertyNameToLineType(const QString & string)
{
    if (string == QStringLiteral("left"))
        return AnchorLineLeft;
    else if (string == QStringLiteral("top"))
        return AnchorLineTop;
    else if (string == QStringLiteral("right"))
        return AnchorLineRight;
    else if (string == QStringLiteral("bottom"))
        return AnchorLineBottom;
    else if (string == QStringLiteral("horizontalCenter"))
        return AnchorLineHorizontalCenter;
    else if (string == QStringLiteral("verticalCenter"))
        return AnchorLineVerticalCenter;
    else if (string == QStringLiteral("baseline"))
        return AnchorLineVerticalCenter;
    else if (string == QStringLiteral("centerIn"))
        return AnchorLineCenter;
    else if (string == QStringLiteral("fill"))
        return AnchorLineFill;

    return AnchorLineInvalid;
}

static PropertyName marginPropertyName(AnchorLineType lineType)
{
    switch (lineType) {
        case AnchorLineLeft:             return PropertyName("anchors.leftMargin");
        case AnchorLineTop:              return PropertyName("anchors.topMargin");
        case AnchorLineRight:            return PropertyName("anchors.rightMargin");
        case AnchorLineBottom:           return PropertyName("anchors.bottomMargin");
        case AnchorLineHorizontalCenter: return PropertyName("anchors.horizontalCenterOffset");
        case AnchorLineVerticalCenter:   return PropertyName("anchors.verticalCenterOffset");
        default:                           return PropertyName();
    }
}

static PropertyName anchorPropertyName(AnchorLineType lineType)
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

bool QmlAnchors::modelHasAnchors() const
{
    return modelHasAnchor(AnchorLineLeft)
            || modelHasAnchor(AnchorLineRight)
            || modelHasAnchor(AnchorLineTop)
            || modelHasAnchor(AnchorLineBottom)
            || modelHasAnchor(AnchorLineHorizontalCenter)
            || modelHasAnchor(AnchorLineVerticalCenter)
            || modelHasAnchor(AnchorLineBaseline);
}

bool QmlAnchors::modelHasAnchor(AnchorLineType sourceAnchorLineType) const
{
    const PropertyName propertyName = anchorPropertyName(sourceAnchorLineType);

    if (sourceAnchorLineType & AnchorLineFill)
        return qmlItemNode().modelNode().hasBindingProperty(propertyName) || qmlItemNode().modelNode().hasBindingProperty("anchors.fill");

    if (sourceAnchorLineType & AnchorLineCenter)
        return qmlItemNode().modelNode().hasBindingProperty(propertyName) || qmlItemNode().modelNode().hasBindingProperty("anchors.centerIn");

    return qmlItemNode().modelNode().hasBindingProperty(anchorPropertyName(sourceAnchorLineType));
}

AnchorLine QmlAnchors::modelAnchor(AnchorLineType sourceAnchorLineType) const
{
 QPair<PropertyName, ModelNode> targetAnchorLinePair;
 if (sourceAnchorLineType & AnchorLineFill && qmlItemNode().modelNode().hasBindingProperty("anchors.fill")) {
     targetAnchorLinePair.second = qmlItemNode().modelNode().bindingProperty("anchors.fill").resolveToModelNode();
     targetAnchorLinePair.first = lineTypeToString(sourceAnchorLineType);
 } else if (sourceAnchorLineType & AnchorLineCenter && qmlItemNode().modelNode().hasBindingProperty("anchors.centerIn")) {
     targetAnchorLinePair.second = qmlItemNode().modelNode().bindingProperty("anchors.centerIn").resolveToModelNode();
     targetAnchorLinePair.first = lineTypeToString(sourceAnchorLineType);
 } else {
     AbstractProperty binding = qmlItemNode().modelNode().bindingProperty(anchorPropertyName(sourceAnchorLineType)).resolveToProperty();
     targetAnchorLinePair.first = binding.name();
     targetAnchorLinePair.second = binding.parentModelNode();
 }

 AnchorLineType targetAnchorLine = propertyNameToLineType(targetAnchorLinePair.first);

 if (targetAnchorLine == AnchorLineInvalid )
     return AnchorLine();


 return AnchorLine(QmlItemNode(targetAnchorLinePair.second), targetAnchorLine);
}

bool QmlAnchors::isValid() const
{
    return m_qmlItemNode.isValid();
}

void QmlAnchors::setAnchor(AnchorLineType sourceAnchorLine,
                          const QmlItemNode &targetQmlItemNode,
                          AnchorLineType targetAnchorLine)
{
    RewriterTransaction transaction = qmlItemNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchors::setAnchor"));
    if (qmlItemNode().isInBaseState()) {
        if ((qmlItemNode().nodeInstance().hasAnchor("anchors.fill") && (sourceAnchorLine & AnchorLineFill))
             || ((qmlItemNode().nodeInstance().hasAnchor("anchors.centerIn") && (sourceAnchorLine & AnchorLineCenter)))) {
            removeAnchor(sourceAnchorLine);
        }

        const PropertyName propertyName = anchorPropertyName(sourceAnchorLine);
        ModelNode targetModelNode = targetQmlItemNode.modelNode();
        QString targetExpression = targetModelNode.validId();
        if (targetQmlItemNode.modelNode() == qmlItemNode().modelNode().parentProperty().parentModelNode())
            targetExpression = "parent";
        if (sourceAnchorLine != AnchorLineCenter && sourceAnchorLine != AnchorLineFill)
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

AnchorLineType QmlAnchors::possibleAnchorLines(AnchorLineType sourceAnchorLineType,
                                               const QmlItemNode &targetQmlItemNode) const
{
    if (!canAnchor(targetQmlItemNode))
        return AnchorLineInvalid;

    if (AnchorLine::isHorizontalAnchorLine(sourceAnchorLineType)) {
        if (!detectHorizontalCycle(targetQmlItemNode, QList<ModelNode>() << qmlItemNode().modelNode()))
            return AnchorLineHorizontalMask;
    }

    if (AnchorLine::isVerticalAnchorLine(sourceAnchorLineType)) {
        if (!detectVerticalCycle(targetQmlItemNode, QList<ModelNode>() << qmlItemNode().modelNode()))
            return AnchorLineVerticalMask;
    }

    return AnchorLineInvalid;
}

AnchorLine QmlAnchors::instanceAnchor(AnchorLineType sourceAnchorLine) const
{
    QPair<PropertyName, qint32> targetAnchorLinePair;
    if (qmlItemNode().nodeInstance().hasAnchor("anchors.fill") && (sourceAnchorLine & AnchorLineFill)) {
        targetAnchorLinePair = qmlItemNode().nodeInstance().anchor("anchors.fill");
        targetAnchorLinePair.first = lineTypeToString(sourceAnchorLine); // TODO: looks wrong
    } else if (qmlItemNode().nodeInstance().hasAnchor("anchors.centerIn") && (sourceAnchorLine & AnchorLineCenter)) {
        targetAnchorLinePair = qmlItemNode().nodeInstance().anchor("anchors.centerIn");
        targetAnchorLinePair.first = lineTypeToString(sourceAnchorLine);
    } else {
        targetAnchorLinePair = qmlItemNode().nodeInstance().anchor(anchorPropertyName(sourceAnchorLine));
    }

    AnchorLineType targetAnchorLine = propertyNameToLineType(targetAnchorLinePair.first);

    if (targetAnchorLine == AnchorLineInvalid )
        return AnchorLine();

    if (targetAnchorLinePair.second < 0) //there might be no node instance for the parent
        return AnchorLine();

    return AnchorLine(QmlItemNode(qmlItemNode().nodeForInstance(qmlItemNode().nodeInstanceView()->instanceForId(targetAnchorLinePair.second))), targetAnchorLine);
}

void QmlAnchors::removeAnchor(AnchorLineType sourceAnchorLine)
{
   RewriterTransaction transaction = qmlItemNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchors::removeAnchor"));
    if (qmlItemNode().isInBaseState()) {
        const PropertyName propertyName = anchorPropertyName(sourceAnchorLine);
        if (qmlItemNode().nodeInstance().hasAnchor("anchors.fill") && (sourceAnchorLine & AnchorLineFill)) {
            qmlItemNode().modelNode().removeProperty("anchors.fill");
            qmlItemNode().modelNode().bindingProperty("anchors.top").setExpression("parent.top");
            qmlItemNode().modelNode().bindingProperty("anchors.left").setExpression("parent.left");
            qmlItemNode().modelNode().bindingProperty("anchors.bottom").setExpression("parent.bottom");
            qmlItemNode().modelNode().bindingProperty("anchors.right").setExpression("parent.right");

        } else if (qmlItemNode().nodeInstance().hasAnchor("anchors.centerIn") && (sourceAnchorLine & AnchorLineCenter)) {
            qmlItemNode().modelNode().removeProperty("anchors.centerIn");
            qmlItemNode().modelNode().bindingProperty("anchors.horizontalCenter").setExpression("parent.horizontalCenter");
            qmlItemNode().modelNode().bindingProperty("anchors.verticalCenter").setExpression("parent.verticalCenter");
        }

        qmlItemNode().modelNode().removeProperty(propertyName);
    }
}

void QmlAnchors::removeAnchors()
{
    RewriterTransaction transaction = qmlItemNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchors::removeAnchors"));
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

bool QmlAnchors::instanceHasAnchor(AnchorLineType sourceAnchorLine) const
{
    if (!qmlItemNode().isValid())
        return false;

    const PropertyName propertyName = anchorPropertyName(sourceAnchorLine);

    if (sourceAnchorLine & AnchorLineFill)
        return qmlItemNode().nodeInstance().hasAnchor(propertyName) || qmlItemNode().nodeInstance().hasAnchor("anchors.fill");

    if (sourceAnchorLine & AnchorLineCenter)
        return qmlItemNode().nodeInstance().hasAnchor(propertyName) || qmlItemNode().nodeInstance().hasAnchor("anchors.centerIn");


    return qmlItemNode().nodeInstance().hasAnchor(propertyName);
}

bool QmlAnchors::instanceHasAnchors() const
{
    return instanceHasAnchor(AnchorLineLeft) ||
           instanceHasAnchor(AnchorLineRight) ||
           instanceHasAnchor(AnchorLineTop) ||
           instanceHasAnchor(AnchorLineBottom) ||
           instanceHasAnchor(AnchorLineHorizontalCenter) ||
           instanceHasAnchor(AnchorLineVerticalCenter) ||
            instanceHasAnchor(AnchorLineBaseline);
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

double QmlAnchors::instanceAnchorLine(AnchorLineType anchorLine) const
{
    switch (anchorLine) {
    case AnchorLineLeft: return instanceLeftAnchorLine();
    case AnchorLineTop: return instanceTopAnchorLine();
    case AnchorLineBottom: return instanceBottomAnchorLine();
    case AnchorLineRight: return instanceRightAnchorLine();
    case AnchorLineHorizontalCenter: return instanceHorizontalCenterAnchorLine();
    case AnchorLineVerticalCenter: return instanceVerticalCenterAnchorLine();
    default: return 0;
    }

    return 0.0;
}

void QmlAnchors::setMargin(AnchorLineType sourceAnchorLineType, double margin) const
{
    PropertyName propertyName = marginPropertyName(sourceAnchorLineType);
    qmlItemNode().setVariantProperty(propertyName, qRound(margin));
}

bool QmlAnchors::instanceHasMargin(AnchorLineType sourceAnchorLineType) const
{
    return !qIsNull(instanceMargin(sourceAnchorLineType));
}

static bool checkForHorizontalCycleRecusive(const QmlAnchors &anchors, QList<QmlItemNode> &visitedItems)
{
    if (!anchors.isValid())
        return false;

    visitedItems.append(anchors.qmlItemNode());
    if (anchors.instanceHasAnchor(AnchorLineLeft)) {
        AnchorLine leftAnchorLine = anchors.instanceAnchor(AnchorLineLeft);
        if (visitedItems.contains(leftAnchorLine.qmlItemNode()) || checkForHorizontalCycleRecusive(leftAnchorLine.qmlItemNode().anchors(), visitedItems))
            return true;
    }

    if (anchors.instanceHasAnchor(AnchorLineRight)) {
        AnchorLine rightAnchorLine = anchors.instanceAnchor(AnchorLineRight);
        if (visitedItems.contains(rightAnchorLine.qmlItemNode()) || checkForHorizontalCycleRecusive(rightAnchorLine.qmlItemNode().anchors(), visitedItems))
            return true;
    }

    if (anchors.instanceHasAnchor(AnchorLineHorizontalCenter)) {
        AnchorLine horizontalCenterAnchorLine = anchors.instanceAnchor(AnchorLineHorizontalCenter);
        if (visitedItems.contains(horizontalCenterAnchorLine.qmlItemNode()) || checkForHorizontalCycleRecusive(horizontalCenterAnchorLine.qmlItemNode().anchors(), visitedItems))
            return true;
    }

    return false;
}

static bool checkForVerticalCycleRecusive(const QmlAnchors &anchors, QList<QmlItemNode> &visitedItems)
{
    if (!anchors.isValid())
        return false;

    visitedItems.append(anchors.qmlItemNode());

    if (anchors.instanceHasAnchor(AnchorLineTop)) {
        AnchorLine topAnchorLine = anchors.instanceAnchor(AnchorLineTop);
        if (visitedItems.contains(topAnchorLine.qmlItemNode()) || checkForVerticalCycleRecusive(topAnchorLine.qmlItemNode().anchors(), visitedItems))
            return true;
    }

    if (anchors.instanceHasAnchor(AnchorLineBottom)) {
        AnchorLine bottomAnchorLine = anchors.instanceAnchor(AnchorLineBottom);
        if (visitedItems.contains(bottomAnchorLine.qmlItemNode()) || checkForVerticalCycleRecusive(bottomAnchorLine.qmlItemNode().anchors(), visitedItems))
            return true;
    }

    if (anchors.instanceHasAnchor(AnchorLineVerticalCenter)) {
        AnchorLine verticalCenterAnchorLine = anchors.instanceAnchor(AnchorLineVerticalCenter);
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

double QmlAnchors::instanceMargin(AnchorLineType sourceAnchorLineType) const
{
    return qmlItemNode().nodeInstance().property(marginPropertyName(sourceAnchorLineType)).toDouble();
}

void QmlAnchors::removeMargin(AnchorLineType sourceAnchorLineType)
{
    if (qmlItemNode().isInBaseState()) {
        PropertyName propertyName = marginPropertyName(sourceAnchorLineType);
        qmlItemNode().modelNode().removeProperty(propertyName);
    }
}

void QmlAnchors::removeMargins()
{
    RewriterTransaction transaction = qmlItemNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchors::removeMargins"));
    removeMargin(AnchorLineLeft);
    removeMargin(AnchorLineRight);
    removeMargin(AnchorLineTop);
    removeMargin(AnchorLineBottom);
    removeMargin(AnchorLineHorizontalCenter);
    removeMargin(AnchorLineVerticalCenter);
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

bool QmlAnchors::checkForCycle(AnchorLineType anchorLineTyp, const QmlItemNode &sourceItem) const
{
    if (anchorLineTyp & AnchorLineHorizontalMask)
        return checkForHorizontalCycle(sourceItem);
    else
        return checkForVerticalCycle(sourceItem);
}

} //QmlDesigner
