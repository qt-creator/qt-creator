// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcukitmanager.h"

#include "mcukitaspect.h"
#include "mculegacyconstants.h"
#include "mcupackage.h"
#include "mcusupport_global.h"
#include "mcusupportconstants.h"
#include "mcusupportoptions.h"
#include "mcusupportplugin.h"
#include "mcusupportsdk.h"
#include "mcusupporttr.h"
#include "mcutarget.h"

#include <cmakeprojectmanager/cmakekitaspect.h>
#include <cmakeprojectmanager/cmaketoolmanager.h>

#include <debugger/debuggeritem.h>
#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggerkitaspect.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionmanager.h>

#include <utils/algorithm.h>

#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>

using CMakeProjectManager::CMakeConfig;
using CMakeProjectManager::CMakeConfigItem;
using CMakeProjectManager::CMakeConfigurationKitAspect;
using namespace ProjectExplorer;
using namespace Utils;

namespace McuSupport::Internal {

// Utils for managing CMake Configurations
static QMap<QByteArray, QByteArray> cMakeConfigToMap(const CMakeConfig &config)
{
    QMap<QByteArray, QByteArray> map;
    for (const auto &configItem : std::as_const(config.toList())) {
        map.insert(configItem.key, configItem.value);
    }
    return map;
}

static CMakeConfig mapToCMakeConfig(const QMap<QByteArray, QByteArray> &map)
{
    QList<CMakeConfigItem> asList;
    for (auto it = map.constKeyValueBegin(); it != map.constKeyValueEnd(); ++it) {
        asList.append(CMakeConfigItem(it->first, it->second));
    }

    return CMakeConfig(asList);
}

namespace McuKitManager {

static const int KIT_VERSION = 9; // Bumps up whenever details in Kit creation change

class McuKitFactory
{
public:
    static void setKitToolchains(Kit *k, const McuToolChainPackagePtr &tcPackage)
    {
        switch (tcPackage->toolchainType()) {
        case McuToolChainPackage::ToolChainType::Unsupported:
            return;

        case McuToolChainPackage::ToolChainType::GHS:
        case McuToolChainPackage::ToolChainType::GHSArm:
            return; // No Green Hills toolchain, because support for it is missing.

        case McuToolChainPackage::ToolChainType::IAR:
        case McuToolChainPackage::ToolChainType::KEIL:
        case McuToolChainPackage::ToolChainType::MSVC:
        case McuToolChainPackage::ToolChainType::GCC:
        case McuToolChainPackage::ToolChainType::MinGW:
        case McuToolChainPackage::ToolChainType::ArmGcc:
            ToolChainKitAspect::setToolChain(k,
                                             tcPackage->toolChain(
                                                 ProjectExplorer::Constants::C_LANGUAGE_ID));
            ToolChainKitAspect::setToolChain(k,
                                             tcPackage->toolChain(
                                                 ProjectExplorer::Constants::CXX_LANGUAGE_ID));
            return;

        default:
            Q_UNREACHABLE();
        }
    }

    static void setKitProperties(Kit *k, const McuTarget *mcuTarget, const FilePath &sdkPath)
    {
        using namespace Constants;
        const QString kitName{generateKitNameFromTarget(mcuTarget)};

        k->setUnexpandedDisplayName(kitName);
        k->setValue(KIT_MCUTARGET_VENDOR_KEY, mcuTarget->platform().vendor);
        k->setValue(KIT_MCUTARGET_MODEL_KEY, mcuTarget->platform().name);
        k->setValue(KIT_MCUTARGET_COLORDEPTH_KEY, mcuTarget->colorDepth());
        k->setValue(KIT_MCUTARGET_SDKVERSION_KEY, mcuTarget->qulVersion().toString());
        k->setValue(KIT_MCUTARGET_KITVERSION_KEY, KIT_VERSION);
        k->setValue(KIT_MCUTARGET_OS_KEY, static_cast<int>(mcuTarget->os()));
        k->setValue(KIT_MCUTARGET_TOOLCHAIN_KEY, mcuTarget->toolChainPackage()->toolChainName());
        k->setAutoDetected(false);
        k->makeSticky();
        if (mcuTarget->toolChainPackage()->isDesktopToolchain())
            k->setDeviceTypeForIcon(DEVICE_TYPE);
        k->setValue(QtSupport::Constants::FLAGS_SUPPLIES_QTQUICK_IMPORT_PATH, true);
        // FIXME: This is treated as a pathlist in CMakeBuildSystem::updateQmlJSCodeModel
        k->setValue(QtSupport::Constants::KIT_QML_IMPORT_PATH, (sdkPath / "include/qul").toString());
        k->setValue(QtSupport::Constants::KIT_HAS_MERGED_HEADER_PATHS_WITH_QML_IMPORT_PATHS, true);
        QSet<Id> irrelevant = {
            SysRootKitAspect::id(),
            QtSupport::Constants::FLAGS_SUPPLIES_QTQUICK_IMPORT_PATH,
            QtSupport::Constants::KIT_QML_IMPORT_PATH,
            QtSupport::Constants::KIT_HAS_MERGED_HEADER_PATHS_WITH_QML_IMPORT_PATHS,
        };
        if (!McuSupportOptions::kitsNeedQtVersion())
            irrelevant.insert(QtSupport::QtKitAspect::id());
        k->setIrrelevantAspects(irrelevant);
    }

    static void setKitDebugger(Kit *k, const McuToolChainPackagePtr &tcPackage)
    {
        if (tcPackage->isDesktopToolchain()) {
            // Qt Creator seems to be smart enough to deduce the right Kit debugger from the ToolChain
            return;
        }

        switch (tcPackage->toolchainType()) {
        case McuToolChainPackage::ToolChainType::Unsupported:
        case McuToolChainPackage::ToolChainType::GHS:
        case McuToolChainPackage::ToolChainType::GHSArm:
        case McuToolChainPackage::ToolChainType::IAR:
            return; // No Green Hills and IAR debugger, because support for it is missing.

        case McuToolChainPackage::ToolChainType::KEIL:
        case McuToolChainPackage::ToolChainType::MSVC:
        case McuToolChainPackage::ToolChainType::GCC:
        case McuToolChainPackage::ToolChainType::MinGW:
        case McuToolChainPackage::ToolChainType::ArmGcc: {
            const QVariant debuggerId = tcPackage->debuggerId();
            if (debuggerId.isValid()) {
                Debugger::DebuggerKitAspect::setDebugger(k, debuggerId);
            }
            return;
        }

        default:
            Q_UNREACHABLE();
        }
    }

    static void setKitDevice(Kit *k, const McuTarget *mcuTarget)
    {
        // "Device Type" Desktop is the default. We use that for the Qt for MCUs Desktop Kit
        if (mcuTarget->toolChainPackage()->isDesktopToolchain())
            return;

        DeviceTypeKitAspect::setDeviceTypeId(k, Constants::DEVICE_TYPE);
    }

    static void setKitDependencies(Kit *k,
                                   const McuTarget *mcuTarget,
                                   const McuPackagePtr &qtForMCUsSdkPackage)
    {
        NameValueItems dependencies;

        auto processPackage = [&dependencies](const McuPackagePtr &package) {
            const auto cmakeVariableName = package->cmakeVariableName();
            if (!cmakeVariableName.isEmpty())
                dependencies.append({cmakeVariableName, package->detectionPath().toUserOutput()});
        };
        for (const auto &package : mcuTarget->packages())
            processPackage(package);
        processPackage(qtForMCUsSdkPackage);

        McuDependenciesKitAspect::setDependencies(k, dependencies);

        auto irrelevant = k->irrelevantAspects();
        irrelevant.insert(McuDependenciesKitAspect::id());
        k->setIrrelevantAspects(irrelevant);
    }

    static void setKitEnvironment(Kit *k,
                                  const McuTarget *mcuTarget,
                                  const McuPackagePtr &qtForMCUsSdkPackage)
    {
        EnvironmentItems changes;
        QStringList pathAdditions; // clazy:exclude=inefficient-qlist-soft

        // The Desktop version depends on the Qt shared libs.
        // As CMake's fileApi is available, we can rely on the "Add library search path to PATH"
        // feature of the run configuration.
        //
        // Since MinGW support is added from Qul 2.3.0,
        // the Qt shared libs for Windows desktop platform have been moved
        // from Qul_DIR/bin to Qul_DIR/lib/(msvc|gnu)
        // and the QPA plugin has been moved to the same location.
        // So Windows host requires to add the path in this case.
        if (mcuTarget->toolChainPackage()->isDesktopToolchain() && HostOsInfo::isWindowsHost()
            && !McuSupportOptions::isLegacyVersion(mcuTarget->qulVersion())) {
            const FilePath libPath = (qtForMCUsSdkPackage->path() / "lib"
                                      / mcuTarget->desktopCompilerId());
            pathAdditions.append(libPath.toUserOutput());
            changes.append({"QT_QPA_PLATFORM_PLUGIN_PATH", libPath.toUserOutput()});
        }

        auto processPackage = [&pathAdditions](const McuPackagePtr &package) {
            if (package->isAddToSystemPath())
                pathAdditions.append(package->path().toUserOutput());
        };

        for (const auto &package : mcuTarget->packages())
            processPackage(package);
        processPackage(qtForMCUsSdkPackage);

        if (!pathAdditions.isEmpty()) {
            const QString path = QLatin1String(HostOsInfo::isWindowsHost() ? "Path" : "PATH");
            pathAdditions.append("${" + path + "}");
            changes.append({path, pathAdditions.join(HostOsInfo::pathListSeparator())});
        }

        if (McuSupportOptions::kitsNeedQtVersion())
            changes.append({QLatin1String("LD_LIBRARY_PATH"), "%{Qt:QT_INSTALL_LIBS}"});

        EnvironmentKitAspect::setEnvironmentChanges(k, changes);
    }

    static void setKitCMakeOptions(Kit *k,
                                   const McuTarget *mcuTarget,
                                   const McuPackagePtr &qtForMCUsSdkPackage)
    {
        using namespace CMakeProjectManager;
        auto configMap = cMakeConfigToMap(CMakeConfigurationKitAspect::configuration(k));

        // CMake ToolChain file for ghs handles CMAKE_*_COMPILER autonomously
        const QList autonomousCompilerDetectionToolchains{
            McuToolChainPackage::ToolChainType::GHS,
            McuToolChainPackage::ToolChainType::GHSArm,
        };
        if (!autonomousCompilerDetectionToolchains.contains(
                mcuTarget->toolChainPackage()->toolchainType())) {
            configMap.insert("CMAKE_CXX_COMPILER", "%{Compiler:Executable:Cxx}");
            configMap.insert("CMAKE_C_COMPILER", "%{Compiler:Executable:C}");
        }

        auto toolchainPackage = mcuTarget->toolChainPackage();
        if (toolchainPackage->isDesktopToolchain()) {
            auto cToolchain = toolchainPackage->toolChain(ProjectExplorer::Constants::C_LANGUAGE_ID);
            auto cxxToolchain = toolchainPackage->toolChain(
                ProjectExplorer::Constants::CXX_LANGUAGE_ID);

            if (cToolchain && cxxToolchain) {
                if (!cxxToolchain->compilerCommand().isEmpty()
                    && !cToolchain->compilerCommand().isEmpty()) {
                    configMap.insert("CMAKE_CXX_COMPILER",
                                     cxxToolchain->compilerCommand().toString().toLatin1());
                    configMap.insert("CMAKE_C_COMPILER",
                                     cToolchain->compilerCommand().toString().toLatin1());
                }
            } else {
                printMessage(Tr::tr("Warning for target %1: invalid toolchain path (%2). "
                                    "Update the toolchain in Edit > Preferences > Kits.")
                                 .arg(generateKitNameFromTarget(mcuTarget),
                                      toolchainPackage->path().toUserOutput()),
                             true);
            }

            if (!McuSupportOptions::isLegacyVersion(mcuTarget->qulVersion())
                && HostOsInfo::isWindowsHost()) {
                // From 2.3.0, QUL_COMPILER_NAME needs to be set on Windows
                // to select proper cmake files depending on the toolchain for Windows.
                configMap.insert("QUL_COMPILER_NAME", mcuTarget->desktopCompilerId().toLatin1());
            }
        } else {
            const FilePath cMakeToolchainFile = mcuTarget->toolChainFilePackage()->path();

            configMap.insert(Legacy::Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE,
                             cMakeToolchainFile.toString().toUtf8());
            if (!cMakeToolchainFile.exists()) {
                printMessage(
                    Tr::tr("Warning for target %1: missing CMake toolchain file expected at %2.")
                        .arg(generateKitNameFromTarget(mcuTarget),
                             cMakeToolchainFile.toUserOutput()),
                    false);
            }
        }

        const FilePath generatorsPath = qtForMCUsSdkPackage->path().pathAppended(
            "/lib/cmake/Qul/QulGenerators.cmake");
        configMap.insert("QUL_GENERATORS", generatorsPath.toString().toUtf8());
        if (!generatorsPath.exists()) {
            printMessage(Tr::tr("Warning for target %1: missing QulGenerators expected at %2.")
                             .arg(generateKitNameFromTarget(mcuTarget),
                                  generatorsPath.toUserOutput()),
                         false);
        }

        configMap.insert("QUL_PLATFORM", mcuTarget->platform().name.toLower().toUtf8());

        if (mcuTarget->colorDepth() != McuTarget::UnspecifiedColorDepth)
            configMap.insert("QUL_COLOR_DEPTH", QString::number(mcuTarget->colorDepth()).toLatin1());
        if (McuSupportOptions::kitsNeedQtVersion())
            configMap.insert("CMAKE_PREFIX_PATH", "%{Qt:QT_INSTALL_PREFIX}");

        if (HostOsInfo::isWindowsHost()) {
            auto type = mcuTarget->toolChainPackage()->toolchainType();
            if (type == McuToolChainPackage::ToolChainType::GHS
                || type == McuToolChainPackage::ToolChainType::GHSArm) {
                // See https://bugreports.qt.io/browse/UL-4247?focusedCommentId=565802&page=com.atlassian.jira.plugin.system.issuetabpanels:comment-tabpanel#comment-565802
                // and https://bugreports.qt.io/browse/UL-4247?focusedCommentId=565803&page=com.atlassian.jira.plugin.system.issuetabpanels:comment-tabpanel#comment-565803
                CMakeGeneratorKitAspect::setGenerator(k, "NMake Makefiles JOM");
            }
        }

        auto processPackage = [&configMap](const McuPackagePtr &package) {
            if (!package->cmakeVariableName().isEmpty())
                configMap.insert(package->cmakeVariableName().toUtf8(),
                                 package->path().toUserOutput().toUtf8());
        };

        for (auto package : mcuTarget->packages())
            processPackage(package);
        processPackage(qtForMCUsSdkPackage);

        CMakeConfigurationKitAspect::setConfiguration(k, mapToCMakeConfig(configMap));
    }

    static void setKitQtVersionOptions(Kit *k)
    {
        if (!McuSupportOptions::kitsNeedQtVersion())
            QtSupport::QtKitAspect::setQtVersion(k, nullptr);
        // else: auto-select a Qt version
    }

}; // class McuKitFactory

// Construct kit
Kit *newKit(const McuTarget *mcuTarget, const McuPackagePtr &qtForMCUsSdk)
{
    const auto init = [&mcuTarget, qtForMCUsSdk](Kit *k) {
        KitGuard kitGuard(k);

        McuKitFactory::setKitProperties(k, mcuTarget, qtForMCUsSdk->path());
        McuKitFactory::setKitDevice(k, mcuTarget);
        McuKitFactory::setKitToolchains(k, mcuTarget->toolChainPackage());
        McuKitFactory::setKitDebugger(k, mcuTarget->toolChainPackage());
        McuKitFactory::setKitEnvironment(k, mcuTarget, qtForMCUsSdk);
        McuKitFactory::setKitCMakeOptions(k, mcuTarget, qtForMCUsSdk);
        McuKitFactory::setKitDependencies(k, mcuTarget, qtForMCUsSdk);
        McuKitFactory::setKitQtVersionOptions(k);

        k->setup();
        k->fix();
    };

    Kit *kit = KitManager::registerKit(init);
    if (kit) {
        printMessage(Tr::tr("Kit for %1 created.").arg(generateKitNameFromTarget(mcuTarget)), false);
    } else {
        printMessage(Tr::tr("Error registering Kit for %1.")
                         .arg(generateKitNameFromTarget(mcuTarget)),
                     true);
    }
    return kit;
}

// Kit Information
QString generateKitNameFromTarget(const McuTarget *mcuTarget)
{
    McuToolChainPackagePtr tcPkg = mcuTarget->toolChainPackage();
    const QString compilerName = tcPkg ? QString::fromLatin1(" (%1)").arg(
                                     tcPkg->toolChainName().toUpper())
                                       : "";
    const QString colorDepth = mcuTarget->colorDepth() != McuTarget::UnspecifiedColorDepth
                                   ? QString::fromLatin1(" %1bpp").arg(mcuTarget->colorDepth())
                                   : "";
    const QString targetName = mcuTarget->platform().displayName.isEmpty()
                                   ? mcuTarget->platform().name
                                   : mcuTarget->platform().displayName;
    return QString::fromLatin1("Qt for MCUs %1.%2 - %3%4%5")
        .arg(QString::number(mcuTarget->qulVersion().majorVersion()),
             QString::number(mcuTarget->qulVersion().minorVersion()),
             targetName,
             colorDepth,
             compilerName);
}

// Kit Information
QVersionNumber kitQulVersion(const Kit *kit)
{
    return QVersionNumber::fromString(
        kit->value(Constants::KIT_MCUTARGET_SDKVERSION_KEY).toString());
}

// Kit Information
static FilePath kitDependencyPath(const Kit *kit, const QString &cmakeVariableName)
{
    const auto config = CMakeConfigurationKitAspect::configuration(kit).toList();
    const auto keyName = cmakeVariableName.toUtf8();
    for (const CMakeConfigItem &configItem : config) {
        if (configItem.key == keyName)
            return FilePath::fromUserInput(QString::fromUtf8(configItem.value));
    }
    return {};
}

// Kit Information
bool kitIsUpToDate(const Kit *kit,
                   const McuTarget *mcuTarget,
                   const McuPackagePtr &qtForMCUsSdkPackage)
{
    return kitQulVersion(kit) == mcuTarget->qulVersion()
           && kitDependencyPath(kit, qtForMCUsSdkPackage->cmakeVariableName()).toUserOutput()
                  == qtForMCUsSdkPackage->path().toUserOutput();
}

// Queries
QList<Kit *> existingKits(const McuTarget *mcuTarget)
{
    using namespace Constants;
    // some models have compatible name changes that refere to the same supported board across versions.
    // name changes are tracked here to recognize the corresponding kits as upgradable.
    static const QMap<QString, QStringList> upgradable_to = {
        {"MIMXRT1170-EVK-FREERTOS", {"MIMXRT1170-EVKB-FREERTOS"}}};
    return Utils::filtered(KitManager::kits(), [mcuTarget](Kit *kit) {
        return kit->value(KIT_MCUTARGET_KITVERSION_KEY) == KIT_VERSION
               && (!mcuTarget
                   || (kit->value(KIT_MCUTARGET_VENDOR_KEY) == mcuTarget->platform().vendor
                       && (kit->value(KIT_MCUTARGET_MODEL_KEY) == mcuTarget->platform().name
                           || upgradable_to[kit->value(KIT_MCUTARGET_MODEL_KEY).toString()].contains(
                               mcuTarget->platform().name))
                       && kit->value(KIT_MCUTARGET_COLORDEPTH_KEY) == mcuTarget->colorDepth()
                       && kit->value(KIT_MCUTARGET_OS_KEY).toInt()
                              == static_cast<int>(mcuTarget->os())
                       && kit->value(KIT_MCUTARGET_TOOLCHAIN_KEY)
                              == mcuTarget->toolChainPackage()->toolChainName()));
    });
}

// Queries
QList<Kit *> matchingKits(const McuTarget *mcuTarget, const McuPackagePtr &qtForMCUsSdkPackage)
{
    return Utils::filtered(existingKits(mcuTarget), [&mcuTarget, qtForMCUsSdkPackage](Kit *kit) {
        return kitIsUpToDate(kit, mcuTarget, qtForMCUsSdkPackage);
    });
}

// Queries
QList<Kit *> upgradeableKits(const McuTarget *mcuTarget, const McuPackagePtr &qtForMCUsSdkPackage)
{
    return Utils::filtered(existingKits(mcuTarget), [&mcuTarget, qtForMCUsSdkPackage](Kit *kit) {
        return !kitIsUpToDate(kit, mcuTarget, qtForMCUsSdkPackage);
    });
}

// Queries
QList<Kit *> kitsWithMismatchedDependencies(const McuTarget *mcuTarget)
{
    return Utils::filtered(existingKits(mcuTarget), [&mcuTarget](Kit *kit) {
        const auto entries = Utils::NameValueDictionary(
            McuDependenciesKitAspect::configuration(kit));
        return Utils::anyOf(mcuTarget->packages(), [&entries](const McuPackagePtr &package) {
            const QString cmakeVariableName = package->cmakeVariableName();
            return !cmakeVariableName.isEmpty()
                   && entries.value(cmakeVariableName) != package->path().toUserOutput();
        });
    });
}

// Queries
QList<Kit *> outdatedKits()
{
    return Utils::filtered(KitManager::kits(), [](Kit *kit) {
        return !kit->value(Constants::KIT_MCUTARGET_VENDOR_KEY).isNull()
               && kit->value(Constants::KIT_MCUTARGET_KITVERSION_KEY) != KIT_VERSION;
    });
}

// Maintenance
void createAutomaticKits(const SettingsHandler::Ptr &settingsHandler)
{
    McuPackagePtr qtForMCUsPackage{createQtForMCUsPackage(settingsHandler)};

    // add a list of package, board, errormessage,
    MessagesList autoGenerationMessages;

    const auto createKits = [&autoGenerationMessages, qtForMCUsPackage, settingsHandler] {
        if (settingsHandler->isAutomaticKitCreationEnabled()) {
            qtForMCUsPackage->updateStatus();
            if (!qtForMCUsPackage->isValidStatus()) {
                switch (qtForMCUsPackage->status()) {
                case McuAbstractPackage::Status::ValidPathInvalidPackage: {
                    const QString message
                        = Tr::tr("Path %1 exists, but does not contain %2.")
                              .arg(qtForMCUsPackage->path().toUserOutput(),
                                   qtForMCUsPackage->detectionPath().toUserOutput());
                    autoGenerationMessages.push_back({qtForMCUsPackage->label(), "", message});
                    printMessage(message, true);
                    break;
                }
                case McuAbstractPackage::Status::InvalidPath: {
                    const QString message
                        = Tr::tr("Path %1 does not exist. Add the path in Edit > Preferences > "
                                 "Devices > MCU.")
                              .arg(qtForMCUsPackage->path().toUserOutput());
                    autoGenerationMessages.push_back({qtForMCUsPackage->label(), "", message});
                    printMessage(message, true);
                    break;
                }
                case McuAbstractPackage::Status::EmptyPath: {
                    const QString message = Tr::tr("Missing %1. Add the path in Edit > Preferences > Devices > MCU.")
                            .arg(qtForMCUsPackage->detectionPath().toUserOutput());
                    autoGenerationMessages.push_back({qtForMCUsPackage->label(), "", message});
                    printMessage(message, true);
                    return;
                }
                default:
                    break;
                }
                return;
            }

            if (CMakeProjectManager::CMakeToolManager::cmakeTools().isEmpty()) {
                const QString message = Tr::tr("No CMake tool was detected. Add a CMake tool in Edit > Preferences > "
                           "Kits > CMake.");
                autoGenerationMessages.push_back({qtForMCUsPackage->label(), "", message});
                printMessage(message, true);
                return;
            }

            McuSdkRepository repo{targetsAndPackages(qtForMCUsPackage, settingsHandler)};
            McuSdkRepository::updateQtDirMacro(qtForMCUsPackage->path());
            repo.expandVariablesAndWildcards();

            bool needsUpgrade = false;
            for (const auto &target : std::as_const(repo.mcuTargets)) {
                // if kit already exists, skip
                if (!matchingKits(target.get(), qtForMCUsPackage).empty())
                    continue;
                if (!upgradeableKits(target.get(), qtForMCUsPackage).empty()) {
                    // if kit exists but wrong version/path
                    needsUpgrade = true;
                } else {
                    // if no kits for this target, create
                    if (target->isValid())
                        newKit(target.get(), qtForMCUsPackage);
                    target->handlePackageProblems(autoGenerationMessages);
                }
            }
            if (needsUpgrade)
                McuSupportPlugin::askUserAboutMcuSupportKitsUpgrade(settingsHandler);
        }
    };

    createKits();
    McuSupportOptions::displayKitCreationMessages(autoGenerationMessages,
                                                  settingsHandler,
                                                  qtForMCUsPackage);
}

// Maintenance
// when the SDK version has changed, and the user has given permission
// to upgrade, create new kits with current data, for the targets
// for which kits already existed
// function parameter is option to keep the old ones or delete them
void upgradeKitsByCreatingNewPackage(const SettingsHandler::Ptr &settingsHandler,
                                     UpgradeOption upgradeOption)
{
    if (upgradeOption == UpgradeOption::Ignore)
        return;

    McuPackagePtr qtForMCUsPackage{createQtForMCUsPackage(settingsHandler)};

    McuSdkRepository repo{targetsAndPackages(qtForMCUsPackage, settingsHandler)};

    MessagesList messages;
    for (const auto &target : std::as_const(repo.mcuTargets)) {
        if (!matchingKits(target.get(), qtForMCUsPackage).empty())
            // already up-to-date
            continue;

        const auto kits = upgradeableKits(target.get(), qtForMCUsPackage);
        if (!kits.empty()) {
            if (upgradeOption == UpgradeOption::Replace) {
                for (auto existingKit : kits)
                    KitManager::deregisterKit(existingKit);
                // Reset cached values that are not valid after an update
                // Exp: a board sdk version that was dropped in newer releases
                target->resetInvalidPathsToDefault();
            }

            if (target->isValid())
                newKit(target.get(), qtForMCUsPackage);
            target->handlePackageProblems(messages);
        }
    }
    // Open the dialog showing warnings and errors in packages
    McuSupportOptions::displayKitCreationMessages(messages, settingsHandler, qtForMCUsPackage);
}

// Maintenance
// when the user manually asks to upgrade a specific kit
// button is available if SDK version changed
void upgradeKitInPlace(ProjectExplorer::Kit *kit,
                       const McuTarget *mcuTarget,
                       const McuPackagePtr &qtForMCUsSdk)
{
    McuKitFactory::setKitProperties(kit, mcuTarget, qtForMCUsSdk->path());
    McuKitFactory::setKitEnvironment(kit, mcuTarget, qtForMCUsSdk);
    McuKitFactory::setKitCMakeOptions(kit, mcuTarget, qtForMCUsSdk);
    McuKitFactory::setKitDependencies(kit, mcuTarget, qtForMCUsSdk);
}

// Maintenance
// If the user changed a path in the McuSupport plugin's UI
// update the corresponding cmake variables in all existing kits
void updatePathsInExistingKits(const SettingsHandler::Ptr &settingsHandler)
{
    McuPackagePtr qtForMCUsPackage{createQtForMCUsPackage(settingsHandler)};

    McuSdkRepository repo{targetsAndPackages(qtForMCUsPackage, settingsHandler)};
    for (const auto &target : std::as_const(repo.mcuTargets)) {
        if (target->isValid()) {
            for (auto *kit : kitsWithMismatchedDependencies(target.get())) {
                if (kitQulVersion(kit) != target->qulVersion()) {
                    //Do not update kits made for other Qt for MCUs SDK versions
                    continue;
                }

                auto changes = cMakeConfigToMap(CMakeConfigurationKitAspect::configuration(kit));

                const auto updateForPackage = [&changes](const McuPackagePtr &package) {
                    if (!package->cmakeVariableName().isEmpty() && package->isValidStatus()) {
                        changes.insert(package->cmakeVariableName().toUtf8(),
                                       package->path().toUserOutput().toUtf8());
                    }
                };

                for (const auto &package : target->packages()) {
                    updateForPackage(package);
                }
                updateForPackage(qtForMCUsPackage);

                CMakeConfigurationKitAspect::setConfiguration(kit,
                                                              CMakeProjectManager::CMakeConfig(
                                                                  mapToCMakeConfig(changes)));
            }
        }
    }
}

// Maintenance
// if we changed minor details in the kits across versions of QtCreator
// this function updates those details in existing older kits
void fixExistingKits(const SettingsHandler::Ptr &settingsHandler)
{
    for (Kit *kit : KitManager::kits()) {
        if (!kit->hasValue(Constants::KIT_MCUTARGET_KITVERSION_KEY))
            continue;

        if (kit->isAutoDetected()) {
            kit->setAutoDetected(false);
        }

        // Check if the MCU kits are flagged as supplying a QtQuick import path, in order
        // to tell the QMLJS code-model that it won't need to add a fall-back import
        // path.
        const Id bringsQtQuickImportPath = QtSupport::Constants::FLAGS_SUPPLIES_QTQUICK_IMPORT_PATH;
        auto irrelevantAspects = kit->irrelevantAspects();
        if (!irrelevantAspects.contains(bringsQtQuickImportPath)) {
            irrelevantAspects.insert(bringsQtQuickImportPath);
            kit->setIrrelevantAspects(irrelevantAspects);
        }
        if (!kit->hasValue(bringsQtQuickImportPath)) {
            kit->setValue(bringsQtQuickImportPath, true);
        }

        // Check if the MCU kit supplies its import path.
        const Id kitQmlImportPath = QtSupport::Constants::KIT_QML_IMPORT_PATH;
        if (!irrelevantAspects.contains(kitQmlImportPath)) {
            irrelevantAspects.insert(kitQmlImportPath);
            kit->setIrrelevantAspects(irrelevantAspects);
        }
        if (!kit->hasValue(kitQmlImportPath)) {
            auto config = CMakeProjectManager::CMakeConfigurationKitAspect::configuration(kit);
            for (const auto &cfgItem : std::as_const(config)) {
                if (cfgItem.key == "QUL_GENERATORS") {
                    auto idx = cfgItem.value.indexOf("/lib/cmake/Qul");
                    auto qulDir = cfgItem.value.left(idx);
                    // FIXME: This is treated as a pathlist in CMakeBuildSystem::updateQmlJSCodeModel
                    kit->setValue(kitQmlImportPath, QVariant(qulDir + "/include/qul"));
                    break;
                }
            }
        }

        // Check if the MCU kit has the flag for merged header/qml-import paths set.
        const Id mergedPaths = QtSupport::Constants::KIT_HAS_MERGED_HEADER_PATHS_WITH_QML_IMPORT_PATHS;
        if (!irrelevantAspects.contains(mergedPaths)) {
            irrelevantAspects.insert(mergedPaths);
            kit->setIrrelevantAspects(irrelevantAspects);
        }
        if (!kit->value(mergedPaths, false).toBool()) {
            kit->setValue(mergedPaths, true);
        }
    }

    // Fix kit dependencies for known targets
    McuPackagePtr qtForMCUsPackage{createQtForMCUsPackage(settingsHandler)};
    qtForMCUsPackage->updateStatus();
    if (qtForMCUsPackage->isValidStatus()) {
        McuSdkRepository repo{targetsAndPackages(qtForMCUsPackage, settingsHandler)};
        for (const auto &target : std::as_const(repo.mcuTargets))
            for (auto kit : existingKits(target.get())) {
                if (McuDependenciesKitAspect::dependencies(kit).isEmpty()) {
                    McuKitFactory::setKitCMakeOptions(kit, target.get(), qtForMCUsPackage);
                    McuKitFactory::setKitDependencies(kit, target.get(), qtForMCUsPackage);
                }
            }
    }
}

// Maintenance
// removes kits with older schemes
void removeOutdatedKits()
{
    for (auto kit : outdatedKits())
        KitManager::deregisterKit(kit);
}

/*
* using kitQmlImportPath of kit found in profile.xml to get the path to Qul
* installation where description file (.json) of the kit is located.
*/
static const FilePaths kitsFiles(const Kit *kit)
{
    const FilePath qulRoot = kitDependencyPath(kit, Legacy::Constants::QUL_CMAKE_VAR);
    return kitsPath(qulRoot).dirEntries(Utils::FileFilter({"*.json"}, QDir::Files));
}

/*
* When file description (.json) of a kit exists in the Qul installation that means
* target is installed.
*/
static bool anyKitDescriptionFileExists(const FilePaths &jsonFiles,
                                        const QStringList &kitsProperties)
{
    static const QRegularExpression re("(\\w+)-(\\w+)-(.+)\\.json");
    for (const FilePath &jsonFile : jsonFiles) {
        const QRegularExpressionMatch match = re.match(jsonFile.fileName());
        QStringList kitsPropertiesFromFileName;
        if (match.hasMatch()) {
            QString toolchain = match.captured(1);
            const QString vendor = match.captured(2);
            const QString device = match.captured(3);

            /*
            * file name of kit starts with "gnu" while in profiles.xml name of
            * toolchain is "gcc" on Linux and "mingw" on Windows
            */
            toolchain = HostOsInfo::isLinuxHost() ? toolchain.replace("gnu", "gcc")
                                                  : toolchain.replace("gnu", "mingw");

            kitsPropertiesFromFileName << toolchain << vendor << device;
        }

        if (kitsPropertiesFromFileName == kitsProperties)
            return true;
    }
    return false;
}

const QList<Kit *> findUninstalledTargetsKits()
{
    QList<Kit *> uninstalledTargetsKits;
    for (Kit *kit : KitManager::kits()) {
        if (!kit->hasValue(Constants::KIT_MCUTARGET_KITVERSION_KEY))
            continue;

        const QStringList kitsProperties = {
            kit->value(Constants::KIT_MCUTARGET_TOOLCHAIN_KEY).toString().toLower(),
            kit->value(Constants::KIT_MCUTARGET_VENDOR_KEY).toString().toLower(),
            kit->value(Constants::KIT_MCUTARGET_MODEL_KEY).toString().toLower(),
        };

        const FilePaths kitsDescriptionFiles = kitsFiles(kit);
        if (!anyKitDescriptionFileExists(kitsDescriptionFiles, kitsProperties))
            uninstalledTargetsKits << kit;
    }

    return uninstalledTargetsKits;
}

void removeUninstalledTargetsKits(const QList<Kit *> uninstalledTargetsKits)
{
    for (const auto &kit : uninstalledTargetsKits)
        KitManager::deregisterKit(kit);
}

} // namespace McuKitManager
} // namespace McuSupport::Internal
