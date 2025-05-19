// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlvisualnode.h"

#include <modelnode.h>

#include <QMatrix4x4>

namespace QmlDesigner {

class QmlModelStateGroup;
class QmlAnchors;
class ItemLibraryEntry;

class QMLDESIGNER_EXPORT Qml3DNode : public QmlVisualNode
{
    friend class QmlAnchors;
public:
    Qml3DNode() : QmlVisualNode() {}
    Qml3DNode(const ModelNode &modelNode)  : QmlVisualNode(modelNode) {}
    bool isValid() const;
    explicit operator bool() const { return isValid(); }
    static bool isValidQml3DNode(const ModelNode &modelNode);
    static bool isValidVisualRoot(const ModelNode &modelNode);

    bool handleEulerRotation(PropertyNameView name);
    bool isBlocked(PropertyNameView propName) const;

    void reparentWithTransform(NodeAbstractProperty &parentProperty);
    bool hasAnimatedTransform();

    friend auto qHash(const Qml3DNode &node) { return qHash(node.modelNode()); }

private:
    void handleEulerRotationSet();
    QMatrix4x4 localTransform() const;
    QMatrix4x4 sceneTransform() const;
    void setLocalTransform(const QMatrix4x4 &newParentSceneTransform,
                           const QMatrix4x4 &oldSceneTransform,
                           bool adjustScale);
};

QMLDESIGNER_EXPORT QList<ModelNode> toModelNodeList(const QList<Qml3DNode> &fxItemNodeList);
QMLDESIGNER_EXPORT QList<Qml3DNode> toQml3DNodeList(const QList<ModelNode> &modelNodeList);

} //QmlDesigner
