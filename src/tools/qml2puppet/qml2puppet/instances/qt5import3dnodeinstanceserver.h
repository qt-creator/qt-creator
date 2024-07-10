// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#ifdef QUICK3D_MODULE
#include "generalhelper.h"
#endif
#include "qt5nodeinstanceserver.h"

#ifdef QUICK3D_MODULE
#include "generalhelper.h"
QT_BEGIN_NAMESPACE
class QQuick3DNode;
QT_END_NAMESPACE
#endif

namespace QmlDesigner {

class Qt5Import3dNodeInstanceServer : public Qt5NodeInstanceServer
{
    Q_OBJECT
public:
    explicit Qt5Import3dNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);

public:
    virtual ~Qt5Import3dNodeInstanceServer();

    void createScene(const CreateSceneCommand &command) override;
    void view3DAction(const View3DActionCommand &command) override;

    void render();

protected:
    void collectItemChangesAndSendChangeCommands() override;
    void startRenderTimer() override;

private:
    void cleanup();

#ifdef QUICK3D_MODULE
    void addInitToRenderQueue();
    void addCurrentNodeToRenderQueue(int count = 1);
    void addIconToRenderQueue(const QString &assetName);

    QQuick3DViewport *m_view3D = nullptr;
    QQuick3DViewport *m_iconView3D = nullptr;
    Internal::GeneralHelper *m_generalHelper = nullptr;

    struct PreviewData
    {
        QString name;
        QVector3D lookAt;
        QVector3D extents;
        QQuick3DNode *node = {};
        QQuaternion cameraRotation;
        QVector3D cameraPosition;
    };
    QHash<QString, PreviewData> m_previewData;

    enum class RenderType
    {
        Init,
        CurrentNode,
        NextIcon
    };
    QList<RenderType> m_renderQueue;

    bool m_refocus = false;
    QString m_currentNode;
    QQuick3DNode *m_sceneNode = {};
    QStringList m_generateIconQueue;
    QQuaternion m_defaultCameraRotation;
    QVector3D m_defaultCameraPosition;
#endif
};

} // namespace QmlDesigner
