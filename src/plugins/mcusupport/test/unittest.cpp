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
#include "armgcc_stm32f769i_freertos_json.h"
#include "armgcc_stm32h750b_metal_json.h"
#include "gcc_desktop_json.h"
#include "iar_nxp_1064_json.h"
#include "iar_stm32f469i_metal_json.h"

#include "mcuhelpers.h"
#include "mcukitmanager.h"
#include "mcusupportconstants.h"
#include "mcusupportsdk.h"
#include "mcutargetdescription.h"
#include "mcutargetfactory.h"
#include "mcutargetfactorylegacy.h"

#include <baremetal/baremetalconstants.h>
#include <cmakeprojectmanager/cmakeconfigitem.h>
#include <cmakeprojectmanager/cmakekitinformation.h>
#include <gmock/gmock-actions.h>
#include <gmock/gmock.h>
#include <projectexplorer/customtoolchain.h>
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
const char id[]{"target_id"};
const char name[]{"target_name"};
const char vendor[]{"target_vendor"};

const char armgcc[]{"armgcc"};
const char cmakeExtension[]{".cmake"};
const char defaultfreeRtosPath[]{"/opt/freertos/default"};
const char freeRtosCMakeVar[]{"FREERTOS_DIR"};
const char freeRtosEnvVar[]{"EVK_MIMXRT1170_FREERTOS_PATH"};
const char gcc[]{"armgcc"};
const char iarEnvVar[]{"IAR_ARM_COMPILER_DIR"};
const char iarLabel[]{"IAR ARM Compiler"};
const char iarSetting[]{"IARToolchain"};
const char iar[]{"iar"};
const char nxp1050FreeRtosEnvVar[]{"IMXRT1050_FREERTOS_DIR"};
const char nxp1050[]{"IMXRT1050"};
const char nxp1064FreeRtosEnvVar[]{"IMXRT1064_FREERTOS_DIR"};
const char nxp1064[]{"IMXRT1064"};
const char nxp1170FreeRtosEnvVar[]{"EVK_MIMXRT1170_FREERTOS_PATH"};
const char nxp1170[]{"EVK_MIMXRT1170"};
const char stm32f7FreeRtosEnvVar[]{"STM32F7_FREERTOS_DIR"};
const char stm32f7[]{"STM32F7"};
const char unsupported[]{"unsupported"};
const char cmakeToolchainLabel[]{"CMake Toolchain File"};

const QStringList jsonFiles{QString::fromUtf8(armgcc_nxp_1050_json),
                            QString::fromUtf8(iar_nxp_1064_json)};

const bool runLegacy{true};
const int colorDepth{32};

const Sdk::McuTargetDescription::Platform platform{id,
                                                   "",
                                                   "",
                                                   {colorDepth},
                                                   Sdk::McuTargetDescription::TargetType::MCU};
const Utils::Id cxxLanguageId{ProjectExplorer::Constants::CXX_LANGUAGE_ID};
} // namespace

using CMakeProjectManager::CMakeConfigItem;
using CMakeProjectManager::CMakeConfigurationKitAspect;
using ProjectExplorer::EnvironmentKitAspect;
using ProjectExplorer::Kit;
using ProjectExplorer::KitManager;
using ProjectExplorer::ToolChain;
using ProjectExplorer::ToolChainManager;

using testing::Return;
using Utils::FilePath;

void verifyIarToolchain(const McuToolChainPackage *iarToolchainPackage)
{
    QVERIFY(iarToolchainPackage != nullptr);
    QCOMPARE(iarToolchainPackage->cmakeToolChainFileName(), QString{iar}.append(cmakeExtension));
    QCOMPARE(iarToolchainPackage->cmakeVariableName(), Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE);
    QCOMPARE(iarToolchainPackage->environmentVariableName(), iarEnvVar);
    QCOMPARE(iarToolchainPackage->isDesktopToolchain(), false);
    QCOMPARE(iarToolchainPackage->toolChainName(), iar);
    QCOMPARE(iarToolchainPackage->toolchainType(), McuToolChainPackage::ToolChainType::IAR);
    QCOMPARE(iarToolchainPackage->label(), iarLabel);

    ProjectExplorer::ToolChainFactory toolchainFactory;
    Utils::Id iarId{BareMetal::Constants::IAREW_TOOLCHAIN_TYPEID};
    ToolChain *iar{toolchainFactory.createToolChain(iarId)};
    iar->setLanguage(cxxLanguageId);
    ToolChainManager::instance()->registerToolChain(iar);

    ToolChain *iarToolchain{iarToolchainPackage->toolChain(cxxLanguageId)};
    QVERIFY(iarToolchain != nullptr);
    QCOMPARE(iarToolchain->displayName(), "IAREW");
    QCOMPARE(iarToolchain->detection(), ToolChain::UninitializedDetection);
}

void verifyGccToolchain(const McuToolChainPackage *gccPackage)
{
    QVERIFY(gccPackage != nullptr);
    QCOMPARE(gccPackage->cmakeToolChainFileName(), QString{unsupported}.append(cmakeExtension));
    QCOMPARE(gccPackage->cmakeVariableName(), "");
    QCOMPARE(gccPackage->environmentVariableName(), "");
    QCOMPARE(gccPackage->isDesktopToolchain(), true);
    QCOMPARE(gccPackage->toolChainName(), unsupported);
    QCOMPARE(gccPackage->toolchainType(), McuToolChainPackage::ToolChainType::GCC);
}

McuSupportTest::McuSupportTest()
    : targetFactory{settingsMockPtr}
    , toolchainPackagePtr{
          new McuToolChainPackage{settingsMockPtr,
                                  {},                                              // label
                                  {},                                              // defaultPath
                                  {},                                              // detectionPath
                                  {},                                              // settingsKey
                                  McuToolChainPackage::ToolChainType::Unsupported, // toolchain type
                                  {},                                              // cmake var name
                                  {}}}                                             // env var name
{}

void McuSupportTest::initTestCase()
{
    targetDescription = Sdk::McuTargetDescription{
        "2.0.1",
        "2",
        platform,
        Sdk::McuTargetDescription::Toolchain{},
        Sdk::McuTargetDescription::BoardSdk{},
        Sdk::McuTargetDescription::FreeRTOS{},
    };

    EXPECT_CALL(*freeRtosPackage, environmentVariableName())
        .WillRepeatedly(Return(QString{freeRtosEnvVar}));
    EXPECT_CALL(*freeRtosPackage, cmakeVariableName())
        .WillRepeatedly(Return(QString{freeRtosCMakeVar}));
    EXPECT_CALL(*freeRtosPackage, isValidStatus()).WillRepeatedly(Return(true));
    EXPECT_CALL(*freeRtosPackage, path())
        .WillRepeatedly(Return(FilePath::fromString(defaultfreeRtosPath)));
    EXPECT_CALL(*freeRtosPackage, isAddToSystemPath()).WillRepeatedly(Return(true));
    EXPECT_CALL(*freeRtosPackage, detectionPath()).WillRepeatedly(Return(Utils::FilePath{}));

    EXPECT_CALL(*sdkPackage, environmentVariableName())
        .WillRepeatedly(Return(QString{freeRtosEnvVar}));
    EXPECT_CALL(*sdkPackage, cmakeVariableName()).WillRepeatedly(Return(QString{freeRtosCMakeVar}));
    EXPECT_CALL(*sdkPackage, isValidStatus()).WillRepeatedly(Return(true));
    EXPECT_CALL(*sdkPackage, path())
        .WillRepeatedly(Return(FilePath::fromString(defaultfreeRtosPath)));
    EXPECT_CALL(*sdkPackage, isAddToSystemPath()).WillRepeatedly(Return(true));
    EXPECT_CALL(*sdkPackage, detectionPath()).WillRepeatedly(Return(Utils::FilePath{}));
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
    auto &freeRtosPackage = description.freeRTOS.packages[0];
    QCOMPARE(freeRtosPackage.envVar, nxp1064FreeRtosEnvVar);
}

void McuSupportTest::test_parseToolchainFromJSON()
{
    Sdk::McuTargetDescription description{Sdk::parseDescriptionJson(iar_stm32f469i_metal_json)};
    QCOMPARE(description.toolchain.id, iar);
    QCOMPARE(description.toolchain.packages.size(), 2);

    const Sdk::PackageDescription &compilerPackage{description.toolchain.packages.at(0)};
    QCOMPARE(compilerPackage.cmakeVar, Constants::TOOLCHAIN_DIR_CMAKE_VARIABLE);
    QCOMPARE(compilerPackage.envVar, iarEnvVar);

    const Sdk::PackageDescription &toolchainFilePackage{description.toolchain.packages.at(1)};
    QCOMPARE(toolchainFilePackage.label, cmakeToolchainLabel);
    QCOMPARE(toolchainFilePackage.envVar, QString{});
    QCOMPARE(toolchainFilePackage.cmakeVar, Constants::TOOLCHAIN_FILE_CMAKE_VARIABLE);
    QCOMPARE(toolchainFilePackage.defaultPath, "$Qul_ROOT/lib/cmake/Qul/toolchain/iar.cmake");
}

void McuSupportTest::test_addNewKit()
{
    const QString cmakeVar = "CMAKE_SDK";
    EXPECT_CALL(*sdkPackage, cmakeVariableName()).WillRepeatedly(Return(cmakeVar));
    Kit kit;

    const McuTarget::Platform platform{id, name, vendor};
    McuTarget mcuTarget{currentQulVersion,                   // version
                        platform,                            // platform
                        McuTarget::OS::FreeRTOS,             // os
                        {sdkPackagePtr, freeRtosPackagePtr}, // packages
                        toolchainPackagePtr};                // toolchain packages

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
    QVERIFY(Utils::indexOf(config.toVector(),
                           [&cmakeVar](const CMakeConfigItem &item) {
                               return item.key == cmakeVar.toUtf8();
                           })
            != -1);
}

void McuSupportTest::test_addFreeRtosCmakeVarToKit()
{
    Kit kit;
    const McuTarget::Platform platform{id, name, vendor};
    McuTarget mcuTarget{currentQulVersion,
                        platform,
                        McuTarget::OS::FreeRTOS,
                        {sdkPackagePtr, freeRtosPackagePtr},
                        toolchainPackagePtr};
    McuKitManager::upgradeKitInPlace(&kit, &mcuTarget, sdkPackagePtr);

    QVERIFY(kit.hasValue(EnvironmentKitAspect::id()));
    QVERIFY(kit.isValid());
    QVERIFY(!kit.allKeys().empty());

    const auto &cmakeConfig{CMakeConfigurationKitAspect::configuration(&kit)};
    QVERIFY(cmakeConfig.size() > 0);

    CMakeConfigItem
        expectedCmakeVar{freeRtosCMakeVar,
                         FilePath::fromString(defaultfreeRtosPath).toUserOutput().toLocal8Bit()};
    QVERIFY(cmakeConfig.contains(expectedCmakeVar));
}

void McuSupportTest::test_legacy_createIarToolchain()
{
    McuToolChainPackagePtr iarToolchainPackage = Sdk::createIarToolChainPackage(settingsMockPtr);
    verifyIarToolchain(iarToolchainPackage.get());
}

void McuSupportTest::test_createIarToolchain()
{
    const auto description = Sdk::parseDescriptionJson(iar_stm32f469i_metal_json);

    McuToolChainPackage *iarToolchainPackage{targetFactory.createToolchain(description.toolchain)};
    verifyIarToolchain(iarToolchainPackage);
}

void McuSupportTest::test_legacy_createDesktopGccToolchain()
{
    McuToolChainPackagePtr gccPackage = Sdk::createGccToolChainPackage(settingsMockPtr);
    verifyGccToolchain(gccPackage.get());
}

void McuSupportTest::test_createDesktopGccToolchain()
{
    const auto description = Sdk::parseDescriptionJson(gcc_desktop_json);
    McuToolChainPackage *gccPackage{targetFactory.createToolchain(description.toolchain)};
    verifyGccToolchain(gccPackage);
}

void McuSupportTest::test_skipTargetCreationWhenToolchainInfoIsMissing()
{
    const auto [targets, packages]{targetFactory.createTargets(targetDescription)};
    QVERIFY(targets.isEmpty());
}

void McuSupportTest::test_returnNullWhenCreatingToolchainIfInfoIsMissing()
{
    Sdk::McuTargetDescription::Toolchain toolchainDescription{};
    toolchainDescription.id = iar;
    McuToolChainPackage *toolchain{targetFactory.createToolchain(toolchainDescription)};
    QCOMPARE(toolchain, nullptr);
}

void McuSupportTest::test_returnNullWhenCreatingToolchainIfIdIsEmpty()
{
    McuToolChainPackage *toolchain{targetFactory.createToolchain({})};
    QCOMPARE(toolchain, nullptr);
}

void McuSupportTest::test_defaultToolchainPackageCtorShouldReturnDefaultToolchainFileName()
{
    QVERIFY(!toolchainPackagePtr->cmakeToolChainFileName().isEmpty());
    QCOMPARE(toolchainPackagePtr->cmakeToolChainFileName(),
             QString{unsupported}.append(cmakeExtension));
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

void McuSupportTest::test_createPackagesWithCorrespondingSettings_data()
{
    QTest::addColumn<QString>("json");
    QTest::addColumn<QSet<QString>>("expectedSettings");

    QSet<QString> commonSettings{{"CypressAutoFlashUtil"},
                                 {"GHSArmToolchain"},
                                 {"GHSToolchain"},
                                 {"GNUArmEmbeddedToolchain"},
                                 {"IARToolchain"},
                                 {"MCUXpressoIDE"},
                                 {"RenesasFlashProgrammer"},
                                 {"Stm32CubeProgrammer"}};

    QTest::newRow("nxp1064") << iar_nxp_1064_json
                             << QSet<QString>{{"EVK_MIMXRT1064_SDK_PATH"},
                                              {QString{Constants::SETTINGS_KEY_FREERTOS_PREFIX}
                                                   .append("IMXRT1064")}}
                                    .unite(commonSettings);
    QTest::newRow("nxp1050") << armgcc_nxp_1050_json
                             << QSet<QString>{{"EVKB_IMXRT1050_SDK_PATH"},
                                              {QString{Constants::SETTINGS_KEY_FREERTOS_PREFIX}
                                                   .append("IMXRT1050")}}
                                    .unite(commonSettings);

    QTest::newRow("stm32h750b") << armgcc_stm32h750b_metal_json
                                << QSet<QString>{{"STM32Cube_FW_H7_SDK_PATH"}}.unite(commonSettings);

    QTest::newRow("stm32f769i") << armgcc_stm32f769i_freertos_json
                                << QSet<QString>{{"STM32Cube_FW_F7_SDK_PATH"}}.unite(commonSettings);

    QTest::newRow("stm32f469i") << iar_stm32f469i_metal_json
                                << QSet<QString>{{"STM32Cube_FW_F4_SDK_PATH"}}.unite(commonSettings);
}

void McuSupportTest::test_createPackagesWithCorrespondingSettings()
{
    QFETCH(QString, json);
    const Sdk::McuTargetDescription description = Sdk::parseDescriptionJson(json.toLocal8Bit());
    const auto [targets,
                packages]{Sdk::targetsFromDescriptions({description}, settingsMockPtr, runLegacy)};
    Q_UNUSED(targets);

    QSet<QString> settings = Utils::transform<QSet<QString>>(packages, [](const auto &package) {
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
                                               "setting",
                                               "Freertos directory",
                                               "/opt/freertos/1064",
                                               "",
                                               {},
                                               true};
    targetDescription.freeRTOS.packages.append(packageDescription);
    targetDescription.toolchain.id = armgcc;
    targetDescription.toolchain.packages.append(Sdk::PackageDescription{});

    const auto [targets, packages]{targetFactory.createTargets(targetDescription)};
    QVERIFY(!targets.empty());
    const McuTargetPtr target{*targets.constBegin()};
    QCOMPARE(target->colorDepth(), colorDepth);
    const auto &tgtPackages{target->packages()};
    QVERIFY(!tgtPackages.empty());
    // for whatever reasons there are more than a single package, get the right one to check
    const auto rtosPackage
            = Utils::findOrDefault(tgtPackages,
                                   [id = QString::fromLatin1(id)](McuPackagePtr pkg) {
        return pkg->label() == id;
    });
    QVERIFY(rtosPackage);
    QCOMPARE(rtosPackage->environmentVariableName(), nxp1064FreeRtosEnvVar);
}

void McuSupportTest::test_createPackages()
{
    Sdk::PackageDescription packageDescription{id,
                                               nxp1064FreeRtosEnvVar,
                                               freeRtosCMakeVar,
                                               "Freertos directory",
                                               "setting",
                                               "/opt/freertos/1064",
                                               "",
                                               {},
                                               true};
    targetDescription.freeRTOS.packages.append(packageDescription);

    const auto packages{targetFactory.createPackages(targetDescription)};
    QVERIFY(!packages.empty());
}

void McuSupportTest::test_removeRtosSuffix_data()
{
    QTest::addColumn<QString>("freeRtosEnvVar");
    QTest::addColumn<QString>("expectedEnvVarWithoutSuffix");

    QTest::newRow("nxp1050") << nxp1050FreeRtosEnvVar << nxp1050;
    QTest::newRow("nxp1064") << nxp1064FreeRtosEnvVar << nxp1064;
    QTest::newRow("nxp1170") << nxp1170FreeRtosEnvVar << nxp1170;
    QTest::newRow("stm32f7") << stm32f7FreeRtosEnvVar << stm32f7;
}

void McuSupportTest::test_removeRtosSuffix()
{
    QFETCH(QString, freeRtosEnvVar);
    QFETCH(QString, expectedEnvVarWithoutSuffix);
    QCOMPARE(removeRtosSuffix(freeRtosEnvVar), expectedEnvVarWithoutSuffix);
}

void McuSupportTest::test_2dot1UsesLegacyImplementation()
{
    QCOMPARE(McuSupportOptions::isLegacyVersion({2, 1}), true);
    QCOMPARE(McuSupportOptions::isLegacyVersion({2, 0}), true);
    QCOMPARE(McuSupportOptions::isLegacyVersion({2, 0, 0}), true);
    QCOMPARE(McuSupportOptions::isLegacyVersion({2, 0, 1}), true);
    QCOMPARE(McuSupportOptions::isLegacyVersion({2, 2, 0}), false);
    QCOMPARE(McuSupportOptions::isLegacyVersion({2, 2, 1}), false);
}

} // namespace McuSupport::Internal::Test
