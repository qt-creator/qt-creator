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
#include <QtQuick3D/private/qquick3dmodel_p.h>
#include <QtQuick3D/private/qquick3dobject_p_p.h>
#include <QtQuick/qquickwindow.h>
#include <QtCore/qvector.h>

#include <limits>

namespace QmlDesigner {
namespace Internal {

SelectionBoxGeometry::SelectionBoxGeometry()
    : QQuick3DGeometry()
{
}

SelectionBoxGeometry::~SelectionBoxGeometry()
{
    for (auto &connection : qAsConst(m_connections))
        QObject::disconnect(connection);
    m_connections.clear();
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

bool QmlDesigner::Internal::SelectionBoxGeometry::isEmpty() const
{
    return m_isEmpty;
}

QSSGBounds3 SelectionBoxGeometry::bounds() const
{
    return m_bounds;
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
    for (auto &connection : qAsConst(m_connections))
        QObject::disconnect(connection);
    m_connections.clear();

    QByteArray vertexData;
    QByteArray indexData;

    static const float floatMin = std::numeric_limits<float>::lowest();
    static const float floatMax = std::numeric_limits<float>::max();

    QVector3D minBounds = QVector3D(floatMax, floatMax, floatMax);
    QVector3D maxBounds = QVector3D(floatMin, floatMin, floatMin);

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
        getBounds(m_targetNode, vertexData, indexData, minBounds, maxBounds, QMatrix4x4());
    } else {
        // Fill some dummy data so geometry won't get rejected
        appendVertexData(vertexData, indexData, minBounds, maxBounds);
    }

    geometry->addAttribute(QSSGRenderGeometry::Attribute::PositionSemantic, 0,
                           QSSGRenderGeometry::Attribute::ComponentType::F32Type);
    geometry->addAttribute(QSSGRenderGeometry::Attribute::IndexSemantic, 0,
                           QSSGRenderGeometry::Attribute::ComponentType::U16Type);
    geometry->setStride(12);
    geometry->setVertexData(vertexData);
    geometry->setIndexData(indexData);
    geometry->setPrimitiveType(QSSGRenderGeometry::Lines);
    geometry->setBounds(minBounds, maxBounds);

    m_bounds = QSSGBounds3(minBounds, maxBounds);

    bool empty = minBounds.isNull() && maxBounds.isNull();
    if (m_isEmpty != empty) {
        m_isEmpty = empty;
        // Delay notification until we're done with spatial node updates
        QTimer::singleShot(0, this, &SelectionBoxGeometry::isEmptyChanged);
    }

    return node;
}

void SelectionBoxGeometry::getBounds(QQuick3DNode *node, QByteArray &vertexData,
                                     QByteArray &indexData, QVector3D &minBounds,
                                     QVector3D &maxBounds, const QMatrix4x4 &transform)
{
    QMatrix4x4 fullTransform;
    auto nodePriv = QQuick3DObjectPrivate::get(node);
    auto renderNode = static_cast<QSSGRenderNode *>(nodePriv->spatialNode);

    // All transforms are relative to targetNode transform, so its local transform is ignored
    if (node != m_targetNode) {
        if (renderNode) {
            if (renderNode->flags.testFlag(QSSGRenderNode::Flag::TransformDirty))
                renderNode->calculateLocalTransform();
            fullTransform = transform * renderNode->localTransform;
        }

        m_connections << QObject::connect(node, &QQuick3DNode::scaleChanged,
                                          this, &SelectionBoxGeometry::update, Qt::QueuedConnection);
        m_connections << QObject::connect(node, &QQuick3DNode::rotationChanged,
                                          this, &SelectionBoxGeometry::update, Qt::QueuedConnection);
        m_connections << QObject::connect(node, &QQuick3DNode::positionChanged,
                                          this, &SelectionBoxGeometry::update, Qt::QueuedConnection);
        m_connections << QObject::connect(node, &QQuick3DNode::pivotChanged,
                                          this, &SelectionBoxGeometry::update, Qt::QueuedConnection);
        m_connections << QObject::connect(node, &QQuick3DNode::orientationChanged,
                                          this, &SelectionBoxGeometry::update, Qt::QueuedConnection);
        m_connections << QObject::connect(node, &QQuick3DNode::rotationOrderChanged,
                                          this, &SelectionBoxGeometry::update, Qt::QueuedConnection);
    }

    QVector<QVector3D> minBoundsVec;
    QVector<QVector3D> maxBoundsVec;

    // Check for children
    const auto children = node->childItems();
    for (const auto child : children) {
        if (auto childNode = qobject_cast<QQuick3DNode *>(child)) {
            QVector3D newMinBounds = minBounds;
            QVector3D newMaxBounds = maxBounds;
            getBounds(childNode, vertexData, indexData, newMinBounds, newMaxBounds, fullTransform);
            minBoundsVec << newMinBounds;
            maxBoundsVec << newMaxBounds;
        }
    }

    // Combine all child bounds
    for (const auto &newBounds : qAsConst(minBoundsVec)) {
        minBounds.setX(qMin(newBounds.x(), minBounds.x()));
        minBounds.setY(qMin(newBounds.y(), minBounds.y()));
        minBounds.setZ(qMin(newBounds.z(), minBounds.z()));
    }
    for (const auto &newBounds : qAsConst(maxBoundsVec)) {
        maxBounds.setX(qMax(newBounds.x(), maxBounds.x()));
        maxBounds.setY(qMax(newBounds.y(), maxBounds.y()));
        maxBounds.setZ(qMax(newBounds.z(), maxBounds.z()));
    }

    if (auto modelNode = qobject_cast<QQuick3DModel *>(node)) {
        if (auto renderModel = static_cast<QSSGRenderModel *>(renderNode)) {
            QWindow *window = static_cast<QWindow *>(m_view3D->window());
            if (window) {
                auto context = QSSGRenderContextInterface::getRenderContextInterface(
                            quintptr(window));
                if (!context.isNull()) {
                    auto bufferManager = context->bufferManager();
                    QSSGBounds3 bounds = renderModel->getModelBounds(bufferManager);
                    QVector3D center = bounds.center();
                    QVector3D extents = bounds.extents();
                    QVector3D localMin = center - extents;
                    QVector3D localMax = center + extents;

                    // Transform all corners of the local bounding box to find final extent in
                    // in parent space

                    auto checkCorner = [&minBounds, &maxBounds, &fullTransform]
                            (const QVector3D &corner) {
                        QVector3D mappedCorner = fullTransform.map(corner);
                        minBounds.setX(qMin(mappedCorner.x(), minBounds.x()));
                        minBounds.setY(qMin(mappedCorner.y(), minBounds.y()));
                        minBounds.setZ(qMin(mappedCorner.z(), minBounds.z()));
                        maxBounds.setX(qMax(mappedCorner.x(), maxBounds.x()));
                        maxBounds.setY(qMax(mappedCorner.y(), maxBounds.y()));
                        maxBounds.setZ(qMax(mappedCorner.z(), maxBounds.z()));
                    };

                    checkCorner(localMin);
                    checkCorner(localMax);
                    checkCorner(QVector3D(localMin.x(), localMin.y(), localMax.z()));
                    checkCorner(QVector3D(localMin.x(), localMax.y(), localMin.z()));
                    checkCorner(QVector3D(localMax.x(), localMin.y(), localMin.z()));
                    checkCorner(QVector3D(localMin.x(), localMax.y(), localMax.z()));
                    checkCorner(QVector3D(localMax.x(), localMax.y(), localMin.z()));
                    checkCorner(QVector3D(localMax.x(), localMin.y(), localMax.z()));
                }
            }
        }
    }

    // Target node and immediate children get selection boxes
    if (transform.isIdentity()) {
        // Adjust bounds to reduce targetNode pixels obscuring the selection box
        QVector3D extents = (maxBounds - minBounds) / 1000.f;
        QVector3D minAdjBounds = minBounds - extents;
        QVector3D maxAdjBounds = maxBounds + extents;

        appendVertexData(vertexData, indexData, minAdjBounds, maxAdjBounds);
    }
}

void SelectionBoxGeometry::appendVertexData(QByteArray &vertexData, QByteArray &indexData,
                                            const QVector3D &minBounds, const QVector3D &maxBounds)
{
    int initialVertexSize = vertexData.size();
    int initialIndexSize = indexData.size();
    const int vertexSize = int(sizeof(float)) * 8 * 3; // 8 vertices, 3 floats/vert
    quint16 indexAdd = quint16(initialVertexSize / 12);
    vertexData.resize(initialVertexSize + vertexSize);
    const int indexSize = int(sizeof(quint16)) * 12 * 2; // 12 lines, 2 vert/line
    indexData.resize(initialIndexSize + indexSize);

    auto dataPtr = reinterpret_cast<float *>(vertexData.data() + initialVertexSize);
    auto indexPtr = reinterpret_cast<quint16 *>(indexData.data() + initialIndexSize);

    *dataPtr++ = maxBounds.x(); *dataPtr++ = maxBounds.y(); *dataPtr++ = maxBounds.z();
    *dataPtr++ = minBounds.x(); *dataPtr++ = maxBounds.y(); *dataPtr++ = maxBounds.z();
    *dataPtr++ = minBounds.x(); *dataPtr++ = minBounds.y(); *dataPtr++ = maxBounds.z();
    *dataPtr++ = maxBounds.x(); *dataPtr++ = minBounds.y(); *dataPtr++ = maxBounds.z();
    *dataPtr++ = maxBounds.x(); *dataPtr++ = maxBounds.y(); *dataPtr++ = minBounds.z();
    *dataPtr++ = minBounds.x(); *dataPtr++ = maxBounds.y(); *dataPtr++ = minBounds.z();
    *dataPtr++ = minBounds.x(); *dataPtr++ = minBounds.y(); *dataPtr++ = minBounds.z();
    *dataPtr++ = maxBounds.x(); *dataPtr++ = minBounds.y(); *dataPtr++ = minBounds.z();

    *indexPtr++ = 0 + indexAdd; *indexPtr++ = 1 + indexAdd;
    *indexPtr++ = 1 + indexAdd; *indexPtr++ = 2 + indexAdd;
    *indexPtr++ = 2 + indexAdd; *indexPtr++ = 3 + indexAdd;
    *indexPtr++ = 3 + indexAdd; *indexPtr++ = 0 + indexAdd;

    *indexPtr++ = 0 + indexAdd; *indexPtr++ = 4 + indexAdd;
    *indexPtr++ = 1 + indexAdd; *indexPtr++ = 5 + indexAdd;
    *indexPtr++ = 2 + indexAdd; *indexPtr++ = 6 + indexAdd;
    *indexPtr++ = 3 + indexAdd; *indexPtr++ = 7 + indexAdd;

    *indexPtr++ = 4 + indexAdd; *indexPtr++ = 5 + indexAdd;
    *indexPtr++ = 5 + indexAdd; *indexPtr++ = 6 + indexAdd;
    *indexPtr++ = 6 + indexAdd; *indexPtr++ = 7 + indexAdd;
    *indexPtr++ = 7 + indexAdd; *indexPtr++ = 4 + indexAdd;
}

}
}

#endif // QUICK3D_MODULE
