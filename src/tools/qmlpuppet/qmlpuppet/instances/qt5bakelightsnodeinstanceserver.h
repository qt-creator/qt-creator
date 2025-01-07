// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "qt5nodeinstanceserver.h"

#if QUICK3D_MODULE && QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QTemporaryDir>
#define BAKE_LIGHTS_SUPPORTED 1
#endif

QT_BEGIN_NAMESPACE
class QProcess;
class QQuick3DViewport;
QT_END_NAMESPACE

namespace QmlDesigner {

class Qt5BakeLightsNodeInstanceServer : public Qt5NodeInstanceServer
{
    Q_OBJECT
public:
    explicit Qt5BakeLightsNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);

#if BAKE_LIGHTS_SUPPORTED
public:
    virtual ~Qt5BakeLightsNodeInstanceServer();

    void createScene(const CreateSceneCommand &command) override;
    void view3DAction(const View3DActionCommand &command) override;

    void render();

protected:
    void collectItemChangesAndSendChangeCommands() override;
    void startRenderTimer() override;
    void bakeLights();

private:
    void abort(const QString &msg);
    void runDenoiser();
    void finish();
    void cleanup();

    QQuick3DViewport *m_view3D = nullptr;
    bool m_bakingStarted = false;
    bool m_callbackReceived = false;
    int m_renderCount = 0;
    QProcess *m_denoiser = nullptr;
    QTemporaryDir m_workingDir;
#else
protected:
    void collectItemChangesAndSendChangeCommands() override {};
#endif
};

} // namespace QmlDesigner
