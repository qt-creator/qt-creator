// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcusupportsdk.h"
#include "mcuhelpers.h"
#include "mcukitmanager.h"
#include "mculegacyconstants.h"
#include "mcupackage.h"
#include "mcusupportconstants.h"
#include "mcusupportoptions.h"
#include "mcusupportplugin.h"
#include "mcusupporttr.h"
#include "mcusupportversiondetection.h"
#include "mcutarget.h"
#include "mcutargetdescription.h"
#include "mcutargetfactory.h"
#include "mcutargetfactorylegacy.h"

#include <baremetal/baremetalconstants.h>
#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>
#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QDirIterator>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>

#include <memory>

using namespace ProjectExplorer;
using namespace Utils;

namespace McuSupport::Internal {

namespace {
const char CMAKE_ENTRIES[]{"cmakeEntries"};
} // namespace

McuPackagePtr createQtForMCUsPackage(const SettingsHandler::Ptr &settingsHandler)
{
    return McuPackagePtr{
        new McuPackage(settingsHandler,
                       {},
                       FileUtils::homePath(), // defaultPath
                       FilePath(Legacy::Constants::QT_FOR_MCUS_SDK_PACKAGE_VALIDATION_PATH)
                           .withExecutableSuffix(),                     // detectionPath
                       Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK, // settingsKey
                       Legacy::Constants::QUL_CMAKE_VAR,
                       Legacy::Constants::QUL_ENV_VAR)};
}

namespace Legacy {

static FilePath findInProgramFiles(const QString &folder)
{
    for (const auto &envVar : QStringList{"ProgramFiles", "ProgramFiles(x86)", "ProgramW6432"}) {
        if (!qtcEnvironmentVariableIsSet(envVar))
            continue;
        FilePath dir = FilePath::fromUserInput(qtcEnvironmentVariable(envVar)) / folder;
        if (dir.exists())
            return dir;
    }
    return {};
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
        return new McuPackagePathVersionDetector(R"(\d+\.\d+\.\w+)");

    return nullptr;
}

McuPackagePtr createBoardSdkPackage(const SettingsHandler::Ptr &settingsHandler,
                                    const McuTargetDescription &desc)
{
    const auto generateSdkName = [](const QString &envVar) {
        qsizetype postfixPos = envVar.indexOf("_SDK_PATH");
        if (postfixPos < 0) {
            postfixPos = envVar.indexOf("_DIR");
        }
        const QString sdkName = postfixPos > 0 ? envVar.left(postfixPos) : envVar;
        return QString{"MCU SDK (%1)"}.arg(sdkName);
    };
    const QString sdkName = generateSdkName(desc.boardSdk.envVar);

    const FilePath defaultPath = [&] {
        const auto envVar = desc.boardSdk.envVar;
        if (qtcEnvironmentVariableIsSet(envVar))
            return FilePath::fromUserInput(qtcEnvironmentVariable(envVar));
        if (!desc.boardSdk.defaultPath.isEmpty()) {
            FilePath defaultPath = FilePath::fromUserInput(QDir::rootPath()
                                                           + desc.boardSdk.defaultPath.toString());
            if (defaultPath.exists())
                return defaultPath;
        }
        return FilePath();
    }();

    const auto *versionDetector = generatePackageVersionDetector(desc.boardSdk.envVar);

    return McuPackagePtr{new McuPackage(settingsHandler,
                                        sdkName,
                                        defaultPath,
                                        {},                             // detection path
                                        keyFromString(desc.boardSdk.envVar), // settings key
                                        Constants::BOARD_SDK_CMAKE_VAR, // cmake var
                                        desc.boardSdk.envVar,           // env var
                                        desc.boardSdk.versions,
                                        {}, // download URL
                                        versionDetector)};
}

McuPackagePtr createFreeRTOSSourcesPackage(const SettingsHandler::Ptr &settingsHandler,
                                           const QString &envVar,
                                           const FilePath &boardSdkDir)
{
    const QString envVarPrefix = removeRtosSuffix(envVar);

    FilePath defaultPath;
    if (qtcEnvironmentVariableIsSet(envVar))
        defaultPath = FilePath::fromUserInput(qtcEnvironmentVariable(envVar));
    else if (!boardSdkDir.isEmpty())
        defaultPath = boardSdkDir;

    return McuPackagePtr{
        new McuPackage(settingsHandler,
                       QString::fromLatin1("FreeRTOS Sources (%1)").arg(envVarPrefix),
                       defaultPath,
                       "tasks.c", // detection path
                       Constants::SETTINGS_KEY_FREERTOS_PREFIX + keyFromString(envVarPrefix),
                       "FREERTOS_DIR",           // cmake var
                       envVar,                   // env var
                       {},                       // versions
                       "https://freertos.org")}; // download url
}

McuPackagePtr createUnsupportedToolChainFilePackage(const SettingsHandler::Ptr &settingsHandler,
                                                    const FilePath &qtForMCUSdkPath)
{
    const FilePath toolchainFilePath = qtForMCUSdkPath / Constants::QUL_TOOLCHAIN_CMAKE_DIR
                                       / "unsupported.cmake";
    return McuPackagePtr{new McuPackage(settingsHandler,
                                        {},
                                        toolchainFilePath,
                                        {},
                                        {},
                                        Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE,
                                        {})};
}

McuToolChainPackagePtr createUnsupportedToolChainPackage(const SettingsHandler::Ptr &settingsHandler)
{
    return McuToolChainPackagePtr{
        new McuToolChainPackage(settingsHandler,
                                {},
                                {},
                                {},
                                {},
                                McuToolChainPackage::ToolChainType::Unsupported,
                                {},
                                {},
                                {},
                                nullptr)};
}

McuToolChainPackagePtr createMsvcToolChainPackage(const SettingsHandler::Ptr &settingsHandler,
                                                  const QStringList &versions)
{
    ToolChain *toolChain = McuToolChainPackage::msvcToolChain(
        ProjectExplorer::Constants::CXX_LANGUAGE_ID);

    const FilePath detectionPath = FilePath("cl").withExecutableSuffix();
    const FilePath defaultPath = toolChain ? toolChain->compilerCommand().parentDir() : FilePath();

    const auto *versionDetector = new McuPackageExecutableVersionDetector(detectionPath,
                                                                          {"/?"},
                                                                          R"(\b(\d+\.\d+)\.\d+\b)");

    return McuToolChainPackagePtr{new McuToolChainPackage(settingsHandler,
                                                          Tr::tr("MSVC Binary directory"),
                                                          defaultPath,
                                                          detectionPath,
                                                          "MsvcToolchain",
                                                          McuToolChainPackage::ToolChainType::MSVC,
                                                          versions,
                                                          {},
                                                          {},
                                                          versionDetector)};
}

McuToolChainPackagePtr createGccToolChainPackage(const SettingsHandler::Ptr &settingsHandler,
                                                 const QStringList &versions)
{
    ToolChain *toolChain = McuToolChainPackage::gccToolChain(
        ProjectExplorer::Constants::CXX_LANGUAGE_ID);

    const FilePath detectionPath = FilePath("bin/g++").withExecutableSuffix();
    const FilePath defaultPath = toolChain ? toolChain->compilerCommand().parentDir().parentDir()
                                           : FilePath();

    const auto *versionDetector = new McuPackageExecutableVersionDetector(detectionPath,
                                                                          {"--version"},
                                                                          R"(\b(\d+\.\d+\.\d+)\b)");

    return McuToolChainPackagePtr{new McuToolChainPackage(settingsHandler,
                                                          Tr::tr("GCC Toolchain"),
                                                          defaultPath,
                                                          detectionPath,
                                                          "GnuToolchain",
                                                          McuToolChainPackage::ToolChainType::GCC,
                                                          versions,
                                                          {},
                                                          {},
                                                          versionDetector)};
}

McuToolChainPackagePtr createArmGccToolchainPackage(const SettingsHandler::Ptr &settingsHandler,
                                                    const QStringList &versions)
{
    const char envVar[] = "ARMGCC_DIR";

    FilePath defaultPath;
    if (qtcEnvironmentVariableIsSet(envVar))
        defaultPath = FilePath::fromUserInput(qtcEnvironmentVariable(envVar));
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

    const FilePath detectionPath = FilePath("bin/arm-none-eabi-g++").withExecutableSuffix();
    const auto *versionDetector = new McuPackageExecutableVersionDetector(detectionPath,
                                                                          {"--version"},
                                                                          R"(\b(\d+\.\d+\.\d+)\b)");

    return McuToolChainPackagePtr{
        new McuToolChainPackage(settingsHandler,
                                Tr::tr("GNU Arm Embedded Toolchain"),
                                defaultPath,
                                detectionPath,
                                "GNUArmEmbeddedToolchain",                  // settingsKey
                                McuToolChainPackage::ToolChainType::ArmGcc, // toolchainType
                                versions,
                                Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE, // cmake var
                                envVar,                                  // env var
                                versionDetector)};
}

McuToolChainPackagePtr createGhsToolchainPackage(const SettingsHandler::Ptr &settingsHandler,
                                                 const QStringList &versions)
{
    const char envVar[] = "GHS_COMPILER_DIR";

    const FilePath defaultPath = FilePath::fromUserInput(qtcEnvironmentVariable(envVar));

    const auto *versionDetector
        = new McuPackageExecutableVersionDetector(FilePath("gversion").withExecutableSuffix(),
                                                  {"-help"},
                                                  R"(\bv(\d+\.\d+\.\d+)\b)");

    return McuToolChainPackagePtr{
        new McuToolChainPackage(settingsHandler,
                                "Green Hills Compiler",
                                defaultPath,
                                FilePath("ccv850").withExecutableSuffix(), // detectionPath
                                "GHSToolchain",                            // settingsKey
                                McuToolChainPackage::ToolChainType::GHS,   // toolchainType
                                versions,
                                Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE, // cmake var
                                envVar,                                  // env var
                                versionDetector)};
}

McuToolChainPackagePtr createGhsArmToolchainPackage(const SettingsHandler::Ptr &settingsHandler,
                                                    const QStringList &versions)
{
    const char envVar[] = "GHS_ARM_COMPILER_DIR";

    const FilePath defaultPath = FilePath::fromUserInput(qtcEnvironmentVariable(envVar));

    const auto *versionDetector
        = new McuPackageExecutableVersionDetector(FilePath("gversion").withExecutableSuffix(),
                                                  {"-help"},
                                                  R"(\bv(\d+\.\d+\.\d+)\b)");

    return McuToolChainPackagePtr{
        new McuToolChainPackage(settingsHandler,
                                "Green Hills Compiler for ARM",
                                defaultPath,
                                FilePath("cxarm").withExecutableSuffix(),   // detectionPath
                                "GHSArmToolchain",                          // settingsKey
                                McuToolChainPackage::ToolChainType::GHSArm, // toolchainType
                                versions,
                                Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE, // cmake var
                                envVar,                                  // env var
                                versionDetector)};
}

McuToolChainPackagePtr createIarToolChainPackage(const SettingsHandler::Ptr &settingsHandler,
                                                 const QStringList &versions)
{
    const char envVar[] = "IAR_ARM_COMPILER_DIR";

    FilePath defaultPath;
    if (qtcEnvironmentVariableIsSet(envVar))
        defaultPath = FilePath::fromUserInput(qtcEnvironmentVariable(envVar));
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
    const auto *versionDetector
        = new McuPackageExecutableVersionDetector(detectionPath,
                                                  {"--version"},
                                                  R"(\bV(\d+\.\d+\.\d+)\.\d+\b)");

    return McuToolChainPackagePtr{
        new McuToolChainPackage(settingsHandler,
                                "IAR ARM Compiler",
                                defaultPath,
                                detectionPath,
                                "IARToolchain",                          // settings key
                                McuToolChainPackage::ToolChainType::IAR, // toolchainType
                                versions,
                                Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE, // cmake var
                                envVar,                                  // env var
                                versionDetector)};
}

McuPackagePtr createStm32CubeProgrammerPackage(const SettingsHandler::Ptr &settingsHandler)
{
    FilePath defaultPath;
    const QString cubePath = "STMicroelectronics/STM32Cube/STM32CubeProgrammer";
    if (HostOsInfo::isWindowsHost())
        defaultPath = findInProgramFiles(cubePath);
    else
        defaultPath = FileUtils::homePath() / cubePath;
    if (!defaultPath.exists())
        FilePath defaultPath = {};

    const FilePath detectionPath = FilePath::fromUserInput(
        QLatin1String(Utils::HostOsInfo::isWindowsHost() ? "bin/STM32_Programmer_CLI.exe"
                                                         : "bin/STM32_Programmer.sh"));

    return McuPackagePtr{
        new McuPackage(settingsHandler,
                       Tr::tr("STM32CubeProgrammer"),
                       defaultPath,
                       detectionPath,
                       "Stm32CubeProgrammer",
                       {},                                                           // cmake var
                       {},                                                           // env var
                       {},                                                           // versions
                       "https://www.st.com/en/development-tools/stm32cubeprog.html", // download url
                       nullptr, // version detector
                       true     // add to path
                       )};
}

McuPackagePtr createMcuXpressoIdePackage(const SettingsHandler::Ptr &settingsHandler)
{
    const char envVar[] = "MCUXpressoIDE_PATH";

    FilePath defaultPath;
    if (qtcEnvironmentVariableIsSet(envVar)) {
        defaultPath = FilePath::fromUserInput(qtcEnvironmentVariable(envVar));
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
        const FilePath programPath = FilePath::fromUserInput("/usr/local/mcuxpressoide/");
        if (programPath.exists())
            defaultPath = programPath;
    }

    return McuPackagePtr{new McuPackage(settingsHandler,
                                        "MCUXpresso IDE",
                                        defaultPath,
                                        FilePath("ide/binaries/crt_emu_cm_redlink")
                                            .withExecutableSuffix(), // detection path
                                        "MCUXpressoIDE",             // settings key
                                        "MCUXPRESSO_IDE_PATH",       // cmake var
                                        envVar,
                                        {},                                     // versions
                                        "https://www.nxp.com/mcuxpresso/ide")}; // download url
}

McuPackagePtr createCypressProgrammerPackage(const SettingsHandler::Ptr &settingsHandler)
{
    const char envVar[] = "CYPRESS_AUTO_FLASH_UTILITY_DIR";

    FilePath defaultPath;
    if (qtcEnvironmentVariableIsSet(envVar)) {
        defaultPath = FilePath::fromUserInput(qtcEnvironmentVariable(envVar));
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

    return McuPackagePtr{
        new McuPackage(settingsHandler,
                       "Cypress Auto Flash Utility",
                       defaultPath,
                       FilePath::fromUserInput("/bin/openocd").withExecutableSuffix(),
                       "CypressAutoFlashUtil",            // settings key
                       "INFINEON_AUTO_FLASH_UTILITY_DIR", // cmake var
                       envVar)};                          // env var
}

McuPackagePtr createRenesasProgrammerPackage(const SettingsHandler::Ptr &settingsHandler)
{
    const char envVar[] = "RENESAS_FLASH_PROGRAMMER_PATH";

    FilePath defaultPath;
    if (qtcEnvironmentVariableIsSet(envVar)) {
        defaultPath = FilePath::fromUserInput(qtcEnvironmentVariable(envVar));
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

    return McuPackagePtr{new McuPackage(settingsHandler,
                                        "Renesas Flash Programmer",
                                        defaultPath,
                                        FilePath("rfp-cli").withExecutableSuffix(),
                                        "RenesasFlashProgrammer",        // settings key
                                        "RENESAS_FLASH_PROGRAMMER_PATH", // cmake var
                                        envVar)};                        // env var
}

} // namespace Legacy

static McuAbstractTargetFactory::Ptr createFactory(bool isLegacy,
                                                   const SettingsHandler::Ptr &settingsHandler,
                                                   const FilePath &qtMcuSdkPath)
{
    McuAbstractTargetFactory::Ptr result;
    if (isLegacy) {
        static const QHash<QString, Legacy::ToolchainCompilerCreator> toolchainCreators = {
            {{"armgcc"}, {[settingsHandler](const QStringList &versions) {
                 return Legacy::createArmGccToolchainPackage(settingsHandler, versions);
             }}},
            {{"greenhills"},
             [settingsHandler](const QStringList &versions) {
                 return Legacy::createGhsToolchainPackage(settingsHandler, versions);
             }},
            {{"iar"}, {[settingsHandler](const QStringList &versions) {
                 return Legacy::createIarToolChainPackage(settingsHandler, versions);
             }}},
            {{"msvc"}, {[settingsHandler](const QStringList &versions) {
                 return Legacy::createMsvcToolChainPackage(settingsHandler, versions);
             }}},
            {{"gcc"}, {[settingsHandler](const QStringList &versions) {
                 return Legacy::createGccToolChainPackage(settingsHandler, versions);
             }}},
            {{"arm-greenhills"}, {[settingsHandler](const QStringList &versions) {
                 return Legacy::createGhsArmToolchainPackage(settingsHandler, versions);
             }}},
        };

        const FilePath toolchainFilePrefix = qtMcuSdkPath
                                             / Legacy::Constants::QUL_TOOLCHAIN_CMAKE_DIR;
        static const QHash<QString, McuPackagePtr> toolchainFiles
            = {{{"armgcc"},
                McuPackagePtr{new McuPackage{settingsHandler,
                                             {},
                                             toolchainFilePrefix / "armgcc.cmake",
                                             {},
                                             {},
                                             Legacy::Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE,
                                             {}}}},

               {{"iar"},
                McuPackagePtr{new McuPackage{settingsHandler,
                                             {},
                                             toolchainFilePrefix / "iar.cmake",
                                             {},
                                             {},
                                             Legacy::Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE,
                                             {}}}},
               {"greenhills",
                McuPackagePtr{new McuPackage{settingsHandler,
                                             {},
                                             toolchainFilePrefix / "ghs.cmake",
                                             {},
                                             {},
                                             Legacy::Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE,
                                             {}}}},
               {"arm-greenhills",
                McuPackagePtr{new McuPackage{settingsHandler,
                                             {},
                                             toolchainFilePrefix / "ghs-arm.cmake",
                                             {},
                                             {},
                                             Legacy::Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE,
                                             {}}}}

            };

        // Note: the vendor name (the key of the hash) is case-sensitive. It has to match the "platformVendor" key in the
        // json file.
        static const QHash<QString, McuPackagePtr> vendorPkgs = {
            {{"ST"}, McuPackagePtr{Legacy::createStm32CubeProgrammerPackage(settingsHandler)}},
            {{"NXP"}, McuPackagePtr{Legacy::createMcuXpressoIdePackage(settingsHandler)}},
            {{"CYPRESS"}, McuPackagePtr{Legacy::createCypressProgrammerPackage(settingsHandler)}},
            {{"RENESAS"}, McuPackagePtr{Legacy::createRenesasProgrammerPackage(settingsHandler)}},
        };

        result = std::make_unique<Legacy::McuTargetFactory>(toolchainCreators,
                                                            toolchainFiles,
                                                            vendorPkgs,
                                                            settingsHandler);
    } else {
        result = std::make_unique<McuTargetFactory>(settingsHandler);
    }
    return result;
}

McuSdkRepository targetsFromDescriptions(const QList<McuTargetDescription> &descriptions,
                                         const SettingsHandler::Ptr &settingsHandler,
                                         const McuPackagePtr &qtForMCUsPackage,
                                         bool isLegacy)
{
    Targets mcuTargets;
    Packages mcuPackages;
    const FilePath &qtForMCUSdkPath{qtForMCUsPackage->path()};
    McuAbstractTargetFactory::Ptr targetFactory = createFactory(isLegacy,
                                                                settingsHandler,
                                                                qtForMCUSdkPath);
    for (const McuTargetDescription &desc : descriptions) {
        auto [targets, packages] = targetFactory->createTargets(desc, qtForMCUsPackage);
        mcuTargets.append(targets);
        mcuPackages.unite(packages);
        mcuPackages.insert(qtForMCUsPackage);
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

    McuSdkRepository repo{mcuTargets, mcuPackages};
    repo.expandVariablesAndWildcards();

    return repo;
}

FilePath kitsPath(const FilePath &qtMcuSdkPath)
{
    return qtMcuSdkPath / "kits/";
}

static FilePaths targetDescriptionFiles(const FilePath &dir)
{
    return kitsPath(dir).dirEntries(Utils::FileFilter({"*.json"}, QDir::Files));
}

static QString getOsSpecificValue(const QJsonValue &entry)
{
    if (entry.isObject()) {
        //The json entry has os-specific values
        return entry[HostOsInfo::isWindowsHost() ? QString("windows") : QString("linux")].toString();
    }
    //The entry does not have os-specific values
    return entry.toString();
}

static VersionDetection parseVersionDetection(const QJsonObject &packageEntry)
{
    const QJsonObject versioning = packageEntry.value("versionDetection").toObject();
    return {
        versioning["regex"].toString(),
        getOsSpecificValue(versioning["filePattern"]),
        versioning["executableArgs"].toString(),
        versioning["xmlElement"].toString(),
        versioning["xmlAttribute"].toString(),
    };
}

static Utils::PathChooser::Kind parseLineEditType(const QJsonValue &type)
{
    //Utility function to handle the different kinds of PathChooser
    //Default is ExistingDirectory, see pathchooser.h for more options
    const auto defaultValue = Utils::PathChooser::Kind::ExistingDirectory;
    if (type.isUndefined()) {
        //No "type" entry in the json file, this is not an error
        return defaultValue;
    }

    const QString typeString = type.toString();
    if (typeString.isNull()) {
        printMessage(::McuSupport::Tr::tr(
                         "Parsing error: the type entry in JSON kit files must be a string, "
                         "defaulting to \"path\"")
                         .arg(typeString),
                     true);

        return defaultValue;

    } else if (typeString.compare("file", Qt::CaseInsensitive) == 0) {
        return Utils::PathChooser::File;
    } else if (typeString.compare("path", Qt::CaseInsensitive) == 0) {
        return Utils::PathChooser::ExistingDirectory;
    } else {
        printMessage(::McuSupport::Tr::tr(
                         "Parsing error: the type entry \"%2\" in JSON kit files is not supported, "
                         "defaulting to \"path\"")
                         .arg(typeString),
                     true);

        return defaultValue;
    }
}

static PackageDescription parsePackage(const QJsonObject &cmakeEntry)
{
    const QVariantList versionsVariantList = cmakeEntry["versions"].toArray().toVariantList();
    const auto versions = Utils::transform<QStringList>(versionsVariantList,
                                                        [&](const QVariant &version) {
                                                            return version.toString();
                                                        });

    //Parse the default value depending on the operating system
    QString defaultPathString = getOsSpecificValue(cmakeEntry["defaultValue"]);
    QString detectionPathString = getOsSpecificValue(cmakeEntry["detectionPath"]);

    QString label = cmakeEntry["label"].toString();

    //Apply translations
    label = McuPackage::packageLabelTranslations.value(label, label);

    return {label,
            cmakeEntry["envVar"].toString(),
            cmakeEntry["cmakeVar"].toString(),
            cmakeEntry["description"].toString(),
            keyFromString(cmakeEntry["setting"].toString()),
            FilePath::fromUserInput(defaultPathString),
            FilePath::fromUserInput(detectionPathString),
            versions,
            parseVersionDetection(cmakeEntry),
            cmakeEntry["addToSystemPath"].toBool(),
            parseLineEditType(cmakeEntry["type"])};
}

static QList<PackageDescription> parsePackages(const QJsonArray &cmakeEntries)
{
    QList<PackageDescription> result;
    for (const auto &cmakeEntryRef : cmakeEntries) {
        const QJsonObject cmakeEntry{cmakeEntryRef.toObject()};
        result.push_back(parsePackage(cmakeEntry));
    }
    return result;
}

McuTargetDescription parseDescriptionJson(const QByteArray &data, const Utils::FilePath &sourceFile)
{
    const QJsonDocument document = QJsonDocument::fromJson(data);
    const QJsonObject target = document.object();
    const QString qulVersion = target.value("qulVersion").toString();
    const QJsonObject platform = target.value("platform").toObject();
    const QString compatVersion = target.value("compatVersion").toString();
    const QJsonObject toolchain = target.value("toolchain").toObject();
    const QJsonObject toolchainFile = toolchain.value("file").toObject();
    const QJsonObject compiler = toolchain.value("compiler").toObject();
    const QJsonObject boardSdk = target.value("boardSdk").toObject();
    const QJsonObject freeRTOS = target.value("freeRTOS").toObject();

    const QJsonArray platformEntries = platform.value(CMAKE_ENTRIES).toArray();
    const QList<PackageDescription> platformPackages{parsePackages(platformEntries)};

    const PackageDescription toolchainPackage = parsePackage(compiler);
    const PackageDescription toolchainFilePackage = parsePackage(toolchainFile);
    const PackageDescription boardSdkPackage{parsePackage(boardSdk)};
    const PackageDescription freeRtosPackage{parsePackage(freeRTOS)};

    const QVariantList toolchainVersions = toolchain.value("versions").toArray().toVariantList();
    const auto toolchainVersionsList = Utils::transform<QStringList>(toolchainVersions,
                                                                     [&](const QVariant &version) {
                                                                         return version.toString();
                                                                     });

    const QVariantList colorDepths = platform.value("colorDepths").toArray().toVariantList();
    const auto colorDepthsVector = Utils::transform<QVector<int>>(colorDepths,
                                                                  [&](const QVariant &colorDepth) {
                                                                      return colorDepth.toInt();
                                                                  });
    const QString platformName = platform.value("platformName").toString();

    return {sourceFile,
            qulVersion,
            compatVersion,
            {platform.value("id").toString(),
             platformName,
             platform.value("vendor").toString(),
             colorDepthsVector,
             platformName == "Desktop" ? McuTargetDescription::TargetType::Desktop
                                       : McuTargetDescription::TargetType::MCU,
             platformPackages},
            {toolchain.value("id").toString(),
             toolchainVersionsList,
             toolchainPackage,
             toolchainFilePackage},
            boardSdkPackage,
            {freeRTOS.value("envVar").toString(), freeRtosPackage}};
}

// https://doc.qt.io/qtcreator/creator-developing-mcu.html#supported-qt-for-mcus-sdks
static QString legacySupportVersionFor(const QString &sdkVersion)
{
    static const QHash<QString, QString> oldSdkQtcRequiredVersion
        = {{{"1.0"}, {"4.11.x"}}, {{"1.1"}, {"4.12.0 or 4.12.1"}}, {{"1.2"}, {"4.12.2 or 4.12.3"}}};
    if (oldSdkQtcRequiredVersion.contains(sdkVersion))
        return oldSdkQtcRequiredVersion.value(sdkVersion);

    if (QVersionNumber::fromString(sdkVersion).majorVersion() == 1)
        return "4.12.4 up to 6.0";

    return QString();
}

bool checkDeprecatedSdkError(const FilePath &qulDir, QString &message)
{
    const McuPackagePathVersionDetector versionDetector(R"((?<=\bQtMCUs.)(\d+\.\d+))");
    const QString sdkDetectedVersion = versionDetector.parseVersion(qulDir);
    const QString legacyVersion = legacySupportVersionFor(sdkDetectedVersion);

    if (!legacyVersion.isEmpty()) {
        message = Tr::tr("Qt for MCUs SDK version %1 detected, "
                         "only supported by Qt Creator version %2. "
                         "This version of Qt Creator requires Qt for MCUs %3 or greater.")
                      .arg(sdkDetectedVersion,
                           legacyVersion,
                           McuSupportOptions::minimalQulVersion().toString());
        return true;
    }

    return false;
}

McuSdkRepository targetsAndPackages(const McuPackagePtr &qtForMCUsPackage,
                                    const SettingsHandler::Ptr &settingsHandler)
{
    const FilePath &qtForMCUSdkPath{qtForMCUsPackage->path()};
    QList<McuTargetDescription> descriptions;
    bool isLegacy{false};

    const FilePaths descriptionFiles = targetDescriptionFiles(qtForMCUSdkPath);
    for (const FilePath &filePath : descriptionFiles) {
        if (!filePath.isReadableFile())
            continue;
        const McuTargetDescription desc = parseDescriptionJson(
            filePath.fileContents().value_or(QByteArray()),
            filePath);
        bool ok = false;
        const int compatVersion = desc.compatVersion.toInt(&ok);
        if (!desc.compatVersion.isEmpty() && ok && compatVersion > MAX_COMPATIBILITY_VERSION) {
            printMessage(Tr::tr("Skipped %1. Unsupported version \"%2\".")
                             .arg(QDir::toNativeSeparators(filePath.fileNameWithPathComponents(1)),
                                  desc.qulVersion),
                         false);
            continue;
        }

        const auto qulVersion{QVersionNumber::fromString(desc.qulVersion)};
        isLegacy = McuSupportOptions::isLegacyVersion(qulVersion);

        if (qulVersion < McuSupportOptions::minimalQulVersion()) {
            const QString legacyVersion = legacySupportVersionFor(desc.qulVersion);
            const QString qtcSupportText
                = !legacyVersion.isEmpty()
                      ? Tr::tr("Detected version \"%1\", only supported by Qt Creator %2.")
                            .arg(desc.qulVersion, legacyVersion)
                      : Tr::tr("Unsupported version \"%1\".").arg(desc.qulVersion);
            printMessage(Tr::tr("Skipped %1. %2 Qt for MCUs version >= %3 required.")
                             .arg(QDir::toNativeSeparators(filePath.fileNameWithPathComponents(1)),
                                  qtcSupportText,
                                  McuSupportOptions::minimalQulVersion().toString()),
                         false);
            continue;
        }
        descriptions.append(desc);
    }

    // No valid description means invalid or old SDK installation.
    if (descriptions.empty()) {
        if (kitsPath(qtForMCUSdkPath).exists()) {
            printMessage(Tr::tr("No valid kit descriptions found at %1.")
                             .arg(kitsPath(qtForMCUSdkPath).toUserOutput()),
                         true);
            return McuSdkRepository{};
        } else {
            QString deprecationMessage;
            if (checkDeprecatedSdkError(qtForMCUSdkPath, deprecationMessage)) {
                printMessage(deprecationMessage, true);
                return McuSdkRepository{};
            }
        }
    }
    McuSdkRepository repo = targetsFromDescriptions(descriptions,
                                                    settingsHandler,
                                                    qtForMCUsPackage,
                                                    isLegacy);

    // Keep targets sorted lexicographically
    Utils::sort(repo.mcuTargets, [](const McuTargetPtr &lhs, const McuTargetPtr &rhs) {
        return McuKitManager::generateKitNameFromTarget(lhs.get())
               < McuKitManager::generateKitNameFromTarget(rhs.get());
    });

    return repo;
}

} // namespace McuSupport::Internal
