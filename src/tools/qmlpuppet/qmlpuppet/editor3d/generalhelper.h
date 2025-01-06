// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifdef QUICK3D_MODULE

#include <QColor>
#include <QHash>
#include <QMatrix4x4>
#include <QObject>
#include <QPointer>
#include <QPointF>
#include <QQuaternion>
#include <QTimer>
#include <QUrl>
#include <QVariant>
#include <QVector3D>
#include <QtQuick3D/private/qquick3dcubemaptexture_p.h>
#include <QtQuick3D/private/qquick3dpickresult_p.h>
#include <QtQuick3D/private/qquick3dsceneenvironment_p.h>
#include <QtQuick3D/private/qquick3dtexture_p.h>

QT_BEGIN_NAMESPACE
class QQuick3DCamera;
class QQuick3DNode;
class QQuick3DViewport;
class QQuick3DMaterial;
class QQuickItem;
QT_END_NAMESPACE

namespace QmlDesigner {
namespace Internal {

class GeneralHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isMacOS READ isMacOS CONSTANT)
    Q_PROPERTY(QVariant bgColor READ bgColor NOTIFY bgColorChanged FINAL)
    Q_PROPERTY(double minGridStep READ minGridStep NOTIFY minGridStepChanged FINAL)
    Q_PROPERTY(double cameraSpeed READ cameraSpeed NOTIFY cameraSpeedChanged FINAL)

    Q_PROPERTY(
        QQuick3DCamera *activeScenePreferredCamera
        READ activeScenePreferredCamera
        NOTIFY activeScenePreferredCameraChanged FINAL)

public:
    GeneralHelper();

    Q_INVOKABLE void requestOverlayUpdate();
    Q_INVOKABLE QString generateUniqueName(const QString &nameRoot);
    Q_INVOKABLE QUrl resolveAbsoluteSourceUrl(const QQuick3DModel *sourceModel);

    Q_INVOKABLE void orbitCamera(QQuick3DCamera *camera, const QVector3D &startRotation,
                                 const QVector3D &lookAtPoint, const QVector3D &pressPos,
                                 const QVector3D &currentPos);
    Q_INVOKABLE QVector3D panCamera(QQuick3DCamera *camera, const QMatrix4x4 startTransform,
                                    const QVector3D &startPosition, const QVector3D &startLookAt,
                                    const QVector3D &pressPos, const QVector3D &currentPos,
                                    float zoomFactor);
    Q_INVOKABLE QVector3D moveCamera(QQuick3DCamera *camera,const QVector3D &startLookAt,
                                     const QVector3D &moveVector);
    Q_INVOKABLE QVector3D rotateCamera(QQuick3DCamera *camera, const QPointF &angles,
                                       const QVector3D &lookAtPoint);

    Q_INVOKABLE void startCameraMove(QQuick3DCamera *camera, const QVector3D moveVector);
    Q_INVOKABLE void stopCameraMove(const QVector3D moveVector);
    Q_INVOKABLE void stopAllCameraMoves();

    Q_INVOKABLE float zoomCamera(QQuick3DViewport *viewPort, QQuick3DCamera *camera, float distance,
                                 float defaultLookAtDistance, const QVector3D &lookAt,
                                 float zoomFactor, bool relative);
    Q_INVOKABLE QVector4D focusNodesToCamera(QQuick3DCamera *camera, float defaultLookAtDistance,
                                             const QVariant &nodes, QQuick3DViewport *viewPort,
                                             float oldZoom, bool updateZoom = true,
                                             bool closeUp = false);
    Q_INVOKABLE QVector4D approachNode(QQuick3DCamera *camera, float defaultLookAtDistance,
                                       QObject *node, QQuick3DViewport *viewPort);
    void calculateBoundsAndFocusCamera(QQuick3DCamera *camera, QQuick3DNode *node,
                                       QQuick3DViewport *viewPort, float defaultLookAtDistance,
                                       bool closeUp, QVector3D &lookAt, QVector3D &extents);
    Q_INVOKABLE void calculateNodeBoundsAndFocusCamera(QQuick3DCamera *camera, QQuick3DNode *node,
                                                       QQuick3DViewport *viewPort,
                                                       float defaultLookAtDistance, bool closeUp);
    Q_INVOKABLE void alignCameras(QQuick3DCamera *camera, const QVariant &nodes);
    Q_INVOKABLE QVector4D alignView(QQuick3DCamera *camera, const QVariant &nodes,
                                    const QVector3D &lookAtPoint, float defaultLookAtDistance);
    Q_INVOKABLE bool fuzzyCompare(double a, double b);
    Q_INVOKABLE void delayedPropertySet(QObject *obj, int delay, const QString &property,
                                        const QVariant& value);
    Q_INVOKABLE QQuick3DPickResult pickViewAt(QQuick3DViewport *view, float posX, float posY);
    Q_INVOKABLE QObject *resolvePick(QQuick3DNode *pickNode);

    Q_INVOKABLE bool isLocked(QQuick3DNode *node) const;
    Q_INVOKABLE bool isHidden(QQuick3DNode *node) const;
    Q_INVOKABLE bool isPickable(QQuick3DNode *node) const;
    Q_INVOKABLE bool isCamera(QQuick3DNode *node) const;
    Q_INVOKABLE bool isOrthographicCamera(QQuick3DNode *node) const;
    Q_INVOKABLE bool isSceneObject(QQuick3DNode *node) const;
    Q_INVOKABLE QQuick3DNode *createParticleEmitterGizmoModel(QQuick3DNode *emitter,
                                                              QQuick3DMaterial *material) const;

    Q_INVOKABLE void storeToolState(const QString &sceneId, const QString &tool,
                                    const QVariant &state, int delayEmit = 0);
    void initToolStates(const QString &sceneId, const QVariantMap &toolStates);
    Q_INVOKABLE void enableItemUpdate(QQuickItem *item, bool enable);
    Q_INVOKABLE QVariantMap getToolStates(const QString &sceneId);
    QString globalStateId() const;
    QString projectStateId() const;
    QString lastSceneIdKey() const;
    QString lastSceneEnvKey() const;
    QString rootSizeKey() const;

    Q_INVOKABLE void setMultiSelectionTargets(QQuick3DNode *multiSelectRootNode,
                                              const QVariantList &selectedList);
    Q_INVOKABLE void resetMultiSelectionNode();
    Q_INVOKABLE void restartMultiSelection();
    Q_INVOKABLE QVariantList multiSelectionTargets() const;
    Q_INVOKABLE void moveMultiSelection(bool commit);
    Q_INVOKABLE void scaleMultiSelection(bool commit);
    Q_INVOKABLE void rotateMultiSelection(bool commit);

    void setSceneEnvironmentData(const QString &sceneId, QQuick3DSceneEnvironment *env);
    Q_INVOKABLE bool hasSceneEnvironmentData(const QString &sceneId) const;
    Q_INVOKABLE QQuick3DSceneEnvironment::QQuick3DEnvironmentBackgroundTypes sceneEnvironmentBgMode(
        const QString &sceneId) const;
    Q_INVOKABLE QColor sceneEnvironmentColor(const QString &sceneId) const;
    Q_INVOKABLE QQuick3DTexture *sceneEnvironmentLightProbe(const QString &sceneId) const;
    Q_INVOKABLE QQuick3DCubeMapTexture *sceneEnvironmentSkyBoxCubeMap(const QString &sceneId) const;
    Q_INVOKABLE void updateSceneEnvToLast(QQuick3DSceneEnvironment *env, QQuick3DTexture *lightProbe,
                                          QQuick3DCubeMapTexture *cubeMap);
    Q_INVOKABLE bool sceneHasLightProbe(const QString &sceneId);

    void clearSceneEnvironmentData();
    void setLastSceneEnvironmentData(const QVariantMap &data);

    bool isMacOS() const;

    void addRotationBlocks(const QSet<QQuick3DNode *> &nodes);
    void removeRotationBlocks(const QSet<QQuick3DNode *> &nodes);
    Q_INVOKABLE bool isRotationBlocked(QQuick3DNode *node) const;

    Q_INVOKABLE QVector3D adjustTranslationForSnap(const QVector3D &newPos,
                                                   const QVector3D &startPos,
                                                   const QVector3D &snapAxes,
                                                   bool globalOrientation,
                                                   QQuick3DNode *node);
    Q_INVOKABLE double adjustRotationForSnap(double newAngle);
    Q_INVOKABLE double adjustScalerForSnap(double newScale);
    QVector3D adjustScaleForSnap(const QVector3D &newScale);

    void setSnapAbsolute(bool enable) { m_snapAbsolute = enable; }
    void setSnapPosition(bool enable) { m_snapPosition = enable; }
    void setSnapRotation(bool enable) { m_snapRotation = enable; }
    void setSnapScale(bool enable) { m_snapScale = enable; }
    void setSnapPositionInterval(double interval);
    void setSnapRotationInterval(double interval) { m_snapRotationInterval = interval; }
    void setSnapScaleInterval(double interval) { m_snapScaleInterval = interval / 100.; }
    void setCameraSpeed(double speed);
    Q_INVOKABLE void setCameraSpeedModifier(double modifier);

    Q_INVOKABLE QString snapPositionDragTooltip(const QVector3D &pos) const;
    Q_INVOKABLE QString snapRotationDragTooltip(double angle) const;
    Q_INVOKABLE QString snapScaleDragTooltip(const QVector3D &scale) const;

    double minGridStep() const;
    double cameraSpeed() const { return m_cameraSpeed; }

    void setBgColor(const QVariant &colors);
    QVariant bgColor() const { return m_bgColor; }

    Q_INVOKABLE QVector3D dirForRotation(const QQuaternion &rotation) const;
    Q_INVOKABLE bool compareVectors(const QVector3D &v1, const QVector3D &v2) const;
    Q_INVOKABLE bool compareQuaternions(const QQuaternion &q1, const QQuaternion &q2) const;

    Q_INVOKABLE void requestTimerEvent(const QString &timerId, qint64 delay);

    QQuick3DCamera *activeScenePreferredCamera() const;
    void setActiveScenePreferredCamera(QQuick3DCamera *camera);

signals:
    void overlayUpdateNeeded();
    void toolStateChanged(const QString &sceneId, const QString &tool, const QVariant &toolState);
    void hiddenStateChanged(QQuick3DNode *node);
    void lockedStateChanged(QQuick3DNode *node);
    void rotationBlocksChanged();
    void bgColorChanged();
    void minGridStepChanged();
    void updateDragTooltip();
    void sceneEnvDataChanged();
    void requestCameraMove(QQuick3DCamera *camera, const QVector3D &moveVector);
    void requestRender();
    void requestActiveScenePreferredCamera();
    void cameraSpeedChanged();
    void requestedTimerEvent(const QString &timerId);
    void activeScenePreferredCameraChanged();

private:
    void handlePendingToolStateUpdate();
    QVector3D pivotScenePosition(QQuick3DNode *node) const;
    bool getBounds(QQuick3DViewport *view3D, QQuick3DNode *node, QVector3D &minBounds,
                   QVector3D &maxBounds);
    QString formatVectorDragTooltip(const QVector3D &vec, const QString &suffix) const;
    QString formatSnapStr(bool snapEnabled, double increment, const QString &suffix) const;

    QTimer m_overlayUpdateTimer;
    QTimer m_toolStateUpdateTimer;
    QHash<QString, QVariantMap> m_toolStates;
    QHash<QString, QVariantMap> m_toolStatesPending;
    QSet<QQuick3DNode *> m_rotationBlockedNodes;
    void updateCombinedCameraMoveVector();

    struct CameraMoveKeyData {
        QQuick3DCamera *camera;
        QList<QVector3D> moveVectors;
        QVector3D combinedMoveVector;
        QTimer timer;
    };
    CameraMoveKeyData m_camMoveData;

    struct SceneEnvData {
        QQuick3DSceneEnvironment::QQuick3DEnvironmentBackgroundTypes backgroundMode;
        QColor clearColor;
        QPointer<QQuick3DTexture> lightProbe;
        QPointer<QQuick3DCubeMapTexture> skyBoxCubeMap;
    };
    QHash<QString, SceneEnvData> m_sceneEnvironmentData;
    QVariantMap m_lastSceneEnvData;

    struct MultiSelData {
        QVector3D startScenePos;
        QVector3D startScale;
        QQuaternion startRot;
        QQuaternion startSceneRot;
    };

    QHash<QQuick3DNode *, MultiSelData> m_multiSelDataMap;
    QVariantList m_multiSelNodes;
    MultiSelData m_multiSelNodeData;
    QQuick3DNode *m_multiSelectRootNode = nullptr;
    QList<QMetaObject::Connection> m_multiSelectConnections;
    bool m_blockMultiSelectionNodePositioning = false;

    QPointer<QQuick3DCamera> m_activeScenePreferredCamera;

    bool m_snapAbsolute = true;
    bool m_snapPosition = false;
    bool m_snapRotation = false;
    bool m_snapScale = false;
    double m_snapPositionInterval = 50.;
    double m_snapRotationInterval = 5.;
    double m_snapScaleInterval = .1;
    double m_cameraSpeed = 10.;
    double m_cameraSpeedModifier = 1.;

    QVariant m_bgColor;
    QHash<QString, QTimer *> m_eventTimers;
};

}
}

#endif // QUICK3D_MODULE
