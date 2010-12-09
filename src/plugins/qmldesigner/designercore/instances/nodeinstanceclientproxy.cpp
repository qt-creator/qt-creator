#include "nodeinstanceclientproxy.h"

#include <QLocalSocket>
#include <QVariant>
#include <QCoreApplication>
#include <QStringList>

#include "nodeinstanceserver.h"

#include "propertyabstractcontainer.h"
#include "propertyvaluecontainer.h"
#include "propertybindingcontainer.h"
#include "instancecontainer.h"
#include "createinstancescommand.h"
#include "createscenecommand.h"
#include "changevaluescommand.h"
#include "changebindingscommand.h"
#include "changefileurlcommand.h"
#include "removeinstancescommand.h"
#include "clearscenecommand.h"
#include "removepropertiescommand.h"
#include "reparentinstancescommand.h"
#include "changeidscommand.h"
#include "changestatecommand.h"
#include "addimportcommand.h"
#include "completecomponentcommand.h"

#include "informationchangedcommand.h"
#include "pixmapchangedcommand.h"
#include "valueschangedcommand.h"
#include "childrenchangedcommand.h"
#include "imagecontainer.h"
#include "statepreviewimagechangedcommand.h"
#include "componentcompletedcommand.h"

namespace QmlDesigner {

NodeInstanceClientProxy::NodeInstanceClientProxy(QObject *parent)
    : QObject(parent),
      m_nodeinstanceServer(new NodeInstanceServer(this)),
      m_blockSize(0)
{
    m_socket = new QLocalSocket(this);
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(readDataStream()));
    connect(m_socket, SIGNAL(error(QLocalSocket::LocalSocketError)), QCoreApplication::instance(), SLOT(quit()));
    connect(m_socket, SIGNAL(disconnected()), QCoreApplication::instance(), SLOT(quit()));
    m_socket->connectToServer(QCoreApplication::arguments().at(1), QIODevice::ReadWrite | QIODevice::Unbuffered);
    m_socket->waitForConnected(-1);
}

void NodeInstanceClientProxy::writeCommand(const QVariant &command)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << quint32(0);
    out << command;
    out.device()->seek(0);
    out << quint32(block.size() - sizeof(quint32));

    m_socket->write(block);
}

void NodeInstanceClientProxy::informationChanged(const InformationChangedCommand &command)
{
  writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::valuesChanged(const ValuesChangedCommand &command)
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

void NodeInstanceClientProxy::flush()
{
}

qint64 NodeInstanceClientProxy::bytesToWrite() const
{
    return m_socket->bytesToWrite();
}

void NodeInstanceClientProxy::readDataStream()
{
    QList<QVariant> commandList;

    while (!m_socket->atEnd()) {
        if (m_socket->bytesAvailable() < int(sizeof(quint32)))
            break;

        QDataStream in(m_socket);

        if (m_blockSize == 0) {
            in >> m_blockSize;
        }

        if (m_socket->bytesAvailable() < m_blockSize)
            break;

        QVariant command;
        in >> command;
        m_blockSize = 0;

        Q_ASSERT(in.status() == QDataStream::Ok);

        commandList.append(command);
    }

    foreach (const QVariant &command, commandList) {
        dispatchCommand(command);
    }
}

NodeInstanceServerInterface *NodeInstanceClientProxy::nodeInstanceServer() const
{
    return m_nodeinstanceServer;
}

void NodeInstanceClientProxy::dispatchCommand(const QVariant &command)
{
    static const int createInstancesCommandType = QMetaType::type("CreateInstancesCommand");
    static const int changeFileUrlCommandType = QMetaType::type("ChangeFileUrlCommand");
    static const int createSceneCommandType = QMetaType::type("CreateSceneCommand");
    static const int clearSceneCommandType = QMetaType::type("ClearSceneCommand");
    static const int removeInstancesCommandType = QMetaType::type("RemoveInstancesCommand");
    static const int removePropertiesCommandType = QMetaType::type("RemovePropertiesCommand");
    static const int changeBindingsCommandType = QMetaType::type("ChangeBindingsCommand");
    static const int changeValuesCommandType = QMetaType::type("ChangeValuesCommand");
    static const int reparentInstancesCommandType = QMetaType::type("ReparentInstancesCommand");
    static const int changeIdsCommandType = QMetaType::type("ChangeIdsCommand");
    static const int changeStateCommandType = QMetaType::type("ChangeStateCommand");
    static const int addImportCommandType = QMetaType::type("AddImportCommand");
    static const int completeComponentCommandType = QMetaType::type("CompleteComponentCommand");

    if (command.userType() ==  createInstancesCommandType)
        nodeInstanceServer()->createInstances(command.value<CreateInstancesCommand>());
    else if (command.userType() ==  changeFileUrlCommandType)
        nodeInstanceServer()->changeFileUrl(command.value<ChangeFileUrlCommand>());
    else if (command.userType() ==  createSceneCommandType)
        nodeInstanceServer()->createScene(command.value<CreateSceneCommand>());
    else if (command.userType() ==  clearSceneCommandType)
        nodeInstanceServer()->clearScene(command.value<ClearSceneCommand>());
    else if (command.userType() ==  removeInstancesCommandType)
        nodeInstanceServer()->removeInstances(command.value<RemoveInstancesCommand>());
    else if (command.userType() ==  removePropertiesCommandType)
        nodeInstanceServer()->removeProperties(command.value<RemovePropertiesCommand>());
    else if (command.userType() ==  changeBindingsCommandType)
        nodeInstanceServer()->changePropertyBindings(command.value<ChangeBindingsCommand>());
    else if (command.userType() ==  changeValuesCommandType)
        nodeInstanceServer()->changePropertyValues(command.value<ChangeValuesCommand>());
    else if (command.userType() ==  reparentInstancesCommandType)
        nodeInstanceServer()->reparentInstances(command.value<ReparentInstancesCommand>());
    else if (command.userType() ==  changeIdsCommandType)
        nodeInstanceServer()->changeIds(command.value<ChangeIdsCommand>());
    else if (command.userType() ==  changeStateCommandType)
        nodeInstanceServer()->changeState(command.value<ChangeStateCommand>());
    else if (command.userType() ==  addImportCommandType)
        nodeInstanceServer()->addImport(command.value<AddImportCommand>());
    else if (command.userType() ==  completeComponentCommandType)
        nodeInstanceServer()->completeComponent(command.value<CompleteComponentCommand>());
    else
        Q_ASSERT(false);
}
} // namespace QmlDesigner
