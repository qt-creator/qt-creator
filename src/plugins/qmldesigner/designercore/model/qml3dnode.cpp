// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qml3dnode.h"
#include "auxiliarydataproperties.h"
#include "bindingproperty.h"
#include "invalidmodelnodeexception.h"
#include "itemlibraryinfo.h"
#include "nodehints.h"
#include "nodelistproperty.h"
#include "qmlanchors.h"
#include "qmlchangeset.h"
#include "variantproperty.h"
#include <metainfo.h>

#include "plaintexteditmodifier.h"
#include "rewriterview.h"
#include "modelmerger.h"
#include "rewritingexception.h"

#include <QUrl>
#include <QPlainTextEdit>
#include <QFileInfo>
#include <QDir>

namespace QmlDesigner {

bool Qml3DNode::isValid() const
{
    return isValidQml3DNode(modelNode());
}

bool Qml3DNode::isValidQml3DNode(const ModelNode &modelNode)
{
    return isValidQmlObjectNode(modelNode) && (modelNode.metaInfo().isQtQuick3DNode());
}

bool Qml3DNode::isValidVisualRoot(const ModelNode &modelNode)
{
    return isValidQmlObjectNode(modelNode)
           && (modelNode.metaInfo().isQtQuick3DNode() || modelNode.metaInfo().isQtQuick3DMaterial());
}

bool Qml3DNode::handleEulerRotation(const PropertyName &name)
{
    if (isBlocked(name))
        return false;

    if (name.startsWith("eulerRotation"))
        handleEulerRotationSet();

    return true;
}

bool Qml3DNode::isBlocked(const PropertyName &propName) const
{
    if (modelNode().isValid() && propName.startsWith("eulerRotation"))
        return modelNode().auxiliaryDataWithDefault(rotBlockProperty).toBool();

    return false;
}

void Qml3DNode::handleEulerRotationSet()
{
    ModelNode node = modelNode();
    // The rotation property is quaternion, which is difficult to deal with for users, so QDS
    // only supports eulerRotation. Since having both on the same object isn't supported,
    // remove the rotation property if eulerRotation is set.
    if (node.isValid() && node.metaInfo().isQtQuick3DNode()) {
        if (!isInBaseState()) {
            QmlPropertyChanges changeSet(currentState().propertyChanges(node));
            Q_ASSERT(changeSet.isValid());
            node = changeSet.modelNode();
        }

        if (node.hasProperty("rotation")) {
            // We need to reset the eulerRotation values as removing rotation will zero them,
            // which is not desirable if the change only targets one of the xyz subproperties.
            // Get the eulerRotation value from instance, as they are not available in model.
            QVector3D eulerVec = instanceValue("eulerRotation").value<QVector3D>();
            node.removeProperty("rotation");
            if (qIsNaN(eulerVec.x()))
                eulerVec.setX(0.);
            if (qIsNaN(eulerVec.y()))
                eulerVec.setY(0.);
            if (qIsNaN(eulerVec.z()))
                eulerVec.setZ(0.);
            node.variantProperty("eulerRotation.x").setValue(eulerVec.x());
            node.variantProperty("eulerRotation.y").setValue(eulerVec.y());
            node.variantProperty("eulerRotation.z").setValue(eulerVec.z());
        }
    }
}

QList<ModelNode> toModelNodeList(const QList<Qml3DNode> &qmlVisualNodeList)
{
    QList<ModelNode> modelNodeList;

    for (const Qml3DNode &qml3DNode : qmlVisualNodeList)
        modelNodeList.append(qml3DNode.modelNode());

    return modelNodeList;
}

QList<Qml3DNode> toQml3DNodeList(const QList<ModelNode> &modelNodeList)
{
    QList<Qml3DNode> qml3DNodeList;

    for (const ModelNode &modelNode : modelNodeList) {
        if (Qml3DNode::isValidQml3DNode(modelNode))
            qml3DNodeList.append(modelNode);
    }

    return qml3DNodeList;
}

} //QmlDesigner
