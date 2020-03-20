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

#pragma once

#ifdef QUICK3D_MODULE

#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dgeometry_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3DUtils/private/qssgbounds3_p.h>

namespace QmlDesigner {
namespace Internal {

class SelectionBoxGeometry : public QQuick3DGeometry
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

    QSSGBounds3 bounds() const;

public Q_SLOTS:
    void setTargetNode(QQuick3DNode *targetNode);
    void setRootNode(QQuick3DNode *rootNode);
    void setView3D(QQuick3DViewport *view);

Q_SIGNALS:
    void targetNodeChanged();
    void rootNodeChanged();
    void view3DChanged();
    void isEmptyChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;

private:
    void getBounds(QQuick3DNode *node, QByteArray &vertexData,
                   QByteArray &indexData, QVector3D &minBounds, QVector3D &maxBounds);
    void appendVertexData(const QMatrix4x4 &m, QByteArray &vertexData, QByteArray &indexData,
                          const QVector3D &minBounds, const QVector3D &maxBounds);
    void trackNodeChanges(QQuick3DNode *node);
    void targetMeshUpdated();

    QQuick3DNode *m_targetNode = nullptr;
    QQuick3DViewport *m_view3D = nullptr;
    QQuick3DNode *m_rootNode = nullptr;
    bool m_isEmpty = true;
    QVector<QMetaObject::Connection> m_connections;
    QSSGBounds3 m_bounds;
    bool m_spatialNodeUpdatePending = false;
    bool m_meshUpdatePending = false;
};

}
}

QML_DECLARE_TYPE(QmlDesigner::Internal::SelectionBoxGeometry)

#endif // QUICK3D_MODULE
