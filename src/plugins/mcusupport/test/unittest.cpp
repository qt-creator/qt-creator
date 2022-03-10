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
#include "mcukitmanager.h"
#include "mcusupportconstants.h"
#include "mcusupportsdk.h"
#include "mcutargetdescription.h"
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

static const QString nxp1050FreeRtosEnvVar{"IMXRT1050_FREERTOS_DIR"};
static const QString nxp1064FreeRtosEnvVar{"IMXRT1064_FREERTOS_DIR"};
static const QString nxp1170FreeRtosEnvVar{"EVK_MIMXRT1170_FREERTOS_PATH"};
static const QString stm32f7FreeRtosEnvVar{"STM32F7_FREERTOS_DIR"};
static const QString stm32f7{"STM32F7"};
static const QString nxp1170{"EVK_MIMXRT1170"};
static const QString nxp1050{"IMXRT1050"};
static const QString nxp1064{"IMXRT1064"};

static const QStringList jsonFiles{QString::fromUtf8(armgcc_nxp_1050_json),
                                   QString::fromUtf8(armgcc_nxp_1064_json)};

using CMakeProjectManager::CMakeConfigItem;
using CMakeProjectManager::CMakeConfigurationKitAspect;
using ProjectExplorer::EnvironmentKitAspect;
using ProjectExplorer::KitManager;
using testing::Return;
using testing::ReturnRef;
using Utils::FilePath;

void McuSupportTest::initTestCase()
{
    EXPECT_CALL(freeRtosPackage, environmentVariableName()).WillRepeatedly(ReturnRef(freeRtosEnvVar));
    EXPECT_CALL(freeRtosPackage, isValidStatus()).WillRepeatedly(Return(true));
    EXPECT_CALL(freeRtosPackage, path())
        .WillRepeatedly(Return(FilePath::fromString(defaultfreeRtosPath)));
}

void McuSupportTest::test_parseBasicInfoFromJson()
{
    const auto description = Sdk::parseDescriptionJson(armgcc_nxp_1064_json);

    QVERIFY(!description.freeRTOS.envVar.isEmpty());
    QVERIFY(description.freeRTOS.boardSdkSubDir.isEmpty());
}

void McuSupportTest::test_addNewKit()
{
    auto &kitManager{*KitManager::instance()};

    QSignalSpy kitAddedSpy(&kitManager, &KitManager::kitAdded);

    auto *newKit{McuKitManager::newKit(&mcuTarget, &freeRtosPackage)};
    QVERIFY(newKit != nullptr);

    QCOMPARE(kitAddedSpy.count(), 1);
    QList<QVariant> arguments = kitAddedSpy.takeFirst();
    auto *createdKit = qvariant_cast<Kit *>(arguments.at(0));
    QVERIFY(createdKit != nullptr);
    QCOMPARE(createdKit, newKit);

    auto cmakeAspect{CMakeConfigurationKitAspect{}};
    QVERIFY(createdKit->hasValue(cmakeAspect.id()));
    QVERIFY(createdKit->value(cmakeAspect.id(), freeRtosCmakeVar).isValid());
}

void McuSupportTest::test_addFreeRtosCmakeVarToKit()
{
    McuKitManager::updateKitEnvironment(&kit, &mcuTarget);

    QVERIFY(kit.hasValue(EnvironmentKitAspect::id()));
    QVERIFY(kit.isValid());
    QVERIFY(!kit.allKeys().empty());

    const auto &cmakeConfig{CMakeConfigurationKitAspect::configuration(&kit)};
    QCOMPARE(cmakeConfig.size(), 1);

    CMakeConfigItem
        expectedCmakeVar{freeRtosCmakeVar.toLocal8Bit(),
                         FilePath::fromString(defaultfreeRtosPath).toUserOutput().toLocal8Bit()};
    QVERIFY(cmakeConfig.contains(expectedCmakeVar));
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
    const auto description = Sdk::parseDescriptionJson(json.toLocal8Bit());
    QVector<McuAbstractPackage *> packages;

    QSet<QString> settings = Utils::transform<QSet<QString>>(packages, [](const auto &package) {
        return package->settingsKey();
    });
    QFETCH(QSet<QString>, expectedSettings);
    QVERIFY(settings.contains(expectedSettings));
}

} // namespace McuSupport::Internal::Test
