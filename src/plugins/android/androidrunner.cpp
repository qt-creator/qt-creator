/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidrunner.h"

#include "androidconstants.h"
#include "androiddeployqtstep.h"
#include "androidconfigurations.h"
#include "androidrunconfiguration.h"
#include "androidmanager.h"
#include "androidavdmanager.h"
#include "androidrunnerworker.h"

#include <QHostAddress>
#include <coreplugin/messagemanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <utils/url.h>

#include <QLoggingCategory>

namespace {
Q_LOGGING_CATEGORY(androidRunnerLog, "qtc.android.run.androidrunner", QtWarningMsg)
}

using namespace ProjectExplorer;
using namespace Utils;

/*
    This uses explicit handshakes between the application and the
    gdbserver start and the host side by using the gdbserver socket.

    For the handshake there are two mechanisms. Only the first method works
    on Android 5.x devices and is chosen as default option. The second
    method can be enabled by setting the QTC_ANDROID_USE_FILE_HANDSHAKE
    environment variable before starting Qt Creator.

    1.) This method uses a TCP server on the Android device which starts
    listening for incoming connections. The socket is forwarded by adb
    and creator connects to it. This is the only method that works
    on Android 5.x devices.

    2.) This method uses two files ("ping" file in the application dir,
    "pong" file in /data/local/tmp/qt).

    The sequence is as follows:

     host: adb forward debugsocket :5039

     host: adb shell rm pong file
     host: adb shell am start
     host: loop until ping file appears

         app start up: launch gdbserver --multi +debug-socket
         app start up: loop until debug socket appear

             gdbserver: normal start up including opening debug-socket,
                        not yet attached to any process

         app start up: 1.) set up ping connection or 2.) touch ping file
         app start up: 1.) accept() or 2.) loop until pong file appears

     host: start gdb
     host: gdb: set up binary, breakpoints, path etc
     host: gdb: target extended-remote :5039

             gdbserver: accepts connection from gdb

     host: gdb: attach <application-pid>

             gdbserver: attaches to the application
                        and stops it

         app start up: stopped now (it is still waiting for
                       the pong anyway)

     host: gdb: continue

             gdbserver: resumes application

         app start up: resumed (still waiting for the pong)

     host: 1) write "ok" to ping pong connection or 2.) write pong file

         app start up: java code continues now, the process
                       is already fully under control
                       of gdbserver. Breakpoints are set etc,
                       we are before main.
         app start up: native code launches

*/

namespace Android {
namespace Internal {

AndroidRunner::AndroidRunner(RunControl *runControl, const QString &intentName)
    : RunWorker(runControl), m_target(runControl->runConfiguration()->target())
{
    setId("AndroidRunner");
    static const int metaTypes[] = {
        qRegisterMetaType<QVector<QStringList> >("QVector<QStringList>"),
        qRegisterMetaType<Utils::Port>("Utils::Port"),
        qRegisterMetaType<AndroidDeviceInfo>("Android::AndroidDeviceInfo")
    };
    Q_UNUSED(metaTypes);

    m_checkAVDTimer.setInterval(2000);
    connect(&m_checkAVDTimer, &QTimer::timeout, this, &AndroidRunner::checkAVD);

    QString intent = intentName.isEmpty() ? AndroidManager::intentName(m_target) : intentName;
    m_packageName = intent.left(intent.indexOf('/'));
    qCDebug(androidRunnerLog) << "Intent name:" << intent << "Package name" << m_packageName;

    const int apiLevel = AndroidManager::deviceApiLevel(m_target);
    qCDebug(androidRunnerLog) << "Device API:" << apiLevel;

    m_worker.reset(new AndroidRunnerWorker(this, m_packageName));
    m_worker->setIntentName(intent);
    m_worker->setIsPreNougat(apiLevel <= 23);

    m_worker->moveToThread(&m_thread);

    connect(this, &AndroidRunner::asyncStart, m_worker.data(), &AndroidRunnerWorker::asyncStart);
    connect(this, &AndroidRunner::asyncStop, m_worker.data(), &AndroidRunnerWorker::asyncStop);
    connect(this, &AndroidRunner::androidDeviceInfoChanged,
            m_worker.data(), &AndroidRunnerWorker::setAndroidDeviceInfo);
    connect(m_worker.data(), &AndroidRunnerWorker::remoteProcessStarted,
            this, &AndroidRunner::handleRemoteProcessStarted);
    connect(m_worker.data(), &AndroidRunnerWorker::remoteProcessFinished,
            this, &AndroidRunner::handleRemoteProcessFinished);
    connect(m_worker.data(), &AndroidRunnerWorker::remoteOutput,
            this, &AndroidRunner::remoteOutput);
    connect(m_worker.data(), &AndroidRunnerWorker::remoteErrorOutput,
            this, &AndroidRunner::remoteErrorOutput);

    connect(&m_outputParser, &QmlDebug::QmlOutputParser::waitingForConnectionOnPort,
            this, &AndroidRunner::qmlServerPortReady);

    m_thread.start();
}

AndroidRunner::~AndroidRunner()
{
    m_thread.quit();
    m_thread.wait();
}

void AndroidRunner::start()
{
    if (!ProjectExplorerPlugin::projectExplorerSettings().deployBeforeRun) {
        qCDebug(androidRunnerLog) << "Run without deployment";
       launchAVD();
       if (!m_launchedAVDName.isEmpty()) {
           m_checkAVDTimer.start();
           return;
       }
    }

    emit asyncStart();
}

void AndroidRunner::stop()
{
    if (m_checkAVDTimer.isActive()) {
        m_checkAVDTimer.stop();
        appendMessage("\n\n" + tr("\"%1\" terminated.").arg(m_packageName),
                      Utils::DebugFormat);
        return;
    }

    emit asyncStop();
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

void AndroidRunner::remoteOutput(const QString &output)
{
    Core::MessageManager::write("LOGCAT: " + output, Core::MessageManager::Silent);
    appendMessage(output, Utils::StdOutFormatSameLine);
    m_outputParser.processOutput(output);
}

void AndroidRunner::remoteErrorOutput(const QString &output)
{
    Core::MessageManager::write("LOGCAT: " + output, Core::MessageManager::Silent);
    appendMessage(output, Utils::StdErrFormatSameLine);
    m_outputParser.processOutput(output);
}

void AndroidRunner::handleRemoteProcessStarted(Utils::Port gdbServerPort,
                                               const QUrl &qmlServer, int pid)
{
    m_pid = ProcessHandle(pid);
    m_gdbServerPort = gdbServerPort;
    m_qmlServer = qmlServer;
    reportStarted();
}

void AndroidRunner::handleRemoteProcessFinished(const QString &errString)
{
    appendMessage(errString, Utils::DebugFormat);
    if (runControl()->isRunning())
        runControl()->initiateStop();
    reportStopped();
}

void AndroidRunner::launchAVD()
{
    if (!m_target || !m_target->project())
        return;

    int deviceAPILevel = AndroidManager::minimumSDK(m_target);
    QString targetArch = AndroidManager::targetArch(m_target);

    // Get AVD info.
    AndroidDeviceInfo info = AndroidConfigurations::showDeviceDialog(
                m_target->project(), deviceAPILevel, targetArch);
    AndroidManager::setDeviceSerialNumber(m_target, info.serialNumber);
    emit androidDeviceInfoChanged(info);
    if (info.isValid()) {
        AndroidAvdManager avdManager;
        if (!info.avdname.isEmpty() && avdManager.findAvd(info.avdname).isEmpty()) {
            bool launched = avdManager.startAvdAsync(info.avdname);
            m_launchedAVDName = launched ? info.avdname:"";
        } else {
            m_launchedAVDName.clear();
        }
    }
}

void AndroidRunner::checkAVD()
{
    const AndroidConfig &config = AndroidConfigurations::currentConfig();
    AndroidAvdManager avdManager(config);
    QString serialNumber = avdManager.findAvd(m_launchedAVDName);
    if (!serialNumber.isEmpty())
        return; // try again on next timer hit

    if (avdManager.isAvdBooted(serialNumber)) {
        m_checkAVDTimer.stop();
        AndroidManager::setDeviceSerialNumber(m_target, serialNumber);
        emit asyncStart();
    } else if (!config.isConnected(serialNumber)) {
        // device was disconnected
        m_checkAVDTimer.stop();
    }
}

} // namespace Internal
} // namespace Android
