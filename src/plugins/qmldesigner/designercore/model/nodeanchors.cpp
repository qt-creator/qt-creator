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

//#include "nodeanchors.h"
//
//#include <model.h>
//#include <modelnode.h>
//
//#include "internalnode_p.h"
//#include "internalnodeanchors.h"
//#include "internalnodestate.h"
//#include "invalidargumentexception.h"
//
//using namespace QmlDesigner::Internal;
//
//namespace QmlDesigner {
//
///*!
//\class QmlDesigner::NodeAnchors
//\ingroup CoreModel
//\brief NodeAnchors is a value holder for an anchor
//*/
//
//NodeAnchors::NodeAnchors(const NodeState &nodeState):
//        m_internalNode(nodeState.m_internalNode),
//        m_internalNodeState(nodeState.m_internalNodeState),
//        m_model(nodeState.m_model)
//{
//}
//
//NodeAnchors::~NodeAnchors()
//{
//}
//
//NodeAnchors::NodeAnchors(const NodeAnchors &other)
//    :m_internalNode(other.m_internalNode),
//    m_model(other.m_model)
//{
//
//}
//
//NodeAnchors::NodeAnchors(const Internal::InternalNodeStatePointer &internalNodeState, Model *model):
//        m_internalNode(internalNodeState->modelNode()),
//        m_internalNodeState(internalNodeState),
//        m_model(model)
//{
//}
//
//NodeAnchors &NodeAnchors::operator=(const NodeAnchors &other)
//{
//    m_internalNode = other.m_internalNode;
//    m_internalNodeState = other.m_internalNodeState;
//    m_model = other.m_model;
//
//    return *this;
//}
//
//ModelNode NodeAnchors::modelNode() const
//{
//    return ModelNode(m_internalNode, m_model.data());
//}
//
//bool NodeAnchors::isValid() const
//{
//    return m_internalNode->isValid()
//            && m_internalNodeState->isValid() &&
//            m_model;
//}
//
//NodeState NodeAnchors::nodeState() const
//{
//    return NodeState(m_internalNodeState, m_internalNode, m_model.data());
//}
//
//void NodeAnchors::setAnchor(AnchorLine::Type sourceAnchorLineType,
//                const ModelNode &targetModelNode,
//                AnchorLine::Type targetAnchorLineType)
//{
//    Q_ASSERT(m_internalNode->isValid());
//    Q_ASSERT(m_internalNodeState->isValid());
//    Q_ASSERT(modelNode().isValid());
//
//    m_model->setAnchor(AnchorLine(nodeState(), sourceAnchorLineType),
//                       AnchorLine(targetModelNode.baseNodeState(), targetAnchorLineType));
//}
//
//bool NodeAnchors::canAnchor(AnchorLine::Type sourceAnchorLineType,
//                            const ModelNode & targetModelNode,
//                            AnchorLine::Type targetAnchorLineType) const
//{
//    if (modelNode() == targetModelNode)
//        return false;
//
//    return InternalNodeAnchors(m_internalNodeState).canAnchor(sourceAnchorLineType, targetModelNode.baseNodeState().internalNodeState(), targetAnchorLineType);
//}
//
//bool NodeAnchors::canAnchor(const ModelNode & targetModelNode) const
//{
//    if (modelNode() == targetModelNode)
//        return false;
//
//    if (possibleAnchorLines(AnchorLine::Left, targetModelNode) != AnchorLine::NoAnchor)
//        return true;
//    else if (possibleAnchorLines(AnchorLine::Top, targetModelNode) != AnchorLine::NoAnchor)
//        return true;
//    else if (possibleAnchorLines(AnchorLine::Right, targetModelNode) != AnchorLine::NoAnchor)
//        return true;
//    else if (possibleAnchorLines(AnchorLine::Bottom, targetModelNode) != AnchorLine::NoAnchor)
//        return true;
//    else if (possibleAnchorLines(AnchorLine::HorizontalCenter, targetModelNode) != AnchorLine::NoAnchor)
//        return true;
//    else
//        return possibleAnchorLines(AnchorLine::VerticalCenter, targetModelNode) != AnchorLine::NoAnchor;
//}
//
//AnchorLine::Type NodeAnchors::possibleAnchorLines(AnchorLine::Type sourceAnchorLineType,
//                                                  const ModelNode & targetModelNode) const
//{
//    if (modelNode() == targetModelNode)
//        return AnchorLine::NoAnchor;
//
//    int anchorTypes = AnchorLine::NoAnchor;
//    const InternalNodeAnchors anchors(m_internalNodeState);
//
//    if (sourceAnchorLineType & AnchorLine::HorizontalMask) {
//        if (anchors.canAnchor(sourceAnchorLineType, targetModelNode.baseNodeState().internalNodeState(), AnchorLine::Left))
//            anchorTypes |= AnchorLine::Left;
//        if (anchors.canAnchor(sourceAnchorLineType, targetModelNode.baseNodeState().internalNodeState(), AnchorLine::Right))
//            anchorTypes |= AnchorLine::Right;
//        if (anchors.canAnchor(sourceAnchorLineType, targetModelNode.baseNodeState().internalNodeState(), AnchorLine::HorizontalCenter))
//            anchorTypes |= AnchorLine::HorizontalCenter;
//    } else if (sourceAnchorLineType & AnchorLine::VerticalMask) {
//        if (anchors.canAnchor(sourceAnchorLineType, targetModelNode.baseNodeState().internalNodeState(), AnchorLine::Top))
//            anchorTypes |= AnchorLine::Top;
//        if (anchors.canAnchor(sourceAnchorLineType, targetModelNode.baseNodeState().internalNodeState(), AnchorLine::Bottom))
//            anchorTypes |= AnchorLine::Bottom;
//        if (anchors.canAnchor(sourceAnchorLineType, targetModelNode.baseNodeState().internalNodeState(), AnchorLine::VerticalCenter))
//            anchorTypes |= AnchorLine::VerticalCenter;
//    }
//
//    return (AnchorLine::Type) anchorTypes;
//}
//
//AnchorLine NodeAnchors::localAnchor(AnchorLine::Type anchorLineType) const
//{
//    return InternalNodeAnchors(m_internalNodeState).anchor(anchorLineType);
//}
//
//AnchorLine NodeAnchors::anchor(AnchorLine::Type anchorLineType) const
//{
//    Internal::InternalNodeState::Pointer statePointer(m_internalNodeState);
//    AnchorLine anchorLine = InternalNodeAnchors(statePointer).anchor(anchorLineType);
//
//    while (!anchorLine.isValid() && statePointer->hasParentState()) {
//        statePointer = statePointer->parentState();
//        anchorLine = InternalNodeAnchors(statePointer).anchor(anchorLineType);
//    }
//
//    return anchorLine;
//}
//
//bool NodeAnchors::hasAnchor(AnchorLine::Type sourceAnchorLineType) const
//{
//    Internal::InternalNodeState::Pointer statePointer(m_internalNodeState);
//    InternalNodeAnchors internalNodeAnchors(statePointer);
//    while (!internalNodeAnchors.hasAnchor(sourceAnchorLineType) &&
//          statePointer->hasParentState()) {
//        statePointer = statePointer->parentState();
//        internalNodeAnchors = InternalNodeAnchors(statePointer);
//    }
//
//    return internalNodeAnchors.hasAnchor(sourceAnchorLineType);
//}
//
//void NodeAnchors::removeAnchor(AnchorLine::Type sourceAnchorLineType)
//{
//    if (hasLocalAnchor(sourceAnchorLineType))
//        m_model->removeAnchor(AnchorLine(nodeState(), sourceAnchorLineType));
//}
//
//void NodeAnchors::removeMargins()
//{
//    removeMargin(AnchorLine::Left);
//    removeMargin(AnchorLine::Right);
//    removeMargin(AnchorLine::Top);
//    removeMargin(AnchorLine::Bottom);
//    removeMargin(AnchorLine::HorizontalCenter);
//    removeMargin(AnchorLine::VerticalCenter);
//    removeMargin(AnchorLine::Baseline);
//}
//
//void NodeAnchors::removeAnchors()
//{
//    removeAnchor(AnchorLine::Left);
//    removeAnchor(AnchorLine::Right);
//    removeAnchor(AnchorLine::Top);
//    removeAnchor(AnchorLine::Bottom);
//    removeAnchor(AnchorLine::HorizontalCenter);
//    removeAnchor(AnchorLine::VerticalCenter);
//    removeAnchor(AnchorLine::Baseline);
//}
//
//bool NodeAnchors::hasLocalAnchor(AnchorLine::Type sourceAnchorLineType) const
//{
//    return InternalNodeAnchors(m_internalNodeState).hasAnchor(sourceAnchorLineType);
//}
//
//bool NodeAnchors::hasLocalAnchors() const
//{
//    return hasLocalAnchor(AnchorLine::Top) ||
//           hasLocalAnchor(AnchorLine::Bottom) ||
//           hasLocalAnchor(AnchorLine::Left) ||
//           hasLocalAnchor(AnchorLine::Right) ||
//           hasLocalAnchor(AnchorLine::VerticalCenter) ||
//           hasLocalAnchor(AnchorLine::HorizontalCenter) ||
//           hasLocalAnchor(AnchorLine::Baseline);
//}
//
//bool NodeAnchors::hasAnchors() const
//{
//    return hasAnchor(AnchorLine::Top) ||
//           hasAnchor(AnchorLine::Bottom) ||
//           hasAnchor(AnchorLine::Left) ||
//           hasAnchor(AnchorLine::Right) ||
//           hasAnchor(AnchorLine::VerticalCenter) ||
//           hasAnchor(AnchorLine::HorizontalCenter) ||
//           hasAnchor(AnchorLine::Baseline);
//}
//
//void NodeAnchors::setMargin(AnchorLine::Type sourceAnchorLineType, double margin) const
//{
//    m_model->setAnchorMargin(AnchorLine(nodeState(), sourceAnchorLineType), margin);
//}
//
//bool NodeAnchors::hasMargin(AnchorLine::Type sourceAnchorLineType) const
//{
//    return InternalNodeAnchors(m_internalNodeState).hasMargin(sourceAnchorLineType);
//}
//
//double NodeAnchors::localMargin(AnchorLine::Type sourceAnchorLineType) const
//{
//    return InternalNodeAnchors(m_internalNodeState).margin(sourceAnchorLineType);
//}
//
//double NodeAnchors::margin(AnchorLine::Type sourceAnchorLineType) const
//{
//    Internal::InternalNodeState::Pointer statePointer(m_internalNodeState);
//    InternalNodeAnchors internalNodeAnchors(statePointer);
//    while (!internalNodeAnchors.hasMargin(sourceAnchorLineType) &&
//          statePointer->hasParentState()) {
//        statePointer = statePointer->parentState();
//        internalNodeAnchors = InternalNodeAnchors(statePointer);
//    }
//
//    return internalNodeAnchors.margin(sourceAnchorLineType);
//}
//
//void NodeAnchors::removeMargin(AnchorLine::Type sourceAnchorLineType)
//{
//    m_model->removeAnchorMargin(AnchorLine(nodeState(), sourceAnchorLineType));
//
//}
//
//QDebug operator<<(QDebug debug, const NodeAnchors &anchors)
//{
//    debug.nospace() << "NodeAnchors(";
//    if (anchors.isValid()) {
//        if (anchors.hasAnchor(AnchorLine::Top))
//            debug << "top";
//        if (anchors.hasMargin(AnchorLine::Top))
//            debug.nospace() << "(" << anchors.margin(AnchorLine::Top) << ")";
//        if (anchors.hasAnchor(AnchorLine::Bottom))
//            debug << "bottom";
//        if (anchors.hasMargin(AnchorLine::Bottom))
//            debug.nospace() << "(" << anchors.margin(AnchorLine::Bottom) << ")";
//        if (anchors.hasAnchor(AnchorLine::Left))
//            debug << "left";
//        if (anchors.hasMargin(AnchorLine::Left))
//            debug.nospace() << "(" << anchors.margin(AnchorLine::Left) << ")";
//        if (anchors.hasAnchor(AnchorLine::Right))
//            debug << "right";
//        if (anchors.hasMargin(AnchorLine::Right))
//            debug.nospace() << "(" << anchors.margin(AnchorLine::Right) << ")";
//        if (anchors.hasAnchor(AnchorLine::VerticalCenter))
//            debug << "verticalCenter";
//        if (anchors.hasMargin(AnchorLine::VerticalCenter))
//            debug.nospace() << "(" << anchors.margin(AnchorLine::VerticalCenter) << ")";
//        if (anchors.hasAnchor(AnchorLine::HorizontalCenter))
//            debug << "horizontalCenter";
//        if (anchors.hasMargin(AnchorLine::HorizontalCenter))
//            debug.nospace() << "(" << anchors.margin(AnchorLine::HorizontalCenter) << ")";
//        if (anchors.hasAnchor(AnchorLine::Baseline))
//            debug << "baseline";
//        if (anchors.hasMargin(AnchorLine::Baseline))
//            debug.nospace() << "(" << anchors.margin(AnchorLine::Baseline) << ")";
//    } else {
//        debug.nospace() << "invalid";
//    }
//
//    debug.nospace() << ")";
//
//    return debug.space();
//}
//
//QTextStream& operator<<(QTextStream &stream, const NodeAnchors &anchors)
//{
//    stream << "NodeAnchors(";
//    if (anchors.isValid()) {
//        if (anchors.hasAnchor(AnchorLine::Top))
//            stream << "top";
//        if (anchors.hasMargin(AnchorLine::Top))
//            stream << "(" << anchors.margin(AnchorLine::Top) << ")";
//        if (anchors.hasAnchor(AnchorLine::Bottom))
//            stream << "bottom";
//        if (anchors.hasMargin(AnchorLine::Bottom))
//            stream << "(" << anchors.margin(AnchorLine::Bottom) << ")";
//        if (anchors.hasAnchor(AnchorLine::Left))
//            stream << "left";
//        if (anchors.hasMargin(AnchorLine::Left))
//            stream << "(" << anchors.margin(AnchorLine::Left) << ")";
//        if (anchors.hasAnchor(AnchorLine::Right))
//            stream << "right";
//        if (anchors.hasMargin(AnchorLine::Right))
//            stream << "(" << anchors.margin(AnchorLine::Right) << ")";
//        if (anchors.hasAnchor(AnchorLine::VerticalCenter))
//            stream << "verticalCenter";
//        if (anchors.hasMargin(AnchorLine::VerticalCenter))
//            stream << "(" << anchors.margin(AnchorLine::VerticalCenter) << ")";
//        if (anchors.hasAnchor(AnchorLine::HorizontalCenter))
//            stream << "horizontalCenter";
//        if (anchors.hasMargin(AnchorLine::HorizontalCenter))
//            stream << "(" << anchors.margin(AnchorLine::HorizontalCenter) << ")";
//        if (anchors.hasAnchor(AnchorLine::Baseline))
//            stream << "baseline";
//        if (anchors.hasMargin(AnchorLine::Baseline))
//            stream << "(" << anchors.margin(AnchorLine::Baseline) << ")";
//    } else {
//        stream << "invalid";
//    }
//
//    stream << ")";
//
//    return stream;
//}
//
//
//} // namespace QmlDesigner
