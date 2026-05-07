// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dockerapi.h"
#include "dockerconstants.h"
#include "dockerdevice.h"
#include "dockersettings.h"
#ifdef WITH_TESTS
#include "dockerdebuggertest.h"
#endif

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/qmldebugcommandlinearguments.h>
#include <projectexplorer/runcontrol.h>

#include <extensionsystem/iplugin.h>

#include <utils/fsengine/fsengine.h>

#include <QtTaskTree/QBarrier>

using namespace Core;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Docker::Internal {

class DockerQmlToolingWorkerFactory final : public RunWorkerFactory
{
public:
    DockerQmlToolingWorkerFactory()
    {
        setId("DockerQmlToolingWorkerFactory");
        setRecipeProducer([](RunControl *runControl) {
            runControl->requestQmlChannel();

            const auto modifier = [runControl](Process &process) {
                const QmlDebugServicesPreset services =
                    servicesForRunMode(runControl->runMode());
                CommandLine cmd = runControl->commandLine();
                QUrl bindServer = runControl->qmlChannel();
                const QString bindHost = runControl->device()->qmlDebugServerBindHost();
                if (!bindHost.isEmpty())
                    bindServer.setHost(bindHost);
                cmd.addArg(qmlDebugTcpArguments(services, bindServer));
                process.setCommand(cmd);
            };
            const ProcessTask processTask(
                runControl->processTaskWithModifier(modifier, {.setupCanceler = false}));
            return Group {
                When (processTask, &Process::started, WorkflowPolicy::StopOnError) >> Do {
                    runControl->createRecipe(runnerIdForRunMode(runControl->runMode()))
                }
            };
        });
        addSupportedRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
        addSupportedRunMode(ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE);
        addSupportedDeviceType(Constants::DOCKER_DEVICE_TYPE);
        addSupportedDeviceType(Podman::Constants::PODMAN_DEVICE_TYPE);
    }
};

void setupDockerRunAndDebugSupport()
{
    static DockerQmlToolingWorkerFactory qmlToolingWorkerFactory;
}

class DockerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Docker.json")

public:
    DockerPlugin()
    {
        FSEngine::registerDeviceScheme(Constants::DOCKER_DEVICE_SCHEME);
        FSEngine::registerDeviceScheme(Podman::Constants::PODMAN_DEVICE_SCHEME);
    }

private:
    ~DockerPlugin() final
    {
        FSEngine::unregisterDeviceScheme(Constants::DOCKER_DEVICE_SCHEME);
        FSEngine::unregisterDeviceScheme(Podman::Constants::PODMAN_DEVICE_SCHEME);
        m_dockerDeviceFactory->shutdownExistingDevices();
        m_podmanDeviceFactory->shutdownExistingDevices();
    }

    void initialize() final
    {
        m_dockerDeviceFactory = std::make_unique<DockerDeviceFactory>(&dockerSettings());
        m_podmanDeviceFactory = std::make_unique<DockerDeviceFactory>(&podmanSettings());
        m_dockerApi = std::make_unique<DockerApi>(&dockerSettings());
        m_podmanApi = std::make_unique<DockerApi>(&podmanSettings());
        setupDockerRunAndDebugSupport();
#ifdef WITH_TESTS
        addTestCreator(createDockerQmlChannelTest);
        addTestCreator(createDockerPortsGatheringTest);
#endif
    }

    std::unique_ptr<DockerDeviceFactory> m_dockerDeviceFactory;
    std::unique_ptr<DockerDeviceFactory> m_podmanDeviceFactory;
    std::unique_ptr<DockerApi> m_dockerApi;
    std::unique_ptr<DockerApi> m_podmanApi;
};

} // Docker::Internal

#include "dockerplugin.moc"
