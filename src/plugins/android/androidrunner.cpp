// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidrunner.h"

#include "androidconstants.h"
#include "androiddevice.h"
#include "androidrunnerworker.h"
#include "androidutils.h"

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

namespace {
static Q_LOGGING_CATEGORY(androidRunnerLog, "qtc.android.run.androidrunner", QtWarningMsg)
}

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace Android::Internal {

Group androidRecipe(RunControl *runControl)
{
    BuildConfiguration *bc = runControl->buildConfiguration();
    QTC_ASSERT(bc, return {});
    QString deviceSerialNumber;
    int apiLevel = -1;

    const Storage<RunnerInterface> glueStorage;

    std::optional<ExecutableItem> avdRecipe;

    if (!projectExplorerSettings().deployBeforeRun && runControl->project()) {
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

    const auto onSetup = [runControl, glueStorage, deviceSerialNumber, apiLevel] {
        RunnerInterface *glue = glueStorage.activeStorage();
        glue->setRunControl(runControl);
        glue->setDeviceSerialNumber(deviceSerialNumber);
        glue->setApiLevel(apiLevel);

        auto iface = runStorage().activeStorage();
        QObject::connect(iface, &RunInterface::canceled, glue, &RunnerInterface::cancel);

        QObject::connect(glue, &RunnerInterface::started, runControl, [runControl, iface](qint64 pid, const QString &packageDir) {
            runControl->setAttachPid(ProcessHandle(pid));
            runControl->setDebugChannel(QString("unix-abstract-connect://%1/debug-socket").arg(packageDir));
            emit iface->started();
        });
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

class AndroidRunWorkerFactory final : public RunWorkerFactory
{
public:
    AndroidRunWorkerFactory()
    {
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
