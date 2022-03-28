/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "mcusupportsdk.h"
#include "mcuhelpers.h"
#include "mcukitmanager.h"
#include "mcupackage.h"
#include "mcusupportconstants.h"
#include "mcusupportoptions.h"
#include "mcusupportplugin.h"
#include "mcusupportversiondetection.h"
#include "mcutarget.h"
#include "mcutargetdescription.h"
#include "mcutargetfactory.h"
#include "mcutargetfactorylegacy.h"

#include <baremetal/baremetalconstants.h>
#include <coreplugin/icore.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QDirIterator>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>

#include <memory>

using namespace Utils;

namespace McuSupport {
namespace Internal {
namespace Sdk {

namespace {
const char CMAKE_ENTRIES[]{"cmakeEntries"};
const char ID[]{"id"};
} // namespace

static FilePath findInProgramFiles(const QString &folder)
{
    for (auto envVar : {"ProgramFiles", "ProgramFiles(x86)", "ProgramW6432"}) {
        if (!qEnvironmentVariableIsSet(envVar))
            continue;
        const FilePath dir = FilePath::fromUserInput(qEnvironmentVariable(envVar)) / folder;
        if (dir.exists())
            return dir;
    }
    return {};
}

McuAbstractPackage *createQtForMCUsPackage()
{
    return new McuPackage(McuPackage::tr("Qt for MCUs SDK"),
                          FileUtils::homePath(),                           // defaultPath
                          FilePath("bin/qmltocpp").withExecutableSuffix(), // detectionPath
                          Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK, // settingsKey
                          QStringLiteral("Qul_ROOT"),                      // cmakeVarName
                          QStringLiteral("Qul_DIR"));                      // envVarName
}

static McuPackageVersionDetector *generatePackageVersionDetector(const QString &envVar)
{
    if (envVar.startsWith("EVK"))
        return new McuPackageXmlVersionDetector("*_manifest_*.xml", "ksdk", "version", ".*");

    if (envVar.startsWith("STM32"))
        return new McuPackageXmlVersionDetector("package.xml",
                                                "PackDescription",
                                                "Release",
                                                R"(\b(\d+\.\d+\.\d+)\b)");

    if (envVar.startsWith("RGL"))
        return new McuPackageDirectoryVersionDetector("rgl_*_obj_*", R"(\d+\.\d+\.\w+)", false);

    return nullptr;
}

/// Create the McuPackage by checking the "boardSdk" property in the JSON file for the board.
/// The name of the environment variable pointing to the the SDK for the board will be defined in the "envVar" property
/// inside the "boardSdk".
McuAbstractPackage *createBoardSdkPackage(const McuTargetDescription &desc)
{
    const auto generateSdkName = [](const QString &envVar) {
        qsizetype postfixPos = envVar.indexOf("_SDK_PATH");
        if (postfixPos < 0) {
            postfixPos = envVar.indexOf("_DIR");
        }
        const QString sdkName = postfixPos > 0 ? envVar.left(postfixPos) : envVar;
        return QString{"MCU SDK (%1)"}.arg(sdkName);
    };
    const QString sdkName = desc.boardSdk.name.isEmpty() ? generateSdkName(desc.boardSdk.envVar)
                                                         : desc.boardSdk.name;

    const FilePath defaultPath = [&] {
        const auto envVar = desc.boardSdk.envVar.toLatin1();
        if (qEnvironmentVariableIsSet(envVar))
            return FilePath::fromUserInput(qEnvironmentVariable(envVar));
        if (!desc.boardSdk.defaultPath.isEmpty()) {
            FilePath defaultPath = FilePath::fromUserInput(QDir::rootPath()
                                                           + desc.boardSdk.defaultPath);
            if (defaultPath.exists())
                return defaultPath;
        }
        return FilePath();
    }();

    const auto versionDetector = generatePackageVersionDetector(desc.boardSdk.envVar);

    return new McuPackage(sdkName,
                          defaultPath,
                          {},                   // detection path
                          desc.boardSdk.envVar, // settings key
                          "QUL_BOARD_SDK_DIR",  // cmake var
                          desc.boardSdk.envVar, // env var
                          {},                   // download URL
                          versionDetector);
}

McuAbstractPackage *createFreeRTOSSourcesPackage(const QString &envVar,
                                                 const FilePath &boardSdkDir,
                                                 const QString &freeRTOSBoardSdkSubDir)
{
    const QString envVarPrefix = removeRtosSuffix(envVar);

    FilePath defaultPath;
    if (qEnvironmentVariableIsSet(envVar.toLatin1()))
        defaultPath = FilePath::fromUserInput(qEnvironmentVariable(envVar.toLatin1()));
    else if (!boardSdkDir.isEmpty() && !freeRTOSBoardSdkSubDir.isEmpty())
        defaultPath = boardSdkDir / freeRTOSBoardSdkSubDir;

    return new McuPackage(QString::fromLatin1("FreeRTOS Sources (%1)").arg(envVarPrefix),
                          defaultPath,
                          {}, // detection path
                          QString{Constants::SETTINGS_KEY_FREERTOS_PREFIX}.append(envVarPrefix),
                          "FREERTOS_DIR",          // cmake var
                          envVar,                  // env var
                          "https://freertos.org"); // download url
}

McuToolChainPackage *createUnsupportedToolChainPackage()
{
    return new McuToolChainPackage({}, {}, {}, {}, McuToolChainPackage::ToolChainType::Unsupported);
}

static McuToolChainPackage *createMsvcToolChainPackage()
{
    return new McuToolChainPackage({}, {}, {}, {}, McuToolChainPackage::ToolChainType::MSVC);
}

static McuToolChainPackage *createGccToolChainPackage()
{
    return new McuToolChainPackage({}, {}, {}, {}, McuToolChainPackage::ToolChainType::GCC);
}

static McuToolChainPackage *createArmGccToolchainPackage()
{
    const char envVar[] = "ARMGCC_DIR";

    FilePath defaultPath;
    if (qEnvironmentVariableIsSet(envVar))
        defaultPath = FilePath::fromUserInput(qEnvironmentVariable(envVar));
    if (defaultPath.isEmpty() && HostOsInfo::isWindowsHost()) {
        const FilePath installDir = findInProgramFiles("GNU Tools ARM Embedded");
        if (installDir.exists()) {
            // If GNU Tools installation dir has only one sub dir,
            // select the sub dir, otherwise the installation dir.
            const FilePaths subDirs = installDir.dirEntries(QDir::Dirs | QDir::NoDotAndDotDot);
            if (subDirs.count() == 1)
                defaultPath = subDirs.first();
        }
    }

    const Utils::FilePath detectionPath = FilePath("bin/arm-none-eabi-g++").withExecutableSuffix();
    const auto versionDetector
        = new McuPackageExecutableVersionDetector(detectionPath,
                                                  {"--version"},
                                                  "\\b(\\d+\\.\\d+\\.\\d+)\\b");

    return new McuToolChainPackage(McuPackage::tr("GNU Arm Embedded Toolchain"),
                                   defaultPath,
                                   detectionPath,
                                   "GNUArmEmbeddedToolchain",                  // settingsKey
                                   McuToolChainPackage::ToolChainType::ArmGcc, // toolchainType
                                   "QUL_TARGET_TOOLCHAIN_DIR",                 // cmake var
                                   envVar,                                     // env var
                                   versionDetector);
}

static McuToolChainPackage *createGhsToolchainPackage()
{
    const char envVar[] = "GHS_COMPILER_DIR";

    const FilePath defaultPath = FilePath::fromUserInput(qEnvironmentVariable(envVar));

    const auto versionDetector
        = new McuPackageExecutableVersionDetector(FilePath("as850").withExecutableSuffix(),
                                                  {"-V"},
                                                  "\\bv(\\d+\\.\\d+\\.\\d+)\\b");

    return new McuToolChainPackage("Green Hills Compiler",
                                   defaultPath,
                                   FilePath("ccv850").withExecutableSuffix(), // detectionPath
                                   "GHSToolchain",                            // settingsKey
                                   McuToolChainPackage::ToolChainType::GHS,   // toolchainType
                                   "QUL_TARGET_TOOLCHAIN_DIR",                // cmake var
                                   envVar,                                    // env var
                                   versionDetector);
}

static McuToolChainPackage *createGhsArmToolchainPackage()
{
    const char envVar[] = "GHS_ARM_COMPILER_DIR";

    const FilePath defaultPath = FilePath::fromUserInput(qEnvironmentVariable(envVar));

    const auto versionDetector
        = new McuPackageExecutableVersionDetector(FilePath("asarm").withExecutableSuffix(),
                                                  {"-V"},
                                                  "\\bv(\\d+\\.\\d+\\.\\d+)\\b");

    return new McuToolChainPackage("Green Hills Compiler for ARM",
                                   defaultPath,
                                   FilePath("cxarm").withExecutableSuffix(),   // detectionPath
                                   "GHSArmToolchain",                          // settingsKey
                                   McuToolChainPackage::ToolChainType::GHSArm, // toolchainType
                                   "QUL_TARGET_TOOLCHAIN_DIR",                 // cmake var
                                   envVar,                                     // env var
                                   versionDetector);
}

static McuToolChainPackage *createIarToolChainPackage()
{
    const char envVar[] = "IAR_ARM_COMPILER_DIR";

    FilePath defaultPath;
    if (qEnvironmentVariableIsSet(envVar))
        defaultPath = FilePath::fromUserInput(qEnvironmentVariable(envVar));
    else {
        const ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainManager::toolChain(
            [](const ProjectExplorer::ToolChain *t) {
                return t->typeId() == BareMetal::Constants::IAREW_TOOLCHAIN_TYPEID;
            });
        if (tc) {
            const FilePath compilerExecPath = tc->compilerCommand();
            defaultPath = compilerExecPath.parentDir().parentDir();
        }
    }

    const FilePath detectionPath = FilePath("bin/iccarm").withExecutableSuffix();
    const auto versionDetector
        = new McuPackageExecutableVersionDetector(detectionPath,
                                                  {"--version"},
                                                  "\\bV(\\d+\\.\\d+\\.\\d+)\\.\\d+\\b");

    return new McuToolChainPackage("IAR ARM Compiler",
                                   defaultPath,
                                   detectionPath,
                                   "IARToolchain",                          // settings key
                                   McuToolChainPackage::ToolChainType::IAR, // toolchainType
                                   "QUL_TARGET_TOOLCHAIN_DIR",              // cmake var
                                   envVar,                                  // env var
                                   versionDetector);
}

static McuPackage *createStm32CubeProgrammerPackage()
{
    FilePath defaultPath;
    const QString cubePath = "STMicroelectronics/STM32Cube/STM32CubeProgrammer";
    if (HostOsInfo::isWindowsHost()) {
        const FilePath programPath = findInProgramFiles(cubePath);
        if (!programPath.isEmpty())
            defaultPath = programPath;
    } else {
        const FilePath programPath = FileUtils::homePath() / cubePath;
        if (programPath.exists())
            defaultPath = programPath;
    }

    const FilePath detectionPath = FilePath::fromString(
        QLatin1String(Utils::HostOsInfo::isWindowsHost() ? "/bin/STM32_Programmer_CLI.exe"
                                                         : "/bin/STM32_Programmer.sh"));

    auto result
        = new McuPackage(McuPackage::tr("STM32CubeProgrammer"),
                         defaultPath,
                         detectionPath,
                         "Stm32CubeProgrammer",
                         {},                                                           // cmake var
                         {},                                                           // env var
                         "https://www.st.com/en/development-tools/stm32cubeprog.html", // download url
                         nullptr, // version detector
                         true,    // add to path
                         "/bin"   // relative path modifier
        );
    return result;
}

static McuPackage *createMcuXpressoIdePackage()
{
    const char envVar[] = "MCUXpressoIDE_PATH";

    FilePath defaultPath;
    if (qEnvironmentVariableIsSet(envVar)) {
        defaultPath = FilePath::fromUserInput(qEnvironmentVariable(envVar));
    } else if (HostOsInfo::isWindowsHost()) {
        const FilePath programPath = FilePath::fromString(QDir::rootPath()) / "nxp";
        if (programPath.exists()) {
            defaultPath = programPath;
            // If default dir has exactly one sub dir that could be the IDE path, pre-select that.
            const FilePaths subDirs = defaultPath.dirEntries(
                {{"MCUXpressoIDE*"}, QDir::Dirs | QDir::NoDotAndDotDot});
            if (subDirs.count() == 1)
                defaultPath = subDirs.first();
        }
    } else {
        const FilePath programPath = FilePath::fromString("/usr/local/mcuxpressoide/");
        if (programPath.exists())
            defaultPath = programPath;
    }

    return new McuPackage("MCUXpresso IDE",
                          defaultPath,
                          FilePath("ide/binaries/crt_emu_cm_redlink")
                              .withExecutableSuffix(), // detection path
                          "MCUXpressoIDE",             // settings key
                          "MCUXPRESSO_IDE_PATH",       // cmake var
                          envVar,
                          "https://www.nxp.com/mcuxpresso/ide"); // download url
}

static McuPackage *createCypressProgrammerPackage()
{
    const char envVar[] = "CYPRESS_AUTO_FLASH_UTILITY_DIR";

    FilePath defaultPath;
    if (qEnvironmentVariableIsSet(envVar)) {
        defaultPath = FilePath::fromUserInput(qEnvironmentVariable(envVar));
    } else if (HostOsInfo::isWindowsHost()) {
        const FilePath candidate = findInProgramFiles("Cypress");
        if (candidate.exists()) {
            // "Cypress Auto Flash Utility 1.0"
            const auto subDirs = candidate.dirEntries({{"Cypress Auto Flash Utility*"}, QDir::Dirs},
                                                      QDir::Unsorted);
            if (!subDirs.empty())
                defaultPath = subDirs.first();
        }
    }

    auto result = new McuPackage("Cypress Auto Flash Utility",
                                 defaultPath,
                                 FilePath("/bin/openocd").withExecutableSuffix(),
                                 "CypressAutoFlashUtil",            // settings key
                                 "INFINEON_AUTO_FLASH_UTILITY_DIR", // cmake var
                                 envVar);                           // env var
    return result;
}

static McuPackage *createRenesasProgrammerPackage()
{
    const char envVar[] = "RenesasFlashProgrammer_PATH";

    FilePath defaultPath;
    if (qEnvironmentVariableIsSet(envVar)) {
        defaultPath = FilePath::fromUserInput(qEnvironmentVariable(envVar));
    } else if (HostOsInfo::isWindowsHost()) {
        const FilePath candidate = findInProgramFiles("Renesas Electronics/Programming Tools");
        if (candidate.exists()) {
            // "Renesas Flash Programmer V3.09"
            const auto subDirs = candidate.dirEntries({{"Renesas Flash Programmer*"}, QDir::Dirs},
                                                      QDir::Unsorted);
            if (!subDirs.empty())
                defaultPath = subDirs.first();
        }
    }

    auto result = new McuPackage("Renesas Flash Programmer",
                                 defaultPath,
                                 FilePath("rfp-cli").withExecutableSuffix(),
                                 "RenesasFlashProgrammer",        // settings key
                                 "RENESAS_FLASH_PROGRAMMER_PATH", // cmake var
                                 envVar);                         // env var
    return result;
}

static McuAbstractTargetFactory::Ptr createFactory(bool isLegacy)
{
    McuAbstractTargetFactory::Ptr result;
    if (isLegacy) {
        static const QHash<QString, McuToolChainPackagePtr> tcPkgs = {
            {{"armgcc"}, McuToolChainPackagePtr{createArmGccToolchainPackage()}},
            {{"greenhills"}, McuToolChainPackagePtr{createGhsToolchainPackage()}},
            {{"iar"}, McuToolChainPackagePtr{createIarToolChainPackage()}},
            {{"msvc"}, McuToolChainPackagePtr{createMsvcToolChainPackage()}},
            {{"gcc"}, McuToolChainPackagePtr{createGccToolChainPackage()}},
            {{"arm-greenhills"}, McuToolChainPackagePtr{createGhsArmToolchainPackage()}},
        };

        // Note: the vendor name (the key of the hash) is case-sensitive. It has to match the "platformVendor" key in the
        // json file.
        static const QHash<QString, McuPackagePtr> vendorPkgs = {
            {{"ST"}, McuPackagePtr{createStm32CubeProgrammerPackage()}},
            {{"NXP"}, McuPackagePtr{createMcuXpressoIdePackage()}},
            {{"CYPRESS"}, McuPackagePtr{createCypressProgrammerPackage()}},
            {{"RENESAS"}, McuPackagePtr{createRenesasProgrammerPackage()}},
        };

        result = std::make_unique<McuTargetFactoryLegacy>(tcPkgs, vendorPkgs);
    } else {
        result = std::make_unique<McuTargetFactory>();
    }
    return result;
}

McuSdkRepository targetsFromDescriptions(const QList<McuTargetDescription> &descriptions,
                                         bool isLegacy)
{
    Targets mcuTargets;
    Packages mcuPackages;

    McuAbstractTargetFactory::Ptr targetFactory = createFactory(isLegacy);
    for (const McuTargetDescription &desc : descriptions) {
        auto [targets, packages] = targetFactory->createTargets(desc);
        mcuTargets.append(targets);
        mcuPackages.unite(packages);
    }

    if (isLegacy) {
        auto [toolchainPkgs, vendorPkgs]{targetFactory->getAdditionalPackages()};
        for (McuToolChainPackagePtr &package : toolchainPkgs) {
            mcuPackages.insert(package);
        }
        for (McuPackagePtr &package : vendorPkgs) {
            mcuPackages.insert(package);
        }
    }
    return McuSdkRepository{mcuTargets, mcuPackages};
}

Utils::FilePath kitsPath(const Utils::FilePath &dir)
{
    return dir / "kits/";
}

static QFileInfoList targetDescriptionFiles(const Utils::FilePath &dir)
{
    const QDir kitsDir(kitsPath(dir).toString(), "*.json");
    return kitsDir.entryInfoList();
}

static QList<PackageDescription> parsePackages(const QJsonArray &cmakeEntries)
{
    QList<PackageDescription> result;
    for (const auto &cmakeEntryRef : cmakeEntries) {
        const QJsonObject cmakeEntry{cmakeEntryRef.toObject()};
        result.push_back({cmakeEntry[ID].toString(),
                          cmakeEntry["envVar"].toString(),
                          cmakeEntry["cmakeVar"].toString(),
                          cmakeEntry["description"].toString(),
                          cmakeEntry["setting"].toString(),
                          FilePath::fromString(cmakeEntry["defaultValue"].toString()),
                          FilePath::fromString(cmakeEntry["validation"].toString()),
                          {},
                          false});
    }
    return result;
}

McuTargetDescription parseDescriptionJson(const QByteArray &data)
{
    const QJsonDocument document = QJsonDocument::fromJson(data);
    const QJsonObject target = document.object();
    const QString qulVersion = target.value("qulVersion").toString();
    const QJsonObject platform = target.value("platform").toObject();
    const QString compatVersion = target.value("compatVersion").toString();
    const QJsonObject toolchain = target.value("toolchain").toObject();
    const QJsonObject boardSdk = target.value("boardSdk").toObject();
    const QJsonObject freeRTOS = target.value("freeRTOS").toObject();

    QJsonArray cmakeEntries = freeRTOS.value(CMAKE_ENTRIES).toArray();
    cmakeEntries.append(toolchain.value(CMAKE_ENTRIES).toArray());
    cmakeEntries.append(boardSdk.value(CMAKE_ENTRIES).toArray());
    const QList<PackageDescription> freeRtosEntries = parsePackages(cmakeEntries);

    const QVariantList toolchainVersions = toolchain.value("versions").toArray().toVariantList();
    const auto toolchainVersionsList = Utils::transform<QStringList>(toolchainVersions,
                                                                     [&](const QVariant &version) {
                                                                         return version.toString();
                                                                     });
    const QVariantList boardSdkVersions = boardSdk.value("versions").toArray().toVariantList();
    const auto boardSdkVersionsList = Utils::transform<QStringList>(boardSdkVersions,
                                                                    [&](const QVariant &version) {
                                                                        return version.toString();
                                                                    });

    const QVariantList colorDepths = platform.value("colorDepths").toArray().toVariantList();
    const auto colorDepthsVector = Utils::transform<QVector<int>>(colorDepths,
                                                                  [&](const QVariant &colorDepth) {
                                                                      return colorDepth.toInt();
                                                                  });
    const QString platformName = platform.value("platformName").toString();

    return {qulVersion,
            compatVersion,
            {
                platform.value("id").toString(),
                platformName,
                platform.value("vendor").toString(),
                colorDepthsVector,
                platformName == "Desktop" ? McuTargetDescription::TargetType::Desktop
                                          : McuTargetDescription::TargetType::MCU,
            },
            {toolchain.value("id").toString(), toolchainVersionsList, {}},
            {
                boardSdk.value("name").toString(),
                boardSdk.value("defaultPath").toString(),
                boardSdk.value("envVar").toString(),
                boardSdkVersionsList,
                {},
            },
            {
                freeRTOS.value("envVar").toString(),
                freeRTOS.value("boardSdkSubDir").toString(),
                freeRtosEntries,
            }};
}

// https://doc.qt.io/qtcreator/creator-developing-mcu.html#supported-qt-for-mcus-sdks
static const QString legacySupportVersionFor(const QString &sdkVersion)
{
    static const QHash<QString, QString> oldSdkQtcRequiredVersion
        = {{{"1.0"}, {"4.11.x"}}, {{"1.1"}, {"4.12.0 or 4.12.1"}}, {{"1.2"}, {"4.12.2 or 4.12.3"}}};
    if (oldSdkQtcRequiredVersion.contains(sdkVersion))
        return oldSdkQtcRequiredVersion.value(sdkVersion);

    if (QVersionNumber::fromString(sdkVersion).majorVersion() == 1)
        return "4.12.4 up to 6.0";

    return QString();
}

bool checkDeprecatedSdkError(const Utils::FilePath &qulDir, QString &message)
{
    const McuPackagePathVersionDetector versionDetector("(?<=\\bQtMCUs.)(\\d+\\.\\d+)");
    const QString sdkDetectedVersion = versionDetector.parseVersion(qulDir.toString());
    const QString legacyVersion = legacySupportVersionFor(sdkDetectedVersion);

    if (!legacyVersion.isEmpty()) {
        message = McuTarget::tr("Qt for MCUs SDK version %1 detected, "
                                "only supported by Qt Creator version %2. "
                                "This version of Qt Creator requires Qt for MCUs %3 or greater.")
                      .arg(sdkDetectedVersion,
                           legacyVersion,
                           McuSupportOptions::minimalQulVersion().toString());
        return true;
    }

    return false;
}

McuSdkRepository targetsAndPackages(const Utils::FilePath &dir)
{
    QList<McuTargetDescription> descriptions;

    bool isLegacy = false;

    auto descriptionFiles = targetDescriptionFiles(dir);
    for (const QFileInfo &fileInfo : descriptionFiles) {
        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QFile::ReadOnly))
            continue;
        const McuTargetDescription desc = parseDescriptionJson(file.readAll());
        const auto pth = Utils::FilePath::fromString(fileInfo.filePath());
        bool ok = false;
        const int compatVersion = desc.compatVersion.toInt(&ok);
        if (!desc.compatVersion.isEmpty() && ok && compatVersion > MAX_COMPATIBILITY_VERSION) {
            printMessage(McuTarget::tr("Skipped %1. Unsupported version \"%2\".")
                             .arg(QDir::toNativeSeparators(pth.fileNameWithPathComponents(1)),
                                  desc.qulVersion),
                         false);
            continue;
        }

        const auto qulVersion{QVersionNumber::fromString(desc.qulVersion)};
        if (qulVersion == McuSupportOptions::minimalQulVersion())
            isLegacy = true;

        if (qulVersion < McuSupportOptions::minimalQulVersion()) {
            const QString legacyVersion = legacySupportVersionFor(desc.qulVersion);
            const QString qtcSupportText
                = !legacyVersion.isEmpty()
                      ? McuTarget::tr("Detected version \"%1\", only supported by Qt Creator %2.")
                            .arg(desc.qulVersion, legacyVersion)
                      : McuTarget::tr("Unsupported version \"%1\".").arg(desc.qulVersion);
            printMessage(McuTarget::tr("Skipped %1. %2 Qt for MCUs version >= %3 required.")
                             .arg(QDir::toNativeSeparators(pth.fileNameWithPathComponents(1)),
                                  qtcSupportText,
                                  McuSupportOptions::minimalQulVersion().toString()),
                         false);
            continue;
        }
        descriptions.append(desc);
    }

    // No valid description means invalid or old SDK installation.
    if (descriptions.empty()) {
        if (kitsPath(dir).exists()) {
            printMessage(McuTarget::tr("No valid kit descriptions found at %1.")
                             .arg(kitsPath(dir).toUserOutput()),
                         true);
            return McuSdkRepository{};
        } else {
            QString deprecationMessage;
            if (checkDeprecatedSdkError(dir, deprecationMessage)) {
                printMessage(deprecationMessage, true);
                return McuSdkRepository{};
            }
        }
    }
    McuSdkRepository repo = targetsFromDescriptions(descriptions, isLegacy);

    // Keep targets sorted lexicographically
    Utils::sort(repo.mcuTargets, [](const McuTargetPtr &lhs, const McuTargetPtr &rhs) {
        return McuKitManager::generateKitNameFromTarget(lhs.get())
               < McuKitManager::generateKitNameFromTarget(rhs.get());
    });
    return repo;
}

FilePath packagePathFromSettings(const QString &settingsKey,
                                 QSettings::Scope scope,
                                 const FilePath &defaultPath)
{
    QSettings *settings = Core::ICore::settings(scope);
    const QString key = QLatin1String(Constants::SETTINGS_GROUP) + '/'
                        + QLatin1String(Constants::SETTINGS_KEY_PACKAGE_PREFIX) + settingsKey;
    const QString path = settings->value(key, defaultPath.toString()).toString();
    return FilePath::fromUserInput(path);
}

} // namespace Sdk
} // namespace Internal
} // namespace McuSupport
