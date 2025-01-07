// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qt5nodeinstanceserver.h"

namespace QmlDesigner {

class Qt5PreviewNodeInstanceServer : public Qt5NodeInstanceServer
{
    Q_OBJECT
public:
    explicit Qt5PreviewNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);

    void createScene(const CreateSceneCommand &command) override;
    void changeState(const ChangeStateCommand &command) override;
    void removeSharedMemory(const RemoveSharedMemoryCommand &command) override;
    void changePreviewImageSize(const ChangePreviewImageSizeCommand &command) override;
    bool isPreviewServer() const override;

    QImage renderPreviewImage();

protected:
    void collectItemChangesAndSendChangeCommands() override;
    void startRenderTimer() override;

private:
    ServerNodeInstance m_currentState;
    QSize m_previewSize{320, 320};
};

} // namespace QmlDesigner
