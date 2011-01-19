#include "nodeinstanceserverproxy.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QCoreApplication>
#include <QUuid>

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

#include "synchronizecommand.h"

#include "nodeinstanceview.h"
#include "nodeinstanceclientproxy.h"

namespace QmlDesigner {

NodeInstanceServerProxy::NodeInstanceServerProxy(NodeInstanceView *nodeInstanceView, RunModus runModus)
    : NodeInstanceServerInterface(nodeInstanceView),
      m_localServer(new QLocalServer(this)),
      m_nodeInstanceView(nodeInstanceView),
      m_firstBlockSize(0),
      m_secondBlockSize(0),
      m_runModus(runModus),
      m_synchronizeId(-1)
{
   QString socketToken(QUuid::createUuid().toString());

   m_localServer->listen(socketToken);
   m_localServer->setMaxPendingConnections(2);

   QString applicationPath =  QCoreApplication::applicationDirPath();
   if (runModus == TestModus)
       applicationPath += "/../../../../../bin";
#ifdef Q_OS_MACX
   applicationPath += "/qmlpuppet.app/Contents/MacOS";
#endif
   applicationPath += "/qmlpuppet";

   m_qmlPuppetEditorProcess = new QProcess(QCoreApplication::instance());
   connect(m_qmlPuppetEditorProcess.data(), SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processFinished(int,QProcess::ExitStatus)));
   m_qmlPuppetEditorProcess->setProcessChannelMode(QProcess::ForwardedChannels);
   m_qmlPuppetEditorProcess->start(applicationPath, QStringList() << socketToken << "editormode" << "-graphicssystem raster");

   if (runModus == NormalModus) {
       m_qmlPuppetPreviewProcess = new QProcess(QCoreApplication::instance());
       connect(m_qmlPuppetPreviewProcess.data(), SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processFinished(int,QProcess::ExitStatus)));
       m_qmlPuppetPreviewProcess->setProcessChannelMode(QProcess::ForwardedChannels);
       m_qmlPuppetPreviewProcess->start(applicationPath, QStringList() << socketToken << "previewmode" << "-graphicssystem raster");
   }

   connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(deleteLater()));

   m_qmlPuppetEditorProcess->waitForStarted();
   Q_ASSERT(m_qmlPuppetEditorProcess->state() == QProcess::Running);

   if (runModus == NormalModus) {
       m_qmlPuppetPreviewProcess->waitForStarted();
       Q_ASSERT(m_qmlPuppetPreviewProcess->state() == QProcess::Running);
   }

   if (!m_localServer->hasPendingConnections())
       m_localServer->waitForNewConnection(-1);

   m_firstSocket = m_localServer->nextPendingConnection();
   Q_ASSERT(m_firstSocket);
   connect(m_firstSocket.data(), SIGNAL(readyRead()), this, SLOT(readFirstDataStream()));

   if (runModus == NormalModus) {
       if (!m_localServer->hasPendingConnections())
           m_localServer->waitForNewConnection(-1);

       m_secondSocket = m_localServer->nextPendingConnection();
       Q_ASSERT(m_secondSocket);
       connect(m_secondSocket.data(), SIGNAL(readyRead()), this, SLOT(readSecondDataStream()));
   }

   m_localServer->close();
}

NodeInstanceServerProxy::~NodeInstanceServerProxy()
{
    if (m_firstSocket)
        m_firstSocket->close();

    if (m_secondSocket)
        m_secondSocket->close();


    if (m_qmlPuppetEditorProcess) {
        m_qmlPuppetEditorProcess->blockSignals(true);
        m_qmlPuppetEditorProcess->kill();
    }

    if (m_qmlPuppetPreviewProcess) {
        m_qmlPuppetPreviewProcess->blockSignals(true);
        m_qmlPuppetPreviewProcess->kill();
    }
}

void NodeInstanceServerProxy::dispatchCommand(const QVariant &command)
{
    static const int informationChangedCommandType = QMetaType::type("InformationChangedCommand");
    static const int valuesChangedCommandType = QMetaType::type("ValuesChangedCommand");
    static const int pixmapChangedCommandType = QMetaType::type("PixmapChangedCommand");
    static const int childrenChangedCommandType = QMetaType::type("ChildrenChangedCommand");
    static const int statePreviewImageChangedCommandType = QMetaType::type("StatePreviewImageChangedCommand");
    static const int componentCompletedCommandType = QMetaType::type("ComponentCompletedCommand");
    static const int synchronizeCommandType = QMetaType::type("SynchronizeCommand");

    if (command.userType() ==  informationChangedCommandType)
        nodeInstanceClient()->informationChanged(command.value<InformationChangedCommand>());
    else if (command.userType() ==  valuesChangedCommandType)
        nodeInstanceClient()->valuesChanged(command.value<ValuesChangedCommand>());
    else if (command.userType() ==  pixmapChangedCommandType)
        nodeInstanceClient()->pixmapChanged(command.value<PixmapChangedCommand>());
    else if (command.userType() == childrenChangedCommandType)
        nodeInstanceClient()->childrenChanged(command.value<ChildrenChangedCommand>());
    else if (command.userType() == statePreviewImageChangedCommandType)
        nodeInstanceClient()->statePreviewImagesChanged(command.value<StatePreviewImageChangedCommand>());
    else if (command.userType() == componentCompletedCommandType)
        nodeInstanceClient()->componentCompleted(command.value<ComponentCompletedCommand>());
    else if (command.userType() == synchronizeCommandType) {
        SynchronizeCommand synchronizeCommand = command.value<SynchronizeCommand>();
        m_synchronizeId = synchronizeCommand.synchronizeId();
    }  else
        Q_ASSERT(false);
}

NodeInstanceClientInterface *NodeInstanceServerProxy::nodeInstanceClient() const
{
    return m_nodeInstanceView.data();
}

static void writeCommandToSocket(const QVariant &command, QLocalSocket *socket)
{
    if(socket) {
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out << quint32(0);
        out << command;
        out.device()->seek(0);
        out << quint32(block.size() - sizeof(quint32));

        socket->write(block);
    }
}

void NodeInstanceServerProxy::writeCommand(const QVariant &command)
{
    writeCommandToSocket(command, m_firstSocket.data());
    writeCommandToSocket(command, m_secondSocket.data());

    if (m_runModus == TestModus) {
        static int synchronizeId = 0;
        synchronizeId++;
        SynchronizeCommand synchronizeCommand(synchronizeId);

        writeCommandToSocket(QVariant::fromValue(synchronizeCommand), m_firstSocket.data());

        while(m_firstSocket->waitForReadyRead()) {
                readFirstDataStream();
                if (m_synchronizeId == synchronizeId)
                    return;
        }
    }
}

void NodeInstanceServerProxy::processFinished(int /*exitCode*/, QProcess::ExitStatus exitStatus)
{
    if (m_firstSocket)
        m_firstSocket->close();
    if (m_secondSocket)
        m_secondSocket->close();
    if (exitStatus == QProcess::CrashExit)
        emit processCrashed();
}


void NodeInstanceServerProxy::readFirstDataStream()
{
    QList<QVariant> commandList;

    while (!m_firstSocket->atEnd()) {
        if (m_firstSocket->bytesAvailable() < int(sizeof(quint32)))
            break;

        QDataStream in(m_firstSocket.data());

        if (m_firstBlockSize == 0) {
            in >> m_firstBlockSize;
        }

        if (m_firstSocket->bytesAvailable() < m_firstBlockSize)
            break;

        QVariant command;
        in >> command;
        m_firstBlockSize = 0;

        Q_ASSERT(in.status() == QDataStream::Ok);

        commandList.append(command);
    }

    foreach (const QVariant &command, commandList) {
        dispatchCommand(command);
    }
}

void NodeInstanceServerProxy::readSecondDataStream()
{
    QList<QVariant> commandList;

    while (!m_secondSocket->atEnd()) {
        if (m_secondSocket->bytesAvailable() < int(sizeof(quint32)))
            break;

        QDataStream in(m_secondSocket.data());

        if (m_secondBlockSize == 0) {
            in >> m_secondBlockSize;
        }

        if (m_secondSocket->bytesAvailable() < m_secondBlockSize)
            break;

        QVariant command;
        in >> command;
        m_secondBlockSize = 0;

        Q_ASSERT(in.status() == QDataStream::Ok);

        commandList.append(command);
    }

    foreach (const QVariant &command, commandList) {
        dispatchCommand(command);
    }
}

void NodeInstanceServerProxy::createInstances(const CreateInstancesCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::changeFileUrl(const ChangeFileUrlCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::createScene(const CreateSceneCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::clearScene(const ClearSceneCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::removeInstances(const RemoveInstancesCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::removeProperties(const RemovePropertiesCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::changePropertyBindings(const ChangeBindingsCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::changePropertyValues(const ChangeValuesCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::reparentInstances(const ReparentInstancesCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::changeIds(const ChangeIdsCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::changeState(const ChangeStateCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::addImport(const AddImportCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::completeComponent(const CompleteComponentCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}
} // namespace QmlDesigner
