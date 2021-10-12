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

#include "connectionmanagerinterface.h"
#include "puppetcreator.h"

#include <changeauxiliarycommand.h>
#include <changebindingscommand.h>
#include <changefileurlcommand.h>
#include <changeidscommand.h>
#include <changelanguagecommand.h>
#include <changenodesourcecommand.h>
#include <changepreviewimagesizecommand.h>
#include <changeselectioncommand.h>
#include <changestatecommand.h>
#include <changevaluescommand.h>
#include <childrenchangedcommand.h>
#include <clearscenecommand.h>
#include <completecomponentcommand.h>
#include <componentcompletedcommand.h>
#include <createinstancescommand.h>
#include <createscenecommand.h>
#include <debugoutputcommand.h>
#include <endpuppetcommand.h>
#include <informationchangedcommand.h>
#include <inputeventcommand.h>
#include <nanotracecommand.h>
#include <pixmapchangedcommand.h>
#include <puppettocreatorcommand.h>
#include <removeinstancescommand.h>
#include <removepropertiescommand.h>
#include <removesharedmemorycommand.h>
#include <reparentinstancescommand.h>
#include <statepreviewimagechangedcommand.h>
#include <synchronizecommand.h>
#include <tokencommand.h>
#include <update3dviewstatecommand.h>
#include <valueschangedcommand.h>
#include <view3dactioncommand.h>
#include <requestmodelnodepreviewimagecommand.h>

#include <import.h>
#include <nodeinstanceview.h>
#include <rewriterview.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <projectexplorer/kit.h>
#include <utils/hostosinfo.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <nanotrace/nanotrace.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QTextStream>
#include <QTimer>
#include <QUuid>

namespace QmlDesigner {

static Q_LOGGING_CATEGORY(instanceViewBenchmark, "qtc.nodeinstances.init", QtWarningMsg);

NodeInstanceServerProxy::NodeInstanceServerProxy(NodeInstanceView *nodeInstanceView,
                                                 ProjectExplorer::Target *target,
                                                 ConnectionManagerInterface &connectionManager)
    : m_nodeInstanceView(nodeInstanceView)
    , m_connectionManager{connectionManager}

{
    if (instanceViewBenchmark().isInfoEnabled())
        m_benchmarkTimer.start();

    m_connectionManager.setUp(this, qrcMappingString(), target, nodeInstanceView);

    qCInfo(instanceViewBenchmark) << "puppets setup:" << m_benchmarkTimer.elapsed();
}

NodeInstanceServerProxy::~NodeInstanceServerProxy()
{
    m_connectionManager.shutDown();
}

void NodeInstanceServerProxy::dispatchCommand(const QVariant &command)
{
    NANOTRACE_SCOPE_ARGS("Update", "dispatchCommand", {"name", command.typeName()});

    static const int informationChangedCommandType = QMetaType::type("InformationChangedCommand");
    static const int valuesChangedCommandType = QMetaType::type("ValuesChangedCommand");
    static const int valuesModifiedCommandType = QMetaType::type("ValuesModifiedCommand");
    static const int pixmapChangedCommandType = QMetaType::type("PixmapChangedCommand");
    static const int childrenChangedCommandType = QMetaType::type("ChildrenChangedCommand");
    static const int statePreviewImageChangedCommandType = QMetaType::type("StatePreviewImageChangedCommand");
    static const int componentCompletedCommandType = QMetaType::type("ComponentCompletedCommand");
    static const int tokenCommandType = QMetaType::type("TokenCommand");
    static const int debugOutputCommandType = QMetaType::type("DebugOutputCommand");
    static const int changeSelectionCommandType = QMetaType::type("ChangeSelectionCommand");
    static const int puppetToCreatorCommandType = QMetaType::type("PuppetToCreatorCommand");
    static const int SyncNanotraceCommandType = QMetaType::type("SyncNanotraceCommand");

    qCInfo(instanceViewBenchmark) << "dispatching command" << command.userType() << command.typeName();
    if (command.userType() == informationChangedCommandType) {
        nodeInstanceClient()->informationChanged(command.value<InformationChangedCommand>());
    } else if (command.userType() == valuesChangedCommandType) {
        nodeInstanceClient()->valuesChanged(command.value<ValuesChangedCommand>());
    } else if (command.userType() == valuesModifiedCommandType) {
        nodeInstanceClient()->valuesModified(command.value<ValuesModifiedCommand>());
    } else if (command.userType() == pixmapChangedCommandType) {
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
    } else if (command.userType() == changeSelectionCommandType) {
        nodeInstanceClient()->selectionChanged(command.value<ChangeSelectionCommand>());
    } else if (command.userType() == puppetToCreatorCommandType) {
        nodeInstanceClient()->handlePuppetToCreatorCommand(command.value<PuppetToCreatorCommand>());
    } else if (command.userType() == SyncNanotraceCommandType) {
        // ignore.
    } else {
        Q_ASSERT(false);
    }

    qCInfo(instanceViewBenchmark) << "dispatching command" << "done" << command.userType();
}

NodeInstanceClientInterface *NodeInstanceServerProxy::nodeInstanceClient() const
{
    return m_nodeInstanceView;
}

QString NodeInstanceServerProxy::qrcMappingString() const
{
    if (m_nodeInstanceView && m_nodeInstanceView->model()) {
        RewriterView *rewriterView = m_nodeInstanceView->model()->rewriterView();
        if (rewriterView) {
            QString mappingString;

            using StringPair = QPair<QString, QString>;

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

void NodeInstanceServerProxy::writeCommand(const QVariant &command)
{
#ifdef NANOTRACE_ENABLED
    if (command.userType() == QMetaType::type("SyncNanotraceCommand")) {
        SyncNanotraceCommand cmd = command.value<SyncNanotraceCommand>();
        NANOTRACE_INSTANT_ARGS("Sync", "writeCommand",
            {"name", cmd.name().toStdString()},
            {"counter", m_connectionManager.writeCounter()});

    } else {
        NANOTRACE_INSTANT_ARGS("Update", "writeCommand",
            {"name", command.typeName()},
            {"counter", m_connectionManager.writeCounter()});
    }
#endif

    m_connectionManager.writeCommand(command);
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

void NodeInstanceServerProxy::update3DViewState(const Update3dViewStateCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::removeInstances(const RemoveInstancesCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::changeSelection(const ChangeSelectionCommand &command)
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

void NodeInstanceServerProxy::inputEvent(const InputEventCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::view3DAction(const View3DActionCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::requestModelNodePreviewImage(const RequestModelNodePreviewImageCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::changeLanguage(const ChangeLanguageCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::changePreviewImageSize(const ChangePreviewImageSizeCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

} // namespace QmlDesigner
