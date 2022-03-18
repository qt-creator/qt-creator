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
#include "armgcc_nxp_1064_json.h"
#include "armgcc_stm32f769i_freertos_json.h"
#include "armgcc_stm32h750b_metal_json.h"
#include "iar_stm32f469i_metal_json.h"
#include "mcuhelpers.h"
#include "mcukitmanager.h"
#include "mcusupportconstants.h"
#include "mcusupportsdk.h"
#include "mcutargetdescription.h"
#include "mcutargetfactory.h"
#include "mcutargetfactorylegacy.h"
#include <utils/algorithm.h>
#include <utils/filepath.h>

#include <cmakeprojectmanager/cmakeconfigitem.h>
#include <cmakeprojectmanager/cmakekitinformation.h>
#include <gmock/gmock.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <qtestcase.h>

#include <algorithm>
#include <ciso646>

namespace McuSupport::Internal::Test {

namespace {
// clazy:excludeall=non-pod-global-static
const QString freeRtosCMakeVar{"FREERTOS_DIR"};
const QString nxp1050FreeRtosEnvVar{"IMXRT1050_FREERTOS_DIR"};
const QString nxp1064FreeRtosEnvVar{"IMXRT1064_FREERTOS_DIR"};
const QString nxp1170FreeRtosEnvVar{"EVK_MIMXRT1170_FREERTOS_PATH"};
const QString stm32f7FreeRtosEnvVar{"STM32F7_FREERTOS_DIR"};
const QString stm32f7{"STM32F7"};
const QString nxp1170{"EVK_MIMXRT1170"};
const QString nxp1050{"IMXRT1050"};
const QString nxp1064{"IMXRT1064"};
const QStringList jsonFiles{QString::fromUtf8(armgcc_nxp_1050_json),
                            QString::fromUtf8(armgcc_nxp_1064_json)};
constexpr bool RUN_LEGACY{true};
constexpr int colorDepth{32};
const QString id{"id"};
} // namespace

using CMakeProjectManager::CMakeConfigItem;
using CMakeProjectManager::CMakeConfigurationKitAspect;
using ProjectExplorer::KitManager;
using Utils::FilePath;

void McuSupportTest::initTestCase() {}

void McuSupportTest::test_parseBasicInfoFromJson()
{
    const auto description = Sdk::parseDescriptionJson(armgcc_nxp_1064_json);

    QVERIFY(!description.freeRTOS.envVar.isEmpty());
    QVERIFY(description.freeRTOS.boardSdkSubDir.isEmpty());
}

void McuSupportTest::test_parseCmakeEntries()
{
    const auto description{Sdk::parseDescriptionJson(armgcc_nxp_1064_json)};

    QVERIFY(not description.freeRTOS.packages.isEmpty());
    auto &freeRtosPackage = description.freeRTOS.packages[0];
    QCOMPARE(freeRtosPackage.envVar, nxp1064FreeRtosEnvVar);
}

void McuSupportTest::test_addNewKit()
{
    const QString cmakeVar = "CMAKE_SDK";
    McuPackage sdkPackage{"sdk",    // label
                          {},       // defaultPath
                          {},       // detectionPath
                          "sdk",    // settingsKey
                          cmakeVar, // cmake var
                          {}};      // env var
    ProjectExplorer::Kit kit;

    McuToolChainPackage
        toolchainPackage{{},                                              // label
                         {},                                              // defaultPath
                         {},                                              // detectionPath
                         {},                                              // settingsKey
                         McuToolChainPackage::ToolChainType::Unsupported, // toolchain type
                         {},                                              // cmake var name
                         {}};                                             // env var name
    const McuTarget::Platform platform{id, name, vendor};
    McuTarget mcuTarget{currentQulVersion,       // version
                        platform,                // platform
                        McuTarget::OS::FreeRTOS, // os
                        {&sdkPackage},           // packages
                        &toolchainPackage};      // toolchain packages

    auto &kitManager{*KitManager::instance()};

    QSignalSpy kitAddedSpy(&kitManager, &KitManager::kitAdded);

    auto *newKit{McuKitManager::newKit(&mcuTarget, &sdkPackage)};
    QVERIFY(newKit != nullptr);

    QCOMPARE(kitAddedSpy.count(), 1);
    QList<QVariant> arguments = kitAddedSpy.takeFirst();
    auto *createdKit = qvariant_cast<ProjectExplorer::Kit *>(arguments.at(0));
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

    QTest::newRow("nxp1064") << armgcc_nxp_1064_json
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
    const auto [targets, packages]{Sdk::targetsFromDescriptions({description}, RUN_LEGACY)};
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

//TODO(piotr.mucko): Enable when mcutargetfactory is delivered.
void McuSupportTest::test_createFreeRtosPackageWithCorrectSetting()
{
    // Sdk::targetsAndPackages(jsonFile, &mcuSdkRepo);
    //
    // QVector<Package *> mcuPackages;
    // auto mcuTargets = Sdk::targetsFromDescriptions({description}, &mcuPackages);
    // QVERIFY(mcuPackages contains freertos package)
    // QVERIFY(freertos package is not empty & has proper value)

    // McuSupportOptions mcuSuportOptions{};
    // mcuSuportOptions.createAutomaticKits();

    QFETCH(QString, freeRtosEnvVar);
    QFETCH(QString, expectedSettingsKey);

    auto *package{Sdk::createFreeRTOSSourcesPackage(freeRtosEnvVar, FilePath{}, QString{})};
    QVERIFY(package != nullptr);

    QCOMPARE(package->settingsKey(), expectedSettingsKey);

    // QVERIFY(freertos package is not empty & has proper value)
    // static McuPackage *createFreeRTOSSourcesPackage(const QString &envVar,
    //                                                 const FilePath &boardSdkDir,
    //                                                 const QString &freeRTOSBoardSdkSubDir)
    // createFreeRtosPackage
    // verify that package's setting is Package_FreeRTOSSourcePackage_IMXRT1064.
    //TODO(me): write settings
    // auto *freeRtosPackage
    // = new McuPackage;
    // freeRtosPackage->writeToSettings();
    //TODO(me): verify that setting is the same as in 2.0.0
}

void McuSupportTest::test_createTargetsTheNewWay_data() {}

void McuSupportTest::test_createTargetsTheNewWay()
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

    Sdk::McuTargetDescription description{
        "2.0.1",
        "2",
        {id, "", "", {colorDepth}, Sdk::McuTargetDescription::TargetType::MCU},
        {},                             // toolchain
        {},                             // boardSDK
        {"", "", {packageDescription}}, //freertos
    };

    Sdk::McuTargetFactory targetFactory{};
    const auto [targets, packages]{targetFactory.createTargets(description)};
    QVERIFY(not targets.empty());
    QCOMPARE(targets.at(0)->colorDepth(), colorDepth);
    const auto &tgtPackages{targets.at(0)->packages()};
    QVERIFY(not tgtPackages.empty());
    const auto rtosPackage{tgtPackages.first()};
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
    Sdk::McuTargetDescription targetDescription{
        "2.0.1",
        "2",
        {id, id, id, {colorDepth}, Sdk::McuTargetDescription::TargetType::MCU},
        {},                             // toolchain
        {},                             // boardSDK
        {"", "", {packageDescription}}, //freertos
    };

    Sdk::McuTargetFactory targetFactory;
    const auto packages{targetFactory.createPackages(targetDescription)};
    QVERIFY(not packages.empty());
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

} // namespace McuSupport::Internal::Test
