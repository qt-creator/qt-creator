// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef QUICK3D_MODULE

#include "camerageometry.h"

#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3D/private/qquick3dcustomcamera_p.h>
#include <QtQuick3D/private/qquick3dfrustumcamera_p.h>
#include <QtQuick3D/private/qquick3dorthographiccamera_p.h>
#include <QtQuick3D/private/qquick3dperspectivecamera_p.h>
#include <QtQuick3D/private/qquick3dutils_p.h>
#include <QtCore/qmath.h>

#include <limits>

namespace QmlDesigner {
namespace Internal {

CameraGeometry::CameraGeometry()
    : GeometryBase()
{
}

CameraGeometry::~CameraGeometry()
{
}

QQuick3DCamera *CameraGeometry::camera() const
{
    return m_camera;
}

QRectF CameraGeometry::viewPortRect() const
{
    return m_viewPortRect;
}

void CameraGeometry::setCamera(QQuick3DCamera *camera)
{
    if (m_camera == camera)
        return;

    if (m_camera)
        m_camera->disconnect(this);
    m_camera = camera;
    if (auto perspectiveCamera = qobject_cast<QQuick3DPerspectiveCamera *>(m_camera)) {
        QObject::connect(perspectiveCamera, &QQuick3DPerspectiveCamera::clipNearChanged,
                         this, &CameraGeometry::handleCameraPropertyChange);
        QObject::connect(perspectiveCamera, &QQuick3DPerspectiveCamera::clipFarChanged,
                         this, &CameraGeometry::handleCameraPropertyChange);
        QObject::connect(perspectiveCamera, &QQuick3DPerspectiveCamera::fieldOfViewChanged,
                         this, &CameraGeometry::handleCameraPropertyChange);
        QObject::connect(perspectiveCamera, &QQuick3DPerspectiveCamera::fieldOfViewOrientationChanged,
                         this, &CameraGeometry::handleCameraPropertyChange);
        if (auto frustumCamera = qobject_cast<QQuick3DFrustumCamera *>(m_camera)) {
            QObject::connect(frustumCamera, &QQuick3DFrustumCamera::topChanged,
                             this, &CameraGeometry::handleCameraPropertyChange);
            QObject::connect(frustumCamera, &QQuick3DFrustumCamera::bottomChanged,
                             this, &CameraGeometry::handleCameraPropertyChange);
            QObject::connect(frustumCamera, &QQuick3DFrustumCamera::rightChanged,
                             this, &CameraGeometry::handleCameraPropertyChange);
            QObject::connect(frustumCamera, &QQuick3DFrustumCamera::leftChanged,
                             this, &CameraGeometry::handleCameraPropertyChange);
        }
    } else if (auto orthoCamera = qobject_cast<QQuick3DOrthographicCamera *>(m_camera)) {
        QObject::connect(orthoCamera, &QQuick3DOrthographicCamera::clipNearChanged,
                         this, &CameraGeometry::handleCameraPropertyChange);
        QObject::connect(orthoCamera, &QQuick3DOrthographicCamera::clipFarChanged,
                         this, &CameraGeometry::handleCameraPropertyChange);
    } else if (auto customCamera = qobject_cast<QQuick3DCustomCamera *>(m_camera)) {
        QObject::connect(customCamera, &QQuick3DCustomCamera::projectionChanged,
                         this, &CameraGeometry::handleCameraPropertyChange);
    }
    emit cameraChanged();
    handleCameraPropertyChange();
}

void CameraGeometry::setViewPortRect(const QRectF &rect)
{
    if (m_viewPortRect == rect)
        return;

    m_viewPortRect = rect;
    emit viewPortRectChanged();
    updateGeometry();
}

void CameraGeometry::handleCameraPropertyChange()
{
    m_cameraUpdatePending = true;
    clear();
    setStride(12); // To avoid div by zero inside QtQuick3D
    update();
}

QSSGRenderGraphObject *CameraGeometry::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (m_cameraUpdatePending) {
        m_cameraUpdatePending = false;
        updateGeometry();
    }

    return QQuick3DGeometry::updateSpatialNode(node);
}

void CameraGeometry::doUpdateGeometry()
{
    if (!m_camera)
        return;

    // If camera properties have been updated, we need to defer updating the frustum geometry
    // to the next frame to ensure camera's spatial node has been properly updated.
    if (m_cameraUpdatePending) {
        update();
        return;
    }

    if (!QQuick3DObjectPrivate::get(m_camera)->spatialNode) {
        // Doing explicit viewport mapping forces cameraNode creation
        m_camera->mapToViewport({}, m_viewPortRect.width(), m_viewPortRect.height());
    }

    GeometryBase::doUpdateGeometry();

    QByteArray vertexData;
    QByteArray indexData;
    QVector3D minBounds;
    QVector3D maxBounds;
    fillVertexData(vertexData, indexData, minBounds, maxBounds);

    addAttribute(QQuick3DGeometry::Attribute::IndexSemantic, 0,
                 QQuick3DGeometry::Attribute::U16Type);
    setVertexData(vertexData);
    setIndexData(indexData);
    setBounds(minBounds, maxBounds);
}

void CameraGeometry::fillVertexData(QByteArray &vertexData, QByteArray &indexData,
                                    QVector3D &minBounds, QVector3D &maxBounds)
{
    const int vertexSize = int(sizeof(float)) * 8 * 3; // 8 vertices, 3 floats/vert
    vertexData.resize(vertexSize);
    const int indexSize = int(sizeof(quint16)) * 12 * 2; // 12 lines, 2 vert/line
    indexData.resize(indexSize);

    auto dataPtr = reinterpret_cast<float *>(vertexData.data());
    auto indexPtr = reinterpret_cast<quint16 *>(indexData.data());

    QMatrix4x4 m;
    QSSGRenderCamera *camera = static_cast<QSSGRenderCamera *>(QQuick3DObjectPrivate::get(m_camera)->spatialNode);
    if (camera) {
        QRectF rect = m_viewPortRect;
        if (rect.isNull())
             rect = QRectF(0, 0, 1000, 1000); // Let's have some visualization for null viewports
        camera->calculateGlobalVariables(rect);
        m = camera->projection.inverted();
    }

    const QVector3D farTopLeft = m * QVector3D(1.f, -1.f, 1.f);
    const QVector3D farBottomRight = m * QVector3D(-1.f, 1.f, 1.f);
    const QVector3D nearTopLeft = m * QVector3D(1.f, -1.f, -1.f);
    const QVector3D nearBottomRight = m * QVector3D(-1.f, 1.f, -1.f);

    *dataPtr++ = nearTopLeft.x();     *dataPtr++ = nearBottomRight.y(); *dataPtr++ = nearTopLeft.z();
    *dataPtr++ = nearTopLeft.x();     *dataPtr++ = nearTopLeft.y();     *dataPtr++ = nearTopLeft.z();
    *dataPtr++ = nearBottomRight.x(); *dataPtr++ = nearTopLeft.y();     *dataPtr++ = nearTopLeft.z();
    *dataPtr++ = nearBottomRight.x(); *dataPtr++ = nearBottomRight.y(); *dataPtr++ = nearTopLeft.z();
    *dataPtr++ = farTopLeft.x();      *dataPtr++ = farBottomRight.y();  *dataPtr++ = farTopLeft.z();
    *dataPtr++ = farTopLeft.x();      *dataPtr++ = farTopLeft.y();      *dataPtr++ = farTopLeft.z();
    *dataPtr++ = farBottomRight.x();  *dataPtr++ = farTopLeft.y();      *dataPtr++ = farTopLeft.z();
    *dataPtr++ = farBottomRight.x();  *dataPtr++ = farBottomRight.y();  *dataPtr++ = farTopLeft.z();

    // near rect
    *indexPtr++ = 0; *indexPtr++ = 1;
    *indexPtr++ = 1; *indexPtr++ = 2;
    *indexPtr++ = 2; *indexPtr++ = 3;
    *indexPtr++ = 3; *indexPtr++ = 0;
    // near to far
    *indexPtr++ = 0; *indexPtr++ = 4;
    *indexPtr++ = 1; *indexPtr++ = 5;
    *indexPtr++ = 2; *indexPtr++ = 6;
    *indexPtr++ = 3; *indexPtr++ = 7;
    // far rect
    *indexPtr++ = 4; *indexPtr++ = 5;
    *indexPtr++ = 5; *indexPtr++ = 6;
    *indexPtr++ = 6; *indexPtr++ = 7;
    *indexPtr++ = 7; *indexPtr++ = 4;

    static const float floatMin = std::numeric_limits<float>::lowest();
    static const float floatMax = std::numeric_limits<float>::max();
    auto vertexPtr = reinterpret_cast<QVector3D *>(vertexData.data());
    minBounds = QVector3D(floatMax, floatMax, floatMax);
    maxBounds = QVector3D(floatMin, floatMin, floatMin);
    for (int i = 0; i < vertexSize / 12; ++i) {
        minBounds[0] = qMin((*vertexPtr)[0], minBounds[0]);
        minBounds[1] = qMin((*vertexPtr)[1], minBounds[1]);
        minBounds[2] = qMin((*vertexPtr)[2], minBounds[2]);
        maxBounds[0] = qMax((*vertexPtr)[0], maxBounds[0]);
        maxBounds[1] = qMax((*vertexPtr)[1], maxBounds[1]);
        maxBounds[2] = qMax((*vertexPtr)[2], maxBounds[2]);
        ++vertexPtr;
    }
}

}
}

#endif // QUICK3D_MODULE
