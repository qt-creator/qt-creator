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

#include <QtQuick3D/qquick3dobject.h>
#include <QtQuick3D/private/qquick3dorthographiccamera_p.h>
#include <QtQuick3D/private/qquick3dperspectivecamera_p.h>
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
#include <QtQuick/qquickitem.h>
#include <QtCore/qmath.h>
#include <QtGui/qscreen.h>

namespace QmlDesigner {
namespace Internal {

const QString _globalStateId = QStringLiteral("@GTS"); // global tool state

GeneralHelper::GeneralHelper()
    : QObject()
{
    m_overlayUpdateTimer.setInterval(16);
    m_overlayUpdateTimer.setSingleShot(true);
    QObject::connect(&m_overlayUpdateTimer, &QTimer::timeout,
                     this, &GeneralHelper::overlayUpdateNeeded);

    m_toolStateUpdateTimer.setSingleShot(true);
    QObject::connect(&m_toolStateUpdateTimer, &QTimer::timeout,
                     this, &GeneralHelper::handlePendingToolStateUpdate);
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

    camera->setEulerRotation(startRotation);
    QVector3D newRotation(-dragVector.y(), -dragVector.x(), 0.f);
    newRotation *= 0.5f; // Emprically determined multiplier for nice drag
    newRotation += startRotation;

    camera->setEulerRotation(newRotation);

    const QVector3D oldLookVector = camera->position() - lookAtPoint;
    QMatrix4x4 m = camera->sceneTransform();
    const float *dataPtr(m.data());
    QVector3D newLookVector(dataPtr[8], dataPtr[9], dataPtr[10]);
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
    float newZoomFactor = relative ? qBound(.01f, zoomFactor * multiplier, 100.f)
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
                }
            }
        }
    }

    // Reset camera position to default zoom
    QMatrix4x4 m = camera->sceneTransform();
    const float *dataPtr(m.data());
    QVector3D newLookVector(dataPtr[8], dataPtr[9], dataPtr[10]);
    newLookVector.normalize();
    newLookVector *= defaultLookAtDistance;

    camera->setPosition(lookAt + newLookVector);

    float newZoomFactor = updateZoom ? qBound(.01f, float(maxExtent / 700.), 100.f) : oldZoom;
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

void GeneralHelper::storeToolState(const QString &sceneId, const QString &tool, const QVariant &state,
                                   int delay)
{
    if (delay > 0) {
        QVariantMap sceneToolState;
        sceneToolState.insert(tool, state);
        m_toolStatesPending.insert(sceneId, sceneToolState);
        m_toolStateUpdateTimer.start(delay);
    } else {
        if (m_toolStateUpdateTimer.isActive())
            handlePendingToolStateUpdate();
        QVariant theState;
        // Convert JS arrays to QVariantLists for easier handling down the line
        if (state.canConvert(QMetaType::QVariantList))
            theState = state.value<QVariantList>();
        else
            theState = state;
        QVariantMap &sceneToolState = m_toolStates[sceneId];
        if (sceneToolState[tool] != theState) {
            sceneToolState.insert(tool, theState);
            emit toolStateChanged(sceneId, tool, theState);
        }
    }
}

void GeneralHelper::initToolStates(const QString &sceneId, const QVariantMap &toolStates)
{
    m_toolStates[sceneId] = toolStates;
}

void GeneralHelper::storeWindowState()
{
    if (!m_edit3DWindow)
        return;

    QVariantMap windowState;
    const QRect geometry = m_edit3DWindow->geometry();
    const bool maximized = m_edit3DWindow->windowState() == Qt::WindowMaximized;
    windowState.insert("maximized", maximized);
    windowState.insert("geometry", geometry);

    storeToolState(globalStateId(), "windowState", windowState, 500);
}

void GeneralHelper::restoreWindowState()
{
    if (!m_edit3DWindow)
        return;

    if (m_toolStates.contains(globalStateId())) {
        const QVariantMap &globalStateMap = m_toolStates[globalStateId()];
        const QString stateKey = QStringLiteral("windowState");
        if (globalStateMap.contains(stateKey)) {
            QVariantMap windowState = globalStateMap[stateKey].value<QVariantMap>();

            doRestoreWindowState(windowState);

            // If the mouse cursor at puppet launch time is in a different screen than the one where the
            // view geometry was saved on, the initial position and size can be incorrect, but if
            // we reset the geometry again asynchronously, it should end up with correct geometry.
            QTimer::singleShot(0, [this, windowState]() {
                doRestoreWindowState(windowState);
                QTimer::singleShot(0, [this]() {
                    // Make sure that the window is at least partially visible on the screen
                    QRect geo = m_edit3DWindow->geometry();
                    QRect sRect = m_edit3DWindow->screen()->geometry();
                    if (geo.left() > sRect.right() - 150)
                        geo.moveRight(sRect.right());
                    if (geo.right() < sRect.left() + 150)
                        geo.moveLeft(sRect.left());
                    if (geo.top() > sRect.bottom() - 150)
                        geo.moveBottom(sRect.bottom());
                    if (geo.bottom() < sRect.top() + 150)
                        geo.moveTop(sRect.top());
                    if (geo.width() > sRect.width())
                        geo.setWidth(sRect.width());
                    if (geo.height() > sRect.height())
                        geo.setHeight(sRect.height());
                    m_edit3DWindow->setGeometry(geo);
                });
            });
        }
    }
}

void GeneralHelper::enableItemUpdate(QQuickItem *item, bool enable)
{
    if (item)
        item->setFlag(QQuickItem::ItemHasContents, enable);
}

QVariantMap GeneralHelper::getToolStates(const QString &sceneId)
{
    handlePendingToolStateUpdate();
    if (m_toolStates.contains(sceneId))
        return m_toolStates[sceneId];
    return {};
}

void GeneralHelper::setEdit3DWindow(QQuickWindow *w)
{
    m_edit3DWindow = w;
}

QString GeneralHelper::globalStateId() const
{
    return _globalStateId;
}

void GeneralHelper::doRestoreWindowState(const QVariantMap &windowState)
{
    const QString geoKey = QStringLiteral("geometry");
    if (windowState.contains(geoKey)) {
        bool maximized = false;
        const QString maxKey = QStringLiteral("maximized");
        if (windowState.contains(maxKey))
            maximized = windowState[maxKey].toBool();

        QRect rect = windowState[geoKey].value<QRect>();

        m_edit3DWindow->setGeometry(rect);
        if (maximized)
            m_edit3DWindow->showMaximized();
    }
}

bool GeneralHelper::isMacOS() const
{
#ifdef Q_OS_MACOS
    return true;
#else
    return false;
#endif
}

void GeneralHelper::handlePendingToolStateUpdate()
{
    m_toolStateUpdateTimer.stop();
    auto sceneIt = m_toolStatesPending.constBegin();
    while (sceneIt != m_toolStatesPending.constEnd()) {
        const QVariantMap &sceneToolState = sceneIt.value();
        auto toolIt = sceneToolState.constBegin();
        while (toolIt != sceneToolState.constEnd()) {
            storeToolState(sceneIt.key(), toolIt.key(), toolIt.value());
            ++toolIt;
        }
        ++sceneIt;
    }
    m_toolStatesPending.clear();
}

}
}

#endif // QUICK3D_MODULE
