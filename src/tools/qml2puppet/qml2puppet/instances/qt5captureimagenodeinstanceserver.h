// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qt5previewnodeinstanceserver.h>

namespace QmlDesigner {

class Qt5CaptureImageNodeInstanceServer : public Qt5PreviewNodeInstanceServer
{
public:
    explicit Qt5CaptureImageNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient)
        : Qt5PreviewNodeInstanceServer(nodeInstanceClient)
    {}

    void createScene(const CreateSceneCommand &command) override;

protected:
    void collectItemChangesAndSendChangeCommands() override;

private:
    QSize m_minimumSize;
    QSize m_maximumSize;
};

} // namespace QmlDesigner
