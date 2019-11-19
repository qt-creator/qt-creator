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

#ifdef QUICK3D_MODULE

#include "selectionboxgeometry.h"

#include <QtQuick3DRuntimeRender/private/qssgrendergeometry_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermodel_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DUtils/private/qssgbounds3_p.h>
#include <QtQuick3D/private/qquick3dmodel_p.h>
#include <QtQuick3D/private/qquick3dobject_p_p.h>
#include <QtQuick/qquickwindow.h>

namespace QmlDesigner {
namespace Internal {

SelectionBoxGeometry::SelectionBoxGeometry()
    : QQuick3DGeometry()
{
}

SelectionBoxGeometry::~SelectionBoxGeometry()
{
}

QQuick3DNode *SelectionBoxGeometry::targetNode() const
{
    return m_targetNode;
}

QQuick3DNode *SelectionBoxGeometry::rootNode() const
{
    return m_rootNode;
}

QQuick3DViewport *SelectionBoxGeometry::view3D() const
{
    return m_view3D;
}

void SelectionBoxGeometry::setTargetNode(QQuick3DNode *targetNode)
{
    if (m_targetNode == targetNode)
        return;

    if (m_targetNode)
        m_targetNode->disconnect(this);
    m_targetNode = targetNode;

    if (auto model = qobject_cast<QQuick3DModel *>(m_targetNode)) {
        QObject::connect(model, &QQuick3DModel::sourceChanged,
                         this, &SelectionBoxGeometry::update, Qt::QueuedConnection);
        QObject::connect(model, &QQuick3DModel::geometryChanged,
                         this, &SelectionBoxGeometry::update, Qt::QueuedConnection);
    }

    emit targetNodeChanged();
    update();
}

void SelectionBoxGeometry::setRootNode(QQuick3DNode *rootNode)
{
    if (m_rootNode == rootNode)
        return;

    m_rootNode = rootNode;

    emit rootNodeChanged();
    update();
}

void SelectionBoxGeometry::setView3D(QQuick3DViewport *view)
{
    if (m_view3D == view)
        return;

    m_view3D = view;

    emit view3DChanged();
    update();
}

QSSGRenderGraphObject *SelectionBoxGeometry::updateSpatialNode(QSSGRenderGraphObject *node)
{
    node = QQuick3DGeometry::updateSpatialNode(node);
    QSSGRenderGeometry *geometry = static_cast<QSSGRenderGeometry *>(node);

    geometry->clear();

    QByteArray vertexData;
    QByteArray indexData;

    QVector3D minBounds(-100.f, -100.f, -100.f);
    QVector3D maxBounds(100.f, 100.f, 100.f);
    QVector3D extents;

    if (m_targetNode) {
        auto rootPriv = QQuick3DObjectPrivate::get(m_rootNode);
        auto targetPriv = QQuick3DObjectPrivate::get(m_targetNode);
        auto rootRN = static_cast<QSSGRenderNode *>(rootPriv->spatialNode);
        auto targetRN = static_cast<QSSGRenderNode *>(targetPriv->spatialNode);
        if (rootRN && targetRN) {
            // Explicitly set local transform of root node to target node parent's global transform
            // to avoid having to reparent the selection box. This has to be done directly on render
            // nodes.
            targetRN->parent->calculateGlobalVariables();
            QMatrix4x4 m = targetRN->parent->globalTransform;
            rootRN->localTransform = m;
            rootRN->markDirty(QSSGRenderNode::TransformDirtyFlag::TransformNotDirty);
            rootRN->calculateGlobalVariables();
        }
        if (auto modelNode = qobject_cast<QQuick3DModel *>(m_targetNode)) {
            auto nodePriv = QQuick3DObjectPrivate::get(m_targetNode);
            if (auto renderModel = static_cast<QSSGRenderModel *>(nodePriv->spatialNode)) {
                QWindow *window = static_cast<QWindow *>(m_view3D->window());
                if (window) {
                    auto context = QSSGRenderContextInterface::getRenderContextInterface(
                                quintptr(window));
                    if (!context.isNull()) {
                        auto bufferManager = context->bufferManager();
                        QSSGBounds3 bounds = renderModel->getModelBounds(bufferManager);
                        QVector3D center = bounds.center();
                        extents = bounds.extents();
                        minBounds = center - extents;
                        maxBounds = center + extents;
                    }
                }
            }
        }
    }

    // Adjust bounds to reduce targetNode pixels obscuring the selection box
    extents /= 1000.f;
    minBounds -= extents;
    maxBounds += extents;

    fillVertexData(vertexData, indexData, minBounds, maxBounds);

    geometry->addAttribute(QSSGRenderGeometry::Attribute::PositionSemantic, 0,
                           QSSGRenderGeometry::Attribute::ComponentType::F32Type);
    geometry->addAttribute(QSSGRenderGeometry::Attribute::IndexSemantic, 0,
                           QSSGRenderGeometry::Attribute::ComponentType::U16Type);
    geometry->setStride(12);
    geometry->setVertexData(vertexData);
    geometry->setIndexData(indexData);
    geometry->setPrimitiveType(QSSGRenderGeometry::Lines);
    geometry->setBounds(minBounds, maxBounds);

    return node;
}

void SelectionBoxGeometry::fillVertexData(QByteArray &vertexData, QByteArray &indexData,
                                          const QVector3D &minBounds, const QVector3D &maxBounds)
{
    const int vertexSize = int(sizeof(float)) * 8 * 3; // 8 vertices, 3 floats/vert
    vertexData.resize(vertexSize);
    const int indexSize = int(sizeof(quint16)) * 12 * 2; // 12 lines, 2 vert/line
    indexData.resize(indexSize);

    auto dataPtr = reinterpret_cast<float *>(vertexData.data());
    auto indexPtr = reinterpret_cast<quint16 *>(indexData.data());

    *dataPtr++ = maxBounds.x(); *dataPtr++ = maxBounds.y(); *dataPtr++ = maxBounds.z();
    *dataPtr++ = minBounds.x(); *dataPtr++ = maxBounds.y(); *dataPtr++ = maxBounds.z();
    *dataPtr++ = minBounds.x(); *dataPtr++ = minBounds.y(); *dataPtr++ = maxBounds.z();
    *dataPtr++ = maxBounds.x(); *dataPtr++ = minBounds.y(); *dataPtr++ = maxBounds.z();
    *dataPtr++ = maxBounds.x(); *dataPtr++ = maxBounds.y(); *dataPtr++ = minBounds.z();
    *dataPtr++ = minBounds.x(); *dataPtr++ = maxBounds.y(); *dataPtr++ = minBounds.z();
    *dataPtr++ = minBounds.x(); *dataPtr++ = minBounds.y(); *dataPtr++ = minBounds.z();
    *dataPtr++ = maxBounds.x(); *dataPtr++ = minBounds.y(); *dataPtr++ = minBounds.z();

    *indexPtr++ = 0; *indexPtr++ = 1;
    *indexPtr++ = 1; *indexPtr++ = 2;
    *indexPtr++ = 2; *indexPtr++ = 3;
    *indexPtr++ = 3; *indexPtr++ = 0;

    *indexPtr++ = 0; *indexPtr++ = 4;
    *indexPtr++ = 1; *indexPtr++ = 5;
    *indexPtr++ = 2; *indexPtr++ = 6;
    *indexPtr++ = 3; *indexPtr++ = 7;

    *indexPtr++ = 4; *indexPtr++ = 5;
    *indexPtr++ = 5; *indexPtr++ = 6;
    *indexPtr++ = 6; *indexPtr++ = 7;
    *indexPtr++ = 7; *indexPtr++ = 4;
}

}
}

#endif // QUICK3D_MODULE
