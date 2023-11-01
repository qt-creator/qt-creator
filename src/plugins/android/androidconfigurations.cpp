// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androiddevice.h"
#include "androidmanager.h"
#include "androidqtversion.h"
#include "androidtoolchain.h"
#include "androidtr.h"
#include "avddialog.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/toolchainmanager.h>

#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggerkitaspect.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtversionmanager.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/persistentsettings.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>
#include <utils/stringutils.h>

#include <QApplication>
#include <QDirIterator>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QTcpSocket>

#include <functional>
#include <memory>

#ifdef WITH_TESTS
#   include <QTest>
#   include "androidplugin.h"
#endif // WITH_TESTS

using namespace QtSupport;
using namespace ProjectExplorer;
using namespace Utils;

namespace {
static Q_LOGGING_CATEGORY(avdConfigLog, "qtc.android.androidconfig", QtWarningMsg)
}

namespace Android {
using namespace Internal;

#ifdef Q_OS_WIN32
#define ANDROID_BAT_SUFFIX ".bat"
#else
#define ANDROID_BAT_SUFFIX ""
#endif

const char JsonFilePath[] = "android/sdk_definitions.json";
const char SdkToolsUrlKey[] = "sdk_tools_url";
const char CommonKey[] = "common";
const char SdkEssentialPkgsKey[] = "sdk_essential_packages";
const char VersionsKey[] = "versions";
const char NdksSubDir[] = "ndk/";
const char SpecificQtVersionsKey[] = "specific_qt_versions";
const char DefaultVersionKey[] = "default";
const char LinuxOsKey[] = "linux";
const char WindowsOsKey[] = "windows";
const char macOsKey[] = "mac";

const char SettingsGroup[] = "AndroidConfigurations";
const char SDKLocationKey[] = "SDKLocation";
const char CustomNdkLocationsKey[] = "CustomNdkLocations";
const char DefaultNdkLocationKey[] = "DefaultNdkLocation";
const char SdkFullyConfiguredKey[] = "AllEssentialsInstalled";
const char SDKManagerToolArgsKey[] = "SDKManagerToolArgs";
const char OpenJDKLocationKey[] = "OpenJDKLocation";
const char OpenSslPriLocationKey[] = "OpenSSLPriLocation";
const char AutomaticKitCreationKey[] = "AutomatiKitCreation";
const char EmulatorArgsKey[] = "EmulatorArgs";

const QLatin1String ArmToolchainPrefix("arm-linux-androideabi");
const QLatin1String X86ToolchainPrefix("x86");
const QLatin1String AArch64ToolchainPrefix("aarch64-linux-android");
const QLatin1String X86_64ToolchainPrefix("x86_64");

const QLatin1String ArmToolsPrefix ("arm-linux-androideabi");
const QLatin1String X86ToolsPrefix("i686-linux-android");
const QLatin1String AArch64ToolsPrefix("aarch64-linux-android");
const QLatin1String X86_64ToolsPrefix("x86_64-linux-android");

const QLatin1String Unknown("unknown");
const QLatin1String keytoolName("keytool");
const Key changeTimeStamp("ChangeTimeStamp");

const char sdkToolsVersionKey[] = "Pkg.Revision";
const char ndkRevisionKey[] = "Pkg.Revision";

static QString sdkSettingsFileName()
{
    return Core::ICore::installerResourcePath("android.xml").toString();
}

static QString ndkPackageMarker()
{
    return QLatin1String(Constants::ndkPackageName) + ";";
}


//////////////////////////////////
// AndroidConfig
//////////////////////////////////

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
            return QLatin1String(Constants::AArch64ToolsDisplayName);
        return QLatin1String(Constants::ArmToolsDisplayName);
    case Abi::X86Architecture:
        if (abi.wordWidth() == 64)
            return QLatin1String(Constants::X86_64ToolsDisplayName);
        return QLatin1String(Constants::X86ToolsDisplayName);
    default:
        return Unknown;
    }
}

void AndroidConfig::load(const QtcSettings &settings)
{
    // user settings
    QVariant emulatorArgs = settings.value(EmulatorArgsKey, QString("-netdelay none -netspeed full"));
    if (emulatorArgs.typeId() == QVariant::StringList) // Changed in 8.0 from QStringList to QString.
        emulatorArgs = ProcessArgs::joinArgs(emulatorArgs.toStringList());
    m_emulatorArgs = emulatorArgs.toString();
    m_sdkLocation = FilePath::fromUserInput(settings.value(SDKLocationKey).toString()).cleanPath();
    m_customNdkList = settings.value(CustomNdkLocationsKey).toStringList();
    m_defaultNdk =
            FilePath::fromUserInput(settings.value(DefaultNdkLocationKey).toString()).cleanPath();
    m_sdkManagerToolArgs = settings.value(SDKManagerToolArgsKey).toStringList();
    m_openJDKLocation = FilePath::fromString(settings.value(OpenJDKLocationKey).toString());
    m_openSslLocation = FilePath::fromString(settings.value(OpenSslPriLocationKey).toString());
    m_automaticKitCreation = settings.value(AutomaticKitCreationKey, true).toBool();
    m_sdkFullyConfigured = settings.value(SdkFullyConfiguredKey, false).toBool();

    PersistentSettingsReader reader;
    if (reader.load(FilePath::fromString(sdkSettingsFileName()))
            && settings.value(changeTimeStamp).toInt() != QFileInfo(sdkSettingsFileName()).lastModified().toMSecsSinceEpoch() / 1000) {
        // persisten settings
        m_sdkLocation = FilePath::fromUserInput(reader.restoreValue(SDKLocationKey, m_sdkLocation.toString()).toString()).cleanPath();
        m_customNdkList = reader.restoreValue(CustomNdkLocationsKey).toStringList();
        m_sdkManagerToolArgs = reader.restoreValue(SDKManagerToolArgsKey, m_sdkManagerToolArgs).toStringList();
        m_openJDKLocation = FilePath::fromString(reader.restoreValue(OpenJDKLocationKey, m_openJDKLocation.toString()).toString());
        m_openSslLocation = FilePath::fromString(reader.restoreValue(OpenSslPriLocationKey, m_openSslLocation.toString()).toString());
        m_automaticKitCreation = reader.restoreValue(AutomaticKitCreationKey, m_automaticKitCreation).toBool();
        m_sdkFullyConfigured = reader.restoreValue(SdkFullyConfiguredKey, m_sdkFullyConfigured).toBool();
        // persistent settings
    }
    m_customNdkList.removeAll("");
    if (!m_defaultNdk.isEmpty() && ndkVersion(m_defaultNdk).isNull()) {
        if (avdConfigLog().isDebugEnabled())
            qCDebug(avdConfigLog).noquote() << "Clearing invalid default NDK setting:"
                                            << m_defaultNdk.toUserOutput();
        m_defaultNdk.clear();
    }
    parseDependenciesJson();
}

void AndroidConfig::save(QtcSettings &settings) const
{
    QFileInfo fileInfo(sdkSettingsFileName());
    if (fileInfo.exists())
        settings.setValue(changeTimeStamp, fileInfo.lastModified().toMSecsSinceEpoch() / 1000);

    // user settings
    settings.setValue(SDKLocationKey, m_sdkLocation.toString());
    settings.setValue(CustomNdkLocationsKey, m_customNdkList);
    settings.setValue(DefaultNdkLocationKey, m_defaultNdk.toString());
    settings.setValue(SDKManagerToolArgsKey, m_sdkManagerToolArgs);
    settings.setValue(OpenJDKLocationKey, m_openJDKLocation.toString());
    settings.setValue(OpenSslPriLocationKey, m_openSslLocation.toString());
    settings.setValue(EmulatorArgsKey, m_emulatorArgs);
    settings.setValue(AutomaticKitCreationKey, m_automaticKitCreation);
    settings.setValue(SdkFullyConfiguredKey, m_sdkFullyConfigured);
}

void AndroidConfig::parseDependenciesJson()
{
    const FilePath sdkConfigUserFile = Core::ICore::userResourcePath(JsonFilePath);
    const FilePath sdkConfigFile = Core::ICore::resourcePath(JsonFilePath);

    if (!sdkConfigUserFile.exists()) {
        sdkConfigUserFile.absolutePath().ensureWritableDir();
        sdkConfigFile.copyFile(sdkConfigUserFile);
    }

    if (sdkConfigFile.lastModified() > sdkConfigUserFile.lastModified()) {
        const FilePath oldUserFile = sdkConfigUserFile.stringAppended(".old");
        oldUserFile.removeFile();
        sdkConfigUserFile.renameFile(oldUserFile);
        sdkConfigFile.copyFile(sdkConfigUserFile);
    }

    QFile jsonFile(sdkConfigUserFile.toString());
    if (!jsonFile.open(QIODevice::ReadOnly)) {
        qCDebug(avdConfigLog, "Couldn't open JSON config file %s.", qPrintable(jsonFile.fileName()));
        return;
    }

    QJsonObject jsonObject = QJsonDocument::fromJson(jsonFile.readAll()).object();

    if (jsonObject.contains(CommonKey) && jsonObject[CommonKey].isObject()) {
        QJsonObject commonObject = jsonObject[CommonKey].toObject();
        // Parse SDK Tools URL
        if (commonObject.contains(SdkToolsUrlKey) && commonObject[SdkToolsUrlKey].isObject()) {
            QJsonObject sdkToolsObj(commonObject[SdkToolsUrlKey].toObject());
            if (HostOsInfo::isMacHost()) {
                m_sdkToolsUrl = sdkToolsObj[macOsKey].toString();
                m_sdkToolsSha256 = QByteArray::fromHex(sdkToolsObj["mac_sha256"].toString().toUtf8());
            } else if (HostOsInfo::isWindowsHost()) {
                m_sdkToolsUrl = sdkToolsObj[WindowsOsKey].toString();
                m_sdkToolsSha256 = QByteArray::fromHex(sdkToolsObj["windows_sha256"].toString().toUtf8());
            } else {
                m_sdkToolsUrl = sdkToolsObj[LinuxOsKey].toString();
                m_sdkToolsSha256 = QByteArray::fromHex(sdkToolsObj["linux_sha256"].toString().toUtf8());
            }
        }

        // Parse common essential packages
        auto appendEssentialsFromArray = [this](QJsonArray array) {
            for (const QJsonValueRef &pkg : array)
                m_commonEssentialPkgs.append(pkg.toString());
        };

        QJsonObject commonEssentials = commonObject[SdkEssentialPkgsKey].toObject();
        appendEssentialsFromArray(commonEssentials[DefaultVersionKey].toArray());

        if (HostOsInfo::isWindowsHost())
            appendEssentialsFromArray(commonEssentials[WindowsOsKey].toArray());
        else if (HostOsInfo::isMacHost())
            appendEssentialsFromArray(commonEssentials[macOsKey].toArray());
        else
            appendEssentialsFromArray(commonEssentials[LinuxOsKey].toArray());
    }

    auto fillQtVersionsRange = [](const QString &shortVersion) {
        QList<QVersionNumber> versions;
        static const QRegularExpression re(R"(([0-9]\.[0-9]+\.)\[([0-9]+)\-([0-9]+)\])");
        QRegularExpressionMatch match = re.match(shortVersion);
        if (match.hasMatch() && match.lastCapturedIndex() == 3)
            for (int i = match.captured(2).toInt(); i <= match.captured(3).toInt(); ++i)
                versions.append(QVersionNumber::fromString(match.captured(1) + QString::number(i)));
        else
            versions.append(QVersionNumber::fromString(shortVersion + ".-1"));

        return versions;
    };

    if (jsonObject.contains(SpecificQtVersionsKey) && jsonObject[SpecificQtVersionsKey].isArray()) {
        const QJsonArray versionsArray = jsonObject[SpecificQtVersionsKey].toArray();
        for (const QJsonValue &item : versionsArray) {
            QJsonObject itemObj = item.toObject();
            SdkForQtVersions specificVersion;
            const auto pkgs = itemObj[SdkEssentialPkgsKey].toArray();
            for (const QJsonValue &pkg : pkgs)
                specificVersion.essentialPackages.append(pkg.toString());
            const auto versions = itemObj[VersionsKey].toArray();
            for (const QJsonValue &pkg : versions)
                specificVersion.versions.append(fillQtVersionsRange(pkg.toString()));

            if (itemObj[VersionsKey].toArray().first().toString() == DefaultVersionKey)
                m_defaultSdkDepends = specificVersion;
            else
                m_specificQtVersions.append(specificVersion);
        }
    }
}

static QList<int> availableNdkPlatformsLegacy(const FilePath &ndkLocation)
{
    QList<int> availableNdkPlatforms;

    ndkLocation
        .pathAppended("platforms")
        .iterateDirectory(
            [&availableNdkPlatforms](const FilePath &filePath) {
                availableNdkPlatforms.push_back(
                    filePath.toString()
                        .mid(filePath.path().lastIndexOf('-') + 1)
                        .toInt());
                return IterationPolicy::Continue;
            },
            {{"android-*"}, QDir::Dirs});

    return availableNdkPlatforms;
}

static QList<int> availableNdkPlatformsV21Plus(const FilePath &ndkLocation, const Abis &abis,
                                                 OsType hostOs)
{
    if (abis.isEmpty())
        return {};

    const QString abi = AndroidConfig::toolsPrefix(abis.first());
    const FilePath libPath =
            AndroidConfig::toolchainPathFromNdk(ndkLocation, hostOs) / "sysroot/usr/lib" / abi;
    const FilePaths dirEntries = libPath.dirEntries(QDir::Dirs | QDir::NoDotAndDotDot);
    const QList<int> availableNdkPlatforms =
            Utils::transform(dirEntries, [](const FilePath &path) {
                return path.fileName().toInt(); });
    return availableNdkPlatforms;
}

static QList<int> availableNdkPlatformsImpl(const FilePath &ndkLocation, const Abis &abis,
                                              OsType hostOs)
{
    QList<int> result = availableNdkPlatformsLegacy(ndkLocation);

    if (result.isEmpty())
        result = availableNdkPlatformsV21Plus(ndkLocation, abis, hostOs);

    return Utils::sorted(std::move(result), std::greater<>());
}

QList<int> AndroidConfig::availableNdkPlatforms(const QtVersion *qtVersion) const
{
    return availableNdkPlatformsImpl(ndkLocation(qtVersion), qtVersion->qtAbis(),
                                     HostOsInfo::hostOs());
}

QStringList AndroidConfig::getCustomNdkList() const
{
    return m_customNdkList;
}

void AndroidConfig::addCustomNdk(const QString &customNdk)
{
    if (!m_customNdkList.contains(customNdk))
        m_customNdkList.append(customNdk);
}

void AndroidConfig::removeCustomNdk(const QString &customNdk)
{
    m_customNdkList.removeAll(customNdk);
}

void AndroidConfig::setDefaultNdk(const Utils::FilePath &defaultNdk)
{
    m_defaultNdk = defaultNdk;
}

FilePath AndroidConfig::defaultNdk() const
{
    return m_defaultNdk;
}

FilePath AndroidConfig::openSslLocation() const
{
    return m_openSslLocation;
}

void AndroidConfig::setOpenSslLocation(const FilePath &openSslLocation)
{
    m_openSslLocation = openSslLocation;
}

QStringList AndroidConfig::apiLevelNamesFor(const SdkPlatformList &platforms)
{
    return Utils::transform(platforms, AndroidConfig::apiLevelNameFor);
}

QString AndroidConfig::apiLevelNameFor(const SdkPlatform *platform)
{
    if (platform && platform->apiLevel() > 0) {
        QString sdkStylePath = platform->sdkStylePath();
        return sdkStylePath.remove("platforms;");
    }

    return {};
}

FilePath AndroidConfig::adbToolPath() const
{
    return m_sdkLocation.pathAppended("platform-tools/adb").withExecutableSuffix();
}

FilePath AndroidConfig::emulatorToolPath() const
{
    const FilePath emulatorFile = m_sdkLocation.pathAppended("emulator/emulator")
                                      .withExecutableSuffix();
    if (emulatorFile.exists())
        return emulatorFile;

    return {};
}

FilePath AndroidConfig::sdkManagerToolPath() const
{
    const FilePath sdkmanagerPath = m_sdkLocation.pathAppended(Constants::cmdlineToolsName)
                                        .pathAppended("latest/bin/sdkmanager" ANDROID_BAT_SUFFIX);
    if (sdkmanagerPath.exists())
        return sdkmanagerPath;

    // If it's a first time install use the path of Constants::cmdlineToolsName temporary download
    const FilePath sdkmanagerTmpPath = m_temporarySdkToolsPath.pathAppended(
        "/bin/sdkmanager" ANDROID_BAT_SUFFIX);
    if (sdkmanagerTmpPath.exists())
        return sdkmanagerTmpPath;

    return {};
}

FilePath AndroidConfig::avdManagerToolPath() const
{
    const FilePath sdkmanagerPath = m_sdkLocation.pathAppended(Constants::cmdlineToolsName)
                                        .pathAppended("/latest/bin/avdmanager" ANDROID_BAT_SUFFIX);
    if (sdkmanagerPath.exists())
        return sdkmanagerPath;

    return {};
}

void AndroidConfig::setTemporarySdkToolsPath(const Utils::FilePath &path)
{
    m_temporarySdkToolsPath = path;
}

FilePath AndroidConfig::sdkToolsVersionPath() const
{
    const FilePath sdkVersionPaths = m_sdkLocation.pathAppended(Constants::cmdlineToolsName)
                                         .pathAppended("/latest/source.properties");
    if (sdkVersionPaths.exists())
        return sdkVersionPaths;

    // If it's a first time install use the path of Constants::cmdlineToolsName temporary download
    const FilePath tmpSdkPath = m_temporarySdkToolsPath.pathAppended("source.properties");
    if (tmpSdkPath.exists())
        return tmpSdkPath;

    return {};
}

FilePath AndroidConfig::toolchainPathFromNdk(const FilePath &ndkLocation, OsType hostOs)
{
    const FilePath tcPath = ndkLocation / "toolchains/";
    FilePath toolchainPath;
    QDirIterator llvmIter(tcPath.toString(), {"llvm*"}, QDir::Dirs);
    if (llvmIter.hasNext()) {
        llvmIter.next();
        toolchainPath = tcPath / llvmIter.fileName() / "prebuilt/";
    }

    // detect toolchain host
    QStringList hostPatterns;
    switch (hostOs) {
    case OsTypeLinux:
        hostPatterns << QLatin1String("linux*");
        break;
    case OsTypeWindows:
        hostPatterns << QLatin1String("windows*");
        break;
    case OsTypeMac:
        hostPatterns << QLatin1String("darwin*");
        break;
    default: /* unknown host */ return {};
    }

    QDirIterator iter(toolchainPath.toString(), hostPatterns, QDir::Dirs);
    if (iter.hasNext()) {
        iter.next();
        return toolchainPath / iter.fileName();
    }

    return {};
}

FilePath AndroidConfig::toolchainPath(const QtVersion *qtVersion) const
{
    return toolchainPathFromNdk(ndkLocation(qtVersion));
}

FilePath AndroidConfig::clangPathFromNdk(const FilePath &ndkLocation)
{
    const FilePath path = toolchainPathFromNdk(ndkLocation);
    if (path.isEmpty())
        return {};
    return path.pathAppended("bin/clang").withExecutableSuffix();
}

FilePath AndroidConfig::gdbPath(const Abi &abi, const QtVersion *qtVersion) const
{
    return gdbPathFromNdk(abi, ndkLocation(qtVersion));
}

FilePath AndroidConfig::gdbPathFromNdk(const Abi &abi, const FilePath &ndkLocation)
{
    const FilePath path = ndkLocation.pathAppended(
        QString("prebuilt/%1/bin/gdb%2").arg(toolchainHostFromNdk(ndkLocation),
                                             QString(QTC_HOST_EXE_SUFFIX)));
    if (path.exists())
        return path;
    // fallback for old NDKs (e.g. 10e)
    return ndkLocation.pathAppended(QString("toolchains/%1-4.9/prebuilt/%2/bin/%3-gdb%4")
                                                   .arg(toolchainPrefix(abi),
                                                        toolchainHostFromNdk(ndkLocation),
                                                        toolsPrefix(abi),
                                                        QString(QTC_HOST_EXE_SUFFIX)));
}

FilePath AndroidConfig::lldbPathFromNdk(const FilePath &ndkLocation)
{
    const FilePath path = ndkLocation.pathAppended(
        QString("toolchains/llvm/prebuilt/%1/bin/lldb%2").arg(toolchainHostFromNdk(ndkLocation),
                                              QString(QTC_HOST_EXE_SUFFIX)));
    if (path.exists())
        return path;
    return {};
}

FilePath AndroidConfig::makePathFromNdk(const FilePath &ndkLocation)
{
    return ndkLocation.pathAppended(
                QString("prebuilt/%1/bin/make%2").arg(toolchainHostFromNdk(ndkLocation),
                                                      QString(QTC_HOST_EXE_SUFFIX)));
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
    return openJDKBinPath().pathAppended(keytoolName).withExecutableSuffix();
}

QVector<AndroidDeviceInfo> AndroidConfig::connectedDevices(QString *error) const
{
    QVector<AndroidDeviceInfo> devices;
    Process adbProc;
    adbProc.setTimeoutS(30);
    CommandLine cmd{adbToolPath(), {"devices"}};
    adbProc.setCommand(cmd);
    adbProc.runBlocking();
    if (adbProc.result() != ProcessResult::FinishedWithSuccess) {
        if (error)
            *error = Tr::tr("Could not run: %1").arg(cmd.toUserOutput());
        return devices;
    }
    QStringList adbDevs = adbProc.allOutput().split('\n', Qt::SkipEmptyParts);
    if (adbDevs.empty())
        return devices;

    for (const QString &line : adbDevs) // remove the daemon logs
        if (line.startsWith("* daemon"))
            adbDevs.removeOne(line);
    adbDevs.removeFirst(); // remove "List of devices attached" header line

    // workaround for '????????????' serial numbers:
    // can use "adb -d" when only one usb device attached
    for (const QString &device : std::as_const(adbDevs)) {
        const QString serialNo = device.left(device.indexOf('\t')).trimmed();
        const QString deviceType = device.mid(device.indexOf('\t')).trimmed();
        AndroidDeviceInfo dev;
        dev.serialNumber = serialNo;
        dev.type = serialNo.startsWith(QLatin1String("emulator")) ? IDevice::Emulator
                                                                  : IDevice::Hardware;
        dev.sdk = getSDKVersion(dev.serialNumber);
        dev.cpuAbi = getAbis(dev.serialNumber);
        if (deviceType == QLatin1String("unauthorized"))
            dev.state = IDevice::DeviceConnected;
        else if (deviceType == QLatin1String("offline"))
            dev.state = IDevice::DeviceDisconnected;
        else
            dev.state = IDevice::DeviceReadyToUse;

        if (dev.type == IDevice::Emulator) {
            dev.avdName = getAvdName(dev.serialNumber);
            if (dev.avdName.isEmpty())
                dev.avdName = serialNo;
        }

        devices.push_back(dev);
    }

    Utils::sort(devices);
    if (devices.isEmpty() && error)
        *error = Tr::tr("No devices found in output of: %1").arg(cmd.toUserOutput());
    return devices;
}

bool AndroidConfig::isConnected(const QString &serialNumber) const
{
    const QVector<AndroidDeviceInfo> devices = connectedDevices();
    for (const AndroidDeviceInfo &device : devices) {
        if (device.serialNumber == serialNumber)
            return true;
    }
    return false;
}

QString AndroidConfig::getDeviceProperty(const QString &device, const QString &property)
{
    // workaround for '????????????' serial numbers
    CommandLine cmd(AndroidConfigurations::currentConfig().adbToolPath(),
                    AndroidDeviceInfo::adbSelector(device));
    cmd.addArgs({"shell", "getprop", property});

    Process adbProc;
    adbProc.setTimeoutS(10);
    adbProc.setCommand(cmd);
    adbProc.runBlocking();
    if (adbProc.result() != ProcessResult::FinishedWithSuccess)
        return {};

    return adbProc.allOutput();
}

int AndroidConfig::getSDKVersion(const QString &device)
{
    QString tmp = getDeviceProperty(device, "ro.build.version.sdk");
    if (tmp.isEmpty())
        return -1;
    return tmp.trimmed().toInt();
}

QString AndroidConfig::getAvdName(const QString &serialnumber)
{
    int index = serialnumber.indexOf(QLatin1String("-"));
    if (index == -1)
        return {};
    bool ok;
    int port = serialnumber.mid(index + 1).toInt(&ok);
    if (!ok)
        return {};

    const QByteArray avdName = "avd name\n";

    QTcpSocket tcpSocket;
    tcpSocket.connectToHost(QHostAddress(QHostAddress::LocalHost), port);
    if (!tcpSocket.waitForConnected(100)) // Don't wait more than 100ms for a local connection
        return {};

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

//!
//! \brief AndroidConfigurations::getProductModel
//! \param device serial number
//! \return the produce model of the device or if that cannot be read the serial number
//!
QString AndroidConfig::getProductModel(const QString &device) const
{
    if (m_serialNumberToDeviceName.contains(device))
        return m_serialNumberToDeviceName.value(device);

    QString model = getDeviceProperty(device, "ro.product.model").trimmed();
    if (model.isEmpty())
        return device;

    if (!device.startsWith(QLatin1String("????")))
        m_serialNumberToDeviceName.insert(device, model);
    return model;
}

QStringList AndroidConfig::getAbis(const QString &device)
{
    const FilePath adbTool = AndroidConfigurations::currentConfig().adbToolPath();
    QStringList result;
    // First try via ro.product.cpu.abilist
    QStringList arguments = AndroidDeviceInfo::adbSelector(device);
    arguments << "shell" << "getprop" << "ro.product.cpu.abilist";
    Process adbProc;
    adbProc.setTimeoutS(10);
    adbProc.setCommand({adbTool, arguments});
    adbProc.runBlocking();
    if (adbProc.result() != ProcessResult::FinishedWithSuccess)
        return result;

    QString output = adbProc.allOutput().trimmed();
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

        Process abiProc;
        abiProc.setTimeoutS(10);
        abiProc.setCommand({adbTool, arguments});
        abiProc.runBlocking();
        if (abiProc.result() != ProcessResult::FinishedWithSuccess)
            return result;

        QString abi = abiProc.allOutput().trimmed();
        if (abi.isEmpty())
            break;
        result << abi;
    }
    return result;
}

bool AndroidConfig::isValidNdk(const QString &ndkLocation) const
{
    auto ndkPath = Utils::FilePath::fromUserInput(ndkLocation);

    if (!ndkPath.exists())
        return false;

    if (!ndkPath.pathAppended("toolchains").exists())
        return false;

    const QVersionNumber version = ndkVersion(ndkPath);
    if (version.isNull())
        return false;

    const FilePath ndkPlatformsDir = ndkPath.pathAppended("platforms");
    if (version.majorVersion() <= 22
            && (!ndkPlatformsDir.exists() || ndkPlatformsDir.toString().contains(' ')))
        return false;

    return true;
}

QString AndroidConfig::bestNdkPlatformMatch(int target, const QtVersion *qtVersion) const
{
    target = std::max(AndroidManager::defaultMinimumSDK(qtVersion), target);
    const QList<int> platforms = availableNdkPlatforms(qtVersion);
    for (const int apiLevel : platforms) {
        if (apiLevel <= target)
            return QString::fromLatin1("android-%1").arg(apiLevel);
    }
    return QString("android-%1").arg(AndroidManager::defaultMinimumSDK(qtVersion));
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
    if (!m_sdkLocation.exists())
        return {};

    const FilePath sdkToolsPropertiesPath = sdkToolsVersionPath();
    const QSettings settings(sdkToolsPropertiesPath.toString(), QSettings::IniFormat);
    return QVersionNumber::fromString(settings.value(sdkToolsVersionKey).toString());
}

QVersionNumber AndroidConfig::buildToolsVersion() const
{
    //TODO: return version according to qt version
    QVersionNumber maxVersion;
    QDir buildToolsDir(m_sdkLocation.pathAppended("build-tools").toString());
    const auto files = buildToolsDir.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot);
    for (const QFileInfo &file: files)
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

FilePath AndroidConfig::ndkLocation(const QtVersion *qtVersion) const
{
    if (!m_defaultNdk.isEmpty())
        return m_defaultNdk; // A selected default NDK is good for any Qt version
    return sdkLocation().resolvePath(ndkSubPathFromQtVersion(*qtVersion));
}

QVersionNumber AndroidConfig::ndkVersion(const QtVersion *qtVersion) const
{
    return ndkVersion(ndkLocation(qtVersion));
}

QVersionNumber AndroidConfig::ndkVersion(const FilePath &ndkPath)
{
    QVersionNumber version;
    if (!ndkPath.exists()) {
        qCDebug(avdConfigLog).noquote() << "Cannot find ndk version. Check NDK path."
                                        << ndkPath.toUserOutput();
        return version;
    }

    const FilePath ndkPropertiesPath = ndkPath.pathAppended("source.properties");
    if (ndkPropertiesPath.exists()) {
        // source.properties files exists in NDK version > 11
        QSettings settings(ndkPropertiesPath.toString(), QSettings::IniFormat);
        auto versionStr = settings.value(ndkRevisionKey).toString();
        version = QVersionNumber::fromString(versionStr);
    } else {
        // No source.properties. There should be a file named RELEASE.TXT
        const FilePath ndkReleaseTxtPath = ndkPath.pathAppended("RELEASE.TXT");
        FileReader reader;
        QString errorString;
        if (reader.fetch(ndkReleaseTxtPath, &errorString)) {
            // RELEASE.TXT contains the ndk version in either of the following formats:
            // r6a
            // r10e (64 bit)
            QString content = QString::fromUtf8(reader.data());
            static const QRegularExpression re("(r)(?<major>[0-9]{1,2})(?<minor>[a-z]{1,1})");
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

QStringList AndroidConfig::allEssentials() const
{
    QtVersions installedVersions = QtVersionManager::versions(
        [](const QtVersion *v) {
            return v->targetDeviceTypes().contains(Android::Constants::ANDROID_DEVICE_TYPE);
        });

    QStringList allPackages(defaultEssentials());
    for (const QtVersion *version : installedVersions)
        allPackages.append(essentialsFromQtVersion(*version));
    allPackages.removeDuplicates();

    return allPackages;
}

static QStringList packagesWithoutNdks(const QStringList &packages)
{
    return Utils::filtered(packages, [] (const QString &p) {
        return !p.startsWith(ndkPackageMarker()); });
}

bool AndroidConfig::allEssentialsInstalled(AndroidSdkManager *sdkManager)
{
    QStringList essentialPkgs(allEssentials());
    const auto installedPkgs = sdkManager->installedSdkPackages();
    for (const AndroidSdkPackage *pkg : installedPkgs) {
        if (essentialPkgs.contains(pkg->sdkStylePath()))
            essentialPkgs.removeOne(pkg->sdkStylePath());
        if (essentialPkgs.isEmpty())
            break;
    }
    if (!m_defaultNdk.isEmpty())
        essentialPkgs = packagesWithoutNdks(essentialPkgs);
    return essentialPkgs.isEmpty() ? true : false;
}

bool AndroidConfig::sdkToolsOk() const
{
    bool exists = sdkLocation().exists();
    bool writable = sdkLocation().isWritableDir();
    bool sdkToolsExist = !sdkToolsVersion().isNull();
    return exists && writable && sdkToolsExist;
}

QStringList AndroidConfig::essentialsFromQtVersion(const QtVersion &version) const
{
    if (auto androidQtVersion = dynamic_cast<const AndroidQtVersion *>(&version)) {
        bool ok;
        const AndroidQtVersion::BuiltWith bw = androidQtVersion->builtWith(&ok);
        if (ok) {
            const QString ndkPackage = ndkPackageMarker() + bw.ndkVersion.toString();
            return QStringList(ndkPackage)
                   + packagesWithoutNdks(m_defaultSdkDepends.essentialPackages);
        }
    }

    QVersionNumber qtVersion = version.qtVersion();
    for (const SdkForQtVersions &item : m_specificQtVersions)
        if (item.containsVersion(qtVersion))
            return item.essentialPackages;

    return m_defaultSdkDepends.essentialPackages;
}

static FilePath ndkSubPath(const SdkForQtVersions &packages)
{
    const QString ndkPrefix = ndkPackageMarker();
    for (const QString &package : packages.essentialPackages)
        if (package.startsWith(ndkPrefix))
            return FilePath::fromString(NdksSubDir) / package.sliced(ndkPrefix.length());

    return {};
}

FilePath AndroidConfig::ndkSubPathFromQtVersion(const QtVersion &version) const
{
    if (auto androidQtVersion = dynamic_cast<const AndroidQtVersion *>(&version)) {
        bool ok;
        const AndroidQtVersion::BuiltWith bw = androidQtVersion->builtWith(&ok);
        if (ok)
            return FilePath::fromString(NdksSubDir) / bw.ndkVersion.toString();
    }

    for (const SdkForQtVersions &item : m_specificQtVersions)
        if (item.containsVersion(version.qtVersion()))
            return ndkSubPath(item);

    return ndkSubPath(m_defaultSdkDepends);
}

QStringList AndroidConfig::defaultEssentials() const
{
    return m_defaultSdkDepends.essentialPackages + m_commonEssentialPkgs;
}

bool SdkForQtVersions::containsVersion(const QVersionNumber &qtVersion) const
{
    return versions.contains(qtVersion)
            || versions.contains(QVersionNumber(qtVersion.majorVersion(),
                                                qtVersion.minorVersion()));
}

FilePath AndroidConfig::openJDKLocation() const
{
    return m_openJDKLocation;
}

void AndroidConfig::setOpenJDKLocation(const FilePath &openJDKLocation)
{
    m_openJDKLocation = openJDKLocation;
}

QString AndroidConfig::toolchainHost(const QtVersion *qtVersion) const
{
    return toolchainHostFromNdk(ndkLocation(qtVersion));
}

QString AndroidConfig::toolchainHostFromNdk(const FilePath &ndkPath)
{
    // detect toolchain host
    QString toolchainHost;
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
    default: /* unknown host */
        return toolchainHost;
    }

    QDirIterator jt(ndkPath.pathAppended("prebuilt").toString(),
                    hostPatterns,
                    QDir::Dirs);
    if (jt.hasNext()) {
        jt.next();
        toolchainHost = jt.fileName();
    }

    return toolchainHost;
}

QString AndroidConfig::emulatorArgs() const
{
    return m_emulatorArgs;
}

void AndroidConfig::setEmulatorArgs(const QString &args)
{
    m_emulatorArgs = args;
}

bool AndroidConfig::automaticKitCreation() const
{
    return m_automaticKitCreation;
}

void AndroidConfig::setAutomaticKitCreation(bool b)
{
    m_automaticKitCreation = b;
}

FilePath AndroidConfig::defaultSdkPath()
{
    QString sdkFromEnvVar = QString::fromLocal8Bit(getenv("ANDROID_SDK_ROOT"));
    if (!sdkFromEnvVar.isEmpty())
        return FilePath::fromUserInput(sdkFromEnvVar).cleanPath();

    // Set default path of SDK as used by Android Studio
    if (HostOsInfo::isMacHost()) {
        return FilePath::fromString(
            QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/Library/Android/sdk");
    }

    if (HostOsInfo::isWindowsHost()) {
        return FilePath::fromString(
            QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/Android/Sdk");
    }

    return FilePath::fromString(
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/Android/Sdk");
}

///////////////////////////////////
// AndroidConfigurations
///////////////////////////////////
void AndroidConfigurations::setConfig(const AndroidConfig &devConfigs)
{
    emit m_instance->aboutToUpdate();
    m_instance->m_config = devConfigs;

    m_instance->save();
    updateAndroidDevice();
    registerNewToolChains();
    updateAutomaticKitList();
    removeOldToolChains();
    emit m_instance->updated();
}

static bool matchToolChain(const ToolChain *atc, const ToolChain *btc)
{
    if (atc == btc)
        return true;

    if (!atc || !btc)
        return false;

    if (atc->typeId() != Constants::ANDROID_TOOLCHAIN_TYPEID || btc->typeId() != Constants::ANDROID_TOOLCHAIN_TYPEID)
        return false;

    return atc->targetAbi() == btc->targetAbi();
}

void AndroidConfigurations::registerNewToolChains()
{
    const Toolchains existingAndroidToolChains
            = ToolChainManager::toolchains(Utils::equal(&ToolChain::typeId, Id(Constants::ANDROID_TOOLCHAIN_TYPEID)));

    const Toolchains newToolchains = AndroidToolChainFactory::autodetectToolChains(
        existingAndroidToolChains);

    for (ToolChain *tc : newToolchains)
        ToolChainManager::registerToolChain(tc);

    registerCustomToolChainsAndDebuggers();
}

void AndroidConfigurations::removeOldToolChains()
{
    const auto tcs = ToolChainManager::toolchains(Utils::equal(&ToolChain::typeId,
                                                               Id(Constants::ANDROID_TOOLCHAIN_TYPEID)));
    for (ToolChain *tc : tcs) {
        if (!tc->isValid())
            ToolChainManager::deregisterToolChain(tc);
    }
}

void AndroidConfigurations::removeUnusedDebuggers()
{
    const QList<QtVersion*> qtVersions = QtVersionManager::versions([](const QtVersion *v) {
        return v->type() == Constants::ANDROID_QT_TYPE;
    });

    QVector<FilePath> uniqueNdks;
    for (const QtVersion *qt : qtVersions) {
        FilePath ndkLocation = currentConfig().ndkLocation(qt);
        if (!uniqueNdks.contains(ndkLocation))
            uniqueNdks.append(ndkLocation);
    }

    uniqueNdks.append(FileUtils::toFilePathList(currentConfig().getCustomNdkList()));

    const QList<Debugger::DebuggerItem> allDebuggers = Debugger::DebuggerItemManager::debuggers();
    for (const Debugger::DebuggerItem &debugger : allDebuggers) {
        if (!debugger.displayName().contains("Android"))
            continue;

        bool isChildOfNdk = false;
        for (const FilePath &path : uniqueNdks) {
            if (debugger.command().isChildOf(path)) {
                isChildOfNdk = true;
                break;
            }
        }

        const bool isMultiAbiNdkGdb = debugger.command().fileName().startsWith("gdb");
        const bool hasMultiAbiName = debugger.displayName().contains("Multi-Abi");

        if (debugger.isAutoDetected() && (!isChildOfNdk || (isMultiAbiNdkGdb && !hasMultiAbiName)))
            Debugger::DebuggerItemManager::deregisterDebugger(debugger.id());
    }
}

static QStringList allSupportedAbis()
{
    return QStringList{
        ProjectExplorer::Constants::ANDROID_ABI_ARMEABI_V7A,
        ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A,
        ProjectExplorer::Constants::ANDROID_ABI_X86,
        ProjectExplorer::Constants::ANDROID_ABI_X86_64,
    };
}

static bool containsAllAbis(const QStringList &abis)
{
    QStringList supportedAbis{allSupportedAbis()};
    for (const QString &abi : abis)
        if (supportedAbis.contains(abi))
            supportedAbis.removeOne(abi);

    return supportedAbis.isEmpty();
}

static QString getMultiOrSingleAbiString(const QStringList &abis)
{
    return containsAllAbis(abis) ? "Multi-Abi" : abis.join(",");
}

static const Debugger::DebuggerItem *existingDebugger(const FilePath &command,
                                                      Debugger::DebuggerEngineType type)
{
    // check if the debugger is already registered, but ignoring the display name
    const Debugger::DebuggerItem *existing = Debugger::DebuggerItemManager::findByCommand(command);

    // Return existing debugger with same command
    if (existing && existing->engineType() == type && existing->isAutoDetected())
        return existing;
    return nullptr;
}

static QVariant findOrRegisterDebugger(ToolChain *tc,
                                       const QStringList &abisList,
                                       bool customDebugger = false)
{
    const auto &currentConfig = AndroidConfigurations::currentConfig();
    const FilePath ndk = static_cast<AndroidToolChain *>(tc)->ndkLocation();
    const FilePath lldbCommand = currentConfig.lldbPathFromNdk(ndk);
    const Debugger::DebuggerItem *existingLldb = existingDebugger(lldbCommand,
                                                                  Debugger::LldbEngineType);
    // Return existing debugger with same command - prefer lldb (limit to sdk/ndk min version?)
    if (existingLldb)
        return existingLldb->id();

    const FilePath gdbCommand = currentConfig.gdbPathFromNdk(tc->targetAbi(), ndk);

    // check if the debugger is already registered, but ignoring the display name
    const Debugger::DebuggerItem *existingGdb = existingDebugger(gdbCommand,
                                                                 Debugger::GdbEngineType);
    // Return existing debugger with same command
    if (existingGdb)
        return existingGdb->id();

    const QString mainName = Tr::tr("Android Debugger (%1, NDK %2)");
    const QString custom = customDebugger ? QString{"Custom "} : QString{};
    // debugger not found, register a new one
    // check lldb
    QVariant registeredLldb;
    if (!lldbCommand.isEmpty()) {
        Debugger::DebuggerItem debugger;
        debugger.setCommand(lldbCommand);
        debugger.setEngineType(Debugger::LldbEngineType);
        debugger.setUnexpandedDisplayName(custom + mainName
                .arg(getMultiOrSingleAbiString(allSupportedAbis()))
                .arg(AndroidConfigurations::currentConfig().ndkVersion(ndk).toString())
                                          + ' ' + debugger.engineTypeName());
        debugger.setAutoDetected(true);
        debugger.reinitializeFromFile();
        registeredLldb = Debugger::DebuggerItemManager::registerDebugger(debugger);
    }

    // we always have a value for gdb (but we shouldn't - we currently use a fallback)
    if (!gdbCommand.exists()) {
        if (!registeredLldb.isNull())
            return registeredLldb;
        return {};
    }

    Debugger::DebuggerItem debugger;
    debugger.setCommand(gdbCommand);
    debugger.setEngineType(Debugger::GdbEngineType);

    // NDK 10 and older have multiple gdb versions per ABI, so check for that.
    const bool oldNdkVersion = currentConfig.ndkVersion(ndk) <= QVersionNumber{11};
    debugger.setUnexpandedDisplayName(custom + mainName
            .arg(getMultiOrSingleAbiString(oldNdkVersion ? abisList : allSupportedAbis()))
            .arg(AndroidConfigurations::currentConfig().ndkVersion(ndk).toString())
                                      + ' ' + debugger.engineTypeName());
    debugger.setAutoDetected(true);
    debugger.reinitializeFromFile();
    QVariant registeredGdb = Debugger::DebuggerItemManager::registerDebugger(debugger);
    return registeredLldb.isNull() ? registeredGdb : registeredLldb;
}

void AndroidConfigurations::registerCustomToolChainsAndDebuggers()
{
    const Toolchains existingAndroidToolChains = ToolChainManager::toolchains(
        Utils::equal(&ToolChain::typeId, Utils::Id(Constants::ANDROID_TOOLCHAIN_TYPEID)));

    const FilePaths customNdks = FileUtils::toFilePathList(currentConfig().getCustomNdkList());
    const Toolchains customToolchains
        = AndroidToolChainFactory::autodetectToolChainsFromNdks(existingAndroidToolChains,
                                                                customNdks,
                                                                true);
    for (ToolChain *tc : customToolchains) {
        ToolChainManager::registerToolChain(tc);
        const auto androidToolChain = static_cast<AndroidToolChain *>(tc);
        QString abiStr;
        if (androidToolChain)
            abiStr = androidToolChain->platformLinkerFlags().at(1).split('-').first();
        findOrRegisterDebugger(tc, {abiStr}, true);
    }
}
void AndroidConfigurations::updateAutomaticKitList()
{
    for (Kit *k : KitManager::kits()) {
        if (DeviceTypeKitAspect::deviceTypeId(k) == Constants::ANDROID_DEVICE_TYPE) {
            if (k->value(Constants::ANDROID_KIT_NDK).isNull() || k->value(Constants::ANDROID_KIT_SDK).isNull()) {
                if (QtVersion *qt = QtKitAspect::qtVersion(k)) {
                    k->setValueSilently(Constants::ANDROID_KIT_NDK, currentConfig().ndkLocation(qt).toString());
                    k->setValue(Constants::ANDROID_KIT_SDK, currentConfig().sdkLocation().toString());
                }
            }
        }
    }

    const QList<Kit *> existingKits = Utils::filtered(KitManager::kits(), [](Kit *k) {
        Id deviceTypeId = DeviceTypeKitAspect::deviceTypeId(k);
        if (k->isAutoDetected() && !k->isSdkProvided()
                && deviceTypeId == Constants::ANDROID_DEVICE_TYPE) {
            return true;
        }
        return false;
    });

    removeUnusedDebuggers();

    QHash<Abi, QList<const QtVersion *> > qtVersionsForArch;
    const QList<QtVersion*> qtVersions = QtVersionManager::versions([](const QtVersion *v) {
        return v->type() == Constants::ANDROID_QT_TYPE;
    });
    for (const QtVersion *qtVersion : qtVersions) {
        const Abis qtAbis = qtVersion->qtAbis();
        if (qtAbis.empty())
            continue;
        qtVersionsForArch[qtAbis.first()].append(qtVersion);
    }

    // register new kits
    const Toolchains toolchains = ToolChainManager::toolchains([](const ToolChain *tc) {
        return tc->isAutoDetected() && tc->typeId() == Constants::ANDROID_TOOLCHAIN_TYPEID
               && tc->isValid();
    });
    QList<Kit *> unhandledKits = existingKits;
    for (ToolChain *tc : toolchains) {
        if (tc->language() != ProjectExplorer::Constants::CXX_LANGUAGE_ID)
            continue;

        for (const QtVersion *qt : qtVersionsForArch.value(tc->targetAbi())) {
            FilePath tcNdk = static_cast<const AndroidToolChain *>(tc)->ndkLocation();
            if (tcNdk != currentConfig().ndkLocation(qt))
                continue;

            const Toolchains allLanguages
                = Utils::filtered(toolchains, [tc, tcNdk](ToolChain *otherTc) {
                      FilePath otherNdk = static_cast<const AndroidToolChain *>(otherTc)->ndkLocation();
                      return tc->targetAbi() == otherTc->targetAbi() && tcNdk == otherNdk;
                  });

            QHash<Id, ToolChain *> toolChainForLanguage;
            for (ToolChain *tc : allLanguages)
                toolChainForLanguage[tc->language()] = tc;

            Kit *existingKit = Utils::findOrDefault(existingKits, [&](const Kit *b) {
                if (qt != QtKitAspect::qtVersion(b))
                    return false;
                return matchToolChain(toolChainForLanguage[ProjectExplorer::Constants::CXX_LANGUAGE_ID],
                                      ToolChainKitAspect::cxxToolChain(b))
                        && matchToolChain(toolChainForLanguage[ProjectExplorer::Constants::C_LANGUAGE_ID],
                                          ToolChainKitAspect::cToolChain(b));
            });

            const auto initializeKit = [allLanguages, tc, qt](Kit *k) {
                k->setAutoDetected(true);
                k->setAutoDetectionSource("AndroidConfiguration");
                DeviceTypeKitAspect::setDeviceTypeId(k, Constants::ANDROID_DEVICE_TYPE);
                for (ToolChain *tc : allLanguages)
                    ToolChainKitAspect::setToolChain(k, tc);
                QtKitAspect::setQtVersion(k, qt);
                QStringList abis = static_cast<const AndroidQtVersion *>(qt)->androidAbis();
                Debugger::DebuggerKitAspect::setDebugger(k, findOrRegisterDebugger(tc, abis));

                BuildDeviceKitAspect::setDeviceId(k, DeviceManager::defaultDesktopDevice()->id());
                k->setSticky(QtKitAspect::id(), true);
                k->setMutable(DeviceKitAspect::id(), true);
                k->setSticky(DeviceTypeKitAspect::id(), true);

                QString versionStr = QLatin1String("Qt %{Qt:Version}");
                if (!qt->isAutodetected())
                    versionStr = QString("%1").arg(qt->displayName());
                k->setUnexpandedDisplayName(Tr::tr("Android %1 Clang %2")
                                                .arg(versionStr)
                                                .arg(getMultiOrSingleAbiString(abis)));
                k->setValueSilently(Constants::ANDROID_KIT_NDK, currentConfig().ndkLocation(qt).toString());
                k->setValueSilently(Constants::ANDROID_KIT_SDK, currentConfig().sdkLocation().toString());
            };

            if (existingKit) {
                initializeKit(existingKit); // Update the existing kit with new data.
                unhandledKits.removeOne(existingKit);
            } else {
                KitManager::registerKit(initializeKit);
            }
        }
    }
    // cleanup any mess that might have existed before, by removing all Android kits that
    // existed before, but weren't re-used
    for (Kit *k : unhandledKits)
        KitManager::deregisterKit(k);
}

Environment AndroidConfig::toolsEnvironment() const
{
    Environment env = Environment::systemEnvironment();
    FilePath jdkLocation = openJDKLocation();
    if (!jdkLocation.isEmpty()) {
        env.set(Constants::JAVA_HOME_ENV_VAR, jdkLocation.toUserOutput());
        env.prependOrSetPath(jdkLocation.pathAppended("bin"));
    }
    return env;
}

AndroidConfig &AndroidConfigurations::currentConfig()
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
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    m_config.save(*settings);
    settings->endGroup();
}

AndroidConfigurations::AndroidConfigurations()
    : m_sdkManager(new AndroidSdkManager(m_config))
{
    load();
    connect(DeviceManager::instance(), &DeviceManager::devicesLoaded,
            this, &AndroidConfigurations::updateAndroidDevice);

    m_instance = this;
}

AndroidConfigurations::~AndroidConfigurations() = default;

static FilePath androidStudioPath()
{
#if defined(Q_OS_WIN)
    const QLatin1String registryKey("HKEY_LOCAL_MACHINE\\SOFTWARE\\Android Studio");
    const QLatin1String valueName("Path");
    const QSettings settings64(registryKey, QSettings::Registry64Format);
    const QSettings settings32(registryKey, QSettings::Registry32Format);
    return FilePath::fromUserInput(
                settings64.value(valueName, settings32.value(valueName).toString()).toString());
#endif
    return {}; // TODO non-Windows
}

FilePath AndroidConfig::getJdkPath()
{
    FilePath jdkHome = FilePath::fromString(qtcEnvironmentVariable(Constants::JAVA_HOME_ENV_VAR));
    if (jdkHome.exists())
        return jdkHome;

    if (HostOsInfo::isWindowsHost()) {
        // Look for Android Studio's jdk first
        const FilePath androidStudioSdkPath = androidStudioPath();
        if (!androidStudioSdkPath.isEmpty()) {
            const FilePath androidStudioSdkJrePath = androidStudioSdkPath / "jre";
            if (androidStudioSdkJrePath.exists())
                jdkHome = androidStudioSdkJrePath;
        }

        if (jdkHome.isEmpty()) {
            QStringList allVersions;
            QSettings settings("HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\JDK\\",
                               QSettings::NativeFormat);
            allVersions = settings.childGroups();
#ifdef Q_OS_WIN
            if (allVersions.isEmpty()) {
                settings.setDefaultFormat(QSettings::Registry64Format);
                allVersions = settings.childGroups();
            }
#endif // Q_OS_WIN

            // Look for the highest existing JDK
            allVersions.sort();
            std::reverse(allVersions.begin(), allVersions.end()); // Order descending
            for (const QString &version : std::as_const(allVersions)) {
                settings.beginGroup(version);
                jdkHome = FilePath::fromUserInput(settings.value("JavaHome").toString());
                settings.endGroup();
                if (jdkHome.exists())
                    break;
            }
        }
    } else {
        QStringList args;
        if (HostOsInfo::isMacHost())
            args << "-c"
                 << "/usr/libexec/java_home";
        else
            args << "-c"
                 << "readlink -f $(which java)";

        Process findJdkPathProc;
        findJdkPathProc.setCommand({"sh", args});
        findJdkPathProc.start();
        findJdkPathProc.waitForFinished();
        QByteArray jdkPath = findJdkPathProc.readAllRawStandardOutput().trimmed();

        if (HostOsInfo::isMacHost()) {
            jdkHome = FilePath::fromUtf8(jdkPath);
        } else {
            jdkPath.replace("bin/java", ""); // For OpenJDK 11
            jdkPath.replace("jre", "");
            jdkPath.replace("//", "/");
            jdkHome = FilePath::fromUtf8(jdkPath);
        }
    }

    return jdkHome;
}

void AndroidConfigurations::load()
{
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    m_config.load(*settings);
    settings->endGroup();
}

void AndroidConfigurations::updateAndroidDevice()
{
    // Remove any dummy Android device, because it won't be usable.
    DeviceManager *const devMgr = DeviceManager::instance();
    IDevice::ConstPtr dev = devMgr->find(Constants::ANDROID_DEVICE_ID);
    if (dev)
        devMgr->removeDevice(dev->id());

    AndroidDeviceManager::instance()->setupDevicesWatcher();
}

AndroidConfigurations *AndroidConfigurations::m_instance = nullptr;

#ifdef WITH_TESTS
void AndroidPlugin::testAndroidConfigAvailableNdkPlatforms_data()
{
    QTest::addColumn<FilePath>("ndkPath");
    QTest::addColumn<Abis>("abis");
    QTest::addColumn<OsType>("hostOs");
    QTest::addColumn<QList<int> >("expectedPlatforms");

    QTest::newRow("ndkLegacy")
                << FilePath::fromUserInput(":/android/tst/ndk/19.2.5345600")
                << Abis()
                << OsTypeOther
                << QList<int>{28, 27, 26, 24, 23, 22, 21, 19, 18, 17, 16};

    const FilePath ndkV21Plus = FilePath::fromUserInput(":/android/tst/ndk/23.1.7779620");
    const QList<int> abis32Bit = {31, 30, 29, 28, 27, 26, 24, 23, 22, 21, 19, 18, 17, 16};
    const QList<int> abis64Bit = {31, 30, 29, 28, 27, 26, 24, 23, 22, 21};
    QTest::newRow("ndkV21Plus armeabi-v7a OsTypeWindows")
                << ndkV21Plus
                << Abis{AndroidManager::androidAbi2Abi(
                       ProjectExplorer::Constants::ANDROID_ABI_ARMEABI_V7A)}
                << OsTypeWindows
                << abis32Bit;

    QTest::newRow("ndkV21Plus arm64-v8a OsTypeLinux")
                << ndkV21Plus
                << Abis{AndroidManager::androidAbi2Abi(
                       ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A)}
                << OsTypeLinux
                << abis64Bit;

    QTest::newRow("ndkV21Plus x86 OsTypeMac")
                << ndkV21Plus
                << Abis{AndroidManager::androidAbi2Abi(
                       ProjectExplorer::Constants::ANDROID_ABI_X86)}
                << OsTypeMac
                << abis32Bit;

    QTest::newRow("ndkV21Plus x86_64 OsTypeWindows")
                << ndkV21Plus
                << Abis{AndroidManager::androidAbi2Abi(
                       ProjectExplorer::Constants::ANDROID_ABI_X86_64)}
                << OsTypeWindows
                << abis64Bit;
}

void AndroidPlugin::testAndroidConfigAvailableNdkPlatforms()
{
    QFETCH(FilePath, ndkPath);
    QFETCH(Abis, abis);
    QFETCH(OsType, hostOs);
    QFETCH(QList<int>, expectedPlatforms);

    const QList<int> foundPlatforms = availableNdkPlatformsImpl(ndkPath, abis, hostOs);
    QCOMPARE(foundPlatforms, expectedPlatforms);
}
#endif // WITH_TESTS

} // namespace Android
