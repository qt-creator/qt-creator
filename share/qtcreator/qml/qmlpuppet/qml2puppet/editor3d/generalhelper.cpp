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
#include "generalhelper.h"

#ifdef QUICK3D_MODULE

#include "selectionboxgeometry.h"

#include <QtQuick3D/private/qquick3dorthographiccamera_p.h>
#include <QtQuick3D/private/qquick3dperspectivecamera_p.h>
#include <QtQuick3D/private/qquick3dobject_p_p.h>
#include <QtQuick3D/private/qquick3dcamera_p.h>
#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dmodel_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3D/private/qquick3ddefaultmaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermodel_p.h>
#include <QtQuick3DUtils/private/qssgbounds3_p.h>
#include <QtQuick/qquickwindow.h>
#include <QtCore/qmath.h>

namespace QmlDesigner {
namespace Internal {

GeneralHelper::GeneralHelper()
    : QObject()
{
    m_overlayUpdateTimer.setInterval(16);
    m_overlayUpdateTimer.setSingleShot(true);
    QObject::connect(&m_overlayUpdateTimer, &QTimer::timeout,
                     this, &GeneralHelper::overlayUpdateNeeded);
}

void GeneralHelper::requestOverlayUpdate()
{
    if (!m_overlayUpdateTimer.isActive())
        m_overlayUpdateTimer.start();
}

QString GeneralHelper::generateUniqueName(const QString &nameRoot)
{
    static QHash<QString, int> counters;
    int count = counters[nameRoot]++;
    return QStringLiteral("%1_%2").arg(nameRoot).arg(count);
}

void GeneralHelper::orbitCamera(QQuick3DCamera *camera, const QVector3D &startRotation,
                                const QVector3D &lookAtPoint, const QVector3D &pressPos,
                                const QVector3D &currentPos)
{
    QVector3D dragVector = currentPos - pressPos;

    if (dragVector.length() < 0.001f)
        return;

    camera->setRotation(startRotation);
    QVector3D newRotation(dragVector.y(), dragVector.x(), 0.f);
    newRotation *= 0.5f; // Emprically determined multiplier for nice drag
    newRotation += startRotation;

    camera->setRotation(newRotation);

    const QVector3D oldLookVector = camera->position() - lookAtPoint;
    QMatrix4x4 m = camera->sceneTransform();
    const float *dataPtr(m.data());
    QVector3D newLookVector(-dataPtr[8], -dataPtr[9], -dataPtr[10]);
    newLookVector.normalize();
    newLookVector *= oldLookVector.length();

    camera->setPosition(lookAtPoint + newLookVector);
}

// Pans camera and returns the new look-at point
QVector3D GeneralHelper::panCamera(QQuick3DCamera *camera, const QMatrix4x4 startTransform,
                                   const QVector3D &startPosition, const QVector3D &startLookAt,
                                   const QVector3D &pressPos, const QVector3D &currentPos,
                                   float zoomFactor)
{
    QVector3D dragVector = currentPos - pressPos;

    if (dragVector.length() < 0.001f)
        return startLookAt;

    const float *dataPtr(startTransform.data());
    const QVector3D xAxis = QVector3D(dataPtr[0], dataPtr[1], dataPtr[2]).normalized();
    const QVector3D yAxis = QVector3D(dataPtr[4], dataPtr[5], dataPtr[6]).normalized();
    const QVector3D xDelta = -1.f * xAxis * dragVector.x();
    const QVector3D yDelta = yAxis * dragVector.y();
    const QVector3D delta = (xDelta + yDelta) * zoomFactor;

    camera->setPosition(startPosition + delta);
    return startLookAt + delta;
}

float GeneralHelper::zoomCamera(QQuick3DCamera *camera, float distance, float defaultLookAtDistance,
                                const QVector3D &lookAt, float zoomFactor, bool relative)
{
    // Emprically determined divisor for nice zoom
    float multiplier = 1.f + (distance / 40.f);
    float newZoomFactor = relative ? qBound(.0001f, zoomFactor * multiplier, 10000.f)
                                   : zoomFactor;

    if (qobject_cast<QQuick3DOrthographicCamera *>(camera)) {
        // Ortho camera we can simply scale
        camera->setScale(QVector3D(newZoomFactor, newZoomFactor, newZoomFactor));
    } else if (qobject_cast<QQuick3DPerspectiveCamera *>(camera)) {
        // Perspective camera is zoomed by moving camera forward or backward while keeping the
        // look-at point the same
        const QVector3D lookAtVec = (camera->position() - lookAt).normalized();
        const float newDistance = defaultLookAtDistance * newZoomFactor;
        camera->setPosition(lookAt + (lookAtVec * newDistance));
    }

    return newZoomFactor;
}

// Return value contains new lookAt point (xyz) and zoom factor (w)
QVector4D GeneralHelper::focusObjectToCamera(QQuick3DCamera *camera, float defaultLookAtDistance,
                                             QQuick3DNode *targetObject, QQuick3DViewport *viewPort,
                                             float oldZoom, bool updateZoom)
{
    if (!camera)
        return QVector4D(0.f, 0.f, 0.f, 1.f);

    QVector3D lookAt = targetObject ? targetObject->scenePosition() : QVector3D();

    // Get object bounds
    const qreal defaultExtent = 200.;
    qreal maxExtent = defaultExtent;
    if (auto modelNode = qobject_cast<QQuick3DModel *>(targetObject)) {
        auto targetPriv = QQuick3DObjectPrivate::get(targetObject);
        if (auto renderModel = static_cast<QSSGRenderModel *>(targetPriv->spatialNode)) {
            QWindow *window = static_cast<QWindow *>(viewPort->window());
            if (window) {
                auto context = QSSGRenderContextInterface::getRenderContextInterface(quintptr(window));
                if (!context.isNull()) {
                    QSSGBounds3 bounds;
                    auto geometry = qobject_cast<SelectionBoxGeometry *>(modelNode->geometry());
                    if (geometry) {
                        bounds = geometry->bounds();
                    } else {
                        auto bufferManager = context->bufferManager();
                        bounds = renderModel->getModelBounds(bufferManager);
                    }

                    QVector3D center = bounds.center();
                    const QVector3D e = bounds.extents();
                    const QVector3D s = targetObject->sceneScale();
                    qreal maxScale = qSqrt(qreal(s.x() * s.x() + s.y() * s.y() + s.z() * s.z()));
                    maxExtent = qSqrt(qreal(e.x() * e.x() + e.y() * e.y() + e.z() * e.z()));
                    maxExtent *= maxScale;

                    if (maxExtent < 0.0001)
                        maxExtent = defaultExtent;

                    // Adjust lookAt to look directly at the center of the object bounds
                    lookAt = renderModel->globalTransform.map(center);
                    lookAt.setZ(-lookAt.z()); // Render node transforms have inverted z
                }
            }
        }
    }

    // Reset camera position to default zoom
    QMatrix4x4 m = camera->sceneTransform();
    const float *dataPtr(m.data());
    QVector3D newLookVector(-dataPtr[8], -dataPtr[9], -dataPtr[10]);
    newLookVector.normalize();
    newLookVector *= defaultLookAtDistance;

    camera->setPosition(lookAt + newLookVector);

    float newZoomFactor = updateZoom ? qBound(.0001f, float(maxExtent / 700.), 10000.f) : oldZoom;
    float cameraZoomFactor = zoomCamera(camera, 0, defaultLookAtDistance, lookAt, newZoomFactor, false);

    return QVector4D(lookAt, cameraZoomFactor);
}

void GeneralHelper::delayedPropertySet(QObject *obj, int delay, const QString &property,
                                       const QVariant &value)
{
    QTimer::singleShot(delay, [obj, property, value]() {
        obj->setProperty(property.toLatin1().constData(), value);
    });
}

QQuick3DNode *GeneralHelper::resolvePick(QQuick3DNode *pickNode)
{
    if (pickNode) {
        // Check if the picked node actually specifies another node as the pick target
        QVariant componentVar = pickNode->property("_pickTarget");
        if (componentVar.isValid()) {
            auto componentNode = componentVar.value<QQuick3DNode *>();
            if (componentNode)
                return componentNode;
        }
    }
    return pickNode;
}

bool GeneralHelper::isMacOS() const
{
#ifdef Q_OS_MACOS
    return true;
#else
    return false;
#endif
}

}
}

#endif // QUICK3D_MODULE
