// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

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

    int m_renderCount = 0;
    bool m_keepRendering = false;

#ifdef QUICK3D_MODULE
    QQuick3DViewport *m_view3D = nullptr;
    Internal::GeneralHelper *m_generalHelper = nullptr;

    struct PreviewData
    {
        QString name;
        QVector3D lookAt;
        QVector3D extents;
        QQuick3DNode *node = {};
    };
    QHash<QString, PreviewData> m_previewData;
    QString m_currentNode;
    QQuick3DNode *m_sceneNode = {};
#endif
};

} // namespace QmlDesigner
