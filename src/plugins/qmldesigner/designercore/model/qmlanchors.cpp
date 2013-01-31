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
#include "variantproperty.h"
#include "bindingproperty.h"
#include "nodeabstractproperty.h"
#include "nodeinstance.h"
#include "rewritertransaction.h"
#include "qmlmodelview.h"
#include "mathutils.h"

namespace QmlDesigner {


static QString lineTypeToString(AnchorLine::Type lineType)
{
    QString typeString;

    switch (lineType) {
        case AnchorLine::Left:             return QLatin1String("left");
        case AnchorLine::Top:              return QLatin1String("top");
        case AnchorLine::Right:            return QLatin1String("right");
        case AnchorLine::Bottom:           return QLatin1String("bottom");
        case AnchorLine::HorizontalCenter: return QLatin1String("horizontalCenter");
        case AnchorLine::VerticalCenter:   return QLatin1String("verticalCenter");
        case AnchorLine::Baseline:         return QLatin1String("baseline");
        case AnchorLine::Fill:             return QLatin1String("fill");
        case AnchorLine::Center:           return QLatin1String("centerIn");
        default:                           return QString();
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

static QString marginPropertyName(AnchorLine::Type lineType)
{
    switch (lineType) {
        case AnchorLine::Left:             return QLatin1String("anchors.leftMargin");
        case AnchorLine::Top:              return QLatin1String("anchors.topMargin");
        case AnchorLine::Right:            return QLatin1String("anchors.rightMargin");
        case AnchorLine::Bottom:           return QLatin1String("anchors.bottomMargin");
        case AnchorLine::HorizontalCenter: return QLatin1String("anchors.horizontalCenterOffset");
        case AnchorLine::VerticalCenter:   return QLatin1String("anchors.verticalCenterOffset");
        default:                           return QString();
    }
}

static QString anchorPropertyName(AnchorLine::Type lineType)
{
    const QString typeString = lineTypeToString(lineType);

    if (typeString.isEmpty())
        return QString();
    else
        return QString("anchors.%1").arg(typeString);
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

void QmlAnchors::beautify()
{
    return;
    if ((instanceHasAnchor(AnchorLine::Left) &&
         instanceHasAnchor(AnchorLine::Right) &&
         instanceHasAnchor(AnchorLine::Top) &&
         instanceHasAnchor(AnchorLine::Bottom)) &&

        (instanceAnchor(AnchorLine::Left).type() == AnchorLine::Left &&
         instanceAnchor(AnchorLine::Right).type() == AnchorLine::Right &&
         instanceAnchor(AnchorLine::Top).type() == AnchorLine::Top &&
         instanceAnchor(AnchorLine::Bottom).type() == AnchorLine::Bottom) &&

        (instanceAnchor(AnchorLine::Left).qmlItemNode() ==
         instanceAnchor(AnchorLine::Right).qmlItemNode() &&
         instanceAnchor(AnchorLine::Top).qmlItemNode() ==
         instanceAnchor(AnchorLine::Bottom).qmlItemNode()) &&
        (instanceAnchor(AnchorLine::Left).qmlItemNode() ==
         instanceAnchor(AnchorLine::Bottom).qmlItemNode())) {

        if (instanceHasAnchor(AnchorLine::Fill))
            return; //avoid recursion

        QmlItemNode targetNode(instanceAnchor(AnchorLine::Left).qmlItemNode());
        removeAnchors();
        setAnchor(AnchorLine::Fill, targetNode, AnchorLine::Fill);
    }

    if ((instanceHasAnchor(AnchorLine::VerticalCenter) &&
         instanceHasAnchor(AnchorLine::HorizontalCenter)) &&

        (instanceAnchor(AnchorLine::VerticalCenter).type() == AnchorLine::VerticalCenter &&
         instanceAnchor(AnchorLine::HorizontalCenter).type() == AnchorLine::HorizontalCenter) &&

        (instanceAnchor(AnchorLine::VerticalCenter).qmlItemNode() ==
         instanceAnchor(AnchorLine::HorizontalCenter).qmlItemNode())) {

        if (instanceHasAnchor(AnchorLine::Center))
            return; //avoid recursion

        QmlItemNode targetNode(instanceAnchor(AnchorLine::VerticalCenter).qmlItemNode());
        removeAnchors();
        setAnchor(AnchorLine::Center, targetNode, AnchorLine::Center);
    }
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

        const QString propertyName = anchorPropertyName(sourceAnchorLine);
        QString targetExpression = targetQmlItemNode.modelNode().validId();
        if (targetQmlItemNode.modelNode() == qmlItemNode().modelNode().parentProperty().parentModelNode())
            targetExpression = "parent";
        if (sourceAnchorLine != AnchorLine::Center && sourceAnchorLine != AnchorLine::Fill)
            targetExpression = targetExpression + QLatin1Char('.') + lineTypeToString(targetAnchorLine);
        qmlItemNode().modelNode().bindingProperty(propertyName).setExpression(targetExpression);
    }
    beautify();
}

bool detectHorizontalCycle(const ModelNode &node, QList<ModelNode> knownNodeList)
{
    if (knownNodeList.contains(node))
        return true;

    knownNodeList.append(node);

    static QStringList validAnchorLines(QStringList() << "right" << "left" << "horizontalCenter");
    static QStringList anchorNames(QStringList() << "anchors.right" << "anchors.left" << "anchors.horizontalCenter");

    foreach (const QString &anchorName, anchorNames) {
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

    static QStringList anchorShortcutNames(QStringList() << "anchors.fill" << "anchors.centerIn");
    foreach (const QString &anchorName, anchorShortcutNames) {
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

    static QStringList validAnchorLines(QStringList() << "top" << "bottom" << "verticalCenter" << "baseline");
    static QStringList anchorNames(QStringList() << "anchors.top" << "anchors.bottom" << "anchors.verticalCenter" << "anchors.baseline");

    foreach (const QString &anchorName, anchorNames) {
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

    static QStringList anchorShortcutNames(QStringList() << "anchors.fill" << "anchors.centerIn");
    foreach (const QString &anchorName, anchorShortcutNames) {
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
    QPair<QString, qint32> targetAnchorLinePair;
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
        const QString propertyName = anchorPropertyName(sourceAnchorLine);
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
    const QString propertyName = anchorPropertyName(sourceAnchorLine);

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

void QmlAnchors::setMargin(AnchorLine::Type sourceAnchorLineType, double margin) const
{
    QString propertyName = marginPropertyName(sourceAnchorLineType);
    qmlItemNode().setVariantProperty(propertyName, qRound(margin));
}

bool QmlAnchors::instanceHasMargin(AnchorLine::Type sourceAnchorLineType) const
{
    return !qIsNull(instanceMargin(sourceAnchorLineType));
}

double QmlAnchors::instanceMargin(AnchorLine::Type sourceAnchorLineType) const
{
    return qmlItemNode().nodeInstance().property(marginPropertyName(sourceAnchorLineType)).toDouble();
}

void QmlAnchors::removeMargin(AnchorLine::Type sourceAnchorLineType)
{
    if (qmlItemNode().isInBaseState()) {
        QString propertyName = marginPropertyName(sourceAnchorLineType);
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

} //QmlDesigner
