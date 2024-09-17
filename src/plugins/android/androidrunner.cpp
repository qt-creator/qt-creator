// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidrunner.h"

#include "androidavdmanager.h"
#include "androiddevice.h"
#include "androidmanager.h"
#include "androidrunnerworker.h"
#include "androidtr.h"

#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitaspect.h>
#include <utils/url.h>

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
    setId("AndroidRunner");
    static const int metaTypes[] = {
        qRegisterMetaType<QList<QStringList>>("QList<QStringList>"),
        qRegisterMetaType<Utils::Port>("Utils::Port"),
        qRegisterMetaType<AndroidDeviceInfo>("Android::AndroidDeviceInfo")
    };
    Q_UNUSED(metaTypes)

    connect(&m_outputParser, &QmlDebug::QmlOutputParser::waitingForConnectionOnPort,
            this, &AndroidRunner::qmlServerPortReady);
}

void AndroidRunner::start()
{
    auto target = runControl()->target();
    QTC_ASSERT(target, return);
    QString deviceSerialNumber;
    int apiLevel = -1;

    const Storage<RunnerInterface> glueStorage;

    std::optional<ExecutableItem> avdRecipe;

    if (!projectExplorerSettings().deployBeforeRun && target->project()) {
        qCDebug(androidRunnerLog) << "Run without deployment";

        const IDevice::ConstPtr device = DeviceKitAspect::device(target->kit());
        AndroidDeviceInfo info = AndroidDevice::androidDeviceInfoFromIDevice(device.get());
        AndroidManager::setDeviceSerialNumber(target, info.serialNumber);
        deviceSerialNumber = info.serialNumber;
        apiLevel = info.sdk;
        qCDebug(androidRunnerLog) << "Android Device Info changed" << deviceSerialNumber
                                  << apiLevel;

        if (!info.avdName.isEmpty()) {
            const Storage<QString> serialNumberStorage;

            avdRecipe = Group {
                serialNumberStorage,
                AndroidAvdManager::startAvdRecipe(info.avdName, serialNumberStorage)
            }.withCancel([glueStorage] {
                return std::make_pair(glueStorage.activeStorage(), &RunnerInterface::canceled);
            });
        }
    } else {
        deviceSerialNumber = AndroidManager::deviceSerialNumber(target);
        apiLevel = AndroidManager::deviceApiLevel(target);
    }

    const auto onSetup = [this, glueStorage, deviceSerialNumber, apiLevel] {
        RunnerInterface *glue = glueStorage.activeStorage();
        glue->setRunControl(runControl());
        glue->setDeviceSerialNumber(deviceSerialNumber);
        glue->setApiLevel(apiLevel);

        connect(this, &AndroidRunner::canceled, glue, &RunnerInterface::cancel);

        connect(glue, &RunnerInterface::started, this, &AndroidRunner::remoteStarted);
        connect(glue, &RunnerInterface::finished, this, &AndroidRunner::remoteFinished);
        connect(glue, &RunnerInterface::stdOut, this, &AndroidRunner::remoteStdOut);
        connect(glue, &RunnerInterface::stdErr, this, &AndroidRunner::remoteStdErr);
    };

    const Group recipe {
        glueStorage,
        onGroupSetup(onSetup),
        avdRecipe ? *avdRecipe : nullItem,
        runnerRecipe(glueStorage)
    };
    m_taskTreeRunner.start(recipe);
    m_packageName = AndroidManager::packageName(target);
}

void AndroidRunner::stop()
{
    if (!m_taskTreeRunner.isRunning())
        return;

    emit canceled();
    appendMessage("\n\n" + Tr::tr("\"%1\" terminated.").arg(m_packageName),
                  Utils::NormalMessageFormat);
}

void AndroidRunner::qmlServerPortReady(Port port)
{
    // FIXME: Note that the passed is nonsense, as the port is on the
    // device side. It only happens to work since we redirect
    // host port n to target port n via adb.
    QUrl serverUrl;
    serverUrl.setHost(QHostAddress(QHostAddress::LocalHost).toString());
    serverUrl.setPort(port.number());
    serverUrl.setScheme(urlTcpScheme());
    qCDebug(androidRunnerLog) << "Qml Server port ready"<< serverUrl;
    emit qmlServerReady(serverUrl);
}

void AndroidRunner::remoteStarted(const Port &debugServerPort, const QUrl &qmlServer, qint64 pid)
{
    m_pid = ProcessHandle(pid);
    m_debugServerPort = debugServerPort;
    m_qmlServer = qmlServer;
    reportStarted();
}

void AndroidRunner::remoteFinished(const QString &errString)
{
    appendMessage(errString, Utils::NormalMessageFormat);
    if (runControl()->isRunning())
        runControl()->initiateStop();
    reportStopped();
}

void AndroidRunner::remoteStdOut(const QString &output)
{
    appendMessage(output, Utils::StdOutFormat);
    m_outputParser.processOutput(output);
}

void AndroidRunner::remoteStdErr(const QString &output)
{
    appendMessage(output, Utils::StdErrFormat);
    m_outputParser.processOutput(output);
}

} // namespace Android::Internal
