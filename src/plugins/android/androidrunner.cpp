// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidrunner.h"

#include "androidconstants.h"
#include "androiddevice.h"
#include "androidrunnerworker.h"
#include "androidutils.h"

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

AndroidRunner::AndroidRunner(RunControl *runControl)
    : RunWorker(runControl)
{
    runControl->setIcon(Utils::Icons::RUN_SMALL_TOOLBAR);
    setId("AndroidRunner");
    static const int metaTypes[] = {
        qRegisterMetaType<QList<QStringList>>("QList<QStringList>"),
        qRegisterMetaType<Utils::Port>("Utils::Port"),
        qRegisterMetaType<AndroidDeviceInfo>("Android::AndroidDeviceInfo")
    };
    Q_UNUSED(metaTypes)
}

void AndroidRunner::start()
{
    auto target = runControl()->target();
    QTC_ASSERT(target, return);
    QString deviceSerialNumber;
    int apiLevel = -1;

    const Storage<RunnerInterface> glueStorage;

    std::optional<ExecutableItem> avdRecipe;

    if (!projectExplorerSettings().deployBeforeRun && runControl()->project()) {
        qCDebug(androidRunnerLog) << "Run without deployment";

        const IDevice::ConstPtr device = RunDeviceKitAspect::device(runControl()->kit());
        AndroidDeviceInfo info = AndroidDevice::androidDeviceInfoFromDevice(device);
        setDeviceSerialNumber(target, info.serialNumber);
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
        deviceSerialNumber = Internal::deviceSerialNumber(target);
        apiLevel = Internal::deviceApiLevel(target);
    }

    const auto onSetup = [this, glueStorage, deviceSerialNumber, apiLevel] {
        RunnerInterface *glue = glueStorage.activeStorage();
        glue->setRunControl(runControl());
        glue->setDeviceSerialNumber(deviceSerialNumber);
        glue->setApiLevel(apiLevel);

        connect(this, &AndroidRunner::canceled, glue, &RunnerInterface::cancel);

        connect(glue, &RunnerInterface::started, this, &AndroidRunner::remoteStarted);
        connect(glue, &RunnerInterface::finished, this, &AndroidRunner::remoteFinished);
    };

    const Group recipe {
        glueStorage,
        onGroupSetup(onSetup),
        avdRecipe ? *avdRecipe : nullItem,
        runnerRecipe(glueStorage)
    };
    m_taskTreeRunner.start(recipe);
}

void AndroidRunner::stop()
{
    if (!m_taskTreeRunner.isRunning())
        return;

    emit canceled();
}

void AndroidRunner::remoteStarted(qint64 pid)
{
    m_pid = ProcessHandle(pid);
    reportStarted();
}

void AndroidRunner::remoteFinished(const QString &errString)
{
    appendMessage(errString, Utils::NormalMessageFormat);
    if (runControl()->isRunning())
        runControl()->initiateStop();
    reportStopped();
}

class AndroidRunWorkerFactory final : public RunWorkerFactory
{
public:
    AndroidRunWorkerFactory()
    {
        setProduct<AndroidRunner>();
        addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
        addSupportedRunConfig(Constants::ANDROID_RUNCONFIG_ID);
    }
};

void setupAndroidRunWorker()
{
    static AndroidRunWorkerFactory theAndroidRunWorkerFactory;
}

} // namespace Android::Internal
