// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qt5nodeinstanceserver.h"

namespace QmlDesigner {

class Qt5RenderNodeInstanceServer : public Qt5NodeInstanceServer
{
    Q_OBJECT
public:
    explicit Qt5RenderNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);

    void createScene(const CreateSceneCommand &command) override;
    void clearScene(const ClearSceneCommand &command) override;
    void completeComponent(const CompleteComponentCommand &command) override;
    void removeSharedMemory(const RemoveSharedMemoryCommand &command) override;
    void changePropertyValues(const ChangeValuesCommand &command) override;
    void changePropertyBindings(const ChangeBindingsCommand &command) override;
    void reparentInstances(const ReparentInstancesCommand &command) override;
    void removeInstances(const RemoveInstancesCommand &command) override;

protected:
    void collectItemChangesAndSendChangeCommands() override;
    ServerNodeInstance findNodeInstanceForItem(QQuickItem *item) const;
    void resizeCanvasToRootItem() override;

private:
    void makeDirtyRecursive(const ServerNodeInstance &instance);

    QSet<ServerNodeInstance> m_dirtyInstanceSet;
};

} // namespace QmlDesigner
