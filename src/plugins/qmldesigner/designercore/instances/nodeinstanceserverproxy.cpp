/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "nodeinstanceserverproxy.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QCoreApplication>
#include <QUuid>
#include <QFileInfo>
#include <QTimer>

#include "propertyabstractcontainer.h"
#include "propertyvaluecontainer.h"
#include "propertybindingcontainer.h"
#include "instancecontainer.h"
#include "createinstancescommand.h"
#include "createscenecommand.h"
#include "changevaluescommand.h"
#include "changebindingscommand.h"
#include "changeauxiliarycommand.h"
#include "changefileurlcommand.h"
#include "removeinstancescommand.h"
#include "clearscenecommand.h"
#include "removepropertiescommand.h"
#include "reparentinstancescommand.h"
#include "changeidscommand.h"
#include "changestatecommand.h"
#include "completecomponentcommand.h"
#include "changenodesourcecommand.h"

#include "informationchangedcommand.h"
#include "pixmapchangedcommand.h"
#include "valueschangedcommand.h"
#include "childrenchangedcommand.h"
#include "imagecontainer.h"
#include "statepreviewimagechangedcommand.h"
#include "componentcompletedcommand.h"
#include "tokencommand.h"
#include "removesharedmemorycommand.h"
#include "endpuppetcommand.h"
#include "synchronizecommand.h"
#include "debugoutputcommand.h"

#include "nodeinstanceview.h"

#include "import.h"

#include <utils/hostosinfo.h>

#include <QMessageBox>

namespace {
static QLatin1String qmlPuppetApplicationDirectoryForTests()
{
    if (Utils::HostOsInfo::isWindowsHost())
        //one more - debug/release dir
        return QLatin1String("/../../../../../../bin/");
    return QLatin1String("/../../../../../bin/");
}
} //namespace

namespace QmlDesigner {

static bool hasQtQuick2(NodeInstanceView *nodeInstanceView)
{
    if (nodeInstanceView && nodeInstanceView->model()) {
        foreach (const Import &import ,nodeInstanceView->model()->imports()) {
            if (import.url() ==  "QtQuick" && import.version().toDouble() >= 2.0)
                return true;
        }
    }

    return false;
}

NodeInstanceServerProxy::NodeInstanceServerProxy(NodeInstanceView *nodeInstanceView, RunModus runModus, const QString &pathToQt)
    : NodeInstanceServerInterface(nodeInstanceView),
      m_localServer(new QLocalServer(this)),
      m_nodeInstanceView(nodeInstanceView),
      m_firstBlockSize(0),
      m_secondBlockSize(0),
      m_thirdBlockSize(0),
      m_writeCommandCounter(0),
      m_firstLastReadCommandCounter(0),
      m_secondLastReadCommandCounter(0),
      m_thirdLastReadCommandCounter(0),
      m_runModus(runModus),
      m_synchronizeId(-1)
{
   Q_UNUSED(pathToQt);
   QString socketToken(QUuid::createUuid().toString());

   m_localServer->listen(socketToken);
   m_localServer->setMaxPendingConnections(3);

   QString applicationPath =  pathToQt + QLatin1String("/bin");
   if (runModus == TestModus) {
       applicationPath = QCoreApplication::applicationDirPath()
           + qmlPuppetApplicationDirectoryForTests()
           + qmlPuppetApplicationName();
   } else {
       applicationPath = macOSBundlePath(applicationPath);
       applicationPath += QLatin1Char('/') + qmlPuppetApplicationName();
       if (!QFileInfo(applicationPath).exists()) { //No qmlpuppet in Qt
           //We have to find out how to give not too intrusive feedback
           applicationPath =  QCoreApplication::applicationDirPath();
           applicationPath = macOSBundlePath(applicationPath);
           applicationPath += QLatin1Char('/') + qmlPuppetApplicationName();
       }
   }

   QByteArray envImportPath = qgetenv("QTCREATOR_QMLPUPPET_PATH");
   if (!envImportPath.isEmpty())
       applicationPath = envImportPath;

   QProcessEnvironment enviroment = QProcessEnvironment::systemEnvironment();

   if (QFileInfo(applicationPath).exists()) {
       m_qmlPuppetEditorProcess = new QProcess;
       m_qmlPuppetEditorProcess->setProcessEnvironment(enviroment);
       m_qmlPuppetEditorProcess->setObjectName("EditorProcess");
       connect(m_qmlPuppetEditorProcess.data(), SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processFinished(int,QProcess::ExitStatus)));
       connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), m_qmlPuppetEditorProcess.data(), SLOT(kill()));
       bool fowardQmlpuppetOutput = !qgetenv("FORWARD_QMLPUPPET_OUTPUT").isEmpty();
       if (fowardQmlpuppetOutput)
           m_qmlPuppetEditorProcess->setProcessChannelMode(QProcess::ForwardedChannels);
       m_qmlPuppetEditorProcess->start(applicationPath, QStringList() << socketToken << "editormode" << "-graphicssystem raster");

       if (runModus == NormalModus) {
           m_qmlPuppetPreviewProcess = new QProcess;
           m_qmlPuppetPreviewProcess->setProcessEnvironment(enviroment);
           m_qmlPuppetPreviewProcess->setObjectName("PreviewProcess");
           connect(m_qmlPuppetPreviewProcess.data(), SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processFinished(int,QProcess::ExitStatus)));
           connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), m_qmlPuppetPreviewProcess.data(), SLOT(kill()));
           if (fowardQmlpuppetOutput)
               m_qmlPuppetPreviewProcess->setProcessChannelMode(QProcess::ForwardedChannels);
           m_qmlPuppetPreviewProcess->start(applicationPath, QStringList() << socketToken << "previewmode" << "-graphicssystem raster");

           m_qmlPuppetRenderProcess = new QProcess;
           m_qmlPuppetRenderProcess->setProcessEnvironment(enviroment);
           m_qmlPuppetRenderProcess->setObjectName("RenderProcess");
           connect(m_qmlPuppetRenderProcess.data(), SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processFinished(int,QProcess::ExitStatus)));
           connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), m_qmlPuppetRenderProcess.data(), SLOT(kill()));
           if (fowardQmlpuppetOutput)
               m_qmlPuppetRenderProcess->setProcessChannelMode(QProcess::ForwardedChannels);
           m_qmlPuppetRenderProcess->start(applicationPath, QStringList() << socketToken << "rendermode" << "-graphicssystem raster");

       }

       connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(deleteLater()));

       if (m_qmlPuppetEditorProcess->waitForStarted(10000)) {
           connect(m_qmlPuppetEditorProcess.data(), SIGNAL(finished(int)), m_qmlPuppetEditorProcess.data(),SLOT(deleteLater()));

           if (runModus == NormalModus) {
               m_qmlPuppetPreviewProcess->waitForStarted();
               connect(m_qmlPuppetPreviewProcess.data(), SIGNAL(finished(int)), m_qmlPuppetPreviewProcess.data(),SLOT(deleteLater()));

               m_qmlPuppetRenderProcess->waitForStarted();
               connect(m_qmlPuppetRenderProcess.data(), SIGNAL(finished(int)), m_qmlPuppetRenderProcess.data(),SLOT(deleteLater()));
           }

           if (!m_localServer->hasPendingConnections())
               m_localServer->waitForNewConnection(10000);

           m_firstSocket = m_localServer->nextPendingConnection();
           connect(m_firstSocket.data(), SIGNAL(readyRead()), this, SLOT(readFirstDataStream()));

           if (runModus == NormalModus) {
               if (!m_localServer->hasPendingConnections())
                   m_localServer->waitForNewConnection(10000);

               m_secondSocket = m_localServer->nextPendingConnection();
               connect(m_secondSocket.data(), SIGNAL(readyRead()), this, SLOT(readSecondDataStream()));

               if (!m_localServer->hasPendingConnections())
                   m_localServer->waitForNewConnection(10000);

               m_thirdSocket = m_localServer->nextPendingConnection();
               connect(m_thirdSocket.data(), SIGNAL(readyRead()), this, SLOT(readThirdDataStream()));
           }

       } else {
               QMessageBox::warning(0, tr("Cannot Start QML Puppet Executable"),
                                    tr("The executable of the QML Puppet process (%1) cannot be started. "
                                       "Please check your installation. "
                                       "QML Puppet is a process which runs in the background to render the items.").
                                    arg(applicationPath));
       }

       m_localServer->close();

   } else {
           QMessageBox::warning(0, tr("Cannot Find QML Puppet Executable"),
                                tr("The executable of the QML Puppet process (%1) cannot be found. "
                                   "Please check your installation. "
                                   "QML Puppet is a process which runs in the background to render the items.").
                                arg(applicationPath));
   }
}

NodeInstanceServerProxy::~NodeInstanceServerProxy()
{
    disconnect(this, SLOT(processFinished(int,QProcess::ExitStatus)));

    writeCommand(QVariant::fromValue(EndPuppetCommand()));

    if (m_firstSocket)
        m_firstSocket->close();

    if (m_secondSocket)
        m_secondSocket->close();

    if (m_thirdSocket)
        m_thirdSocket->close();


    if (m_qmlPuppetEditorProcess)
        QTimer::singleShot(3000, m_qmlPuppetEditorProcess.data(), SLOT(terminate()));

    if (m_qmlPuppetPreviewProcess)
        QTimer::singleShot(3000, m_qmlPuppetPreviewProcess.data(), SLOT(terminate()));

    if (m_qmlPuppetRenderProcess)
         QTimer::singleShot(3000, m_qmlPuppetRenderProcess.data(), SLOT(terminate()));
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
    static const int tokenCommandType = QMetaType::type("TokenCommand");
    static const int debugOutputCommandType = QMetaType::type("DebugOutputCommand");

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
    else if (command.userType() == tokenCommandType)
        nodeInstanceClient()->token(command.value<TokenCommand>());
    else if (command.userType() == debugOutputCommandType)
        nodeInstanceClient()->debugOutput(command.value<DebugOutputCommand>());
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

static void writeCommandToSocket(const QVariant &command, QLocalSocket *socket, unsigned int commandCounter)
{
    if (socket) {
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_8);
        out << quint32(0);
        out << quint32(commandCounter);
        out << command;
        out.device()->seek(0);
        out << quint32(block.size() - sizeof(quint32));

        socket->write(block);
    }
}

void NodeInstanceServerProxy::writeCommand(const QVariant &command)
{
    writeCommandToSocket(command, m_firstSocket.data(), m_writeCommandCounter);
    writeCommandToSocket(command, m_secondSocket.data(), m_writeCommandCounter);
    writeCommandToSocket(command, m_thirdSocket.data(), m_writeCommandCounter);
    m_writeCommandCounter++;
    if (m_runModus == TestModus) {
        static int synchronizeId = 0;
        synchronizeId++;
        SynchronizeCommand synchronizeCommand(synchronizeId);

        writeCommandToSocket(QVariant::fromValue(synchronizeCommand), m_firstSocket.data(), m_writeCommandCounter);
        m_writeCommandCounter++;

        while (m_firstSocket->waitForReadyRead(100)) {
                readFirstDataStream();
                if (m_synchronizeId == synchronizeId)
                    return;
        }
    }
}

void NodeInstanceServerProxy::processFinished(int /*exitCode*/, QProcess::ExitStatus exitStatus)
{
    qDebug() << "Process finished:" << sender();

    writeCommand(QVariant::fromValue(EndPuppetCommand()));

    if (m_firstSocket)
        m_firstSocket->close();
    if (m_secondSocket)
        m_secondSocket->close();
    if (m_thirdSocket)
        m_thirdSocket->close();

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
        in.setVersion(QDataStream::Qt_4_8);

        if (m_firstBlockSize == 0)
            in >> m_firstBlockSize;

        if (m_firstSocket->bytesAvailable() < m_firstBlockSize)
            break;

        quint32 commandCounter;
        in >> commandCounter;
        bool commandLost = !((m_firstLastReadCommandCounter == 0 && commandCounter == 0) || (m_firstLastReadCommandCounter + 1 == commandCounter));
        if (commandLost)
            qDebug() << "server command lost: " << m_firstLastReadCommandCounter <<  commandCounter;
        m_firstLastReadCommandCounter = commandCounter;


        QVariant command;
        in >> command;
        m_firstBlockSize = 0;

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
        in.setVersion(QDataStream::Qt_4_8);

        if (m_secondBlockSize == 0)
            in >> m_secondBlockSize;

        if (m_secondSocket->bytesAvailable() < m_secondBlockSize)
            break;

        quint32 commandCounter;
        in >> commandCounter;
        bool commandLost = !((m_secondLastReadCommandCounter == 0 && commandCounter == 0) || (m_secondLastReadCommandCounter + 1 == commandCounter));
        if (commandLost)
            qDebug() << "server command lost: " << m_secondLastReadCommandCounter <<  commandCounter;
        m_secondLastReadCommandCounter = commandCounter;


        QVariant command;
        in >> command;
        m_secondBlockSize = 0;

        commandList.append(command);
    }

    foreach (const QVariant &command, commandList) {
        dispatchCommand(command);
    }
}

void NodeInstanceServerProxy::readThirdDataStream()
{
    QList<QVariant> commandList;

    while (!m_thirdSocket->atEnd()) {
        if (m_thirdSocket->bytesAvailable() < int(sizeof(quint32)))
            break;

        QDataStream in(m_thirdSocket.data());
        in.setVersion(QDataStream::Qt_4_8);

        if (m_thirdBlockSize == 0)
            in >> m_thirdBlockSize;

        if (m_thirdSocket->bytesAvailable() < m_thirdBlockSize)
            break;

        quint32 commandCounter;
        in >> commandCounter;
        bool commandLost = !((m_thirdLastReadCommandCounter == 0 && commandCounter == 0) || (m_thirdLastReadCommandCounter + 1 == commandCounter));
        if (commandLost)
            qDebug() << "server command lost: " << m_thirdLastReadCommandCounter <<  commandCounter;
        m_thirdLastReadCommandCounter = commandCounter;


        QVariant command;
        in >> command;
        m_thirdBlockSize = 0;

        commandList.append(command);
    }

    foreach (const QVariant &command, commandList) {
        dispatchCommand(command);
    }
}

QString NodeInstanceServerProxy::qmlPuppetApplicationName() const
{
    if (hasQtQuick2(m_nodeInstanceView.data()))
        return QLatin1String("qml2puppet" QTC_HOST_EXE_SUFFIX);
    return QLatin1String("qmlpuppet" QTC_HOST_EXE_SUFFIX);
}

QString NodeInstanceServerProxy::macOSBundlePath(const QString &path) const
{
    QString applicationPath = path;
    if (Utils::HostOsInfo::isMacHost())
        applicationPath += QLatin1String("/qmlpuppet.app/Contents/MacOS");
   return applicationPath;
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

void NodeInstanceServerProxy::changeAuxiliaryValues(const ChangeAuxiliaryCommand &command)
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

void NodeInstanceServerProxy::completeComponent(const CompleteComponentCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::changeNodeSource(const ChangeNodeSourceCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::token(const TokenCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::removeSharedMemory(const RemoveSharedMemoryCommand &command)
{
   writeCommand(QVariant::fromValue(command));
}

} // namespace QmlDesigner
