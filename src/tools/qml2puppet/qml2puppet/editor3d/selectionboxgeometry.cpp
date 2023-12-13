// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef QUICK3D_MODULE

#include "selectionboxgeometry.h"

#include <QtQuick3DRuntimeRender/private/qssgrendermodel_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3D/private/qquick3dmodel_p.h>
#include <QtQuick3D/private/qquick3dscenemanager_p.h>
#include <QtQuick3D/qquick3dobject.h>
#include <QtQuick/qquickwindow.h>
#include <QtCore/qvector.h>

#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#else
#include <QtQuick3DRuntimeRender/ssg/qssgrendercontextcore.h>
#endif

#include <limits>

namespace QmlDesigner {
namespace Internal {

static const float floatMin = std::numeric_limits<float>::lowest();
static const float floatMax = std::numeric_limits<float>::max();
static const QVector3D maxVec = QVector3D(floatMax, floatMax, floatMax);
static const QVector3D minVec = QVector3D(floatMin, floatMin, floatMin);

SelectionBoxGeometry::SelectionBoxGeometry()
    : GeometryBase()
{
}

SelectionBoxGeometry::~SelectionBoxGeometry()
{
    for (auto &connection : std::as_const(m_connections))
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

void SelectionBoxGeometry::setEmpty(bool isEmpty)
{
    if (m_isEmpty != isEmpty) {
        m_isEmpty = isEmpty;
        emit isEmptyChanged();
    }
}

QSSGBounds3 SelectionBoxGeometry::bounds() const
{
    return m_bounds;
}

void SelectionBoxGeometry::clearGeometry()
{
    clear();
    setStride(12); // To avoid div by zero inside QtQuick3D
    setEmpty(true);
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
                         this, &SelectionBoxGeometry::spatialNodeUpdateNeeded, Qt::QueuedConnection);
        QObject::connect(model, &QQuick3DModel::geometryChanged,
                         this, &SelectionBoxGeometry::spatialNodeUpdateNeeded, Qt::QueuedConnection);
    }
    if (m_targetNode) {
        QObject::connect(m_targetNode, &QQuick3DNode::parentChanged,
                         this, &SelectionBoxGeometry::spatialNodeUpdateNeeded, Qt::QueuedConnection);
    }

    clearGeometry();
    emit targetNodeChanged();
    spatialNodeUpdateNeeded();
}

void SelectionBoxGeometry::setRootNode(QQuick3DNode *rootNode)
{
    if (m_rootNode == rootNode)
        return;

    m_rootNode = rootNode;

    emit rootNodeChanged();
    spatialNodeUpdateNeeded();
}

void SelectionBoxGeometry::setView3D(QQuick3DViewport *view)
{
    if (m_view3D == view)
        return;

    m_view3D = view;

    emit view3DChanged();
    spatialNodeUpdateNeeded();
}

QSSGRenderGraphObject *SelectionBoxGeometry::updateSpatialNode(QSSGRenderGraphObject *node)
{

    if (m_spatialNodeUpdatePending) {
        m_spatialNodeUpdatePending = false;
        updateGeometry();
    }

#if QT_VERSION_MAJOR == 6 && QT_VERSION_MINOR == 4
    if (!node) {
        markAllDirty();
        auto geometryNode = new QSSGRenderGeometry();
        node = geometryNode;
        emit geometryNodeDirty();

        // This is a work around for the issue of incorrect geometry objects getting matched for
        // cached mesh data in QSSGBufferManager::loadRenderMesh in QtQuick3D in 6.4 (see QDS-8843).
        // Each setting of stride value increments the generation id of the geometry node.
        // By incrementing generation id by different amounts, we can ensure QSSGBufferManager cache
        // never matches wrong mesh data.
        // The cache should be cleared of old objects after they are unused for one frame.
        // With puppet reset & multiselection, we can create multiple new boxes per frame,
        // so there's no count that really guarantees there are no invalid cache matches, but
        // even just 8 should make them very unlikely.
        // We start count at 12 here to avoid overlapping with gridgeometry cache ids.
        static int dirtyCount = 12;
        if (++dirtyCount > 20)
            dirtyCount = 12;
        for (int i = 0; i < dirtyCount; ++i)
            geometryNode->setStride(stride());
    }
#endif

    return QQuick3DGeometry::updateSpatialNode(node);
}

void SelectionBoxGeometry::doUpdateGeometry()
{
    // Some changes require a frame to be rendered for us to be able to calculate geometry,
    // so defer calculations until after next frame.
    if (m_spatialNodeUpdatePending) {
        update();
        return;
    }

    GeometryBase::doUpdateGeometry();

    for (auto &connection : std::as_const(m_connections))
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
            rootRN->markDirty(QSSGRenderNode::DirtyFlag::TransformDirty);
            rootRN->calculateGlobalVariables();
        } else if (!m_spatialNodeUpdatePending) {
            // Necessary spatial nodes do not yet exist. Defer selection box creation one frame.
            m_spatialNodeUpdatePending = true;
            update();
        }
        getBounds(m_targetNode, vertexData, indexData, minBounds, maxBounds);
        generateVertexData(vertexData, indexData, minBounds, maxBounds);

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
        generateVertexData(vertexData, indexData, minBounds, maxBounds);
    }

    addAttribute(QQuick3DGeometry::Attribute::IndexSemantic, 0,
                 QQuick3DGeometry::Attribute::U16Type);
    setVertexData(vertexData);
    setIndexData(indexData);
    setBounds(minBounds, maxBounds);

    m_bounds = QSSGBounds3(minBounds, maxBounds);

    setEmpty(minBounds.isNull() && maxBounds.isNull());
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
            if (renderNode->isDirty(QSSGRenderNode::DirtyFlag::TransformDirty)) {
                renderNode->localTransform = QSSGRenderNode::calculateTransformMatrix(
                            node->position(), node->scale(), node->pivot(), node->rotation());
            }
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
    for (const auto &newBounds : std::as_const(minBoundsVec))
        combineMinBounds(localMinBounds, newBounds);
    for (const auto &newBounds : std::as_const(maxBoundsVec))
        combineMaxBounds(localMaxBounds, newBounds);

    if (qobject_cast<QQuick3DModel *>(node)) {
        if (auto renderModel = static_cast<QSSGRenderModel *>(renderNode)) {
            QWindow *window = static_cast<QWindow *>(m_view3D->window());
            if (window) {
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 1)
                QSSGRef<QSSGRenderContextInterface> context;
                context = QQuick3DObjectPrivate::get(this)->sceneManager->rci;
                if (!context.isNull()) {
#else
                const auto &sm = QQuick3DObjectPrivate::get(this)->sceneManager;
                auto context = sm->wattached ? sm->wattached->rci().get() : nullptr;
                if (context) {
#endif
                    const auto &bufferManager(context->bufferManager());
                    QSSGBounds3 bounds = bufferManager->getModelBounds(renderModel);
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
}

void SelectionBoxGeometry::generateVertexData(QByteArray &vertexData, QByteArray &indexData,
                                              const QVector3D &minBounds, const QVector3D &maxBounds)
{
    // Adjust bounds to reduce targetNode pixels obscuring the selection box
    QVector3D extents = (maxBounds - minBounds) / 1000.f;
    QVector3D minAdjBounds = minBounds - extents;
    QVector3D maxAdjBounds = maxBounds + extents;

    // Selection box has 8 corners with three short lines towards other corners on each corner
    const int vertexSize = int(sizeof(float)) * 8 * 4 * 3; // 8 corners, 4 verts/corner, 3 floats/vert
    vertexData.resize(vertexSize);
    const int indexSize = int(sizeof(quint16)) * 8 * 3 * 2; // 8 * 3 lines, 2 vert/line
    indexData.resize(indexSize);

    auto dataPtr = reinterpret_cast<float *>(vertexData.data());
    auto indexPtr = reinterpret_cast<quint16 *>(indexData.data());

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
        *dataPtr++ = corners[i].x();
        *dataPtr++ = corners[i].y();
        *dataPtr++ = corners[i].z();
    }

    // Percentage of full box lines to show at each corner. Set to .5 for full box
    static const float lineLen = .15f;

    int nextVertIdx = 8;

    // Add line from corner 'start', towards corner 'end'
    auto addCornerLine = [&](int start, int end) {
        QVector3D vert = corners[start] + lineLen * (corners[end] - corners[start]);
        *dataPtr++ = vert.x(); *dataPtr++ = vert.y(); *dataPtr++ = vert.z();
        *indexPtr++ = start; *indexPtr++ = nextVertIdx++;
    };

    addCornerLine(0, 1);
    addCornerLine(0, 3);
    addCornerLine(0, 4);

    addCornerLine(1, 0);
    addCornerLine(1, 2);
    addCornerLine(1, 5);

    addCornerLine(2, 1);
    addCornerLine(2, 3);
    addCornerLine(2, 6);

    addCornerLine(3, 0);
    addCornerLine(3, 2);
    addCornerLine(3, 7);

    addCornerLine(4, 0);
    addCornerLine(4, 5);
    addCornerLine(4, 7);

    addCornerLine(5, 1);
    addCornerLine(5, 4);
    addCornerLine(5, 6);

    addCornerLine(6, 2);
    addCornerLine(6, 5);
    addCornerLine(6, 7);

    addCornerLine(7, 3);
    addCornerLine(7, 4);
    addCornerLine(7, 6);
}

void SelectionBoxGeometry::trackNodeChanges(QQuick3DNode *node)
{
    m_connections << QObject::connect(node, &QQuick3DNode::sceneScaleChanged,
                                      this, &SelectionBoxGeometry::updateGeometry, Qt::QueuedConnection);
    m_connections << QObject::connect(node, &QQuick3DNode::sceneRotationChanged,
                                      this, &SelectionBoxGeometry::updateGeometry, Qt::QueuedConnection);
    m_connections << QObject::connect(node, &QQuick3DNode::scenePositionChanged,
                                      this, &SelectionBoxGeometry::updateGeometry, Qt::QueuedConnection);
    m_connections << QObject::connect(node, &QQuick3DNode::pivotChanged,
                                      this, &SelectionBoxGeometry::updateGeometry, Qt::QueuedConnection);
}

void SelectionBoxGeometry::spatialNodeUpdateNeeded()
{
    m_spatialNodeUpdatePending = true;
    clearGeometry();
    update();
}

}
}

#endif // QUICK3D_MODULE
