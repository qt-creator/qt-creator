// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifdef QUICK3D_MODULE

#include "geometrybase.h"

#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3DUtils/private/qssgbounds3_p.h>

namespace QmlDesigner {
namespace Internal {

class SelectionBoxGeometry : public GeometryBase
{
    Q_OBJECT
    Q_PROPERTY(QQuick3DNode *targetNode READ targetNode WRITE setTargetNode NOTIFY targetNodeChanged)
    Q_PROPERTY(QQuick3DNode *rootNode READ rootNode WRITE setRootNode NOTIFY rootNodeChanged)
    Q_PROPERTY(QQuick3DViewport *view3D READ view3D WRITE setView3D NOTIFY view3DChanged)
    Q_PROPERTY(bool isEmpty READ isEmpty NOTIFY isEmptyChanged)

public:
    SelectionBoxGeometry();
    ~SelectionBoxGeometry() override;

    QQuick3DNode *targetNode() const;
    QQuick3DNode *rootNode() const;
    QQuick3DViewport *view3D() const;
    bool isEmpty() const;
    void setEmpty(bool isEmpty);

    QSSGBounds3 bounds() const;

public slots:
    void setTargetNode(QQuick3DNode *targetNode);
    void setRootNode(QQuick3DNode *rootNode);
    void setView3D(QQuick3DViewport *view);

signals:
    void targetNodeChanged();
    void rootNodeChanged();
    void view3DChanged();
    void isEmptyChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void doUpdateGeometry() override;

private:
    void getBounds(QQuick3DNode *node, QByteArray &vertexData,
                   QByteArray &indexData, QVector3D &minBounds, QVector3D &maxBounds);
    void generateVertexData(QByteArray &vertexData, QByteArray &indexData,
                            const QVector3D &minBounds, const QVector3D &maxBounds);
    void trackNodeChanges(QQuick3DNode *node);
    void spatialNodeUpdateNeeded();
    void clearGeometry();

    QQuick3DNode *m_targetNode = nullptr;
    QQuick3DViewport *m_view3D = nullptr;
    QQuick3DNode *m_rootNode = nullptr;
    bool m_isEmpty = true;
    QVector<QMetaObject::Connection> m_connections;
    QSSGBounds3 m_bounds;
    bool m_spatialNodeUpdatePending = false;
};

}
}

QML_DECLARE_TYPE(QmlDesigner::Internal::SelectionBoxGeometry)

#endif // QUICK3D_MODULE
