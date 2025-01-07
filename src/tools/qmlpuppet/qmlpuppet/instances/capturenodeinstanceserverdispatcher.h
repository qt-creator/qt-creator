// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "nodeinstanceserverdispatcher.h"

namespace QmlDesigner {

class CaptureNodeInstanceServerDispatcher : public NodeInstanceServerDispatcher
{
public:
    CaptureNodeInstanceServerDispatcher(const QStringList &serverNames,
                                        NodeInstanceClientInterface *nodeInstanceClient)
        : NodeInstanceServerDispatcher{serverNames, nodeInstanceClient}
        , m_nodeInstanceClient{nodeInstanceClient}
    {}

    void createScene(const CreateSceneCommand &command);

private:
    void collectItemChangesAndSendChangeCommands();

private:
    NodeInstanceClientInterface *m_nodeInstanceClient;
};

} // namespace QmlDesigner
