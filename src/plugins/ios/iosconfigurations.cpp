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

#include "iosconfigurations.h"
#include "iosconstants.h"
#include "iosdevice.h"
#include "iossimulator.h"
#include "simulatorcontrol.h"
#include "iosprobe.h"

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/synchronousprocess.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggerkitinformation.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtversionfactory.h>

#include <QDir>
#include <QDomDocument>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QHash>
#include <QList>
#include <QLoggingCategory>
#include <QProcess>
#include <QSettings>
#include <QStringList>
#include <QStandardPaths>
#include <QTimer>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;
using namespace Debugger;

namespace {
Q_LOGGING_CATEGORY(kitSetupLog, "qtc.ios.kitSetup")
Q_LOGGING_CATEGORY(iosCommonLog, "qtc.ios.common")
}

using ToolChainPair = std::pair<ClangToolChain *, ClangToolChain *>;
namespace Ios {
namespace Internal {

const char SettingsGroup[] = "IosConfigurations";
const char ignoreAllDevicesKey[] = "IgnoreAllDevices";
const char screenshotDirPathKey[] = "ScreeshotDirPath";

const char provisioningTeamsTag[] = "IDEProvisioningTeams";
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

static Core::Id deviceId(const QString &sdkName)
{
    if (sdkName.startsWith("iphoneos", Qt::CaseInsensitive))
        return Constants::IOS_DEVICE_TYPE;
    else if (sdkName.startsWith("iphonesimulator", Qt::CaseInsensitive))
        return Constants::IOS_SIMULATOR_TYPE;
    return Core::Id();
}

static bool isSimulatorDeviceId(const Core::Id &id)
{
    return id == Constants::IOS_SIMULATOR_TYPE;
}

static QList<ClangToolChain *> clangToolChains(const QList<ToolChain *> &toolChains)
{
    QList<ClangToolChain *> clangToolChains;
    foreach (ToolChain *toolChain, toolChains)
        if (toolChain->typeId() == ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID)
            clangToolChains.append(static_cast<ClangToolChain *>(toolChain));
    return clangToolChains;
}

static QList<ClangToolChain *> autoDetectedIosToolChains()
{
    const QList<ClangToolChain *> toolChains = clangToolChains(ToolChainManager::toolChains());
    return Utils::filtered(toolChains, [](ClangToolChain *toolChain) {
        return toolChain->isAutoDetected()
               && (toolChain->displayName().startsWith("iphone")
                   || toolChain->displayName().startsWith("Apple Clang")); // TODO tool chains should be marked directly
    });
}

static ToolChainPair findToolChainForPlatform(const XcodePlatform &platform,
                                              const XcodePlatform::ToolchainTarget &target,
                                              const QList<ClangToolChain *> &toolChains)
{
    ToolChainPair platformToolChains;
    auto toolchainMatch = [](ClangToolChain *toolChain, const Utils::FileName &compilerPath, const QStringList &flags) {
        return compilerPath == toolChain->compilerCommand()
                && flags == toolChain->platformCodeGenFlags()
                && flags == toolChain->platformLinkerFlags();
    };
    platformToolChains.first = Utils::findOrDefault(toolChains, std::bind(toolchainMatch, std::placeholders::_1,
                                                                  platform.cCompilerPath,
                                                                  target.backendFlags));
    platformToolChains.second = Utils::findOrDefault(toolChains, std::bind(toolchainMatch, std::placeholders::_1,
                                                                   platform.cxxCompilerPath,
                                                                   target.backendFlags));
    return platformToolChains;
}

static QHash<XcodePlatform::ToolchainTarget, ToolChainPair> findToolChains(const QList<XcodePlatform> &platforms)
{
    QHash<XcodePlatform::ToolchainTarget, ToolChainPair> platformToolChainHash;
    const QList<ClangToolChain *> toolChains = autoDetectedIosToolChains();
    for (const XcodePlatform &platform : platforms) {
        for (const XcodePlatform::ToolchainTarget &target : platform.targets) {
            ToolChainPair platformToolchains = findToolChainForPlatform(platform, target,
                                                                        toolChains);
            if (platformToolchains.first || platformToolchains.second)
                platformToolChainHash.insert(target, platformToolchains);
        }
    }
    return platformToolChainHash;
}

static QSet<Kit *> existingAutoDetectedIosKits()
{
    return Utils::filtered(KitManager::kits(), [](Kit *kit) -> bool {
        Core::Id deviceKind = DeviceTypeKitInformation::deviceTypeId(kit);
        return kit->isAutoDetected() && (deviceKind == Constants::IOS_DEVICE_TYPE
                                         || deviceKind == Constants::IOS_SIMULATOR_TYPE);
    }).toSet();
}

static void printKits(const QSet<Kit *> &kits)
{
    foreach (const Kit *kit, kits)
        qCDebug(kitSetupLog) << "  -" << kit->displayName();
}

static void setupKit(Kit *kit, Core::Id pDeviceType, const ToolChainPair& toolChains,
                     const QVariant &debuggerId, const Utils::FileName &sdkPath, BaseQtVersion *qtVersion)
{
    DeviceTypeKitInformation::setDeviceTypeId(kit, pDeviceType);
    if (toolChains.first)
        ToolChainKitInformation::setToolChain(kit, toolChains.first);
    else
        ToolChainKitInformation::clearToolChain(kit, ProjectExplorer::Constants::C_LANGUAGE_ID);
    if (toolChains.second)
        ToolChainKitInformation::setToolChain(kit, toolChains.second);
    else
        ToolChainKitInformation::clearToolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);

    QtKitInformation::setQtVersion(kit, qtVersion);
    // only replace debugger with the default one if we find an unusable one here
    // (since the user could have changed it)
    if ((!DebuggerKitInformation::debugger(kit)
            || !DebuggerKitInformation::debugger(kit)->isValid()
            || DebuggerKitInformation::debugger(kit)->engineType() != LldbEngineType)
            && debuggerId.isValid())
        DebuggerKitInformation::setDebugger(kit, debuggerId);

    kit->setMutable(DeviceKitInformation::id(), true);
    kit->setSticky(QtKitInformation::id(), true);
    kit->setSticky(ToolChainKitInformation::id(), true);
    kit->setSticky(DeviceTypeKitInformation::id(), true);
    kit->setSticky(SysRootKitInformation::id(), true);
    kit->setSticky(DebuggerKitInformation::id(), false);

    SysRootKitInformation::setSysRoot(kit, sdkPath);
}

static QVersionNumber findXcodeVersion(const Utils::FileName &developerPath)
{
    FileName xcodeInfo = developerPath.parentDir().appendPath("Info.plist");
    if (xcodeInfo.exists()) {
        QSettings settings(xcodeInfo.toString(), QSettings::NativeFormat);
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

    Utils::SynchronousProcess p;
    p.setTimeoutS(3);
    // path is assumed to be valid file path to .mobileprovision
    const QStringList args = {"smime", "-inform", "der", "-verify", "-in", path};
    Utils::SynchronousProcessResponse res = p.runBlocking("openssl", args);
    if (res.result != Utils::SynchronousProcessResponse::Finished)
        qCDebug(iosCommonLog) << "Reading signed provisioning file failed" << path;
    return res.stdOut().toLatin1();
}

void IosConfigurations::updateAutomaticKitList()
{
    const QList<XcodePlatform> platforms = XcodeProbe::detectPlatforms().values();
    if (!platforms.isEmpty())
        setDeveloperPath(platforms.first().developerPath);
    qCDebug(kitSetupLog) << "Developer path:" << developerPath();

    // target -> tool chain
    const auto targetToolChainHash = findToolChains(platforms);

    const auto qtVersions = QtVersionManager::versions([](const BaseQtVersion *v) {
        return v->isValid() && v->type() == Constants::IOSQT;
    }).toSet();

    const DebuggerItem *possibleDebugger = DebuggerItemManager::findByEngineType(LldbEngineType);
    const QVariant debuggerId = (possibleDebugger ? possibleDebugger->id() : QVariant());

    QSet<Kit *> existingKits = existingAutoDetectedIosKits();
    qCDebug(kitSetupLog) << "Existing auto-detected iOS kits:";
    printKits(existingKits);
    QSet<Kit *> resultingKits;
    for (const XcodePlatform &platform : platforms) {
        for (const auto &sdk : platform.sdks) {
            const auto targets = Utils::filtered(platform.targets,
                                           [&sdk](const XcodePlatform::ToolchainTarget &target) {
                 return sdk.architectures.first() == target.architecture;
            });
            if (targets.empty())
                continue;

            const auto target = targets.front();

            const ToolChainPair &platformToolchains = targetToolChainHash.value(target);
            if (!platformToolchains.first && !platformToolchains.second) {
                qCDebug(kitSetupLog) << "  - No tool chain found";
                continue;
            }
            Core::Id pDeviceType = deviceId(sdk.directoryName);
            if (!pDeviceType.isValid()) {
                qCDebug(kitSetupLog) << "Unsupported/Invalid device type" << sdk.directoryName;
                continue;
            }

            for (BaseQtVersion *qtVersion : qtVersions) {
                qCDebug(kitSetupLog) << "  - Qt version:" << qtVersion->displayName();
                Kit *kit = Utils::findOrDefault(existingKits, [&pDeviceType, &platformToolchains, &qtVersion](const Kit *kit) {
                    // we do not compare the sdk (thus automatically upgrading it in place if a
                    // new Xcode is used). Change?
                    return DeviceTypeKitInformation::deviceTypeId(kit) == pDeviceType
                            && ToolChainKitInformation::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID) == platformToolchains.second
                            && ToolChainKitInformation::toolChain(kit, ProjectExplorer::Constants::C_LANGUAGE_ID) == platformToolchains.first
                            && QtKitInformation::qtVersion(kit) == qtVersion;
                });
                QTC_ASSERT(!resultingKits.contains(kit), continue);
                if (kit) {
                    qCDebug(kitSetupLog) << "    - Kit matches:" << kit->displayName();
                    kit->blockNotification();
                    setupKit(kit, pDeviceType, platformToolchains, debuggerId, sdk.path, qtVersion);
                    kit->unblockNotification();
                } else {
                    qCDebug(kitSetupLog) << "    - Setting up new kit";
                    kit = new Kit;
                    kit->blockNotification();
                    kit->setAutoDetected(true);
                    const QString baseDisplayName = isSimulatorDeviceId(pDeviceType)
                            ? tr("%1 Simulator").arg(qtVersion->unexpandedDisplayName())
                            : qtVersion->unexpandedDisplayName();
                    kit->setUnexpandedDisplayName(baseDisplayName);
                    setupKit(kit, pDeviceType, platformToolchains, debuggerId, sdk.path, qtVersion);
                    kit->unblockNotification();
                    KitManager::registerKit(kit);
                }
                resultingKits.insert(kit);
            }
        }
    }
    // remove unused kits
    existingKits.subtract(resultingKits);
    qCDebug(kitSetupLog) << "Removing unused kits:";
    printKits(existingKits);
    foreach (Kit *kit, existingKits)
        KitManager::deregisterKit(kit);
}

static IosConfigurations *m_instance = nullptr;

IosConfigurations *IosConfigurations::instance()
{
    return m_instance;
}

void IosConfigurations::initialize()
{
    QTC_CHECK(m_instance == 0);
    m_instance = new IosConfigurations(0);
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

void IosConfigurations::setScreenshotDir(const FileName &path)
{
    if (m_instance->m_screenshotDir != path) {
        m_instance->m_screenshotDir = path;
        m_instance->save();
    }
}

FileName IosConfigurations::screenshotDir()
{
    return m_instance->m_screenshotDir;
}

FileName IosConfigurations::developerPath()
{
    return m_instance->m_developerPath;
}

QVersionNumber IosConfigurations::xcodeVersion()
{
    return m_instance->m_xcodeVersion;
}

void IosConfigurations::save()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    settings->setValue(ignoreAllDevicesKey, m_ignoreAllDevices);
    settings->setValue(screenshotDirPathKey, m_screenshotDir.toString());
    settings->endGroup();
}

IosConfigurations::IosConfigurations(QObject *parent)
    : QObject(parent)
{
    load();
}

void IosConfigurations::load()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    m_ignoreAllDevices = settings->value(ignoreAllDevicesKey, false).toBool();
    m_screenshotDir = FileName::fromString(settings->value(screenshotDirPathKey).toString());
    if (!m_screenshotDir.exists()) {
        QString defaultDir = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first();
        m_screenshotDir = FileName::fromString(defaultDir);
    }

    settings->endGroup();
}

void IosConfigurations::updateSimulators()
{
    // currently we have just one simulator
    DeviceManager *devManager = DeviceManager::instance();
    Core::Id devId = Constants::IOS_SIMULATOR_DEVICE_ID;
    IDevice::ConstPtr dev = devManager->find(devId);
    if (dev.isNull()) {
        dev = IDevice::ConstPtr(new IosSimulator(devId));
        devManager->addDevice(dev);
    }
    SimulatorControl::updateAvailableSimulators();
}

void IosConfigurations::setDeveloperPath(const FileName &devPath)
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

void IosConfigurations::loadProvisioningData(bool notify)
{
    m_developerTeams.clear();
    m_provisioningProfiles.clear();

    // Populate Team id's
    const QSettings xcodeSettings(xcodePlistPath, QSettings::NativeFormat);
    const QVariantMap teamMap = xcodeSettings.value(provisioningTeamsTag).toMap();
    QList<QVariantMap> teams;
    QMapIterator<QString, QVariant> accountiterator(teamMap);
    while (accountiterator.hasNext()) {
        accountiterator.next();
        QVariantMap teamInfo = accountiterator.value().toMap();
        int provisioningTeamIsFree = teamInfo.value(freeTeamTag).toBool() ? 1 : 0;
        teamInfo[freeTeamTag] = provisioningTeamIsFree;
        teamInfo[emailTag] = accountiterator.key();
        teams.append(teamInfo);
    }

    // Sort team id's to move the free provisioning teams at last of the list.
    Utils::sort(teams, [](const QVariantMap &teamInfo1, const QVariantMap &teamInfo2) {
        return teamInfo1.value(freeTeamTag).toInt() < teamInfo2.value(freeTeamTag).toInt();
    });

    foreach (auto teamInfo, teams) {
        auto team = std::make_shared<DevelopmentTeam>();
        team->m_name = teamInfo.value(teamNameTag).toString();
        team->m_email = teamInfo.value(emailTag).toString();
        team->m_identifier = teamInfo.value(teamIdTag).toString();
        team->m_freeTeam = teamInfo.value(freeTeamTag).toInt() == 1;
        m_developerTeams.append(team);
    }

    const QDir provisioningProflesDir(provisioningProfileDirPath);
    const QStringList filters = {"*.mobileprovision"};
    foreach (QFileInfo fileInfo, provisioningProflesDir.entryInfoList(filters, QDir::NoDotAndDotDot | QDir::Files)) {
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
                         Utils::equal(&DevelopmentTeam::identifier, teamID));
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
    return Utils::findOrDefault(m_instance->m_provisioningProfiles,
                                Utils::equal(&ProvisioningProfile::identifier, profileID));
}

static ClangToolChain *createToolChain(const XcodePlatform &platform,
                                       const XcodePlatform::ToolchainTarget &target,
                                       Core::Id l)
{
    if (!l.isValid())
        return nullptr;

    if (l != Core::Id(ProjectExplorer::Constants::C_LANGUAGE_ID)
            && l != Core::Id(ProjectExplorer::Constants::CXX_LANGUAGE_ID))
        return nullptr;

    ClangToolChain *toolChain = new ClangToolChain(ToolChain::AutoDetection);
    toolChain->setLanguage(l);
    toolChain->setDisplayName(target.name);
    toolChain->setPlatformCodeGenFlags(target.backendFlags);
    toolChain->setPlatformLinkerFlags(target.backendFlags);
    toolChain->resetToolChain(l == Core::Id(ProjectExplorer::Constants::CXX_LANGUAGE_ID) ?
                                  platform.cxxCompilerPath : platform.cCompilerPath);

    return toolChain;
}

QSet<Core::Id> IosToolChainFactory::supportedLanguages() const
{
    return {
        ProjectExplorer::Constants::C_LANGUAGE_ID,
        ProjectExplorer::Constants::CXX_LANGUAGE_ID
    };
}

QList<ToolChain *> IosToolChainFactory::autoDetect(const QList<ToolChain *> &existingToolChains)
{
    QList<ClangToolChain *> existingClangToolChains = clangToolChains(existingToolChains);
    const QList<XcodePlatform> platforms = XcodeProbe::detectPlatforms().values();
    QList<ClangToolChain *> toolChains;
    toolChains.reserve(platforms.size());
    foreach (const XcodePlatform &platform, platforms) {
        for (const XcodePlatform::ToolchainTarget &target : platform.targets) {
            ToolChainPair platformToolchains = findToolChainForPlatform(platform, target,
                                                                        existingClangToolChains);
            auto createOrAdd = [&](ClangToolChain* toolChain, Core::Id l) {
                if (!toolChain) {
                    toolChain = createToolChain(platform, target, l);
                    existingClangToolChains.append(toolChain);
                }
                toolChains.append(toolChain);
            };

            createOrAdd(platformToolchains.first, ProjectExplorer::Constants::C_LANGUAGE_ID);
            createOrAdd(platformToolchains.second, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
        }
    }
    return Utils::transform(toolChains, [](ClangToolChain *tc) -> ToolChain * { return tc; });
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
    return tr("%1 - Free Provisioning Team : %2")
            .arg(m_identifier).arg(m_freeTeam ? tr("Yes") : tr("No"));
}

QDebug &operator<<(QDebug &stream, DevelopmentTeamPtr team)
{
    QTC_ASSERT(team, return stream);
    stream << team->displayName() << team->identifier() << team->isFreeProfile();
    foreach (auto profile, team->m_profiles)
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
    return tr("Team: %1\nApp ID: %2\nExpiration date: %3").arg(m_team->identifier()).arg(m_appID)
            .arg(m_expirationDate.toLocalTime().toString(Qt::SystemLocaleShortDate));
}

QDebug &operator<<(QDebug &stream, std::shared_ptr<ProvisioningProfile> profile)
{
    QTC_ASSERT(profile, return stream);
    return stream << profile->displayName() << profile->identifier() << profile->details();
}

} // namespace Internal
} // namespace Ios
