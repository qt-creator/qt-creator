// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>
#include <modelnode.h>
#include "qmlobjectnode.h"
#include "qmlstate.h"
#include "qmlvisualnode.h"

#include <QStringList>
#include <QRectF>
#include <QTransform>

namespace QmlDesigner {

class QmlModelStateGroup;
class QmlAnchors;
class ItemLibraryEntry;

class QMLDESIGNERCORE_EXPORT Qml3DNode : public QmlVisualNode
{
    friend class QmlAnchors;
public:
    Qml3DNode() : QmlVisualNode() {}
    Qml3DNode(const ModelNode &modelNode)  : QmlVisualNode(modelNode) {}
    bool isValid() const;
    explicit operator bool() const { return isValid(); }
    static bool isValidQml3DNode(const ModelNode &modelNode);
    static bool isValidVisualRoot(const ModelNode &modelNode);

    bool handleEulerRotation(const PropertyName &name);
    bool isBlocked(const PropertyName &propName) const;

    friend auto qHash(const Qml3DNode &node) { return qHash(node.modelNode()); }

private:
    void handleEulerRotationSet();
};

QMLDESIGNERCORE_EXPORT QList<ModelNode> toModelNodeList(const QList<Qml3DNode> &fxItemNodeList);
QMLDESIGNERCORE_EXPORT QList<Qml3DNode> toQml3DNodeList(const QList<ModelNode> &modelNodeList);

} //QmlDesigner
