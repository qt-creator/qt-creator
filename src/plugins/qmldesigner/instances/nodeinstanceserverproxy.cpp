// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nodeinstanceserverproxy.h"

#include "connectionmanagerinterface.h"
#include "nodeinstancetracing.h"

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
#include <nanotrace/nanotrace.h>
#include <projectexplorer/kit.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtsupportconstants.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLoggingCategory>
#include <QTextStream>
#include <QTimer>
#include <QUuid>

namespace QmlDesigner {

static Q_LOGGING_CATEGORY(instanceViewBenchmark, "qtc.nodeinstances.init", QtWarningMsg);

using NanotraceHR::keyValue;
using NodeInstanceTracing::category;

NodeInstanceServerProxy::NodeInstanceServerProxy(NodeInstanceView *nodeInstanceView,
                                                 ProjectExplorer::Target *target,
                                                 ConnectionManagerInterface &connectionManager,
                                                 ExternalDependenciesInterface &externalDependencies)
    : m_nodeInstanceView(nodeInstanceView)
    , m_connectionManager{connectionManager}

{
    NanotraceHR::Tracer tracer{"node instance server proxy constructor", category()};

    if (instanceViewBenchmark().isInfoEnabled())
        m_benchmarkTimer.start();

    m_connectionManager.setUp(this, qrcMappingString(), target, nodeInstanceView, externalDependencies);

    qCInfo(instanceViewBenchmark) << "puppets setup:" << m_benchmarkTimer.elapsed();
}

NodeInstanceServerProxy::~NodeInstanceServerProxy()
{
    NanotraceHR::Tracer tracer{"node instance server proxy destructor", category()};

    m_connectionManager.shutDown();
}

void NodeInstanceServerProxy::dispatchCommand(const QVariant &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy dispatch command", category()};

    NANOTRACE_SCOPE_ARGS("Update", "dispatchCommand", {"name", command.typeName()});

    static const int informationChangedCommandType = QMetaType::fromName("InformationChangedCommand").id();
    static const int valuesChangedCommandType = QMetaType::fromName("ValuesChangedCommand").id();
    static const int valuesModifiedCommandType = QMetaType::fromName("ValuesModifiedCommand").id();
    static const int pixmapChangedCommandType = QMetaType::fromName("PixmapChangedCommand").id();
    static const int childrenChangedCommandType = QMetaType::fromName("ChildrenChangedCommand").id();
    static const int statePreviewImageChangedCommandType = QMetaType::fromName("StatePreviewImageChangedCommand").id();
    static const int componentCompletedCommandType = QMetaType::fromName("ComponentCompletedCommand").id();
    static const int tokenCommandType = QMetaType::fromName("TokenCommand").id();
    static const int debugOutputCommandType = QMetaType::fromName("DebugOutputCommand").id();
    static const int changeSelectionCommandType = QMetaType::fromName("ChangeSelectionCommand").id();
    static const int puppetToCreatorCommandType = QMetaType::fromName("PuppetToCreatorCommand").id();
    static const int SyncNanotraceCommandType = QMetaType::fromName("SyncNanotraceCommand").id();

    qCInfo(instanceViewBenchmark) << "dispatching command" << command.typeId() << command.typeName();
    if (command.typeId() == informationChangedCommandType) {
        nodeInstanceClient()->informationChanged(command.value<InformationChangedCommand>());
    } else if (command.typeId() == valuesChangedCommandType) {
        nodeInstanceClient()->valuesChanged(command.value<ValuesChangedCommand>());
    } else if (command.typeId() == valuesModifiedCommandType) {
        nodeInstanceClient()->valuesModified(command.value<ValuesModifiedCommand>());
    } else if (command.typeId() == pixmapChangedCommandType) {
        nodeInstanceClient()->pixmapChanged(command.value<PixmapChangedCommand>());
    } else if (command.typeId() == childrenChangedCommandType) {
        nodeInstanceClient()->childrenChanged(command.value<ChildrenChangedCommand>());
    } else if (command.typeId() == statePreviewImageChangedCommandType) {
        nodeInstanceClient()->statePreviewImagesChanged(command.value<StatePreviewImageChangedCommand>());
    } else if (command.typeId() == componentCompletedCommandType) {
        nodeInstanceClient()->componentCompleted(command.value<ComponentCompletedCommand>());
    } else if (command.typeId() == tokenCommandType) {
        nodeInstanceClient()->token(command.value<TokenCommand>());
    } else if (command.typeId() == debugOutputCommandType) {
        nodeInstanceClient()->debugOutput(command.value<DebugOutputCommand>());
    } else if (command.typeId() == changeSelectionCommandType) {
        nodeInstanceClient()->selectionChanged(command.value<ChangeSelectionCommand>());
    } else if (command.typeId() == puppetToCreatorCommandType) {
        nodeInstanceClient()->handlePuppetToCreatorCommand(command.value<PuppetToCreatorCommand>());
    } else if (command.typeId() == SyncNanotraceCommandType) {
        // ignore.
    } else {
        QTC_ASSERT(false, );
        Q_ASSERT(false);
    }

    qCInfo(instanceViewBenchmark) << "dispatching command"
                                  << "done" << command.typeId();
}

NodeInstanceClientInterface *NodeInstanceServerProxy::nodeInstanceClient() const
{
    NanotraceHR::Tracer tracer{"node instance server proxy node instance client", category()};

    return m_nodeInstanceView;
}

QString NodeInstanceServerProxy::qrcMappingString() const
{
    NanotraceHR::Tracer tracer{"node instance server proxy qrc mapping string", category()};

    if (m_nodeInstanceView && m_nodeInstanceView->model()) {
        RewriterView *rewriterView = m_nodeInstanceView->model()->rewriterView();
        if (rewriterView) {
            QString mappingString;

            using StringPair = QPair<QString, QString>;

            const QSet<QPair<QString, QString>> &pairs = rewriterView->qrcMapping();
            for (const StringPair &pair : pairs) {
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
    NanotraceHR::Tracer tracer{"node instance server proxy write command", category()};

#ifdef NANOTRACE_DESIGNSTUDIO_ENABLED
    if (command.typeId() == QMetaType::fromName("SyncNanotraceCommand").id()) {
        SyncNanotraceCommand cmd = command.value<SyncNanotraceCommand>();
        NANOTRACE_INSTANT_ARGS("Sync", "writeCommand",
            {"name", cmd.name().toStdString()},
            {"counter", int64_t(m_connectionManager.writeCounter())});

    } else {
        NANOTRACE_INSTANT_ARGS("Update", "writeCommand",
            {"name", command.typeName()},
            {"counter", int64_t(m_connectionManager.writeCounter())});
    }
#endif

    m_connectionManager.writeCommand(command);
}

void NodeInstanceServerProxy::createInstances(const CreateInstancesCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy create instances", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::changeFileUrl(const ChangeFileUrlCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy change file url", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::createScene(const CreateSceneCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy create scene", category()};

    qCInfo(instanceViewBenchmark) << Q_FUNC_INFO << m_benchmarkTimer.elapsed();
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::clearScene(const ClearSceneCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy clear scene", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::update3DViewState(const Update3dViewStateCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy update 3d view state", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::removeInstances(const RemoveInstancesCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy remove instances", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::changeSelection(const ChangeSelectionCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy change selection", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::removeProperties(const RemovePropertiesCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy remove properties", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::changePropertyBindings(const ChangeBindingsCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy change property bindings", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::changePropertyValues(const ChangeValuesCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy change property values", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::changeAuxiliaryValues(const ChangeAuxiliaryCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy change auxiliary values", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::reparentInstances(const ReparentInstancesCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy reparent instances", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::changeIds(const ChangeIdsCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy change ids", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::changeState(const ChangeStateCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy change state", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::completeComponent(const CompleteComponentCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy complete component", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::changeNodeSource(const ChangeNodeSourceCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy change node source", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::token(const TokenCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy token", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::removeSharedMemory(const RemoveSharedMemoryCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy remove shared memory", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::benchmark(const QString &message)
{
    qCInfo(instanceViewBenchmark) << message << m_benchmarkTimer.elapsed();
}

void NodeInstanceServerProxy::inputEvent(const InputEventCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy input event", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::view3DAction(const View3DActionCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy view 3d action", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::requestModelNodePreviewImage(const RequestModelNodePreviewImageCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy request model node preview image",
                               category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::changeLanguage(const ChangeLanguageCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy change language", category()};

    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceServerProxy::changePreviewImageSize(const ChangePreviewImageSizeCommand &command)
{
    NanotraceHR::Tracer tracer{"node instance server proxy change preview image size", category()};

    writeCommand(QVariant::fromValue(command));
}

} // namespace QmlDesigner
