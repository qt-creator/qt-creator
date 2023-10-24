// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "unittest.h"

#include "armgcc_ek_ra6m3g_baremetal_json.h"
#include "armgcc_mimxrt1050_evk_freertos_json.h"
#include "armgcc_mimxrt1064_evk_freertos_json.h"
#include "armgcc_mimxrt1170_evk_freertos_json.h"
#include "armgcc_stm32f769i_discovery_freertos_json.h"
#include "armgcc_stm32h750b_discovery_baremetal_json.h"
#include "errors_json.h"
#include "gcc_desktop_json.h"
#include "ghs_rh850_d1m1a_baremetal_json.h"
#include "ghs_tviic2d6m_baremetal_json.h"
#include "iar_mimxrt1064_evk_freertos_json.h"
#include "iar_stm32f469i_discovery_baremetal_json.h"
#include "mingw_desktop_json.h"
#include "msvc_desktop_json.h"
#include "wildcards_test_kit_json.h"

#include "mcuhelpers.h"
#include "mcukitmanager.h"
#include "mculegacyconstants.h"
#include "mcupackage.h"
#include "mcusupportconstants.h"
#include "mcusupportoptions.h"
#include "mcusupportsdk.h"
#include "mcusupportversiondetection.h"
#include "mcutargetdescription.h"
#include "mcutargetfactory.h"
#include "mcutargetfactorylegacy.h"

#include <baremetal/baremetalconstants.h>
#include <cmakeprojectmanager/cmakeconfigitem.h>
#include <cmakeprojectmanager/cmakekitaspect.h>
#include <gmock/gmock-actions.h>
#include <gmock/gmock.h>

#include <projectexplorer/customtoolchain.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QtTest>

#include <algorithm>
#include <tuple>
#include <type_traits>

namespace McuSupport::Internal::Test {

using namespace Utils;

using Legacy::Constants::QUL_CMAKE_VAR;
using Legacy::Constants::QUL_ENV_VAR;
using Legacy::Constants::QUL_LABEL;
using Legacy::Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE;
using Legacy::Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE;

using CMakeProjectManager::CMakeConfigurationKitAspect;
using ProjectExplorer::Kit;
using ProjectExplorer::KitManager;
using ProjectExplorer::ToolChain;
using ProjectExplorer::ToolChainManager;

using testing::_;
using testing::Return;

namespace {
const char empty[]{""};
const char armGcc[]{"armgcc"};
const char armGccVersionDetectionRegex[]{R"(\b(\d+\.\d+\.\d+)\b)"};
const char armGccDir[]{"/opt/armgcc"};
const char armGccDirectorySetting[]{"GNUArmEmbeddedToolchain"};
const char armGccEnvVar[]{"ARMGCC_DIR"};
const char armGccLabel[]{"GNU Arm Embedded Toolchain"};
const QString armGccSuffix{HostOsInfo::withExecutableSuffix("bin/arm-none-eabi-g++")};
const char armGccToolchainFilePath[]{"/opt/qtformcu/2.2/lib/cmake/Qul/toolchain/armgcc.cmake"};
const char armGccToolchainFileUnexpandedPath[]{"%{Qul_ROOT}/lib/cmake/Qul/toolchain/armgcc.cmake"};
const char armGccVersion[]{"10.3.1"};
const char msvcVersion[]{"14.29"};
const char boardSdkVersion[]{"2.12.0"};
const QString boardSdkCmakeVar{Legacy::Constants::BOARD_SDK_CMAKE_VAR};
const char boardSdkDir[]{"/opt/Qul/2.3.0/boardDir/"};
const char fallbackDir[]{"/abc/def/fallback"};
const char freeRtosCMakeVar[]{"FREERTOS_DIR"};
const char freeRtosEnvVar[]{"EVK_MIMXRT1170_FREERTOS_PATH"};
const char freeRtosLabel[]{"FreeRTOS directory"};
const char freeRtosPath[]{"/opt/freertos/default"};
const char freeRtosDetectionPath[]{"tasks.c"};
const char freeRtosNxpPathSuffix[]{"rtos/freertos/freertos_kernel"};
const char freeRtosStmPathSuffix[]{"/Middlewares/Third_Party/FreeRTOS/Source"};
const char freeRtosSetting[]{"Freertos"};
const char greenhillToolchainFilePath[]{"/opt/qtformcu/2.2/lib/cmake/Qul/toolchain/ghs.cmake"};
const char greenhillToolchainFileUnexpandedPath[]{"%{Qul_ROOT}/lib/cmake/Qul/toolchain/ghs.cmake"};
const char greenhillCompilerDir[]{"/abs/ghs"};
const char greenhillSetting[]{"GHSToolchain"};
const QStringList greenhillVersions{{"2018.1.5"}};
const char iarDir[]{"/opt/iar/compiler"};
const char iarEnvVar[]{"IAR_ARM_COMPILER_DIR"};
const char iarLabel[]{"IAR ARM Compiler"};
const char iarSetting[]{"IARToolchain"};
const char iarToolchainFilePath[]{"/opt/qtformcu/2.2/lib/cmake/Qul/toolchain/iar.cmake"};
const char iarToolchainFileUnexpandedPath[]{"%{Qul_ROOT}/lib/cmake/Qul/toolchain/iar.cmake"};
const char iarVersionDetectionRegex[]{R"(\bV(\d+\.\d+\.\d+)\.\d+\b)"};
const QStringList iarVersions{{"9.20.4"}};
const char iar[]{"iar"};
const char id[]{"target_id"};
const char name[]{"target_name"};
const char nxp1050FreeRtosEnvVar[]{"IMXRT1050_FREERTOS_DIR"};
const char nxp1050[]{"IMXRT1050"};
const char nxp1064FreeRtosEnvVar[]{"IMXRT1064_FREERTOS_DIR"};
const char nxp1064[]{"IMXRT1064"};
const char nxp1170FreeRtosEnvVar[]{"EVK_MIMXRT1170_FREERTOS_PATH"};
const char nxp1170[]{"EVK_MIMXRT1170"};
const char qtForMcuSdkPath[]{"/opt/qtformcu/2.2"};
const char stm32f7FreeRtosEnvVar[]{"STM32F7_FREERTOS_DIR"};
const char stm32f7[]{"STM32F7"};
const char vendor[]{"target_vendor"};
const QString settingsPrefix = QLatin1String(Constants::SETTINGS_GROUP) + '/'
                               + QLatin1String(Constants::SETTINGS_KEY_PACKAGE_PREFIX);

const char defaultToolPath[]{"/opt/biz/foo"};
const char xpressoIdePath[]{"/usr/local/mcuxpressoide"};
const char xpressoIdeLabel[]{"MCUXpresso IDE"};
const char xpressoIdeSetting[]{"MCUXpressoIDE"};
const char xpressoIdeCmakeVar[]{"MCUXPRESSO_IDE_PATH"};
const char xpressoIdeEnvVar[]{"MCUXpressoIDE_PATH"};
const QString xpressoIdeDetectionPath{
    HostOsInfo::withExecutableSuffix("ide/binaries/crt_emu_cm_redlink")};

const char stmCubeProgrammerSetting[]{"Stm32CubeProgrammer"};
const char stmCubeProgrammerLabel[]{"STM32CubeProgrammer"};
const QString stmCubeProgrammerPath{defaultToolPath};
const QString stmCubeProgrammerDetectionPath{HostOsInfo::isWindowsHost()
                                                 ? QString("bin/STM32_Programmer_CLI.exe")
                                                 : QString("bin/STM32_Programmer.sh")};

const char renesasProgrammerSetting[]{"RenesasFlashProgrammer"};
const char renesasProgrammerCmakeVar[]{"RENESAS_FLASH_PROGRAMMER_PATH"};
const char renesasProgrammerEnvVar[]{"RENESAS_FLASH_PROGRAMMER_PATH"};
const char renesasProgrammerLabel[]{"Renesas Flash Programmer"};
const QString renesasProgrammerDetectionPath{HostOsInfo::withExecutableSuffix("rfp-cli")};

const QString renesasE2StudioPath{(FileUtils::homePath() / "/e2_studio/workspace").toUserOutput()};

const char cypressProgrammerSetting[]{"CypressAutoFlashUtil"};
const char cypressProgrammerCmakeVar[]{"INFINEON_AUTO_FLASH_UTILITY_DIR"};
const char cypressProgrammerEnvVar[]{"CYPRESS_AUTO_FLASH_UTILITY_DIR"};
const char cypressProgrammerLabel[]{"Cypress Auto Flash Utility"};
const QString cypressProgrammerDetectionPath{HostOsInfo::withExecutableSuffix("/bin/openocd")};

const char jlinkPath[]{"/opt/SEGGER/JLink"};
const char jlinkSetting[]{"JLinkPath"};
const char jlinkCmakeVar[]{"JLINK_PATH"};
const char jlinkEnvVar[]{"JLINK_PATH"};
const char jlinkLabel[]{"Path to SEGGER J-Link"};
const QString jlinkDetectionPath{HostOsInfo::isWindowsHost()
                                                 ? QString("JLink.exe")
                                                 : QString("JLinkExe")};

const QString unsupportedToolchainFilePath = QString{qtForMcuSdkPath}
                                             + "/lib/cmake/Qul/toolchain/unsupported.cmake";

// Build/Testing/QtMCUs
const auto testing_output_dir = (FilePath::fromString(QCoreApplication::applicationDirPath()).parentDir()
                           / "Testing/QtMCUs")
                              .canonicalPath();

const QStringList jsonFiles{QString::fromUtf8(armgcc_mimxrt1050_evk_freertos_json),
                            QString::fromUtf8(iar_mimxrt1064_evk_freertos_json)};

const bool runLegacy{true};
const int colorDepth{32};

const PackageDescription
    qtForMCUsSDKDescription{QUL_LABEL,
                            QUL_ENV_VAR,
                            QUL_CMAKE_VAR,
                            {},
                            Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK,
                            qtForMcuSdkPath,
                            Legacy::Constants::QT_FOR_MCUS_SDK_PACKAGE_VALIDATION_PATH,
                            {},
                            VersionDetection{},
                            false,
                            Utils::PathChooser::Kind::ExistingDirectory};

const McuTargetDescription::Platform platformDescription{id,
                                                         "",
                                                         "",
                                                         {colorDepth},
                                                         McuTargetDescription::TargetType::MCU,
                                                         {qtForMCUsSDKDescription}};
const Id cxxLanguageId{ProjectExplorer::Constants::CXX_LANGUAGE_ID};
} // namespace

//Expand variables in a tested {targets, packages} pair
auto expandTargetsAndPackages = [](Targets &targets, Packages &packages) {
    McuSdkRepository{targets, packages}.expandVariablesAndWildcards();
};

void verifyIarToolchain(const McuToolChainPackagePtr &iarToolchainPackage)
{
    ProjectExplorer::ToolChainFactory toolchainFactory;
    Id iarId{BareMetal::Constants::IAREW_TOOLCHAIN_TYPEID};
    ToolChain *iarToolchain{ProjectExplorer::ToolChainFactory::createToolChain(iarId)};
    iarToolchain->setLanguage(cxxLanguageId);
    ToolChainManager::registerToolChain(iarToolchain);

    QVERIFY(iarToolchainPackage != nullptr);
    QCOMPARE(iarToolchainPackage->cmakeVariableName(), TOOLCHAIN_DIR_CMAKE_VARIABLE);
    QCOMPARE(iarToolchainPackage->environmentVariableName(), iarEnvVar);
    QCOMPARE(iarToolchainPackage->isDesktopToolchain(), false);
    QCOMPARE(iarToolchainPackage->toolChainName(), iar);
    QCOMPARE(iarToolchainPackage->toolchainType(), McuToolChainPackage::ToolChainType::IAR);
    QCOMPARE(iarToolchainPackage->label(), iarLabel);

    iarToolchain = iarToolchainPackage->toolChain(cxxLanguageId);
    QVERIFY(iarToolchain != nullptr);
    QCOMPARE(iarToolchain->displayName(), "IAREW");
    QCOMPARE(iarToolchain->detection(), ToolChain::UninitializedDetection);
}

void verifyArmGccToolchain(const McuToolChainPackagePtr &armGccPackage, const QStringList &versions)
{
    //Fake register and fake detect compiler.
    ProjectExplorer::ToolChainFactory toolchainFactory;
    Id armGccId{ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID};

    ToolChain *armToolchain{ProjectExplorer::ToolChainFactory::createToolChain(armGccId)};
    armToolchain->setLanguage(cxxLanguageId);
    ToolChainManager::registerToolChain(armToolchain);

    QVERIFY(armGccPackage != nullptr);
    QCOMPARE(armGccPackage->cmakeVariableName(), TOOLCHAIN_DIR_CMAKE_VARIABLE);
    QCOMPARE(armGccPackage->environmentVariableName(), armGccEnvVar);
    QCOMPARE(armGccPackage->isDesktopToolchain(), false);
    QCOMPARE(armGccPackage->toolChainName(), armGcc);
    QCOMPARE(armGccPackage->toolchainType(), McuToolChainPackage::ToolChainType::ArmGcc);
    QCOMPARE(armGccPackage->settingsKey(), armGccDirectorySetting);
    QCOMPARE(armGccPackage->versions(), versions);

    // FIXME(piotr.mucko): Re-enable when toolchains retrieval from McuToolChainPackage is unified for arm and iar.
    // armToolchain = armGccPackage->toolChain(cxxLanguageId);
    // QVERIFY(armToolchain != nullptr);
}

void verifyGccToolchain(const McuToolChainPackagePtr &gccPackage, const QStringList &versions)
{
    QVERIFY(gccPackage != nullptr);
    QCOMPARE(gccPackage->cmakeVariableName(), "");
    QCOMPARE(gccPackage->environmentVariableName(), "");
    QCOMPARE(gccPackage->isDesktopToolchain(), true);
    QCOMPARE(gccPackage->toolChainName(), "gcc");
    QCOMPARE(gccPackage->toolchainType(), McuToolChainPackage::ToolChainType::GCC);
    QVERIFY(allOf(versions, [&](const QString &v) { return gccPackage->versions().contains(v); }));
}

void verifyMsvcToolchain(const McuToolChainPackagePtr &msvcPackage, const QStringList &versions)
{
    QVERIFY(msvcPackage != nullptr);
    QCOMPARE(msvcPackage->cmakeVariableName(), "");
    QCOMPARE(msvcPackage->environmentVariableName(), "");
    QCOMPARE(msvcPackage->toolchainType(), McuToolChainPackage::ToolChainType::MSVC);
    QCOMPARE(msvcPackage->isDesktopToolchain(), true);
    QCOMPARE(msvcPackage->toolChainName(), "msvc");
    QVERIFY(allOf(versions, [&](const QString &v) { return msvcPackage->versions().contains(v); }));
}

void verifyMingwToolchain(const McuToolChainPackagePtr &mingwPackage, const QStringList &versions)
{
    QVERIFY(mingwPackage != nullptr);
    QCOMPARE(mingwPackage->cmakeVariableName(), "");
    QCOMPARE(mingwPackage->environmentVariableName(), "");
    QCOMPARE(mingwPackage->toolchainType(), McuToolChainPackage::ToolChainType::MinGW);
    QCOMPARE(mingwPackage->isDesktopToolchain(), true);
    QCOMPARE(mingwPackage->toolChainName(), "mingw");
    QVERIFY(allOf(versions, [&](const QString &v) { return mingwPackage->versions().contains(v); }));
}

void verifyTargetToolchains(const Targets &targets,
                            const QString &toolchainFilePath,
                            const QString &toolchainFileDefaultPath,
                            const QString &compilerPath,
                            const Key &compilerSetting,
                            const QStringList &versions)
{
    QCOMPARE(targets.size(), 1);
    const auto &target{targets.first()};

    const auto toolchainFile{target->toolChainFilePackage()};
    QVERIFY(toolchainFile);
    QCOMPARE(toolchainFile->cmakeVariableName(), TOOLCHAIN_FILE_CMAKE_VARIABLE);
    QCOMPARE(toolchainFile->settingsKey(), empty);
    QCOMPARE(toolchainFile->path().toString(), toolchainFilePath);
    QCOMPARE(toolchainFile->defaultPath().toString(), toolchainFileDefaultPath);

    const auto toolchainCompiler{target->toolChainPackage()};
    QVERIFY(toolchainCompiler);
    QCOMPARE(toolchainCompiler->cmakeVariableName(), TOOLCHAIN_DIR_CMAKE_VARIABLE);
    QCOMPARE(toolchainCompiler->path().toString(), compilerPath);
    QCOMPARE(toolchainCompiler->settingsKey(), compilerSetting);
    QCOMPARE(toolchainCompiler->versions(), versions);
}

void verifyBoardSdk(const McuPackagePtr &boardSdk,
                    const QString &cmakeVariable,
                    const QString &environmentVariable,
                    const QStringList &versions)
{
    QVERIFY(boardSdk);
    QCOMPARE(boardSdk->cmakeVariableName(), cmakeVariable);
    QCOMPARE(boardSdk->environmentVariableName(), environmentVariable);
    QCOMPARE(boardSdk->settingsKey(), keyFromString(environmentVariable));
    QCOMPARE(boardSdk->detectionPath().toString(), empty);
    QCOMPARE(boardSdk->versions(), versions);
}

void verifyFreeRtosPackage(const McuPackagePtr &freeRtos,
                           const QString &envVar,
                           const FilePath &boardSdkDir,
                           const QString &freeRtosPath,
                           const QString &freeRtosDetectionPath,
                           const Key &expectedSettingsKey)
{
    QVERIFY(freeRtos);
    QCOMPARE(freeRtos->environmentVariableName(), envVar);
    QCOMPARE(freeRtos->cmakeVariableName(), freeRtosCMakeVar);
    QCOMPARE(freeRtos->settingsKey(), expectedSettingsKey);
    QCOMPARE(freeRtos->path().cleanPath().toString(), freeRtosPath);
    QCOMPARE(freeRtos->detectionPath().cleanPath().toString(), freeRtosDetectionPath);
    QVERIFY(freeRtos->path().toUserOutput().startsWith(boardSdkDir.cleanPath().toUserOutput()));
}

void verifyPackage(const McuPackagePtr &package,
                   const QString &path,
                   const QString &defaultPath,
                   const Key &setting,
                   const QString &cmakeVar,
                   const QString &envVar,
                   const QString &label,
                   const QString &detectionPath,
                   const QStringList &versions)
{
    QVERIFY(package);
    QCOMPARE(package->defaultPath().toString(), defaultPath);
    QCOMPARE(package->path().toString(), path);
    QCOMPARE(package->cmakeVariableName(), cmakeVar);
    QCOMPARE(package->environmentVariableName(), envVar);
    QCOMPARE(package->label(), label);
    QCOMPARE(package->detectionPath().toString(), detectionPath);
    QCOMPARE(package->settingsKey(), setting);
    QCOMPARE(package->versions(), versions);
}

// create fake files and folders for testing under the "testing_output_dir" folder
bool createFakePath(const FilePath& path, const bool is_file = false)
{
    if (path.exists())
        return true;

    //create an empty file or folder
    if (is_file) {
        if (!path.parentDir().createDir()) {
            qWarning() << "Could not create the parent dir for testing file " << path;
            return false;
        }
        if (!path.writeFileContents("placeholder text")) {
            qWarning() << "Could not write to testing file " << path;
            return false;
        }
    } else if (!path.createDir()) {
        qWarning() << "Could not create testing dir " << path;
        return false;
    }

    return true;
};

McuSupportTest::McuSupportTest()
    : targetFactory{settingsMockPtr}
    , compilerDescription{armGccLabel, armGccEnvVar, TOOLCHAIN_DIR_CMAKE_VARIABLE, armGccLabel, armGccDirectorySetting, {}, {}, {}, {}, false, Utils::PathChooser::Kind::ExistingDirectory }
     , toochainFileDescription{armGccLabel, armGccEnvVar, TOOLCHAIN_FILE_CMAKE_VARIABLE, armGccLabel, armGccDirectorySetting, {}, {}, {}, {}, false, Utils::PathChooser::Kind::ExistingDirectory }
    , targetDescription {
        "autotest-sourceFile",
        "2.0.1",
        "2",
        platformDescription,
        McuTargetDescription::Toolchain{armGcc, {}, compilerDescription, toochainFileDescription},
        PackageDescription{},
        McuTargetDescription::FreeRTOS{},
    }
    , toolchainPackagePtr{new McuToolChainPackage{
          settingsMockPtr,
          {},                                              // label
          {},                                              // defaultPath
          {},                                              // detectionPath
          {},                                              // settingsKey
          McuToolChainPackage::ToolChainType::Unsupported, // toolchain type
          {},                                              // versions
          {},                                              // cmake var name
          {},                                              // env var name
          nullptr                                          // version detector
      }}
    , armGccToolchainPackagePtr{new McuToolChainPackage{settingsMockPtr,
                                                        armGccLabel,
                                                        armGccDir,
                                                        {}, // validation path
                                                        armGccDirectorySetting,
                                                        McuToolChainPackage::ToolChainType::ArmGcc,
                                                        {armGccVersion},
                                                        TOOLCHAIN_DIR_CMAKE_VARIABLE,
                                                        armGccEnvVar,
                                                        nullptr}}
    , iarToolchainPackagePtr{new McuToolChainPackage{settingsMockPtr,
                                                     iarLabel,
                                                     iarDir,
                                                     {}, // validation path
                                                     iarSetting,
                                                     McuToolChainPackage::ToolChainType::IAR,
                                                     {armGccVersion},
                                                     TOOLCHAIN_DIR_CMAKE_VARIABLE,
                                                     iarEnvVar,
                                                     nullptr}}
    , platform{id, name, vendor}
    , mcuTarget{currentQulVersion,
                platform,
                McuTarget::OS::FreeRTOS,
                {sdkPackagePtr, freeRtosPackagePtr},
                armGccToolchainPackagePtr,
                armGccToolchainFilePackagePtr}
{
    testing::Mock::AllowLeak(settingsMockPtr.get());
    testing::FLAGS_gmock_verbose = "error";
}

// load the test file
std::pair<Targets, Packages> McuSupportTest::createTestingKitTargetsAndPackages(QByteArray test_file)
{
    McuTargetFactory factory(settingsMockPtr);
    auto [targets, packages] = factory.createTargets(parseDescriptionJson(test_file), sdkPackagePtr);
    expandTargetsAndPackages(targets, packages);
    return {targets, packages};
}

void McuSupportTest::initTestCase()
{
    EXPECT_CALL(*freeRtosPackage, environmentVariableName())
        .WillRepeatedly(Return(QString{freeRtosEnvVar}));
    EXPECT_CALL(*freeRtosPackage, cmakeVariableName())
        .WillRepeatedly(Return(QString{freeRtosCMakeVar}));
    EXPECT_CALL(*freeRtosPackage, isValidStatus()).WillRepeatedly(Return(true));
    EXPECT_CALL(*freeRtosPackage, path())
        .WillRepeatedly(Return(FilePath::fromUserInput(freeRtosPath)));
    EXPECT_CALL(*freeRtosPackage, isAddToSystemPath()).WillRepeatedly(Return(true));
    EXPECT_CALL(*freeRtosPackage, detectionPath()).WillRepeatedly(Return(FilePath{}));

    ON_CALL(*sdkPackage, label()).WillByDefault(Return(QString{QUL_LABEL}));
    ON_CALL(*sdkPackage, settingsKey())
        .WillByDefault(Return(Key{Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK}));
    ON_CALL(*sdkPackage, environmentVariableName()).WillByDefault(Return(QString{QUL_ENV_VAR}));
    ON_CALL(*sdkPackage, cmakeVariableName()).WillByDefault(Return(QString{QUL_CMAKE_VAR}));
    ON_CALL(*sdkPackage, isValidStatus()).WillByDefault(Return(true));
    ON_CALL(*sdkPackage, path()).WillByDefault(Return(FilePath::fromUserInput(qtForMcuSdkPath)));
    ON_CALL(*sdkPackage, isAddToSystemPath()).WillByDefault(Return(true));
    ON_CALL(*sdkPackage, detectionPath()).WillByDefault(Return(FilePath{}));

    EXPECT_CALL(*armGccToolchainFilePackage, environmentVariableName())
        .WillRepeatedly(Return(QString{QString{}}));
    EXPECT_CALL(*armGccToolchainFilePackage, cmakeVariableName())
        .WillRepeatedly(Return(QString{TOOLCHAIN_FILE_CMAKE_VARIABLE}));
    EXPECT_CALL(*armGccToolchainFilePackage, isValidStatus()).WillRepeatedly(Return(true));
    EXPECT_CALL(*armGccToolchainFilePackage, path())
        .WillRepeatedly(Return(FilePath::fromUserInput(armGccToolchainFilePath)));
    EXPECT_CALL(*armGccToolchainFilePackage, isAddToSystemPath()).WillRepeatedly(Return(false));
    EXPECT_CALL(*armGccToolchainFilePackage, detectionPath()).WillRepeatedly(Return(FilePath{}));

    ON_CALL(*settingsMockPtr, getPath)
        .WillByDefault([](const Key &, QSettings::Scope, const FilePath &m_defaultPath) {
            return m_defaultPath;
        });
}

void McuSupportTest::init()
{
    McuSdkRepository::globalMacros()
        ->insert("MCU_TESTING_FOLDER",
                 [dir = testing_output_dir.absoluteFilePath().toString()] { return dir; });
    qDebug() << __func__;
}

void McuSupportTest::cleanup()
{
    QVERIFY(testing::Mock::VerifyAndClearExpectations(settingsMockPtr.get()));
    QVERIFY(testing::Mock::VerifyAndClearExpectations(freeRtosPackage));
    QVERIFY(testing::Mock::VerifyAndClearExpectations(sdkPackage));
}

void McuSupportTest::test_parseBasicInfoFromJson()
{
    const auto description = parseDescriptionJson(iar_mimxrt1064_evk_freertos_json);

    QVERIFY(!description.freeRTOS.envVar.isEmpty());
}

void McuSupportTest::test_parseCmakeEntries()
{
    const auto description{parseDescriptionJson(iar_mimxrt1064_evk_freertos_json)};

    const auto &freeRtos = description.freeRTOS.package;
    QCOMPARE(freeRtos.envVar, nxp1064FreeRtosEnvVar);
}

void McuSupportTest::test_parseToolchainFromJSON_data()
{
    QTest::addColumn<QString>("json");
    QTest::addColumn<QString>("environmentVariable");
    QTest::addColumn<QString>("label");
    QTest::addColumn<QString>("toolchainFileDefaultPath");
    QTest::addColumn<QString>("id");

    //TODO(me): Add ghs nxp 1064 nxp 1070.
    QTest::newRow("armgcc_mimxrt1050_evk_freertos_json")
        << armgcc_mimxrt1050_evk_freertos_json << armGccEnvVar << armGccLabel
        << armGccToolchainFileUnexpandedPath << armGcc;
    QTest::newRow("armgcc_mimxrt1064_evk_freertos_json")
        << armgcc_mimxrt1064_evk_freertos_json << armGccEnvVar << armGccLabel
        << armGccToolchainFileUnexpandedPath << armGcc;
    QTest::newRow("armgcc_mimxrt1170_evk_freertos_json")
        << armgcc_mimxrt1170_evk_freertos_json << armGccEnvVar << armGccLabel
        << armGccToolchainFileUnexpandedPath << armGcc;
    QTest::newRow("armgcc_stm32f769i_discovery_freertos_json")
        << armgcc_stm32f769i_discovery_freertos_json << armGccEnvVar << armGccLabel
        << armGccToolchainFileUnexpandedPath << armGcc;
    QTest::newRow("armgcc_stm32h750b_discovery_baremetal_json")
        << armgcc_stm32h750b_discovery_baremetal_json << armGccEnvVar << armGccLabel
        << armGccToolchainFileUnexpandedPath << armGcc;

    QTest::newRow("iar_stm32f469i_discovery_baremetal_json")
        << iar_stm32f469i_discovery_baremetal_json << iarEnvVar << iarLabel
        << iarToolchainFileUnexpandedPath << iar;
}

void McuSupportTest::test_parseToolchainFromJSON()
{
    QFETCH(QString, json);
    QFETCH(QString, environmentVariable);
    QFETCH(QString, label);
    QFETCH(QString, toolchainFileDefaultPath);
    QFETCH(QString, id);

    McuTargetDescription description{parseDescriptionJson(json.toLocal8Bit())};
    QCOMPARE(description.toolchain.id, id);

    const PackageDescription &compilerPackage{description.toolchain.compiler};
    QCOMPARE(compilerPackage.cmakeVar, TOOLCHAIN_DIR_CMAKE_VARIABLE);
    QCOMPARE(compilerPackage.envVar, environmentVariable);

    const PackageDescription &toolchainFilePackage{description.toolchain.file};
    QCOMPARE(toolchainFilePackage.label, QString{});
    QCOMPARE(toolchainFilePackage.envVar, QString{});
    QCOMPARE(toolchainFilePackage.cmakeVar, TOOLCHAIN_FILE_CMAKE_VARIABLE);
    QCOMPARE(toolchainFilePackage.defaultPath.cleanPath().toString(), toolchainFileDefaultPath);
}

void McuSupportTest::test_legacy_createIarToolchain()
{
    McuToolChainPackagePtr iarToolchainPackage = Legacy::createIarToolChainPackage(settingsMockPtr,
                                                                                   iarVersions);
    verifyIarToolchain(iarToolchainPackage);
}

void McuSupportTest::test_createIarToolchain()
{
    const auto description = parseDescriptionJson(iar_stm32f469i_discovery_baremetal_json);

    McuToolChainPackagePtr iarToolchainPackage{targetFactory.createToolchain(description.toolchain)};
    verifyIarToolchain(iarToolchainPackage);
}

void McuSupportTest::test_legacy_createDesktopGccToolchain()
{
    McuToolChainPackagePtr gccPackage = Legacy::createGccToolChainPackage(settingsMockPtr,
                                                                          {armGccVersion});
    verifyGccToolchain(gccPackage, {armGccVersion});
}

void McuSupportTest::test_createDesktopGccToolchain()
{
    const auto description = parseDescriptionJson(gcc_desktop_json);
    McuToolChainPackagePtr gccPackage{targetFactory.createToolchain(description.toolchain)};
    verifyGccToolchain(gccPackage, {});
}

void McuSupportTest::test_legacy_createDesktopMsvcToolchain()
{
    McuToolChainPackagePtr msvcPackage = Legacy::createMsvcToolChainPackage(settingsMockPtr,
                                                                            {msvcVersion});
    verifyMsvcToolchain(msvcPackage, {msvcVersion});
}

void McuSupportTest::test_createDesktopMsvcToolchain()
{
    const auto description = parseDescriptionJson(msvc_desktop_json);
    McuToolChainPackagePtr msvcPackage{targetFactory.createToolchain(description.toolchain)};
    verifyMsvcToolchain(msvcPackage, {});
}

void McuSupportTest::test_createDesktopMingwToolchain()
{
    const auto description = parseDescriptionJson(mingw_desktop_json);
    McuToolChainPackagePtr mingwPackage{targetFactory.createToolchain(description.toolchain)};
    verifyMingwToolchain(mingwPackage, {"11.2.0"});
}

void McuSupportTest::test_verifyManuallyCreatedArmGccToolchain()
{
    verifyArmGccToolchain(armGccToolchainPackagePtr, {armGccVersion});
}

void McuSupportTest::test_legacy_createArmGccToolchain()
{
    McuToolChainPackagePtr armGccPackage = Legacy::createArmGccToolchainPackage(settingsMockPtr,
                                                                                {armGccVersion});
    verifyArmGccToolchain(armGccPackage, {armGccVersion});
}

void McuSupportTest::test_createArmGccToolchain_data()
{
    QTest::addColumn<QString>("json");
    QTest::newRow("armgcc_mimxrt1050_evk_freertos_json") << armgcc_mimxrt1050_evk_freertos_json;
    QTest::newRow("armgcc_stm32f769i_discovery_freertos_json")
        << armgcc_stm32f769i_discovery_freertos_json;
    QTest::newRow("armgcc_stm32h750b_discovery_baremetal_json")
        << armgcc_stm32h750b_discovery_baremetal_json;
    QTest::newRow("armgcc_mimxrt1170_evk_freertos_json") << armgcc_mimxrt1170_evk_freertos_json;
}

void McuSupportTest::test_createArmGccToolchain()
{
    QFETCH(QString, json);

    const auto description = parseDescriptionJson(json.toLocal8Bit());
    McuToolChainPackagePtr armGccPackage{targetFactory.createToolchain(description.toolchain)};
    verifyArmGccToolchain(armGccPackage, description.toolchain.versions);
}

void McuSupportTest::test_mapParsedToolchainIdToCorrespondingType_data()
{
    QTest::addColumn<McuTargetDescription>("description");
    QTest::addColumn<McuToolChainPackage::ToolChainType>("toolchainType");

    QTest::newRow("armgcc_stm32h750b_discovery_baremetal_json")
        << parseDescriptionJson(armgcc_stm32h750b_discovery_baremetal_json)
        << McuToolChainPackage::ToolChainType::ArmGcc;
    QTest::newRow("iar_mimxrt1064_evk_freertos_json")
        << parseDescriptionJson(iar_mimxrt1064_evk_freertos_json)
        << McuToolChainPackage::ToolChainType::IAR;
    QTest::newRow("iar_stm32f469i") << parseDescriptionJson(iar_stm32f469i_discovery_baremetal_json)
                                    << McuToolChainPackage::ToolChainType::IAR;
}

void McuSupportTest::test_mapParsedToolchainIdToCorrespondingType()
{
    QFETCH(McuTargetDescription, description);
    QFETCH(McuToolChainPackage::ToolChainType, toolchainType);

    const McuToolChainPackage *toolchain{targetFactory.createToolchain(description.toolchain)};

    QVERIFY(toolchain != nullptr);
    QCOMPARE(toolchain->toolchainType(), toolchainType);
}

void McuSupportTest::test_legacy_createPackagesWithCorrespondingSettings_data()
{
    QTest::addColumn<QString>("json");
    QTest::addColumn<QSet<Key>>("expectedSettings");

    QSet<Key> commonSettings{{"CypressAutoFlashUtil"},
                             {"MCUXpressoIDE"},
                             {"RenesasFlashProgrammer"},
                             {"Stm32CubeProgrammer"}};

    QTest::newRow("iar_mimxrt1064_evk_freertos_json")
        << iar_mimxrt1064_evk_freertos_json
        << QSet<Key>{"EVK_MIMXRT1064_SDK_PATH",
                     Key{QByteArray(Legacy::Constants::SETTINGS_KEY_FREERTOS_PREFIX).append("IMXRT1064")},
                     "IARToolchain"}
               .unite(commonSettings);
    QTest::newRow("stm32f469i") << iar_stm32f469i_discovery_baremetal_json
                                << QSet<Key>{{"STM32Cube_FW_F4_SDK_PATH"}, "IARToolchain"}.unite(
                                       commonSettings);
    QTest::newRow("nxp1050")
        << armgcc_mimxrt1050_evk_freertos_json
        << QSet<Key>{"EVKB_IMXRT1050_SDK_PATH",
                     Key{QByteArray(Legacy::Constants::SETTINGS_KEY_FREERTOS_PREFIX).append("IMXRT1050")},
                     "GNUArmEmbeddedToolchain"}
                .unite(commonSettings);
    QTest::newRow("armgcc_stm32h750b_discovery_baremetal_json")
        << armgcc_stm32h750b_discovery_baremetal_json
        << QSet<Key>{{"STM32Cube_FW_H7_SDK_PATH"}, "GNUArmEmbeddedToolchain"}.unite(
               commonSettings);
    QTest::newRow("armgcc_stm32f769i_discovery_freertos_json")
        << armgcc_stm32f769i_discovery_freertos_json
        << QSet<Key>{{"STM32Cube_FW_F7_SDK_PATH"}, "GNUArmEmbeddedToolchain"}.unite(
               commonSettings);

    QTest::newRow("ghs_rh850_d1m1a_baremetal_json")
        << ghs_rh850_d1m1a_baremetal_json << QSet<Key>{"GHSToolchain"}.unite(commonSettings);
}

void McuSupportTest::test_legacy_createPackagesWithCorrespondingSettings()
{
    QFETCH(QString, json);
    const McuTargetDescription description = parseDescriptionJson(json.toLocal8Bit());
    auto [targets, packages]{
        targetsFromDescriptions({description}, settingsMockPtr, sdkPackagePtr, runLegacy)};
    Q_UNUSED(targets);

    QSet<Key> settings = transform<QSet<Key>>(packages, [](const auto &package) {
        return package->settingsKey();
    });
    QFETCH(QSet<Key>, expectedSettings);
    QVERIFY(settings.contains(expectedSettings));
}

void McuSupportTest::test_createTargets()
{
    targetDescription.freeRTOS.package = {id,
                                          nxp1064FreeRtosEnvVar,
                                          freeRtosCMakeVar,
                                          freeRtosSetting,
                                          freeRtosLabel,
                                          freeRtosPath,
                                          freeRtosDetectionPath,
                                          {},
                                          VersionDetection{},
                                          true,
                                          Utils::PathChooser::Kind::ExistingDirectory};
    targetDescription.toolchain.id = armGcc;

    auto [targets, packages]{targetFactory.createTargets(targetDescription, sdkPackagePtr)};
    expandTargetsAndPackages(targets, packages);
    QCOMPARE(targets.size(), 1);
    const McuTargetPtr target{targets.at(0)};

    QCOMPARE(target->colorDepth(), colorDepth);

    const auto &tgtPackages{target->packages()};
    QCOMPARE(tgtPackages.size(), 5); // qtForMCUs, toolchain file + compiler,
                                     // freertos, board sdk

    // target should contain freertos package
    QVERIFY(anyOf(tgtPackages, [](const McuPackagePtr &pkg) {
        return (pkg->environmentVariableName() == nxp1064FreeRtosEnvVar
                && pkg->cmakeVariableName() == freeRtosCMakeVar && pkg->label() == id);
    }));

    // all packages should contain target's tooclhain compiler package.
    QVERIFY(anyOf(packages, [](const McuPackagePtr &pkg) {
        return (pkg->cmakeVariableName() == TOOLCHAIN_DIR_CMAKE_VARIABLE);
    }));

    // target should contain tooclhain copmiler package.
    QVERIFY(anyOf(tgtPackages, [](const McuPackagePtr &pkg) {
        return (pkg->cmakeVariableName() == TOOLCHAIN_DIR_CMAKE_VARIABLE);
    }));

    // all packages should contain target's tooclhain file package.
    QVERIFY(anyOf(packages, [](const McuPackagePtr &pkg) {
        return (pkg->cmakeVariableName() == TOOLCHAIN_FILE_CMAKE_VARIABLE);
    }));

    // target should contain tooclhain file package.
    QVERIFY(anyOf(tgtPackages, [](const McuPackagePtr &pkg) {
        return (pkg->cmakeVariableName() == TOOLCHAIN_FILE_CMAKE_VARIABLE);
    }));
}

void McuSupportTest::test_createPackages()
{
    targetDescription.freeRTOS.package = {id,
                                          nxp1064FreeRtosEnvVar,
                                          freeRtosCMakeVar,
                                          freeRtosLabel,
                                          freeRtosSetting,
                                          freeRtosPath,
                                          freeRtosDetectionPath,
                                          {},
                                          VersionDetection{},
                                          true,
                                          Utils::PathChooser::Kind::ExistingDirectory};

    const auto packages{targetFactory.createPackages(targetDescription)};
    QVERIFY(!packages.empty());
}

void McuSupportTest::test_removeRtosSuffixFromEnvironmentVariable_data()
{
    QTest::addColumn<QString>("freeRtosEnvVar");
    QTest::addColumn<QString>("expectedEnvVarWithoutSuffix");

    QTest::newRow("armgcc_mimxrt1050_evk_freertos_json") << nxp1050FreeRtosEnvVar << nxp1050;
    QTest::newRow("iar_mimxrt1064_evk_freertos_json") << nxp1064FreeRtosEnvVar << nxp1064;
    QTest::newRow("nxp1170") << nxp1170FreeRtosEnvVar << nxp1170;
    QTest::newRow("stm32f7") << stm32f7FreeRtosEnvVar << stm32f7;
}

void McuSupportTest::test_removeRtosSuffixFromEnvironmentVariable()
{
    QFETCH(QString, freeRtosEnvVar);
    QFETCH(QString, expectedEnvVarWithoutSuffix);
    QCOMPARE(removeRtosSuffix(freeRtosEnvVar), expectedEnvVarWithoutSuffix);
}

void McuSupportTest::test_twoDotOneUsesLegacyImplementation()
{
    QCOMPARE(McuSupportOptions::isLegacyVersion({2, 1}), true);
    QCOMPARE(McuSupportOptions::isLegacyVersion({2, 0}), true);
    QCOMPARE(McuSupportOptions::isLegacyVersion({2, 0, 0}), true);
    QCOMPARE(McuSupportOptions::isLegacyVersion({2, 0, 1}), true);
    QCOMPARE(McuSupportOptions::isLegacyVersion({2, 2, 0}), true);
    QCOMPARE(McuSupportOptions::isLegacyVersion({2, 2, 1}), true);
    QCOMPARE(McuSupportOptions::isLegacyVersion({2, 3, 0}), false);
}
void McuSupportTest::test_useFallbackPathForToolchainWhenPathFromSettingsIsNotAvailable()
{
    PackageDescription compilerDescription{armGcc,
                                           armGccEnvVar,
                                           TOOLCHAIN_DIR_CMAKE_VARIABLE,
                                           armGcc,
                                           armGccDirectorySetting,
                                           fallbackDir,
                                           {},
                                           {},
                                           VersionDetection{},
                                           false,
                                           Utils::PathChooser::Kind::ExistingDirectory};
    McuTargetDescription::Toolchain toolchainDescription{armGcc, {}, compilerDescription, {}};

    EXPECT_CALL(*settingsMockPtr, getPath(Key{armGccDirectorySetting}, _, FilePath{fallbackDir}))
        .Times(2)
        .WillRepeatedly(Return(FilePath{fallbackDir}));

    McuToolChainPackage *toolchain = targetFactory.createToolchain(toolchainDescription);

    QCOMPARE(toolchain->path().toString(), fallbackDir);
}

void McuSupportTest::test_usePathFromSettingsForToolchainPath()
{
    PackageDescription compilerDescription{{},
                                           armGccEnvVar,
                                           TOOLCHAIN_DIR_CMAKE_VARIABLE,
                                           armGcc,
                                           armGccDirectorySetting,
                                           empty,
                                           {},
                                           {},
                                           VersionDetection{},
                                           false,
                                           Utils::PathChooser::Kind::ExistingDirectory};
    McuTargetDescription::Toolchain toolchainDescription{armGcc, {}, compilerDescription, {}};

    EXPECT_CALL(*settingsMockPtr, getPath(Key{armGccDirectorySetting}, _, FilePath{empty}))
        .Times(2)
        .WillOnce(Return(FilePath{empty}))      // system scope settings
        .WillOnce(Return(FilePath{armGccDir})); // user scope settings

    McuToolChainPackage *toolchain = targetFactory.createToolchain(toolchainDescription);
    QCOMPARE(toolchain->path().toString(), armGccDir);
}

void McuSupportTest::test_addNewKit()
{
    const QString cmakeVar = "CMAKE_SDK";
    EXPECT_CALL(*sdkPackage, cmakeVariableName()).WillRepeatedly(Return(cmakeVar));

    auto &kitManager{*KitManager::instance()};

    QSignalSpy kitAddedSpy(&kitManager, &KitManager::kitAdded);

    auto *newKit{McuKitManager::newKit(&mcuTarget, sdkPackagePtr)};
    QVERIFY(newKit != nullptr);

    QCOMPARE(kitAddedSpy.count(), 1);
    QList<QVariant> arguments = kitAddedSpy.takeFirst();
    auto *createdKit = qvariant_cast<Kit *>(arguments.at(0));
    QVERIFY(createdKit != nullptr);
    QCOMPARE(createdKit, newKit);

    const auto config = CMakeConfigurationKitAspect::configuration(newKit);
    QVERIFY(!config.empty());
    QVERIFY(indexOf(config.toVector(),
                    [&cmakeVar](const CMakeProjectManager::CMakeConfigItem &item) {
                        return item.key == cmakeVar.toUtf8();
                    })
            != -1);
}

void McuSupportTest::test_getFullToolchainFilePathFromTarget()
{
    QCOMPARE(mcuTarget.toolChainFilePackage()->path().toUserOutput(),
             armGccToolchainFilePackagePtr->path().toUserOutput());
}

void McuSupportTest::test_legacy_getPredefinedToolchainFilePackage()
{
    QHash<QString, Legacy::ToolchainCompilerCreator> toolchainCreators{
        {armGcc, [this](const QStringList & /*versions*/) { return armGccToolchainPackagePtr; }}};
    Legacy::McuTargetFactory legacyTargetFactory{toolchainCreators,
                                                 {{armGcc, armGccToolchainFilePackagePtr}},
                                                 {},
                                                 settingsMockPtr};
    auto armToolchainFile = legacyTargetFactory.getToolchainFile(qtForMcuSdkPath, armGcc);
    QVERIFY(armToolchainFile);
    QCOMPARE(armToolchainFile, armGccToolchainFilePackagePtr);
}

void McuSupportTest::test_legacy_createUnsupportedToolchainFilePackage()
{
    QHash<QString, Legacy::ToolchainCompilerCreator> toolchainCreators{
        {armGcc, [this](const QStringList & /*versions*/) { return armGccToolchainPackagePtr; }}};
    Legacy::McuTargetFactory legacyTargetFactory{toolchainCreators,
                                                 {{armGcc, armGccToolchainFilePackagePtr}},
                                                 {},
                                                 settingsMockPtr};

    auto unsupportedToolchainFile = legacyTargetFactory.getToolchainFile(qtForMcuSdkPath, iar);
    QVERIFY(unsupportedToolchainFile);
    QCOMPARE(unsupportedToolchainFile->path().toString(), unsupportedToolchainFilePath);
    QCOMPARE(unsupportedToolchainFile->cmakeVariableName(), TOOLCHAIN_FILE_CMAKE_VARIABLE);
}

void McuSupportTest::test_legacy_createTargetWithToolchainPackages_data()
{
    QTest::addColumn<QString>("json");
    QTest::addColumn<QString>("toolchainFilePath");
    QTest::addColumn<QString>("toolchainFileDefaultPath");
    QTest::addColumn<QString>("compilerPath");
    QTest::addColumn<Key>("compilerSetting");
    QTest::addColumn<QStringList>("versions");

    QTest::newRow("armgcc_mimxrt1050_evk_freertos_json")
        << armgcc_mimxrt1050_evk_freertos_json << armGccToolchainFilePath
        << armGccToolchainFileUnexpandedPath << armGccDir << keyFromString(armGccDirectorySetting)
        << QStringList{armGccVersion};
    QTest::newRow("armgcc_mimxrt1064_evk_freertos_json")
        << armgcc_mimxrt1064_evk_freertos_json << armGccToolchainFilePath
        << armGccToolchainFileUnexpandedPath << armGccDir << keyFromString(armGccDirectorySetting)
        << QStringList{armGccVersion};
    QTest::newRow("armgcc_mimxrt1170_evk_freertos_json")
        << armgcc_mimxrt1170_evk_freertos_json << armGccToolchainFilePath
        << armGccToolchainFileUnexpandedPath << armGccDir << keyFromString(armGccDirectorySetting)
        << QStringList{armGccVersion};
    QTest::newRow("armgcc_stm32h750b_discovery_baremetal_json")
        << armgcc_stm32h750b_discovery_baremetal_json << armGccToolchainFilePath
        << armGccToolchainFileUnexpandedPath << armGccDir << keyFromString(armGccDirectorySetting)
        << QStringList{armGccVersion};
    QTest::newRow("armgcc_stm32f769i_discovery_freertos_json")
        << armgcc_stm32f769i_discovery_freertos_json << armGccToolchainFilePath
        << armGccToolchainFileUnexpandedPath << armGccDir << keyFromString(armGccDirectorySetting)
        << QStringList{armGccVersion};
    QTest::newRow("iar_stm32f469i_discovery_baremetal_json")
        << iar_stm32f469i_discovery_baremetal_json << iarToolchainFilePath
        << iarToolchainFileUnexpandedPath << iarDir << keyFromString(iarSetting) << iarVersions;
    QTest::newRow("iar_mimxrt1064_evk_freertos_json")
        << iar_mimxrt1064_evk_freertos_json << iarToolchainFilePath
        << iarToolchainFileUnexpandedPath << iarDir << keyFromString(iarSetting) << iarVersions;
    QTest::newRow("ghs_rh850_d1m1a_baremetal_json")
        << ghs_rh850_d1m1a_baremetal_json << greenhillToolchainFilePath
        << greenhillToolchainFileUnexpandedPath << greenhillCompilerDir
        << keyFromString(greenhillSetting) << greenhillVersions;
}

void McuSupportTest::test_legacy_createTargetWithToolchainPackages()
{
    QFETCH(QString, json);
    QFETCH(QString, toolchainFilePath);
    QFETCH(QString, toolchainFileDefaultPath);
    QFETCH(QString, compilerPath);
    QFETCH(Key, compilerSetting);
    QFETCH(QStringList, versions);

    const McuTargetDescription description = parseDescriptionJson(json.toLocal8Bit());

    EXPECT_CALL(*settingsMockPtr,
                getPath(Key{Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK}, _, _))
        .WillRepeatedly(Return(FilePath::fromUserInput(qtForMcuSdkPath)));
    EXPECT_CALL(*settingsMockPtr, getPath(compilerSetting, _, _))
        .WillRepeatedly(Return(FilePath::fromUserInput(compilerPath)));

    const auto [targets, packages]{
        targetsFromDescriptions({description}, settingsMockPtr, sdkPackagePtr, runLegacy)};
    Q_UNUSED(packages);

    verifyTargetToolchains(targets,
                           toolchainFilePath,
                           toolchainFilePath,
                           compilerPath,
                           compilerSetting,
                           versions);
}

void McuSupportTest::test_createTargetWithToolchainPackages_data()
{
    test_legacy_createTargetWithToolchainPackages_data();
}

void McuSupportTest::test_createTargetWithToolchainPackages()
{
    QFETCH(QString, json);
    QFETCH(QString, toolchainFilePath);
    QFETCH(QString, toolchainFileDefaultPath);
    QFETCH(QString, compilerPath);
    QFETCH(Key, compilerSetting);
    QFETCH(QStringList, versions);

    EXPECT_CALL(*settingsMockPtr, getPath(compilerSetting, _, _))
        .WillRepeatedly(Return(FilePath::fromUserInput(compilerPath)));

    EXPECT_CALL(*settingsMockPtr,
                getPath(Key{Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK}, _, _))
        .WillRepeatedly(Return(FilePath::fromUserInput(qtForMcuSdkPath)));

    const McuTargetDescription description = parseDescriptionJson(json.toLocal8Bit());
    const auto [targets, packages]{
        targetsFromDescriptions({description}, settingsMockPtr, sdkPackagePtr, !runLegacy)};
    Q_UNUSED(packages);

    const auto qtForMCUsSDK = findOrDefault(packages, [](const McuPackagePtr &pkg) {
        return (pkg->cmakeVariableName() == QUL_CMAKE_VAR);
    });

    verifyPackage(qtForMCUsSDK,
                  qtForMcuSdkPath,
                  {},
                  Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK,
                  QUL_CMAKE_VAR,
                  QUL_ENV_VAR,
                  QUL_LABEL,
                  {},
                  {});

    verifyTargetToolchains(targets,
                           toolchainFilePath,
                           toolchainFileDefaultPath, // default path is unexpanded.
                           compilerPath,
                           compilerSetting,
                           versions);
}

void McuSupportTest::test_addToolchainFileInfoToKit()
{
    McuKitManager::upgradeKitInPlace(&kit, &mcuTarget, sdkPackagePtr);

    QVERIFY(kit.isValid());
    QCOMPARE(kit.value(Constants::KIT_MCUTARGET_TOOLCHAIN_KEY).toString(), armGcc);

    const auto &cmakeConfig{CMakeConfigurationKitAspect::configuration(&kit)};
    QVERIFY(!cmakeConfig.empty());
    QCOMPARE(cmakeConfig.valueOf(TOOLCHAIN_FILE_CMAKE_VARIABLE), armGccToolchainFilePath);
}

void McuSupportTest::test_legacy_createBoardSdk_data()
{
    QTest::addColumn<QString>("json");
    QTest::addColumn<QString>("cmakeVariable");
    QTest::addColumn<QString>("environmentVariable");
    QTest::addColumn<QStringList>("versions");

    QTest::newRow("armgcc_mimxrt1050_evk_freertos_json")
        << armgcc_mimxrt1050_evk_freertos_json << boardSdkCmakeVar << "EVKB_IMXRT1050_SDK_PATH"
        << QStringList{boardSdkVersion};

    QTest::newRow("armgcc_mimxrt1064_evk_freertos_json")
        << armgcc_mimxrt1064_evk_freertos_json << boardSdkCmakeVar << "EVK_MIMXRT1064_SDK_PATH"
        << QStringList{boardSdkVersion};
    QTest::newRow("armgcc_stm32h750b_discovery_baremetal_json")
        << armgcc_stm32h750b_discovery_baremetal_json << boardSdkCmakeVar
        << "STM32Cube_FW_H7_SDK_PATH" << QStringList{"1.10.0"};
    QTest::newRow("armgcc_stm32f769i_discovery_freertos_json")
        << armgcc_stm32f769i_discovery_freertos_json << boardSdkCmakeVar
        << "STM32Cube_FW_F7_SDK_PATH" << QStringList{"1.17.0"};
    QTest::newRow("iar_stm32f469i_discovery_baremetal_json")
        << iar_stm32f469i_discovery_baremetal_json << boardSdkCmakeVar << "STM32Cube_FW_F4_SDK_PATH"
        << QStringList{"1.27.0"};
    QTest::newRow("iar_mimxrt1064_evk_freertos_json")
        << iar_mimxrt1064_evk_freertos_json << boardSdkCmakeVar << "EVK_MIMXRT1064_SDK_PATH"
        << QStringList{boardSdkVersion};
    QTest::newRow("ghs_rh850_d1m1a_baremetal_json")
        << ghs_rh850_d1m1a_baremetal_json << boardSdkCmakeVar << "RGL_DIR" << QStringList{"2.0.0", "2.0.0a"};
}

void McuSupportTest::test_legacy_createBoardSdk()
{
    QFETCH(QString, json);
    QFETCH(QString, cmakeVariable);
    QFETCH(QString, environmentVariable);
    QFETCH(QStringList, versions);

    McuTargetDescription target{parseDescriptionJson(json.toLocal8Bit())};
    McuPackagePtr boardSdk{Legacy::createBoardSdkPackage(settingsMockPtr, target)};

    verifyBoardSdk(boardSdk, cmakeVariable, environmentVariable, versions);
}

void McuSupportTest::test_createBoardSdk_data()
{
    test_legacy_createBoardSdk_data();
}

void McuSupportTest::test_createBoardSdk()
{
    QFETCH(QString, json);
    QFETCH(QString, cmakeVariable);
    QFETCH(QString, environmentVariable);
    QFETCH(QStringList, versions);

    McuTargetDescription target{parseDescriptionJson(json.toLocal8Bit())};

    McuPackagePtr boardSdk{targetFactory.createPackage(target.boardSdk)};

    verifyBoardSdk(boardSdk, cmakeVariable, environmentVariable, versions);
}

void McuSupportTest::test_legacy_createFreeRtosPackage_data()
{
    QTest::addColumn<QString>("json");
    QTest::addColumn<QStringList>("versions");
    QTest::addColumn<Key>("expectedSettingsKey");
    QTest::addColumn<FilePath>("expectedPath");
    QTest::addColumn<FilePath>("expectedDetectionPath");

    QTest::newRow("armgcc_mimxrt1050_evk_freertos_json")
        << armgcc_mimxrt1050_evk_freertos_json << QStringList{boardSdkVersion}
        << keyFromString(QString{Legacy::Constants::SETTINGS_KEY_FREERTOS_PREFIX}.append(nxp1050))
        << FilePath::fromUserInput(boardSdkDir) / freeRtosNxpPathSuffix
        << FilePath::fromUserInput(freeRtosDetectionPath);
    QTest::newRow("armgcc_mimxrt1064_evk_freertos_json")
        << armgcc_mimxrt1064_evk_freertos_json << QStringList{boardSdkVersion}
        << keyFromString(QString{Legacy::Constants::SETTINGS_KEY_FREERTOS_PREFIX}.append(nxp1064))
        << FilePath::fromUserInput(boardSdkDir) / freeRtosNxpPathSuffix
        << FilePath::fromUserInput(freeRtosDetectionPath);
    QTest::newRow("iar_mimxrt1064_evk_freertos_json")
        << iar_mimxrt1064_evk_freertos_json << QStringList{boardSdkVersion}
        << keyFromString(QString{Legacy::Constants::SETTINGS_KEY_FREERTOS_PREFIX}.append(nxp1064))
        << FilePath::fromUserInput(boardSdkDir) / freeRtosNxpPathSuffix
        << FilePath::fromUserInput(freeRtosDetectionPath);
    QTest::newRow("armgcc_stm32f769i_discovery_freertos_json")
        << armgcc_stm32f769i_discovery_freertos_json << QStringList{"1.16.0"}
        << keyFromString(QString{Legacy::Constants::SETTINGS_KEY_FREERTOS_PREFIX}.append(stm32f7))
        << FilePath::fromUserInput(boardSdkDir) / freeRtosStmPathSuffix
        << FilePath::fromUserInput(freeRtosDetectionPath);
}

void McuSupportTest::test_legacy_createFreeRtosPackage()
{
    QFETCH(QString, json);
    QFETCH(QStringList, versions);
    QFETCH(Key, expectedSettingsKey);
    QFETCH(FilePath, expectedPath);
    QFETCH(FilePath, expectedDetectionPath);

    McuTargetDescription targetDescription{parseDescriptionJson(json.toLocal8Bit())};

    EXPECT_CALL(*settingsMockPtr, getPath(expectedSettingsKey, _, _))
        .WillRepeatedly(Return(FilePath::fromString(expectedPath.toUserOutput())));
    McuPackagePtr freeRtos{Legacy::createFreeRTOSSourcesPackage(settingsMockPtr,
                                                                targetDescription.freeRTOS.envVar,
                                                                FilePath{})};

    verifyFreeRtosPackage(freeRtos,
                          targetDescription.freeRTOS.envVar,
                          boardSdkDir,
                          expectedPath.toString(),
                          expectedDetectionPath.toString(),
                          expectedSettingsKey);
}

void McuSupportTest::test_createFreeRtosPackage_data()
{
    test_legacy_createFreeRtosPackage_data();
}

void McuSupportTest::test_createFreeRtosPackage()
{
    QFETCH(QString, json);
    QFETCH(QStringList, versions);
    QFETCH(Key, expectedSettingsKey);
    QFETCH(FilePath, expectedPath);
    QFETCH(FilePath, expectedDetectionPath);

    McuTargetDescription targetDescription{parseDescriptionJson(json.toLocal8Bit())};

    EXPECT_CALL(*settingsMockPtr, getPath(keyFromString(targetDescription.boardSdk.envVar), _, _))
        .WillRepeatedly(Return(FilePath::fromString(boardSdkDir)));

    auto [targets, packages]{targetFactory.createTargets(targetDescription, sdkPackagePtr)};
    expandTargetsAndPackages(targets, packages);

    auto freeRtos = findOrDefault(packages, [](const McuPackagePtr &pkg) {
        return (pkg->cmakeVariableName() == freeRtosCMakeVar);
    });

    verifyFreeRtosPackage(freeRtos,
                          targetDescription.freeRTOS.envVar,
                          boardSdkDir,
                          expectedPath.toString(),
                          expectedDetectionPath.toString(),
                          expectedSettingsKey);
}

void McuSupportTest::test_legacy_doNOTcreateFreeRtosPackageForMetalVariants_data()

{
    QTest::addColumn<QString>("json");
    QTest::newRow("iar_stm32f469i_discovery_baremetal_json")
        << iar_stm32f469i_discovery_baremetal_json;
    QTest::newRow("armgcc_stm32h750b_discovery_baremetal_json")
        << armgcc_stm32h750b_discovery_baremetal_json;
    QTest::newRow("ghs_rh850_d1m1a_baremetal_json") << ghs_rh850_d1m1a_baremetal_json;
    QTest::newRow("gcc_desktop_json") << gcc_desktop_json;
}

void McuSupportTest::test_legacy_doNOTcreateFreeRtosPackageForMetalVariants()
{
    QFETCH(QString, json);

    McuTargetDescription targetDescription{parseDescriptionJson(json.toLocal8Bit())};
    QCOMPARE(targetDescription.freeRTOS.package.cmakeVar, "");

    auto [targets, packages] = targetFactory.createTargets(targetDescription, sdkPackagePtr);

    auto freeRtos = findOrDefault(packages, [](const McuPackagePtr &pkg) {
        return (pkg->cmakeVariableName() == freeRtosCMakeVar);
    });
    QCOMPARE(freeRtos, nullptr);
}

void McuSupportTest::test_legacy_createQtMCUsPackage()
{
    EXPECT_CALL(*settingsMockPtr,
                getPath(Key{Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK}, _, _))
        .WillRepeatedly(Return(FilePath::fromUserInput(qtForMcuSdkPath)));

    McuPackagePtr qtForMCUsSDK = createQtForMCUsPackage(settingsMockPtr);

    QVERIFY(qtForMCUsSDK);
    QCOMPARE(qtForMCUsSDK->settingsKey(), Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK);
    QCOMPARE(qtForMCUsSDK->detectionPath(),
             FilePath::fromUserInput(Legacy::Constants::QT_FOR_MCUS_SDK_PACKAGE_VALIDATION_PATH)
                 .withExecutableSuffix());
    QCOMPARE(qtForMCUsSDK->path().toString(), qtForMcuSdkPath);
}

void McuSupportTest::test_legacy_supportMultipleToolchainVersions()
{
    const auto description = parseDescriptionJson(mulitple_toolchain_versions);

    QVERIFY(!description.toolchain.versions.empty());
    QCOMPARE(description.toolchain.versions.size(), 2);
}

void McuSupportTest::test_passExecutableVersionDetectorToToolchainPackage_data()
{
    const char version[]{"--version"};

    QTest::addColumn<QString>("json");
    QTest::addColumn<QString>("versionDetectionPath");
    QTest::addColumn<QString>("versionDetectionArgs");
    QTest::addColumn<QString>("versionRegex");
    QTest::newRow("armgcc_mimxrt1050_evk_freertos_json")
        << armgcc_mimxrt1050_evk_freertos_json << armGccSuffix << version
        << armGccVersionDetectionRegex;
    QTest::newRow("armgcc_stm32h750b_discovery_baremetal_json")
        << armgcc_stm32h750b_discovery_baremetal_json << armGccSuffix << version
        << armGccVersionDetectionRegex;
    QTest::newRow("armgcc_stm32f769i_discovery_freertos_json")
        << armgcc_stm32f769i_discovery_freertos_json << armGccSuffix << version
        << armGccVersionDetectionRegex;

    QTest::newRow("iar_stm32f469i_discovery_baremetal_json")
        << iar_stm32f469i_discovery_baremetal_json << HostOsInfo::withExecutableSuffix("bin/iccarm") << version
        << iarVersionDetectionRegex;
}

void McuSupportTest::test_passExecutableVersionDetectorToToolchainPackage()
{
    QFETCH(QString, json);
    QFETCH(QString, versionDetectionPath);
    QFETCH(QString, versionDetectionArgs);
    QFETCH(QString, versionRegex);
    const McuTargetDescription description = parseDescriptionJson(json.toLocal8Bit());

    QCOMPARE(description.toolchain.compiler.versionDetection.filePattern, versionDetectionPath);
    QCOMPARE(description.toolchain.compiler.versionDetection.executableArgs, versionDetectionArgs);
    QCOMPARE(description.toolchain.compiler.versionDetection.regex, versionRegex);

    McuToolChainPackagePtr toolchain{targetFactory.createToolchain(description.toolchain)};

    const auto *versionDetector{toolchain->getVersionDetector()};
    QVERIFY(versionDetector != nullptr);

    QCOMPARE(typeid(*versionDetector).name(), typeid(McuPackageExecutableVersionDetector).name());
}

void McuSupportTest::test_legacy_passExecutableVersionDetectorToToolchainPackage_data()
{
    test_passExecutableVersionDetectorToToolchainPackage_data();
}

void McuSupportTest::test_legacy_passExecutableVersionDetectorToToolchainPackage()
{
    QFETCH(QString, json);
    QFETCH(QString, versionDetectionPath);
    QFETCH(QString, versionDetectionArgs);
    QFETCH(QString, versionRegex);

    const McuTargetDescription description = parseDescriptionJson(json.toLocal8Bit());
    McuToolChainPackagePtr toolchain{targetFactory.createToolchain(description.toolchain)};

    const auto *versionDetector{toolchain->getVersionDetector()};
    QVERIFY(versionDetector != nullptr);

    QCOMPARE(typeid(*versionDetector).name(), typeid(McuPackageExecutableVersionDetector).name());
}

void McuSupportTest::test_legacy_passXMLVersionDetectorToNxpAndStmBoardSdkPackage_data()
{
    QTest::addColumn<QString>("json");
    QTest::addColumn<QString>("xmlElement");
    QTest::addColumn<QString>("xmlAttribute");
    QTest::addColumn<QString>("filePattern");

    QTest::newRow("armgcc_mimxrt1050_evk_freertos_json")
        << armgcc_mimxrt1050_evk_freertos_json << "ksdk"
        << "version"
        << "*_manifest_*.xml";
    QTest::newRow("armgcc_stm32h750b_discovery_baremetal_json")
        << armgcc_stm32h750b_discovery_baremetal_json << "PackDescription"
        << "Release"
        << "package.xml";
    QTest::newRow("armgcc_stm32f769i_discovery_freertos_json")
        << armgcc_stm32f769i_discovery_freertos_json << "PackDescription"
        << "Release"
        << "package.xml";
    QTest::newRow("iar_stm32f469i_discovery_baremetal_json")
        << iar_stm32f469i_discovery_baremetal_json << "PackDescription"
        << "Release"
        << "package.xml";
    QTest::newRow("iar_mimxrt1064_evk_freertos_json") << iar_mimxrt1064_evk_freertos_json << "ksdk"
                                                      << "version"
                                                      << "*_manifest_*.xml";
}

void McuSupportTest::test_legacy_passXMLVersionDetectorToNxpAndStmBoardSdkPackage()
{
    QFETCH(QString, json);
    QFETCH(QString, xmlElement);
    QFETCH(QString, xmlAttribute);
    QFETCH(QString, filePattern);

    const McuTargetDescription description = parseDescriptionJson(json.toLocal8Bit());
    McuPackagePtr boardSdk{Legacy::createBoardSdkPackage(settingsMockPtr, description)};

    const auto *versionDetector{boardSdk->getVersionDetector()};
    QVERIFY(versionDetector != nullptr);

    QCOMPARE(typeid(*versionDetector).name(), typeid(McuPackageXmlVersionDetector).name());
}

void McuSupportTest::test_passXMLVersionDetectorToNxpAndStmBoardSdkPackage_data()
{
    test_legacy_passXMLVersionDetectorToNxpAndStmBoardSdkPackage_data();
}

void McuSupportTest::test_passXMLVersionDetectorToNxpAndStmBoardSdkPackage()
{
    QFETCH(QString, json);
    QFETCH(QString, xmlElement);
    QFETCH(QString, xmlAttribute);
    QFETCH(QString, filePattern);

    const McuTargetDescription description = parseDescriptionJson(json.toLocal8Bit());

    QCOMPARE(description.boardSdk.versionDetection.xmlElement, xmlElement);
    QCOMPARE(description.boardSdk.versionDetection.xmlAttribute, xmlAttribute);
    QCOMPARE(description.boardSdk.versionDetection.filePattern, filePattern);

    McuPackagePtr boardSdk{targetFactory.createPackage(description.boardSdk)};

    const auto *versionDetector{boardSdk->getVersionDetector()};
    QVERIFY(versionDetector != nullptr);

    QCOMPARE(typeid(*versionDetector).name(), typeid(McuPackageXmlVersionDetector).name());
}

void McuSupportTest::test_passDirectoryVersionDetectorToRenesasBoardSdkPackage()
{
    const McuTargetDescription description = parseDescriptionJson(ghs_rh850_d1m1a_baremetal_json);

    QCOMPARE(description.boardSdk.versionDetection.regex, R"(\d+\.\d+\.\w+)");

    McuPackagePtr boardSdk{targetFactory.createPackage(description.boardSdk)};

    const auto *versionDetector{boardSdk->getVersionDetector()};
    QVERIFY(versionDetector != nullptr);
    QCOMPARE(typeid(*versionDetector).name(), typeid(McuPackagePathVersionDetector).name());
}

void McuSupportTest::test_resolveEnvironmentVariablesInDefaultPath()
{
    Utils::Environment::modifySystemEnvironment({{QUL_ENV_VAR, qtForMcuSdkPath}});
    QCOMPARE(qtcEnvironmentVariable(QUL_ENV_VAR), qtForMcuSdkPath);

    const QString qulEnvVariable = QString("%{Env:") + QUL_ENV_VAR + "}";
    const QString toolchainFileDefaultPath = qulEnvVariable + "/lib/cmake/Qul/toolchain/iar.cmake";
    const QString toolchainFilePath = QString{qtForMcuSdkPath}
                                      + "/lib/cmake/Qul/toolchain/iar.cmake";

    toochainFileDescription.defaultPath = FilePath::fromUserInput(toolchainFileDefaultPath);
    targetDescription.toolchain.file = toochainFileDescription;

    auto [targets, packages]{targetFactory.createTargets(targetDescription, sdkPackagePtr)};
    expandTargetsAndPackages(targets, packages);
    auto qtForMCUPkg = findOrDefault(packages, [](const McuPackagePtr &pkg) {
        return pkg->environmentVariableName() == QUL_ENV_VAR;
    });

    QVERIFY(qtForMCUPkg);
    QCOMPARE(qtForMCUPkg->path().toString(), qtForMcuSdkPath);

    auto toolchainFilePkg = findOrDefault(packages, [](const McuPackagePtr &pkg) {
        return (pkg->cmakeVariableName() == TOOLCHAIN_FILE_CMAKE_VARIABLE);
    });

    QVERIFY(toolchainFilePkg);
    QVERIFY(targets.size() == 1);

    QCOMPARE(toolchainFilePkg->path().toString(), toolchainFilePath);
    QVERIFY(toolchainFilePkg->path().toString().startsWith(qtForMcuSdkPath));
    QCOMPARE(toolchainFilePkg->defaultPath().toString(), toolchainFileDefaultPath);

    Utils::Environment::modifySystemEnvironment(
        {{QUL_ENV_VAR, qtForMcuSdkPath, EnvironmentItem::Unset}});
    QVERIFY(!qtcEnvironmentVariableIsSet(QUL_ENV_VAR));
}

void McuSupportTest::test_resolveCmakeVariablesInDefaultPath()
{
    const QString qulCmakeVariable = QString("%{") + QUL_CMAKE_VAR + "}";
    const QString toolchainFileDefaultPath = qulCmakeVariable
                                             + "/lib/cmake/Qul/toolchain/iar.cmake";
    const QString toolchainFilePath = QString{qtForMcuSdkPath}
                                      + "/lib/cmake/Qul/toolchain/iar.cmake";
    toochainFileDescription.defaultPath = FilePath::fromUserInput(toolchainFileDefaultPath);
    targetDescription.toolchain.file = toochainFileDescription;

    auto [targets, packages]{targetFactory.createTargets(targetDescription, sdkPackagePtr)};
    expandTargetsAndPackages(targets, packages);
    auto qtForMCUPkg = findOrDefault(packages, [](const McuPackagePtr &pkg) {
        return pkg->cmakeVariableName() == QUL_CMAKE_VAR;
    });

    QVERIFY(qtForMCUPkg);
    QCOMPARE(qtForMCUPkg->path().toString(), qtForMcuSdkPath);

    auto toolchainFilePkg = findOrDefault(packages, [](const McuPackagePtr &pkg) {
        return (pkg->cmakeVariableName() == TOOLCHAIN_FILE_CMAKE_VARIABLE);
    });

    QVERIFY(toolchainFilePkg);
    QVERIFY(targets.size() == 1);

    QCOMPARE(toolchainFilePkg->path().toString(), toolchainFilePath);
    QVERIFY(toolchainFilePkg->path().toString().startsWith(qtForMcuSdkPath));
    QCOMPARE(toolchainFilePkg->defaultPath().toString(), toolchainFileDefaultPath);
}

void McuSupportTest::test_legacy_createThirdPartyPackage_data()
{
    QTest::addColumn<PackageCreator>("creator");
    QTest::addColumn<QString>("json");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("defaultPath");
    QTest::addColumn<Key>("setting");
    QTest::addColumn<QString>("cmakeVar");
    QTest::addColumn<QString>("envVar");
    QTest::addColumn<QString>("label");
    QTest::addColumn<QString>("detectionPath");

    QTest::newRow("armgcc_mimxrt1050_evk_freertos_json mcuXpresso")
        << PackageCreator{[this]() { return Legacy::createMcuXpressoIdePackage(settingsMockPtr); }}
        << armgcc_mimxrt1050_evk_freertos_json << xpressoIdePath << xpressoIdePath
        << keyFromString(xpressoIdeSetting) << xpressoIdeCmakeVar << xpressoIdeEnvVar << xpressoIdeLabel
        << xpressoIdeDetectionPath;

    QTest::newRow("armgcc_mimxrt1064_evk_freertos_json mcuXpresso")
        << PackageCreator{[this]() { return Legacy::createMcuXpressoIdePackage(settingsMockPtr); }}
        << armgcc_mimxrt1064_evk_freertos_json << xpressoIdePath << xpressoIdePath
        << keyFromString(xpressoIdeSetting) << xpressoIdeCmakeVar << xpressoIdeEnvVar
        << xpressoIdeLabel << xpressoIdeDetectionPath;

    QTest::newRow("armgcc_mimxrt1170_evk_freertos_json mcuXpresso")
        << PackageCreator{[this]() { return Legacy::createMcuXpressoIdePackage(settingsMockPtr); }}
        << armgcc_mimxrt1170_evk_freertos_json << xpressoIdePath << xpressoIdePath
        << keyFromString(xpressoIdeSetting) << xpressoIdeCmakeVar << xpressoIdeEnvVar
        << xpressoIdeLabel << xpressoIdeDetectionPath;

    QTest::newRow("armgcc_stm32h750b_discovery_baremetal_json stmCubeProgrammer")
        << PackageCreator{[this]() {
               return Legacy::createStm32CubeProgrammerPackage(settingsMockPtr);
           }}
        << armgcc_stm32h750b_discovery_baremetal_json << stmCubeProgrammerPath
        << stmCubeProgrammerPath << keyFromString(stmCubeProgrammerSetting) << empty << empty
        << stmCubeProgrammerLabel << stmCubeProgrammerDetectionPath;
    QTest::newRow("armgcc_stm32f769i_discovery_freertos_json stmCubeProgrammer")
        << PackageCreator{[this]() {
               return Legacy::createStm32CubeProgrammerPackage(settingsMockPtr);
           }}
        << armgcc_stm32f769i_discovery_freertos_json << stmCubeProgrammerPath
        << stmCubeProgrammerPath << keyFromString(stmCubeProgrammerSetting) << empty << empty
        << stmCubeProgrammerLabel << stmCubeProgrammerDetectionPath;
    QTest::newRow("ghs_rh850_d1m1a_baremetal_json renesasProgrammer")
        << PackageCreator{[this]() {
               return Legacy::createRenesasProgrammerPackage(settingsMockPtr);
           }}
        << ghs_rh850_d1m1a_baremetal_json << empty << empty
        << keyFromString(renesasProgrammerSetting) << renesasProgrammerCmakeVar
        << renesasProgrammerEnvVar << renesasProgrammerLabel << renesasProgrammerDetectionPath;
}

void McuSupportTest::test_legacy_createThirdPartyPackage()
{
    QFETCH(PackageCreator, creator);
    QFETCH(QString, json);
    QFETCH(QString, path);
    QFETCH(QString, defaultPath);
    QFETCH(Key, setting);
    QFETCH(QString, cmakeVar);
    QFETCH(QString, envVar);
    QFETCH(QString, label);
    QFETCH(QString, detectionPath);

    EXPECT_CALL(*settingsMockPtr, getPath(Key{setting}, _, _))
        .Times(2)
        .WillRepeatedly(Return(FilePath::fromUserInput(defaultPath)));

    McuPackagePtr thirdPartyPackage{creator()};
    verifyPackage(thirdPartyPackage,
                  path,
                  defaultPath,
                  setting,
                  cmakeVar,
                  envVar,
                  label,
                  detectionPath,
                  {});
}

void McuSupportTest::test_createThirdPartyPackage_data()
{
    QTest::addColumn<QString>("json");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("defaultPath");
    QTest::addColumn<Key>("setting");
    QTest::addColumn<QString>("cmakeVar");
    QTest::addColumn<QString>("envVar");
    QTest::addColumn<QString>("label");
    QTest::addColumn<QString>("detectionPath");

    // Sometimes the jsons have different values than the legacy packages
    // Enter the expected values from the jsons here when they diverge from legacy values
    QString programFiles = qtcEnvironmentVariable("Env:PROGRAMFILES(x86)");
    const QString renesasProgrammerDefaultPath = {
        HostOsInfo::isWindowsHost()
            ? QString("%1/Renesas Electronics/Programming Tools/Renesas "
                      "Flash Programmer V3.09").arg(programFiles)
            : QString("")};

    QTest::newRow("armgcc_mimxrt1050_evk_freertos_json mcuXpresso")
        << armgcc_mimxrt1050_evk_freertos_json << xpressoIdePath << xpressoIdePath
        << keyFromString(xpressoIdeSetting) << xpressoIdeCmakeVar << xpressoIdeEnvVar
        << xpressoIdeLabel << xpressoIdeDetectionPath;

    QTest::newRow("armgcc_mimxrt1064_evk_freertos_json mcuXpresso")
        << armgcc_mimxrt1064_evk_freertos_json << xpressoIdePath << xpressoIdePath
        << keyFromString(xpressoIdeSetting) << xpressoIdeCmakeVar << xpressoIdeEnvVar
        << xpressoIdeLabel << xpressoIdeDetectionPath;

    QTest::newRow("armgcc_mimxrt1170_evk_freertos_json mcuXpresso")
        << armgcc_mimxrt1170_evk_freertos_json << xpressoIdePath << xpressoIdePath
        << keyFromString(xpressoIdeSetting) << xpressoIdeCmakeVar << xpressoIdeEnvVar << xpressoIdeLabel
        << xpressoIdeDetectionPath;

    QTest::newRow("armgcc_stm32h750b_discovery_baremetal_json stmCubeProgrammer")
        << armgcc_stm32h750b_discovery_baremetal_json << stmCubeProgrammerPath
        << stmCubeProgrammerPath << keyFromString(stmCubeProgrammerSetting) << empty << empty
        << stmCubeProgrammerLabel << stmCubeProgrammerDetectionPath;

    QTest::newRow("armgcc_stm32f769i_discovery_freertos_json stmCubeProgrammer")
        << armgcc_stm32f769i_discovery_freertos_json << stmCubeProgrammerPath
        << stmCubeProgrammerPath << keyFromString(stmCubeProgrammerSetting) << empty << empty
        << stmCubeProgrammerLabel << stmCubeProgrammerDetectionPath;

    QTest::newRow("ghs_rh850_d1m1a_baremetal_json renesasProgrammer")
        << ghs_rh850_d1m1a_baremetal_json << renesasProgrammerDefaultPath << empty
        << keyFromString("FlashProgrammerPath") << renesasProgrammerCmakeVar
        << "RenesasFlashProgrammer_PATH" << renesasProgrammerLabel
        << renesasProgrammerDetectionPath;
}

void McuSupportTest::test_createThirdPartyPackage()
{
    QFETCH(QString, json);
    QFETCH(QString, path);
    QFETCH(QString, defaultPath);
    QFETCH(Key, setting);
    QFETCH(QString, cmakeVar);
    QFETCH(QString, envVar);
    QFETCH(QString, label);
    QFETCH(QString, detectionPath);

    McuTargetDescription targetDescription{parseDescriptionJson(json.toLocal8Bit())};

    EXPECT_CALL(*settingsMockPtr, getPath(Key{setting}, QSettings::SystemScope, _))
        .Times(testing::AtMost(1))
        .WillOnce(Return(FilePath::fromUserInput(defaultPath)));

    EXPECT_CALL(*settingsMockPtr, getPath(Key{setting}, QSettings::UserScope, _))
        .Times(testing::AtMost(1))
        .WillOnce(Return(FilePath::fromUserInput(path)));

    auto [targets, packages] = targetFactory.createTargets(targetDescription, sdkPackagePtr);
    expandTargetsAndPackages(targets, packages);

    auto thirdPartyPackage = findOrDefault(packages, [&setting](const McuPackagePtr &pkg) {
        return (pkg->settingsKey() == setting);
    });

    verifyPackage(thirdPartyPackage,
                  path,
                  defaultPath,
                  setting,
                  cmakeVar,
                  envVar,
                  label,
                  detectionPath,
                  {});
}

void McuSupportTest::test_legacy_createCypressProgrammer3rdPartyPackage()
{
    EXPECT_CALL(*settingsMockPtr, getPath(Key{cypressProgrammerSetting}, _, _))
        .Times(2)
        .WillRepeatedly(Return(FilePath::fromUserInput(defaultToolPath)));

    McuPackagePtr thirdPartyPackage{Legacy::createCypressProgrammerPackage(settingsMockPtr)};
    verifyPackage(thirdPartyPackage,
                  defaultToolPath,
                  defaultToolPath,
                  cypressProgrammerSetting,
                  cypressProgrammerCmakeVar,
                  cypressProgrammerEnvVar,
                  cypressProgrammerLabel,
                  cypressProgrammerDetectionPath,
                  {});
}

void McuSupportTest::test_createJLink3rdPartyPackage()
{
    McuTargetDescription targetDescription{parseDescriptionJson(armgcc_ek_ra6m3g_baremetal_json)};

    EXPECT_CALL(*settingsMockPtr, getPath(Key{jlinkSetting}, QSettings::SystemScope, _))
        .Times(testing::AtMost(1))
        .WillOnce(Return(FilePath::fromUserInput(jlinkPath)));

    EXPECT_CALL(*settingsMockPtr, getPath(Key{jlinkSetting}, QSettings::UserScope, _))
        .Times(testing::AtMost(1))
        .WillOnce(Return(FilePath::fromUserInput(jlinkPath)));

    auto [targets, packages] = targetFactory.createTargets(targetDescription, sdkPackagePtr);
    expandTargetsAndPackages(targets, packages);

    auto thirdPartyPackage = findOrDefault(packages, [](const McuPackagePtr &pkg) {
        return (pkg->settingsKey() == jlinkSetting);
    });

    verifyPackage(thirdPartyPackage,
                  jlinkPath,
                  jlinkPath,
                  jlinkSetting,
                  jlinkCmakeVar,
                  jlinkEnvVar,
                  jlinkLabel,
                  jlinkDetectionPath,
                  {});
}

void McuSupportTest::test_differentValueForEachOperationSystem()
{
    const auto packageDescription = parseDescriptionJson(armgcc_mimxrt1050_evk_freertos_json);
    auto default_path_entry = packageDescription.platform.entries[0].defaultPath.toString();
    auto validation_path_entry = packageDescription.platform.entries[0].detectionPath.toString();

    //TODO: Revisit whether this test is required and not currently covered by the third party packages
    if (HostOsInfo::isWindowsHost()) {
        QCOMPARE(QString("%{Env:ROOT}/nxp/MCUXpressoIDE*"), default_path_entry);
        QCOMPARE(QString("ide/binaries/crt_emu_cm_redlink.exe"), validation_path_entry);
    } else {
        QCOMPARE(QString("/usr/local/mcuxpressoide"), default_path_entry);
        QCOMPARE(QString("ide/binaries/crt_emu_cm_redlink"), validation_path_entry);
    }
};
void McuSupportTest::test_addToSystemPathFlag()
{
    const auto targetDescription = parseDescriptionJson(armgcc_stm32f769i_discovery_freertos_json);

    const auto programmerPackage = targetDescription.platform.entries[0];
    const auto compilerPackage = targetDescription.toolchain.compiler;
    const auto toolchainFilePackage = targetDescription.toolchain.file;
    const auto boardSdkPackage = targetDescription.boardSdk;
    const auto freeRtosPackage = targetDescription.freeRTOS.package;

    QCOMPARE(programmerPackage.shouldAddToSystemPath, true);
    QCOMPARE(compilerPackage.shouldAddToSystemPath, false);
    QCOMPARE(toolchainFilePackage.shouldAddToSystemPath, false);
    QCOMPARE(boardSdkPackage.shouldAddToSystemPath, false);
    QCOMPARE(freeRtosPackage.shouldAddToSystemPath, false);
}

void McuSupportTest::test_processWildcards_data()
{
    QTest::addColumn<QString>("package_label");
    QTest::addColumn<QString>("expected_path");
    QTest::addColumn<QStringList>("paths");

    QTest::newRow("wildcard_at_the_end") << "FAKE_WILDCARD_TEST_1" << "folder-123" << QStringList {"folder-123/" };
    QTest::newRow("wildcard_in_th_middle") << "FAKE_WILDCARD_TEST_2" << "file-123.exe" << QStringList {"file-123.exe"};
    QTest::newRow("wildcard_at_the_end") << "FAKE_WILDCARD_TEST_3" << "123-file.exe" << QStringList( "123-file.exe");
    QTest::newRow("multi_wildcards")
        << "FAKE_WILDCARD_TEST_MULTI"
        << "2019/Community/VC/Tools/MSVC/14.29.30133/bin/Hostx64/x64"
        << QStringList{
               "2019/Community/VC/Tools/MSVC/14.29.30133/bin/Hostx64/x64/",
               "2019/Alpha/Beta/Gamma/",
               "2019/Community/VC/Tools/MSVC/",
               "2019/Community/VC/Tools/MSVC/14.29.30133/bin/",
               "2019/Community/VC/Tools/MSVC/14.29.30133/bin/Hostx64/",
               "2019/Enterprise/VC/Tools/MSVC/",
           };
}

void McuSupportTest::test_processWildcards()
{
    QFETCH(QString, package_label);
    QFETCH(QString, expected_path);
    QFETCH(QStringList, paths);

    for (const auto &path : paths)
        QVERIFY(createFakePath(testing_output_dir / "wildcards" / path, !path.endsWith("/")));

    auto [targets, packages] = createTestingKitTargetsAndPackages(wildcards_test_kit);
    auto testWildcardsPackage = findOrDefault(packages, [&](const McuPackagePtr &pkg) {
        return (pkg->label() == package_label);
    });
    QVERIFY(testWildcardsPackage != nullptr);
    QVERIFY(paths.size() > 0);
    // FilePaths with "/" at the end and without it evaluate to different paths.
    QCOMPARE(testWildcardsPackage->path(),
             FilePath(testing_output_dir / "wildcards" / expected_path));
}

void McuSupportTest::test_nonemptyVersionDetector()
{
    PackageDescription pkgDesc;
    pkgDesc.label = "GNU Arm Embedded Toolchain";
    pkgDesc.envVar = "ARMGCC_DIR";
    // pkgDesc.cmakeVar left empty
    // pkgDesc.description left empty
    pkgDesc.setting = "GNUArmEmbeddedToolchain";
    // pkgDesc.defaultPath left empty
    // pkgDesc.validationPath left empty
    // pkgDesc.versions left empty
    pkgDesc.versionDetection.filePattern = "bin/arm-none-eabi-g++";
    pkgDesc.versionDetection.regex = "\\b(\\d+\\.\\d+\\.\\d+)\\b";
    pkgDesc.versionDetection.executableArgs = "--version";
    // pkgDesc.versionDetection.xmlElement left empty
    // pkgDesc.versionDetection.xmlAttribute left empty
    pkgDesc.shouldAddToSystemPath = false;
    const auto package = targetFactory.createPackage(pkgDesc);
    const McuPackageVersionDetector *detector = package->getVersionDetector();
    QVERIFY(detector != nullptr);
    QCOMPARE(typeid(*detector).name(), typeid(McuPackageExecutableVersionDetector).name());
}

void McuSupportTest::test_emptyVersionDetector()
{
    PackageDescription pkgDesc;
    pkgDesc.label = "GNU Arm Embedded Toolchain";
    pkgDesc.envVar = "ARMGCC_DIR";
    // pkgDesc.cmakeVar left empty
    // pkgDesc.description left empty
    pkgDesc.setting = "GNUArmEmbeddedToolchain";
    // pkgDesc.defaultPath left empty
    // pkgDesc.validationPath left empty
    // pkgDesc.versions left empty
    // Version detection left completely empty
        // pkgDesc.versionDetection.filePattern
        // pkgDesc.versionDetection.regex
        // pkgDesc.versionDetection.executableArgs
        // pkgDesc.versionDetection.xmlElement
        // pkgDesc.versionDetection.xmlAttribute
    pkgDesc.shouldAddToSystemPath = false;

    const auto package = targetFactory.createPackage(pkgDesc);
    QVERIFY(package->getVersionDetector() == nullptr);
}

void McuSupportTest::test_emptyVersionDetectorFromJson()
{
    const auto targetDescription = parseDescriptionJson(armgcc_mimxrt1050_evk_freertos_json);
    auto [targets, packages]{targetFactory.createTargets(targetDescription, sdkPackagePtr)};
    expandTargetsAndPackages(targets, packages);

    auto freeRtos = findOrDefault(packages, [](const McuPackagePtr &pkg) {
        return (pkg->cmakeVariableName() == freeRtosCMakeVar);
    });

    QVERIFY2(!freeRtos->cmakeVariableName().isEmpty(), "The freeRTOS package was not found, but there should be one.");

    // For the FreeRTOS package there used to be no version check defined. Use this package to check if this was
    // considered correctly
    QVERIFY(freeRtos->getVersionDetector() == nullptr);
}

void McuSupportTest::test_expectedValueType()
{
    const auto targetDescription = parseDescriptionJson(armgcc_mimxrt1050_evk_freertos_json);

    QCOMPARE(targetDescription.toolchain.file.type, Utils::PathChooser::Kind::File);
    QCOMPARE(targetDescription.toolchain.compiler.type, Utils::PathChooser::Kind::ExistingDirectory);
    QCOMPARE(targetDescription.boardSdk.type, Utils::PathChooser::Kind::ExistingDirectory);
    QCOMPARE(targetDescription.freeRTOS.package.type, Utils::PathChooser::Kind::ExistingDirectory);
    QCOMPARE(targetDescription.platform.entries[0].type,
             Utils::PathChooser::Kind::ExistingDirectory);
}

} // namespace McuSupport::Internal::Test
