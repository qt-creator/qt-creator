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

#include "mcutargetdescription.h"
#include "nxp_1064_json.h"
#include "unittest.h"
#include "utils/filepath.h"
#include <cmakeprojectmanager/cmakeconfigitem.h>
#include <cmakeprojectmanager/cmakekitinformation.h>
#include <gmock/gmock.h>
#include <QJsonArray>
#include <QJsonDocument>

namespace McuSupport::Internal::Test {

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
    EXPECT_CALL(freeRtosPackage, validStatus()).WillRepeatedly(Return(true));
    EXPECT_CALL(freeRtosPackage, path())
        .WillRepeatedly(Return(FilePath::fromString(defaultfreeRtosPath)));
}

void McuSupportTest::test_parseBasicInfoFromJson()
{
    const auto description = Sdk::parseDescriptionJson(nxp_1064_json);

    QVERIFY(not description.freeRTOS.envVar.isEmpty());
    QVERIFY(description.freeRTOS.boardSdkSubDir.isEmpty());
}

void McuSupportTest::test_addNewKit()
{
    auto &kitManager{*KitManager::instance()};

    QSignalSpy kitAddedSpy(&kitManager, &KitManager::kitAdded);

    auto *newKit{McuSupportOptions::newKit(&mcuTarget, &freeRtosPackage)};
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
    McuSupportOptions::updateKitEnvironment(&kit, &mcuTarget);

    QVERIFY(kit.hasValue(EnvironmentKitAspect::id()));
    QVERIFY(kit.isValid());
    QVERIFY(not kit.allKeys().empty());

    const auto &cmakeConfig{CMakeConfigurationKitAspect::configuration(&kit)};
    QCOMPARE(cmakeConfig.size(), 1);

    CMakeConfigItem expectedCmakeVar{freeRtosCmakeVar.toLocal8Bit(),
                                     defaultfreeRtosPath.toLocal8Bit()};
    QVERIFY(cmakeConfig.contains(expectedCmakeVar));
}

} // namespace McuSupport::Internal::Test
