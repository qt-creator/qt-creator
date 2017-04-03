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

#include "androidtoolmanager.h"

#include "utils/algorithm.h"
#include "utils/qtcassert.h"
#include "utils/runextensions.h"
#include "utils/synchronousprocess.h"

#include <QApplication>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QSettings>

#include <chrono>

namespace {
Q_LOGGING_CATEGORY(avdManagerLog, "qtc.android.avdManager")
}

namespace Android {
namespace Internal {

using namespace std;

// Avd list keys to parse avd
const char avdInfoNameKey[] = "Name:";
const char avdInfoPathKey[] = "Path:";
const char avdInfoAbiKey[] = "abi.type";
const char avdInfoTargetKey[] = "target";
const char avdInfoErrorKey[] = "Error:";

const QVersionNumber avdManagerIntroVersion(25, 3 ,0);

const int avdCreateTimeoutMs = 30000;

/*!
    Runs the \c avdmanager tool specific to configuration \a config with arguments \a args. Returns
    \c true if the command is successfully executed. Output is copied into \a output. The function
    blocks the calling thread.
 */
static bool avdManagerCommand(const AndroidConfig config, const QStringList &args, QString *output)
{
    QString avdManagerToolPath = config.avdManagerToolPath().toString();
    Utils::SynchronousProcess proc;
    Utils::SynchronousProcessResponse response = proc.runBlocking(avdManagerToolPath, args);
    if (response.result == Utils::SynchronousProcessResponse::Finished) {
        if (output)
            *output = response.allOutput();
        return true;
    }
    return false;
}

/*!
    Parses the \a line for a [spaces]key[spaces]value[spaces] pattern and returns
    \c true if the key is found, \c false otherwise. The value is copied into \a value.
 */
static bool valueForKey(QString key, const QString &line, QString *value = nullptr)
{
    auto trimmedInput = line.trimmed();
    if (trimmedInput.startsWith(key)) {
        if (value)
            *value = trimmedInput.section(key, 1, 1).trimmed();
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

static AndroidConfig::CreateAvdInfo createAvdCommand(const AndroidConfig config,
                                                     const AndroidConfig::CreateAvdInfo &info)
{
    AndroidConfig::CreateAvdInfo result = info;

    if (!result.isValid()) {
        qCDebug(avdManagerLog) << "AVD Create failed. Invalid CreateAvdInfo" << result.name
                               << result.target.name << result.target.apiLevel;
        result.error = QApplication::translate("AndroidAvdManager",
                                               "Cannot create AVD. Invalid input.");
        return result;
    }

    QStringList arguments({"create", "avd", "-k", result.target.package, "-n", result.name});

    if (!result.abi.isEmpty()) {
        SystemImage image = Utils::findOrDefault(result.target.systemImages,
                                                 Utils::equal(&SystemImage::abiName, result.abi));
        if (image.isValid()) {
            arguments << "-k" << image.package;
        } else {
            qCDebug(avdManagerLog) << "AVD Create failed. Cannot find system image for the platform"
                                   << result.abi << result.target.name;
            result.error = QApplication::translate("AndroidAvdManager",
                                                   "Cannot create AVD. Cannot find system image for "
                                                   "the ABI %1(%2).").arg(result.abi).arg(result.target.name);
            return result;
        }

    } else {
        arguments << "-k" << result.target.package;
    }

    if (result.sdcardSize > 0)
        arguments << "-c" << QString::fromLatin1("%1M").arg(result.sdcardSize);

    QProcess proc;
    proc.start(config.avdManagerToolPath().toString(), arguments);
    if (!proc.waitForStarted()) {
        result.error = QApplication::translate("AndroidAvdManager",
                                               "Could not start process \"%1 %2\"")
                .arg(config.avdManagerToolPath().toString(), arguments.join(' '));
        return result;
    }
    QTC_CHECK(proc.state() == QProcess::Running);
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
        if (proc.state() != QProcess::Running)
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

/*!
    \class AvdManagerOutputParser
    \brief The AvdManagerOutputParser class is a helper class to parse the output of the avdmanager
    commands.
 */
class AvdManagerOutputParser
{
public:
    AndroidDeviceInfoList listVirtualDevices(const AndroidConfig &config);
    AndroidDeviceInfoList parseAvdList(const QString &output);

private:
    bool parseAvd(const QStringList &deviceInfo, AndroidDeviceInfo *avd);
};


AndroidAvdManager::AndroidAvdManager(const AndroidConfig &config):
    m_config(config),
    m_androidTool(new AndroidToolManager(m_config)),
    m_parser(new AvdManagerOutputParser)
{

}

AndroidAvdManager::~AndroidAvdManager()
{

}

bool AndroidAvdManager::avdManagerUiToolAvailable() const
{
    return m_config.sdkToolsVersion() < avdManagerIntroVersion;
}

void AndroidAvdManager::launchAvdManagerUiTool() const
{
    if (avdManagerUiToolAvailable()) {
        m_androidTool->launchAvdManager();
     } else {
        qCDebug(avdManagerLog) << "AVD Ui tool launch failed. UI tool not available"
                               << m_config.sdkToolsVersion();
    }
}

QFuture<AndroidConfig::CreateAvdInfo>
AndroidAvdManager::createAvd(AndroidConfig::CreateAvdInfo info) const
{
    if (m_config.sdkToolsVersion() < avdManagerIntroVersion)
        return m_androidTool->createAvd(info);

    return Utils::runAsync(&createAvdCommand, m_config, info);
}

bool AndroidAvdManager::removeAvd(const QString &name) const
{
    if (m_config.sdkToolsVersion() < avdManagerIntroVersion)
        return m_androidTool->removeAvd(name);

    Utils::SynchronousProcess proc;
    proc.setTimeoutS(5);
    Utils::SynchronousProcessResponse response
            = proc.runBlocking(m_config.avdManagerToolPath().toString(),
                               QStringList({"delete", "avd", "-n", name}));
    return response.result == Utils::SynchronousProcessResponse::Finished && response.exitCode == 0;
}

QFuture<AndroidDeviceInfoList> AndroidAvdManager::avdList() const
{
    if (m_config.sdkToolsVersion() < avdManagerIntroVersion)
        return m_androidTool->androidVirtualDevicesFuture();

    return Utils::runAsync(&AvdManagerOutputParser::listVirtualDevices, m_parser.get(), m_config);
}

QString AndroidAvdManager::startAvd(const QString &name) const
{
    if (!findAvd(name).isEmpty() || startAvdAsync(name))
        return waitForAvd(name);
    return QString();
}

bool AndroidAvdManager::startAvdAsync(const QString &avdName) const
{
    QProcess *avdProcess = new QProcess();
    QObject::connect(avdProcess, static_cast<void (QProcess::*)(int)>(&QProcess::finished),
                     avdProcess, &QObject::deleteLater);

    // start the emulator
    QStringList arguments;
    if (AndroidConfigurations::force32bitEmulator())
        arguments << "-force-32bit";

    arguments << "-partition-size" << QString::number(m_config.partitionSize())
              << "-avd" << avdName;
    avdProcess->start(m_config.emulatorToolPath().toString(), arguments);
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
        if (device.type != AndroidDeviceInfo::Emulator)
            continue;
        if (device.avdname == avdName)
            return device.serialNumber;
    }
    return QString();
}

QString AndroidAvdManager::waitForAvd(const QString &avdName, const QFutureInterface<bool> &fi) const
{
    // we cannot use adb -e wait-for-device, since that doesn't work if a emulator is already running
    // 60 rounds of 2s sleeping, two minutes for the avd to start
    QString serialNumber;
    for (int i = 0; i < 60; ++i) {
        if (fi.isCanceled())
            return QString();
        serialNumber = findAvd(avdName);
        if (!serialNumber.isEmpty())
            return waitForBooted(serialNumber, fi) ?  serialNumber : QString();
        QThread::sleep(2);
    }
    return QString();
}

bool AndroidAvdManager::isAvdBooted(const QString &device) const
{
    QStringList arguments = AndroidDeviceInfo::adbSelector(device);
    arguments << "shell" << "getprop" << "init.svc.bootanim";

    Utils::SynchronousProcess adbProc;
    adbProc.setTimeoutS(10);
    Utils::SynchronousProcessResponse response =
            adbProc.runBlocking(m_config.adbToolPath().toString(), arguments);
    if (response.result != Utils::SynchronousProcessResponse::Finished)
        return false;
    QString value = response.allOutput().trimmed();
    return value == "stopped";
}

bool AndroidAvdManager::waitForBooted(const QString &serialNumber, const QFutureInterface<bool> &fi) const
{
    // found a serial number, now wait until it's done booting...
    for (int i = 0; i < 60; ++i) {
        if (fi.isCanceled())
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

AndroidDeviceInfoList AvdManagerOutputParser::listVirtualDevices(const AndroidConfig &config)
{
    QString output;
    if (!avdManagerCommand(config, QStringList({"list", "avd"}), &output)) {
        qCDebug(avdManagerLog) << "Avd list command failed" << output << config.sdkToolsVersion();
        return {};
    }
    return parseAvdList(output);
}

AndroidDeviceInfoList AvdManagerOutputParser::parseAvdList(const QString &output)
{
    AndroidDeviceInfoList avdList;
    QStringList avdInfo;
    auto parseAvdInfo = [&avdInfo, &avdList, this] () {
        AndroidDeviceInfo avd;
        if (parseAvd(avdInfo, &avd)) {
            // armeabi-v7a devices can also run armeabi code
            if (avd.cpuAbi.contains("armeabi-v7a"))
                avd.cpuAbi << "armeabi";
            avd.state = AndroidDeviceInfo::OkState;
            avd.type = AndroidDeviceInfo::Emulator;
            avdList << avd;
        } else {
            qCDebug(avdManagerLog) << "Avd Parsing: Parsing failed: " << avdInfo;
        }
        avdInfo.clear();
    };

    foreach (QString line,  output.split('\n')) {
        if (line.startsWith("---------") || line.isEmpty()) {
            parseAvdInfo();
        } else {
            avdInfo << line;
        }
    }

    if (!avdInfo.isEmpty())
        parseAvdInfo();

    Utils::sort(avdList);

    return avdList;
}

bool AvdManagerOutputParser::parseAvd(const QStringList &deviceInfo, AndroidDeviceInfo *avd)
{
    QTC_ASSERT(avd, return false);
    foreach (const QString &line, deviceInfo) {
        QString value;
        if (valueForKey(avdInfoErrorKey, line)) {
            qCDebug(avdManagerLog) << "Avd Parsing: Skip avd device. Error key found:" << line;
            return false;
        } else if (valueForKey(avdInfoNameKey, line, &value)) {
            avd->avdname = value;
        } else if (valueForKey(avdInfoPathKey, line, &value)) {
            const Utils::FileName avdPath = Utils::FileName::fromString(value);
            if (avdPath.exists())
            {
                // Get ABI.
                Utils::FileName configFile = avdPath;
                configFile.appendPath("config.ini");
                QSettings config(configFile.toString(), QSettings::IniFormat);
                value = config.value(avdInfoAbiKey).toString();
                if (!value.isEmpty())
                    avd->cpuAbi << value;
                else
                   qCDebug(avdManagerLog) << "Avd Parsing: Cannot find ABI:" << configFile;

                // Get Target
                Utils::FileName avdInfoFile = avdPath.parentDir();
                QString avdInfoFileName = avdPath.toFileInfo().baseName() + ".ini";
                avdInfoFile.appendPath(avdInfoFileName);
                QSettings avdInfo(avdInfoFile.toString(), QSettings::IniFormat);
                value = avdInfo.value(avdInfoTargetKey).toString();
                if (!value.isEmpty())
                    avd->sdk = value.section('-', -1).toInt();
                else
                   qCDebug(avdManagerLog) << "Avd Parsing: Cannot find sdk API:" << avdInfoFile.toString();
            }
        }
    }
    return true;
}

} // namespace Internal
} // namespace Android
