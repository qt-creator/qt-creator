// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidrunner.h"

#include "androidconstants.h"
#include "androiddevice.h"
#include "androidrunnerworker.h"
#include "androidtr.h"
#include "androidutils.h"

#include <debugger/debuggerrunconfigurationaspect.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/url.h>
#include <utils/utilsicons.h>

#include <QHostAddress>
#include <QLoggingCategory>
#include <QTcpServer>

namespace {
static Q_LOGGING_CATEGORY(androidRunnerLog, "qtc.android.run.androidrunner", QtWarningMsg)
}

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace Android::Internal {

Group androidKicker(const StoredBarrier &barrier, RunControl *runControl)
{
    BuildConfiguration *bc = runControl->buildConfiguration();
    QTC_ASSERT(bc, return {});
    QString deviceSerialNumber;
    int apiLevel = -1;

    const Storage<RunnerInterface> glueStorage;

    std::optional<ExecutableItem> avdRecipe;

    if (!projectExplorerSettings().deployBeforeRun() && runControl->project()) {
        qCDebug(androidRunnerLog) << "Run without deployment";

        const IDevice::ConstPtr device = RunDeviceKitAspect::device(runControl->kit());
        AndroidDeviceInfo info = AndroidDevice::androidDeviceInfoFromDevice(device);
        setDeviceSerialNumber(bc, info.serialNumber);
        deviceSerialNumber = info.serialNumber;
        apiLevel = info.sdk;
        qCDebug(androidRunnerLog) << "Android Device Info changed" << deviceSerialNumber
                                  << apiLevel;

        if (!info.avdName.isEmpty()) {
            const Storage<QString> serialNumberStorage;

            avdRecipe = Group {
                serialNumberStorage,
                startAvdRecipe(info.avdName, serialNumberStorage)
            }.withCancel([glueStorage] {
                return std::make_pair(glueStorage.activeStorage(), &RunnerInterface::canceled);
            });
        }
    } else {
        deviceSerialNumber = Internal::deviceSerialNumber(bc);
        apiLevel = Internal::deviceApiLevel(bc);
    }

    const auto onSetup = [runControl, glueStorage, deviceSerialNumber, apiLevel, barrier] {
        RunnerInterface *glue = glueStorage.activeStorage();
        glue->setRunControl(runControl);
        glue->setDeviceSerialNumber(deviceSerialNumber);
        glue->setApiLevel(apiLevel);

        auto aspect = runControl->aspectData<Debugger::DebuggerRunConfigurationAspect>();
        const Id runMode = runControl->runMode();
        const bool debuggingMode = runMode == ProjectExplorer::Constants::DEBUG_RUN_MODE;
        QmlDebugServicesPreset services = NoQmlDebugServices;
        if (debuggingMode && aspect->useQmlDebugger)
            services = QmlDebuggerServices;
        else if (runMode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE)
            services = QmlProfilerServices;
        else if (runMode == ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE)
            services = QmlPreviewServices;
        glue->setQmlDebugServicesPreset(services);

        if (services != NoQmlDebugServices) {
            qCDebug(androidRunnerLog) << "QML debugging enabled";
            QTcpServer server;
            const bool isListening = server.listen(QHostAddress::LocalHost);
            QTC_ASSERT(isListening,
                       qDebug() << Tr::tr("No free ports available on host for QML debugging."));
            QUrl qmlChannel;
            qmlChannel.setScheme(Utils::urlTcpScheme());
            qmlChannel.setHost(server.serverAddress().toString());
            qmlChannel.setPort(server.serverPort());
            runControl->setQmlChannel(qmlChannel);
            qCDebug(androidRunnerLog) << "QML server:" << qmlChannel.toDisplayString();
        }

        auto iface = runStorage().activeStorage();
        QObject::connect(iface, &RunInterface::canceled, glue, &RunnerInterface::cancel);
        QObject::connect(glue, &RunnerInterface::started, barrier.activeStorage(), &Barrier::advance,
                         Qt::QueuedConnection);
        QObject::connect(glue, &RunnerInterface::finished, runControl, [runControl](const QString &errorString) {
            runControl->postMessage(errorString, Utils::NormalMessageFormat);
            if (runControl->isRunning())
                runControl->initiateStop();
        });
    };

    return {
        glueStorage,
        onGroupSetup(onSetup),
        avdRecipe ? *avdRecipe : nullItem,
        runnerRecipe(glueStorage)
    };
}

Group androidRecipe(RunControl *runControl)
{
    const auto kicker = [runControl](const StoredBarrier &barrier) {
        return androidKicker(barrier, runControl);
    };
    return When (kicker) >> Do {
        Sync([runControl] { runControl->reportStarted(); })
    };
}

class AndroidRunWorkerFactory final : public RunWorkerFactory
{
public:
    AndroidRunWorkerFactory()
    {
        setId("AndroidRunWorkerFactory");
        setRecipeProducer(androidRecipe);
        addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
        addSupportedRunConfig(Constants::ANDROID_RUNCONFIG_ID);
    }
};

void setupAndroidRunWorker()
{
    static AndroidRunWorkerFactory theAndroidRunWorkerFactory;
}

} // namespace Android::Internal
