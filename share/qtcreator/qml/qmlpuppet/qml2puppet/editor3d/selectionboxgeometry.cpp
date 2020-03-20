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
#include <QtQuick3D/qquick3dobject.h>
#include <QtQuick/qquickwindow.h>
#include <QtCore/qvector.h>
#include <QtCore/qtimer.h>

#include <limits>

namespace QmlDesigner {
namespace Internal {

static const float floatMin = std::numeric_limits<float>::lowest();
static const float floatMax = std::numeric_limits<float>::max();
static const QVector3D maxVec = QVector3D(floatMax, floatMax, floatMax);
static const QVector3D minVec = QVector3D(floatMin, floatMin, floatMin);

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
                         this, &SelectionBoxGeometry::targetMeshUpdated, Qt::QueuedConnection);
        QObject::connect(model, &QQuick3DModel::geometryChanged,
                         this, &SelectionBoxGeometry::targetMeshUpdated, Qt::QueuedConnection);
    }
    if (m_targetNode) {
        QObject::connect(m_targetNode, &QQuick3DNode::parentChanged,
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
    // If target node mesh has been updated, we need to defer updating the box geometry
    // to the next frame to ensure target node geometry has been updated
    if (m_meshUpdatePending) {
        QTimer::singleShot(0, this, &SelectionBoxGeometry::update);
        m_meshUpdatePending = false;
        return node;
    }

    node = QQuick3DGeometry::updateSpatialNode(node);
    QSSGRenderGeometry *geometry = static_cast<QSSGRenderGeometry *>(node);

    geometry->clear();
    for (auto &connection : qAsConst(m_connections))
        QObject::disconnect(connection);
    m_connections.clear();

    QByteArray vertexData;
    QByteArray indexData;

    QVector3D minBounds = maxVec;
    QVector3D maxBounds = minVec;

    if (m_targetNode) {
        auto rootPriv = QQuick3DObjectPrivate::get(m_rootNode);
        auto targetPriv = QQuick3DObjectPrivate::get(m_targetNode);
        auto rootRN = static_cast<QSSGRenderNode *>(rootPriv->spatialNode);
        auto targetRN = static_cast<QSSGRenderNode *>(targetPriv->spatialNode);
        if (rootRN && targetRN) {
            // Explicitly set local transform of root node to target node parent's global transform
            // to avoid having to reparent the selection box. This has to be done directly on render
            // nodes.
            QMatrix4x4 m;
            if (targetRN->parent) {
                targetRN->parent->calculateGlobalVariables();
                m = targetRN->parent->globalTransform;
            }
            rootRN->localTransform = m;
            rootRN->markDirty(QSSGRenderNode::TransformDirtyFlag::TransformNotDirty);
            rootRN->calculateGlobalVariables();
            m_spatialNodeUpdatePending = false;
        } else if (!m_spatialNodeUpdatePending) {
            m_spatialNodeUpdatePending = true;
            // A necessary spatial node doesn't yet exist. Defer selection box creation one frame.
            // Note: We don't share pending flag with target mesh update, which is checked and
            // cleared at the beginning of this method, as there would be potential for an endless
            // loop in case we can't ever resolve one of the spatial nodes.
            QTimer::singleShot(0, this, &SelectionBoxGeometry::update);
            return node;
        }
        getBounds(m_targetNode, vertexData, indexData, minBounds, maxBounds);
        appendVertexData(QMatrix4x4(), vertexData, indexData, minBounds, maxBounds);

        // Track changes in ancestors, as they can move node without affecting node properties
        auto parentNode = m_targetNode->parentNode();
        while (parentNode) {
            trackNodeChanges(parentNode);
            parentNode = parentNode->parentNode();
        }
    } else {
        // Fill some dummy data so geometry won't get rejected
        minBounds = {};
        maxBounds = {};
        appendVertexData(QMatrix4x4(), vertexData, indexData, minBounds, maxBounds);
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

void SelectionBoxGeometry::getBounds(
        QQuick3DNode *node, QByteArray &vertexData, QByteArray &indexData,
        QVector3D &minBounds, QVector3D &maxBounds)
{
    QMatrix4x4 localTransform;
    auto nodePriv = QQuick3DObjectPrivate::get(node);
    auto renderNode = static_cast<QSSGRenderNode *>(nodePriv->spatialNode);

    if (node != m_targetNode) {
        if (renderNode) {
            if (renderNode->flags.testFlag(QSSGRenderNode::Flag::TransformDirty))
                renderNode->calculateLocalTransform();
            localTransform = renderNode->localTransform;
        }
        trackNodeChanges(node);
    }

    QVector3D localMinBounds = maxVec;
    QVector3D localMaxBounds = minVec;

    // Find bounds for children
    QVector<QVector3D> minBoundsVec;
    QVector<QVector3D> maxBoundsVec;
    const auto children = node->childItems();
    for (const auto child : children) {
        if (auto childNode = qobject_cast<QQuick3DNode *>(child)) {
            QVector3D newMinBounds = minBounds;
            QVector3D newMaxBounds = maxBounds;
            getBounds(childNode, vertexData, indexData, newMinBounds, newMaxBounds);
            minBoundsVec << newMinBounds;
            maxBoundsVec << newMaxBounds;
        }
    }

    auto combineMinBounds = [](QVector3D &target, const QVector3D &source) {
        target.setX(qMin(source.x(), target.x()));
        target.setY(qMin(source.y(), target.y()));
        target.setZ(qMin(source.z(), target.z()));
    };
    auto combineMaxBounds = [](QVector3D &target, const QVector3D &source) {
        target.setX(qMax(source.x(), target.x()));
        target.setY(qMax(source.y(), target.y()));
        target.setZ(qMax(source.z(), target.z()));
    };
    auto transformCorner = [&](const QMatrix4x4 &m, QVector3D &minTarget, QVector3D &maxTarget,
            const QVector3D &corner) {
        QVector3D mappedCorner = m.map(corner);
        combineMinBounds(minTarget, mappedCorner);
        combineMaxBounds(maxTarget, mappedCorner);
    };
    auto transformCorners = [&](const QMatrix4x4 &m, QVector3D &minTarget, QVector3D &maxTarget,
            const QVector3D &minCorner, const QVector3D &maxCorner) {
        transformCorner(m, minTarget, maxTarget, minCorner);
        transformCorner(m, minTarget, maxTarget, maxCorner);
        transformCorner(m, minTarget, maxTarget, QVector3D(minCorner.x(), minCorner.y(), maxCorner.z()));
        transformCorner(m, minTarget, maxTarget, QVector3D(minCorner.x(), maxCorner.y(), minCorner.z()));
        transformCorner(m, minTarget, maxTarget, QVector3D(maxCorner.x(), minCorner.y(), minCorner.z()));
        transformCorner(m, minTarget, maxTarget, QVector3D(minCorner.x(), maxCorner.y(), maxCorner.z()));
        transformCorner(m, minTarget, maxTarget, QVector3D(maxCorner.x(), maxCorner.y(), minCorner.z()));
        transformCorner(m, minTarget, maxTarget, QVector3D(maxCorner.x(), minCorner.y(), maxCorner.z()));
    };

    // Combine all child bounds
    for (const auto &newBounds : qAsConst(minBoundsVec))
        combineMinBounds(localMinBounds, newBounds);
    for (const auto &newBounds : qAsConst(maxBoundsVec))
        combineMaxBounds(localMaxBounds, newBounds);

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

                    combineMinBounds(localMinBounds, localMin);
                    combineMaxBounds(localMaxBounds, localMax);
                }
            }
        }
    } else {
        combineMinBounds(localMinBounds, {});
        combineMaxBounds(localMaxBounds, {});
    }

    if (localMaxBounds == minVec) {
        localMinBounds = {};
        localMaxBounds = {};
    }

    // Transform local space bounding box to parent space
    transformCorners(localTransform, minBounds, maxBounds, localMinBounds, localMaxBounds);

    // Immediate child boxes
    if (node->parentNode() == m_targetNode)
        appendVertexData(localTransform, vertexData, indexData, localMinBounds, localMaxBounds);
}

void SelectionBoxGeometry::appendVertexData(const QMatrix4x4 &m, QByteArray &vertexData,
                                            QByteArray &indexData, const QVector3D &minBounds,
                                            const QVector3D &maxBounds)
{
    // Adjust bounds to reduce targetNode pixels obscuring the selection box
    QVector3D extents = (maxBounds - minBounds) / 1000.f;
    QVector3D minAdjBounds = minBounds - extents;
    QVector3D maxAdjBounds = maxBounds + extents;

    int initialVertexSize = vertexData.size();
    int initialIndexSize = indexData.size();
    const int vertexSize = int(sizeof(float)) * 8 * 3; // 8 vertices, 3 floats/vert
    quint16 indexAdd = quint16(initialVertexSize / 12);
    vertexData.resize(initialVertexSize + vertexSize);
    const int indexSize = int(sizeof(quint16)) * 12 * 2; // 12 lines, 2 vert/line
    indexData.resize(initialIndexSize + indexSize);

    auto dataPtr = reinterpret_cast<float *>(vertexData.data() + initialVertexSize);
    auto indexPtr = reinterpret_cast<quint16 *>(indexData.data() + initialIndexSize);

    QVector3D corners[8];
    corners[0] = QVector3D(maxAdjBounds.x(), maxAdjBounds.y(), maxAdjBounds.z());
    corners[1] = QVector3D(minAdjBounds.x(), maxAdjBounds.y(), maxAdjBounds.z());
    corners[2] = QVector3D(minAdjBounds.x(), minAdjBounds.y(), maxAdjBounds.z());
    corners[3] = QVector3D(maxAdjBounds.x(), minAdjBounds.y(), maxAdjBounds.z());
    corners[4] = QVector3D(maxAdjBounds.x(), maxAdjBounds.y(), minAdjBounds.z());
    corners[5] = QVector3D(minAdjBounds.x(), maxAdjBounds.y(), minAdjBounds.z());
    corners[6] = QVector3D(minAdjBounds.x(), minAdjBounds.y(), minAdjBounds.z());
    corners[7] = QVector3D(maxAdjBounds.x(), minAdjBounds.y(), minAdjBounds.z());

    for (int i = 0; i < 8; ++i) {
        corners[i] = m.map(corners[i]);
        *dataPtr++ = corners[i].x(); *dataPtr++ = corners[i].y(); *dataPtr++ = corners[i].z();
    }

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

void SelectionBoxGeometry::trackNodeChanges(QQuick3DNode *node)
{
    m_connections << QObject::connect(node, &QQuick3DNode::sceneScaleChanged,
                                      this, &SelectionBoxGeometry::update, Qt::QueuedConnection);
    m_connections << QObject::connect(node, &QQuick3DNode::sceneRotationChanged,
                                      this, &SelectionBoxGeometry::update, Qt::QueuedConnection);
    m_connections << QObject::connect(node, &QQuick3DNode::scenePositionChanged,
                                      this, &SelectionBoxGeometry::update, Qt::QueuedConnection);
    m_connections << QObject::connect(node, &QQuick3DNode::pivotChanged,
                                      this, &SelectionBoxGeometry::update, Qt::QueuedConnection);
}

void SelectionBoxGeometry::targetMeshUpdated()
{
    m_meshUpdatePending = true;
    update();
}

}
}

#endif // QUICK3D_MODULE
