// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#ifdef QUICK3D_MODULE
#include "generalhelper.h"
#endif
#include "qt5nodeinstanceserver.h"

QT_BEGIN_NAMESPACE
class QQuick3DNode;
QT_END_NAMESPACE

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
    void finish();
    void cleanup();

    int m_renderCount = 0;
    bool m_keepRendering = false;

#ifdef QUICK3D_MODULE
    QQuick3DViewport *m_view3D = nullptr;
    Internal::GeneralHelper *m_generalHelper = nullptr;
    QQuick3DNode *m_previewNode = nullptr;
    QVector3D m_lookAt;
#endif
};

} // namespace QmlDesigner
