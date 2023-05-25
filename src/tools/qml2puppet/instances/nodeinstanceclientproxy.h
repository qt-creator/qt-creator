// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "nodeinstanceclientinterface.h"

#include <QObject>
#include <QHash>
#include <QWeakPointer>
#include <QFile>
#include <QTimer>

#include <memory>

QT_BEGIN_NAMESPACE
class QLocalSocket;
class QIODevice;
QT_END_NAMESPACE

namespace QmlDesigner {

class NodeInstanceServerInterface;
class CreateSceneCommand;
class CreateInstancesCommand;
class ClearSceneCommand;
class ReparentInstancesCommand;
class Update3dViewStateCommand;
class ChangeFileUrlCommand;
class ChangeValuesCommand;
class ChangeAuxiliaryCommand;
class ChangeBindingsCommand;
class ChangeIdsCommand;
class RemoveInstancesCommand;
class RemovePropertiesCommand;
class CompleteComponentCommand;
class ChangeStateCommand;
class ChangeNodeSourceCommand;
class EndPuppetCommand;
class ChangeSelectionCommand;
class PuppetToCreatorCommand;
class InputEventCommand;
class View3DActionCommand;
class RequestModelNodePreviewImageCommand;
class ChangeLanguageCommand;
class ChangePreviewImageSizeCommand;
class StartNanotraceCommand;

class NodeInstanceClientProxy : public QObject, public NodeInstanceClientInterface
{
    Q_OBJECT

public:
    NodeInstanceClientProxy(QObject *parent);
    ~NodeInstanceClientProxy() override;

    void informationChanged(const InformationChangedCommand &command) override;
    void valuesChanged(const ValuesChangedCommand &command) override;
    void valuesModified(const ValuesModifiedCommand &command) override;
    void pixmapChanged(const PixmapChangedCommand &command) override;
    void childrenChanged(const ChildrenChangedCommand &command) override;
    void statePreviewImagesChanged(const StatePreviewImageChangedCommand &command) override;
    void componentCompleted(const ComponentCompletedCommand &command) override;
    void token(const TokenCommand &command) override;
    void debugOutput(const DebugOutputCommand &command) override;
    void puppetAlive(const PuppetAliveCommand &command);
    void selectionChanged(const ChangeSelectionCommand &command) override;
    void handlePuppetToCreatorCommand(const PuppetToCreatorCommand &command) override;
    void capturedData(const CapturedDataCommand &capturedData) override;
    void sceneCreated(const SceneCreatedCommand &command) override;

    void flush() override;
    void synchronizeWithClientProcess() override;
    qint64 bytesToWrite() const override;

protected:
    void initializeSocket();
    void initializeCapturedStream(const QString &fileName);
    void writeCommand(const QVariant &command);
    void dispatchCommand(const QVariant &command);
    NodeInstanceServerInterface *nodeInstanceServer() const;
    void setNodeInstanceServer(std::unique_ptr<NodeInstanceServerInterface> nodeInstanceServer);

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
    void removeSharedMemory(const RemoveSharedMemoryCommand &command);
    void redirectToken(const TokenCommand &command);
    void redirectToken(const EndPuppetCommand &command);
    void changeSelection(const ChangeSelectionCommand &command);
    static QVariant readCommandFromIOStream(QIODevice *ioDevice, quint32 *readCommandCounter, quint32 *blockSize);
    void inputEvent(const InputEventCommand &command);
    void view3DAction(const View3DActionCommand &command);
    void requestModelNodePreviewImage(const RequestModelNodePreviewImageCommand &command);
    void changeLanguage(const ChangeLanguageCommand &command);
    void changePreviewImageSize(const ChangePreviewImageSizeCommand &command);
    void startNanotrace(const StartNanotraceCommand& command);

protected slots:
    void readDataStream();
    void sendPuppetAliveCommand();

private:
    QFile m_controlStream;
    QTimer m_puppetAliveTimer;
    QIODevice *m_inputIoDevice;
    QIODevice *m_outputIoDevice;
    QLocalSocket *m_localSocket;
    std::unique_ptr<NodeInstanceServerInterface> m_nodeInstanceServer;
    quint32 m_writeCommandCounter;
    int m_synchronizeId;
};

} // namespace QmlDesigner
