// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qml3dnode.h"

#include "qmltimelinekeyframegroup.h"

#include <auxiliarydataproperties.h>
#include <bindingproperty.h>
#include <nodeabstractproperty.h>
#include <variantproperty.h>

#include <utils/qtcassert.h>

#include <QQuaternion>

namespace QmlDesigner {

using NanotraceHR::keyValue;

using ModelTracing::category;

bool Qml3DNode::isValid(SL sl) const
{
    return isValidQml3DNode(modelNode(), sl);
}

bool Qml3DNode::isValidQml3DNode(const ModelNode &modelNode, SL sl)
{
    NanotraceHR::Tracer tracer{"qml 3d node is valid Qml 3D node",
                               category(),
                               keyValue("model node", modelNode),
                               keyValue("caller location", sl)};

    return isValidQmlObjectNode(modelNode) && (modelNode.metaInfo().isQtQuick3DNode());
}

bool Qml3DNode::isValidVisualRoot(const ModelNode &modelNode, SL sl)
{
    NanotraceHR::Tracer tracer{"qml 3d node is valid visual root",
                               category(),
                               keyValue("model node", modelNode),
                               keyValue("caller location", sl)};

    return isValidQmlObjectNode(modelNode)
           && (modelNode.metaInfo().isQtQuick3DNode() || modelNode.metaInfo().isQtQuick3DMaterial());
}

bool Qml3DNode::handleEulerRotation(PropertyNameView name, SL sl)
{
    NanotraceHR::Tracer tracer{"1ml 3d node handle euler rotation",
                               category(),
                               keyValue("model node", *this),
                               keyValue("property name", name),
                               keyValue("caller location", sl)};

    if (isBlocked(name))
        return false;

    if (name.startsWith("eulerRotation"))
        handleEulerRotationSet();

    return true;
}

bool Qml3DNode::isBlocked(PropertyNameView propName, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml 3d node is blocked",
                               category(),
                               keyValue("model node", *this),
                               keyValue("property name", propName),
                               keyValue("caller location", sl)};

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
            QmlPropertyChanges changeSet(currentState().ensurePropertyChangesForTarget(node));
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

QMatrix4x4 Qml3DNode::sceneTransform() const
{
    if (modelNode().hasParentProperty()) {
        Qml3DNode parentNode = modelNode().parentProperty().parentModelNode();
        if (isValidQml3DNode(parentNode))
            return parentNode.sceneTransform() * localTransform();
    }
    return localTransform();
}

static QVector3D vector3DFromString(const QString &s, bool *ok)
{
    if (s.count(QLatin1Char(',')) != 2) {
        if (ok)
            *ok = false;
        return {};
    }

    bool xGood, yGood, zGood;
    int indexOpen = s.indexOf(QLatin1Char('('));
    int indexClose = s.indexOf(QLatin1Char(')'));
    if (indexClose == -1)
        indexClose = s.length();
    int index1 = s.indexOf(QLatin1Char(','));
    int index2 = s.indexOf(QLatin1Char(','), index1 + 1);
    qreal xCoord = s.mid(indexOpen + 1, index1 - indexOpen - 1).toDouble(&xGood);
    qreal yCoord = s.mid(index1 + 1, index2 - index1 - 1).toDouble(&yGood);
    qreal zCoord = s.mid(index2 + 1, indexClose - index2 - 1).toDouble(&zGood);

    if (!xGood || !yGood || !zGood) {
        if (ok)
            *ok = false;
        return QVector3D();
    }

    if (ok)
        *ok = true;
    return QVector3D(xCoord, yCoord, zCoord);
}

static QQuaternion quaternionFromString(const QString &s, bool *ok)
{
    if (s.count(QLatin1Char(',')) != 3) {
        if (ok)
            *ok = false;
        return {};
    }

    bool xGood, yGood, zGood, wGood;
    int indexOpen = s.indexOf(QLatin1Char('('));
    int indexClose = s.indexOf(QLatin1Char(')'));
    if (indexClose == -1)
        indexClose = s.length();
    int index1 = s.indexOf(QLatin1Char(','));
    int index2 = s.indexOf(QLatin1Char(','), index1 + 1);
    int index3 = s.indexOf(QLatin1Char(','), index2 + 1);

    qreal w = s.mid(indexOpen + 1, index1 - indexOpen - 1).toDouble(&wGood);
    qreal x = s.mid(index1 + 1, index2 - index1 - 1).toDouble(&xGood);
    qreal y = s.mid(index2 + 1, index3 - index2 - 1).toDouble(&yGood);
    qreal z = s.mid(index3 + 1, indexClose - index3 - 1).toDouble(&zGood);

    if (!xGood || !yGood || !zGood || !wGood) {
        if (ok)
            *ok = false;
        return QQuaternion();
    }

    if (ok)
        *ok = true;

    return QQuaternion(w, x, y, z);
}

struct Exists {
    bool x = false;
    bool y = false;
    bool z = false;
};

static float floatPropertyValue(const ModelNode &node, const PropertyName &propName, bool &exists)
{
    auto prop = node.variantProperty(propName);
    exists = prop.exists();
    if (exists)
        return prop.value().value<float>();
    else
        return 0.f;
};

static QVector3D vec3dPropertyValue(const ModelNode &node, const PropertyName &propName, Exists &exists)
{
    auto prop = node.property(propName);
    if (prop.exists()) {
        exists = {true, true, true};
        if (prop.isBindingProperty()) {
            bool ok = false;
            QVector3D vec = vector3DFromString(prop.toBindingProperty().expression(), &ok);
            if (ok)
                return vec;
            return {};
        }
        return prop.toVariantProperty().value().value<QVector3D>();
    } else {
        QVector3D propVal;
        propVal.setX(floatPropertyValue(node, propName + ".x", exists.x));
        propVal.setY(floatPropertyValue(node, propName + ".y", exists.y));
        propVal.setZ(floatPropertyValue(node, propName + ".z", exists.z));
        return propVal;
    }
};

QMatrix4x4 Qml3DNode::localTransform() const
{
    if (!isValidQml3DNode(*this))
        return {};

    Exists exists;
    QVector3D position = vec3dPropertyValue(modelNode(), "position", exists);

    // Position can also be expressed by plain x, y, and z properties
    if (!exists.x)
        position.setX(floatPropertyValue(modelNode(), "x", exists.x));
    if (!exists.y)
        position.setY(floatPropertyValue(modelNode(), "y", exists.y));
    if (!exists.z)
        position.setZ(floatPropertyValue(modelNode(), "z", exists.z));

    QQuaternion rotation;
    auto rotProp = modelNode().property("rotation");
    if (rotProp.exists()) {
        if (rotProp.isBindingProperty()) {
            bool ok = false;
            QQuaternion q = quaternionFromString(rotProp.toBindingProperty().expression(), &ok);
            if (ok)
                rotation = q.normalized();
            else
                rotation = {};
        } else {
            rotation = rotProp.toVariantProperty().value().value<QQuaternion>().normalized();
        }
    } else {
        QVector3D eulerRot = vec3dPropertyValue(modelNode(), "eulerRotation", exists);
        rotation = QQuaternion::fromEulerAngles(eulerRot);
    }

    QVector3D scale = vec3dPropertyValue(modelNode(), "scale", exists);
    if (!exists.x)
        scale.setX(1.f);
    if (!exists.y)
        scale.setY(1.f);
    if (!exists.z)
        scale.setZ(1.f);

    const QVector3D pivot = vec3dPropertyValue(modelNode(), "pivot", exists);
    auto offset = (-pivot * scale);

    QMatrix4x4 transform;

    transform(0, 0) = scale[0];
    transform(1, 1) = scale[1];
    transform(2, 2) = scale[2];

    transform(0, 3) = offset[0];
    transform(1, 3) = offset[1];
    transform(2, 3) = offset[2];

    transform = QMatrix4x4{rotation.toRotationMatrix()} * transform;

    transform(0, 3) += position[0];
    transform(1, 3) += position[1];
    transform(2, 3) += position[2];

    return transform;
}

static void normalizeMatrix(QMatrix4x4 &m)
{
    QVector4D c0 = m.column(0);
    QVector4D c1 = m.column(1);
    QVector4D c2 = m.column(2);
    QVector4D c3 = m.column(3);

    c0.normalize();
    c1.normalize();
    c2.normalize();
    c3.normalize();

    m.setColumn(0, c0);
    m.setColumn(1, c1);
    m.setColumn(2, c2);
    m.setColumn(3, c3);
}

static QMatrix3x3 getUpper3x3(const QMatrix4x4 &m)
{
    const float values[9] = {m(0, 0), m(0, 1), m(0, 2),
                             m(1, 0), m(1, 1), m(1, 2),
                             m(2, 0), m(2, 1), m(2, 2)};
    return QMatrix3x3(values);
}

static QVector3D getPosition(const QMatrix4x4 &m)
{
    return QVector3D(m(0, 3), m(1, 3), m(2, 3));
}

static QVector3D getScale(const QMatrix4x4 &m)
{
    const float scaleX = m.column(0).length();
    const float scaleY = m.column(1).length();
    const float scaleZ = m.column(2).length();
    return QVector3D(scaleX, scaleY, scaleZ);
}

static bool transformHasScalingAndRotation(const QMatrix4x4 &transform)
{
    QVector3D scale = getScale(transform);
    bool hasUniformScale = qFuzzyCompare(scale.x(), scale.y())
                           && qFuzzyCompare(scale.y(), scale.z());

    QVector3D xAxis = transform.column(0).toVector3D() / scale.x();
    QVector3D yAxis = transform.column(1).toVector3D() / scale.y();
    QVector3D zAxis = transform.column(2).toVector3D() / scale.z();

    bool hasRotation = !qFuzzyCompare(xAxis, QVector3D(1.0f, 0.0f, 0.0f)) ||
                       !qFuzzyCompare(yAxis, QVector3D(0.0f, 1.0f, 0.0f)) ||
                       !qFuzzyCompare(zAxis, QVector3D(0.0f, 0.0f, 1.0f));

    return !hasUniformScale && hasRotation;
}

bool Qml3DNode::hasAnimatedTransform(SL sl)
{
    NanotraceHR::Tracer tracer{"qml 3d node has animated transform",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    QmlTimeline timeline = currentTimeline();
    if (!timeline)
        return false;

    static const QSet<PropertyName> checkProps{
        "position", "position.x", "position.y", "position.z", "x", "y", "z",
        "rotation", "rotation.x", "rotation.y", "rotation.z", "rotation.w",
        "eulerRotation", "eulerRotation.x", "eulerRotation.y", "eulerRotation.z",
        "scale", "scale.x", "scale.y", "scale.z",
        "pivot", "pivot.x", "pivot.y", "pivot.z"
    };

    for (ModelNode node = modelNode(); isValidQml3DNode(node); node = node.parentProperty().parentModelNode()) {
        const QList<QmlTimelineKeyframeGroup> groups = timeline.keyframeGroupsForTarget(node);
        if (std::ranges::any_of(groups, [&](const auto &group) {
                return checkProps.contains(group.propertyName());
            })) {
            return true;
        }
    }

    return false;
}

void Qml3DNode::setLocalTransform(const QMatrix4x4 &newParentSceneTransform,
                                  const QMatrix4x4 &oldSceneTransform,
                                  bool adjustScale)
{
    QMatrix4x4 newLocalTransform = newParentSceneTransform.inverted() * oldSceneTransform;
    QMatrix4x4 normalMatrix(newLocalTransform);
    normalizeMatrix(normalMatrix);

    QTC_ASSERT(!qFuzzyIsNull(normalMatrix.determinant()), return);

    QQuaternion rotation = QQuaternion::fromRotationMatrix(getUpper3x3(normalMatrix)).normalized();
    QVector3D position;

    Exists exists;
    QVector3D pivot = vec3dPropertyValue(modelNode(), "pivot", exists);

    // Since reparenting always happens in base state, we also adjust transform in base state
    auto setProp = [this](const PropertyName &propName, float value, float defaultValue) {
        if (qFuzzyCompare(value, defaultValue))
            modelNode().removeProperty(propName);
        else
            modelNode().variantProperty(propName).setValue(value);
    };

    const QVector3D scale = getScale(newLocalTransform);
    if (adjustScale) {
        position = getPosition(newLocalTransform) - rotation.rotatedVector(-pivot * scale);
        modelNode().removeProperty("scale");
        setProp("scale.x", scale.x(), 1.f);
        setProp("scale.y", scale.y(), 1.f);
        setProp("scale.z", scale.z(), 1.f);
    } else {
        QMatrix4x4 oldLocalTransform = localTransform();
        QVector3D oldScale = getScale(oldLocalTransform);
        position = getPosition(newLocalTransform)
                   - (rotation.rotatedVector(-pivot * oldScale));
    }

    modelNode().removeProperty("position");
    modelNode().removeProperty("position.x");
    modelNode().removeProperty("position.y");
    modelNode().removeProperty("position.z");
    modelNode().removeProperty("rotation");
    modelNode().removeProperty("eulerRotation");

    setProp("x", position.x(), 0.f);
    setProp("y", position.y(), 0.f);
    setProp("z", position.z(), 0.f);

    const QVector3D eulerRotation = rotation.toEulerAngles();
    setProp("eulerRotation.x", eulerRotation.x(), 0.f);
    setProp("eulerRotation.y", eulerRotation.y(), 0.f);
    setProp("eulerRotation.z", eulerRotation.z(), 0.f);
}

void Qml3DNode::reparentWithTransform(NodeAbstractProperty &parentProperty, SL sl)
{
    NanotraceHR::Tracer tracer{"qml 3d node reparent with transform",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    Qml3DNode oldParent3d;
    if (modelNode().hasParentProperty())
        oldParent3d = modelNode().parentProperty().parentModelNode();
    Qml3DNode newParent3d(parentProperty.parentModelNode());

    QMatrix4x4 oldParentSceneTransform = oldParent3d.sceneTransform();
    QMatrix4x4 newParentSceneTransform = newParent3d.sceneTransform();
    QMatrix4x4 oldSceneTransform = sceneTransform();
    bool isNewScalable = !transformHasScalingAndRotation(newParentSceneTransform);
    bool isOldScalable = !transformHasScalingAndRotation(oldParentSceneTransform);

    parentProperty.reparentHere(modelNode());

    if (oldParentSceneTransform != newParentSceneTransform) {
        setLocalTransform(newParentSceneTransform, oldSceneTransform,
                          isNewScalable && isOldScalable);
    }

}

} //QmlDesigner
