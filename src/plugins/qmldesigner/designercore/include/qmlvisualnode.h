// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>
#include <modelnode.h>
#include "qmlobjectnode.h"
#include "qmlstate.h"

#include <QStringList>
#include <QRectF>
#include <QTransform>
#include <QVector3D>

namespace QmlDesigner {

class QmlModelStateGroup;
class QmlAnchors;
class ItemLibraryEntry;

inline constexpr AuxiliaryDataKeyView invisibleProperty{AuxiliaryDataType::Document, "invisible"};

class QMLDESIGNERCORE_EXPORT QmlVisualNode : public QmlObjectNode
{
    friend class QmlAnchors;
public:

    class Position
    {
        friend class QmlVisualNode;
    public:
        Position() {}
        Position(const QPointF &position) :
            m_2dPos(position)
        {}
        Position(const QVector3D &position) :
            m_3dPos(position)
        {}

        QList<QPair<PropertyName, QVariant>> propertyPairList() const;

    private:
        QPointF m_2dPos;
        QVector3D m_3dPos;
    };

    QmlVisualNode() : QmlObjectNode() {}
    QmlVisualNode(const ModelNode &modelNode)  : QmlObjectNode(modelNode) {}
    bool isValid() const override;
    static bool isValidQmlVisualNode(const ModelNode &modelNode);
    bool isRootNode() const;

    QList<QmlVisualNode> children() const;
    QList<QmlObjectNode> resources() const;
    QList<QmlObjectNode> allDirectSubNodes() const;

    bool hasChildren() const;
    bool hasResources() const;

    const QList<QmlVisualNode> allDirectSubModelNodes() const;
    const QList<QmlVisualNode> allSubModelNodes() const;
    bool hasAnySubModelNodes() const;
    void setVisibilityOverride(bool visible);
    bool visibilityOverride() const;

    void initializePosition(const Position &position);

    static bool isItemOr3DNode(const ModelNode &modelNode);

    static QmlObjectNode createQmlObjectNode(AbstractView *view,
                                             const ItemLibraryEntry &itemLibraryEntry,
                                             const Position &position,
                                             QmlVisualNode parentQmlItemNode);

    static QmlObjectNode createQmlObjectNode(AbstractView *view,
                                             const ItemLibraryEntry &itemLibraryEntry,
                                             const Position &position,
                                             NodeAbstractProperty parentProperty,
                                             bool createInTransaction = true);

    static QmlVisualNode createQml3DNode(AbstractView *view,
                                         const ItemLibraryEntry &itemLibraryEntry,
                                         qint32 sceneRootId = -1, const QVector3D &position = {},
                                         bool createInTransaction = true);

    static NodeListProperty findSceneNodeProperty(AbstractView *view, qint32 sceneRootId);

    static bool isFlowTransition(const ModelNode &node);
    static bool isFlowDecision(const ModelNode &node);
    static bool isFlowWildcard(const ModelNode &node);

    bool isFlowTransition() const;
    bool isFlowDecision() const;
    bool isFlowWildcard() const;

private:
    void setDoubleProperty(const PropertyName &name, double value);
};

class QMLDESIGNERCORE_EXPORT QmlModelStateGroup
{
    friend class QmlObjectNode;
    friend class StatesEditorView;

public:

    QmlModelStateGroup() : m_modelNode(ModelNode()) {}

    ModelNode modelNode() const { return m_modelNode; }
    QStringList names() const;
    QList<QmlModelState> allStates() const;
    QmlModelState state(const QString &name) const;
    QmlModelState addState(const QString &name);
    void removeState(const QString &name);

protected:
    QmlModelStateGroup(const ModelNode &modelNode) : m_modelNode(modelNode) {}

private:
    ModelNode m_modelNode;
};

QMLDESIGNERCORE_EXPORT QList<ModelNode> toModelNodeList(const QList<QmlItemNode> &fxItemNodeList);
QMLDESIGNERCORE_EXPORT QList<QmlVisualNode> toQmlVisualNodeList(const QList<ModelNode> &modelNodeList);

} //QmlDesigner
