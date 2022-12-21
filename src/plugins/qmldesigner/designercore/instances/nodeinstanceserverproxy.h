// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "nodeinstanceserverinterface.h"

#include <QElapsedTimer>
#include <QFile>
#include <QPointer>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QLocalServer;
class QLocalSocket;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Target;
}

namespace QmlDesigner {

class NodeInstanceClientInterface;
class NodeInstanceView;
class NodeInstanceClientProxy;
class BaseConnectionManager;
class ConnectionManagerInterface;
class ExternalDependenciesInterface;

class NodeInstanceServerProxy : public NodeInstanceServerInterface
{
    Q_OBJECT

    friend BaseConnectionManager;

public:
    explicit NodeInstanceServerProxy(NodeInstanceView *nodeInstanceView,
                                     ProjectExplorer::Target *target,
                                     ConnectionManagerInterface &connectionManager,
                                     ExternalDependenciesInterface &externalDependencies);
    ~NodeInstanceServerProxy() override;
    void createInstances(const CreateInstancesCommand &command) override;
    void changeFileUrl(const ChangeFileUrlCommand &command) override;
    void createScene(const CreateSceneCommand &command) override;
    void update3DViewState(const Update3dViewStateCommand &command) override;
    void clearScene(const ClearSceneCommand &command) override;
    void removeInstances(const RemoveInstancesCommand &command) override;
    void changeSelection(const ChangeSelectionCommand &command) override;
    void removeProperties(const RemovePropertiesCommand &command) override;
    void changePropertyBindings(const ChangeBindingsCommand &command) override;
    void changePropertyValues(const ChangeValuesCommand &command) override;
    void changeAuxiliaryValues(const ChangeAuxiliaryCommand &command) override;
    void reparentInstances(const ReparentInstancesCommand &command) override;
    void changeIds(const ChangeIdsCommand &command) override;
    void changeState(const ChangeStateCommand &command) override;
    void completeComponent(const CompleteComponentCommand &command) override;
    void changeNodeSource(const ChangeNodeSourceCommand &command) override;
    void token(const TokenCommand &command) override;
    void removeSharedMemory(const RemoveSharedMemoryCommand &command) override;
    void benchmark(const QString &message) override;
    void inputEvent(const InputEventCommand &command) override;
    void view3DAction(const View3DActionCommand &command) override;
    void requestModelNodePreviewImage(const RequestModelNodePreviewImageCommand &command) override;
    void changeLanguage(const ChangeLanguageCommand &command) override;
    void changePreviewImageSize(const ChangePreviewImageSizeCommand &command) override;
    void dispatchCommand(const QVariant &command) override;

    NodeInstanceView *nodeInstanceView() const { return m_nodeInstanceView; }

    QString qrcMappingString() const;

protected:
    void writeCommand(const QVariant &command);
    NodeInstanceClientInterface *nodeInstanceClient() const;

private:
    NodeInstanceView *m_nodeInstanceView{};
    QElapsedTimer m_benchmarkTimer;
    ConnectionManagerInterface &m_connectionManager;
};

} // namespace QmlDesigner
