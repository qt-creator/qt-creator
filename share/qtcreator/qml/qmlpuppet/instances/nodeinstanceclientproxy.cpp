/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "nodeinstanceclientproxy.h"

#include <QLocalSocket>
#include <QVariant>
#include <QCoreApplication>
#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QBuffer>

#include "nodeinstanceserverinterface.h"

#include "changeauxiliarycommand.h"
#include "changebindingscommand.h"
#include "changefileurlcommand.h"
#include "changeidscommand.h"
#include "changelanguagecommand.h"
#include "changenodesourcecommand.h"
#include "changepreviewimagesizecommand.h"
#include "changeselectioncommand.h"
#include "changestatecommand.h"
#include "changevaluescommand.h"
#include "childrenchangedcommand.h"
#include "clearscenecommand.h"
#include "completecomponentcommand.h"
#include "componentcompletedcommand.h"
#include "createinstancescommand.h"
#include "createscenecommand.h"
#include "debugoutputcommand.h"
#include "endpuppetcommand.h"
#include "imagecontainer.h"
#include "informationchangedcommand.h"
#include "inputeventcommand.h"
#include "instancecontainer.h"
#include "pixmapchangedcommand.h"
#include "propertyabstractcontainer.h"
#include "propertybindingcontainer.h"
#include "propertyvaluecontainer.h"
#include "puppetalivecommand.h"
#include "puppettocreatorcommand.h"
#include "removeinstancescommand.h"
#include "removepropertiescommand.h"
#include "removesharedmemorycommand.h"
#include "reparentinstancescommand.h"
#include "statepreviewimagechangedcommand.h"
#include "synchronizecommand.h"
#include "tokencommand.h"
#include "update3dviewstatecommand.h"
#include "valueschangedcommand.h"
#include "view3dactioncommand.h"

namespace QmlDesigner {

constexpr void (QLocalSocket::*LocalSocketErrorFunction)(QLocalSocket::LocalSocketError)
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    = &QLocalSocket::error;
#else
    = &QLocalSocket::errorOccurred;
#endif

NodeInstanceClientProxy::NodeInstanceClientProxy(QObject *parent)
    : QObject(parent),
      m_inputIoDevice(nullptr),
      m_outputIoDevice(nullptr),
      m_nodeInstanceServer(nullptr),
      m_writeCommandCounter(0),
      m_synchronizeId(-1)
{
    connect(&m_puppetAliveTimer, &QTimer::timeout, this, &NodeInstanceClientProxy::sendPuppetAliveCommand);
    m_puppetAliveTimer.setInterval(2000);
    m_puppetAliveTimer.start();
}

void NodeInstanceClientProxy::initializeSocket()
{
    QLocalSocket *localSocket = new QLocalSocket(this);
    connect(localSocket, &QIODevice::readyRead, this, &NodeInstanceClientProxy::readDataStream);
    connect(localSocket, LocalSocketErrorFunction,
            QCoreApplication::instance(), &QCoreApplication::quit);
    connect(localSocket, &QLocalSocket::disconnected, QCoreApplication::instance(), &QCoreApplication::quit);
    localSocket->connectToServer(QCoreApplication::arguments().at(1), QIODevice::ReadWrite | QIODevice::Unbuffered);
    localSocket->waitForConnected(-1);

    m_inputIoDevice = localSocket;
    m_outputIoDevice = localSocket;
}

void NodeInstanceClientProxy::initializeCapturedStream(const QString &fileName)
{
    m_inputIoDevice = new QFile(fileName, this);
    bool inputStreamCanBeOpened = m_inputIoDevice->open(QIODevice::ReadOnly);
    if (!inputStreamCanBeOpened) {
        qDebug() << "Input stream file cannot be opened: " << fileName;
        exit(-1);
    }

    if (QCoreApplication::arguments().count() == 3) {
        QFileInfo inputFileInfo(fileName);
        m_outputIoDevice = new QFile(inputFileInfo.path()+ "/" + inputFileInfo.baseName() + ".commandcontrolstream", this);
        bool outputStreamCanBeOpened = m_outputIoDevice->open(QIODevice::WriteOnly);
        if (!outputStreamCanBeOpened) {
            qDebug() << "Output stream file cannot be opened";
            exit(-1);
        }
    } else if (QCoreApplication::arguments().count() == 4) {
        m_controlStream.setFileName(QCoreApplication::arguments().at(3));
        bool controlStreamCanBeOpened = m_controlStream.open(QIODevice::ReadOnly);
        if (!controlStreamCanBeOpened) {
            qDebug() << "Control stream file cannot be opened";
            exit(-1);
        }
    }

}

bool compareCommands(const QVariant &command, const QVariant &controlCommand)
{
    static const int informationChangedCommandType = QMetaType::type("InformationChangedCommand");
    static const int valuesChangedCommandType = QMetaType::type("ValuesChangedCommand");
    static const int valuesModifiedCommandType = QMetaType::type("ValuesModifiedCommand");
    static const int pixmapChangedCommandType = QMetaType::type("PixmapChangedCommand");
    static const int childrenChangedCommandType = QMetaType::type("ChildrenChangedCommand");
    static const int statePreviewImageChangedCommandType = QMetaType::type("StatePreviewImageChangedCommand");
    static const int componentCompletedCommandType = QMetaType::type("ComponentCompletedCommand");
    static const int synchronizeCommandType = QMetaType::type("SynchronizeCommand");
    static const int tokenCommandType = QMetaType::type("TokenCommand");
    static const int debugOutputCommandType = QMetaType::type("DebugOutputCommand");
    static const int changeSelectionCommandType = QMetaType::type("ChangeSelectionCommand");

    if (command.userType() == controlCommand.userType()) {
        if (command.userType() == informationChangedCommandType)
            return command.value<InformationChangedCommand>() == controlCommand.value<InformationChangedCommand>();
        else if (command.userType() == valuesChangedCommandType)
            return command.value<ValuesChangedCommand>() == controlCommand.value<ValuesChangedCommand>();
        else if (command.userType() == valuesModifiedCommandType)
            return command.value<ValuesModifiedCommand>() == controlCommand.value<ValuesModifiedCommand>();
        else if (command.userType() == pixmapChangedCommandType)
            return command.value<PixmapChangedCommand>() == controlCommand.value<PixmapChangedCommand>();
        else if (command.userType() == childrenChangedCommandType)
            return command.value<ChildrenChangedCommand>() == controlCommand.value<ChildrenChangedCommand>();
        else if (command.userType() == statePreviewImageChangedCommandType)
            return command.value<StatePreviewImageChangedCommand>() == controlCommand.value<StatePreviewImageChangedCommand>();
        else if (command.userType() == componentCompletedCommandType)
            return command.value<ComponentCompletedCommand>() == controlCommand.value<ComponentCompletedCommand>();
        else if (command.userType() == synchronizeCommandType)
            return command.value<SynchronizeCommand>() == controlCommand.value<SynchronizeCommand>();
        else if (command.userType() == tokenCommandType)
            return command.value<TokenCommand>() == controlCommand.value<TokenCommand>();
        else if (command.userType() == debugOutputCommandType)
            return command.value<DebugOutputCommand>() == controlCommand.value<DebugOutputCommand>();
        else if (command.userType() == changeSelectionCommandType)
            return command.value<ChangeSelectionCommand>() == controlCommand.value<ChangeSelectionCommand>();
    }

    return false;
}

void NodeInstanceClientProxy::writeCommand(const QVariant &command)
{
    if (m_controlStream.isReadable()) {
        static quint32 readCommandCounter = 0;
        static quint32 blockSize = 0;

        QVariant controlCommand = readCommandFromIOStream(&m_controlStream, &readCommandCounter, &blockSize);

        if (!compareCommands(command, controlCommand)) {
            qDebug() << "Commands differ!";
            exit(-1);
        }
    } else if (m_outputIoDevice) {
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_8);
        out << quint32(0);
        out << quint32(m_writeCommandCounter);
        m_writeCommandCounter++;
        out << command;
        out.device()->seek(0);
        out << quint32(block.size() - sizeof(quint32));

        m_outputIoDevice->write(block);
    }
}

void NodeInstanceClientProxy::informationChanged(const InformationChangedCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::valuesChanged(const ValuesChangedCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::valuesModified(const ValuesModifiedCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::pixmapChanged(const PixmapChangedCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::childrenChanged(const ChildrenChangedCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::statePreviewImagesChanged(const StatePreviewImageChangedCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::componentCompleted(const ComponentCompletedCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::token(const TokenCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::debugOutput(const DebugOutputCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::puppetAlive(const PuppetAliveCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::selectionChanged(const ChangeSelectionCommand &command)
{
     writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::handlePuppetToCreatorCommand(const PuppetToCreatorCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::flush()
{
}

void NodeInstanceClientProxy::synchronizeWithClientProcess()
{
    if (m_synchronizeId >= 0) {
        SynchronizeCommand synchronizeCommand(m_synchronizeId);
        writeCommand(QVariant::fromValue(synchronizeCommand));
    }
}

qint64 NodeInstanceClientProxy::bytesToWrite() const
{
    return m_inputIoDevice->bytesToWrite();
}

QVariant NodeInstanceClientProxy::readCommandFromIOStream(QIODevice *ioDevice, quint32 *readCommandCounter, quint32 *blockSize)
{
    QDataStream in(ioDevice);
    in.setVersion(QDataStream::Qt_4_8);

    if (*blockSize == 0) {
        in >> *blockSize;
    }

    if (ioDevice->bytesAvailable() < *blockSize)
        return QVariant();

    quint32 commandCounter;
    in >> commandCounter;
    bool commandLost = !((commandCounter == 0 && *readCommandCounter == 0) || (*readCommandCounter + 1 == commandCounter));
    if (commandLost)
        qDebug() << "client command lost: " << *readCommandCounter <<  commandCounter;
    *readCommandCounter = commandCounter;

    QVariant command;
    in >> command;
    *blockSize = 0;

    if (in.status() != QDataStream::Ok) {
        qWarning() << "Stream is not OK";
        exit(1);
    }

    return command;
}

void NodeInstanceClientProxy::inputEvent(const InputEventCommand &command)
{
    nodeInstanceServer()->inputEvent(command);
}

void NodeInstanceClientProxy::view3DAction(const View3DActionCommand &command)
{
    nodeInstanceServer()->view3DAction(command);
}

void NodeInstanceClientProxy::changeLanguage(const ChangeLanguageCommand &command)
{
    nodeInstanceServer()->changeLanguage(command);
}

void NodeInstanceClientProxy::changePreviewImageSize(const ChangePreviewImageSizeCommand &command)
{
    nodeInstanceServer()->changePreviewImageSize(command);
}

void NodeInstanceClientProxy::readDataStream()
{
    QList<QVariant> commandList;

    while (!m_inputIoDevice->atEnd()) {
        if (m_inputIoDevice->bytesAvailable() < int(sizeof(quint32)))
            break;

        static quint32 readCommandCounter = 0;
        static quint32 blockSize = 0;

        QVariant command = readCommandFromIOStream(m_inputIoDevice, &readCommandCounter, &blockSize);

        if (command.isValid())
            commandList.append(command);
        else
            break;
    }

    for (const QVariant &command : qAsConst(commandList))
        dispatchCommand(command);
}

void NodeInstanceClientProxy::sendPuppetAliveCommand()
{
    puppetAlive(PuppetAliveCommand());
}

NodeInstanceServerInterface *NodeInstanceClientProxy::nodeInstanceServer() const
{
    return m_nodeInstanceServer;
}

void NodeInstanceClientProxy::setNodeInstanceServer(NodeInstanceServerInterface *nodeInstanceServer)
{
    m_nodeInstanceServer = nodeInstanceServer;
}

void NodeInstanceClientProxy::createInstances(const CreateInstancesCommand &command)
{
    nodeInstanceServer()->createInstances(command);
}

void NodeInstanceClientProxy::changeFileUrl(const ChangeFileUrlCommand &command)
{
    nodeInstanceServer()->changeFileUrl(command);
}

void NodeInstanceClientProxy::createScene(const CreateSceneCommand &command)
{
    nodeInstanceServer()->createScene(command);
}

void NodeInstanceClientProxy::update3DViewState(const Update3dViewStateCommand &command)
{
    nodeInstanceServer()->update3DViewState(command);
}

void NodeInstanceClientProxy::clearScene(const ClearSceneCommand &command)
{
    nodeInstanceServer()->clearScene(command);
}

void NodeInstanceClientProxy::removeInstances(const RemoveInstancesCommand &command)
{
    nodeInstanceServer()->removeInstances(command);
}

void NodeInstanceClientProxy::removeProperties(const RemovePropertiesCommand &command)
{
    nodeInstanceServer()->removeProperties(command);
}

void NodeInstanceClientProxy::changePropertyBindings(const ChangeBindingsCommand &command)
{
    nodeInstanceServer()->changePropertyBindings(command);
}

void NodeInstanceClientProxy::changePropertyValues(const ChangeValuesCommand &command)
{
    nodeInstanceServer()->changePropertyValues(command);
}

void NodeInstanceClientProxy::changeAuxiliaryValues(const ChangeAuxiliaryCommand &command)
{
    nodeInstanceServer()->changeAuxiliaryValues(command);
}

void NodeInstanceClientProxy::reparentInstances(const ReparentInstancesCommand &command)
{
    nodeInstanceServer()->reparentInstances(command);
}

void NodeInstanceClientProxy::changeIds(const ChangeIdsCommand &command)
{
    nodeInstanceServer()->changeIds(command);
}

void NodeInstanceClientProxy::changeState(const ChangeStateCommand &command)
{
    nodeInstanceServer()->changeState(command);
}

void NodeInstanceClientProxy::completeComponent(const CompleteComponentCommand &command)
{
    nodeInstanceServer()->completeComponent(command);
}

void NodeInstanceClientProxy::changeNodeSource(const ChangeNodeSourceCommand &command)
{
    nodeInstanceServer()->changeNodeSource(command);
}

void NodeInstanceClientProxy::removeSharedMemory(const RemoveSharedMemoryCommand &command)
{
    nodeInstanceServer()->removeSharedMemory(command);
}
void NodeInstanceClientProxy::redirectToken(const TokenCommand &command)
{
    nodeInstanceServer()->token(command);
}

void NodeInstanceClientProxy::redirectToken(const EndPuppetCommand & /*command*/)
{
    if (m_outputIoDevice && m_outputIoDevice->isOpen())
        m_outputIoDevice->close();

    if (m_inputIoDevice && m_inputIoDevice->isOpen())
        m_inputIoDevice->close();

    if (m_controlStream.isOpen())
        m_controlStream.close();

    qDebug() << "End Process: " << QCoreApplication::applicationPid();
    QCoreApplication::exit();
}

void NodeInstanceClientProxy::changeSelection(const ChangeSelectionCommand &command)
{
    nodeInstanceServer()->changeSelection(command);
}

void NodeInstanceClientProxy::dispatchCommand(const QVariant &command)
{
    static const int createInstancesCommandType = QMetaType::type("CreateInstancesCommand");
    static const int update3dViewStateCommand = QMetaType::type("Update3dViewStateCommand");
    static const int changeFileUrlCommandType = QMetaType::type("ChangeFileUrlCommand");
    static const int createSceneCommandType = QMetaType::type("CreateSceneCommand");
    static const int clearSceneCommandType = QMetaType::type("ClearSceneCommand");
    static const int removeInstancesCommandType = QMetaType::type("RemoveInstancesCommand");
    static const int removePropertiesCommandType = QMetaType::type("RemovePropertiesCommand");
    static const int changeBindingsCommandType = QMetaType::type("ChangeBindingsCommand");
    static const int changeValuesCommandType = QMetaType::type("ChangeValuesCommand");
    static const int changeAuxiliaryCommandType = QMetaType::type("ChangeAuxiliaryCommand");
    static const int reparentInstancesCommandType = QMetaType::type("ReparentInstancesCommand");
    static const int changeIdsCommandType = QMetaType::type("ChangeIdsCommand");
    static const int changeStateCommandType = QMetaType::type("ChangeStateCommand");
    static const int completeComponentCommandType = QMetaType::type("CompleteComponentCommand");
    static const int synchronizeCommandType = QMetaType::type("SynchronizeCommand");
    static const int changeNodeSourceCommandType = QMetaType::type("ChangeNodeSourceCommand");
    static const int removeSharedMemoryCommandType = QMetaType::type("RemoveSharedMemoryCommand");
    static const int tokenCommandType = QMetaType::type("TokenCommand");
    static const int endPuppetCommandType = QMetaType::type("EndPuppetCommand");
    static const int changeSelectionCommandType = QMetaType::type("ChangeSelectionCommand");
    static const int inputEventCommandType = QMetaType::type("InputEventCommand");
    static const int view3DActionCommandType = QMetaType::type("View3DActionCommand");
    static const int changeLanguageCommand = QMetaType::type("ChangeLanguageCommand");
    static const int changePreviewImageSizeCommand = QMetaType::type(
        "ChangePreviewImageSizeCommand");

    const int commandType = command.userType();

    if (commandType == inputEventCommandType)
        inputEvent(command.value<InputEventCommand>());
    else if (commandType == createInstancesCommandType)
        createInstances(command.value<CreateInstancesCommand>());
    else if (commandType == update3dViewStateCommand)
        update3DViewState(command.value<Update3dViewStateCommand>());
    else if (commandType == changeFileUrlCommandType)
        changeFileUrl(command.value<ChangeFileUrlCommand>());
    else if (commandType == createSceneCommandType)
        createScene(command.value<CreateSceneCommand>());
    else if (commandType == clearSceneCommandType)
        clearScene(command.value<ClearSceneCommand>());
    else if (commandType == removeInstancesCommandType)
        removeInstances(command.value<RemoveInstancesCommand>());
    else if (commandType == removePropertiesCommandType)
        removeProperties(command.value<RemovePropertiesCommand>());
    else if (commandType == changeBindingsCommandType)
        changePropertyBindings(command.value<ChangeBindingsCommand>());
    else if (commandType == changeValuesCommandType)
        changePropertyValues(command.value<ChangeValuesCommand>());
    else if (commandType == changeAuxiliaryCommandType)
        changeAuxiliaryValues(command.value<ChangeAuxiliaryCommand>());
    else if (commandType == reparentInstancesCommandType)
        reparentInstances(command.value<ReparentInstancesCommand>());
    else if (commandType == changeIdsCommandType)
        changeIds(command.value<ChangeIdsCommand>());
    else if (commandType == changeStateCommandType)
        changeState(command.value<ChangeStateCommand>());
    else if (commandType == completeComponentCommandType)
        completeComponent(command.value<CompleteComponentCommand>());
    else if (commandType == changeNodeSourceCommandType)
        changeNodeSource(command.value<ChangeNodeSourceCommand>());
    else if (commandType == removeSharedMemoryCommandType)
        removeSharedMemory(command.value<RemoveSharedMemoryCommand>());
    else if (commandType == tokenCommandType)
        redirectToken(command.value<TokenCommand>());
    else if (commandType == endPuppetCommandType)
        redirectToken(command.value<EndPuppetCommand>());
    else if (commandType == view3DActionCommandType)
        view3DAction(command.value<View3DActionCommand>());
    else if (commandType == synchronizeCommandType) {
        SynchronizeCommand synchronizeCommand = command.value<SynchronizeCommand>();
        m_synchronizeId = synchronizeCommand.synchronizeId();
    } else if (commandType == changeSelectionCommandType) {
        ChangeSelectionCommand changeSelectionCommand = command.value<ChangeSelectionCommand>();
        changeSelection(changeSelectionCommand);
    } else if (command.userType() == changeLanguageCommand) {
        changeLanguage(command.value<ChangeLanguageCommand>());
    } else if (command.userType() == changePreviewImageSizeCommand) {
        changePreviewImageSize(command.value<ChangePreviewImageSizeCommand>());
    } else {
        Q_ASSERT(false);
    }
}
} // namespace QmlDesigner
