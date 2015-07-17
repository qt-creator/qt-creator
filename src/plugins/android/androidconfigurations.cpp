/**************************************************************************
**
** Copyright (C) 2015 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidtoolchain.h"
#include "androiddevice.h"
#include "androidgdbserverkitinformation.h"
#include "androidqtversion.h"
#include "androiddevicedialog.h"
#include "avddialog.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <utils/hostosinfo.h>
#include <utils/persistentsettings.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/devicesupport/devicemanager.h>
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
#include <utils/sleep.h>

#include <QDateTime>
#include <QSettings>
#include <QStringList>
#include <QProcess>
#include <QFileInfo>
#include <QDirIterator>
#include <QMetaObject>
#include <QApplication>
#include <QtConcurrentRun>

#include <QStringListModel>
#include <QMessageBox>
#include <QTcpSocket>
#include <QHostAddress>

#include <functional>

using namespace ProjectExplorer;
using namespace Utils;

namespace Android {
using namespace Internal;

namespace {
    const QLatin1String SettingsGroup("AndroidConfigurations");
    const QLatin1String SDKLocationKey("SDKLocation");
    const QLatin1String NDKLocationKey("NDKLocation");
    const QLatin1String NDKToolchainVersionKey("NDKToolchainVersion");
    const QLatin1String AntLocationKey("AntLocation");
    const QLatin1String OpenJDKLocationKey("OpenJDKLocation");
    const QLatin1String KeystoreLocationKey("KeystoreLocation");
    const QLatin1String AutomaticKitCreationKey("AutomatiKitCreation");
    const QLatin1String UseGradleKey("UseGradle");
    const QLatin1String MakeExtraSearchDirectory("MakeExtraSearchDirectory");
    const QLatin1String DefaultDevice("DefaultDevice");
    const QLatin1String PartitionSizeKey("PartitionSize");
    const QLatin1String ToolchainHostKey("ToolchainHost");

    const QLatin1String ArmToolchainPrefix("arm-linux-androideabi");
    const QLatin1String X86ToolchainPrefix("x86");
    const QLatin1String MipsToolchainPrefix("mipsel-linux-android");
    const QLatin1String AArch64ToolchainPrefix("aarch64-linux-android");
    const QLatin1String X86_64ToolchainPrefix("x86_64");

    const QLatin1String ArmToolsPrefix("arm-linux-androideabi");
    const QLatin1String X86ToolsPrefix("i686-linux-android");
    const QLatin1String MipsToolsPrefix("mipsel-linux-android");
    const QLatin1String AArch64ToolsPrefix("aarch64-linux-android");
    const QLatin1String X86_64ToolsPrefix("x86_64-linux-android");

    const QLatin1String ArmToolsDisplayName("arm");
    const QLatin1String X86ToolsDisplayName("i686");
    const QLatin1String MipsToolsDisplayName("mipsel");
    const QLatin1String AArch64ToolsDisplayName("aarch64");
    const QLatin1String X86_64ToolsDisplayName("x86_64");

    const QLatin1String Unknown("unknown");
    const QLatin1String keytoolName("keytool");
    const QLatin1String jarsignerName("jarsigner");
    const QLatin1String changeTimeStamp("ChangeTimeStamp");

    static QString sdkSettingsFileName()
    {
        return QFileInfo(Core::ICore::settings(QSettings::SystemScope)->fileName()).absolutePath()
                + QLatin1String("/qtcreator/android.xml");
    }

    bool androidDevicesLessThan(const AndroidDeviceInfo &dev1, const AndroidDeviceInfo &dev2)
    {
        if (dev1.serialNumber.contains(QLatin1String("????")) != dev2.serialNumber.contains(QLatin1String("????")))
            return !dev1.serialNumber.contains(QLatin1String("????"));
        if (dev1.type != dev2.type)
            return dev1.type == AndroidDeviceInfo::Hardware;
        if (dev1.sdk != dev2.sdk)
            return dev1.sdk < dev2.sdk;
        if (dev1.avdname != dev2.avdname)
            return dev1.avdname < dev2.avdname;

        return dev1.serialNumber < dev2.serialNumber;
    }

    static QStringList cleanAndroidABIs(const QStringList &abis)
    {
        QStringList res;
        foreach (const QString &abi, abis) {
            int index = abi.lastIndexOf(QLatin1Char('/'));
            if (index == -1)
                res << abi;
            else
                res << abi.mid(index + 1);
        }
        return res;
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

                QProcess proc;
                proc.setProcessChannelMode(QProcess::MergedChannels);
                proc.start(executable, QStringList() << shell);
                if (!proc.waitForFinished(2000)) {
                    proc.kill();
                    return true;
                }
                if (proc.readAll().contains("x86-64"))
                    return false;
                return true;
            }
        }
        return false;
    }

    // Some preview sdks use a non integer version
    int apiLevelFromAndroidList(const QString &string)
    {
        bool ok;
        int result = string.toInt(&ok);
        if (ok)
            return result;
        Utils::FileName sdkLocation = AndroidConfigurations::currentConfig().sdkLocation();
        sdkLocation.appendPath(QLatin1String("/platforms/android-") + string + QLatin1String("/source.properties"));
        result = QSettings(sdkLocation.toString(), QSettings::IniFormat).value(QLatin1String("AndroidVersion.ApiLevel")).toInt(&ok);
        if (ok)
            return result;
        if (string == QLatin1String("L"))
            return 21;
        if (string == QLatin1String("MNC"))
            return 22;
        return 23; // At least
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
    } else if (toolchainPrefix == MipsToolchainPrefix) {
        arch = Abi::MipsArchitecture;
    } else if (toolchainPrefix == AArch64ToolchainPrefix) {
        arch = Abi::ArmArchitecture;
        wordWidth = 64;
    } else if (toolchainPrefix == X86_64ToolchainPrefix) {
        arch = Abi::X86Architecture;
        wordWidth = 64;
    }

    Abi abi = ProjectExplorer::Abi(arch,
                                   ProjectExplorer::Abi::LinuxOS,
                                   ProjectExplorer::Abi::AndroidLinuxFlavor, ProjectExplorer::Abi::ElfFormat,
                                   wordWidth);
    return abi;
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
    case Abi::MipsArchitecture:
        return MipsToolchainPrefix;
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
    case Abi::MipsArchitecture:
        return MipsToolsPrefix;
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
    case Abi::MipsArchitecture:
        return MipsToolsDisplayName;
    default:
        return Unknown;
    }
}

void AndroidConfig::load(const QSettings &settings)
{
    // user settings
    m_partitionSize = settings.value(PartitionSizeKey, 1024).toInt();
    m_sdkLocation = FileName::fromString(settings.value(SDKLocationKey).toString());
    m_ndkLocation = FileName::fromString(settings.value(NDKLocationKey).toString());
    m_antLocation = FileName::fromString(settings.value(AntLocationKey).toString());
    m_useGradle = settings.value(UseGradleKey, false).toBool();
    m_openJDKLocation = FileName::fromString(settings.value(OpenJDKLocationKey).toString());
    m_keystoreLocation = FileName::fromString(settings.value(KeystoreLocationKey).toString());
    m_toolchainHost = settings.value(ToolchainHostKey).toString();
    m_automaticKitCreation = settings.value(AutomaticKitCreationKey, true).toBool();
    QString extraDirectory = settings.value(MakeExtraSearchDirectory).toString();
    m_makeExtraSearchDirectories.clear();
    if (!extraDirectory.isEmpty())
        m_makeExtraSearchDirectories << extraDirectory;

    PersistentSettingsReader reader;
    if (reader.load(FileName::fromString(sdkSettingsFileName()))
            && settings.value(changeTimeStamp).toInt() != QFileInfo(sdkSettingsFileName()).lastModified().toMSecsSinceEpoch() / 1000) {
        // persisten settings
        m_sdkLocation = FileName::fromString(reader.restoreValue(SDKLocationKey, m_sdkLocation.toString()).toString());
        m_ndkLocation = FileName::fromString(reader.restoreValue(NDKLocationKey, m_ndkLocation.toString()).toString());
        m_antLocation = FileName::fromString(reader.restoreValue(AntLocationKey, m_antLocation.toString()).toString());
        m_openJDKLocation = FileName::fromString(reader.restoreValue(OpenJDKLocationKey, m_openJDKLocation.toString()).toString());
        m_keystoreLocation = FileName::fromString(reader.restoreValue(KeystoreLocationKey, m_keystoreLocation.toString()).toString());
        m_toolchainHost = reader.restoreValue(ToolchainHostKey, m_toolchainHost).toString();
        m_automaticKitCreation = reader.restoreValue(AutomaticKitCreationKey, m_automaticKitCreation).toBool();
        QString extraDirectory = reader.restoreValue(MakeExtraSearchDirectory).toString();
        m_makeExtraSearchDirectories.clear();
        if (!extraDirectory.isEmpty())
            m_makeExtraSearchDirectories << extraDirectory;
        // persistent settings
    }
    m_availableSdkPlatformsUpToDate = false;
    m_NdkInformationUpToDate = false;
}

AndroidConfig::AndroidConfig()
    : m_useGradle(false),
      m_availableSdkPlatformsUpToDate(false),
      m_NdkInformationUpToDate(false)
{

}

void AndroidConfig::save(QSettings &settings) const
{
    QFileInfo fileInfo(sdkSettingsFileName());
    if (fileInfo.exists())
        settings.setValue(changeTimeStamp, fileInfo.lastModified().toMSecsSinceEpoch() / 1000);

    // user settings
    settings.setValue(SDKLocationKey, m_sdkLocation.toString());
    settings.setValue(NDKLocationKey, m_ndkLocation.toString());
    settings.setValue(AntLocationKey, m_antLocation.toString());
    settings.setValue(UseGradleKey, m_useGradle);
    settings.setValue(OpenJDKLocationKey, m_openJDKLocation.toString());
    settings.setValue(KeystoreLocationKey, m_keystoreLocation.toString());
    settings.setValue(PartitionSizeKey, m_partitionSize);
    settings.setValue(AutomaticKitCreationKey, m_automaticKitCreation);
    settings.setValue(ToolchainHostKey, m_toolchainHost);
    settings.setValue(MakeExtraSearchDirectory,
                      m_makeExtraSearchDirectories.isEmpty() ? QString()
                                                             : m_makeExtraSearchDirectories.at(0));
}

void AndroidConfig::updateNdkInformation() const
{
    if (m_NdkInformationUpToDate)
        return;
    m_availableNdkPlatforms.clear();
    FileName path = ndkLocation();
    QDirIterator it(path.appendPath(QLatin1String("platforms")).toString(), QStringList() << QLatin1String("android-*"), QDir::Dirs);
    while (it.hasNext()) {
        const QString &fileName = it.next();
        m_availableNdkPlatforms.push_back(fileName.mid(fileName.lastIndexOf(QLatin1Char('-')) + 1).toInt());
    }
    Utils::sort(m_availableNdkPlatforms, std::greater<int>());

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

    path = ndkLocation();
    QDirIterator jt(path.appendPath(QLatin1String("prebuilt")).toString(), hostPatterns, QDir::Dirs);
    if (jt.hasNext()) {
        jt.next();
        m_toolchainHost = jt.fileName();
    }

    m_NdkInformationUpToDate = true;
}

bool AndroidConfig::sortSdkPlatformByApiLevel(const SdkPlatform &a, const SdkPlatform &b)
{
    if (a.apiLevel != b.apiLevel)
        return a.apiLevel > b.apiLevel;
    if (a.name != b.name)
        return a.name < b.name;
    return false;
}

void AndroidConfig::updateAvailableSdkPlatforms() const
{
    if (m_availableSdkPlatformsUpToDate)
        return;
    m_availableSdkPlatforms.clear();

    QProcess proc;
    proc.setProcessEnvironment(androidToolEnvironment().toProcessEnvironment());
    proc.start(androidToolPath().toString(), QStringList() << QLatin1String("list") << QLatin1String("target")); // list avaialbe AVDs
    if (!proc.waitForFinished(10000)) {
        proc.terminate();
        return;
    }

    SdkPlatform platform;
    while (proc.canReadLine()) {
        const QString line = QString::fromLocal8Bit(proc.readLine().trimmed());
        if (line.startsWith(QLatin1String("id:")) && line.contains(QLatin1String("android-"))) {
            int index = line.indexOf(QLatin1String("\"android-"));
            if (index == -1)
                continue;
            QString androidTarget = line.mid(index + 1, line.length() - index - 2);
            const QString tmp = androidTarget.mid(androidTarget.lastIndexOf(QLatin1Char('-')) + 1);
            platform.apiLevel = apiLevelFromAndroidList(tmp);
        } else if (line.startsWith(QLatin1String("Name:"))) {
            platform.name = line.mid(6);
        } else if (line.startsWith(QLatin1String("Tag/ABIs :"))) {
            platform.abis = cleanAndroidABIs(line.mid(10).trimmed().split(QLatin1String(", ")));
        } else if (line.startsWith(QLatin1String("ABIs"))) {
            platform.abis = cleanAndroidABIs(line.mid(6).trimmed().split(QLatin1String(", ")));
        } else if (line.startsWith(QLatin1String("---")) || line.startsWith(QLatin1String("==="))) {
            if (platform.apiLevel == -1)
                continue;
            auto it = qLowerBound(m_availableSdkPlatforms.begin(), m_availableSdkPlatforms.end(),
                                  platform, sortSdkPlatformByApiLevel);
            m_availableSdkPlatforms.insert(it, platform);
            platform = SdkPlatform();
        }
    }

    if (platform.apiLevel != -1) {
        auto it = qLowerBound(m_availableSdkPlatforms.begin(), m_availableSdkPlatforms.end(),
                              platform, sortSdkPlatformByApiLevel);
        m_availableSdkPlatforms.insert(it, platform);
    }

    m_availableSdkPlatformsUpToDate = true;
}

QStringList AndroidConfig::apiLevelNamesFor(const QList<SdkPlatform> &platforms)
{
    return Utils::transform(platforms, AndroidConfig::apiLevelNameFor);
}

QString AndroidConfig::apiLevelNameFor(const SdkPlatform &platform)
{
    return QLatin1String("android-") + QString::number(platform.apiLevel);
}

QList<SdkPlatform> AndroidConfig::sdkTargets(int minApiLevel) const
{
    updateAvailableSdkPlatforms();
    QList<SdkPlatform> result;
    for (int i = 0; i < m_availableSdkPlatforms.size(); ++i) {
        if (m_availableSdkPlatforms.at(i).apiLevel >= minApiLevel)
            result << m_availableSdkPlatforms.at(i);
        else
            break;
    }
    return result;
}

FileName AndroidConfig::adbToolPath() const
{
    FileName path = m_sdkLocation;
    return path.appendPath(QLatin1String("platform-tools/adb" QTC_HOST_EXE_SUFFIX));
}

Environment AndroidConfig::androidToolEnvironment() const
{
    Environment env = Environment::systemEnvironment();
    if (!m_openJDKLocation.isEmpty())
        env.set(QLatin1String("JAVA_HOME"), m_openJDKLocation.toUserOutput());
    return env;
}

FileName AndroidConfig::androidToolPath() const
{
    if (HostOsInfo::isWindowsHost()) {
        // I want to switch from using android.bat to using an executable. All it really does is call
        // Java and I've made some progress on it. So if android.exe exists, return that instead.
        FileName path = m_sdkLocation;
        path.appendPath(QLatin1String("tools/android" QTC_HOST_EXE_SUFFIX));
        if (path.exists())
            return path;
        path = m_sdkLocation;
        return path.appendPath(QLatin1String("tools/android" ANDROID_BAT_SUFFIX));
    } else {
        FileName path = m_sdkLocation;
        return path.appendPath(QLatin1String("tools/android"));
    }
}

FileName AndroidConfig::antToolPath() const
{
    if (!m_antLocation.isEmpty())
        return m_antLocation;
    else
        return FileName::fromLatin1("ant");
}

FileName AndroidConfig::emulatorToolPath() const
{
    FileName path = m_sdkLocation;
    return path.appendPath(QLatin1String("tools/emulator" QTC_HOST_EXE_SUFFIX));
}

FileName AndroidConfig::toolPath(const Abi &abi, const QString &ndkToolChainVersion) const
{
    FileName path = m_ndkLocation;
    return path.appendPath(QString::fromLatin1("toolchains/%1-%2/prebuilt/%3/bin/%4")
            .arg(toolchainPrefix(abi))
            .arg(ndkToolChainVersion)
            .arg(toolchainHost())
            .arg(toolsPrefix(abi)));
}

FileName AndroidConfig::gccPath(const Abi &abi, const QString &ndkToolChainVersion) const
{
    return toolPath(abi, ndkToolChainVersion).appendString(QLatin1String("-gcc" QTC_HOST_EXE_SUFFIX));
}

FileName AndroidConfig::gdbPath(const Abi &abi, const QString &ndkToolChainVersion) const
{
    return toolPath(abi, ndkToolChainVersion).appendString(QLatin1String("-gdb" QTC_HOST_EXE_SUFFIX));
}

FileName AndroidConfig::openJDKBinPath() const
{
    FileName path = m_openJDKLocation;
    if (!path.isEmpty())
        return path.appendPath(QLatin1String("bin"));
    return path;
}

FileName AndroidConfig::keytoolPath() const
{
    return openJDKBinPath().appendPath(keytoolName);
}

QVector<AndroidDeviceInfo> AndroidConfig::connectedDevices(QString *error) const
{
    return connectedDevices(adbToolPath().toString(), error);
}

QVector<AndroidDeviceInfo> AndroidConfig::connectedDevices(const QString &adbToolPath, QString *error)
{
    QVector<AndroidDeviceInfo> devices;
    QProcess adbProc;
    adbProc.start(adbToolPath, QStringList() << QLatin1String("devices"));
    if (!adbProc.waitForFinished(10000)) {
        adbProc.kill();
        if (error)
            *error = QApplication::translate("AndroidConfiguration",
                                             "Could not run: %1")
                .arg(adbToolPath + QLatin1String(" devices"));
        return devices;
    }
    QList<QByteArray> adbDevs = adbProc.readAll().trimmed().split('\n');
    if (adbDevs.empty())
        return devices;

    while (adbDevs.first().startsWith("* daemon"))
        adbDevs.removeFirst(); // remove the daemon logs
    adbDevs.removeFirst(); // remove "List of devices attached" header line

    // workaround for '????????????' serial numbers:
    // can use "adb -d" when only one usb device attached
    foreach (const QByteArray &device, adbDevs) {
        const QString serialNo = QString::fromLatin1(device.left(device.indexOf('\t')).trimmed());
        const QString deviceType = QString::fromLatin1(device.mid(device.indexOf('\t'))).trimmed();
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

        if (dev.type == AndroidDeviceInfo::Emulator)
            dev.avdname = getAvdName(dev.serialNumber);

        devices.push_back(dev);
    }

    Utils::sort(devices, androidDevicesLessThan);
    if (devices.isEmpty() && error)
        *error = QApplication::translate("AndroidConfiguration",
                                         "No devices found in output of: %1")
            .arg(adbToolPath + QLatin1String(" devices"));
    return devices;
}

AndroidConfig::CreateAvdInfo AndroidConfig::gatherCreateAVDInfo(QWidget *parent, int minApiLevel, QString targetArch) const
{
    CreateAvdInfo result;
    AvdDialog d(minApiLevel, targetArch, this, parent);
    if (d.exec() != QDialog::Accepted || !d.isValid())
        return result;

    result.target = d.target();
    result.name = d.name();
    result.abi = d.abi();
    result.sdcardSize = d.sdcardSize();
    return result;
}

QFuture<AndroidConfig::CreateAvdInfo> AndroidConfig::createAVD(CreateAvdInfo info) const
{
    return QtConcurrent::run(&AndroidConfig::createAVDImpl, info, androidToolPath(), androidToolEnvironment());
}

AndroidConfig::CreateAvdInfo AndroidConfig::createAVDImpl(CreateAvdInfo info, FileName androidToolPath, Environment env)
{
    QProcess proc;
    proc.setProcessEnvironment(env.toProcessEnvironment());
    QStringList arguments;
    arguments << QLatin1String("create") << QLatin1String("avd")
              << QLatin1String("-t") << info.target
              << QLatin1String("-n") << info.name
              << QLatin1String("-b") << info.abi;
    if (info.sdcardSize > 0)
        arguments << QLatin1String("-c") << QString::fromLatin1("%1M").arg(info.sdcardSize);
    proc.start(androidToolPath.toString(), arguments);
    if (!proc.waitForStarted()) {
        info.error = QApplication::translate("AndroidConfig", "Could not start process \"%1 %2\"")
                .arg(androidToolPath.toString(), arguments.join(QLatin1Char(' ')));
        return info;
    }

    proc.write(QByteArray("yes\n")); // yes to "Do you wish to create a custom hardware profile"

    QByteArray question;
    while (true) {
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

        if (proc.state() != QProcess::Running)
            break;
    }

    proc.waitForFinished();

    QString errorOutput = QString::fromLocal8Bit(proc.readAllStandardError());
    // The exit code is always 0, so we need to check stderr
    // For now assume that any output at all indicates a error
    if (!errorOutput.isEmpty()) {
        info.error = errorOutput;
    }

    return info;
}

bool AndroidConfig::removeAVD(const QString &name) const
{
    QProcess proc;
    proc.setProcessEnvironment(androidToolEnvironment().toProcessEnvironment());
    proc.start(androidToolPath().toString(),
               QStringList() << QLatin1String("delete") << QLatin1String("avd")
               << QLatin1String("-n") << name);
    if (!proc.waitForFinished(5000)) {
        proc.terminate();
        return false;
    }
    return !proc.exitCode();
}

QFuture<QVector<AndroidDeviceInfo>> AndroidConfig::androidVirtualDevicesFuture()
{
    return QtConcurrent::run(&AndroidConfig::androidVirtualDevices, androidToolPath().toString(), androidToolEnvironment());
}

QVector<AndroidDeviceInfo> AndroidConfig::androidVirtualDevices(const QString &androidTool, const Environment &environment)
{
    QVector<AndroidDeviceInfo> devices;
    QProcess proc;
    proc.setProcessEnvironment(environment.toProcessEnvironment());
    proc.start(androidTool,
               QStringList() << QLatin1String("list") << QLatin1String("avd")); // list available AVDs
    if (!proc.waitForFinished(20000)) {
        proc.terminate();
        return devices;
    }
    QList<QByteArray> avds = proc.readAll().trimmed().split('\n');
    if (avds.empty())
        return devices;

    while (avds.first().startsWith("* daemon"))
        avds.removeFirst(); // remove the daemon logs
    avds.removeFirst(); // remove "List of devices attached" header line

    bool nextLineIsTargetLine = false;

    AndroidDeviceInfo dev;
    for (int i = 0; i < avds.size(); i++) {
        QString line = QLatin1String(avds.at(i));
        if (!line.contains(QLatin1String("Name:")))
            continue;

        int index = line.indexOf(QLatin1Char(':')) + 2;
        if (index >= line.size())
            break;
        dev.avdname = line.mid(index).trimmed();
        dev.sdk = -1;
        dev.cpuAbi.clear();
        ++i;
        for (; i < avds.size(); ++i) {
            line = QLatin1String(avds[i]);
            if (line.contains(QLatin1String("---------")))
                break;

            if (line.contains(QLatin1String("Target:")) || nextLineIsTargetLine) {
                if (line.contains(QLatin1String("Google APIs"))) {
                    nextLineIsTargetLine = true;
                    continue;
                }

                nextLineIsTargetLine = false;

                int lastIndex = line.lastIndexOf(QLatin1Char(' '));
                if (lastIndex == -1) // skip line
                    break;
                QString tmp = line.mid(lastIndex).remove(QLatin1Char(')')).trimmed();
                dev.sdk = apiLevelFromAndroidList(tmp);
            }
            if (line.contains(QLatin1String("Tag/ABI:"))) {
                int lastIndex = line.lastIndexOf(QLatin1Char('/')) + 1;
                if (lastIndex >= line.size())
                    break;
                dev.cpuAbi = QStringList() << line.mid(lastIndex);
            } else if (line.contains(QLatin1String("ABI:"))) {
                int lastIndex = line.lastIndexOf(QLatin1Char(' ')) + 1;
                if (lastIndex >= line.size())
                    break;
                dev.cpuAbi = QStringList() << line.mid(lastIndex).trimmed();
            }
        }
        // armeabi-v7a devices can also run armeabi code
        if (dev.cpuAbi == QStringList(QLatin1String("armeabi-v7a")))
            dev.cpuAbi << QLatin1String("armeabi");
        dev.state = AndroidDeviceInfo::OkState;
        dev.type = AndroidDeviceInfo::Emulator;
        if (dev.cpuAbi.isEmpty() || dev.sdk == -1)
            continue;
        devices.push_back(dev);
    }
    Utils::sort(devices, androidDevicesLessThan);

    return devices;
}

QString AndroidConfig::startAVD(const QString &name) const
{
    if (!findAvd(name).isEmpty() || startAVDAsync(name))
        return waitForAvd(name);
    return QString();
}

bool AndroidConfig::startAVDAsync(const QString &avdName) const
{
    QProcess *avdProcess = new QProcess();
    avdProcess->connect(avdProcess, SIGNAL(finished(int)), avdProcess, SLOT(deleteLater()));

    // start the emulator
    QStringList arguments;
    if (AndroidConfigurations::force32bitEmulator())
        arguments << QLatin1String("-force-32bit");

    arguments << QLatin1String("-partition-size") << QString::number(partitionSize())
              << QLatin1String("-avd") << avdName;
    avdProcess->start(emulatorToolPath().toString(), arguments);
    if (!avdProcess->waitForStarted(-1)) {
        delete avdProcess;
        return false;
    }
    return true;
}

QString AndroidConfig::findAvd(const QString &avdName) const
{
    QVector<AndroidDeviceInfo> devices = connectedDevices();
    foreach (AndroidDeviceInfo device, devices) {
        if (device.type != AndroidDeviceInfo::Emulator)
            continue;
        if (device.avdname == avdName)
            return device.serialNumber;
    }
    return QString();
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

bool AndroidConfig::waitForBooted(const QString &serialNumber, const QFutureInterface<bool> &fi) const
{
    // found a serial number, now wait until it's done booting...
    for (int i = 0; i < 60; ++i) {
        if (fi.isCanceled())
            return false;
        if (hasFinishedBooting(serialNumber)) {
            return true;
        } else {
            Utils::sleep(2000);
            if (!isConnected(serialNumber)) // device was disconnected
                return false;
        }
    }
    return false;
}

QString AndroidConfig::waitForAvd(const QString &avdName, const QFutureInterface<bool> &fi) const
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
        Utils::sleep(2000);
    }
    return QString();
}

bool AndroidConfig::isBootToQt(const QString &device) const
{
    return isBootToQt(adbToolPath().toString(), device);
}

bool AndroidConfig::isBootToQt(const QString &adbToolPath, const QString &device)
{
    // workaround for '????????????' serial numbers
    QStringList arguments = AndroidDeviceInfo::adbSelector(device);
    arguments << QLatin1String("shell")
              << QLatin1String("ls -l /system/bin/appcontroller || ls -l /usr/bin/appcontroller && echo Boot2Qt");

    QProcess adbProc;
    adbProc.start(adbToolPath, arguments);
    if (!adbProc.waitForFinished(10000)) {
        adbProc.kill();
        return false;
    }
    return adbProc.readAll().contains("Boot2Qt");
}


QString AndroidConfig::getDeviceProperty(const QString &adbToolPath, const QString &device, const QString &property)
{
    // workaround for '????????????' serial numbers
    QStringList arguments = AndroidDeviceInfo::adbSelector(device);
    arguments << QLatin1String("shell") << QLatin1String("getprop")
              << property;

    QProcess adbProc;
    adbProc.start(adbToolPath, arguments);
    if (!adbProc.waitForFinished(10000)) {
        adbProc.kill();
        return QString();
    }

    return QString::fromLocal8Bit(adbProc.readAll());
}

int AndroidConfig::getSDKVersion(const QString &device) const
{
    return getSDKVersion(adbToolPath().toString(), device);
}

int AndroidConfig::getSDKVersion(const QString &adbToolPath, const QString &device)
{
    QString tmp = getDeviceProperty(adbToolPath, device, QLatin1String("ro.build.version.sdk"));
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
    int port = serialnumber.mid(index + 1).toInt(&ok);
    if (!ok)
        return QString();

    QByteArray avdName = "avd name\n";

    QTcpSocket tcpSocket;
    tcpSocket.connectToHost(QHostAddress(QHostAddress::LocalHost), port);
    tcpSocket.waitForConnected();
    tcpSocket.write(avdName + "exit\n");
    tcpSocket.waitForDisconnected();

    QByteArray response = tcpSocket.readAll();
    int start = response.indexOf("OK\r\n");
    if (start == -1)
        return QString();
    start = start + 4;

    int end = response.indexOf("\r\n", start);
    if (end == -1)
        return QString();
    return QString::fromLatin1(response.mid(start, end - start));
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

    QString model = getDeviceProperty(adbToolPath().toString(), device, QLatin1String("ro.product.model")).trimmed();
    if (model.isEmpty())
        return device;

    if (!device.startsWith(QLatin1String("????")))
        m_serialNumberToDeviceName.insert(device, model);
    return model;
}

bool AndroidConfig::hasFinishedBooting(const QString &device) const
{
    QStringList arguments = AndroidDeviceInfo::adbSelector(device);
    arguments << QLatin1String("shell") << QLatin1String("getprop")
              << QLatin1String("init.svc.bootanim");

    QProcess adbProc;
    adbProc.start(adbToolPath().toString(), arguments);
    if (!adbProc.waitForFinished(10000)) {
        adbProc.kill();
        return false;
    }
    QString value = QString::fromLocal8Bit(adbProc.readAll().trimmed());
    if (value == QLatin1String("stopped"))
        return true;
    return false;
}

QStringList AndroidConfig::getAbis(const QString &device) const
{
    return getAbis(adbToolPath().toString(), device);
}

QStringList AndroidConfig::getAbis(const QString &adbToolPath, const QString &device)
{
    QStringList result;
    // First try via ro.product.cpu.abilist
    QStringList arguments = AndroidDeviceInfo::adbSelector(device);
    arguments << QLatin1String("shell") << QLatin1String("getprop");
    arguments << QLatin1String("ro.product.cpu.abilist");
    QProcess adbProc;
    adbProc.start(adbToolPath, arguments);
    if (!adbProc.waitForFinished(10000)) {
        adbProc.kill();
        return result;
    }
    QString output = QString::fromLocal8Bit(adbProc.readAll().trimmed());
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

        QProcess adbProc;
        adbProc.start(adbToolPath, arguments);
        if (!adbProc.waitForFinished(10000)) {
            adbProc.kill();
            return result;
        }
        QString abi = QString::fromLocal8Bit(adbProc.readAll().trimmed());
        if (abi.isEmpty())
            break;
        result << abi;
    }
    return result;
}

SdkPlatform AndroidConfig::highestAndroidSdk() const
{
    updateAvailableSdkPlatforms();
    if (m_availableSdkPlatforms.isEmpty())
        return SdkPlatform();
    return m_availableSdkPlatforms.first();
}

QString AndroidConfig::bestNdkPlatformMatch(int target) const
{
    target = std::max(9, target);
    updateNdkInformation();
    foreach (int apiLevel, m_availableNdkPlatforms) {
        if (apiLevel <= target)
            return QString::fromLatin1("android-%1").arg(apiLevel);
    }
    return QLatin1String("android-9");
}

FileName AndroidConfig::sdkLocation() const
{
    return m_sdkLocation;
}

void AndroidConfig::setSdkLocation(const FileName &sdkLocation)
{
    m_sdkLocation = sdkLocation;
    m_availableSdkPlatformsUpToDate = false;
}

FileName AndroidConfig::ndkLocation() const
{
    return m_ndkLocation;
}

void AndroidConfig::setNdkLocation(const FileName &ndkLocation)
{
    m_ndkLocation = ndkLocation;
    m_NdkInformationUpToDate = false;
}

FileName AndroidConfig::antLocation() const
{
    return m_antLocation;
}

void AndroidConfig::setAntLocation(const FileName &antLocation)
{
    m_antLocation = antLocation;
}

FileName AndroidConfig::openJDKLocation() const
{
    return m_openJDKLocation;
}

void AndroidConfig::setOpenJDKLocation(const FileName &openJDKLocation)
{
    m_openJDKLocation = openJDKLocation;
    m_availableSdkPlatformsUpToDate = false;
}

FileName AndroidConfig::keystoreLocation() const
{
    return m_keystoreLocation;
}

void AndroidConfig::setKeystoreLocation(const FileName &keystoreLocation)
{
    m_keystoreLocation = keystoreLocation;
}

QString AndroidConfig::toolchainHost() const
{
    updateNdkInformation();
    return m_toolchainHost;
}

QStringList AndroidConfig::makeExtraSearchDirectories() const
{
    return m_makeExtraSearchDirectories;
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

bool AndroidConfig::useGrandle() const
{
    return m_useGradle;
}

void AndroidConfig::setUseGradle(bool b)
{
    m_useGradle = b;
}

///////////////////////////////////
// AndroidConfigurations
///////////////////////////////////
void AndroidConfigurations::setConfig(const AndroidConfig &devConfigs)
{
    m_instance->m_config = devConfigs;

    m_instance->save();
    m_instance->updateAndroidDevice();
    m_instance->registerNewToolChains();
    m_instance->updateAutomaticKitList();
    m_instance->removeOldToolChains();
    emit m_instance->updated();
}

AndroidDeviceInfo AndroidConfigurations::showDeviceDialog(Project *project,
                                                          int apiLevel, const QString &abi,
                                                          Options options)
{
    QString serialNumber = defaultDevice(project, abi);
    AndroidDeviceDialog dialog(apiLevel, abi, options, serialNumber, Core::ICore::mainWindow());
    if (dialog.exec() == QDialog::Accepted) {
        AndroidDeviceInfo info = dialog.device();
        if (dialog.saveDeviceSelection()) {
            const QString serialNumber = info.type == AndroidDeviceInfo::Hardware ?
                        info.serialNumber : info.avdname;
            if (!serialNumber.isEmpty())
                AndroidConfigurations::setDefaultDevice(project, abi, serialNumber);
        }
        return info;
    }
    return AndroidDeviceInfo();
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

static bool equalKits(Kit *a, Kit *b)
{
    if (QtSupport::QtKitInformation::qtVersion(a) != QtSupport::QtKitInformation::qtVersion(b))
        return false;
    ToolChain *atc = ToolChainKitInformation::toolChain(a);
    ToolChain *btc = ToolChainKitInformation::toolChain(b);
    if (atc == btc)
        return true;
    if (!atc || atc->typeId() != Constants::ANDROID_TOOLCHAIN_ID)
        return false;
    if (!btc || btc->typeId() != Constants::ANDROID_TOOLCHAIN_ID)
        return false;
    AndroidToolChain *aatc = static_cast<AndroidToolChain *>(atc);
    AndroidToolChain *bbtc = static_cast<AndroidToolChain *>(btc);
    if (aatc->ndkToolChainVersion() == bbtc->ndkToolChainVersion()
            && aatc->targetAbi() == bbtc->targetAbi())
        return true;
    return false;
}

void AndroidConfigurations::registerNewToolChains()
{
    QList<ToolChain *> existingToolChains = ToolChainManager::toolChains();
    QList<ToolChain *> toolchains = AndroidToolChainFactory::createToolChainsForNdk(AndroidConfigurations::currentConfig().ndkLocation());
    foreach (ToolChain *tc, toolchains) {
        bool found = false;
        for (int i = 0; i < existingToolChains.count(); ++i) {
            if (*(existingToolChains.at(i)) == *tc) {
                found = true;
                break;
            }
        }
        if (found)
            delete tc;
        else
            ToolChainManager::registerToolChain(tc);
    }
}

void AndroidConfigurations::removeOldToolChains()
{
    foreach (ToolChain *tc, ToolChainManager::toolChains()) {
        if (tc->typeId() == Constants::ANDROID_TOOLCHAIN_ID) {
            if (!tc->isValid())
                ToolChainManager::deregisterToolChain(tc);
        }
    }
}

void AndroidConfigurations::updateAutomaticKitList()
{
    QList<AndroidToolChain *> toolchains;
    if (AndroidConfigurations::currentConfig().automaticKitCreation()) {
        // having a empty toolchains list will remove all autodetected kits for android
        // exactly what we want in that case
        foreach (ToolChain *tc, ToolChainManager::toolChains()) {
            if (!tc->isAutoDetected())
                continue;
            if (tc->typeId() != Constants::ANDROID_TOOLCHAIN_ID)
                continue;
            if (!tc->isValid()) // going to be deleted
                continue;
            toolchains << static_cast<AndroidToolChain *>(tc);
        }
    }

    QList<Kit *> existingKits;

    foreach (Kit *k, KitManager::kits()) {
        if (DeviceTypeKitInformation::deviceTypeId(k) != Core::Id(Constants::ANDROID_DEVICE_TYPE))
            continue;
        if (!k->isAutoDetected())
            continue;
        if (k->isSdkProvided())
            continue;

        // Update code for 3.0 beta, which shipped with a bug for the debugger settings
        ToolChain *tc =ToolChainKitInformation::toolChain(k);
        if (tc && Debugger::DebuggerKitInformation::debuggerCommand(k) != tc->suggestedDebugger()) {
            Debugger::DebuggerItem debugger;
            debugger.setCommand(tc->suggestedDebugger());
            debugger.setEngineType(Debugger::GdbEngineType);
            debugger.setUnexpandedDisplayName(tr("Android Debugger for %1").arg(tc->displayName()));
            debugger.setAutoDetected(true);
            debugger.setAbi(tc->targetAbi());
            QVariant id = Debugger::DebuggerItemManager::registerDebugger(debugger);
            Debugger::DebuggerKitInformation::setDebugger(k, id);
        }
        existingKits << k;
    }

    QHash<Abi, QList<QtSupport::BaseQtVersion *> > qtVersionsForArch;
    foreach (QtSupport::BaseQtVersion *qtVersion, QtSupport::QtVersionManager::unsortedVersions()) {
        if (qtVersion->type() != QLatin1String(Constants::ANDROIDQT))
            continue;
        QList<Abi> qtAbis = qtVersion->qtAbis();
        if (qtAbis.empty())
            continue;
        qtVersionsForArch[qtAbis.first()].append(qtVersion);
    }

    DeviceManager *dm = DeviceManager::instance();
    IDevice::ConstPtr device = dm->find(Core::Id(Constants::ANDROID_DEVICE_ID));
    if (device.isNull()) {
        // no device, means no sdk path
        foreach (Kit *k, existingKits)
            KitManager::deregisterKit(k);
        return;
    }

    // register new kits
    QList<Kit *> newKits;
    foreach (AndroidToolChain *tc, toolchains) {
        if (tc->isSecondaryToolChain())
            continue;
        QList<QtSupport::BaseQtVersion *> qtVersions = qtVersionsForArch.value(tc->targetAbi());
        foreach (QtSupport::BaseQtVersion *qt, qtVersions) {
            Kit *newKit = new Kit;
            newKit->setAutoDetected(true);
            newKit->setIconPath(FileName::fromString(QLatin1String(Constants::ANDROID_SETTINGS_CATEGORY_ICON)));
            DeviceTypeKitInformation::setDeviceTypeId(newKit, Core::Id(Constants::ANDROID_DEVICE_TYPE));
            ToolChainKitInformation::setToolChain(newKit, tc);
            QtSupport::QtKitInformation::setQtVersion(newKit, qt);
            DeviceKitInformation::setDevice(newKit, device);

            Debugger::DebuggerItem debugger;
            debugger.setCommand(tc->suggestedDebugger());
            debugger.setEngineType(Debugger::GdbEngineType);
            debugger.setUnexpandedDisplayName(tr("Android Debugger for %1").arg(tc->displayName()));
            debugger.setAutoDetected(true);
            debugger.setAbi(tc->targetAbi());
            QVariant id = Debugger::DebuggerItemManager::registerDebugger(debugger);
            Debugger::DebuggerKitInformation::setDebugger(newKit, id);

            AndroidGdbServerKitInformation::setGdbSever(newKit, tc->suggestedGdbServer());
            newKit->makeSticky();
            newKits << newKit;
        }
    }

    for (int i = existingKits.count() - 1; i >= 0; --i) {
        Kit *existingKit = existingKits.at(i);
        for (int j = 0; j < newKits.count(); ++j) {
            Kit *newKit = newKits.at(j);
            if (equalKits(existingKit, newKit)) {
                // Kit is already registered, nothing to do
                newKits.removeAt(j);
                existingKits.at(i)->makeSticky();
                existingKits.removeAt(i);
                ToolChainKitInformation::setToolChain(existingKit, ToolChainKitInformation::toolChain(newKit));
                KitManager::deleteKit(newKit);
                j = newKits.count();
            }
        }
    }

    foreach (Kit *k, existingKits) {
        ToolChain *tc = ToolChainKitInformation::toolChain(k);
        QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(k);
        if (tc && tc->typeId() == Constants::ANDROID_TOOLCHAIN_ID
                && tc->isValid()
                && qtVersion && qtVersion->type() == QLatin1String(Constants::ANDROIDQT)) {
            k->makeUnSticky();
            k->setAutoDetected(false);
        } else {
            KitManager::deregisterKit(k);
        }
    }

    foreach (Kit *kit, newKits) {
        AndroidToolChain *tc = static_cast<AndroidToolChain *>(ToolChainKitInformation::toolChain(kit));
        AndroidQtVersion *qt = static_cast<AndroidQtVersion *>(QtSupport::QtKitInformation::qtVersion(kit));
        kit->setUnexpandedDisplayName(tr("Android for %1 (GCC %2, Qt %3)")
                            .arg(qt->targetArch())
                            .arg(tc->ndkToolChainVersion())
                            .arg(qt->qtVersionString()));
        KitManager::registerKit(kit);
    }
}

bool AndroidConfigurations::force32bitEmulator()
{
    return m_instance->m_force32bit;
}

/**
 * Workaround for '????????????' serial numbers
 * @return ("-d") for buggy devices, ("-s", <serial no>) for normal
 */
QStringList AndroidDeviceInfo::adbSelector(const QString &serialNumber)
{
    if (serialNumber.startsWith(QLatin1String("????")))
        return QStringList() << QLatin1String("-d");
    return QStringList() << QLatin1String("-s") << serialNumber;
}

const AndroidConfig &AndroidConfigurations::currentConfig()
{
    return m_instance->m_config; // ensure that m_instance is initialized
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

AndroidConfigurations::AndroidConfigurations(QObject *parent)
    : QObject(parent)
{
    load();

    connect(ProjectExplorer::SessionManager::instance(), SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            this, SLOT(clearDefaultDevices(ProjectExplorer::Project*)));

    m_force32bit = is32BitUserSpace();

    m_instance = this;
}

static FileName javaHomeForJavac(const FileName &location)
{
    QFileInfo fileInfo = location.toFileInfo();
    int tries = 5;
    while (tries > 0) {
        QDir dir = fileInfo.dir();
        dir.cdUp();
        if (QFileInfo::exists(dir.filePath(QLatin1String("lib/tools.jar"))))
            return FileName::fromString(dir.path());
        if (fileInfo.isSymLink())
            fileInfo.setFile(fileInfo.symLinkTarget());
        else
            break;
        --tries;
    }
    return FileName();
}

void AndroidConfigurations::load()
{
    bool saveSettings = false;
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    m_config.load(*settings);

    if (m_config.antLocation().isEmpty()) {
        Environment env = Environment::systemEnvironment();
        FileName location = env.searchInPath(QLatin1String("ant"));
        QFileInfo fi = location.toFileInfo();
        if (fi.exists() && fi.isExecutable() && !fi.isDir()) {
            m_config.setAntLocation(location);
            saveSettings = true;
        }
    }

    if (m_config.openJDKLocation().isEmpty()) {
        if (HostOsInfo::isLinuxHost()) {
            Environment env = Environment::systemEnvironment();
            FileName location = env.searchInPath(QLatin1String("javac"));
            QFileInfo fi = location.toFileInfo();
            if (fi.exists() && fi.isExecutable() && !fi.isDir()) {
                m_config.setOpenJDKLocation(javaHomeForJavac(location));
                saveSettings = true;
            }
        } else if (HostOsInfo::isMacHost()) {
            QString javaHome = QLatin1String("/System/Library/Frameworks/JavaVM.framework/Versions/CurrentJDK/Home");
            if (QFileInfo::exists(javaHome))
                m_config.setOpenJDKLocation(FileName::fromString(javaHome));
        } else if (HostOsInfo::isWindowsHost()) {
            QSettings settings(QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Javasoft\\Java Development Kit"), QSettings::NativeFormat);
            QStringList allVersions = settings.childGroups();
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
                    settings.beginGroup(version);
                    QString tmpJavaHome = settings.value(QLatin1String("JavaHome")).toString();
                    settings.endGroup();
                    if (!QFileInfo::exists(tmpJavaHome))
                        continue;

                    major = tmpMajor;
                    minor = tmpMinor;
                    javaHome = tmpJavaHome;
                }
            }
            if (!javaHome.isEmpty()) {
                m_config.setOpenJDKLocation(FileName::fromString(javaHome));
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
        devMgr->addDevice(IDevice::Ptr(new AndroidDevice));
    else if (devMgr->find(Constants::ANDROID_DEVICE_ID))
        devMgr->removeDevice(Core::Id(Constants::ANDROID_DEVICE_ID));
}

AndroidConfigurations *AndroidConfigurations::m_instance = 0;

} // namespace Android
