// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qt5previewnodeinstanceserver.h>

namespace QmlDesigner {

class Qt5CapturePreviewNodeInstanceServer : public Qt5PreviewNodeInstanceServer
{
public:
    explicit Qt5CapturePreviewNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient)
        : Qt5PreviewNodeInstanceServer(nodeInstanceClient)
    {}

protected:
    void collectItemChangesAndSendChangeCommands() override;

private:
};

} // namespace QmlDesigner
