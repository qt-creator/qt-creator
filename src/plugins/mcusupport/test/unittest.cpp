// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "unittest.h"

#include "armgcc_nxp_1050_json.h"
#include "armgcc_nxp_1064_json.h"
#include "armgcc_nxp_mimxrt1170_evk_freertos_json.h"
#include "armgcc_stm32f769i_freertos_json.h"
#include "armgcc_stm32h750b_metal_json.h"
#include "gcc_desktop_json.h"
#include "ghs_rh850_d1m1a_baremetal_json.h"
#include "iar_nxp_1064_json.h"
#include "iar_stm32f469i_metal_json.h"
#include "msvc_desktop_json.h"

#include "mcuhelpers.h"
#include "mcukitmanager.h"
#include "mculegacyconstants.h"
#include "mcusupportconstants.h"
#include "mcusupportoptions.h"
#include "mcusupportsdk.h"
#include "mcusupportversiondetection.h"
#include "mcutargetdescription.h"
#include "mcutargetfactorylegacy.h"

#include <baremetal/baremetalconstants.h>
#include <cmakeprojectmanager/cmakeconfigitem.h>
#include <cmakeprojectmanager/cmakekitinformation.h>
#include <gmock/gmock-actions.h>
#include <gmock/gmock.h>

#include <projectexplorer/customtoolchain.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QtTest>

#include <algorithm>
#include <type_traits>

namespace McuSupport::Internal::Test {

using namespace Utils;
using Legacy::Constants::BOARD_SDK_CMAKE_VAR;
using Legacy::Constants::QT_FOR_MCUS_SDK_PACKAGE_VALIDATION_PATH;
using Legacy::Constants::QUL_CMAKE_VAR;
using Legacy::Constants::QUL_ENV_VAR;
using Legacy::Constants::QUL_LABEL;
using Legacy::Constants::SETTINGS_KEY_FREERTOS_PREFIX;
using Legacy::Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE;
using Legacy::Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE;

using CMakeProjectManager::CMakeConfigItem;
using CMakeProjectManager::CMakeConfigurationKitAspect;
using ProjectExplorer::EnvironmentKitAspect;
using ProjectExplorer::Kit;
using ProjectExplorer::KitManager;
using ProjectExplorer::ToolChain;
using ProjectExplorer::ToolChainManager;

using testing::_;
using testing::Return;

namespace {
const char empty[]{""};
const char armGcc[]{"armgcc"};
const char armGccVersionDetectionRegex[]{R"(\bv(\d+\.\d+\.\d+)\b)"};
const char armGccDir[]{"/opt/armgcc"};
const char armGccDirectorySetting[]{"GNUArmEmbeddedToolchain"};
const char armGccEnvVar[]{"ARMGCC_DIR"};
const char armGccLabel[]{"GNU Arm Embedded Toolchain"};
const char armGccSuffix[]{"bin/arm-none-eabi-g++"};
const char armGccToolchainFilePath[]{"/opt/toolchain/armgcc.cmake"};
const char armGccToolchainFileDefaultPath[]{
    "/opt/qtformcu/2.2/lib/cmake/Qul/toolchain/armgcc.cmake"};
const char armGccToolchainFilePathWithVariable[]{
    "%{Qul_ROOT}/lib/cmake/Qul/toolchain/armgcc.cmake"};
const char armGccVersion[]{"9.3.1"};
const char armGccNewVersion[]{"10.3.1"};
const char msvcVersion[]{"14.29"};
const char boardSdkVersion[]{"2.11.0"};
const QString boardSdkCmakeVar{Legacy::Constants::BOARD_SDK_CMAKE_VAR};
const char boardSdkDir[]{"/opt/Qul/2.3.0/boardDir/"};
const char cmakeToolchainLabel[]{"CMake Toolchain File"};
const char fallbackDir[]{"/abc/def/fallback"};
const char freeRtosCMakeVar[]{"FREERTOS_DIR"};
const char freeRtosEnvVar[]{"EVK_MIMXRT1170_FREERTOS_PATH"};
const char freeRtosLabel[]{"FreeRTOS directory"};
const char freeRtosPath[]{"/opt/freertos/default"};
const char freeRtosNxpPathSuffix[]{"rtos/freertos/freertos_kernel"};
const char freeRtosStmPathSuffix[]{"/Middlewares/Third_Party/FreeRTOS/Source"};
const char freeRtosSetting[]{"Freertos"};
const char greenhillToolchainFilePath[]{"/opt/toolchain/ghs.cmake"};
const char greenhillToolchainFileDefaultPath[]{
    "/opt/qtformcu/2.2/lib/cmake/Qul/toolchain/ghs.cmake"};
const char greenhillCompilerDir[]{"/abs/ghs"};
const char greenhillSetting[]{"GHSToolchain"};
const QStringList greenhillVersions{{"2018.1.5"}};
const char iarDir[]{"/opt/iar/compiler"};
const char iarEnvVar[]{"IAR_ARM_COMPILER_DIR"};
const char iarLabel[]{"IAR ARM Compiler"};
const char iarSetting[]{"IARToolchain"};
const char iarToolchainFilePath[]{"/opt/toolchain/iar.cmake"};
const char iarToolchainFileDefaultPath[]{"/opt/qtformcu/2.2/lib/cmake/Qul/toolchain/iar.cmake"};
const char iarVersionDetectionRegex[]{R"(\bV(\d+\.\d+\.\d+)\.\d+\b)"};
const QStringList iarVersions{{"8.50.9"}};
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
const char qulLabel[]{"Qt for MCUs SDK"};
const char stm32f7FreeRtosEnvVar[]{"STM32F7_FREERTOS_DIR"};
const char stm32f7[]{"STM32F7"};
const char unsupported[]{"unsupported"};
const char vendor[]{"target_vendor"};
const QString settingsPrefix = QLatin1String(Constants::SETTINGS_GROUP) + '/'
                               + QLatin1String(Constants::SETTINGS_KEY_PACKAGE_PREFIX);

const QString unsupportedToolchainFilePath = QString{qtForMcuSdkPath}
                                             + "/lib/cmake/Qul/toolchain/unsupported.cmake";

const QStringList jsonFiles{QString::fromUtf8(armgcc_nxp_1050_json),
                            QString::fromUtf8(iar_nxp_1064_json)};

const bool runLegacy{true};
const int colorDepth{32};

const PackageDescription qtForMCUsSDKDescription{
    Legacy::Constants::QUL_LABEL,
    QUL_ENV_VAR,
    QUL_CMAKE_VAR,
    Legacy::Constants::QUL_LABEL,
    Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK,
    qtForMcuSdkPath,
    Legacy::Constants::QT_FOR_MCUS_SDK_PACKAGE_VALIDATION_PATH,
};

const McuTargetDescription::Platform platformDescription{id,
                                                         "",
                                                         "",
                                                         {colorDepth},
                                                         McuTargetDescription::TargetType::MCU,
                                                         {qtForMCUsSDKDescription}};
const Id cxxLanguageId{ProjectExplorer::Constants::CXX_LANGUAGE_ID};
} // namespace

void verifyIarToolchain(const McuToolChainPackagePtr &iarToolchainPackage)
{
    ProjectExplorer::ToolChainFactory toolchainFactory;
    Id iarId{BareMetal::Constants::IAREW_TOOLCHAIN_TYPEID};
    ToolChain *iarToolchain{toolchainFactory.createToolChain(iarId)};
    iarToolchain->setLanguage(cxxLanguageId);
    ToolChainManager::instance()->registerToolChain(iarToolchain);

    QVERIFY(iarToolchainPackage != nullptr);
    QCOMPARE(iarToolchainPackage->cmakeVariableName(),
             Legacy::Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE);
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

    ToolChain *armToolchain{toolchainFactory.createToolChain(armGccId)};
    armToolchain->setLanguage(cxxLanguageId);
    ToolChainManager::instance()->registerToolChain(armToolchain);

    QVERIFY(armGccPackage != nullptr);
    QCOMPARE(armGccPackage->cmakeVariableName(), Legacy::Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE);
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

void verifyTargetToolchains(const Targets &targets,
                            const QString &toolchainFilePath,
                            const QString &compilerPath,
                            const QString &compilerSetting,
                            const QStringList &versions)
{
    QCOMPARE(targets.size(), 1);
    const auto &target{targets.first()};

    const auto toolchainFile{target->toolChainFilePackage()};
    QVERIFY(toolchainFile);
    QCOMPARE(toolchainFile->cmakeVariableName(), Legacy::Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE);
    QCOMPARE(toolchainFile->settingsKey(), empty);
    QCOMPARE(toolchainFile->path().toString(), toolchainFilePath);
    QCOMPARE(toolchainFile->defaultPath().toString(), toolchainFilePath);

    const auto toolchainCompiler{target->toolChainPackage()};
    QVERIFY(toolchainCompiler);
    QCOMPARE(toolchainCompiler->cmakeVariableName(),
             Legacy::Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE);
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
    QCOMPARE(boardSdk->settingsKey(), environmentVariable);
    QCOMPARE(boardSdk->detectionPath().toString(), empty);
    QCOMPARE(boardSdk->versions(), versions);
}

void verifyFreeRtosPackage(const McuPackagePtr &freeRtos,
                           const QString &envVar,
                           const FilePath &boardSdkDir,
                           const QString &freeRtosPath,
                           const QString &expectedSettingsKey)
{
    QVERIFY(freeRtos);
    QCOMPARE(freeRtos->environmentVariableName(), envVar);
    QCOMPARE(freeRtos->cmakeVariableName(), freeRtosCMakeVar);
    QCOMPARE(freeRtos->settingsKey(), expectedSettingsKey);
    QCOMPARE(freeRtos->path().cleanPath().toString(), freeRtosPath);
    QVERIFY(freeRtos->path().toString().startsWith(boardSdkDir.cleanPath().toString()));
}

McuSupportTest::McuSupportTest()
    : targetFactory{settingsMockPtr}
    , compilerDescription{armGccLabel, armGccEnvVar, Legacy::Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE, armGccLabel, armGccDirectorySetting, {}, {}, {}, {}, false}
     , toochainFileDescription{armGccLabel, armGccEnvVar, Legacy::Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE, armGccLabel, armGccDirectorySetting, {}, {}, {}, {}, false}
    , targetDescription {
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
                                                        Legacy::Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE,
                                                        armGccEnvVar,
                                                        nullptr}}
    , iarToolchainPackagePtr{new McuToolChainPackage{settingsMockPtr,
                                                     iarLabel,
                                                     iarDir,
                                                     {}, // validation path
                                                     iarSetting,
                                                     McuToolChainPackage::ToolChainType::IAR,
                                                     {armGccVersion},
                                                     Legacy::Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE,
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

    EXPECT_CALL(*sdkPackage, environmentVariableName()).WillRepeatedly(Return(QString{QUL_ENV_VAR}));
    EXPECT_CALL(*sdkPackage, cmakeVariableName()).WillRepeatedly(Return(QString{QUL_CMAKE_VAR}));
    EXPECT_CALL(*sdkPackage, isValidStatus()).WillRepeatedly(Return(true));
    EXPECT_CALL(*sdkPackage, path()).WillRepeatedly(Return(FilePath::fromUserInput(qtForMcuSdkPath)));
    EXPECT_CALL(*sdkPackage, isAddToSystemPath()).WillRepeatedly(Return(true));
    EXPECT_CALL(*sdkPackage, detectionPath()).WillRepeatedly(Return(FilePath{}));

    EXPECT_CALL(*armGccToolchainFilePackage, environmentVariableName())
        .WillRepeatedly(Return(QString{QString{}}));
    EXPECT_CALL(*armGccToolchainFilePackage, cmakeVariableName())
        .WillRepeatedly(Return(QString{Legacy::Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE}));
    EXPECT_CALL(*armGccToolchainFilePackage, isValidStatus()).WillRepeatedly(Return(true));
    EXPECT_CALL(*armGccToolchainFilePackage, path())
        .WillRepeatedly(Return(FilePath::fromUserInput(armGccToolchainFilePath)));
    EXPECT_CALL(*armGccToolchainFilePackage, isAddToSystemPath()).WillRepeatedly(Return(false));
    EXPECT_CALL(*armGccToolchainFilePackage, detectionPath()).WillRepeatedly(Return(FilePath{}));

    ON_CALL(*settingsMockPtr, getPath)
        .WillByDefault([](const QString &, QSettings::Scope, const FilePath &m_defaultPath) {
            return m_defaultPath;
        });
}

void McuSupportTest::init()
{
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
    const auto description = parseDescriptionJson(iar_nxp_1064_json);

    QVERIFY(!description.freeRTOS.envVar.isEmpty());
}

void McuSupportTest::test_parseCmakeEntries()
{
    const auto description{parseDescriptionJson(iar_nxp_1064_json)};

    auto &freeRtos = description.freeRTOS.package;
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
    QTest::newRow("armgcc_nxp_1050_json") << armgcc_nxp_1050_json << armGccEnvVar << armGccLabel
                                          << armGccToolchainFilePathWithVariable << armGcc;
    QTest::newRow("armgcc_stm32f769i_freertos_json")
        << armgcc_stm32f769i_freertos_json << armGccEnvVar << armGccLabel
        << armGccToolchainFilePathWithVariable << armGcc;

    QTest::newRow("armgcc_stm32h750b_metal_json")
        << armgcc_stm32h750b_metal_json << armGccEnvVar << armGccLabel
        << armGccToolchainFilePathWithVariable << armGcc;

    QTest::newRow("armgcc_nxp_mimxrt1170_evk_freertos_json")
        << armgcc_nxp_mimxrt1170_evk_freertos_json << armGccEnvVar << armGccLabel
        << armGccToolchainFilePathWithVariable << armGcc;

    QTest::newRow("iar_stm32f469i_metal_json")
        << iar_stm32f469i_metal_json << iarEnvVar << iarLabel << iarToolchainFileDefaultPath << iar;
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
    QCOMPARE(compilerPackage.cmakeVar, Legacy::Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE);
    QCOMPARE(compilerPackage.envVar, environmentVariable);

    const PackageDescription &toolchainFilePackage{description.toolchain.file};
    QCOMPARE(toolchainFilePackage.label, cmakeToolchainLabel);
    QCOMPARE(toolchainFilePackage.envVar, QString{});
    QCOMPARE(toolchainFilePackage.cmakeVar, Legacy::Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE);
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
    const auto description = parseDescriptionJson(iar_stm32f469i_metal_json);

    McuToolChainPackagePtr iarToolchainPackage{targetFactory.createToolchain(description.toolchain)};
    verifyIarToolchain(iarToolchainPackage);
}

void McuSupportTest::test_legacy_createDesktopGccToolchain()
{
    McuToolChainPackagePtr gccPackage = Legacy::createGccToolChainPackage(settingsMockPtr,
                                                                          {armGccNewVersion});
    verifyGccToolchain(gccPackage, {armGccNewVersion});
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
    QStringList versions{armGccVersion, armGccNewVersion};

    QTest::addColumn<QString>("json");
    QTest::newRow("armgcc_nxp_1050_json") << armgcc_nxp_1050_json;
    QTest::newRow("armgcc_stm32f769i_freertos_json") << armgcc_stm32f769i_freertos_json;
    QTest::newRow("armgcc_stm32h750b_metal_json") << armgcc_stm32h750b_metal_json;
    QTest::newRow("armgcc_nxp_mimxrt1170_evk_freertos_json")
        << armgcc_nxp_mimxrt1170_evk_freertos_json;
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

    QTest::newRow("armgcc_stm32h750b") << parseDescriptionJson(armgcc_stm32h750b_metal_json)
                                       << McuToolChainPackage::ToolChainType::ArmGcc;
    QTest::newRow("iar_nxp1064") << parseDescriptionJson(iar_nxp_1064_json)
                                 << McuToolChainPackage::ToolChainType::IAR;
    QTest::newRow("iar_stm32f469i") << parseDescriptionJson(iar_stm32f469i_metal_json)
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
    QTest::addColumn<QSet<QString>>("expectedSettings");

    QSet<QString> commonSettings{{"CypressAutoFlashUtil"},
                                 {"MCUXpressoIDE"},
                                 {"RenesasFlashProgrammer"},
                                 {"Stm32CubeProgrammer"}};

    QTest::newRow("nxp1064") << iar_nxp_1064_json
                             << QSet<QString>{{"EVK_MIMXRT1064_SDK_PATH"},
                                              {QString{
                                                  Legacy::Constants::SETTINGS_KEY_FREERTOS_PREFIX}
                                                   .append("IMXRT1064")},
                                              "IARToolchain"}
                                    .unite(commonSettings);
    QTest::newRow("stm32f469i") << iar_stm32f469i_metal_json
                                << QSet<QString>{{"STM32Cube_FW_F4_SDK_PATH"}, "IARToolchain"}.unite(
                                       commonSettings);
    QTest::newRow("nxp1050") << armgcc_nxp_1050_json
                             << QSet<QString>{{"EVKB_IMXRT1050_SDK_PATH"},
                                              {QString{
                                                  Legacy::Constants::SETTINGS_KEY_FREERTOS_PREFIX}
                                                   .append("IMXRT1050")},
                                              "GNUArmEmbeddedToolchain"}
                                    .unite(commonSettings);
    QTest::newRow("stm32h750b") << armgcc_stm32h750b_metal_json
                                << QSet<QString>{{"STM32Cube_FW_H7_SDK_PATH"},
                                                 "GNUArmEmbeddedToolchain"}
                                       .unite(commonSettings);
    QTest::newRow("stm32f769i") << armgcc_stm32f769i_freertos_json
                                << QSet<QString>{{"STM32Cube_FW_F7_SDK_PATH"},
                                                 "GNUArmEmbeddedToolchain"}
                                       .unite(commonSettings);

    QTest::newRow("ghs_rh850_d1m1a_baremetal_json")
        << ghs_rh850_d1m1a_baremetal_json << QSet<QString>{"GHSToolchain"}.unite(commonSettings);
}

void McuSupportTest::test_legacy_createPackagesWithCorrespondingSettings()
{
    QFETCH(QString, json);
    const McuTargetDescription description = parseDescriptionJson(json.toLocal8Bit());
    const auto [targets, packages]{
        targetsFromDescriptions({description}, settingsMockPtr, qtForMcuSdkPath, runLegacy)};
    Q_UNUSED(targets);

    QSet<QString> settings = transform<QSet<QString>>(packages, [](const auto &package) {
        return package->settingsKey();
    });
    QFETCH(QSet<QString>, expectedSettings);
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
                                          "",
                                          {},
                                          VersionDetection{},
                                          true};
    targetDescription.toolchain.id = armGcc;

    const auto [targets, packages]{targetFactory.createTargets(targetDescription, qtForMcuSdkPath)};
    QCOMPARE(targets.size(), 1);
    const McuTargetPtr target{targets.at(0)};

    QCOMPARE(target->colorDepth(), colorDepth);

    const auto &tgtPackages{target->packages()};
    QCOMPARE(tgtPackages.size(), 5);

    // target should contain freertos package
    QVERIFY(anyOf(tgtPackages, [](const McuPackagePtr &pkg) {
        return (pkg->environmentVariableName() == nxp1064FreeRtosEnvVar
                && pkg->cmakeVariableName() == freeRtosCMakeVar && pkg->label() == id);
    }));

    // all packages should contain target's tooclhain compiler package.
    QVERIFY(anyOf(packages, [](const McuPackagePtr &pkg) {
        return (pkg->cmakeVariableName() == Legacy::Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE);
    }));

    // target should contain tooclhain copmiler package.
    QVERIFY(anyOf(tgtPackages, [](const McuPackagePtr &pkg) {
        return (pkg->cmakeVariableName()
                == Legacy::Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE /*and pkg->disconnect*/);
    }));

    // all packages should contain target's tooclhain file package.
    QVERIFY(anyOf(packages, [](const McuPackagePtr &pkg) {
        return (pkg->cmakeVariableName() == Legacy::Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE);
    }));

    // target should contain tooclhain file package.
    QVERIFY(anyOf(tgtPackages, [](const McuPackagePtr &pkg) {
        return (pkg->cmakeVariableName()
                == Legacy::Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE /*and pkg->disconnect*/);
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
                                          "",
                                          {},
                                          VersionDetection{},
                                          true};

    const auto packages{targetFactory.createPackages(targetDescription)};
    QVERIFY(!packages.empty());
}

void McuSupportTest::test_removeRtosSuffixFromEnvironmentVariable_data()
{
    QTest::addColumn<QString>("freeRtosEnvVar");
    QTest::addColumn<QString>("expectedEnvVarWithoutSuffix");

    QTest::newRow("armgcc_nxp_1050_json") << nxp1050FreeRtosEnvVar << nxp1050;
    QTest::newRow("iar_nxp_1064_json") << nxp1064FreeRtosEnvVar << nxp1064;
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
                                           Legacy::Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE,
                                           armGcc,
                                           armGccDirectorySetting,
                                           fallbackDir,
                                           {},
                                           {},
                                           VersionDetection{},
                                           false};
    McuTargetDescription::Toolchain toolchainDescription{armGcc, {}, compilerDescription, {}};

    EXPECT_CALL(*settingsMockPtr, getPath(QString{armGccDirectorySetting}, _, FilePath{fallbackDir}))
        .Times(2)
        .WillRepeatedly(Return(FilePath{fallbackDir}));

    McuToolChainPackage *toolchain = targetFactory.createToolchain(toolchainDescription);

    QCOMPARE(toolchain->path().toString(), fallbackDir);
}

void McuSupportTest::test_usePathFromSettingsForToolchainPath()
{
    PackageDescription compilerDescription{{},
                                           armGccEnvVar,
                                           Legacy::Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE,
                                           armGcc,
                                           armGccDirectorySetting,
                                           empty,
                                           {},
                                           {},
                                           VersionDetection{},
                                           false};
    McuTargetDescription::Toolchain toolchainDescription{armGcc, {}, compilerDescription, {}};

    EXPECT_CALL(*settingsMockPtr, getPath(QString{armGccDirectorySetting}, _, FilePath{empty}))
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
    QVERIFY(
        indexOf(config.toVector(),
                [&cmakeVar](const CMakeConfigItem &item) { return item.key == cmakeVar.toUtf8(); })
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
    QCOMPARE(unsupportedToolchainFile->cmakeVariableName(),
             Legacy::Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE);
}

void McuSupportTest::test_legacy_createTargetWithToolchainPackages_data()
{
    QTest::addColumn<QString>("json");
    QTest::addColumn<QString>("toolchainFilePath");
    QTest::addColumn<QString>("compilerPath");
    QTest::addColumn<QString>("compilerSetting");
    QTest::addColumn<QStringList>("versions");

    QTest::newRow("nxp1050") << armgcc_nxp_1050_json << armGccToolchainFileDefaultPath << armGccDir
                             << armGccDirectorySetting
                             << QStringList{armGccVersion, armGccNewVersion};
    QTest::newRow("stm32h750b") << armgcc_stm32h750b_metal_json << armGccToolchainFileDefaultPath
                                << armGccDir << armGccDirectorySetting
                                << QStringList{armGccVersion};
    QTest::newRow("stm32f769i") << armgcc_stm32f769i_freertos_json << armGccToolchainFileDefaultPath
                                << armGccDir << armGccDirectorySetting
                                << QStringList{armGccVersion, armGccNewVersion};
    QTest::newRow("stm32f469i") << iar_stm32f469i_metal_json << iarToolchainFileDefaultPath
                                << iarDir << iarSetting << iarVersions;
    QTest::newRow("iar_nxp_1064_json")
        << iar_nxp_1064_json << iarToolchainFileDefaultPath << iarDir << iarSetting << iarVersions;
    QTest::newRow("ghs_rh850_d1m1a_baremetal_json")
        << ghs_rh850_d1m1a_baremetal_json << greenhillToolchainFileDefaultPath
        << greenhillCompilerDir << greenhillSetting << greenhillVersions;
}

void McuSupportTest::test_legacy_createTargetWithToolchainPackages()
{
    QFETCH(QString, json);
    QFETCH(QString, toolchainFilePath);
    QFETCH(QString, compilerPath);
    QFETCH(QString, compilerSetting);
    QFETCH(QStringList, versions);

    const McuTargetDescription description = parseDescriptionJson(json.toLocal8Bit());

    EXPECT_CALL(*settingsMockPtr,
                getPath(QString{Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK}, _, _))
        .WillRepeatedly(Return(FilePath::fromUserInput(qtForMcuSdkPath)));
    EXPECT_CALL(*settingsMockPtr, getPath(compilerSetting, _, _))
        .WillRepeatedly(Return(FilePath::fromUserInput(compilerPath)));

    const auto [targets, packages]{
        targetsFromDescriptions({description}, settingsMockPtr, qtForMcuSdkPath, runLegacy)};
    Q_UNUSED(packages);

    verifyTargetToolchains(targets, toolchainFilePath, compilerPath, compilerSetting, versions);
}

void McuSupportTest::test_createTargetWithToolchainPackages_data()
{
    test_legacy_createTargetWithToolchainPackages_data();
}

void McuSupportTest::test_createTargetWithToolchainPackages()
{
    QFETCH(QString, json);
    QFETCH(QString, toolchainFilePath);
    QFETCH(QString, compilerPath);
    QFETCH(QString, compilerSetting);
    QFETCH(QStringList, versions);

    EXPECT_CALL(*settingsMockPtr, getPath(compilerSetting, _, _))
        .WillRepeatedly(Return(FilePath::fromUserInput(compilerPath)));

    EXPECT_CALL(*settingsMockPtr,
                getPath(QString{Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK}, _, _))
        .WillRepeatedly(Return(FilePath::fromUserInput(qtForMcuSdkPath)));

    const McuTargetDescription description = parseDescriptionJson(json.toLocal8Bit());
    const auto [targets, packages]{
        targetsFromDescriptions({description}, settingsMockPtr, qtForMcuSdkPath, !runLegacy)};
    Q_UNUSED(packages);

    verifyTargetToolchains(targets, toolchainFilePath, compilerPath, compilerSetting, versions);
}

void McuSupportTest::test_addToolchainFileInfoToKit()
{
    McuKitManager::upgradeKitInPlace(&kit, &mcuTarget, sdkPackagePtr);

    QVERIFY(kit.isValid());
    QCOMPARE(kit.value(Constants::KIT_MCUTARGET_TOOLCHAIN_KEY).toString(), armGcc);

    const auto &cmakeConfig{CMakeConfigurationKitAspect::configuration(&kit)};
    QVERIFY(!cmakeConfig.empty());
    QCOMPARE(cmakeConfig.valueOf(Legacy::Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE),
             armGccToolchainFilePath);
}

void McuSupportTest::test_legacy_createBoardSdk_data()
{
    QTest::addColumn<QString>("json");
    QTest::addColumn<QString>("cmakeVariable");
    QTest::addColumn<QString>("environmentVariable");
    QTest::addColumn<QStringList>("versions");

    QTest::newRow("armgcc_nxp_1050_json")
        << armgcc_nxp_1050_json << boardSdkCmakeVar << "EVKB_IMXRT1050_SDK_PATH"
        << QStringList{boardSdkVersion};

    QTest::newRow("armgcc_nxp_1064_json") << armgcc_nxp_1064_json << boardSdkCmakeVar
                                          << "EVK_MIMXRT1064_SDK_PATH" << QStringList{"2.11.1"};
    QTest::newRow("stm32h750b") << armgcc_stm32h750b_metal_json << boardSdkCmakeVar
                                << "STM32Cube_FW_H7_SDK_PATH" << QStringList{"1.5.0"};
    QTest::newRow("stm32f769i") << armgcc_stm32f769i_freertos_json << boardSdkCmakeVar
                                << "STM32Cube_FW_F7_SDK_PATH" << QStringList{"1.16.0"};
    QTest::newRow("stm32f469i") << iar_stm32f469i_metal_json << boardSdkCmakeVar
                                << "STM32Cube_FW_F4_SDK_PATH" << QStringList{"1.25.0"};
    QTest::newRow("nxp1064") << iar_nxp_1064_json << boardSdkCmakeVar << "EVK_MIMXRT1064_SDK_PATH"
                             << QStringList{boardSdkVersion};
    QTest::newRow("ghs_rh850_d1m1a_baremetal_json")
        << ghs_rh850_d1m1a_baremetal_json << boardSdkCmakeVar << "RGL_DIR" << QStringList{"2.0.0a"};
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
    QTest::addColumn<QString>("expectedSettingsKey");
    QTest::addColumn<FilePath>("expectedPath");

    QTest::newRow("armgcc_nxp_1050_json")
        << armgcc_nxp_1050_json << QStringList{boardSdkVersion}
        << QString{Legacy::Constants::SETTINGS_KEY_FREERTOS_PREFIX}.append(nxp1050)
        << FilePath::fromUserInput(boardSdkDir) / freeRtosNxpPathSuffix;
    QTest::newRow("armgcc_nxp_1064_json")
        << armgcc_nxp_1064_json << QStringList{boardSdkVersion}
        << QString{Legacy::Constants::SETTINGS_KEY_FREERTOS_PREFIX}.append(nxp1064)
        << FilePath::fromUserInput(boardSdkDir) / freeRtosNxpPathSuffix;
    QTest::newRow("iar_nxp_1064_json")
        << iar_nxp_1064_json << QStringList{boardSdkVersion}
        << QString{Legacy::Constants::SETTINGS_KEY_FREERTOS_PREFIX}.append(nxp1064)
        << FilePath::fromUserInput(boardSdkDir) / freeRtosNxpPathSuffix;
    QTest::newRow("armgcc_stm32f769i_freertos_json")
        << armgcc_stm32f769i_freertos_json << QStringList{"1.16.0"}
        << QString{Legacy::Constants::SETTINGS_KEY_FREERTOS_PREFIX}.append(stm32f7)
        << FilePath::fromUserInput(boardSdkDir) / freeRtosStmPathSuffix;
}

void McuSupportTest::test_legacy_createFreeRtosPackage()
{
    QFETCH(QString, json);
    QFETCH(QStringList, versions);
    QFETCH(QString, expectedSettingsKey);
    QFETCH(FilePath, expectedPath);

    McuTargetDescription targetDescription{parseDescriptionJson(json.toLocal8Bit())};

    EXPECT_CALL(*settingsMockPtr, getPath(expectedSettingsKey, _, _))
        .WillRepeatedly(Return(FilePath::fromString(expectedPath.toUserOutput())));
    McuPackagePtr freeRtos{Legacy::createFreeRTOSSourcesPackage(settingsMockPtr,
                                                                targetDescription.freeRTOS.envVar,
                                                                FilePath{})};

    verifyFreeRtosPackage(freeRtos,
                          targetDescription.freeRTOS.envVar,
                          boardSdkDir,
                          expectedPath.toUserOutput(),
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
    QFETCH(QString, expectedSettingsKey);
    QFETCH(FilePath, expectedPath);

    McuTargetDescription targetDescription{parseDescriptionJson(json.toLocal8Bit())};

    EXPECT_CALL(*settingsMockPtr, getPath(targetDescription.boardSdk.envVar, _, _))
        .WillRepeatedly(Return(FilePath::fromString(boardSdkDir)));

    auto [targets, packages] = targetFactory.createTargets(targetDescription, qtForMcuSdkPath);

    auto freeRtos = findOrDefault(packages, [](const McuPackagePtr &pkg) {
        return (pkg->cmakeVariableName() == freeRtosCMakeVar);
    });

    verifyFreeRtosPackage(freeRtos,
                          targetDescription.freeRTOS.envVar,
                          boardSdkDir,
                          expectedPath.toUserOutput(),
                          expectedSettingsKey);
}

void McuSupportTest::test_legacy_doNOTcreateFreeRtosPackageForMetalVariants_data()

{
    QTest::addColumn<QString>("json");
    QTest::newRow("iar_stm32f469i_metal_json") << iar_stm32f469i_metal_json;
    QTest::newRow("armgcc_stm32h750b_metal_json") << armgcc_stm32h750b_metal_json;
    QTest::newRow("ghs_rh850_d1m1a_baremetal_json") << ghs_rh850_d1m1a_baremetal_json;
    QTest::newRow("gcc_desktop_json") << gcc_desktop_json;
}

void McuSupportTest::test_legacy_doNOTcreateFreeRtosPackageForMetalVariants()
{
    QFETCH(QString, json);

    McuTargetDescription targetDescription{parseDescriptionJson(json.toLocal8Bit())};
    QCOMPARE(targetDescription.freeRTOS.package.cmakeVar, "");

    auto [targets, packages] = targetFactory.createTargets(targetDescription, qtForMcuSdkPath);

    auto freeRtos = findOrDefault(packages, [](const McuPackagePtr &pkg) {
        return (pkg->cmakeVariableName() == freeRtosCMakeVar);
    });
    QCOMPARE(freeRtos, nullptr);
}

void McuSupportTest::test_legacy_createQtMCUsPackage()
{
    EXPECT_CALL(*settingsMockPtr,
                getPath(QString{Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK}, _, _))
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
    const auto description = parseDescriptionJson(armgcc_stm32f769i_freertos_json);

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
    QTest::newRow("armgcc_nxp_1050_json")
        << armgcc_nxp_1050_json << armGccSuffix << version << armGccVersionDetectionRegex;
    QTest::newRow("armgcc_stm32h750b_metal_json")
        << armgcc_stm32h750b_metal_json << armGccSuffix << version << armGccVersionDetectionRegex;
    QTest::newRow("armgcc_stm32f769i_freertos_json")
        << armgcc_stm32f769i_freertos_json << armGccSuffix << version
        << armGccVersionDetectionRegex;

    QTest::newRow("iar_stm32f469i_metal_json") << iar_stm32f469i_metal_json << QString{"bin/iccarm"}
                                               << version << iarVersionDetectionRegex;
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

    QTest::newRow("armgcc_nxp_1050_json") << armgcc_nxp_1050_json << "ksdk"
                                          << "version"
                                          << "*_manifest_*.xml";
    QTest::newRow("armgcc_stm32h750b_metal_json")
        << armgcc_stm32h750b_metal_json << "PackDescription"
        << "Release"
        << "package.xml";
    QTest::newRow("armgcc_stm32f769i_freertos_json")
        << armgcc_stm32f769i_freertos_json << "PackDescription"
        << "Release"
        << "package.xml";
    QTest::newRow("iar_stm32f469i_metal_json") << iar_stm32f469i_metal_json << "PackDescription"
                                               << "Release"
                                               << "package.xml";
    QTest::newRow("iar_nxp_1064_json") << iar_nxp_1064_json << "ksdk"
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

    QCOMPARE(description.boardSdk.versionDetection.filePattern, "rgl_*_obj_*");
    QCOMPARE(description.boardSdk.versionDetection.regex, R"(\d+\.\d+\.\w+)");
    QCOMPARE(description.boardSdk.versionDetection.isFile, false);

    McuPackagePtr boardSdk{targetFactory.createPackage(description.boardSdk)};

    const auto *versionDetector{boardSdk->getVersionDetector()};
    QVERIFY(versionDetector != nullptr);
    QCOMPARE(typeid(*versionDetector).name(), typeid(McuPackageDirectoryVersionDetector).name());
}

void McuSupportTest::test_resolveEnvironmentVariablesInDefaultPath()
{
    QVERIFY(qputenv(QUL_ENV_VAR, qtForMcuSdkPath));
    QCOMPARE(qEnvironmentVariable(QUL_ENV_VAR), qtForMcuSdkPath);

    const QString qulEnvVariable = QString("%{Env:") + QUL_ENV_VAR + "}";
    toochainFileDescription.defaultPath = FilePath::fromUserInput(
        qulEnvVariable + "/lib/cmake/Qul/toolchain/iar.cmake");
    targetDescription.toolchain.file = toochainFileDescription;

    auto [targets, packages] = targetFactory.createTargets(targetDescription, qtForMcuSdkPath);
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

    QString expectedPkgPath = QString{qtForMcuSdkPath} + "/lib/cmake/Qul/toolchain/iar.cmake";
    QCOMPARE(toolchainFilePkg->path().toString(), expectedPkgPath);
    QVERIFY(toolchainFilePkg->path().toString().startsWith(qtForMcuSdkPath));
    QCOMPARE(toolchainFilePkg->defaultPath().toString(), expectedPkgPath);

    QVERIFY(qunsetenv(QUL_ENV_VAR));
}

void McuSupportTest::test_resolveCmakeVariablesInDefaultPath()
{
    const QString qulCmakeVariable = QString("%{") + QUL_CMAKE_VAR + "}";
    toochainFileDescription.defaultPath = FilePath::fromUserInput(
        qulCmakeVariable + "/lib/cmake/Qul/toolchain/iar.cmake");
    targetDescription.toolchain.file = toochainFileDescription;

    auto [targets, packages] = targetFactory.createTargets(targetDescription, qtForMcuSdkPath);
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

    QString expectedPkgPath = QString{qtForMcuSdkPath} + "/lib/cmake/Qul/toolchain/iar.cmake";
    QCOMPARE(toolchainFilePkg->path().toString(), expectedPkgPath);
    QVERIFY(toolchainFilePkg->path().toString().startsWith(qtForMcuSdkPath));
    QCOMPARE(toolchainFilePkg->defaultPath().toString(), expectedPkgPath);
}

void McuSupportTest::test_legacy_createThirdPartyPackage_data()
{
    const QString defaultToolPath{"/opt/biz/foo"};

    const char xpressoIdeSetting[]{"MCUXpressoIDE"};
    const char xpressoIdeCmakeVar[]{"MCUXPRESSO_IDE_PATH"};
    const char xpressoIdeEnvVar[]{"MCUXpressoIDE_PATH"};

    const char stmCubeProgrammerSetting[]{"Stm32CubeProgrammer"};
    const QString stmCubeProgrammerPath{defaultToolPath + "/bin"};

    const char renesasProgrammerSetting[]{"RenesasFlashProgrammer"};
    const char renesasProgrammerCmakeVar[]{"RENESAS_FLASH_PROGRAMMER_PATH"};
    const QString renesasProgrammerEnvVar{renesasProgrammerCmakeVar};

    const char cypressProgrammerSetting[]{"CypressAutoFlashUtil"};
    const char cypressProgrammerCmakeVar[]{"INFINEON_AUTO_FLASH_UTILITY_DIR"};
    const char cypressProgrammerEnvVar[]{"CYPRESS_AUTO_FLASH_UTILITY_DIR"};

    QTest::addColumn<PackageCreator>("creator");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("defaultPath");
    QTest::addColumn<QString>("setting");
    QTest::addColumn<QString>("cmakeVar");
    QTest::addColumn<QString>("envVar");

    QTest::newRow("mcuXpresso") << PackageCreator{[this]() {
        return Legacy::createMcuXpressoIdePackage(settingsMockPtr);
    }} << defaultToolPath << defaultToolPath
                                << xpressoIdeSetting << xpressoIdeCmakeVar << xpressoIdeEnvVar;
    QTest::newRow("stmCubeProgrammer") << PackageCreator{[this]() {
        return Legacy::createStm32CubeProgrammerPackage(settingsMockPtr);
    }} << stmCubeProgrammerPath << defaultToolPath
                                       << stmCubeProgrammerSetting << empty << empty;

    QTest::newRow("renesasProgrammer") << PackageCreator{[this]() {
        return Legacy::createRenesasProgrammerPackage(settingsMockPtr);
    }} << defaultToolPath << defaultToolPath
                                       << renesasProgrammerSetting << renesasProgrammerCmakeVar
                                       << renesasProgrammerEnvVar;

    QTest::newRow("cypressProgrammer") << PackageCreator{[this]() {
        return Legacy::createCypressProgrammerPackage(settingsMockPtr);
    }} << defaultToolPath << defaultToolPath
                                       << cypressProgrammerSetting << cypressProgrammerCmakeVar
                                       << cypressProgrammerEnvVar;
}

void McuSupportTest::test_legacy_createThirdPartyPackage()
{
    QFETCH(PackageCreator, creator);
    QFETCH(QString, path);
    QFETCH(QString, defaultPath);
    QFETCH(QString, setting);
    QFETCH(QString, cmakeVar);
    QFETCH(QString, envVar);

    if (!envVar.isEmpty())
        QVERIFY(qputenv(envVar.toLocal8Bit(), defaultPath.toLocal8Bit()));

    EXPECT_CALL(*settingsMockPtr, getPath(QString{setting}, _, _))
        .Times(2)
        .WillRepeatedly(Return(FilePath::fromUserInput(defaultPath)));

    McuPackagePtr thirdPartyPacakge{creator()};
    QVERIFY(thirdPartyPacakge);
    QCOMPARE(thirdPartyPacakge->settingsKey(), setting);
    QCOMPARE(thirdPartyPacakge->environmentVariableName(), envVar);
    QCOMPARE(thirdPartyPacakge->path().toString(), path);

    if (!envVar.isEmpty())
        QVERIFY(qunsetenv(envVar.toLocal8Bit()));
}

} // namespace McuSupport::Internal::Test
