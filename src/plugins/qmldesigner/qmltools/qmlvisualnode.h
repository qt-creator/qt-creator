// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlobjectnode.h"
#include "qmlstate.h"
#include <modelnode.h>

#include <QStringList>
#include <QRectF>
#include <QTransform>
#include <QVector3D>

namespace QmlDesigner {

class QmlModelStateGroup;
class QmlAnchors;
class ItemLibraryEntry;

inline constexpr AuxiliaryDataKeyView invisibleProperty{AuxiliaryDataType::Document, "invisible"};

class QMLDESIGNER_EXPORT QmlVisualNode : public QmlObjectNode
{
    friend class QmlAnchors;
public:

    class Position : public QVector3D
    {
        friend class QmlVisualNode;
    public:
        Position() {}
        Position(const QPointF &position) :
            QVector3D(position)
        {}
        Position(const QVector3D &position) :
            QVector3D(position),
            m_is3D(true)
        {}

        bool is3D() const;
        QList<QPair<PropertyName, QVariant>> propertyPairList() const;

    private:
        bool m_is3D = false;
    };

    QmlVisualNode() = default;
    QmlVisualNode(const ModelNode &modelNode)  : QmlObjectNode(modelNode)
    {}

    bool isValid(SL sl = {}) const;
    explicit operator bool() const { return isValid(); }

    static bool isValidQmlVisualNode(const ModelNode &modelNode, SL sl = {});
    bool isRootNode(SL sl = {}) const;

    QList<QmlVisualNode> children(SL sl = {}) const;
    QList<QmlObjectNode> resources(SL sl = {}) const;
    QList<QmlObjectNode> allDirectSubNodes(SL sl = {}) const;

    bool hasChildren(SL sl = {}) const;
    bool hasResources(SL sl = {}) const;

    const QList<QmlVisualNode> allDirectSubModelNodes(SL sl = {}) const;
    const QList<QmlVisualNode> allSubModelNodes(SL sl = {}) const;
    bool hasAnySubModelNodes(SL sl = {}) const;
    void setVisibilityOverride(bool visible, SL sl = {});
    bool visibilityOverride(SL sl = {}) const;

    void scatter(const ModelNode &targetNode, const std::optional<int> &offset, SL sl = {});
    void translate(const QVector3D &vector, SL sl = {});
    void setPosition(const Position &position, SL sl = {});
    Position position(SL sl = {}) const;

    static bool isItemOr3DNode(const ModelNode &modelNode, SL sl = {});

    static QmlObjectNode createQmlObjectNode(AbstractView *view,
                                             const ItemLibraryEntry &itemLibraryEntry,
                                             const Position &position,
                                             QmlVisualNode parentQmlItemNode,
                                             SL sl = {});

    static QmlObjectNode createQmlObjectNode(AbstractView *view,
                                             const ItemLibraryEntry &itemLibraryEntry,
                                             const Position &position,
                                             NodeAbstractProperty parentProperty,
                                             bool createInTransaction = true,
                                             SL sl = {});

    static QmlVisualNode createQml3DNode(AbstractView *view,
                                         const ItemLibraryEntry &itemLibraryEntry,
                                         qint32 sceneRootId = -1,
                                         const QVector3D &position = {},
                                         bool createInTransaction = true,
                                         SL sl = {});

    static QmlVisualNode createQml3DNode(AbstractView *view,
                                         const TypeName &typeName,
                                         const ModelNode &parentNode,
                                         const QString &importName = {},
                                         const QVector3D &position = {},
                                         bool createInTransaction = true,
                                         SL sl = {});

    static NodeListProperty findSceneNodeProperty(AbstractView *view, qint32 sceneRootId, SL sl = {});

private:
    void setDoubleProperty(PropertyNameView name, double value);
};

class QMLDESIGNER_EXPORT QmlModelStateGroup
{
    friend class QmlObjectNode;

protected:
    using SL = ModelTracing::SourceLocation;

public:
    QmlModelStateGroup() = default;
    QmlModelStateGroup(const ModelNode &modelNode) : m_modelNode(modelNode) {}

    explicit operator bool() const { return m_modelNode.isValid(); }

    ModelNode modelNode() const { return m_modelNode; }

    QStringList names(SL sl = {}) const;
    QList<QmlModelState> allStates(SL sl = {}) const;
    QmlModelState state(const QString &name, SL sl = {}) const;
    QmlModelState addState(const QString &name, SL sl = {});
    void removeState(const QString &name, SL sl = {});

private:
    ModelNode m_modelNode;
};

QMLDESIGNER_EXPORT QList<ModelNode> toModelNodeList(const QList<QmlItemNode> &fxItemNodeList);
QMLDESIGNER_EXPORT QList<QmlVisualNode> toQmlVisualNodeList(const QList<ModelNode> &modelNodeList);

} //QmlDesigner
