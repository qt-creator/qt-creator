// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <nodeinstanceserver.h>

#include <memory>
#include <vector>

namespace QmlDesigner {

class NodeInstanceServerDispatcher : public NodeInstanceServerInterface
{
public:
    NodeInstanceServerDispatcher(const QStringList &serverNames,
                                 NodeInstanceClientInterface *nodeInstanceClient);

    void createInstances(const CreateInstancesCommand &command);
    void changeFileUrl(const ChangeFileUrlCommand &command);
    void createScene(const CreateSceneCommand &command);
    void clearScene(const ClearSceneCommand &command);
    void update3DViewState(const Update3dViewStateCommand &command);
    void removeInstances(const RemoveInstancesCommand &command);
    void removeProperties(const RemovePropertiesCommand &command);
    void changePropertyBindings(const ChangeBindingsCommand &command);
    void changePropertyValues(const ChangeValuesCommand &command);
    void changeAuxiliaryValues(const ChangeAuxiliaryCommand &command);
    void reparentInstances(const ReparentInstancesCommand &command);
    void changeIds(const ChangeIdsCommand &command);
    void changeState(const ChangeStateCommand &command);
    void completeComponent(const CompleteComponentCommand &command);
    void changeNodeSource(const ChangeNodeSourceCommand &command);
    void token(const TokenCommand &command);
    void removeSharedMemory(const RemoveSharedMemoryCommand &command);
    void changeSelection(const ChangeSelectionCommand &command);
    void inputEvent(const InputEventCommand &command);
    void view3DAction(const View3DActionCommand &command);
    void requestModelNodePreviewImage(const RequestModelNodePreviewImageCommand &command);
    void changeLanguage(const ChangeLanguageCommand &command);
    void changePreviewImageSize(const ChangePreviewImageSizeCommand &command);

private:
    void addServer(const QString &serverName, NodeInstanceClientInterface *nodeInstanceClient);

protected:
    std::vector<std::unique_ptr<NodeInstanceServer>> m_servers;
};

} // namespace QmlDesigner
