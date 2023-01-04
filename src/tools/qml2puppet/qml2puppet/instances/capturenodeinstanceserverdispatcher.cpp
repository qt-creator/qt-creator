// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "capturenodeinstanceserverdispatcher.h"

#include "nodeinstanceclientinterface.h"
#include "qt5capturepreviewnodeinstanceserver.h"
#include "qt5informationnodeinstanceserver.h"
#include "qt5rendernodeinstanceserver.h"
#include "scenecreatedcommand.h"

namespace QmlDesigner {

void CaptureNodeInstanceServerDispatcher::createScene(const CreateSceneCommand &command)
{
    NodeInstanceServerDispatcher::createScene(command);

    QTimer::singleShot(100,
                       this,
                       &CaptureNodeInstanceServerDispatcher::collectItemChangesAndSendChangeCommands);
}

void CaptureNodeInstanceServerDispatcher::collectItemChangesAndSendChangeCommands()
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->collectItemChangesAndSendChangeCommands();

    m_nodeInstanceClient->sceneCreated({});
}

} // namespace QmlDesigner
