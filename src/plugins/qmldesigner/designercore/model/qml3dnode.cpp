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

#include "qml3dnode.h"
#include <metainfo.h>
#include "qmlchangeset.h"
#include "nodelistproperty.h"
#include "nodehints.h"
#include "variantproperty.h"
#include "bindingproperty.h"
#include "qmlanchors.h"
#include "invalidmodelnodeexception.h"
#include "itemlibraryinfo.h"

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
    return isValidQmlObjectNode(modelNode)
            && modelNode.metaInfo().isValid()
            && (modelNode.metaInfo().isSubclassOf("QtQuick3D.Node"));
}

void Qml3DNode::setVariantProperty(const PropertyName &name, const QVariant &value)
{
    if (isBlocked(name))
        return;

    if (name.startsWith("eulerRotation"))
        handleEulerRotationSet();

    QmlObjectNode::setVariantProperty(name, value);
}

void Qml3DNode::setBindingProperty(const PropertyName &name, const QString &expression)
{
    if (isBlocked(name))
        return;

    if (name.startsWith("eulerRotation"))
        handleEulerRotationSet();

    QmlObjectNode::setBindingProperty(name, expression);
}

bool Qml3DNode::isBlocked(const PropertyName &propName) const
{
    if (modelNode().isValid() && propName.startsWith("eulerRotation"))
        return modelNode().auxiliaryData("rotBlocked@Internal").toBool();

    return false;
}

void Qml3DNode::handleEulerRotationSet()
{
    ModelNode node = modelNode();
    // The rotation property is quaternion, which is difficult to deal with for users, so QDS
    // only supports eulerRotation. Since having both on the same object isn't supported,
    // remove the rotation property if eulerRotation is set.
    if (node.isValid() && node.isSubclassOf("QtQuick3D.Node")) {
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
