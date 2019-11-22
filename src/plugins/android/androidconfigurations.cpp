/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidtoolchain.h"
#include "androiddevice.h"
#include "androidmanager.h"
#include "androidqtversion.h"
#include "androiddevicedialog.h"
#include "androidsdkmanager.h"
#include "androidtoolmanager.h"
#include "avddialog.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/session.h>
#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggerkitinformation.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/synchronousprocess.h>
#include <utils/environment.h>

#include <QApplication>
#include <QDirIterator>
#include <QFileInfo>
#include <QHostAddress>
#include <QLoggingCategory>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QStringList>
#include <QTcpSocket>
#include <QThread>

#include <functional>
#include <memory>

using namespace ProjectExplorer;
using namespace Utils;

namespace {
Q_LOGGING_CATEGORY(avdConfigLog, "qtc.android.androidconfig", QtWarningMsg)
}

namespace Android {
using namespace Internal;

namespace {
    const char jdkSettingsPath[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\Java Development Kit";

    const QLatin1String SettingsGroup("AndroidConfigurations");
    const QLatin1String SDKLocationKey("SDKLocation");
    const QLatin1String SDKManagerToolArgsKey("SDKManagerToolArgs");
    const QLatin1String NDKLocationKey("NDKLocation");
    const QLatin1String OpenJDKLocationKey("OpenJDKLocation");
    const QLatin1String KeystoreLocationKey("KeystoreLocation");
    const QLatin1String AutomaticKitCreationKey("AutomatiKitCreation");
    const QLatin1String PartitionSizeKey("PartitionSize");
    const QLatin1String ToolchainHostKey("ToolchainHost");

    const QLatin1String ArmToolchainPrefix("arm-linux-androideabi");
    const QLatin1String X86ToolchainPrefix("x86");
    const QLatin1String AArch64ToolchainPrefix("aarch64-linux-android");
    const QLatin1String X86_64ToolchainPrefix("x86_64");

    const QLatin1String ArmToolsPrefix("arm-linux-androideabi");
    const QLatin1String X86ToolsPrefix("i686-linux-android");
    const QLatin1String AArch64ToolsPrefix("aarch64-linux-android");
    const QLatin1String X86_64ToolsPrefix("x86_64-linux-android");

    const QLatin1String ArmToolsDisplayName("arm");
    const QLatin1String X86ToolsDisplayName("i686");
    const QLatin1String AArch64ToolsDisplayName("aarch64");
    const QLatin1String X86_64ToolsDisplayName("x86_64");

    const QLatin1String Unknown("unknown");
    const QLatin1String keytoolName("keytool");
    const QLatin1String changeTimeStamp("ChangeTimeStamp");

    const QLatin1String sdkToolsVersionKey("Pkg.Revision");
    const QLatin1String ndkRevisionKey("Pkg.Revision");

    const QString defaultQtLiveApk = Core::ICore::resourcePath() +
            ("/qtlive/android/armv7/qt_live_viewer.apk");

    static QString sdkSettingsFileName()
    {
        return Core::ICore::installerResourcePath() + "/android.xml";
    }

    static bool is32BitUserSpace()
    {
        // Do the exact same check as android's emulator is doing:
        if (HostOsInfo::isLinuxHost()) {
            if (QSysInfo::WordSize == 32 ) {
                Environment env = Environment::systemEnvironment();
                QString executable = env.searchInPath(QLatin1String("file")).toString();
                QString shell = env.value(QLatin1String("SHELL"));
                if (executable.isEmpty() || shell.isEmpty())
                    return true; // we can't detect, but creator is 32bit so assume 32bit

                SynchronousProcess proc;
                proc.setProcessChannelMode(QProcess::MergedChannels);
                proc.setTimeoutS(30);
                SynchronousProcessResponse response = proc.runBlocking({executable, {shell}});
                if (response.result != SynchronousProcessResponse::Finished)
                    return true;
                return !response.allOutput().contains("x86-64");
            }
        }
        return false;
    }
}

//////////////////////////////////
// AndroidConfig
//////////////////////////////////

Abi AndroidConfig::abiForToolChainPrefix(const QString &toolchainPrefix)
{
    Abi::Architecture arch = Abi::UnknownArchitecture;
    unsigned char wordWidth = 32;
    if (toolchainPrefix == ArmToolchainPrefix) {
        arch = Abi::ArmArchitecture;
    } else if (toolchainPrefix == X86ToolchainPrefix) {
        arch = Abi::X86Architecture;
    } else if (toolchainPrefix == AArch64ToolchainPrefix) {
        arch = Abi::ArmArchitecture;
        wordWidth = 64;
    } else if (toolchainPrefix == X86_64ToolchainPrefix) {
        arch = Abi::X86Architecture;
        wordWidth = 64;
    }

    return Abi(arch, Abi::LinuxOS, Abi::AndroidLinuxFlavor, Abi::ElfFormat, wordWidth);
}

QLatin1String AndroidConfig::toolchainPrefix(const Abi &abi)
{
    switch (abi.architecture()) {
    case Abi::ArmArchitecture:
        if (abi.wordWidth() == 64)
            return AArch64ToolchainPrefix;
        return ArmToolchainPrefix;
    case Abi::X86Architecture:
        if (abi.wordWidth() == 64)
            return X86_64ToolchainPrefix;
        return X86ToolchainPrefix;
    default:
        return Unknown;
    }
}

QLatin1String AndroidConfig::toolsPrefix(const Abi &abi)
{
    switch (abi.architecture()) {
    case Abi::ArmArchitecture:
        if (abi.wordWidth() == 64)
            return AArch64ToolsPrefix;
        return ArmToolsPrefix;
    case Abi::X86Architecture:
        if (abi.wordWidth() == 64)
            return X86_64ToolsPrefix;
        return X86ToolsPrefix;
    default:
        return Unknown;
    }
}

QLatin1String AndroidConfig::displayName(const Abi &abi)
{
    switch (abi.architecture()) {
    case Abi::ArmArchitecture:
        if (abi.wordWidth() == 64)
            return AArch64ToolsDisplayName;
        return ArmToolsDisplayName;
    case Abi::X86Architecture:
        if (abi.wordWidth() == 64)
            return X86_64ToolsDisplayName;
        return X86ToolsDisplayName;
    default:
        return Unknown;
    }
}

void AndroidConfig::load(const QSettings &settings)
{
    // user settings
    m_partitionSize = settings.value(PartitionSizeKey, 1024).toInt();
    m_sdkLocation = FilePath::fromString(settings.value(SDKLocationKey).toString());
    m_sdkManagerToolArgs = settings.value(SDKManagerToolArgsKey).toStringList();
    m_ndkLocation = FilePath::fromString(settings.value(NDKLocationKey).toString());
    m_openJDKLocation = FilePath::fromString(settings.value(OpenJDKLocationKey).toString());
    m_keystoreLocation = FilePath::fromString(settings.value(KeystoreLocationKey).toString());
    m_toolchainHost = settings.value(ToolchainHostKey).toString();
    m_automaticKitCreation = settings.value(AutomaticKitCreationKey, true).toBool();

    PersistentSettingsReader reader;
    if (reader.load(FilePath::fromString(sdkSettingsFileName()))
            && settings.value(changeTimeStamp).toInt() != QFileInfo(sdkSettingsFileName()).lastModified().toMSecsSinceEpoch() / 1000) {
        // persisten settings
        m_sdkLocation = FilePath::fromString(reader.restoreValue(SDKLocationKey, m_sdkLocation.toString()).toString());
        m_sdkManagerToolArgs = reader.restoreValue(SDKManagerToolArgsKey, m_sdkManagerToolArgs).toStringList();
        m_ndkLocation = FilePath::fromString(reader.restoreValue(NDKLocationKey, m_ndkLocation.toString()).toString());
        m_openJDKLocation = FilePath::fromString(reader.restoreValue(OpenJDKLocationKey, m_openJDKLocation.toString()).toString());
        m_keystoreLocation = FilePath::fromString(reader.restoreValue(KeystoreLocationKey, m_keystoreLocation.toString()).toString());
        m_toolchainHost = reader.restoreValue(ToolchainHostKey, m_toolchainHost).toString();
        m_automaticKitCreation = reader.restoreValue(AutomaticKitCreationKey, m_automaticKitCreation).toBool();
        // persistent settings
    }
    m_NdkInformationUpToDate = false;
}

void AndroidConfig::save(QSettings &settings) const
{
    QFileInfo fileInfo(sdkSettingsFileName());
    if (fileInfo.exists())
        settings.setValue(changeTimeStamp, fileInfo.lastModified().toMSecsSinceEpoch() / 1000);

    // user settings
    settings.setValue(SDKLocationKey, m_sdkLocation.toString());
    settings.setValue(SDKManagerToolArgsKey, m_sdkManagerToolArgs);
    settings.setValue(NDKLocationKey, m_ndkLocation.toString());
    settings.setValue(OpenJDKLocationKey, m_openJDKLocation.toString());
    settings.setValue(KeystoreLocationKey, m_keystoreLocation.toString());
    settings.setValue(PartitionSizeKey, m_partitionSize);
    settings.setValue(AutomaticKitCreationKey, m_automaticKitCreation);
    settings.setValue(ToolchainHostKey, m_toolchainHost);
}

void AndroidConfig::updateNdkInformation() const
{
    if (m_NdkInformationUpToDate)
        return;
    m_availableNdkPlatforms.clear();
    QDirIterator it(m_ndkLocation.pathAppended("platforms").toString(), QStringList("android-*"), QDir::Dirs);
    while (it.hasNext()) {
        const QString &fileName = it.next();
        m_availableNdkPlatforms.push_back(fileName.midRef(fileName.lastIndexOf(QLatin1Char('-')) + 1).toInt());
    }
    Utils::sort(m_availableNdkPlatforms, std::greater<>());

    // detect toolchain host
    QStringList hostPatterns;
    switch (HostOsInfo::hostOs()) {
    case OsTypeLinux:
        hostPatterns << QLatin1String("linux*");
        break;
    case OsTypeWindows:
        hostPatterns << QLatin1String("windows*");
        break;
    case OsTypeMac:
        hostPatterns << QLatin1String("darwin*");
        break;
    default: /* unknown host */ return;
    }

    QDirIterator jt(m_ndkLocation.pathAppended("prebuilt").toString(), hostPatterns, QDir::Dirs);
    if (jt.hasNext()) {
        jt.next();
        m_toolchainHost = jt.fileName();
    }

    m_NdkInformationUpToDate = true;
}

QStringList AndroidConfig::apiLevelNamesFor(const SdkPlatformList &platforms)
{
    return Utils::transform(platforms, AndroidConfig::apiLevelNameFor);
}

QString AndroidConfig::apiLevelNameFor(const SdkPlatform *platform)
{
    return platform && platform->apiLevel() > 0 ?
                QString("android-%1").arg(platform->apiLevel()) : "";
}

FilePath AndroidConfig::adbToolPath() const
{
    return m_sdkLocation.pathAppended("platform-tools/adb" QTC_HOST_EXE_SUFFIX);
}

FilePath AndroidConfig::androidToolPath() const
{
    if (HostOsInfo::isWindowsHost()) {
        // I want to switch from using android.bat to using an executable. All it really does is call
        // Java and I've made some progress on it. So if android.exe exists, return that instead.
        const FilePath path = m_sdkLocation.pathAppended("tools/android" QTC_HOST_EXE_SUFFIX);
        if (path.exists())
            return path;
        return m_sdkLocation.pathAppended("tools/android" ANDROID_BAT_SUFFIX);
    }
    return m_sdkLocation.pathAppended("tools/android");
}

FilePath AndroidConfig::emulatorToolPath() const
{
    QString relativePath = "emulator/emulator";
    if (sdkToolsVersion() < QVersionNumber(25, 3, 0))
        relativePath = "tools/emulator";
    return m_sdkLocation.pathAppended(relativePath + QTC_HOST_EXE_SUFFIX);
}

FilePath AndroidConfig::sdkManagerToolPath() const
{
    QString toolPath = "tools/bin/sdkmanager";
    if (HostOsInfo::isWindowsHost())
        toolPath += ANDROID_BAT_SUFFIX;
    return m_sdkLocation.pathAppended(toolPath);
}

FilePath AndroidConfig::avdManagerToolPath() const
{
    QString toolPath = "tools/bin/avdmanager";
    if (HostOsInfo::isWindowsHost())
        toolPath += ANDROID_BAT_SUFFIX;
    return m_sdkLocation.pathAppended(toolPath);
}

FilePath AndroidConfig::aaptToolPath() const
{
    const Utils::FilePath aaptToolPath = m_sdkLocation.pathAppended("build-tools");
    QString toolPath = QString("%1/aapt").arg(buildToolsVersion().toString());
    if (HostOsInfo::isWindowsHost())
        toolPath += QTC_HOST_EXE_SUFFIX;
    return aaptToolPath.pathAppended(toolPath);
}

FilePath AndroidConfig::toolchainPath() const
{
    const FilePath toolchainPath = m_ndkLocation.pathAppended("toolchains/llvm/prebuilt/");

    // detect toolchain host
    QStringList hostPatterns;
    switch (HostOsInfo::hostOs()) {
    case OsTypeLinux:
        hostPatterns << QLatin1String("linux*");
        break;
    case OsTypeWindows:
        hostPatterns << QLatin1String("windows*");
        break;
    case OsTypeMac:
        hostPatterns << QLatin1String("darwin*");
        break;
    default: /* unknown host */ return FilePath();
    }

    QDirIterator iter(toolchainPath.toString(), hostPatterns, QDir::Dirs);
    if (iter.hasNext()) {
        iter.next();
        return toolchainPath.pathAppended(iter.fileName());
    }

    return {};
}

FilePath AndroidConfig::clangPath() const
{
    const FilePath path = toolchainPath();
    if (path.isEmpty())
        return {};
    return path.pathAppended(HostOsInfo::withExecutableSuffix("bin/clang"));
}

FilePath AndroidConfig::gdbPath(const ProjectExplorer::Abi &abi) const
{
    const FilePath path = m_ndkLocation.pathAppended(
                QString("prebuilt/%1/bin/gdb%2").arg(toolchainHost(), QTC_HOST_EXE_SUFFIX));
    if (path.exists())
        return path;
    // fallback for old NDKs (e.g. 10e)
    return m_ndkLocation.pathAppended(QString("toolchains/%1-4.9/prebuilt/%2/bin/%3-gdb%4")
            .arg(toolchainPrefix(abi), toolchainHost(), toolsPrefix(abi), QTC_HOST_EXE_SUFFIX));
}

FilePath AndroidConfig::makePath() const
{
    return m_ndkLocation.pathAppended(
                QString("prebuilt/%1/bin/make%2").arg(toolchainHost(), QTC_HOST_EXE_SUFFIX));
}

FilePath AndroidConfig::openJDKBinPath() const
{
    const FilePath path = m_openJDKLocation;
    if (!path.isEmpty())
        return path.pathAppended("bin");
    return path;
}

FilePath AndroidConfig::keytoolPath() const
{
    return openJDKBinPath().pathAppended(keytoolName);
}

QVector<AndroidDeviceInfo> AndroidConfig::connectedDevices(QString *error) const
{
    return connectedDevices(adbToolPath(), error);
}

QVector<AndroidDeviceInfo> AndroidConfig::connectedDevices(const FilePath &adbToolPath, QString *error)
{
    QVector<AndroidDeviceInfo> devices;
    SynchronousProcess adbProc;
    adbProc.setTimeoutS(30);
    CommandLine cmd{adbToolPath, {"devices"}};
    SynchronousProcessResponse response = adbProc.runBlocking(cmd);
    if (response.result != SynchronousProcessResponse::Finished) {
        if (error)
            *error = QApplication::translate("AndroidConfiguration", "Could not run: %1")
                .arg(cmd.toUserOutput());
        return devices;
    }
    QStringList adbDevs = response.allOutput().split('\n', QString::SkipEmptyParts);
    if (adbDevs.empty())
        return devices;

    while (adbDevs.first().startsWith("* daemon"))
        adbDevs.removeFirst(); // remove the daemon logs
    adbDevs.removeFirst(); // remove "List of devices attached" header line

    // workaround for '????????????' serial numbers:
    // can use "adb -d" when only one usb device attached
    foreach (const QString &device, adbDevs) {
        const QString serialNo = device.left(device.indexOf('\t')).trimmed();
        const QString deviceType = device.mid(device.indexOf('\t')).trimmed();
        if (isBootToQt(adbToolPath, serialNo))
            continue;
        AndroidDeviceInfo dev;
        dev.serialNumber = serialNo;
        dev.type = serialNo.startsWith(QLatin1String("emulator")) ? AndroidDeviceInfo::Emulator : AndroidDeviceInfo::Hardware;
        dev.sdk = getSDKVersion(adbToolPath, dev.serialNumber);
        dev.cpuAbi = getAbis(adbToolPath, dev.serialNumber);
        if (deviceType == QLatin1String("unauthorized"))
            dev.state = AndroidDeviceInfo::UnAuthorizedState;
        else if (deviceType == QLatin1String("offline"))
            dev.state = AndroidDeviceInfo::OfflineState;
        else
            dev.state = AndroidDeviceInfo::OkState;

        if (dev.type == AndroidDeviceInfo::Emulator) {
            dev.avdname = getAvdName(dev.serialNumber);
            if (dev.avdname.isEmpty())
                dev.avdname = serialNo;
        }

        devices.push_back(dev);
    }

    Utils::sort(devices);
    if (devices.isEmpty() && error)
        *error = QApplication::translate("AndroidConfiguration",
                                         "No devices found in output of: %1")
            .arg(cmd.toUserOutput());
    return devices;
}

bool AndroidConfig::isConnected(const QString &serialNumber) const
{
    QVector<AndroidDeviceInfo> devices = connectedDevices();
    foreach (AndroidDeviceInfo device, devices) {
        if (device.serialNumber == serialNumber)
            return true;
    }
    return false;
}

bool AndroidConfig::isBootToQt(const QString &device) const
{
    return isBootToQt(adbToolPath(), device);
}

bool AndroidConfig::isBootToQt(const FilePath &adbToolPath, const QString &device)
{
    // workaround for '????????????' serial numbers
    CommandLine cmd(adbToolPath, AndroidDeviceInfo::adbSelector(device));
    cmd.addArg("shell");
    cmd.addArg("ls -l /system/bin/appcontroller || ls -l /usr/bin/appcontroller && echo Boot2Qt");

    SynchronousProcess adbProc;
    adbProc.setTimeoutS(10);
    SynchronousProcessResponse response = adbProc.runBlocking(cmd);
    return response.result == SynchronousProcessResponse::Finished
            && response.allOutput().contains(QLatin1String("Boot2Qt"));
}


QString AndroidConfig::getDeviceProperty(const FilePath &adbToolPath, const QString &device, const QString &property)
{
    // workaround for '????????????' serial numbers
    CommandLine cmd(adbToolPath, AndroidDeviceInfo::adbSelector(device));
    cmd.addArgs({"shell", "getprop", property});

    SynchronousProcess adbProc;
    adbProc.setTimeoutS(10);
    SynchronousProcessResponse response = adbProc.runBlocking(cmd);
    if (response.result != SynchronousProcessResponse::Finished)
        return QString();

    return response.allOutput();
}

int AndroidConfig::getSDKVersion(const QString &device) const
{
    return getSDKVersion(adbToolPath(), device);
}

int AndroidConfig::getSDKVersion(const FilePath &adbToolPath, const QString &device)
{
    QString tmp = getDeviceProperty(adbToolPath, device, "ro.build.version.sdk");
    if (tmp.isEmpty())
        return -1;
    return tmp.trimmed().toInt();
}

QString AndroidConfig::getAvdName(const QString &serialnumber)
{
    int index = serialnumber.indexOf(QLatin1String("-"));
    if (index == -1)
        return QString();
    bool ok;
    int port = serialnumber.midRef(index + 1).toInt(&ok);
    if (!ok)
        return QString();

    const QByteArray avdName = "avd name\n";

    QTcpSocket tcpSocket;
    tcpSocket.connectToHost(QHostAddress(QHostAddress::LocalHost), port);
    if (!tcpSocket.waitForConnected(100)) // Don't wait more than 100ms for a local connection
        return QString{};

    tcpSocket.write(avdName + "exit\n");
    tcpSocket.waitForDisconnected(500);

    QByteArray name;
    const QByteArrayList response = tcpSocket.readAll().split('\n');
    // The input "avd name" might not be echoed as-is, but contain ASCII
    // control sequences.
    for (int i = response.size() - 1; i > 1; --i) {
        if (response.at(i).startsWith("OK")) {
            name = response.at(i - 1);
            break;
        }
    }
    return QString::fromLatin1(name).trimmed();
}

AndroidConfig::OpenGl AndroidConfig::getOpenGLEnabled(const QString &emulator) const
{
    QDir dir = QDir::home();
    if (!dir.cd(QLatin1String(".android")))
        return OpenGl::Unknown;
    if (!dir.cd(QLatin1String("avd")))
        return OpenGl::Unknown;
    if (!dir.cd(emulator + QLatin1String(".avd")))
        return OpenGl::Unknown;
    QFile file(dir.filePath(QLatin1String("config.ini")));
    if (!file.exists())
        return OpenGl::Unknown;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return OpenGl::Unknown;
    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        if (line.contains("hw.gpu.enabled") && line.contains("yes"))
            return OpenGl::Enabled;
    }
    return OpenGl::Disabled;
}

//!
//! \brief AndroidConfigurations::getProductModel
//! \param device serial number
//! \return the produce model of the device or if that cannot be read the serial number
//!
QString AndroidConfig::getProductModel(const QString &device) const
{
    if (m_serialNumberToDeviceName.contains(device))
        return m_serialNumberToDeviceName.value(device);

    QString model = getDeviceProperty(adbToolPath(), device, "ro.product.model").trimmed();
    if (model.isEmpty())
        return device;

    if (!device.startsWith(QLatin1String("????")))
        m_serialNumberToDeviceName.insert(device, model);
    return model;
}

QStringList AndroidConfig::getAbis(const QString &device) const
{
    return getAbis(adbToolPath(), device);
}

QStringList AndroidConfig::getAbis(const FilePath &adbToolPath, const QString &device)
{
    QStringList result;
    // First try via ro.product.cpu.abilist
    QStringList arguments = AndroidDeviceInfo::adbSelector(device);
    arguments << "shell" << "getprop" << "ro.product.cpu.abilist";
    SynchronousProcess adbProc;
    adbProc.setTimeoutS(10);
    SynchronousProcessResponse response = adbProc.runBlocking({adbToolPath, arguments});
    if (response.result != SynchronousProcessResponse::Finished)
        return result;

    QString output = response.allOutput().trimmed();
    if (!output.isEmpty()) {
        QStringList result = output.split(QLatin1Char(','));
        if (!result.isEmpty())
            return result;
    }

    // Fall back to ro.product.cpu.abi, ro.product.cpu.abi2 ...
    for (int i = 1; i < 6; ++i) {
        QStringList arguments = AndroidDeviceInfo::adbSelector(device);
        arguments << QLatin1String("shell") << QLatin1String("getprop");
        if (i == 1)
            arguments << QLatin1String("ro.product.cpu.abi");
        else
            arguments << QString::fromLatin1("ro.product.cpu.abi%1").arg(i);

        SynchronousProcess abiProc;
        abiProc.setTimeoutS(10);
        SynchronousProcessResponse abiResponse = abiProc.runBlocking({adbToolPath, arguments});
        if (abiResponse.result != SynchronousProcessResponse::Finished)
            return result;

        QString abi = abiResponse.allOutput().trimmed();
        if (abi.isEmpty())
            break;
        result << abi;
    }
    return result;
}

bool AndroidConfig::useNativeUiTools() const
{
    const QVersionNumber version = sdkToolsVersion();
    return !version.isNull() && version <= QVersionNumber(25, 3 ,0);
}

QString AndroidConfig::bestNdkPlatformMatch(int target) const
{
    target = std::max(AndroidManager::apiLevelRange().first, target);
    updateNdkInformation();
    foreach (int apiLevel, m_availableNdkPlatforms) {
        if (apiLevel <= target)
            return QString::fromLatin1("android-%1").arg(apiLevel);
    }
    return QString("android-%1").arg(AndroidManager::apiLevelRange().first);
}

FilePath AndroidConfig::sdkLocation() const
{
    return m_sdkLocation;
}

void AndroidConfig::setSdkLocation(const FilePath &sdkLocation)
{
    m_sdkLocation = sdkLocation;
}

QVersionNumber AndroidConfig::sdkToolsVersion() const
{
    QVersionNumber version;
    if (m_sdkLocation.exists()) {
        const Utils::FilePath sdkToolsPropertiesPath
                = m_sdkLocation.pathAppended("tools/source.properties");
        QSettings settings(sdkToolsPropertiesPath.toString(), QSettings::IniFormat);
        auto versionStr = settings.value(sdkToolsVersionKey).toString();
        version = QVersionNumber::fromString(versionStr);
    }
    return version;
}

QVersionNumber AndroidConfig::buildToolsVersion() const
{
    QVersionNumber maxVersion;
    QDir buildToolsDir(m_sdkLocation.pathAppended("build-tools").toString());
    for (const QFileInfo &file: buildToolsDir.entryList(QDir::Dirs|QDir::NoDotAndDotDot))
        maxVersion = qMax(maxVersion, QVersionNumber::fromString(file.fileName()));
    return maxVersion;
}


QStringList AndroidConfig::sdkManagerToolArgs() const
{
    return m_sdkManagerToolArgs;
}

void AndroidConfig::setSdkManagerToolArgs(const QStringList &args)
{
    m_sdkManagerToolArgs = args;
}

FilePath AndroidConfig::ndkLocation() const
{
    return m_ndkLocation;
}

static inline QString gdbServerArch(const QString &androidAbi)
{
    if (androidAbi == "arm64-v8a") {
        return QString("arm64");
    } else if (androidAbi == "armeabi-v7a") {
        return QString("arm");
    } else if (androidAbi == "x86_64") {
        return QString("x86_64");
    } else if (androidAbi == "x86") {
        return QString("x86");
    } else {
        return {};
    }
}

FilePath AndroidConfig::gdbServer(const QString &androidAbi) const
{
    const FilePath path = AndroidConfigurations::currentConfig().ndkLocation()
            .pathAppended(QString("prebuilt/android-%1/gdbserver/gdbserver")
                                                .arg(gdbServerArch(androidAbi)));
    if (path.exists())
        return path;
    return {};
}

QVersionNumber AndroidConfig::ndkVersion() const
{
    QVersionNumber version;
    if (!m_ndkLocation.exists()) {
        qCDebug(avdConfigLog) << "Cannot find ndk version. Check NDK path."
                              << m_ndkLocation.toString();
        return version;
    }

    const FilePath ndkPropertiesPath = m_ndkLocation.pathAppended("source.properties");
    if (ndkPropertiesPath.exists()) {
        // source.properties files exists in NDK version > 11
        QSettings settings(ndkPropertiesPath.toString(), QSettings::IniFormat);
        auto versionStr = settings.value(ndkRevisionKey).toString();
        version = QVersionNumber::fromString(versionStr);
    } else {
        // No source.properties. There should be a file named RELEASE.TXT
        const FilePath ndkReleaseTxtPath = m_ndkLocation.pathAppended("RELEASE.TXT");
        Utils::FileReader reader;
        QString errorString;
        if (reader.fetch(ndkReleaseTxtPath.toString(), &errorString)) {
            // RELEASE.TXT contains the ndk version in either of the following formats:
            // r6a
            // r10e (64 bit)
            QString content = QString::fromUtf8(reader.data());
            QRegularExpression re("(r)(?<major>[0-9]{1,2})(?<minor>[a-z]{1,1})");
            QRegularExpressionMatch match = re.match(content);
            if (match.hasMatch()) {
                QString major = match.captured("major");
                QString minor = match.captured("minor");
                // Minor version: a = 0, b = 1, c = 2 and so on.
                // Int equivalent = minorVersionChar - 'a'. i.e. minorVersionChar - 97.
                version = QVersionNumber::fromString(QString("%1.%2.0").arg(major)
                                                     .arg((int)minor[0].toLatin1() - 97));
            } else {
                qCDebug(avdConfigLog) << "Cannot find ndk version. Cannot parse RELEASE.TXT."
                                      << content;
            }
        } else {
            qCDebug(avdConfigLog) << "Cannot find ndk version." << errorString;
        }
    }
    return version;
}

void AndroidConfig::setNdkLocation(const FilePath &ndkLocation)
{
    m_ndkLocation = ndkLocation;
    m_NdkInformationUpToDate = false;
}

FilePath AndroidConfig::openJDKLocation() const
{
    return m_openJDKLocation;
}

void AndroidConfig::setOpenJDKLocation(const FilePath &openJDKLocation)
{
    m_openJDKLocation = openJDKLocation;
}

FilePath AndroidConfig::keystoreLocation() const
{
    return m_keystoreLocation;
}

void AndroidConfig::setKeystoreLocation(const FilePath &keystoreLocation)
{
    m_keystoreLocation = keystoreLocation;
}

QString AndroidConfig::toolchainHost() const
{
    updateNdkInformation();
    return m_toolchainHost;
}

unsigned AndroidConfig::partitionSize() const
{
    return m_partitionSize;
}

void AndroidConfig::setPartitionSize(unsigned partitionSize)
{
    m_partitionSize = partitionSize;
}

bool AndroidConfig::automaticKitCreation() const
{
    return m_automaticKitCreation;
}

void AndroidConfig::setAutomaticKitCreation(bool b)
{
    m_automaticKitCreation = b;
}

FilePath AndroidConfig::qtLiveApkPath() const
{
    QString apkPathStr(defaultQtLiveApk);
    if (qEnvironmentVariableIsSet("QTC_QT_LIVE_APK_PATH"))
         apkPathStr = QString::fromLocal8Bit(qgetenv("QTC_QT_LIVE_APK_PATH"));
    return Utils::FilePath::fromString(apkPathStr);
}

///////////////////////////////////
// AndroidConfigurations
///////////////////////////////////
void AndroidConfigurations::setConfig(const AndroidConfig &devConfigs)
{
    emit m_instance->aboutToUpdate();
    m_instance->m_config = devConfigs;

    m_instance->save();
    m_instance->updateAndroidDevice();
    m_instance->registerNewToolChains();
    m_instance->updateAutomaticKitList();
    m_instance->removeOldToolChains();
    emit m_instance->updated();
}

AndroidDeviceInfo AndroidConfigurations::showDeviceDialog(Project *project,
                                                          int apiLevel, const QStringList &abis)
{
    QString serialNumber;
    for (const QString &abi : abis) {
        serialNumber = defaultDevice(project, abi);
        if (!serialNumber.isEmpty())
            break;
    }
    AndroidDeviceDialog dialog(apiLevel, abis, serialNumber, Core::ICore::mainWindow());
    AndroidDeviceInfo info = dialog.device();
    if (dialog.saveDeviceSelection() && info.isValid()) {
        const QString serialNumber = info.type == AndroidDeviceInfo::Hardware ?
                    info.serialNumber : info.avdname;
        if (!serialNumber.isEmpty())
            AndroidConfigurations::setDefaultDevice(project, AndroidManager::devicePreferredAbi(info.cpuAbi, abis), serialNumber);
    }
    return info;
}

void AndroidConfigurations::clearDefaultDevices(Project *project)
{
    if (m_instance->m_defaultDeviceForAbi.contains(project))
        m_instance->m_defaultDeviceForAbi.remove(project);
}

void AndroidConfigurations::setDefaultDevice(Project *project, const QString &abi, const QString &serialNumber)
{
    m_instance->m_defaultDeviceForAbi[project][abi] = serialNumber;
}

QString AndroidConfigurations::defaultDevice(Project *project, const QString &abi)
{
    if (!m_instance->m_defaultDeviceForAbi.contains(project))
        return QString();
    const QMap<QString, QString> &map = m_instance->m_defaultDeviceForAbi.value(project);
    if (!map.contains(abi))
        return QString();
    return map.value(abi);
}

static bool matchToolChain(const ToolChain *atc, const ToolChain *btc)
{
    if (atc == btc)
        return true;

    if (!atc || !btc)
        return false;

    if (atc->typeId() != Constants::ANDROID_TOOLCHAIN_TYPEID || btc->typeId() != Constants::ANDROID_TOOLCHAIN_TYPEID)
        return false;

    auto aatc = static_cast<const AndroidToolChain *>(atc);
    auto abtc = static_cast<const AndroidToolChain *>(btc);
    return aatc->targetAbi() == abtc->targetAbi();
}

void AndroidConfigurations::registerNewToolChains()
{
    const QList<ToolChain *> existingAndroidToolChains
            = ToolChainManager::toolChains(Utils::equal(&ToolChain::typeId,
                                                        Core::Id(Constants::ANDROID_TOOLCHAIN_TYPEID)));
    const QList<ToolChain *> newToolchains
            = AndroidToolChainFactory::autodetectToolChainsForNdk(existingAndroidToolChains);
    foreach (ToolChain *tc, newToolchains)
        ToolChainManager::registerToolChain(tc);
}

void AndroidConfigurations::removeOldToolChains()
{
    foreach (ToolChain *tc, ToolChainManager::toolChains(Utils::equal(&ToolChain::typeId, Core::Id(Constants::ANDROID_TOOLCHAIN_TYPEID)))) {
        if (!tc->isValid())
            ToolChainManager::deregisterToolChain(tc);
    }
}

static QVariant findOrRegisterDebugger(ToolChain *tc)
{
    const FilePath command = AndroidConfigurations::currentConfig().gdbPath(tc->targetAbi());
    // check if the debugger is already registered, but ignoring the display name
    const Debugger::DebuggerItem *existing = Debugger::DebuggerItemManager::findByCommand(command);
    if (existing && existing->engineType() == Debugger::GdbEngineType && existing->isAutoDetected()
            && existing->abis() == Abis{tc->targetAbi()})
        return existing->id();
    // debugger not found, register a new one
    Debugger::DebuggerItem debugger;
    debugger.setCommand(command);
    debugger.setEngineType(Debugger::GdbEngineType);
    debugger.setUnexpandedDisplayName(
        AndroidConfigurations::tr("Android Debugger for %1").arg(tc->displayName()));
    debugger.setAutoDetected(true);
    debugger.setAbi(tc->targetAbi());
    debugger.reinitializeFromFile();
    return Debugger::DebuggerItemManager::registerDebugger(debugger);
}

void AndroidConfigurations::updateAutomaticKitList()
{
    const QList<Kit *> androidKits = Utils::filtered(KitManager::kits(), [](Kit *k) {
        Core::Id deviceTypeId = DeviceTypeKitAspect::deviceTypeId(k);
        return deviceTypeId == Core::Id(Constants::ANDROID_DEVICE_TYPE);
    });

    for (auto k: androidKits) {
        if (k->value(Constants::ANDROID_KIT_NDK).isNull() || k->value(Constants::ANDROID_KIT_SDK).isNull()) {
            k->setValueSilently(Constants::ANDROID_KIT_NDK, currentConfig().ndkLocation().toString());
            k->setValue(Constants::ANDROID_KIT_SDK, currentConfig().sdkLocation().toString());
        }
    }

    const QList<Kit *> existingKits = Utils::filtered(KitManager::kits(), [](Kit *k) {
        Core::Id deviceTypeId = DeviceTypeKitAspect::deviceTypeId(k);
        if (k->isAutoDetected() && !k->isSdkProvided()
                && deviceTypeId == Core::Id(Constants::ANDROID_DEVICE_TYPE)) {
            if (!QtSupport::QtKitAspect::qtVersion(k))
                KitManager::deregisterKit(k); // Remove autoDetected kits without Qt.
            else
                return true;
        }
        return false;
    });

    QHash<Abi, QList<const QtSupport::BaseQtVersion *> > qtVersionsForArch;
    const QList<QtSupport::BaseQtVersion *> qtVersions
            = QtSupport::QtVersionManager::versions([](const QtSupport::BaseQtVersion *v) {
        return v->type() == Constants::ANDROIDQT;
    });
    for (const QtSupport::BaseQtVersion *qtVersion : qtVersions) {
        const Abis qtAbis = qtVersion->qtAbis();
        if (qtAbis.empty())
            continue;
        qtVersionsForArch[qtAbis.first()].append(qtVersion);
    }

    DeviceManager *dm = DeviceManager::instance();
    IDevice::ConstPtr device = dm->find(Core::Id(Constants::ANDROID_DEVICE_ID));
    if (device.isNull()) {
        // no device, means no sdk path
        for (Kit *k : existingKits)
            KitManager::deregisterKit(k);
        return;
    }

    // register new kits
    const QList<ToolChain *> toolchains = ToolChainManager::toolChains([](const ToolChain *tc) {
        return tc->isAutoDetected()
            && tc->isValid()
            && tc->typeId() == Constants::ANDROID_TOOLCHAIN_TYPEID;
    });
    for (ToolChain *tc : toolchains) {
        if (tc->language() != Core::Id(ProjectExplorer::Constants::CXX_LANGUAGE_ID))
            continue;
        const QList<ToolChain *> allLanguages = Utils::filtered(toolchains,
                                                                       [tc](ToolChain *otherTc) {
            return tc->targetAbi() == otherTc->targetAbi();
        });

        QHash<Core::Id, ToolChain *> toolChainForLanguage;
        for (ToolChain *tc : allLanguages)
            toolChainForLanguage[tc->language()] = tc;

        for (const QtSupport::BaseQtVersion *qt : qtVersionsForArch.value(tc->targetAbi())) {
            Kit *existingKit = Utils::findOrDefault(existingKits, [&](const Kit *b) {
                if (qt != QtSupport::QtKitAspect::qtVersion(b))
                    return false;
                return matchToolChain(toolChainForLanguage[ProjectExplorer::Constants::CXX_LANGUAGE_ID],
                                      ToolChainKitAspect::toolChain(b, ProjectExplorer::Constants::CXX_LANGUAGE_ID))
                        && matchToolChain(toolChainForLanguage[ProjectExplorer::Constants::C_LANGUAGE_ID],
                                          ToolChainKitAspect::toolChain(b, ProjectExplorer::Constants::C_LANGUAGE_ID));
            });

            const auto initializeKit = [allLanguages, device, tc, qt](Kit *k) {
                k->setAutoDetected(true);
                k->setAutoDetectionSource("AndroidConfiguration");
                DeviceTypeKitAspect::setDeviceTypeId(k, Core::Id(Constants::ANDROID_DEVICE_TYPE));
                for (ToolChain *tc : allLanguages)
                    ToolChainKitAspect::setToolChain(k, tc);
                QtSupport::QtKitAspect::setQtVersion(k, qt);
                DeviceKitAspect::setDevice(k, device);
                Debugger::DebuggerKitAspect::setDebugger(k, findOrRegisterDebugger(tc));
                k->makeSticky();
                k->setUnexpandedDisplayName(tr("Android for %1 (Clang %2)")
                                                  .arg(static_cast<const AndroidQtVersion *>(qt)->androidAbis().join(","))
                                                  .arg(qt->displayName()));
                k->setValueSilently(Constants::ANDROID_KIT_NDK, currentConfig().ndkLocation().toString());
                k->setValueSilently(Constants::ANDROID_KIT_SDK, currentConfig().sdkLocation().toString());
            };

            if (existingKit)
                initializeKit(existingKit); // Update the existing kit with new data.
            else
                KitManager::registerKit(initializeKit);
        }
    }
}

bool AndroidConfigurations::force32bitEmulator()
{
    return m_instance->m_force32bit;
}

QProcessEnvironment AndroidConfigurations::toolsEnvironment(const AndroidConfig &config)
{
    Environment env = Environment::systemEnvironment();
    Utils::FilePath jdkLocation = config.openJDKLocation();
    if (!jdkLocation.isEmpty()) {
        env.set("JAVA_HOME", jdkLocation.toUserOutput());
        env.prependOrSetPath(jdkLocation.pathAppended("bin").toUserOutput());
    }
    return env.toProcessEnvironment();
}

/**
 * Workaround for '????????????' serial numbers
 * @return ("-d") for buggy devices, ("-s", <serial no>) for normal
 */
QStringList AndroidDeviceInfo::adbSelector(const QString &serialNumber)
{
    if (serialNumber.startsWith(QLatin1String("????")))
        return QStringList("-d");
    return QStringList({"-s",  serialNumber});
}

bool AndroidDeviceInfo::operator<(const AndroidDeviceInfo &other) const
{
    if (serialNumber.contains("????") != other.serialNumber.contains("????"))
        return !serialNumber.contains("????");
    if (type != other.type)
        return type == AndroidDeviceInfo::Hardware;
    if (sdk != other.sdk)
        return sdk < other.sdk;
    if (avdname != other.avdname)
        return avdname < other.avdname;

    return serialNumber < other.serialNumber;
}

const AndroidConfig &AndroidConfigurations::currentConfig()
{
    return m_instance->m_config; // ensure that m_instance is initialized
}

AndroidSdkManager *AndroidConfigurations::sdkManager()
{
    return m_instance->m_sdkManager.get();
}

AndroidConfigurations *AndroidConfigurations::instance()
{
    return m_instance;
}

void AndroidConfigurations::save()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    m_config.save(*settings);
    settings->endGroup();
}

AndroidConfigurations::AndroidConfigurations()
    : m_sdkManager(new AndroidSdkManager(m_config))
{
    load();

    connect(SessionManager::instance(), &SessionManager::projectRemoved,
            this, &AndroidConfigurations::clearDefaultDevices);
    connect(DeviceManager::instance(), &DeviceManager::devicesLoaded,
            this, &AndroidConfigurations::updateAndroidDevice);

    m_force32bit = is32BitUserSpace();

    m_instance = this;
}

AndroidConfigurations::~AndroidConfigurations() = default;

static FilePath javaHomeForJavac(const FilePath &location)
{
    QFileInfo fileInfo = location.toFileInfo();
    int tries = 5;
    while (tries > 0) {
        QDir dir = fileInfo.dir();
        dir.cdUp();
        if (QFileInfo::exists(dir.filePath(QLatin1String("lib/tools.jar"))))
            return FilePath::fromString(dir.path());
        if (fileInfo.isSymLink())
            fileInfo.setFile(fileInfo.symLinkTarget());
        else
            break;
        --tries;
    }
    return FilePath();
}

void AndroidConfigurations::load()
{
    bool saveSettings = false;
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    m_config.load(*settings);

    if (m_config.openJDKLocation().isEmpty()) {
        if (HostOsInfo::isLinuxHost()) {
            Environment env = Environment::systemEnvironment();
            FilePath location = env.searchInPath(QLatin1String("javac"));
            QFileInfo fi = location.toFileInfo();
            if (fi.exists() && fi.isExecutable() && !fi.isDir()) {
                m_config.setOpenJDKLocation(javaHomeForJavac(location));
                saveSettings = true;
            }
        } else if (HostOsInfo::isMacHost()) {
            QFileInfo javaHomeExec(QLatin1String("/usr/libexec/java_home"));
            if (javaHomeExec.isExecutable() && !javaHomeExec.isDir()) {
                SynchronousProcess proc;
                proc.setTimeoutS(2);
                proc.setProcessChannelMode(QProcess::MergedChannels);
                SynchronousProcessResponse response =
                        proc.runBlocking({javaHomeExec.absoluteFilePath(), {}});
                if (response.result == SynchronousProcessResponse::Finished) {
                    const QString &javaHome = response.allOutput().trimmed();
                    if (!javaHome.isEmpty() && QFileInfo::exists(javaHome))
                        m_config.setOpenJDKLocation(FilePath::fromString(javaHome));
                }
            }
        } else if (HostOsInfo::isWindowsHost()) {
            QStringList allVersions;
            std::unique_ptr<QSettings> settings(new QSettings(jdkSettingsPath,
                                                              QSettings::NativeFormat));
            allVersions = settings->childGroups();
#ifdef Q_OS_WIN
            if (allVersions.isEmpty()) {
                settings.reset(new QSettings(jdkSettingsPath, QSettings::Registry64Format));
                allVersions = settings->childGroups();
            }
#endif // Q_OS_WIN

            QString javaHome;
            int major = -1;
            int minor = -1;
            foreach (const QString &version, allVersions) {
                QStringList parts = version.split(QLatin1Char('.'));
                if (parts.size() != 2) // not interested in 1.7.0_u21
                    continue;
                bool okMajor, okMinor;
                int tmpMajor = parts.at(0).toInt(&okMajor);
                int tmpMinor = parts.at(1).toInt(&okMinor);
                if (!okMajor || !okMinor)
                    continue;
                if (tmpMajor > major
                        || (tmpMajor == major
                            && tmpMinor > minor)) {
                    settings->beginGroup(version);
                    QString tmpJavaHome = settings->value(QLatin1String("JavaHome")).toString();
                    settings->endGroup();
                    if (!QFileInfo::exists(tmpJavaHome))
                        continue;

                    major = tmpMajor;
                    minor = tmpMinor;
                    javaHome = tmpJavaHome;
                }
            }
            if (!javaHome.isEmpty()) {
                m_config.setOpenJDKLocation(FilePath::fromString(javaHome));
                saveSettings = true;
            }
        }
    }

    settings->endGroup();

    if (saveSettings)
        save();
}

void AndroidConfigurations::updateAndroidDevice()
{
    DeviceManager * const devMgr = DeviceManager::instance();
    if (m_instance->m_config.adbToolPath().exists())
        devMgr->addDevice(AndroidDevice::create());
    else if (devMgr->find(Constants::ANDROID_DEVICE_ID))
        devMgr->removeDevice(Core::Id(Constants::ANDROID_DEVICE_ID));
}

AndroidConfigurations *AndroidConfigurations::m_instance = nullptr;

QDebug &operator<<(QDebug &stream, const AndroidDeviceInfo &device)
{
    stream << "Type:"<< (device.type == AndroidDeviceInfo::Emulator ? "Emulator" : "Device")
           << ", ABI:" << device.cpuAbi << ", Serial:" << device.serialNumber
           << ", Name:" << device.avdname << ", API:" << device.sdk
           << ", Authorised:" << !device.unauthorized;
    return stream;
}

} // namespace Android
