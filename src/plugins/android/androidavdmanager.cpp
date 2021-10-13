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

#include "androidavdmanager.h"

#include "avdmanageroutputparser.h"

#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/runextensions.h>

#include <QApplication>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QSettings>

#include <chrono>
#include <functional>

using namespace Utils;

namespace {
static Q_LOGGING_CATEGORY(avdManagerLog, "qtc.android.avdManager", QtWarningMsg)
}

namespace Android {
namespace Internal {

using namespace std;

const int avdCreateTimeoutMs = 30000;

/*!
    Runs the \c avdmanager tool specific to configuration \a config with arguments \a args. Returns
    \c true if the command is successfully executed. Output is copied into \a output. The function
    blocks the calling thread.
 */
bool AndroidAvdManager::avdManagerCommand(const AndroidConfig &config, const QStringList &args, QString *output)
{
    CommandLine cmd(config.avdManagerToolPath(), args);
    QtcProcess proc;
    Environment env = AndroidConfigurations::toolsEnvironment(config);
    proc.setEnvironment(env);
    qCDebug(avdManagerLog) << "Running AVD Manager command:" << cmd.toUserOutput();
    proc.setCommand(cmd);
    proc.runBlocking();
    if (proc.result() == ProcessResult::FinishedWithSuccess) {
        if (output)
            *output = proc.allOutput();
        return true;
    }
    return false;
}

static bool checkForTimeout(const chrono::steady_clock::time_point &start,
                            int msecs = 3000)
{
    bool timedOut = false;
    auto end = chrono::steady_clock::now();
    if (chrono::duration_cast<chrono::milliseconds>(end-start).count() > msecs)
        timedOut = true;
    return timedOut;
}

static CreateAvdInfo createAvdCommand(const AndroidConfig &config, const CreateAvdInfo &info)
{
    CreateAvdInfo result = info;

    if (!result.isValid()) {
        qCDebug(avdManagerLog) << "AVD Create failed. Invalid CreateAvdInfo" << result.name
                               << result.systemImage->displayText() << result.systemImage->apiLevel();
        result.error = QApplication::translate("AndroidAvdManager",
                                               "Cannot create AVD. Invalid input.");
        return result;
    }

    QStringList arguments({"create", "avd", "-n", result.name});

    arguments << "-k" << result.systemImage->sdkStylePath();

    if (result.sdcardSize > 0)
        arguments << "-c" << QString::fromLatin1("%1M").arg(result.sdcardSize);

    if (!result.deviceDefinition.isEmpty() && result.deviceDefinition != "Custom")
        arguments << "-d" << QString::fromLatin1("%1").arg(result.deviceDefinition);

    if (result.overwrite)
        arguments << "-f";

    const FilePath avdManagerTool = config.avdManagerToolPath();
    qCDebug(avdManagerLog)
            << "Running AVD Manager command:" << CommandLine(avdManagerTool, arguments).toUserOutput();
    QtcProcess proc;
    proc.setProcessMode(ProcessMode::Writer);
    proc.setEnvironment(AndroidConfigurations::toolsEnvironment(config));
    proc.setCommand({avdManagerTool, arguments});
    proc.start();
    if (!proc.waitForStarted()) {
        result.error = QApplication::translate("AndroidAvdManager",
                                               "Could not start process \"%1 %2\"")
                .arg(avdManagerTool.toString(), arguments.join(' '));
        return result;
    }
    QTC_CHECK(proc.isRunning());
    proc.write(QByteArray("yes\n")); // yes to "Do you wish to create a custom hardware profile"

    auto start = chrono::steady_clock::now();
    QString errorOutput;
    QByteArray question;
    while (errorOutput.isEmpty()) {
        proc.waitForReadyRead(500);
        question += proc.readAllStandardOutput();
        if (question.endsWith(QByteArray("]:"))) {
            // truncate to last line
            int index = question.lastIndexOf(QByteArray("\n"));
            if (index != -1)
                question = question.mid(index);
            if (question.contains("hw.gpu.enabled"))
                proc.write(QByteArray("yes\n"));
            else
                proc.write(QByteArray("\n"));
            question.clear();
        }
        // The exit code is always 0, so we need to check stderr
        // For now assume that any output at all indicates a error
        errorOutput = QString::fromLocal8Bit(proc.readAllStandardError());
        if (!proc.isRunning())
            break;

        // For a sane input and command, process should finish before timeout.
        if (checkForTimeout(start, avdCreateTimeoutMs)) {
            result.error = QApplication::translate("AndroidAvdManager",
                                                   "Cannot create AVD. Command timed out.");
        }
    }

    // Kill the running process.
    if (proc.state() != QProcess::NotRunning) {
        proc.terminate();
        if (!proc.waitForFinished(3000))
            proc.kill();
    }

    QTC_CHECK(proc.state() == QProcess::NotRunning);
    result.error = errorOutput;
    return result;
}

static void avdProcessFinished(int exitCode, QtcProcess *p)
{
    QTC_ASSERT(p, return);
    if (exitCode) {
        QString title = QCoreApplication::translate("Android::Internal::AndroidAvdManager",
                                                    "AVD Start Error");
        QMessageBox::critical(Core::ICore::dialogParent(), title,
                              QString::fromLatin1(p->readAllStandardOutput()));
    }
    p->deleteLater();
}

AndroidAvdManager::AndroidAvdManager(const AndroidConfig &config)
    : m_config(config)
{

}

AndroidAvdManager::~AndroidAvdManager() = default;

QFuture<CreateAvdInfo> AndroidAvdManager::createAvd(CreateAvdInfo info) const
{
    return runAsync(&createAvdCommand, m_config, info);
}

bool AndroidAvdManager::removeAvd(const QString &name) const
{
    const CommandLine command(m_config.avdManagerToolPath(), {"delete", "avd", "-n", name});
    qCDebug(avdManagerLog) << "Running command (removeAvd):" << command.toUserOutput();
    QtcProcess proc;
    proc.setTimeoutS(5);
    proc.setEnvironment(AndroidConfigurations::toolsEnvironment(m_config));
    proc.setCommand(command);
    proc.runBlocking();
    return proc.result() == ProcessResult::FinishedWithSuccess;
}

static void avdConfigEditManufacturerTag(const QString &avdPathStr, bool recoverMode = false)
{
    const FilePath avdPath = FilePath::fromString(avdPathStr);
    if (avdPath.exists()) {
        const QString configFilePath = avdPath.pathAppended("config.ini").toString();
        QFile configFile(configFilePath);
        if (configFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
            QString newContent;
            QTextStream textStream(&configFile);
            while (!textStream.atEnd()) {
                QString line = textStream.readLine();
                if (!line.contains("hw.device.manufacturer"))
                    newContent.append(line + "\n");
                else if (recoverMode)
                    newContent.append(line.replace("#", "") + "\n");
                else
                    newContent.append("#" + line + "\n");
            }
            configFile.resize(0);
            textStream << newContent;
            configFile.close();
        }
    }
}

static AndroidDeviceInfoList listVirtualDevices(const AndroidConfig &config)
{
    QString output;
    AndroidDeviceInfoList avdList;
    /*
        Currenly avdmanager tool fails to parse some AVDs because the correct
        device definitions at devices.xml does not have some of the newest devices.
        Particularly, failing because of tag "hw.device.manufacturer", thus removing
        it would make paring successful. However, it has to be returned afterwards,
        otherwise, Android Studio would give an error during parsing also. So this fix
        aim to keep support for Qt Creator and Android Studio.
    */
    QStringList allAvdErrorPaths;
    QStringList avdErrorPaths;

    do {
        if (!AndroidAvdManager::avdManagerCommand(config, {"list", "avd"}, &output)) {
            qCDebug(avdManagerLog)
                << "Avd list command failed" << output << config.sdkToolsVersion();
            return {};
        }

        avdErrorPaths.clear();
        avdList = parseAvdList(output, &avdErrorPaths);
        allAvdErrorPaths << avdErrorPaths;
        for (const QString &avdPathStr : qAsConst(avdErrorPaths))
            avdConfigEditManufacturerTag(avdPathStr); // comment out manufacturer tag
    } while (!avdErrorPaths.isEmpty());               // try again

    for (const QString &avdPathStr : qAsConst(allAvdErrorPaths))
        avdConfigEditManufacturerTag(avdPathStr, true); // re-add manufacturer tag

    return avdList;
}

QFuture<AndroidDeviceInfoList> AndroidAvdManager::avdList() const
{
    return runAsync(listVirtualDevices, m_config);
}

QString AndroidAvdManager::startAvd(const QString &name) const
{
    if (!findAvd(name).isEmpty() || startAvdAsync(name))
        return waitForAvd(name);
    return QString();
}

bool AndroidAvdManager::startAvdAsync(const QString &avdName) const
{
    QFileInfo info(m_config.emulatorToolPath().toString());
    if (!info.exists()) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              tr("Emulator Tool Is Missing"),
                              tr("Install the missing emulator tool (%1) to the"
                                 " installed Android SDK.")
                              .arg(m_config.emulatorToolPath().toString()));
        return false;
    }
    auto avdProcess = new QtcProcess();
    avdProcess->setProcessChannelMode(QProcess::MergedChannels);
    QObject::connect(avdProcess, &QtcProcess::finished, avdProcess,
                     [avdProcess] { avdProcessFinished(avdProcess->exitCode(), avdProcess); });

    // start the emulator
    QStringList arguments;
    if (AndroidConfigurations::force32bitEmulator())
        arguments << "-force-32bit";

    arguments << m_config.emulatorArgs() << "-avd" << avdName;
    qCDebug(avdManagerLog) << "Running command (startAvdAsync):"
                           << CommandLine(m_config.emulatorToolPath(), arguments).toUserOutput();
    avdProcess->setCommand({m_config.emulatorToolPath(), arguments});
    avdProcess->start();
    if (!avdProcess->waitForStarted(-1)) {
        delete avdProcess;
        return false;
    }
    return true;
}

QString AndroidAvdManager::findAvd(const QString &avdName) const
{
    QVector<AndroidDeviceInfo> devices = m_config.connectedDevices();
    foreach (AndroidDeviceInfo device, devices) {
        if (device.type != ProjectExplorer::IDevice::Emulator)
            continue;
        if (device.avdName == avdName)
            return device.serialNumber;
    }
    return QString();
}

QString AndroidAvdManager::waitForAvd(const QString &avdName,
                                      const std::function<bool()> &cancelChecker) const
{
    // we cannot use adb -e wait-for-device, since that doesn't work if a emulator is already running
    // 60 rounds of 2s sleeping, two minutes for the avd to start
    QString serialNumber;
    for (int i = 0; i < 60; ++i) {
        if (cancelChecker && cancelChecker())
            return QString();
        serialNumber = findAvd(avdName);
        if (!serialNumber.isEmpty())
            return waitForBooted(serialNumber, cancelChecker) ?  serialNumber : QString();
        QThread::sleep(2);
    }
    return QString();
}

bool AndroidAvdManager::isAvdBooted(const QString &device) const
{
    QStringList arguments = AndroidDeviceInfo::adbSelector(device);
    arguments << "shell" << "getprop" << "init.svc.bootanim";

    const CommandLine command({m_config.adbToolPath(), arguments});
    qCDebug(avdManagerLog) << "Running command (isAvdBooted):" << command.toUserOutput();
    QtcProcess adbProc;
    adbProc.setTimeoutS(10);
    adbProc.setCommand(command);
    adbProc.runBlocking();
    if (adbProc.result() != ProcessResult::FinishedWithSuccess)
        return false;
    QString value = adbProc.allOutput().trimmed();
    return value == "stopped";
}

bool AndroidAvdManager::waitForBooted(const QString &serialNumber,
                                      const std::function<bool()> &cancelChecker) const
{
    // found a serial number, now wait until it's done booting...
    for (int i = 0; i < 60; ++i) {
        if (cancelChecker && cancelChecker())
            return false;
        if (isAvdBooted(serialNumber)) {
            return true;
        } else {
            QThread::sleep(2);
            if (!m_config.isConnected(serialNumber)) // device was disconnected
                return false;
        }
    }
    return false;
}

} // namespace Internal
} // namespace Android
