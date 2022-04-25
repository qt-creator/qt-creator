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

#pragma once

#include "mcutarget.h"
#include "mcutargetfactory.h"
#include "packagemock.h"
#include "settingshandlermock.h"

#include <projectexplorer/kit.h>

#include <QObject>
#include <QSignalSpy>
#include <QTest>

namespace McuSupport::Internal::Test {

class McuSupportTest : public QObject
{
    Q_OBJECT

public:
    McuSupportTest();

private slots:
    void initTestCase();

    void test_addNewKit();
    void test_parseBasicInfoFromJson();
    void test_parseCmakeEntries();
    void test_parseToolchainFromJSON_data();
    void test_parseToolchainFromJSON();
    void test_mapParsedToolchainIdToCorrespondingType_data();
    void test_mapParsedToolchainIdToCorrespondingType();
    void test_legacy_createPackagesWithCorrespondingSettings();
    void test_legacy_createPackagesWithCorrespondingSettings_data();
    void test_legacy_createTargetWithToolchainPackages_data();
    void test_legacy_createTargetWithToolchainPackages();
    void test_createTargetWithToolchainPackages_data();
    void test_createTargetWithToolchainPackages();

    void test_createFreeRtosPackageWithCorrectSetting_data();
    void test_createFreeRtosPackageWithCorrectSetting();
    void test_createTargets();
    void test_createPackages();
    void test_addFreeRtosCmakeVarToKit();
    void test_legacy_createIarToolchain();
    void test_createIarToolchain();
    void test_legacy_createDesktopGccToolchain();
    void test_createDesktopGccToolchain();
    void test_verifyManuallyCreatedArmGccToolchain();
    void test_legacy_createArmGccToolchain();
    void test_createArmGccToolchain_data();
    void test_createArmGccToolchain();
    void test_removeRtosSuffixFromEnvironmentVariable_data();
    void test_removeRtosSuffixFromEnvironmentVariable();

    void test_twoDotOneUsesLegacyImplementation();
    void test_addToolchainFileInfoToKit();
    void test_getFullToolchainFilePathFromTarget();
    void test_legacy_getPredefinedToolchainFilePackage();
    void test_legacy_createUnsupportedToolchainFilePackage();

private:
    QVersionNumber currentQulVersion{2, 0};
    PackageMock *freeRtosPackage{new PackageMock};
    PackageMock *sdkPackage{new PackageMock};
    McuPackagePtr freeRtosPackagePtr{freeRtosPackage};
    McuPackagePtr sdkPackagePtr{sdkPackage};

    QSharedPointer<SettingsHandlerMock> settingsMockPtr{new SettingsHandlerMock};
    McuTargetFactory targetFactory;
    Sdk::McuTargetDescription targetDescription;
    McuToolChainPackagePtr toolchainPackagePtr;
    McuToolChainPackagePtr armGccToolchainPackagePtr;
    McuToolChainPackagePtr iarToolchainPackagePtr;
    PackageMock *armGccToolchainFilePackage{new PackageMock};
    McuPackagePtr armGccToolchainFilePackagePtr{armGccToolchainFilePackage};
    McuTarget::Platform platform;
    McuTarget mcuTarget;
    ProjectExplorer::Kit kit;
}; // class McuSupportTest

} // namespace McuSupport::Internal::Test
