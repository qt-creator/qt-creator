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

#include "camerageometry.h"

#include <QtQuick3DRuntimeRender/private/qssgrendergeometry_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3D/private/qquick3dcustomcamera_p.h>
#include <QtQuick3D/private/qquick3dfrustumcamera_p.h>
#include <QtQuick3D/private/qquick3dorthographiccamera_p.h>
#include <QtQuick3D/private/qquick3dperspectivecamera_p.h>
#include <QtQuick3D/private/qquick3dutils_p.h>
#include <QtCore/qmath.h>
#include <QtCore/qtimer.h>

#include <limits>

namespace QmlDesigner {
namespace Internal {

CameraGeometry::CameraGeometry()
    : QQuick3DGeometry()
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
    update();
}

void CameraGeometry::setViewPortRect(const QRectF &rect)
{
    if (m_viewPortRect == rect)
        return;

    m_viewPortRect = rect;
    emit viewPortRectChanged();
    update();
}

void CameraGeometry::handleCameraPropertyChange()
{
    m_cameraUpdatePending = true;
    update();
}

QSSGRenderGraphObject *CameraGeometry::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!m_camera)
        return node;

    // If camera properties have been updated, we need to defer updating the frustum geometry
    // to the next frame to ensure camera's spatial node has been properly updated.
    if (m_cameraUpdatePending) {
        QTimer::singleShot(0, this, &CameraGeometry::update);
        m_cameraUpdatePending = false;
        return node;
    }

    if (!m_camera->cameraNode()) {
        // Doing explicit viewport mapping forces cameraNode creation
        m_camera->mapToViewport({}, m_viewPortRect.width(), m_viewPortRect.height());
    }

    node = QQuick3DGeometry::updateSpatialNode(node);
    QSSGRenderGeometry *geometry = static_cast<QSSGRenderGeometry *>(node);

    geometry->clear();

    QByteArray vertexData;
    QByteArray indexData;
    QVector3D minBounds;
    QVector3D maxBounds;
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
    QSSGRenderCamera *camera = m_camera->cameraNode();
    if (camera) {
        if (qobject_cast<QQuick3DOrthographicCamera *>(m_camera)) {
            // For some reason ortho cameras show double what projection suggests,
            // so give them doubled viewport to match visualization to actual camera view
            camera->calculateGlobalVariables(QRectF(0, 0, m_viewPortRect.width() * 2.0,
                                               m_viewPortRect.height() * 2.0));
        } else {
            camera->calculateGlobalVariables(m_viewPortRect);
        }
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
