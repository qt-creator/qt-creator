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

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/synchronousprocess.h>

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

// Avd list keys to parse avd
const char avdInfoNameKey[] = "Name:";
const char avdInfoPathKey[] = "Path:";
const char avdInfoAbiKey[] = "abi.type";
const char avdInfoTargetKey[] = "target";
const char avdInfoErrorKey[] = "Error:";
const char avdInfoSdcardKey[] = "Sdcard";
const char avdInfoTargetTypeKey[] = "Target";
const char avdInfoDeviceKey[] = "Device";
const char avdInfoSkinKey[] = "Skin";

const int avdCreateTimeoutMs = 30000;

/*!
    Runs the \c avdmanager tool specific to configuration \a config with arguments \a args. Returns
    \c true if the command is successfully executed. Output is copied into \a output. The function
    blocks the calling thread.
 */
bool AndroidAvdManager::avdManagerCommand(const AndroidConfig &config, const QStringList &args, QString *output)
{
    CommandLine cmd(config.avdManagerToolPath(), args);
    Utils::SynchronousProcess proc;
    auto env = AndroidConfigurations::toolsEnvironment(config).toStringList();
    proc.setEnvironment(env);
    qCDebug(avdManagerLog) << "Running AVD Manager command:" << cmd.toUserOutput();
    SynchronousProcessResponse response = proc.runBlocking(cmd);
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

    const QString avdManagerTool = config.avdManagerToolPath().toString();
    qCDebug(avdManagerLog)
            << "Running AVD Manager command:" << CommandLine(avdManagerTool, arguments).toUserOutput();
    QProcess proc;
    proc.start(avdManagerTool, arguments);
    if (!proc.waitForStarted()) {
        result.error = QApplication::translate("AndroidAvdManager",
                                               "Could not start process \"%1 %2\"")
                .arg(avdManagerTool, arguments.join(' '));
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

static void avdProcessFinished(int exitCode, QProcess *p)
{
    QTC_ASSERT(p, return);
    if (exitCode) {
        QString title = QCoreApplication::translate("Android::Internal::AndroidAvdManager",
                                                    "AVD Start Error");
        QMessageBox::critical(Core::ICore::dialogParent(), title,
                              QString::fromLatin1(p->readAll()));
    }
    p->deleteLater();
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
    m_parser(new AvdManagerOutputParser)
{

}

AndroidAvdManager::~AndroidAvdManager() = default;

QFuture<CreateAvdInfo> AndroidAvdManager::createAvd(CreateAvdInfo info) const
{
    return Utils::runAsync(&createAvdCommand, m_config, info);
}

bool AndroidAvdManager::removeAvd(const QString &name) const
{
    const CommandLine command(m_config.avdManagerToolPath(), {"delete", "avd", "-n", name});
    qCDebug(avdManagerLog) << "Running command (removeAvd):" << command.toUserOutput();
    Utils::SynchronousProcess proc;
    proc.setTimeoutS(5);
    const Utils::SynchronousProcessResponse response = proc.runBlocking(command);
    return response.result == Utils::SynchronousProcessResponse::Finished && response.exitCode == 0;
}

QFuture<AndroidDeviceInfoList> AndroidAvdManager::avdList() const
{
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
    QFileInfo info(m_config.emulatorToolPath().toString());
    if (!info.exists()) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              tr("Emulator Tool Is Missing"),
                              tr("Install the missing emulator tool (%1) to the"
                                 " installed Android SDK.")
                              .arg(m_config.emulatorToolPath().toString()));
        return false;
    }
    auto avdProcess = new QProcess();
    avdProcess->setProcessChannelMode(QProcess::MergedChannels);
    QObject::connect(avdProcess,
                     QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     avdProcess,
                     std::bind(&avdProcessFinished, std::placeholders::_1, avdProcess));

    // start the emulator
    QStringList arguments;
    if (AndroidConfigurations::force32bitEmulator())
        arguments << "-force-32bit";

    arguments << "-partition-size" << QString::number(m_config.partitionSize())
              << "-avd" << avdName;
    qCDebug(avdManagerLog) << "Running command (startAvdAsync):"
                           << CommandLine(m_config.emulatorToolPath(), arguments).toUserOutput();
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

QString AndroidAvdManager::waitForAvd(const QString &avdName,
                                      const std::function<bool()> &cancelChecker) const
{
    // we cannot use adb -e wait-for-device, since that doesn't work if a emulator is already running
    // 60 rounds of 2s sleeping, two minutes for the avd to start
    QString serialNumber;
    for (int i = 0; i < 60; ++i) {
        if (cancelChecker())
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
    SynchronousProcess adbProc;
    adbProc.setTimeoutS(10);
    const SynchronousProcessResponse response = adbProc.runBlocking(command);
    if (response.result != Utils::SynchronousProcessResponse::Finished)
        return false;
    QString value = response.allOutput().trimmed();
    return value == "stopped";
}

bool AndroidAvdManager::waitForBooted(const QString &serialNumber,
                                      const std::function<bool()> &cancelChecker) const
{
    // found a serial number, now wait until it's done booting...
    for (int i = 0; i < 60; ++i) {
        if (cancelChecker())
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

/* Currenly avdmanager tool fails to parse some AVDs because the correct
 * device definitions at devices.xml does not have some of the newest devices.
 * Particularly, failing because of tag "hw.device.manufacturer", thus removing
 * it would make paring successful. However, it has to be returned afterwards,
 * otherwise, Android Studio would give an error during parsing also. So this fix
 * aim to keep support for Qt Creator and Android Studio.
 */
static const QString avdManufacturerError = "no longer exists as a device";
static QStringList avdErrorPaths;

static void AvdConfigEditManufacturerTag(const QString &avdPathStr, bool recoverMode = false)
{
    const Utils::FilePath avdPath = Utils::FilePath::fromString(avdPathStr);
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

AndroidDeviceInfoList AvdManagerOutputParser::listVirtualDevices(const AndroidConfig &config)
{
    QString output;
    AndroidDeviceInfoList avdList;

    do {
        if (!AndroidAvdManager::avdManagerCommand(config, {"list", "avd"}, &output)) {
            qCDebug(avdManagerLog)
                << "Avd list command failed" << output << config.sdkToolsVersion();
            return {};
        }

        avdList = parseAvdList(output);
    } while (output.contains(avdManufacturerError));

    for (const QString &avdPathStr : avdErrorPaths)
        AvdConfigEditManufacturerTag(avdPathStr, true);

    return avdList;
}

AndroidDeviceInfoList AvdManagerOutputParser::parseAvdList(const QString &output)
{
    AndroidDeviceInfoList avdList;
    QStringList avdInfo;
    auto parseAvdInfo = [&avdInfo, &avdList, this] () {
        AndroidDeviceInfo avd;
        if (!avdInfo.filter(avdManufacturerError).isEmpty()) {
            for (const QString &line : avdInfo) {
                QString value;
                if (valueForKey(avdInfoPathKey, line, &value)) {
                    avdErrorPaths.append(value);
                    AvdConfigEditManufacturerTag(value);
                }
            }
        } else if (parseAvd(avdInfo, &avd)) {
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

    for (const QString &line : output.split('\n')) {
        if (line.startsWith("---------") || line.isEmpty())
            parseAvdInfo();
        else
            avdInfo << line;
    }

    Utils::sort(avdList);

    return avdList;
}

bool AvdManagerOutputParser::parseAvd(const QStringList &deviceInfo, AndroidDeviceInfo *avd)
{
    QTC_ASSERT(avd, return false);
    for (const QString &line : deviceInfo) {
        QString value;
        if (valueForKey(avdInfoErrorKey, line)) {
            qCDebug(avdManagerLog) << "Avd Parsing: Skip avd device. Error key found:" << line;
            return false;
        } else if (valueForKey(avdInfoNameKey, line, &value)) {
            avd->avdname = value;
        } else if (valueForKey(avdInfoPathKey, line, &value)) {
            const Utils::FilePath avdPath = Utils::FilePath::fromString(value);
            if (avdPath.exists())
            {
                // Get ABI.
                const Utils::FilePath configFile = avdPath.pathAppended("config.ini");
                QSettings config(configFile.toString(), QSettings::IniFormat);
                value = config.value(avdInfoAbiKey).toString();
                if (!value.isEmpty())
                    avd->cpuAbi << value;
                else
                   qCDebug(avdManagerLog) << "Avd Parsing: Cannot find ABI:" << configFile;

                // Get Target
                const QString avdInfoFileName = avd->avdname + ".ini";
                const Utils::FilePath
                        avdInfoFile = avdPath.parentDir().pathAppended(avdInfoFileName);
                QSettings avdInfo(avdInfoFile.toString(), QSettings::IniFormat);
                value = avdInfo.value(avdInfoTargetKey).toString();
                if (!value.isEmpty())
                    avd->sdk = value.section('-', -1).toInt();
                else
                   qCDebug(avdManagerLog) << "Avd Parsing: Cannot find sdk API:" << avdInfoFile.toString();
            }
        } else if (valueForKey(avdInfoDeviceKey, line, &value)) {
            avd->avdDevice = value.remove(0, 2);
        } else if (valueForKey(avdInfoTargetTypeKey, line, &value)) {
            avd->avdTarget = value.remove(0, 2);
        } else if (valueForKey(avdInfoSkinKey, line, &value)) {
            avd->avdSkin = value.remove(0, 2);
        } else if (valueForKey(avdInfoSdcardKey, line, &value)) {
            avd->avdSdcardSize = value.remove(0, 2);
        }
    }
    return true;
}

} // namespace Internal
} // namespace Android
