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

#include "nodeinstanceserverproxy.h"

#include "puppetcreator.h"

#include <createinstancescommand.h>
#include <createscenecommand.h>
#include <changevaluescommand.h>
#include <changebindingscommand.h>
#include <changeauxiliarycommand.h>
#include <changefileurlcommand.h>
#include <removeinstancescommand.h>
#include <clearscenecommand.h>
#include <removepropertiescommand.h>
#include <reparentinstancescommand.h>
#include <changeidscommand.h>
#include <changestatecommand.h>
#include <completecomponentcommand.h>
#include <changenodesourcecommand.h>

#include <informationchangedcommand.h>
#include <pixmapchangedcommand.h>
#include <valueschangedcommand.h>
#include <childrenchangedcommand.h>
#include <statepreviewimagechangedcommand.h>
#include <componentcompletedcommand.h>
#include <tokencommand.h>
#include <removesharedmemorycommand.h>
#include <endpuppetcommand.h>
#include <synchronizecommand.h>
#include <debugoutputcommand.h>

#include <nodeinstanceview.h>
#include <import.h>
#include <rewriterview.h>

#ifndef QMLDESIGNER_TEST
#include <qmldesignerplugin.h>
#endif

#include <coreplugin/icore.h>
#include <utils/hostosinfo.h>
#include <coreplugin/messagebox.h>
#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/kit.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtsupportconstants.h>

#include <QLocalServer>
#include <QLocalSocket>
#include <QLoggingCategory>
#include <QProcess>
#include <QCoreApplication>
#include <QUuid>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QTextStream>
#include <QMessageBox>

namespace QmlDesigner {

static Q_LOGGING_CATEGORY(instanceViewBenchmark, "qtc.nodeinstances.init")

void NodeInstanceServerProxy::showCannotConnectToPuppetWarningAndSwitchToEditMode()
{
#ifndef QMLDESIGNER_TEST
    Core::AsynchronousMessageBox::warning(tr("Cannot Connect to QML Emulation Layer (QML Puppet)"),
                                          tr("The executable of the QML emulation layer (QML Puppet) may not be responding. "
                                             "Switching to another kit might help."));

    QmlDesignerPlugin::instance()->switchToTextModeDeferred();
    m_nodeInstanceView->emitDocumentMessage(tr("Cannot Connect to QML Emulation Layer (QML Puppet)"));
#endif

}

NodeInstanceServerProxy::NodeInstanceServerProxy(NodeInstanceView *nodeInstanceView,
                                                 RunModus runModus,
                                                 ProjectExplorer::Kit *kit,
                                                 ProjectExplorer::Project *project)
    : NodeInstanceServerInterface(nodeInstanceView),
      m_localServer(new QLocalServer(this)),
      m_nodeInstanceView(nodeInstanceView),
      m_runModus(runModus)
{
    if (instanceViewBenchmark().isInfoEnabled())
        m_benchmarkTimer.start();

   QString socketToken(QUuid::createUuid().toString());
   m_localServer->listen(socketToken);
   m_localServer->setMaxPendingConnections(3);

   PuppetCreator puppetCreator(kit, project, nodeInstanceView->model());
   puppetCreator.setQrcMappingString(qrcMappingString());

   puppetCreator.createQml2PuppetExecutableIfMissing();

   m_qmlPuppetEditorProcess = puppetCreator.createPuppetProcess("editormode",
                                                              socketToken,
                                                              this,
                                                              SLOT(printEditorProcessOutput()),
                                                              SLOT(processFinished(int,QProcess::ExitStatus)));

   if (runModus == NormalModus) {
       m_qmlPuppetRenderProcess = puppetCreator.createPuppetProcess("rendermode",
                                                                    socketToken,
                                                                    this,
                                                                    SLOT(printRenderProcessOutput()),
                                                                    SLOT(processFinished(int,QProcess::ExitStatus)));
       m_qmlPuppetPreviewProcess = puppetCreator.createPuppetProcess("previewmode",
                                                                     socketToken,
                                                                     this,
                                                                     SLOT(printPreviewProcessOutput()),
                                                                     SLOT(processFinished(int,QProcess::ExitStatus)));
   }

   const int second = 1000;
   const int waitConstant = 8 * second;
   if (m_qmlPuppetEditorProcess->waitForStarted(waitConstant)) {
       connect(m_qmlPuppetEditorProcess.data(), static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            m_qmlPuppetEditorProcess.data(), &QProcess::deleteLater);
    qCInfo(instanceViewBenchmark) << "puppets started:" << m_benchmarkTimer.elapsed();

       if (runModus == NormalModus) {
           m_qmlPuppetPreviewProcess->waitForStarted(waitConstant / 2);
           connect(m_qmlPuppetPreviewProcess.data(), static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                m_qmlPuppetPreviewProcess.data(), &QProcess::deleteLater);

           m_qmlPuppetRenderProcess->waitForStarted(waitConstant / 2);
           connect(m_qmlPuppetRenderProcess.data(), static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                m_qmlPuppetRenderProcess.data(), &QProcess::deleteLater);
       }

       bool connectedToPuppet = true;

       if (!m_localServer->hasPendingConnections())
           connectedToPuppet = m_localServer->waitForNewConnection(waitConstant / 4);

       if (connectedToPuppet) {
           m_firstSocket = m_localServer->nextPendingConnection();
           connect(m_firstSocket.data(), &QIODevice::readyRead, this,
                   &NodeInstanceServerProxy::readFirstDataStream);

           if (runModus == NormalModus) {
               if (!m_localServer->hasPendingConnections())
                   connectedToPuppet = m_localServer->waitForNewConnection(waitConstant / 4);

               if (connectedToPuppet) {
                   m_secondSocket = m_localServer->nextPendingConnection();
                   connect(m_secondSocket.data(), &QIODevice::readyRead, this, &NodeInstanceServerProxy::readSecondDataStream);

                   if (!m_localServer->hasPendingConnections())
                        connectedToPuppet = m_localServer->waitForNewConnection(waitConstant / 4);

    qCInfo(instanceViewBenchmark) << "puppets connected:" << m_benchmarkTimer.elapsed();
                   if (connectedToPuppet) {
                       m_thirdSocket = m_localServer->nextPendingConnection();
                       connect(m_thirdSocket.data(), &QIODevice::readyRead, this, &NodeInstanceServerProxy::readThirdDataStream);
                   } else {
                       showCannotConnectToPuppetWarningAndSwitchToEditMode();
                   }
               } else {
                   showCannotConnectToPuppetWarningAndSwitchToEditMode();
               }
           }
       } else {
           showCannotConnectToPuppetWarningAndSwitchToEditMode();
       }

   } else {
       showCannotConnectToPuppetWarningAndSwitchToEditMode();
   }

   m_localServer->close();


   int indexOfCapturePuppetStream = QCoreApplication::arguments().indexOf("-capture-puppet-stream");
   if (indexOfCapturePuppetStream > 0) {
       m_captureFileForTest.setFileName(QCoreApplication::arguments().at(indexOfCapturePuppetStream + 1));
       bool isOpen = m_captureFileForTest.open(QIODevice::WriteOnly);
       qDebug() << "file is open: " << isOpen;
   }

#ifndef QMLDESIGNER_TEST
   DesignerSettings settings = QmlDesignerPlugin::instance()->settings();
   int timeOutTime = settings.value(DesignerSettingsKey::PUPPET_KILL_TIMEOUT).toInt();
   m_firstTimer.setInterval(timeOutTime);
   m_secondTimer.setInterval(timeOutTime);
   m_thirdTimer.setInterval(timeOutTime);

    if (QmlDesignerPlugin::instance()->settings().value(DesignerSettingsKey::
            DEBUG_PUPPET).toString().isEmpty()) {
       connect(&m_firstTimer, SIGNAL(timeout()), this, SLOT(processFinished()));
       connect(&m_secondTimer, SIGNAL(timeout()), this, SLOT(processFinished()));
       connect(&m_thirdTimer, SIGNAL(timeout()), this, SLOT(processFinished()));
   }
#endif
}

NodeInstanceServerProxy::~NodeInstanceServerProxy()
{
    disconnect(this, SLOT(processFinished(int,QProcess::ExitStatus)));

    writeCommand(QVariant::fromValue(EndPuppetCommand()));

    if (m_firstSocket) {
        m_firstSocket->waitForBytesWritten(1000);
        m_firstSocket->abort();
    }

    if (m_secondSocket) {
        m_secondSocket->waitForBytesWritten(1000);
        m_secondSocket->abort();
    }

    if (m_thirdSocket) {
        m_thirdSocket->waitForBytesWritten(1000);
        m_thirdSocket->abort();
    }

    if (m_qmlPuppetEditorProcess) {
        QTimer::singleShot(3000, m_qmlPuppetEditorProcess.data(), &QProcess::terminate);
        QTimer::singleShot(6000, m_qmlPuppetEditorProcess.data(), &QProcess::kill);
    }

    if (m_qmlPuppetPreviewProcess) {
        QTimer::singleShot(3000, m_qmlPuppetPreviewProcess.data(), &QProcess::terminate);
        QTimer::singleShot(6000, m_qmlPuppetPreviewProcess.data(), &QProcess::kill);
    }

    if (m_qmlPuppetRenderProcess) {
         QTimer::singleShot(3000, m_qmlPuppetRenderProcess.data(), &QProcess::terminate);
         QTimer::singleShot(6000, m_qmlPuppetRenderProcess.data(), &QProcess::kill);
    }
}

void NodeInstanceServerProxy::dispatchCommand(const QVariant &command, PuppetStreamType puppetStreamType)
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
    static const int puppetAliveCommandType = QMetaType::type("PuppetAliveCommand");

    qCInfo(instanceViewBenchmark) << "dispatching command" << command.userType() << command.typeName();
    if (command.userType() ==  informationChangedCommandType) {
        nodeInstanceClient()->informationChanged(command.value<InformationChangedCommand>());
    } else if (command.userType() ==  valuesChangedCommandType) {
        nodeInstanceClient()->valuesChanged(command.value<ValuesChangedCommand>());
    } else if (command.userType() ==  pixmapChangedCommandType) {
        nodeInstanceClient()->pixmapChanged(command.value<PixmapChangedCommand>());
    } else if (command.userType() == childrenChangedCommandType) {
        nodeInstanceClient()->childrenChanged(command.value<ChildrenChangedCommand>());
    } else if (command.userType() == statePreviewImageChangedCommandType) {
        nodeInstanceClient()->statePreviewImagesChanged(command.value<StatePreviewImageChangedCommand>());
    } else if (command.userType() == componentCompletedCommandType) {
        nodeInstanceClient()->componentCompleted(command.value<ComponentCompletedCommand>());
    } else if (command.userType() == tokenCommandType) {
        nodeInstanceClient()->token(command.value<TokenCommand>());
    } else if (command.userType() == debugOutputCommandType) {
        nodeInstanceClient()->debugOutput(command.value<DebugOutputCommand>());
    } else if (command.userType() == puppetAliveCommandType) {
        puppetAlive(puppetStreamType);
    } else if (command.userType() == synchronizeCommandType) {
        SynchronizeCommand synchronizeCommand = command.value<SynchronizeCommand>();
        m_synchronizeId = synchronizeCommand.synchronizeId();
    }  else
        Q_ASSERT(false);
     qCInfo(instanceViewBenchmark) << "dispatching command" << "done" << command.userType();
}

NodeInstanceClientInterface *NodeInstanceServerProxy::nodeInstanceClient() const
{
    return m_nodeInstanceView.data();
}

void NodeInstanceServerProxy::puppetAlive(NodeInstanceServerProxy::PuppetStreamType puppetStreamType)
{
    switch (puppetStreamType) {
    case FirstPuppetStream:
        m_firstTimer.stop();
        m_firstTimer.start();
        break;
    case SecondPuppetStream:
        m_secondTimer.stop();
        m_secondTimer.start();
        break;
    case ThirdPuppetStream:
        m_thirdTimer.stop();
        m_thirdTimer.start();
        break;
    default:
        break;
    }
}

QString NodeInstanceServerProxy::qrcMappingString() const
{
    if (m_nodeInstanceView && m_nodeInstanceView.data()->model()) {
        RewriterView *rewriterView = m_nodeInstanceView.data()->model()->rewriterView();
        if (rewriterView) {
            QString mappingString;

            typedef QPair<QString, QString> StringPair;

            foreach (const StringPair &pair, rewriterView->qrcMapping()) {
                if (!mappingString.isEmpty())
                    mappingString.append(QLatin1String(";"));
                mappingString.append(pair.first);
                mappingString.append(QLatin1String("="));
                mappingString.append(pair.second);
            }

            return mappingString;
        }
    }

    return QString();
}

void NodeInstanceServerProxy::processFinished()
{
    processFinished(-1, QProcess::CrashExit);
}

static void writeCommandToIODecive(const QVariant &command, QIODevice *ioDevice, unsigned int commandCounter)
{
    if (ioDevice) {
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_8);
        out << quint32(0);
        out << quint32(commandCounter);
        out << command;
        out.device()->seek(0);
        out << quint32(block.size() - sizeof(quint32));

        ioDevice->write(block);
    }
}

void NodeInstanceServerProxy::writeCommand(const QVariant &command)
{
    writeCommandToIODecive(command, m_firstSocket.data(), m_writeCommandCounter);
    writeCommandToIODecive(command, m_secondSocket.data(), m_writeCommandCounter);
    writeCommandToIODecive(command, m_thirdSocket.data(), m_writeCommandCounter);

    if (m_captureFileForTest.isWritable()) {
        qDebug() << "Write stream to file: " << m_captureFileForTest.fileName();
        writeCommandToIODecive(command, &m_captureFileForTest, m_writeCommandCounter);
        qDebug() << "\twrite file: " << m_captureFileForTest.pos();
    }

    m_writeCommandCounter++;
    if (m_runModus == TestModus) {
        static int synchronizeId = 0;
        synchronizeId++;
        SynchronizeCommand synchronizeCommand(synchronizeId);

        writeCommandToIODecive(QVariant::fromValue(synchronizeCommand), m_firstSocket.data(), m_writeCommandCounter);
        m_writeCommandCounter++;

        while (m_firstSocket->waitForReadyRead(100)) {
                readFirstDataStream();
                if (m_synchronizeId == synchronizeId)
                    return;
        }
    }
}

void NodeInstanceServerProxy::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QProcess* finishedProcess = qobject_cast<QProcess*>(sender());
    if (finishedProcess)
        qWarning() << "Process" << (exitStatus == QProcess::CrashExit ? "crashed:" : "finished:") << finishedProcess->arguments() << "exitCode:" << exitCode;
    else
        qWarning() << "Process" << (exitStatus == QProcess::CrashExit ? "crashed:" : "finished:") << sender() << "exitCode:" << exitCode;

    if (m_captureFileForTest.isOpen()) {
        m_captureFileForTest.close();
        Core::AsynchronousMessageBox::warning(tr("QML Emulation Layer (QML Puppet) Crashed"),
                             tr("You are recording a puppet stream and the emulations layer crashed. "
                                "It is recommended to reopen the Qt Quick Designer and start again."));
    }


    writeCommand(QVariant::fromValue(EndPuppetCommand()));

    if (m_firstSocket) {
        m_firstSocket->waitForBytesWritten(1000);
        m_firstSocket->abort();
    }

    if (m_secondSocket) {
        m_secondSocket->waitForBytesWritten(1000);
        m_secondSocket->abort();
    }

    if (m_thirdSocket) {
        m_thirdSocket->waitForBytesWritten(1000);
        m_thirdSocket->abort();
    }

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
        dispatchCommand(command, FirstPuppetStream);
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
        dispatchCommand(command, SecondPuppetStream);
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
        dispatchCommand(command, ThirdPuppetStream);
    }
}

void NodeInstanceServerProxy::printEditorProcessOutput()
{
    while (m_qmlPuppetEditorProcess && m_qmlPuppetEditorProcess->canReadLine()) {
        QByteArray line = m_qmlPuppetEditorProcess->readLine();
        line.chop(1);
        qDebug().nospace() << "Editor Puppet: " << line;
    }
    qDebug() << "\n";
}

void NodeInstanceServerProxy::printPreviewProcessOutput()
{
    while (m_qmlPuppetPreviewProcess && m_qmlPuppetPreviewProcess->canReadLine()) {
        QByteArray line = m_qmlPuppetPreviewProcess->readLine();
        line.chop(1);
        qDebug().nospace() << "Preview Puppet: " << line;
    }
    qDebug() << "\n";
}

void NodeInstanceServerProxy::printRenderProcessOutput()
{
    while (m_qmlPuppetRenderProcess && m_qmlPuppetRenderProcess->canReadLine()) {
        QByteArray line = m_qmlPuppetRenderProcess->readLine();
        line.chop(1);
        qDebug().nospace() << "Render Puppet: " << line;
    }

    qDebug() << "\n";
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
    qCInfo(instanceViewBenchmark) << Q_FUNC_INFO << m_benchmarkTimer.elapsed();
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

void NodeInstanceServerProxy::benchmark(const QString &message)
{
    qCInfo(instanceViewBenchmark) << message << m_benchmarkTimer.elapsed();
}

} // namespace QmlDesigner
