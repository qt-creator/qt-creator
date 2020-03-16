/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "aligndistribute.h"

#include <nodeabstractproperty.h>
#include <qmldesignerplugin.h>
#include <qmlmodelnodeproxy.h>

#include <modelnode.h>
#include <variantproperty.h>

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/qtcassert.h>

#include <cmath>

namespace QmlDesigner {

AlignDistribute::AlignDistribute(QObject *parent)
    : QObject(parent)
{}

bool AlignDistribute::multiSelection() const
{
    if (!m_qmlObjectNode.isValid())
        return false;

    return m_qmlObjectNode.view()->selectedModelNodes().count() > 1;
}

bool AlignDistribute::selectionHasAnchors() const
{
    if (!m_qmlObjectNode.isValid())
        return true;

    const auto selectionContext = SelectionContext(m_qmlObjectNode.view());
    for (const ModelNode &modelNode : selectionContext.selectedModelNodes()) {
        QmlItemNode itemNode(modelNode);
        if (itemNode.instanceHasAnchors())
            return true;
    }
    return false;
}

bool AlignDistribute::selectionExclusivlyItems() const
{
    if (!m_qmlObjectNode.isValid())
        return false;

    const auto selectionContext = SelectionContext(m_qmlObjectNode.view());
    for (const ModelNode &modelNode : selectionContext.selectedModelNodes()) {
        if (!QmlItemNode::isValidQmlItemNode(modelNode))
            return false;
    }
    return true;
}

bool AlignDistribute::selectionContainsRootItem() const
{
    if (!m_qmlObjectNode.isValid())
        return true;

    const auto selectionContext = SelectionContext(m_qmlObjectNode.view());
    for (const ModelNode &modelNode : selectionContext.selectedModelNodes()) {
        QmlItemNode itemNode(modelNode);
        if (itemNode.isRootNode())
            return true;
    }
    return false;
}

void AlignDistribute::setModelNodeBackend(const QVariant &modelNodeBackend)
{
    auto modelNodeBackendObject = modelNodeBackend.value<QObject *>();
    const auto backendObjectCasted = qobject_cast<const QmlModelNodeProxy *>(modelNodeBackendObject);

    if (backendObjectCasted)
        m_qmlObjectNode = backendObjectCasted->qmlObjectNode();

    emit modelNodeBackendChanged();
}

// The purpose of this function is to suppress the following warning:
// Warning: Property declaration modelNodeBackendProperty has no READ accessor
// function or associated MEMBER variable. The property will be invalid.
QVariant AlignDistribute::modelNodeBackend() const
{
    return {};
}

void AlignDistribute::registerDeclarativeType()
{
    qmlRegisterType<AlignDistribute>("HelperWidgets", 2, 0, "AlignDistribute");
}

// utility functions

inline qreal width(const QmlItemNode &qmlItemNode)
{
    return qmlItemNode.instanceSize().width();
}
inline qreal halfWidth(const QmlItemNode &qmlItemNode)
{
    return qmlItemNode.instanceSize().width() * 0.5;
}
inline qreal height(const QmlItemNode &qmlItemNode)
{
    return qmlItemNode.instanceSize().height();
}
inline qreal halfHeight(const QmlItemNode &qmlItemNode)
{
    return qmlItemNode.instanceSize().height() * 0.5;
}
inline qreal left(const QmlItemNode &qmlItemNode)
{
    return qmlItemNode.instanceScenePosition().x();
}
inline qreal centerHorizontal(const QmlItemNode &qmlItemNode)
{
    return qmlItemNode.instanceScenePosition().x() + halfWidth(qmlItemNode);
}
inline qreal right(const QmlItemNode &qmlItemNode)
{
    return qmlItemNode.instanceScenePosition().x() + width(qmlItemNode);
}
inline qreal top(const QmlItemNode &qmlItemNode)
{
    return qmlItemNode.instanceScenePosition().y();
}
inline qreal centerVertical(const QmlItemNode &qmlItemNode)
{
    return qmlItemNode.instanceScenePosition().y() + halfHeight(qmlItemNode);
}
inline qreal bottom(const QmlItemNode &qmlItemNode)
{
    return qmlItemNode.instanceScenePosition().y() + height(qmlItemNode);
}

bool compareByLeft(const ModelNode &node1, const ModelNode &node2)
{
    const QmlItemNode itemNode1 = QmlItemNode(node1);
    const QmlItemNode itemNode2 = QmlItemNode(node2);
    if (itemNode1.isValid() && itemNode2.isValid())
        return left(itemNode1) < left(itemNode2);
    return false;
}

bool compareByCenterH(const ModelNode &node1, const ModelNode &node2)
{
    const QmlItemNode itemNode1 = QmlItemNode(node1);
    const QmlItemNode itemNode2 = QmlItemNode(node2);
    if (itemNode1.isValid() && itemNode2.isValid())
        return centerHorizontal(itemNode1) < centerHorizontal(itemNode2);
    return false;
}

bool compareByRight(const ModelNode &node1, const ModelNode &node2)
{
    const QmlItemNode itemNode1 = QmlItemNode(node1);
    const QmlItemNode itemNode2 = QmlItemNode(node2);
    if (itemNode1.isValid() && itemNode2.isValid())
        return right(itemNode1) < right(itemNode2);
    return false;
}

bool compareByTop(const ModelNode &node1, const ModelNode &node2)
{
    const QmlItemNode itemNode1 = QmlItemNode(node1);
    const QmlItemNode itemNode2 = QmlItemNode(node2);
    if (itemNode1.isValid() && itemNode2.isValid())
        return top(itemNode1) < top(itemNode2);
    return false;
}

bool compareByCenterV(const ModelNode &node1, const ModelNode &node2)
{
    const QmlItemNode itemNode1 = QmlItemNode(node1);
    const QmlItemNode itemNode2 = QmlItemNode(node2);
    if (itemNode1.isValid() && itemNode2.isValid())
        return centerVertical(itemNode1) < centerVertical(itemNode2);
    return false;
}

bool compareByBottom(const ModelNode &node1, const ModelNode &node2)
{
    const QmlItemNode itemNode1 = QmlItemNode(node1);
    const QmlItemNode itemNode2 = QmlItemNode(node2);
    if (itemNode1.isValid() && itemNode2.isValid())
        return bottom(itemNode1) < bottom(itemNode2);
    return false;
}

unsigned getDepth(const ModelNode &node)
{
    if (node.isRootNode())
        return 0;

    return 1 + getDepth(node.parentProperty().parentModelNode());
}

bool compareByDepth(const ModelNode &node1, const ModelNode &node2)
{
    if (node1.isValid() && node2.isValid())
        return getDepth(node1) < getDepth(node2);
    return false;
}

static inline QRectF getBoundingRect(const QList<ModelNode> &modelNodeList)
{
    QRectF boundingRect;
    for (const ModelNode &modelNode : modelNodeList) {
        if (QmlItemNode::isValidQmlItemNode(modelNode)) {
            const QmlItemNode qmlItemNode(modelNode);
            boundingRect = boundingRect.united(qmlItemNode.instanceSceneBoundingRect());
        }
    }
    return boundingRect;
}

static inline QSizeF getSummedSize(const QList<ModelNode> &modelNodeList)
{
    QSizeF summedSize = QSizeF(0, 0);
    for (const ModelNode &modelNode : modelNodeList) {
        if (QmlItemNode::isValidQmlItemNode(modelNode)) {
            const QmlItemNode qmlItemNode(modelNode);
            summedSize += qmlItemNode.instanceSize();
        }
    }
    return summedSize;
}

static inline qreal getInstanceSceneX(const QmlItemNode &qmlItemNode)
{
    const qreal x = qmlItemNode.modelValue("x").toReal();
    if (qmlItemNode.hasInstanceParentItem())
        return x + getInstanceSceneX(qmlItemNode.instanceParentItem());
    return x;
}

static inline qreal getInstanceSceneY(const QmlItemNode &qmlItemNode)
{
    const qreal y = qmlItemNode.modelValue("y").toReal();
    if (qmlItemNode.hasInstanceParentItem())
        return y + getInstanceSceneY(qmlItemNode.instanceParentItem());
    return y;
}

// utility functions

void AlignDistribute::alignObjects(Target target, AlignTo alignTo, const QString &keyObject)
{
    QTC_ASSERT(m_qmlObjectNode.isValid(), return );

    const auto selectionContext = SelectionContext(m_qmlObjectNode.view());
    if (selectionContext.selectedModelNodes().empty())
        return;

    AbstractView *view = selectionContext.view();
    QList<ModelNode> selectedNodes = selectionContext.selectedModelNodes();
    QRectF boundingRect;
    QmlItemNode keyObjectQmlItemNode;

    switch (alignTo) {
    case AlignTo::Selection: {
        boundingRect = getBoundingRect(selectedNodes);
        break;
    }
    case AlignTo::Root: {
        const QmlItemNode rootQmlItemNode(selectionContext.view()->rootModelNode());
        boundingRect = rootQmlItemNode.instanceSceneBoundingRect();
        break;
    }
    case AlignTo::KeyObject: {
        if (!view->hasId(keyObject))
            return;

        const auto keyObjectModelNode = view->modelNodeForId(keyObject);
        keyObjectQmlItemNode = keyObjectModelNode;
        boundingRect = keyObjectQmlItemNode.instanceSceneBoundingRect();
        break;
    }
    }

    Utils::sort(selectedNodes, compareByDepth);

    const QByteArray operationName = "align" + QVariant::fromValue(target).toByteArray();
    auto alignPosition =
        [](Target target, const QRectF &boundingRect, const QmlItemNode &qmlItemNode) {
            switch (target) {
            case Target::Left:
                return boundingRect.left();
            case Target::CenterH:
                return boundingRect.center().x() - halfWidth(qmlItemNode);
            case Target::Right:
                return boundingRect.right() - width(qmlItemNode);
            case Target::Top:
                return boundingRect.top();
            case Target::CenterV:
                return boundingRect.center().y() - halfHeight(qmlItemNode);
            case Target::Bottom:
                return boundingRect.bottom() - height(qmlItemNode);
            }
            return 0.0;
        };

    view->executeInTransaction("DesignerActionManager|" + operationName, [&]() {
        for (const ModelNode &modelNode : selectedNodes) {
            QTC_ASSERT(!modelNode.isRootNode(), continue);
            if (QmlItemNode::isValidQmlItemNode(modelNode)) {
                QmlItemNode qmlItemNode(modelNode);
                qreal myPos;
                qreal parentPos;
                QByteArray propertyName;
                switch (getDimension(target)) {
                case Dimension::X: {
                    myPos = qmlItemNode.instanceScenePosition().x();
                    parentPos = getInstanceSceneX(qmlItemNode.instanceParentItem());
                    propertyName = "x";
                    break;
                }
                case Dimension::Y: {
                    myPos = qmlItemNode.instanceScenePosition().y();
                    parentPos = getInstanceSceneY(qmlItemNode.instanceParentItem());
                    propertyName = "y";
                    break;
                }
                }
                if (alignTo == AlignTo::KeyObject && qmlItemNode == keyObjectQmlItemNode)
                    qmlItemNode.setVariantProperty(propertyName, myPos - parentPos);
                else
                    qmlItemNode.setVariantProperty(propertyName,
                                                   qRound(alignPosition(target,
                                                                        boundingRect,
                                                                        qmlItemNode))
                                                       - parentPos);
            }
        }
    });
}

void AlignDistribute::distributeObjects(Target target, AlignTo alignTo, const QString &keyObject)
{
    QTC_ASSERT(m_qmlObjectNode.isValid(), return );

    const auto selectionContext = SelectionContext(m_qmlObjectNode.view());
    if (selectionContext.selectedModelNodes().empty())
        return;

    AbstractView *view = selectionContext.view();
    QList<ModelNode> selectedNodes = selectionContext.selectedModelNodes();
    QRectF boundingRect;

    switch (alignTo) {
    case AlignDistribute::AlignTo::Selection: {
        boundingRect = getBoundingRect(selectedNodes);
        break;
    }
    case AlignDistribute::AlignTo::Root: {
        const QmlItemNode rootQmlItemNode(selectionContext.view()->rootModelNode());
        boundingRect = rootQmlItemNode.instanceSceneBoundingRect();
        break;
    }
    case AlignDistribute::AlignTo::KeyObject: {
        if (!view->hasId(keyObject))
            return;

        // Remove key object from selected nodes list and set bounding box according to key object.
        const auto keyObjectModelNode = view->modelNodeForId(keyObject);
        selectedNodes.removeOne(keyObjectModelNode);
        const QmlItemNode keyObjectQmlItemNode(keyObjectModelNode);
        boundingRect = keyObjectQmlItemNode.instanceSceneBoundingRect();
        break;
    }
    }

    Utils::sort(selectedNodes, getCompareFunction(target));

    auto getMargins = [](Target target, const QList<ModelNode> &nodes) {
        switch (target) {
        case Target::Left:
            return QMarginsF(0, 0, width(QmlItemNode(nodes.last())), 0);
        case Target::CenterH:
            return QMarginsF(halfWidth(QmlItemNode(nodes.first())),
                             0,
                             halfWidth(QmlItemNode(nodes.last())),
                             0);
        case Target::Right:
            return QMarginsF(width(QmlItemNode(nodes.first())), 0, 0, 0);
        case Target::Top:
            return QMarginsF(0, 0, 0, height(QmlItemNode(nodes.last())));
        case Target::CenterV:
            return QMarginsF(0,
                             halfHeight(QmlItemNode(nodes.first())),
                             0,
                             halfHeight(QmlItemNode(nodes.last())));
        case Target::Bottom:
            return QMarginsF(0, height(QmlItemNode(nodes.first())), 0, 0);
        }
        return QMarginsF();
    };

    boundingRect -= getMargins(target, selectedNodes);

    auto distributePosition = [](Target target, const QmlItemNode &node) {
        switch (target) {
        case Target::Left:
            return 0.0;
        case Target::CenterH:
            return halfWidth(node);
        case Target::Right:
            return width(node);
        case Target::Top:
            return 0.0;
        case Target::CenterV:
            return halfHeight(node);
        case Target::Bottom:
            return height(node);
        }
        return 0.0;
    };

    QPointF position = boundingRect.topLeft();
    const qreal equidistant = (getDimension(target) == Dimension::X)
                                  ? boundingRect.width() / (selectedNodes.size() - 1)
                                  : boundingRect.height() / (selectedNodes.size() - 1);
    qreal tmp;
    if (std::modf(equidistant, &tmp) != 0.0) {
        if (!executePixelPerfectDialog())
            return;
    }

    for (const ModelNode &modelNode : selectedNodes) {
        if (QmlItemNode::isValidQmlItemNode(modelNode)) {
            QmlItemNode qmlItemNode(modelNode);
            qreal currentPosition;
            if (getDimension(target) == Dimension::X) {
                currentPosition = position.x();
                position.rx() += equidistant;
            } else {
                currentPosition = position.y();
                position.ry() += equidistant;
            }
            modelNode.setAuxiliaryData("tmp",
                                       qRound(currentPosition
                                              - distributePosition(target, qmlItemNode)));
        }
    }

    // Append key object to selected nodes list again. This needs to be done, so relative distribution
    // will also work on nested item targets. Otherwise change of parents position will move the inner
    // objects too. To compensate for that, the absolute parent position needs to be substracted.
    if (alignTo == AlignTo::KeyObject) {
        if (!view->hasId(keyObject))
            return;

        const auto keyObjectModelNode = view->modelNodeForId(keyObject);
        const QmlItemNode keyObjectQmlItemNode(keyObjectModelNode);
        const auto scenePosition = keyObjectQmlItemNode.instanceScenePosition();
        keyObjectModelNode.setAuxiliaryData("tmp",
                                            (getDimension(target) == Dimension::X
                                                 ? scenePosition.x()
                                                 : scenePosition.y()));
        selectedNodes.append(keyObjectModelNode);
    }

    Utils::sort(selectedNodes, compareByDepth);

    const QByteArray operationName = "distribute" + QVariant::fromValue(target).toByteArray();

    view->executeInTransaction("DesignerActionManager|" + operationName, [&]() {
        for (const ModelNode &modelNode : selectedNodes) {
            QTC_ASSERT(!modelNode.isRootNode(), continue);
            if (QmlItemNode::isValidQmlItemNode(modelNode)) {
                QmlItemNode qmlItemNode(modelNode);
                qreal parentPosition;
                QByteArray propertyName;
                switch (getDimension(target)) {
                case Dimension::X: {
                    parentPosition = getInstanceSceneX(qmlItemNode.instanceParentItem());
                    propertyName = "x";
                    break;
                }
                case Dimension::Y: {
                    parentPosition = getInstanceSceneY(qmlItemNode.instanceParentItem());
                    propertyName = "y";
                    break;
                }
                }
                qmlItemNode.setVariantProperty(propertyName,
                                               modelNode.auxiliaryData("tmp").toReal()
                                                   - parentPosition);
                modelNode.removeAuxiliaryData("tmp");
            }
        }
    });
}

void AlignDistribute::distributeSpacing(Dimension dimension,
                                        AlignTo alignTo,
                                        const QString &keyObject,
                                        const qreal &distance,
                                        DistributeOrigin distributeOrigin)
{
    QTC_ASSERT(m_qmlObjectNode.isValid(), return );

    const auto selectionContext = SelectionContext(m_qmlObjectNode.view());
    if (selectionContext.selectedModelNodes().empty())
        return;

    AbstractView *view = selectionContext.view();
    QList<ModelNode> selectedNodes = selectionContext.selectedModelNodes();
    QRectF boundingRect;

    switch (alignTo) {
    case AlignDistribute::AlignTo::Selection: {
        boundingRect = getBoundingRect(selectedNodes);
        break;
    }
    case AlignDistribute::AlignTo::Root: {
        const QmlItemNode rootQmlItemNode(selectionContext.view()->rootModelNode());
        boundingRect = rootQmlItemNode.instanceSceneBoundingRect();
        break;
    }
    case AlignDistribute::AlignTo::KeyObject: {
        if (!view->hasId(keyObject))
            return;

        // Remove key object from selected nodes list and set bounding box according to key object.
        const auto keyObjectModelNode = view->modelNodeForId(keyObject);
        selectedNodes.removeOne(keyObjectModelNode);
        const QmlItemNode keyObjectQmlItemNode(keyObjectModelNode);
        boundingRect = keyObjectQmlItemNode.instanceSceneBoundingRect();
        break;
    }
    }

    Utils::sort(selectedNodes, (dimension == Dimension::X) ? compareByCenterH : compareByCenterV);

    // Calculate the space between the items and set a proper start position for the different
    // distribution directions/origins.
    QPointF position = boundingRect.topLeft();
    const QSizeF summedSize = getSummedSize(selectedNodes);
    qreal equidistant = distance;

    if (distributeOrigin == DistributeOrigin::None) {
        const qreal lengthDifference = (dimension == Dimension::X)
                                           ? (boundingRect.width() - summedSize.width())
                                           : (boundingRect.height() - summedSize.height());
        equidistant = lengthDifference / (selectedNodes.size() - 1);
        qreal tmp;
        if (std::modf(equidistant, &tmp) != 0.0) {
            if (!executePixelPerfectDialog())
                return;
        }
    } else if (distributeOrigin == DistributeOrigin::Center
               || distributeOrigin == DistributeOrigin::BottomRight) {
        const qreal multiplier = (distributeOrigin == DistributeOrigin::Center) ? 0.5 : 1.0;
        if (dimension == Dimension::X) {
            const qreal totalLength = summedSize.width() + (distance * (selectedNodes.size() - 1));
            position.rx() -= (totalLength - boundingRect.width()) * multiplier;
        } else {
            const qreal totalLength = summedSize.height() + (distance * (selectedNodes.size() - 1));
            position.ry() -= (totalLength - boundingRect.height()) * multiplier;
        }
    }

    for (const ModelNode &modelNode : selectedNodes) {
        if (QmlItemNode::isValidQmlItemNode(modelNode)) {
            const QmlItemNode qmlItemNode(modelNode);
            qreal currentPosition;
            if (dimension == Dimension::X) {
                currentPosition = position.x();
                position.rx() += width(qmlItemNode) + equidistant;
            } else {
                currentPosition = position.y();
                position.ry() += height(qmlItemNode) + equidistant;
            }
            modelNode.setAuxiliaryData("tmp", qRound(currentPosition));
        }
    }

    // Append key object to selected nodes list again. This needs to be done, so relative distribution
    // will also work on nested item targets. Otherwise change of parents position will move the inner
    // objects too. To compensate for that, the absolute parent position needs to be substracted.
    if (alignTo == AlignTo::KeyObject) {
        if (!view->hasId(keyObject))
            return;

        const auto keyObjectModelNode = view->modelNodeForId(keyObject);
        const QmlItemNode keyObjectQmlItemNode(keyObjectModelNode);
        const auto scenePosition = keyObjectQmlItemNode.instanceScenePosition();
        keyObjectModelNode.setAuxiliaryData("tmp",
                                            (dimension == Dimension::X ? scenePosition.x()
                                                                       : scenePosition.y()));
        selectedNodes.append(keyObjectModelNode);
    }

    Utils::sort(selectedNodes, compareByDepth);

    const QByteArray operationName = (dimension == Dimension::X) ? "distributeSpacingHorizontal"
                                                                 : "distributeSpacingVertical";

    view->executeInTransaction("DesignerActionManager|" + operationName, [&]() {
        for (const ModelNode &modelNode : selectedNodes) {
            QTC_ASSERT(!modelNode.isRootNode(), continue);
            if (QmlItemNode::isValidQmlItemNode(modelNode)) {
                QmlItemNode qmlItemNode(modelNode);
                qreal parentPos;
                QByteArray propertyName;
                switch (dimension) {
                case Dimension::X: {
                    parentPos = getInstanceSceneX(qmlItemNode.instanceParentItem());
                    propertyName = "x";
                    break;
                }
                case Dimension::Y: {
                    parentPos = getInstanceSceneY(qmlItemNode.instanceParentItem());
                    propertyName = "y";
                    break;
                }
                }
                qmlItemNode.setVariantProperty(propertyName,
                                               modelNode.auxiliaryData("tmp").toReal() - parentPos);
                modelNode.removeAuxiliaryData("tmp");
            }
        }
    });
}

AlignDistribute::CompareFunction AlignDistribute::getCompareFunction(Target target) const
{
    static const std::map<Target, CompareFunction> cmpMap = {{Target::Left, compareByLeft},
                                                             {Target::CenterH, compareByCenterH},
                                                             {Target::Right, compareByRight},
                                                             {Target::Top, compareByTop},
                                                             {Target::CenterV, compareByCenterV},
                                                             {Target::Bottom, compareByBottom}};
    return cmpMap.at(target);
}

AlignDistribute::Dimension AlignDistribute::getDimension(Target target) const
{
    switch (target) {
    case Target::Left:
    case Target::CenterH:
    case Target::Right: {
        return Dimension::X;
    }
    case Target::Top:
    case Target::CenterV:
    case Target::Bottom: {
        return Dimension::Y;
    }
    }
    return Dimension::X;
}

bool AlignDistribute::executePixelPerfectDialog() const
{
    QDialogButtonBox::StandardButton pressed = Utils::CheckableMessageBox::doNotAskAgainQuestion(
        Core::ICore::dialogParent(),
        tr("Cannot Distribute Perfectly"),
        tr("These objects cannot be distributed to equal pixel values. "
           "Do you want to distribute to the nearest possible values?"),
        Core::ICore::settings(),
        "WarnAboutPixelPerfectDistribution");
    return (pressed == QDialogButtonBox::Yes) ? true : false;
}

} // namespace QmlDesigner
