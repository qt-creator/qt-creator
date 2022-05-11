/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "unittest.h"

#include "armgcc_nxp_1050_json.h"
#include "armgcc_nxp_mimxrt1170_evk_freertos_json.h"
#include "armgcc_stm32f769i_freertos_json.h"
#include "armgcc_stm32h750b_metal_json.h"
#include "gcc_desktop_json.h"
#include "ghs_rh850_d1m1a_baremetal_json.h"
#include "iar_nxp_1064_json.h"
#include "iar_stm32f469i_metal_json.h"

#include "mcuhelpers.h"
#include "mcukitmanager.h"
#include "mculegacyconstants.h"
#include "mcusupportconstants.h"
#include "mcusupportoptions.h"
#include "mcusupportsdk.h"
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
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>
#include <utils/algorithm.h>
#include <utils/filepath.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QtTest>

#include <algorithm>

namespace McuSupport::Internal::Test {

namespace {
const char empty[]{""};
const char armGccDir[]{"/opt/armgcc"};
const char armGccDirectorySetting[]{"GNUArmEmbeddedToolchain"};
const char armGccEnvVar[]{"ARMGCC_DIR"};
const char armGccLabel[]{"GNU Arm Embedded Toolchain"};
const char armGccToolchainFilePath[]{"/opt/qtformcu/2.2/lib/cmake/Qul/toolchain/armgcc.cmake"};
const char armGcc[]{"armgcc"};
const char cmakeToolchainLabel[]{"CMake Toolchain File"};
const char fallbackDir[]{"/abc/def/fallback"};
const char freeRtosCMakeVar[]{"FREERTOS_DIR"};
const char freeRtosDescription[]{"Freertos directory"};
const char freeRtosEnvVar[]{"EVK_MIMXRT1170_FREERTOS_PATH"};
const char freeRtosLabel[]{"FreeRTOS directory"};
const char freeRtosPath[]{"/opt/freertos/default"};
const char freeRtosSetting[]{"Freertos"};
const char greenhillToolchainFilePath[]{"/opt/qtformcu/2.2/lib/cmake/Qul/toolchain/ghs.cmake"};
const char greenhillCompilerDir[]{"/abs/ghs"};
const char greenhillSetting[]{"GHSToolchain"};
const char iarDir[]{"/opt/iar/compiler"};
const char iarEnvVar[]{"IAR_ARM_COMPILER_DIR"};
const char iarLabel[]{"IAR ARM Compiler"};
const char iarSetting[]{"IARToolchain"};
const char iarToolchainFilePath[]{"/opt/qtformcu/2.2/lib/cmake/Qul/toolchain/iar.cmake"};
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
const char qulCmakeVar[]{"Qul_ROOT"};
const char qulEnvVar[]{"Qul_DIR"};
const char stm32f7FreeRtosEnvVar[]{"STM32F7_FREERTOS_DIR"};
const char stm32f7[]{"STM32F7"};
const char unsupported[]{"unsupported"};
const char vendor[]{"target_vendor"};
const QString settingsPrefix = QLatin1String(Constants::SETTINGS_GROUP) + '/'
                               + QLatin1String(Constants::SETTINGS_KEY_PACKAGE_PREFIX);
const char qmlToCppSuffixPath[]{"bin/qmltocpp"};

const QString unsupportedToolchainFilePath = QString{qtForMcuSdkPath}
                                             + "/lib/cmake/Qul/toolchain/unsupported.cmake";

const QStringList jsonFiles{QString::fromUtf8(armgcc_nxp_1050_json),
                            QString::fromUtf8(iar_nxp_1064_json)};

const bool runLegacy{true};
const int colorDepth{32};

const Sdk::McuTargetDescription::Platform
    platformDescription{id, "", "", {colorDepth}, Sdk::McuTargetDescription::TargetType::MCU};
const Utils::Id cxxLanguageId{ProjectExplorer::Constants::CXX_LANGUAGE_ID};
} // namespace

using namespace Utils;

using CMakeProjectManager::CMakeConfigItem;
using CMakeProjectManager::CMakeConfigurationKitAspect;
using ProjectExplorer::EnvironmentKitAspect;
using ProjectExplorer::Kit;
using ProjectExplorer::KitManager;
using ProjectExplorer::ToolChain;
using ProjectExplorer::ToolChainManager;

using testing::_;
using testing::Return;

void verifyIarToolchain(const McuToolChainPackagePtr &iarToolchainPackage)
{
    ProjectExplorer::ToolChainFactory toolchainFactory;
    Id iarId{BareMetal::Constants::IAREW_TOOLCHAIN_TYPEID};
    ToolChain *iarToolchain{toolchainFactory.createToolChain(iarId)};
    iarToolchain->setLanguage(cxxLanguageId);
    ToolChainManager::instance()->registerToolChain(iarToolchain);

    QVERIFY(iarToolchainPackage != nullptr);
    QCOMPARE(iarToolchainPackage->cmakeVariableName(), Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE);
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

void verifyArmGccToolchain(const McuToolChainPackagePtr &armGccPackage)
{
    //Fake register and fake detect compiler.
    ProjectExplorer::ToolChainFactory toolchainFactory;
    Id armGccId{ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID};

    ToolChain *armToolchain{toolchainFactory.createToolChain(armGccId)};
    armToolchain->setLanguage(cxxLanguageId);
    ToolChainManager::instance()->registerToolChain(armToolchain);

    QVERIFY(armGccPackage != nullptr);
    QCOMPARE(armGccPackage->cmakeVariableName(), Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE);
    QCOMPARE(armGccPackage->environmentVariableName(), armGccEnvVar);
    QCOMPARE(armGccPackage->isDesktopToolchain(), false);
    QCOMPARE(armGccPackage->toolChainName(), armGcc);
    QCOMPARE(armGccPackage->toolchainType(), McuToolChainPackage::ToolChainType::ArmGcc);
    QCOMPARE(armGccPackage->settingsKey(), armGccDirectorySetting);

    // FIXME(piotr.mucko): Re-enable when toolchains retrieval from McuToolChainPackage is unified for arm and iar.
    // armToolchain = armGccPackage->toolChain(cxxLanguageId);
    // QVERIFY(armToolchain != nullptr);
}

void verifyGccToolchain(const McuToolChainPackagePtr &gccPackage)
{
    QVERIFY(gccPackage != nullptr);
    QCOMPARE(gccPackage->cmakeVariableName(), "");
    QCOMPARE(gccPackage->environmentVariableName(), "");
    QCOMPARE(gccPackage->isDesktopToolchain(), true);
    QCOMPARE(gccPackage->toolChainName(), unsupported);
    QCOMPARE(gccPackage->toolchainType(), McuToolChainPackage::ToolChainType::GCC);
}

void verifyTargetToolchains(const Targets &targets,
                            const QString &toolchainFilePath,
                            const QString &compilerPath,
                            const QString &compilerSetting)
{
    QCOMPARE(targets.size(), 1);
    const auto &target{targets.first()};

    const auto toolchainFile{target->toolChainFilePackage()};
    QVERIFY(toolchainFile);
    QCOMPARE(toolchainFile->cmakeVariableName(), Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE);
    QCOMPARE(toolchainFile->settingsKey(), empty);
    QCOMPARE(toolchainFile->path().toString(), toolchainFilePath);

    const auto toolchainCompiler{target->toolChainPackage()};
    QVERIFY(toolchainCompiler);
    QCOMPARE(toolchainCompiler->cmakeVariableName(), Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE);
    QCOMPARE(toolchainCompiler->path().toString(), compilerPath);
    QCOMPARE(toolchainCompiler->settingsKey(), compilerSetting);
}

McuSupportTest::McuSupportTest()
    : targetFactory{settingsMockPtr}
    , toolchainPackagePtr{new McuToolChainPackage{
          settingsMockPtr,
          {},                                              // label
          {},                                              // defaultPath
          {},                                              // detectionPath
          {},                                              // settingsKey
          McuToolChainPackage::ToolChainType::Unsupported, // toolchain type
          {},                                              // cmake var name
          {}}}                                             // env var name
    , armGccToolchainPackagePtr{new McuToolChainPackage{settingsMockPtr,
                                                        armGccLabel,
                                                        armGccDir,
                                                        {}, // validation path
                                                        armGccDirectorySetting,
                                                        McuToolChainPackage::ToolChainType::ArmGcc,
                                                        Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE,
                                                        armGccEnvVar}}
    , iarToolchainPackagePtr{new McuToolChainPackage{settingsMockPtr,
                                                     iarLabel,
                                                     iarDir,
                                                     {}, // validation path
                                                     iarSetting,
                                                     McuToolChainPackage::ToolChainType::IAR,
                                                     Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE,
                                                     iarEnvVar}}
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
    targetDescription = Sdk::McuTargetDescription{
        "2.0.1",
        "2",
        platformDescription,
        Sdk::McuTargetDescription::Toolchain{},
        Sdk::McuTargetDescription::BoardSdk{},
        Sdk::McuTargetDescription::FreeRTOS{},
    };

    EXPECT_CALL(*freeRtosPackage, environmentVariableName())
        .WillRepeatedly(Return(QString{freeRtosEnvVar}));
    EXPECT_CALL(*freeRtosPackage, cmakeVariableName())
        .WillRepeatedly(Return(QString{freeRtosCMakeVar}));
    EXPECT_CALL(*freeRtosPackage, isValidStatus()).WillRepeatedly(Return(true));
    EXPECT_CALL(*freeRtosPackage, path()).WillRepeatedly(Return(FilePath::fromString(freeRtosPath)));
    EXPECT_CALL(*freeRtosPackage, isAddToSystemPath()).WillRepeatedly(Return(true));
    EXPECT_CALL(*freeRtosPackage, detectionPath()).WillRepeatedly(Return(FilePath{}));

    EXPECT_CALL(*sdkPackage, environmentVariableName()).WillRepeatedly(Return(QString{qulEnvVar}));
    EXPECT_CALL(*sdkPackage, cmakeVariableName()).WillRepeatedly(Return(QString{qulCmakeVar}));
    EXPECT_CALL(*sdkPackage, isValidStatus()).WillRepeatedly(Return(true));
    EXPECT_CALL(*sdkPackage, path()).WillRepeatedly(Return(FilePath::fromString(qtForMcuSdkPath)));
    EXPECT_CALL(*sdkPackage, isAddToSystemPath()).WillRepeatedly(Return(true));
    EXPECT_CALL(*sdkPackage, detectionPath()).WillRepeatedly(Return(FilePath{}));

    EXPECT_CALL(*armGccToolchainFilePackage, environmentVariableName())
        .WillRepeatedly(Return(QString{QString{}}));
    EXPECT_CALL(*armGccToolchainFilePackage, cmakeVariableName())
        .WillRepeatedly(Return(QString{Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE}));
    EXPECT_CALL(*armGccToolchainFilePackage, isValidStatus()).WillRepeatedly(Return(true));
    EXPECT_CALL(*armGccToolchainFilePackage, path())
        .WillRepeatedly(Return(FilePath::fromString(armGccToolchainFilePath)));
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
    const auto description = Sdk::parseDescriptionJson(iar_nxp_1064_json);

    QVERIFY(!description.freeRTOS.envVar.isEmpty());
    QVERIFY(description.freeRTOS.boardSdkSubDir.isEmpty());
}

void McuSupportTest::test_parseCmakeEntries()
{
    const auto description{Sdk::parseDescriptionJson(iar_nxp_1064_json)};

    QVERIFY(!description.freeRTOS.packages.isEmpty());
    auto &freeRtos = description.freeRTOS.packages[0];
    QCOMPARE(freeRtos.envVar, nxp1064FreeRtosEnvVar);
}

void McuSupportTest::test_parseToolchainFromJSON_data()
{
    QTest::addColumn<QString>("json");
    QTest::addColumn<QString>("environmentVariable");
    QTest::addColumn<QString>("label");
    QTest::addColumn<QString>("toolchainFile");
    QTest::addColumn<QString>("id");

    //TODO(me): Add ghs nxp 1064 nxp 1070.
    QTest::newRow("armgcc_nxp_1050_json")
        << armgcc_nxp_1050_json << armGccEnvVar << armGccLabel << armGccToolchainFilePath << armGcc;
    QTest::newRow("armgcc_stm32f769i_freertos_json")
        << armgcc_stm32f769i_freertos_json << armGccEnvVar << armGccLabel << armGccToolchainFilePath
        << armGcc;

    QTest::newRow("armgcc_stm32h750b_metal_json")
        << armgcc_stm32h750b_metal_json << armGccEnvVar << armGccLabel << armGccToolchainFilePath
        << armGcc;

    QTest::newRow("armgcc_nxp_mimxrt1170_evk_freertos_json")
        << armgcc_nxp_mimxrt1170_evk_freertos_json << armGccEnvVar << armGccLabel
        << armGccToolchainFilePath << armGcc;

    QTest::newRow("iar_stm32f469i_metal_json")
        << iar_stm32f469i_metal_json << iarEnvVar << iarLabel << iarToolchainFilePath << iar;
}

void McuSupportTest::test_parseToolchainFromJSON()
{
    QFETCH(QString, json);
    QFETCH(QString, environmentVariable);
    QFETCH(QString, label);
    QFETCH(QString, toolchainFile);
    QFETCH(QString, id);
    Sdk::McuTargetDescription description{Sdk::parseDescriptionJson(json.toLocal8Bit())};
    QCOMPARE(description.toolchain.id, id);

    const Sdk::PackageDescription &compilerPackage{description.toolchain.compiler};
    QCOMPARE(compilerPackage.cmakeVar, Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE);
    QCOMPARE(compilerPackage.envVar, environmentVariable);

    const Sdk::PackageDescription &toolchainFilePackage{description.toolchain.file};
    QCOMPARE(toolchainFilePackage.label, cmakeToolchainLabel);
    QCOMPARE(toolchainFilePackage.envVar, QString{});
    QCOMPARE(toolchainFilePackage.cmakeVar, Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE);
    QCOMPARE(toolchainFilePackage.defaultPath.cleanPath().toString(), toolchainFile);
}

void McuSupportTest::test_legacy_createIarToolchain()
{
    McuToolChainPackagePtr iarToolchainPackage = Sdk::createIarToolChainPackage(settingsMockPtr);
    verifyIarToolchain(iarToolchainPackage);
}

void McuSupportTest::test_createIarToolchain()
{
    const auto description = Sdk::parseDescriptionJson(iar_stm32f469i_metal_json);

    McuToolChainPackagePtr iarToolchainPackage{targetFactory.createToolchain(description.toolchain)};
    verifyIarToolchain(iarToolchainPackage);
}

void McuSupportTest::test_legacy_createDesktopGccToolchain()
{
    McuToolChainPackagePtr gccPackage = Sdk::createGccToolChainPackage(settingsMockPtr);
    verifyGccToolchain(gccPackage);
}

void McuSupportTest::test_createDesktopGccToolchain()
{
    const auto description = Sdk::parseDescriptionJson(gcc_desktop_json);
    McuToolChainPackagePtr gccPackage{targetFactory.createToolchain(description.toolchain)};
    verifyGccToolchain(gccPackage);
}

void McuSupportTest::test_verifyManuallyCreatedArmGccToolchain()
{
    verifyArmGccToolchain(armGccToolchainPackagePtr);
}

void McuSupportTest::test_legacy_createArmGccToolchain()
{
    McuToolChainPackagePtr armGccPackage = Sdk::createArmGccToolchainPackage(settingsMockPtr);
    verifyArmGccToolchain(armGccPackage);
}

void McuSupportTest::test_createArmGccToolchain_data()
{
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

    const auto description = Sdk::parseDescriptionJson(json.toLocal8Bit());
    McuToolChainPackagePtr armGccPackage{targetFactory.createToolchain(description.toolchain)};
    verifyArmGccToolchain(armGccPackage);
}

void McuSupportTest::test_mapParsedToolchainIdToCorrespondingType_data()
{
    QTest::addColumn<Sdk::McuTargetDescription>("description");
    QTest::addColumn<McuToolChainPackage::ToolChainType>("toolchainType");

    QTest::newRow("armgcc_stm32h750b") << Sdk::parseDescriptionJson(armgcc_stm32h750b_metal_json)
                                       << McuToolChainPackage::ToolChainType::ArmGcc;
    QTest::newRow("iar_nxp1064") << Sdk::parseDescriptionJson(iar_nxp_1064_json)
                                 << McuToolChainPackage::ToolChainType::IAR;
    QTest::newRow("iar_stm32f469i") << Sdk::parseDescriptionJson(iar_stm32f469i_metal_json)
                                    << McuToolChainPackage::ToolChainType::IAR;
}

void McuSupportTest::test_mapParsedToolchainIdToCorrespondingType()
{
    QFETCH(Sdk::McuTargetDescription, description);
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
                                              {QString{Constants::SETTINGS_KEY_FREERTOS_PREFIX}
                                                   .append("IMXRT1064")},
                                              "IARToolchain"}
                                    .unite(commonSettings);
    QTest::newRow("stm32f469i") << iar_stm32f469i_metal_json
                                << QSet<QString>{{"STM32Cube_FW_F4_SDK_PATH"}, "IARToolchain"}.unite(
                                       commonSettings);
    QTest::newRow("nxp1050") << armgcc_nxp_1050_json
                             << QSet<QString>{{"EVKB_IMXRT1050_SDK_PATH"},
                                              {QString{Constants::SETTINGS_KEY_FREERTOS_PREFIX}
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
    const Sdk::McuTargetDescription description = Sdk::parseDescriptionJson(json.toLocal8Bit());
    const auto [targets, packages]{
        Sdk::targetsFromDescriptions({description}, settingsMockPtr, qtForMcuSdkPath, runLegacy)};
    Q_UNUSED(targets);

    QSet<QString> settings = transform<QSet<QString>>(packages, [](const auto &package) {
        return package->settingsKey();
    });
    QFETCH(QSet<QString>, expectedSettings);
    QVERIFY(settings.contains(expectedSettings));
}

void McuSupportTest::test_createFreeRtosPackageWithCorrectSetting_data()
{
    QTest::addColumn<QString>("freeRtosEnvVar");
    QTest::addColumn<QString>("expectedSettingsKey");

    QTest::newRow("nxp1050") << nxp1050FreeRtosEnvVar
                             << QString{Constants::SETTINGS_KEY_FREERTOS_PREFIX}.append(nxp1050);
    QTest::newRow("nxp1064") << nxp1064FreeRtosEnvVar
                             << QString{Constants::SETTINGS_KEY_FREERTOS_PREFIX}.append(nxp1064);
    QTest::newRow("nxp1170") << nxp1170FreeRtosEnvVar
                             << QString{Constants::SETTINGS_KEY_FREERTOS_PREFIX}.append(nxp1170);
    QTest::newRow("stm32f7") << stm32f7FreeRtosEnvVar
                             << QString{Constants::SETTINGS_KEY_FREERTOS_PREFIX}.append(stm32f7);
}

void McuSupportTest::test_createFreeRtosPackageWithCorrectSetting()
{
    QFETCH(QString, freeRtosEnvVar);
    QFETCH(QString, expectedSettingsKey);

    McuPackagePtr package{
        Sdk::createFreeRTOSSourcesPackage(settingsMockPtr, freeRtosEnvVar, FilePath{}, FilePath{})};
    QVERIFY(package != nullptr);

    QCOMPARE(package->settingsKey(), expectedSettingsKey);
}

void McuSupportTest::test_createTargets()
{
    Sdk::PackageDescription packageDescription{id,
                                               nxp1064FreeRtosEnvVar,
                                               freeRtosCMakeVar,
                                               freeRtosSetting,
                                               freeRtosDescription,
                                               freeRtosPath,
                                               "",
                                               {},
                                               true};
    targetDescription.freeRTOS.packages.append(packageDescription);
    targetDescription.toolchain.id = armGcc;

    const auto [targets, packages]{targetFactory.createTargets(targetDescription, qtForMcuSdkPath)};
    QVERIFY(!targets.empty());
    const McuTargetPtr target{targets.at(0)};
    QCOMPARE(target->colorDepth(), colorDepth);
    const auto &tgtPackages{target->packages()};
    QVERIFY(!tgtPackages.empty());

    QVERIFY(anyOf(tgtPackages, [](const McuPackagePtr &pkg) {
        return (pkg->environmentVariableName() == nxp1064FreeRtosEnvVar
                && pkg->cmakeVariableName() == freeRtosCMakeVar && pkg->label() == id);
    }));
}

void McuSupportTest::test_createPackages()
{
    Sdk::PackageDescription packageDescription{id,
                                               nxp1064FreeRtosEnvVar,
                                               freeRtosCMakeVar,
                                               freeRtosLabel,
                                               freeRtosSetting,
                                               freeRtosPath,
                                               "",
                                               {},
                                               true};
    targetDescription.freeRTOS.packages.append(packageDescription);

    const auto packages{targetFactory.createPackages(targetDescription)};
    QVERIFY(!packages.empty());
}

void McuSupportTest::test_removeRtosSuffixFromEnvironmentVariable_data()
{
    QTest::addColumn<QString>("freeRtosEnvVar");
    QTest::addColumn<QString>("expectedEnvVarWithoutSuffix");

    QTest::newRow("nxp1050") << nxp1050FreeRtosEnvVar << nxp1050;
    QTest::newRow("nxp1064") << nxp1064FreeRtosEnvVar << nxp1064;
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
    QCOMPARE(McuSupportOptions::isLegacyVersion({2, 2, 0}), false);
    QCOMPARE(McuSupportOptions::isLegacyVersion({2, 2, 1}), false);
}
void McuSupportTest::test_useFallbackPathForToolchainWhenPathFromSettingsIsNotAvailable()
{
    Sdk::PackageDescription compilerDescription{armGcc,
                                                armGccEnvVar,
                                                Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE,
                                                armGcc,
                                                armGccDirectorySetting,
                                                fallbackDir,
                                                {},
                                                {},
                                                false};
    Sdk::McuTargetDescription::Toolchain toolchainDescription{armGcc, {}, compilerDescription, {}};

    EXPECT_CALL(*settingsMockPtr, getPath(QString{armGccDirectorySetting}, _, FilePath{fallbackDir}))
        .Times(2)
        .WillRepeatedly(Return(FilePath{fallbackDir}));

    McuToolChainPackage *toolchain = targetFactory.createToolchain(toolchainDescription);

    QCOMPARE(toolchain->path().toString(), fallbackDir);
}

void McuSupportTest::test_usePathFromSettingsForToolchainPath()
{
    Sdk::PackageDescription compilerDescription{{},
                                                armGccEnvVar,
                                                Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE,
                                                armGcc,
                                                armGccDirectorySetting,
                                                empty,
                                                {},
                                                {},
                                                false};
    Sdk::McuTargetDescription::Toolchain toolchainDescription{armGcc, {}, compilerDescription, {}};

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
    QVERIFY(config.size() > 0);
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
    QHash<QString, ToolchainCompilerCreator> toolchainCreators{
        {armGcc, [this] { return armGccToolchainPackagePtr; }}};
    McuTargetFactoryLegacy legacyTargetFactory{toolchainCreators,
                                               {{armGcc, armGccToolchainFilePackagePtr}},
                                               {},
                                               settingsMockPtr};
    auto armToolchainFile = legacyTargetFactory.getToolchainFile(qtForMcuSdkPath, armGcc);
    QVERIFY(armToolchainFile);
    QCOMPARE(armToolchainFile, armGccToolchainFilePackagePtr);
}

void McuSupportTest::test_legacy_createUnsupportedToolchainFilePackage()
{
    QHash<QString, ToolchainCompilerCreator> toolchainCreators{
        {armGcc, [this] { return armGccToolchainPackagePtr; }}};
    McuTargetFactoryLegacy legacyTargetFactory{toolchainCreators,
                                               {{armGcc, armGccToolchainFilePackagePtr}},
                                               {},
                                               settingsMockPtr};

    auto unsupportedToolchainFile = legacyTargetFactory.getToolchainFile(qtForMcuSdkPath, iar);
    QVERIFY(unsupportedToolchainFile);
    QCOMPARE(unsupportedToolchainFile->path().toString(), unsupportedToolchainFilePath);
    QCOMPARE(unsupportedToolchainFile->cmakeVariableName(),
             Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE);
}

void McuSupportTest::test_legacy_createTargetWithToolchainPackages_data()
{
    QTest::addColumn<QString>("json");
    QTest::addColumn<QString>("toolchainFilePath");
    QTest::addColumn<QString>("compilerPath");
    QTest::addColumn<QString>("compilerSetting");

    QTest::newRow("nxp1050") << armgcc_nxp_1050_json << armGccToolchainFilePath << armGccDir
                             << armGccDirectorySetting;
    QTest::newRow("stm32h750b") << armgcc_stm32h750b_metal_json << armGccToolchainFilePath
                                << armGccDir << armGccDirectorySetting;
    QTest::newRow("stm32f769i") << armgcc_stm32f769i_freertos_json << armGccToolchainFilePath
                                << armGccDir << armGccDirectorySetting;
    QTest::newRow("stm32f469i") << iar_stm32f469i_metal_json << iarToolchainFilePath << iarDir
                                << iarSetting;
    QTest::newRow("nxp1064") << iar_nxp_1064_json << iarToolchainFilePath << iarDir << iarSetting;
    QTest::newRow("nxp1064") << iar_nxp_1064_json << iarToolchainFilePath << iarDir << iarSetting;
    QTest::newRow("ghs_rh850_d1m1a_baremetal_json")
        << ghs_rh850_d1m1a_baremetal_json << greenhillToolchainFilePath << greenhillCompilerDir
        << greenhillSetting;
}

void McuSupportTest::test_legacy_createTargetWithToolchainPackages()
{
    QFETCH(QString, json);
    QFETCH(QString, toolchainFilePath);
    QFETCH(QString, compilerPath);
    QFETCH(QString, compilerSetting);

    const Sdk::McuTargetDescription description = Sdk::parseDescriptionJson(json.toLocal8Bit());

    EXPECT_CALL(*settingsMockPtr,
                getPath(QString{Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK}, _, _))
        .WillRepeatedly(Return(FilePath::fromString(qtForMcuSdkPath)));
    EXPECT_CALL(*settingsMockPtr, getPath(compilerSetting, _, _))
        .WillRepeatedly(Return(FilePath::fromString(compilerPath)));

    const auto [targets, packages]{
        Sdk::targetsFromDescriptions({description}, settingsMockPtr, qtForMcuSdkPath, runLegacy)};
    Q_UNUSED(packages);

    verifyTargetToolchains(targets, toolchainFilePath, compilerPath, compilerSetting);
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

    EXPECT_CALL(*settingsMockPtr, getPath(compilerSetting, _, _))
        .WillRepeatedly(Return(FilePath::fromString(compilerPath)));

    const Sdk::McuTargetDescription description = Sdk::parseDescriptionJson(json.toLocal8Bit());
    const auto [targets, packages]{
        Sdk::targetsFromDescriptions({description}, settingsMockPtr, qtForMcuSdkPath, !runLegacy)};
    Q_UNUSED(packages);

    verifyTargetToolchains(targets, toolchainFilePath, compilerPath, compilerSetting);
}

void McuSupportTest::test_addToolchainFileInfoToKit()
{
    McuKitManager::upgradeKitInPlace(&kit, &mcuTarget, sdkPackagePtr);

    QVERIFY(kit.isValid());
    QCOMPARE(kit.value(Constants::KIT_MCUTARGET_TOOLCHAIN_KEY).toString(), armGcc);

    const auto &cmakeConfig{CMakeConfigurationKitAspect::configuration(&kit)};
    QVERIFY(!cmakeConfig.empty());
    QCOMPARE(cmakeConfig.valueOf(Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE), armGccToolchainFilePath);
}

} // namespace McuSupport::Internal::Test
