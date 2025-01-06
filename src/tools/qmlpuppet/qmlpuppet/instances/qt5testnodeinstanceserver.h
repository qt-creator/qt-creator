// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qt5nodeinstanceserver.h"

namespace QmlDesigner {

class Qt5TestNodeInstanceServer : public Qt5NodeInstanceServer
{
public:
    Qt5TestNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);


    void createInstances(const CreateInstancesCommand &command) override;
    void changeFileUrl(const ChangeFileUrlCommand &command) override;
    void changePropertyValues(const ChangeValuesCommand &command) override;
    void changePropertyBindings(const ChangeBindingsCommand &command) override;
    void changeAuxiliaryValues(const ChangeAuxiliaryCommand &command) override;
    void changeIds(const ChangeIdsCommand &command) override;
    void createScene(const CreateSceneCommand &command) override;
    void clearScene(const ClearSceneCommand &command) override;
    void removeInstances(const RemoveInstancesCommand &command) override;
    void removeProperties(const RemovePropertiesCommand &command) override;
    void reparentInstances(const ReparentInstancesCommand &command) override;
    void changeState(const ChangeStateCommand &command) override;
    void completeComponent(const CompleteComponentCommand &command) override;
    void changeNodeSource(const ChangeNodeSourceCommand &command) override;
    void removeSharedMemory(const RemoveSharedMemoryCommand &command) override;
    void changeSelection(const ChangeSelectionCommand &command) override;

    using Qt5NodeInstanceServer::createInstances;

protected:
    void collectItemChangesAndSendChangeCommands() override;
    void sendChildrenChangedCommand(const QList<ServerNodeInstance> &childList);
    bool isDirtyRecursiveForNonInstanceItems(QQuickItem *item) const;
};

} // namespace QmlDesigner
