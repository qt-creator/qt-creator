// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iosconfigurations.h"

#include "iosconstants.h"
#include "iosdevice.h"
#include "iosprobe.h"
#include "iossimulator.h"
#include "iostr.h"
#include "simulatorcontrol.h"

#include <coreplugin/icore.h>

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/sysrootkitaspect.h>
#include <projectexplorer/toolchainkitaspect.h>
#include <projectexplorer/toolchainconfigwidget.h>

#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggerkitaspect.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtversionfactory.h>

#include <utils/algorithm.h>
#include <utils/futuresynchronizer.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDir>
#include <QDomDocument>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QHash>
#include <QList>
#include <QLoggingCategory>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QTimer>

#include <memory>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;
using namespace Debugger;

namespace {
static Q_LOGGING_CATEGORY(kitSetupLog, "qtc.ios.kitSetup", QtWarningMsg)
static Q_LOGGING_CATEGORY(iosCommonLog, "qtc.ios.common", QtWarningMsg)
}

using ToolchainPair = std::pair<GccToolchain *, GccToolchain *>;
namespace Ios {
namespace Internal {

const bool IgnoreAllDevicesDefault = false;

const char SettingsGroup[] = "IosConfigurations";
const char ignoreAllDevicesKey[] = "IgnoreAllDevices";

const char freeTeamTag[] = "isFreeProvisioningTeam";
const char emailTag[] = "eMail";
const char teamNameTag[] = "teamName";
const char teamIdTag[] = "teamID";

const char udidTag[] = "UUID";
const char profileNameTag[] = "Name";
const char appIdTag[] = "AppIDName";
const char expirationDateTag[] = "ExpirationDate";
const char profileTeamIdTag[] = "TeamIdentifier";

static const QString xcodePlistPath = QDir::homePath() + "/Library/Preferences/com.apple.dt.Xcode.plist";
static const QString provisioningProfileDirPath = QDir::homePath() + "/Library/MobileDevice/Provisioning Profiles";

static Id deviceId(const QString &sdkName)
{
    if (sdkName.startsWith("iphoneos", Qt::CaseInsensitive))
        return Constants::IOS_DEVICE_TYPE;
    else if (sdkName.startsWith("iphonesimulator", Qt::CaseInsensitive))
        return Constants::IOS_SIMULATOR_TYPE;
    return {};
}

static bool isSimulatorDeviceId(const Id &id)
{
    return id == Constants::IOS_SIMULATOR_TYPE;
}

static QList<GccToolchain *> clangToolchains(const Toolchains &toolchains)
{
    QList<GccToolchain *> clangToolchains;
    for (Toolchain *toolchain : toolchains)
        if (toolchain->typeId() == ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID)
            clangToolchains.append(static_cast<GccToolchain *>(toolchain));
    return clangToolchains;
}

static QList<GccToolchain *> autoDetectedIosToolchains()
{
    const QList<GccToolchain *> toolchains = clangToolchains(ToolchainManager::toolchains());
    return filtered(toolchains, [](GccToolchain *toolChain) {
        return toolChain->isAutoDetected()
               && (toolChain->displayName().startsWith("iphone")
                   || toolChain->displayName().startsWith("Apple Clang")); // TODO tool chains should be marked directly
    });
}

static ToolchainPair findToolchainForPlatform(const XcodePlatform &platform,
                                              const XcodePlatform::ToolchainTarget &target,
                                              const QList<GccToolchain *> &toolchains)
{
    ToolchainPair platformToolchains;
    auto toolchainMatch = [](GccToolchain *toolChain, const FilePath &compilerPath, const QStringList &flags) {
        return compilerPath == toolChain->compilerCommand()
                && flags == toolChain->platformCodeGenFlags()
                && flags == toolChain->platformLinkerFlags();
    };
    platformToolchains.first = findOrDefault(toolchains, std::bind(toolchainMatch, std::placeholders::_1,
                                                                  platform.cCompilerPath,
                                                                  target.backendFlags));
    platformToolchains.second = findOrDefault(toolchains, std::bind(toolchainMatch, std::placeholders::_1,
                                                                   platform.cxxCompilerPath,
                                                                   target.backendFlags));
    return platformToolchains;
}

static QHash<XcodePlatform::ToolchainTarget, ToolchainPair> findToolchains(const QList<XcodePlatform> &platforms)
{
    QHash<XcodePlatform::ToolchainTarget, ToolchainPair> platformToolchainHash;
    const QList<GccToolchain *> toolchains = autoDetectedIosToolchains();
    for (const XcodePlatform &platform : platforms) {
        for (const XcodePlatform::ToolchainTarget &target : platform.targets) {
            ToolchainPair platformToolchains = findToolchainForPlatform(platform, target,
                                                                        toolchains);
            if (platformToolchains.first || platformToolchains.second)
                platformToolchainHash.insert(target, platformToolchains);
        }
    }
    return platformToolchainHash;
}

static QSet<Kit *> existingAutoDetectedIosKits()
{
    return toSet(filtered(KitManager::kits(), [](Kit *kit) -> bool {
        Id deviceKind = RunDeviceTypeKitAspect::deviceTypeId(kit);
        return kit->isAutoDetected() && (deviceKind == Constants::IOS_DEVICE_TYPE
                                         || deviceKind == Constants::IOS_SIMULATOR_TYPE);
    }));
}

static void printKits(const QSet<Kit *> &kits)
{
    for (const Kit *kit : kits)
        qCDebug(kitSetupLog) << "  -" << kit->displayName();
}

static void setupKit(Kit *kit, Id pDeviceType, const ToolchainPair& toolchains,
                     const QVariant &debuggerId, const FilePath &sdkPath, QtVersion *qtVersion)
{
    RunDeviceTypeKitAspect::setDeviceTypeId(kit, pDeviceType);
    if (toolchains.first)
        ToolchainKitAspect::setToolchain(kit, toolchains.first);
    else
        ToolchainKitAspect::clearToolchain(kit, ProjectExplorer::Constants::C_LANGUAGE_ID);
    if (toolchains.second)
        ToolchainKitAspect::setToolchain(kit, toolchains.second);
    else
        ToolchainKitAspect::clearToolchain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);

    QtKitAspect::setQtVersion(kit, qtVersion);
    // only replace debugger with the default one if we find an unusable one here
    // (since the user could have changed it)
    if ((!DebuggerKitAspect::debugger(kit)
            || !DebuggerKitAspect::debugger(kit)->isValid()
            || DebuggerKitAspect::debugger(kit)->engineType() != LldbEngineType)
            && debuggerId.isValid())
        DebuggerKitAspect::setDebugger(kit, debuggerId);

    kit->setSticky(QtKitAspect::id(), true);
    kit->setSticky(ToolchainKitAspect::id(), true);
    kit->setSticky(RunDeviceTypeKitAspect::id(), true);
    kit->setSticky(SysRootKitAspect::id(), true);
    kit->setSticky(DebuggerKitAspect::id(), false);

    SysRootKitAspect::setSysRoot(kit, sdkPath);
}

static QVersionNumber findXcodeVersion(const FilePath &developerPath)
{
    const FilePath xcodeInfo = developerPath.parentDir().pathAppended("Info.plist");
    if (xcodeInfo.exists()) {
        QSettings settings(xcodeInfo.toUrlishString(), QSettings::NativeFormat);
        return QVersionNumber::fromString(settings.value("CFBundleShortVersionString").toString());
    } else {
        qCDebug(iosCommonLog) << "Error finding Xcode version." << xcodeInfo.toUserOutput() <<
                                 "does not exist.";
    }
    return QVersionNumber();
}

static QByteArray decodeProvisioningProfile(const QString &path)
{
    QTC_ASSERT(!path.isEmpty(), return QByteArray());

    Process p;
    // path is assumed to be valid file path to .mobileprovision
    p.setCommand({"openssl", {"smime", "-inform", "der", "-verify", "-in", path}});
    using namespace std::chrono_literals;
    p.runBlocking(3s);
    if (p.result() != ProcessResult::FinishedWithSuccess)
        qCDebug(iosCommonLog) << "Reading signed provisioning file failed" << path;
    return p.cleanedStdOut().toLatin1();
}

void IosConfigurations::updateAutomaticKitList()
{
    const QList<XcodePlatform> platforms = XcodeProbe::detectPlatforms().values();
    if (!platforms.isEmpty())
        setDeveloperPath(platforms.first().developerPath);
    qCDebug(kitSetupLog) << "Developer path:" << developerPath();

    // target -> tool chain
    const auto targetToolchainHash = findToolchains(platforms);

    const auto qtVersions = toSet(QtVersionManager::versions([](const QtVersion *v) {
        return v->isValid() && v->type() == Constants::IOSQT;
    }));

    const DebuggerItem *possibleDebugger = DebuggerItemManager::findByEngineType(LldbEngineType);
    const QVariant debuggerId = (possibleDebugger ? possibleDebugger->id() : QVariant());

    QSet<Kit *> existingKits = existingAutoDetectedIosKits();
    qCDebug(kitSetupLog) << "Existing auto-detected iOS kits:";
    printKits(existingKits);
    QSet<Kit *> resultingKits;
    for (const XcodePlatform &platform : platforms) {
        for (const auto &sdk : platform.sdks) {
            const auto targets = filtered(platform.targets,
                                          [&sdk](const XcodePlatform::ToolchainTarget &target) {
                 return sdk.architectures.first() == target.architecture;
            });
            if (targets.empty())
                continue;

            const auto target = targets.front();

            const ToolchainPair &platformToolchains = targetToolchainHash.value(target);
            if (!platformToolchains.first && !platformToolchains.second) {
                qCDebug(kitSetupLog) << "  - No tool chain found";
                continue;
            }
            Id pDeviceType = deviceId(sdk.directoryName);
            if (!pDeviceType.isValid()) {
                qCDebug(kitSetupLog) << "Unsupported/Invalid device type" << sdk.directoryName;
                continue;
            }

            for (QtVersion *qtVersion : qtVersions) {
                qCDebug(kitSetupLog) << "  - Qt version:" << qtVersion->displayName();
                Kit *kit = findOrDefault(existingKits, [&pDeviceType, &platformToolchains, &qtVersion](const Kit *kit) {
                    // we do not compare the sdk (thus automatically upgrading it in place if a
                    // new Xcode is used). Change?
                    return RunDeviceTypeKitAspect::deviceTypeId(kit) == pDeviceType
                            && ToolchainKitAspect::cxxToolchain(kit) == platformToolchains.second
                            && ToolchainKitAspect::cToolchain(kit) == platformToolchains.first
                            && QtKitAspect::qtVersion(kit) == qtVersion;
                });
                QTC_ASSERT(!resultingKits.contains(kit), continue);
                if (kit) {
                    qCDebug(kitSetupLog) << "    - Kit matches:" << kit->displayName();
                    kit->blockNotification();
                    setupKit(kit, pDeviceType, platformToolchains, debuggerId, sdk.path, qtVersion);
                    kit->unblockNotification();
                } else {
                    qCDebug(kitSetupLog) << "    - Setting up new kit";
                    const auto init = [&](Kit *k) {
                        k->setAutoDetected(true);
                        const QString baseDisplayName = isSimulatorDeviceId(pDeviceType)
                                ? Tr::tr("%1 Simulator").arg(qtVersion->unexpandedDisplayName())
                                : qtVersion->unexpandedDisplayName();
                        k->setUnexpandedDisplayName(baseDisplayName);
                        setupKit(k, pDeviceType, platformToolchains, debuggerId, sdk.path, qtVersion);
                    };
                    kit = KitManager::registerKit(init);
                }
                resultingKits.insert(kit);
            }
        }
    }
    // remove unused kits
    existingKits.subtract(resultingKits);
    qCDebug(kitSetupLog) << "Removing unused kits:";
    printKits(existingKits);
    KitManager::deregisterKits(toList(existingKits));
}

static IosConfigurations *m_instance = nullptr;

IosConfigurations *IosConfigurations::instance()
{
    return m_instance;
}

void IosConfigurations::initialize()
{
    QTC_CHECK(m_instance == nullptr);
    m_instance = new IosConfigurations(nullptr);
}

void IosConfigurations::kitsRestored()
{
    disconnect(KitManager::instance(), &KitManager::kitsLoaded,
               this, &IosConfigurations::kitsRestored);
    IosConfigurations::updateAutomaticKitList();
    connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsChanged,
            IosConfigurations::instance(), &IosConfigurations::updateAutomaticKitList);
}

bool IosConfigurations::ignoreAllDevices()
{
    return m_instance->m_ignoreAllDevices;
}

void IosConfigurations::setIgnoreAllDevices(bool ignoreDevices)
{
    if (ignoreDevices != m_instance->m_ignoreAllDevices) {
        m_instance->m_ignoreAllDevices = ignoreDevices;
        m_instance->save();
    }
}

FilePath IosConfigurations::developerPath()
{
    return m_instance->m_developerPath;
}

QVersionNumber IosConfigurations::xcodeVersion()
{
    return m_instance->m_xcodeVersion;
}

void IosConfigurations::save()
{
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    settings->setValueWithDefault(ignoreAllDevicesKey, m_ignoreAllDevices, IgnoreAllDevicesDefault);
    settings->endGroup();
}

IosConfigurations::IosConfigurations(QObject *parent)
    : QObject(parent)
{
    load();
    connect(KitManager::instance(), &KitManager::kitsLoaded,
            this, &IosConfigurations::kitsRestored);
}

void IosConfigurations::load()
{
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    m_ignoreAllDevices = settings->value(ignoreAllDevicesKey, IgnoreAllDevicesDefault).toBool();
    settings->endGroup();
}

void IosConfigurations::updateSimulators()
{
    // currently we have just one simulator
    DeviceManager *devManager = DeviceManager::instance();
    Id devId = Constants::IOS_SIMULATOR_DEVICE_ID;
    IDevice::Ptr dev = devManager->find(devId);
    if (!dev) {
        dev = IDevice::Ptr(new IosSimulator(devId));
        devManager->addDevice(dev);
    }
    Utils::futureSynchronizer()->addFuture(SimulatorControl::updateAvailableSimulators(this));
}

void IosConfigurations::setDeveloperPath(const FilePath &devPath)
{
    static bool hasDevPath = false;
    if (devPath != m_instance->m_developerPath) {
        m_instance->m_developerPath = devPath;
        m_instance->save();
        if (!hasDevPath && !devPath.isEmpty()) {
            hasDevPath = true;
            QTimer::singleShot(1000, IosDeviceManager::instance(),
                               &IosDeviceManager::monitorAvailableDevices);
            m_instance->updateSimulators();

            // Find xcode version.
            m_instance->m_xcodeVersion = findXcodeVersion(m_instance->m_developerPath);
        }
    }
}

void IosConfigurations::initializeProvisioningData()
{
    // Initialize provisioning data only on demand. i.e. when first call to a provisioing data API
    // is made.
    if (m_provisioningDataWatcher)
        return;

    // Instantiate m_provisioningDataWatcher to mark initialized before calling loadProvisioningData.
    m_provisioningDataWatcher = new QFileSystemWatcher(this);

    m_instance->loadProvisioningData(false);

    // Watch the provisioing profiles folder and xcode plist for changes and
    // update the content accordingly.
    m_provisioningDataWatcher->addPath(xcodePlistPath);
    m_provisioningDataWatcher->addPath(provisioningProfileDirPath);
    connect(m_provisioningDataWatcher, &QFileSystemWatcher::directoryChanged,
            std::bind(&IosConfigurations::loadProvisioningData, this, true));
    connect(m_provisioningDataWatcher, &QFileSystemWatcher::fileChanged,
            std::bind(&IosConfigurations::loadProvisioningData, this, true));
}

static QVariantMap getTeamMap(const QSettings &xcodeSettings)
{
    // Check the version for Xcode 16.2 and later
    const QVariantMap teamMap = xcodeSettings.value("IDEProvisioningTeamByIdentifier").toMap();
    if (!teamMap.isEmpty())
        return teamMap;
    // Fall back to setting from Xcode < 16.2
    return xcodeSettings.value("IDEProvisioningTeams").toMap();
}

static QHash<QString, QString> getIdentifierToEmail(const QSettings &xcodeSettings)
{
    // Available for Xcode 16.2 and later, where the keys for the IDEProvisioningTeamByIdentifier
    // (see getTeamMap) are identifiers, and the "email" is in yet another "map":
    // "DVTDeveloperAccountManagerAppleIDLists" => {
    //   "IDE.Identifiers.Prod" => [
    //     0 => {
    //       "identifier" => "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
    //     }
    //   ]
    //   "IDE.Prod" => [
    //     0 => {
    //       "username" => "xxxx"
    //     }
    //   ]
    // }
    const QVariantMap accountMap
        = xcodeSettings.value("DVTDeveloperAccountManagerAppleIDLists").toMap();
    const QVariantList idList = accountMap.value("IDE.Identifiers.Prod").toList();
    const QVariantList emailList = accountMap.value("IDE.Prod").toList();
    QHash<QString, QString> result;
    const int size = std::min(idList.size(), emailList.size());
    for (int i = 0; i < size; ++i) {
        result.insert(
            idList.at(i).toMap().value("identifier").toString(),
            emailList.at(i).toMap().value("username").toString());
    }
    return result;
}

void IosConfigurations::loadProvisioningData(bool notify)
{
    m_developerTeams.clear();
    m_provisioningProfiles.clear();

    // Populate Team id's
    const QSettings xcodeSettings(xcodePlistPath, QSettings::NativeFormat);
    const QVariantMap teamMap = getTeamMap(xcodeSettings);
    const QHash<QString, QString> identifierToName = getIdentifierToEmail(xcodeSettings);
    QList<QVariantMap> teams;
    for (auto accountiterator = teamMap.cbegin(), end = teamMap.cend();
            accountiterator != end; ++accountiterator) {
        // difference between Qt 5 (map) and Qt 6 (list of maps)
        const bool isList = accountiterator->userType() == QMetaType::QVariantList;
        const QVariantList teamsList = isList ? accountiterator.value().toList()
                                              : QVariantList({accountiterator.value()});
        for (const QVariant &teamInfoIt : teamsList) {
            QVariantMap teamInfo = teamInfoIt.toMap();
            int provisioningTeamIsFree = teamInfo.value(freeTeamTag).toBool() ? 1 : 0;
            teamInfo[freeTeamTag] = provisioningTeamIsFree;
            teamInfo[emailTag]
                = identifierToName.value(accountiterator.key(), /*default=*/accountiterator.key());
            teams.append(teamInfo);
        }
    }

    // Sort team id's to move the free provisioning teams at last of the list.
    sort(teams, [](const QVariantMap &teamInfo1, const QVariantMap &teamInfo2) {
        return teamInfo1.value(freeTeamTag).toInt() < teamInfo2.value(freeTeamTag).toInt();
    });

    for (auto teamInfo : std::as_const(teams)) {
        auto team = std::make_shared<DevelopmentTeam>();
        team->m_name = teamInfo.value(teamNameTag).toString();
        team->m_email = teamInfo.value(emailTag).toString();
        team->m_identifier = teamInfo.value(teamIdTag).toString();
        team->m_freeTeam = teamInfo.value(freeTeamTag).toInt() == 1;
        m_developerTeams.append(team);
    }

    const QDir provisioningProflesDir(provisioningProfileDirPath);
    const QStringList filters = {"*.mobileprovision"};
    const QList<QFileInfo> fileInfos = provisioningProflesDir.entryInfoList(filters,
                                                                            QDir::NoDotAndDotDot
                                                                                | QDir::Files);
    for (const QFileInfo &fileInfo : fileInfos) {
        QDomDocument provisioningDoc;
        auto profile = std::make_shared<ProvisioningProfile>();
        QString teamID;
        if (provisioningDoc.setContent(decodeProvisioningProfile(fileInfo.absoluteFilePath()))) {
            QDomNodeList nodes =  provisioningDoc.elementsByTagName("key");
            for (int i = 0;i<nodes.count(); ++i) {
                QDomElement e = nodes.at(i).toElement();

                if (e.text().compare(udidTag) == 0)
                    profile->m_identifier = e.nextSiblingElement().text();

                if (e.text().compare(profileNameTag) == 0)
                    profile->m_name = e.nextSiblingElement().text();

                if (e.text().compare(appIdTag) == 0)
                    profile->m_appID = e.nextSiblingElement().text();

                if (e.text().compare(expirationDateTag) == 0)
                    profile->m_expirationDate = QDateTime::fromString(e.nextSiblingElement().text(),
                                                                      Qt::ISODate).toUTC();

                if (e.text().compare(profileTeamIdTag) == 0) {
                    teamID = e.nextSibling().firstChildElement().text();
                    auto team =  developmentTeam(teamID);
                    if (team) {
                        profile->m_team = team;
                        team->m_profiles.append(profile);
                    }
                }
            }
        } else {
            qCDebug(iosCommonLog) << "Error reading provisoing profile" << fileInfo.absoluteFilePath();
        }

        if (profile->m_team)
            m_provisioningProfiles.append(profile);
        else
            qCDebug(iosCommonLog) << "Skipping profile. No corresponding team found" << profile;
    }

    if (notify)
        emit provisioningDataChanged();
}

const DevelopmentTeams &IosConfigurations::developmentTeams()
{
    QTC_CHECK(m_instance);
    m_instance->initializeProvisioningData();
    return m_instance->m_developerTeams;
}

DevelopmentTeamPtr IosConfigurations::developmentTeam(const QString &teamID)
{
    QTC_CHECK(m_instance);
    m_instance->initializeProvisioningData();
    return findOrDefault(m_instance->m_developerTeams,
                         equal(&DevelopmentTeam::identifier, teamID));
}

const ProvisioningProfiles &IosConfigurations::provisioningProfiles()
{
    QTC_CHECK(m_instance);
    m_instance->initializeProvisioningData();
    return m_instance->m_provisioningProfiles;
}

ProvisioningProfilePtr IosConfigurations::provisioningProfile(const QString &profileID)
{
    QTC_CHECK(m_instance);
    m_instance->initializeProvisioningData();
    return findOrDefault(m_instance->m_provisioningProfiles,
                                equal(&ProvisioningProfile::identifier, profileID));
}

class IosToolchainFactory final : public ToolchainFactory
{
public:
    IosToolchainFactory()
    {
        setSupportedLanguages({ProjectExplorer::Constants::C_LANGUAGE_ID,
                               ProjectExplorer::Constants::CXX_LANGUAGE_ID});
    }

    Toolchains autoDetect(const ToolchainDetector &detector) const final;

    std::unique_ptr<ToolchainConfigWidget> createConfigurationWidget(
        const ToolchainBundle &bundle) const override
    {
        return GccToolchain::createConfigurationWidget(bundle);
    }
};

Toolchains IosToolchainFactory::autoDetect(const ToolchainDetector &detector) const
{
    if (detector.device->type() != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
        return {};

    QList<GccToolchain *> existingClangToolchains = clangToolchains(detector.alreadyKnown);
    const QList<XcodePlatform> platforms = XcodeProbe::detectPlatforms().values();
    Toolchains toolchains;
    toolchains.reserve(platforms.size());
    for (const XcodePlatform &platform : platforms) {
        for (const XcodePlatform::ToolchainTarget &target : platform.targets) {
            ToolchainPair platformToolchains = findToolchainForPlatform(platform, target,
                                                                        existingClangToolchains);
            auto createOrAdd = [&](GccToolchain *toolChain, Id l) {
                if (!toolChain) {
                    toolChain = new GccToolchain(ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID,
                                                 GccToolchain::Clang);
                    toolChain->setPriority(Toolchain::PriorityHigh);
                    toolChain->setDetection(Toolchain::AutoDetection);
                    toolChain->setLanguage(l);
                    toolChain->setDisplayName(target.name);
                    toolChain->setPlatformCodeGenFlags(target.backendFlags);
                    toolChain->setPlatformLinkerFlags(target.backendFlags);
                    toolChain->resetToolchain(l == ProjectExplorer::Constants::CXX_LANGUAGE_ID ?
                                                  platform.cxxCompilerPath : platform.cCompilerPath);
                    existingClangToolchains.append(toolChain);
                }
                toolchains.append(toolChain);
            };

            createOrAdd(platformToolchains.first, ProjectExplorer::Constants::C_LANGUAGE_ID);
            createOrAdd(platformToolchains.second, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
        }
    }
    return toolchains;
}

void setupIosToolchain()
{
    static IosToolchainFactory theIosToolchainFactory;
}

QString DevelopmentTeam::identifier() const
{
    return m_identifier;
}

QString DevelopmentTeam::displayName() const
{
    return QString("%1 - %2").arg(m_email).arg(m_name);
}

QString DevelopmentTeam::details() const
{
    return Tr::tr("%1 - Free Provisioning Team : %2")
            .arg(m_identifier).arg(m_freeTeam ? Tr::tr("Yes") : Tr::tr("No"));
}

QDebug &operator<<(QDebug &stream, DevelopmentTeamPtr team)
{
    QTC_ASSERT(team, return stream);
    stream << team->displayName() << team->identifier() << team->isFreeProfile();
    for (const auto &profile : std::as_const(team->m_profiles))
        stream << "Profile:" << profile;
    return stream;
}

QString ProvisioningProfile::identifier() const
{
    return m_identifier;
}

QString ProvisioningProfile::displayName() const
{
    return m_name;
}

QString ProvisioningProfile::details() const
{
    return Tr::tr("Team: %1\nApp ID: %2\nExpiration date: %3").arg(m_team->identifier()).arg(m_appID)
            .arg(QLocale::system().toString(m_expirationDate.toLocalTime(), QLocale::ShortFormat));
}

QDebug &operator<<(QDebug &stream, std::shared_ptr<ProvisioningProfile> profile)
{
    QTC_ASSERT(profile, return stream);
    return stream << profile->displayName() << profile->identifier() << profile->details();
}

} // namespace Internal
} // namespace Ios
