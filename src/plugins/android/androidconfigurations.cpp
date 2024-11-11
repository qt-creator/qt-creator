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
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/persistentsettings.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>
#include <utils/stringutils.h>

#include <QApplication>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>

#include <functional>
#include <memory>

#ifdef WITH_TESTS
#   include <QTest>
#endif // WITH_TESTS

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Tasking;
using namespace Utils;

namespace {
static Q_LOGGING_CATEGORY(avdConfigLog, "qtc.android.androidconfig", QtWarningMsg)

std::optional<FilePath> tryGetFirstDirectory(const FilePath &path, const QStringList &nameFilters)
{
    std::optional<FilePath> ret;
    path.iterateDirectory(
        [&ret](const FilePath &p) {
            if (p.exists()) {
                ret = p;
                return IterationPolicy::Stop;
            }

            return IterationPolicy::Continue;
        },
        FileFilter{nameFilters, QDir::Dirs});

    return ret;
}
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

const QLatin1String ArmToolsDisplayName("arm");
const QLatin1String X86ToolsDisplayName("i686");
const QLatin1String AArch64ToolsDisplayName("aarch64");
const QLatin1String X86_64ToolsDisplayName("x86_64");

const QLatin1String Unknown("unknown");
const QLatin1String keytoolName("keytool");
const Key changeTimeStamp("ChangeTimeStamp");

const char sdkToolsVersionKey[] = "Pkg.Revision";
const char ndkRevisionKey[] = "Pkg.Revision";

static FilePath sdkSettingsFileName()
{
    return Core::ICore::installerResourcePath("android.xml");
}

static QString ndkPackageMarker()
{
    return QLatin1String(Constants::ndkPackageName) + ";";
}

static QString platformsPackageMarker()
{
    return QLatin1String(Constants::platformsPackageName) + ";";
}

static QString buildToolsPackageMarker()
{
    return QLatin1String(Constants::buildToolsPackageName) + ";";
}

static QString getDeviceProperty(const QString &device, const QString &property)
{
    // workaround for '????????????' serial numbers
    Process adbProc;
    adbProc.setCommand({AndroidConfig::adbToolPath(),
                        {AndroidDeviceInfo::adbSelector(device), "shell", "getprop", property}});
    adbProc.runBlocking();
    if (adbProc.result() == ProcessResult::FinishedWithSuccess)
        return adbProc.allOutput();
    return {};
}

static QLatin1String toolsPrefix(const Abi &abi)
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

static QLatin1String toolchainPrefix(const Abi &abi)
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

static FilePath gdbPathFromNdk(const Abi &abi, const FilePath &ndkLocation)
{
    const FilePath path = ndkLocation.pathAppended(
        QString("prebuilt/%1/bin/gdb%2").arg(AndroidConfig::toolchainHostFromNdk(ndkLocation),
                                             QString(QTC_HOST_EXE_SUFFIX)));
    if (path.exists())
        return path;
    // fallback for old NDKs (e.g. 10e)
    return ndkLocation.pathAppended(QString("toolchains/%1-4.9/prebuilt/%2/bin/%3-gdb%4")
                                        .arg(toolchainPrefix(abi),
                                             AndroidConfig::toolchainHostFromNdk(ndkLocation),
                                             toolsPrefix(abi),
                                             QString(QTC_HOST_EXE_SUFFIX)));
}

static FilePath lldbPathFromNdk(const FilePath &ndkLocation)
{
    const FilePath path = ndkLocation.pathAppended(
        QString("toolchains/llvm/prebuilt/%1/bin/lldb%2")
            .arg(AndroidConfig::toolchainHostFromNdk(ndkLocation), QString(QTC_HOST_EXE_SUFFIX)));
    return path.exists() ? path : FilePath();
}

namespace AndroidConfig {

struct SdkForQtVersions
{
    QList<QVersionNumber> versions;
    QStringList essentialPackages;

    bool containsVersion(const QVersionNumber &qtVersion) const
    {
        return versions.contains(qtVersion)
               || versions.contains(QVersionNumber(qtVersion.majorVersion(),
                                                   qtVersion.minorVersion()));
    }
};

struct AndroidConfigData
{
    void load(const QtcSettings &settings);
    void save(QtcSettings &settings) const;
    void parseDependenciesJson();

    FilePath m_sdkLocation;
    FilePath m_temporarySdkToolsPath;
    QStringList m_sdkManagerToolArgs;
    FilePath m_openJDKLocation;
    FilePath m_keystoreLocation;
    FilePath m_openSslLocation;
    QString m_emulatorArgs;
    bool m_automaticKitCreation = true;
    QUrl m_sdkToolsUrl;
    QByteArray m_sdkToolsSha256;
    QStringList m_commonEssentialPkgs;
    QList<SdkForQtVersions> m_specificQtVersions;
    QStringList m_customNdkList;
    FilePath m_defaultNdk;
    bool m_sdkFullyConfigured = false;
    QHash<QString, QString> m_serialNumberToDeviceName; // cache
};

static AndroidConfigData &config()
{
    static AndroidConfigData theAndroidConfig;
    return theAndroidConfig;
}

static FilePath ndkSubPath(const SdkForQtVersions &packages)
{
    const QString ndkPrefix = ndkPackageMarker();
    for (const QString &package : packages.essentialPackages)
        if (package.startsWith(ndkPrefix))
            return FilePath::fromString(NdksSubDir) / package.sliced(ndkPrefix.length());

    return {};
}

static FilePath ndkSubPathFromQtVersion(const QtVersion &version)
{
    if (auto androidQtVersion = dynamic_cast<const AndroidQtVersion *>(&version)) {
        bool ok;
        const AndroidQtVersion::BuiltWith bw = androidQtVersion->builtWith(&ok);
        if (ok)
            return FilePath::fromString(NdksSubDir) / bw.ndkVersion.toString();
    }

    for (const SdkForQtVersions &item : config().m_specificQtVersions) {
        if (item.containsVersion(version.qtVersion()))
            return ndkSubPath(item);
    }
    return {};
}

//////////////////////////////////
// AndroidConfig
//////////////////////////////////

QLatin1String displayName(const Abi &abi)
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

void AndroidConfigData::load(const QtcSettings &settings)
{
    // user settings
    QVariant emulatorArgs = settings.value(EmulatorArgsKey, QString("-netdelay none -netspeed full"));
    if (emulatorArgs.typeId() == QMetaType::QStringList) // Changed in 8.0 from QStringList to QString.
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
    if (reader.load(sdkSettingsFileName())
            && settings.value(changeTimeStamp).toInt() != sdkSettingsFileName().lastModified().toMSecsSinceEpoch() / 1000) {
        // persisten settings
        m_sdkLocation = FilePath::fromSettings(
                            reader.restoreValue(SDKLocationKey, m_sdkLocation.toSettings()))
                            .cleanPath();
        m_customNdkList = reader.restoreValue(CustomNdkLocationsKey).toStringList();
        m_sdkManagerToolArgs = reader.restoreValue(SDKManagerToolArgsKey, m_sdkManagerToolArgs).toStringList();
        m_openJDKLocation = FilePath::fromSettings(
            reader.restoreValue(OpenJDKLocationKey, m_openJDKLocation.toSettings()));
        m_openSslLocation = FilePath::fromSettings(
            reader.restoreValue(OpenSslPriLocationKey, m_openSslLocation.toSettings()));
        m_automaticKitCreation = reader.restoreValue(AutomaticKitCreationKey, m_automaticKitCreation).toBool();
        m_sdkFullyConfigured = reader.restoreValue(SdkFullyConfiguredKey, m_sdkFullyConfigured).toBool();
        // persistent settings
    }
    m_customNdkList.removeAll("");
    if (!m_defaultNdk.isEmpty() && AndroidConfig::ndkVersion(m_defaultNdk).isNull()) {
        if (avdConfigLog().isDebugEnabled())
            qCDebug(avdConfigLog).noquote() << "Clearing invalid default NDK setting:"
                                            << m_defaultNdk.toUserOutput();
        m_defaultNdk.clear();
    }
    parseDependenciesJson();
}

void AndroidConfigData::save(QtcSettings &settings) const
{
    const FilePath sdkSettingsFile = sdkSettingsFileName();
    if (sdkSettingsFile.exists())
        settings.setValue(changeTimeStamp, sdkSettingsFile.lastModified().toMSecsSinceEpoch() / 1000);

    // user settings
    settings.setValue(SDKLocationKey, m_sdkLocation.toSettings());
    settings.setValue(CustomNdkLocationsKey, m_customNdkList);
    settings.setValue(DefaultNdkLocationKey, m_defaultNdk.toSettings());
    settings.setValue(SDKManagerToolArgsKey, m_sdkManagerToolArgs);
    settings.setValue(OpenJDKLocationKey, m_openJDKLocation.toSettings());
    settings.setValue(OpenSslPriLocationKey, m_openSslLocation.toSettings());
    settings.setValue(EmulatorArgsKey, m_emulatorArgs);
    settings.setValue(AutomaticKitCreationKey, m_automaticKitCreation);
    settings.setValue(SdkFullyConfiguredKey, m_sdkFullyConfigured);
}

void AndroidConfigData::parseDependenciesJson()
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

    const expected_str<QByteArray> result = sdkConfigUserFile.fileContents();
    if (!result) {
        qCDebug(
            avdConfigLog,
            "Couldn't open JSON config file %s. %s",
            qPrintable(sdkConfigUserFile.fileName()),
            qPrintable(result.error()));
        return;
    }

    QJsonObject jsonObject = QJsonDocument::fromJson(*result).object();

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

            m_specificQtVersions.append(specificVersion);
        }
    }
}

static QList<int> availableNdkPlatformsLegacy(const FilePath &ndkLocation)
{
    QList<int> availableNdkPlatforms;

    ndkLocation.pathAppended("platforms")
        .iterateDirectory(
            [&availableNdkPlatforms](const FilePath &filePath) {
                const QString fileName = filePath.fileName();
                availableNdkPlatforms.push_back(fileName.mid(fileName.lastIndexOf('-') + 1).toInt());
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

    const QString abi = toolsPrefix(abis.first());
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

static QList<int> availableNdkPlatforms(const QtVersion *qtVersion)
{
    return availableNdkPlatformsImpl(AndroidConfig::ndkLocation(qtVersion), qtVersion->qtAbis(),
                                     HostOsInfo::hostOs());
}

QStringList getCustomNdkList() { return config().m_customNdkList; }

void addCustomNdk(const QString &customNdk)
{
    if (!config().m_customNdkList.contains(customNdk))
        config().m_customNdkList.append(customNdk);
}

void removeCustomNdk(const QString &customNdk)
{
    config().m_customNdkList.removeAll(customNdk);
}

void setDefaultNdk(const FilePath &defaultNdk) { config().m_defaultNdk = defaultNdk; }

FilePath defaultNdk() { return config().m_defaultNdk; }

FilePath openSslLocation() { return config().m_openSslLocation; }

void setOpenSslLocation(const FilePath &openSslLocation)
{
    config().m_openSslLocation = openSslLocation;
}

QStringList apiLevelNamesFor(const SdkPlatformList &platforms)
{
    return Utils::transform(platforms, AndroidConfig::apiLevelNameFor);
}

QString apiLevelNameFor(const SdkPlatform *platform)
{
    if (platform && platform->apiLevel() > 0) {
        QString sdkStylePath = platform->sdkStylePath();
        return sdkStylePath.remove("platforms;");
    }
    return {};
}

FilePath adbToolPath()
{
    return config().m_sdkLocation.pathAppended("platform-tools/adb").withExecutableSuffix();
}

FilePath emulatorToolPath()
{
    const FilePath emulatorFile
        = config().m_sdkLocation.pathAppended("emulator/emulator").withExecutableSuffix();
    return emulatorFile.exists() ? emulatorFile : FilePath();
}

FilePath sdkManagerToolPath()
{
    const FilePath sdkmanagerPath = config()
                                        .m_sdkLocation.pathAppended(Constants::cmdlineToolsName)
                                        .pathAppended("latest/bin/sdkmanager" ANDROID_BAT_SUFFIX);
    if (sdkmanagerPath.exists())
        return sdkmanagerPath;

    // If it's a first time install use the path of Constants::cmdlineToolsName temporary download
    const FilePath sdkmanagerTmpPath = config().m_temporarySdkToolsPath.pathAppended(
        "/bin/sdkmanager" ANDROID_BAT_SUFFIX);
    return sdkmanagerTmpPath.exists() ? sdkmanagerTmpPath : FilePath();
}

FilePath avdManagerToolPath()
{
    const FilePath sdkmanagerPath = config()
                                        .m_sdkLocation.pathAppended(Constants::cmdlineToolsName)
                                        .pathAppended("/latest/bin/avdmanager" ANDROID_BAT_SUFFIX);
    return sdkmanagerPath.exists() ? sdkmanagerPath : FilePath();
}

void setTemporarySdkToolsPath(const FilePath &path)
{
    config().m_temporarySdkToolsPath = path;
}

FilePath toolchainPathFromNdk(const FilePath &ndkLocation, OsType hostOs)
{
    const FilePath tcPath = ndkLocation / "toolchains";

    const std::optional<FilePath> llvmDir = tryGetFirstDirectory(tcPath, {"llvm*"});
    if (!llvmDir)
        return {};

    const FilePath prebuiltDir = *llvmDir / "prebuilt";

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

    const std::optional<FilePath> toolchainDir = tryGetFirstDirectory(prebuiltDir, hostPatterns);
    if (!toolchainDir)
        return {};

    return *toolchainDir;
}

FilePath toolchainPath(const QtVersion *qtVersion)
{
    return toolchainPathFromNdk(ndkLocation(qtVersion));
}

FilePath clangPathFromNdk(const FilePath &ndkLocation)
{
    const FilePath path = toolchainPathFromNdk(ndkLocation);
    if (path.isEmpty())
        return {};
    return path.pathAppended("bin/clang").withExecutableSuffix();
}

FilePath makePathFromNdk(const FilePath &ndkLocation)
{
    return ndkLocation.pathAppended(
                QString("prebuilt/%1/bin/make%2").arg(toolchainHostFromNdk(ndkLocation),
                                                      QString(QTC_HOST_EXE_SUFFIX)));
}

static FilePath openJDKBinPath()
{
    const FilePath path = config().m_openJDKLocation;
    return path.isEmpty() ? path : path.pathAppended("bin");
}

FilePath keytoolPath()
{
    return openJDKBinPath().pathAppended(keytoolName).withExecutableSuffix();
}

QStringList devicesCommandOutput()
{
    Process adbProcess;
    adbProcess.setCommand({adbToolPath(), {"devices"}});
    adbProcess.runBlocking();
    if (adbProcess.result() != ProcessResult::FinishedWithSuccess)
        return {};

    // mid(1) - remove "List of devices attached" header line.
    // Example output: "List of devices attached\nemulator-5554\tdevice\n\n".
    return adbProcess.allOutput().split('\n', Qt::SkipEmptyParts).mid(1);
}

ExecutableItem devicesCommandOutputRecipe(const Storage<QStringList> &outputStorage)
{
    const auto onSetup = [](Process &process) { process.setCommand({adbToolPath(), {"devices"}}); };
    const auto onDone = [outputStorage](const Process &process) {
        // mid(1) - remove "List of devices attached" header line.
        // Example output: "List of devices attached\nemulator-5554\tdevice\n\n".
        *outputStorage = process.allOutput().split('\n', Qt::SkipEmptyParts).mid(1);
    };
    return ProcessTask(onSetup, onDone);
}

bool sdkFullyConfigured() { return config().m_sdkFullyConfigured; }

void setSdkFullyConfigured(bool allEssentialsInstalled)
{
    config().m_sdkFullyConfigured = allEssentialsInstalled;
}

int getSDKVersion(const QString &device)
{
    const QString tmp = getDeviceProperty(device, "ro.build.version.sdk");
    return tmp.isEmpty() ? -1 : tmp.trimmed().toInt();
}

//!
//! \brief AndroidConfig::getProductModel
//! \param device serial number
//! \return the produce model of the device or if that cannot be read the serial number
//!
QString getProductModel(const QString &device)
{
    if (config().m_serialNumberToDeviceName.contains(device))
        return config().m_serialNumberToDeviceName.value(device);

    const QString model = getDeviceProperty(device, "ro.product.model").trimmed();
    if (model.isEmpty())
        return device;

    if (!device.startsWith("????"))
        config().m_serialNumberToDeviceName.insert(device, model);
    return model;
}

QStringList getAbis(const QString &device)
{
    const FilePath adbTool = AndroidConfig::adbToolPath();
    QStringList result;
    // First try via ro.product.cpu.abilist
    Process adbProc;
    adbProc.setCommand({adbTool,
        {AndroidDeviceInfo::adbSelector(device), "shell", "getprop", "ro.product.cpu.abilist"}});
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
        CommandLine cmd{adbTool, {AndroidDeviceInfo::adbSelector(device), "shell", "getprop"}};
        if (i == 1)
            cmd.addArg("ro.product.cpu.abi");
        else
            cmd.addArg(QString::fromLatin1("ro.product.cpu.abi%1").arg(i));
        Process abiProc;
        abiProc.setCommand(cmd);
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

bool isValidNdk(const QString &ndkLocation)
{
    const FilePath ndkPath = FilePath::fromUserInput(ndkLocation);

    if (!ndkPath.exists() || !ndkPath.pathAppended("toolchains").exists())
        return false;

    const QVersionNumber version = AndroidConfig::ndkVersion(ndkPath);
    if (version.isNull())
        return false;

    const FilePath ndkPlatformsDir = ndkPath.pathAppended("platforms");
    if (version.majorVersion() <= 22
        && (!ndkPlatformsDir.exists() || ndkPlatformsDir.pathView().contains(' ')))
        return false;
    return true;
}

QString bestNdkPlatformMatch(int target, const QtVersion *qtVersion)
{
    target = std::max(AndroidManager::defaultMinimumSDK(qtVersion), target);
    const QList<int> platforms = availableNdkPlatforms(qtVersion);
    for (const int apiLevel : platforms) {
        if (apiLevel <= target)
            return QString::fromLatin1("android-%1").arg(apiLevel);
    }
    return QString("android-%1").arg(AndroidManager::defaultMinimumSDK(qtVersion));
}

FilePath sdkLocation()
{
    return config().m_sdkLocation;
}

void setSdkLocation(const FilePath &sdkLocation)
{
    config().m_sdkLocation = sdkLocation;
}

static FilePath sdkToolsVersionPath()
{
    const FilePath sdkVersionPaths = config()
                                         .m_sdkLocation.pathAppended(Constants::cmdlineToolsName)
                                         .pathAppended("/latest/source.properties");
    if (sdkVersionPaths.exists())
        return sdkVersionPaths;

    // If it's a first time install use the path of Constants::cmdlineToolsName temporary download
    const FilePath tmpSdkPath = config().m_temporarySdkToolsPath.pathAppended("source.properties");
    if (tmpSdkPath.exists())
        return tmpSdkPath;

    return {};
}

QVersionNumber sdkToolsVersion()
{
    if (!config().m_sdkLocation.exists())
        return {};

    const FilePath sdkToolsPropertiesPath = sdkToolsVersionPath();
    const QSettings settings(sdkToolsPropertiesPath.toFSPathString(), QSettings::IniFormat);
    return QVersionNumber::fromString(settings.value(sdkToolsVersionKey).toString());
}

QVersionNumber buildToolsVersion()
{
    //TODO: return version according to qt version
    QVersionNumber maxVersion;
    const FilePath buildToolsPath = config().m_sdkLocation.pathAppended("build-tools");
    for (const FilePath &file : buildToolsPath.dirEntries(QDir::Dirs | QDir::NoDotAndDotDot))
        maxVersion = qMax(maxVersion, QVersionNumber::fromString(file.fileName()));
    return maxVersion;
}

QStringList sdkManagerToolArgs() { return config().m_sdkManagerToolArgs; }

void setSdkManagerToolArgs(const QStringList &args) { config().m_sdkManagerToolArgs = args; }

FilePath ndkLocation(const QtVersion *qtVersion)
{
    if (!config().m_defaultNdk.isEmpty())
        return config().m_defaultNdk; // A selected default NDK is good for any Qt version
    return sdkLocation().resolvePath(ndkSubPathFromQtVersion(*qtVersion));
}

QVersionNumber ndkVersion(const QtVersion *qtVersion)
{
    return ndkVersion(ndkLocation(qtVersion));
}

QVersionNumber ndkVersion(const FilePath &ndkPath)
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
        QSettings settings(ndkPropertiesPath.toFSPathString(), QSettings::IniFormat);
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

QUrl sdkToolsUrl() { return config().m_sdkToolsUrl; }

QByteArray getSdkToolsSha256() { return config().m_sdkToolsSha256; }

static QStringList commonEssentials()
{
    return config().m_commonEssentialPkgs;
}

static QString essentialBuiltWithBuildToolsPackage(int builtWithApiVersion)
{
    // For build-tools, to avoid the situation of potentially having the essential packages
    // invalidated whenever a new minor version is released, check if any version with major
    // version matching builtWith apiVersion and use it as essential, otherwise use the any
    // other one that has an minimum major version of builtWith apiVersion.
    const BuildToolsList buildTools =
        AndroidConfigurations::sdkManager()->filteredBuildTools(builtWithApiVersion);
    const BuildToolsList apiBuildTools
        = Utils::filtered(buildTools, [builtWithApiVersion] (const BuildTools *pkg) {
              return pkg->revision().majorVersion() == builtWithApiVersion; });
    const QString installedBuildTool = [apiBuildTools] () -> QString {
        for (const BuildTools *pkg : apiBuildTools) {
            if (pkg->state() == AndroidSdkPackage::Installed)
                return pkg->sdkStylePath();
        }
        return {};
    }();

    if (installedBuildTool.isEmpty()) {
        if (!apiBuildTools.isEmpty())
            return apiBuildTools.first()->sdkStylePath();
        else if (!buildTools.isEmpty())
            return buildTools.first()->sdkStylePath();
        // This means there's  something wrong with sdkmanager, return a default version anyway
        else
            return buildToolsPackageMarker() + QString::number(builtWithApiVersion) + ".0.0";
    }

    return installedBuildTool;
}

static QStringList essentialsFromQtVersion(const QtVersion &version)
{
    if (auto androidQtVersion = dynamic_cast<const AndroidQtVersion *>(&version)) {
        bool ok;
        const AndroidQtVersion::BuiltWith bw = androidQtVersion->builtWith(&ok);
        if (ok) {
            QStringList builtWithPackages;
            builtWithPackages.append(ndkPackageMarker() + bw.ndkVersion.toString());
            const QString apiVersion = QString::number(bw.apiVersion);
            builtWithPackages.append(platformsPackageMarker() + "android-" + apiVersion);
            builtWithPackages.append(essentialBuiltWithBuildToolsPackage(bw.apiVersion));

            return builtWithPackages;
        }
    }

    const QVersionNumber qtVersion = version.qtVersion();
    for (const SdkForQtVersions &item : config().m_specificQtVersions) {
        if (item.containsVersion(qtVersion))
            return item.essentialPackages;
    }
    return {};
}

QStringList allEssentials()
{
    const QtVersions installedVersions = QtVersionManager::versions(&QtVersion::isAndroidQtVersion);

    QStringList allPackages(commonEssentials());
    for (const QtVersion *version : installedVersions)
        allPackages.append(essentialsFromQtVersion(*version));
    allPackages.removeDuplicates();

    return allPackages;
}

QString optionalSystemImagePackage()
{
    const QStringList essentialPkgs(allEssentials());
    QStringList platforms = Utils::filtered(essentialPkgs, [](const QString &item) {
        return item.startsWith(platformsPackageMarker(), Qt::CaseInsensitive);
    });
    if (platforms.isEmpty())
        return {};

    platforms.sort();
    const QStringList platformBits = platforms.last().split('-');
    if (platformBits.isEmpty())
        return {};

    const int apiLevel = platformBits.last().toInt();
    if (apiLevel < 1)
        return {};

    QString hostArch = QSysInfo::currentCpuArchitecture();
    if (hostArch == "arm64")
        hostArch = ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A;
    else if (hostArch == "i386")
        hostArch = ProjectExplorer::Constants::ANDROID_ABI_X86;
    else if (hostArch == "arm")
        hostArch = ProjectExplorer::Constants::ANDROID_ABI_ARMEABI_V7A;

    const auto imageName = QLatin1String("%1;android-%2;google_apis_playstore;%3")
                               .arg(Constants::systemImagesPackageName).arg(apiLevel).arg(hostArch);

    const SdkPlatformList sdkPlatforms = AndroidConfigurations::sdkManager()->filteredSdkPlatforms(
        apiLevel, AndroidSdkPackage::AnyValidState);

    if (sdkPlatforms.isEmpty())
        return {};

    const SystemImageList images = sdkPlatforms.first()->systemImages(AndroidSdkPackage::Available);
    for (const SystemImage *img : images) {
        if (img->sdkStylePath() == imageName)
            return imageName;
    }

    return {};
}

static QStringList packagesWithoutNdks(const QStringList &packages)
{
    return Utils::filtered(packages, [] (const QString &p) {
        return !p.startsWith(ndkPackageMarker());
    });
}

bool allEssentialsInstalled()
{
    QStringList essentialPkgs(allEssentials());
    const auto installedPkgs = AndroidConfigurations::sdkManager()->installedSdkPackages();
    for (const AndroidSdkPackage *pkg : installedPkgs) {
        if (essentialPkgs.contains(pkg->sdkStylePath()))
            essentialPkgs.removeOne(pkg->sdkStylePath());
        if (essentialPkgs.isEmpty())
            break;
    }
    if (!config().m_defaultNdk.isEmpty())
        essentialPkgs = packagesWithoutNdks(essentialPkgs);
    return essentialPkgs.isEmpty() ? true : false;
}

bool sdkToolsOk()
{
    const bool exists = sdkLocation().exists();
    const bool writable = sdkLocation().isWritableDir();
    const bool sdkToolsExist = !sdkToolsVersion().isNull();
    return exists && writable && sdkToolsExist;
}

FilePath openJDKLocation() { return config().m_openJDKLocation; }

void setOpenJDKLocation(const FilePath &openJDKLocation)
{
    config().m_openJDKLocation = openJDKLocation;
}

QString toolchainHost(const QtVersion *qtVersion)
{
    return toolchainHostFromNdk(ndkLocation(qtVersion));
}

QString toolchainHostFromNdk(const FilePath &ndkPath)
{
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
    default: /* unknown host */
        return {};
    }

    const std::optional<FilePath> toolchainDir
        = tryGetFirstDirectory(ndkPath.pathAppended("prebuilt"), hostPatterns);
    if (!toolchainDir)
        return {};

    return toolchainDir->fileName();
}

QString emulatorArgs() { return config().m_emulatorArgs; }

void setEmulatorArgs(const QString &args) { config().m_emulatorArgs = args; }

bool automaticKitCreation() { return config().m_automaticKitCreation; }

void setAutomaticKitCreation(bool b) { config().m_automaticKitCreation = b; }

FilePath defaultSdkPath()
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

Environment toolsEnvironment()
{
    Environment env = Environment::systemEnvironment();
    FilePath jdkLocation = openJDKLocation();
    if (!jdkLocation.isEmpty()) {
        env.set(Constants::JAVA_HOME_ENV_VAR, jdkLocation.toUserOutput());
        env.prependOrSetPath(jdkLocation.pathAppended("bin"));
    }
    return env;
}

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

FilePath getJdkPath()
{
    FilePath jdkHome = FilePath::fromString(qtcEnvironmentVariable(Constants::JAVA_HOME_ENV_VAR));
    if (jdkHome.exists())
        return jdkHome;

    if (HostOsInfo::isWindowsHost()) {
        // Look for Android Studio's jdk first
        const FilePath androidStudioSdkPath = androidStudioPath();
        if (!androidStudioSdkPath.isEmpty()) {
            const QStringList allVersions{"jbr", "jre"};
            for (const QString &version : allVersions) {
                const FilePath androidStudioSdkJbrPath = androidStudioSdkPath / version;
                if (androidStudioSdkJbrPath.exists())
                    return androidStudioSdkJbrPath;
            }
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
        findJdkPathProc.runBlocking();
        QByteArray jdkPath = findJdkPathProc.rawStdOut().trimmed();

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

} // namespace AndroidConfig

///////////////////////////////////
// AndroidConfigurations
///////////////////////////////////

AndroidConfigurations *m_instance = nullptr;

AndroidConfigurations::AndroidConfigurations()
    : m_sdkManager(new AndroidSdkManager)
{
    load();
    connect(DeviceManager::instance(), &DeviceManager::devicesLoaded,
            this, &AndroidConfigurations::updateAndroidDevice);

    m_instance = this;
}

void AndroidConfigurations::applyConfig()
{
    emit m_instance->aboutToUpdate();
    m_instance->save();
    updateAndroidDevice();
    registerNewToolchains();
    updateAutomaticKitList();
    removeOldToolchains();
    emit m_instance->updated();
}

static bool matchKit(const ToolchainBundle &bundle, const Kit &kit)
{
    using namespace ProjectExplorer::Constants;
    for (const Id lang : {Id(C_LANGUAGE_ID), Id(CXX_LANGUAGE_ID)}) {
        const Toolchain * const tc = ToolchainKitAspect::toolchain(&kit, lang);
        if (!tc || tc->typeId() != Constants::ANDROID_TOOLCHAIN_TYPEID
            || tc->targetAbi() != bundle.targetAbi()) {
            return false;
        }
    }
    return true;
}

void AndroidConfigurations::registerNewToolchains()
{
    const Toolchains existingAndroidToolchains
            = ToolchainManager::toolchains(Utils::equal(&Toolchain::typeId, Id(Constants::ANDROID_TOOLCHAIN_TYPEID)));
    ToolchainManager::registerToolchains(autodetectToolchains(existingAndroidToolchains));
    registerCustomToolchainsAndDebuggers();
}

void AndroidConfigurations::removeOldToolchains()
{
    const auto invalidAndroidTcs = ToolchainManager::toolchains([](const Toolchain *tc) {
        return tc->id() == Constants::ANDROID_TOOLCHAIN_TYPEID && !tc->isValid();
    });
    ToolchainManager::deregisterToolchains(invalidAndroidTcs);
}

void AndroidConfigurations::removeUnusedDebuggers()
{
    const QtVersions qtVersions = QtVersionManager::versions([](const QtVersion *v) {
        return v->type() == Constants::ANDROID_QT_TYPE;
    });

    QList<FilePath> uniqueNdks;
    for (const QtVersion *qt : qtVersions) {
        FilePath ndkLocation = AndroidConfig::ndkLocation(qt);
        if (!uniqueNdks.contains(ndkLocation))
            uniqueNdks.append(ndkLocation);
    }

    uniqueNdks.append(FileUtils::toFilePathList(AndroidConfig::getCustomNdkList()));

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

static QVariant findOrRegisterDebugger(Toolchain *tc,
                                       const QStringList &abisList,
                                       bool customDebugger = false)
{
    const FilePath ndk = static_cast<AndroidToolchain *>(tc)->ndkLocation();
    const FilePath lldbCommand = lldbPathFromNdk(ndk);
    const Debugger::DebuggerItem *existingLldb = existingDebugger(lldbCommand,
                                                                  Debugger::LldbEngineType);
    // Return existing debugger with same command - prefer lldb (limit to sdk/ndk min version?)
    if (existingLldb)
        return existingLldb->id();

    const FilePath gdbCommand = gdbPathFromNdk(tc->targetAbi(), ndk);

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
                .arg(AndroidConfig::ndkVersion(ndk).toString())
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
    const bool oldNdkVersion = AndroidConfig::ndkVersion(ndk) <= QVersionNumber{11};
    debugger.setUnexpandedDisplayName(custom + mainName
            .arg(getMultiOrSingleAbiString(oldNdkVersion ? abisList : allSupportedAbis()))
            .arg(AndroidConfig::ndkVersion(ndk).toString())
                                      + ' ' + debugger.engineTypeName());
    debugger.setAutoDetected(true);
    debugger.reinitializeFromFile();
    QVariant registeredGdb = Debugger::DebuggerItemManager::registerDebugger(debugger);
    return registeredLldb.isNull() ? registeredGdb : registeredLldb;
}

void AndroidConfigurations::registerCustomToolchainsAndDebuggers()
{
    const Toolchains existingAndroidToolchains = ToolchainManager::toolchains(
        Utils::equal(&Toolchain::typeId, Id(Constants::ANDROID_TOOLCHAIN_TYPEID)));

    const FilePaths customNdks = FileUtils::toFilePathList(AndroidConfig::getCustomNdkList());
    const Toolchains customToolchains
        = autodetectToolchainsFromNdks(existingAndroidToolchains, customNdks, true);

    ToolchainManager::registerToolchains(customToolchains);
    for (Toolchain *tc : customToolchains) {
        const auto androidToolchain = static_cast<AndroidToolchain *>(tc);
        QString abiStr;
        if (androidToolchain)
            abiStr = androidToolchain->platformLinkerFlags().at(1).split('-').first();
        findOrRegisterDebugger(tc, {abiStr}, true);
    }
}
void AndroidConfigurations::updateAutomaticKitList()
{
    for (Kit *k : KitManager::kits()) {
        if (DeviceTypeKitAspect::deviceTypeId(k) == Constants::ANDROID_DEVICE_TYPE) {
            if (k->value(Constants::ANDROID_KIT_NDK).isNull() || k->value(Constants::ANDROID_KIT_SDK).isNull()) {
                if (QtVersion *qt = QtKitAspect::qtVersion(k)) {
                    k->setValueSilently(
                        Constants::ANDROID_KIT_NDK, AndroidConfig::ndkLocation(qt).toSettings());
                    k->setValue(Constants::ANDROID_KIT_SDK, AndroidConfig::sdkLocation().toSettings());
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
    const QtVersions qtVersions = QtVersionManager::versions([](const QtVersion *v) {
        return v->type() == Constants::ANDROID_QT_TYPE;
    });
    for (const QtVersion *qtVersion : qtVersions) {
        const Abis qtAbis = qtVersion->qtAbis();
        if (qtAbis.empty())
            continue;
        qtVersionsForArch[qtAbis.first()].append(qtVersion);
    }

    // register new kits

    const QList<ToolchainBundle> bundles = Utils::filtered(
        ToolchainBundle::collectBundles(
            ToolchainManager::toolchains([](const Toolchain *tc) {
                return tc->isAutoDetected() && tc->typeId() == Constants::ANDROID_TOOLCHAIN_TYPEID;
            }),
            ToolchainBundle::HandleMissing::CreateAndRegister),
        [](const ToolchainBundle &b) { return b.isCompletelyValid(); });

    QList<Kit *> unhandledKits = existingKits;
    for (const ToolchainBundle &bundle : bundles) {
        for (const QtVersion *qt : qtVersionsForArch.value(bundle.targetAbi())) {
            const FilePath tcNdk = bundle.get(&AndroidToolchain::ndkLocation);
            if (tcNdk != AndroidConfig::ndkLocation(qt))
                continue;

            Kit *existingKit = Utils::findOrDefault(existingKits, [&](const Kit *b) {
                if (qt != QtKitAspect::qtVersion(b))
                    return false;
                return matchKit(bundle, *b);
            });

            const auto initializeKit = [&bundle, qt](Kit *k) {
                k->setAutoDetected(true);
                k->setAutoDetectionSource("AndroidConfiguration");
                DeviceTypeKitAspect::setDeviceTypeId(k, Constants::ANDROID_DEVICE_TYPE);
                ToolchainKitAspect::setBundle(k, bundle);
                QtKitAspect::setQtVersion(k, qt);
                QStringList abis = static_cast<const AndroidQtVersion *>(qt)->androidAbis();
                Debugger::DebuggerKitAspect::setDebugger(
                    k, findOrRegisterDebugger(bundle.toolchains().first(), abis));

                BuildDeviceKitAspect::setDeviceId(k, DeviceManager::defaultDesktopDevice()->id());
                k->setSticky(QtKitAspect::id(), true);
                k->setSticky(DeviceTypeKitAspect::id(), true);

                QString versionStr = QLatin1String("Qt %{Qt:Version}");
                if (!qt->isAutodetected())
                    versionStr = QString("%1").arg(qt->displayName());
                k->setUnexpandedDisplayName(Tr::tr("Android %1 Clang %2")
                                                .arg(versionStr)
                                                .arg(getMultiOrSingleAbiString(abis)));
                k->setValueSilently(
                    Constants::ANDROID_KIT_NDK, AndroidConfig::ndkLocation(qt).toSettings());
                k->setValueSilently(
                    Constants::ANDROID_KIT_SDK, AndroidConfig::sdkLocation().toSettings());
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
    KitManager::deregisterKits(unhandledKits);
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
    AndroidConfig::config().save(*settings);
    settings->endGroup();
}

void AndroidConfigurations::load()
{
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    AndroidConfig::config().load(*settings);
    settings->endGroup();
}

void AndroidConfigurations::updateAndroidDevice()
{
    // Remove any dummy Android device, because it won't be usable.
    DeviceManager *const devMgr = DeviceManager::instance();
    IDevice::ConstPtr dev = devMgr->find(Constants::ANDROID_DEVICE_ID);
    if (dev)
        devMgr->removeDevice(dev->id());
    setupDevicesWatcher();
}

#ifdef WITH_TESTS

class AndroidConfigurationsTest final : public QObject
{
    Q_OBJECT

private slots:
   void testAndroidConfigAvailableNdkPlatforms_data();
   void testAndroidConfigAvailableNdkPlatforms();
};

void AndroidConfigurationsTest::testAndroidConfigAvailableNdkPlatforms_data()
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

void AndroidConfigurationsTest::testAndroidConfigAvailableNdkPlatforms()
{
    QFETCH(FilePath, ndkPath);
    QFETCH(Abis, abis);
    QFETCH(OsType, hostOs);
    QFETCH(QList<int>, expectedPlatforms);

    const QList<int> foundPlatforms = AndroidConfig::availableNdkPlatformsImpl(ndkPath, abis, hostOs);
    QCOMPARE(foundPlatforms, expectedPlatforms);
}

QObject *createAndroidConfigurationsTest()
{
    return new AndroidConfigurationsTest;
}

#endif // WITH_TESTS

void setupAndroidConfigurations()
{
    static AndroidConfigurations theAndroidConfigurations;
}

} // namespace Android

#include "androidconfigurations.moc"
