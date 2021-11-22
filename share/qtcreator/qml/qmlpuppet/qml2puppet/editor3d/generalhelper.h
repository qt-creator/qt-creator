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

#include <QHash>
#include <QMatrix4x4>
#include <QObject>
#include <QPointer>
#include <QQuaternion>
#include <QTimer>
#include <QVariant>
#include <QVector3D>
#include <QtQuick3D/private/qquick3dpickresult_p.h>

QT_BEGIN_NAMESPACE
class QQuick3DCamera;
class QQuick3DNode;
class QQuick3DViewport;
class QQuickItem;
QT_END_NAMESPACE

namespace QmlDesigner {
namespace Internal {

class GeneralHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isMacOS READ isMacOS CONSTANT)

public:
    GeneralHelper();

    Q_INVOKABLE void requestOverlayUpdate();
    Q_INVOKABLE QString generateUniqueName(const QString &nameRoot);

    Q_INVOKABLE void orbitCamera(QQuick3DCamera *camera, const QVector3D &startRotation,
                                 const QVector3D &lookAtPoint, const QVector3D &pressPos,
                                 const QVector3D &currentPos);
    Q_INVOKABLE QVector3D panCamera(QQuick3DCamera *camera, const QMatrix4x4 startTransform,
                                    const QVector3D &startPosition, const QVector3D &startLookAt,
                                    const QVector3D &pressPos, const QVector3D &currentPos,
                                    float zoomFactor);
    Q_INVOKABLE float zoomCamera(QQuick3DViewport *viewPort, QQuick3DCamera *camera, float distance,
                                 float defaultLookAtDistance, const QVector3D &lookAt,
                                 float zoomFactor, bool relative);
    Q_INVOKABLE QVector4D focusNodesToCamera(QQuick3DCamera *camera, float defaultLookAtDistance,
                                             const QVariant &nodes, QQuick3DViewport *viewPort,
                                             float oldZoom, bool updateZoom = true,
                                             bool closeUp = false);
    Q_INVOKABLE bool fuzzyCompare(double a, double b);
    Q_INVOKABLE void delayedPropertySet(QObject *obj, int delay, const QString &property,
                                        const QVariant& value);
    Q_INVOKABLE QQuick3DPickResult pickViewAt(QQuick3DViewport *view, float posX, float posY);
    Q_INVOKABLE QQuick3DNode *resolvePick(QQuick3DNode *pickNode);

    Q_INVOKABLE void registerGizmoTarget(QQuick3DNode *node);
    Q_INVOKABLE void unregisterGizmoTarget(QQuick3DNode *node);
    Q_INVOKABLE bool isLocked(QQuick3DNode *node);
    Q_INVOKABLE bool isHidden(QQuick3DNode *node);
    Q_INVOKABLE bool isPickable(QQuick3DNode *node);

    Q_INVOKABLE void storeToolState(const QString &sceneId, const QString &tool,
                                    const QVariant &state, int delayEmit = 0);
    void initToolStates(const QString &sceneId, const QVariantMap &toolStates);
    Q_INVOKABLE void enableItemUpdate(QQuickItem *item, bool enable);
    Q_INVOKABLE QVariantMap getToolStates(const QString &sceneId);
    QString globalStateId() const;
    QString lastSceneIdKey() const;
    QString rootSizeKey() const;

    Q_INVOKABLE double brightnessScaler() const;

    Q_INVOKABLE void setMultiSelectionTargets(QQuick3DNode *multiSelectRootNode,
                                              const QVariantList &selectedList);
    Q_INVOKABLE void resetMultiSelectionNode();
    Q_INVOKABLE void restartMultiSelection();
    Q_INVOKABLE QVariantList multiSelectionTargets() const;
    Q_INVOKABLE void moveMultiSelection(bool commit);
    Q_INVOKABLE void scaleMultiSelection(bool commit);
    Q_INVOKABLE void rotateMultiSelection(bool commit);

    bool isMacOS() const;

    void addRotationBlocks(const QSet<QQuick3DNode *> &nodes);
    void removeRotationBlocks(const QSet<QQuick3DNode *> &nodes);
    Q_INVOKABLE bool isRotationBlocked(QQuick3DNode *node) const;

signals:
    void overlayUpdateNeeded();
    void toolStateChanged(const QString &sceneId, const QString &tool, const QVariant &toolState);
    void hiddenStateChanged(QQuick3DNode *node);
    void lockedStateChanged(QQuick3DNode *node);
    void rotationBlocksChanged();

protected:
    bool eventFilter(QObject *obj, QEvent *event) final;

private:
    void handlePendingToolStateUpdate();
    QVector3D pivotScenePosition(QQuick3DNode *node) const;

    QTimer m_overlayUpdateTimer;
    QTimer m_toolStateUpdateTimer;
    QHash<QString, QVariantMap> m_toolStates;
    QHash<QString, QVariantMap> m_toolStatesPending;
    QSet<QQuick3DNode *> m_gizmoTargets;
    QSet<QQuick3DNode *> m_rotationBlockedNodes;

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
};

}
}

#endif // QUICK3D_MODULE
