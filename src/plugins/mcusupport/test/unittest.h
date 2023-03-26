// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    std::pair<Targets, Packages> createTestingKitTargetsAndPackages(QByteArray test_file);

private slots:
    void initTestCase();
    void init();
    void cleanup();

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
    void test_legacy_createQtMCUsPackage();

    void test_createTargets();
    void test_createPackages();
    void test_legacy_createIarToolchain();
    void test_createIarToolchain();
    void test_legacy_createDesktopGccToolchain();
    void test_createDesktopGccToolchain();
    void test_legacy_createDesktopMsvcToolchain();
    void test_createDesktopMsvcToolchain();
    void test_createDesktopMingwToolchain();
    void test_verifyManuallyCreatedArmGccToolchain();
    void test_legacy_createArmGccToolchain();
    void test_createArmGccToolchain_data();
    void test_createArmGccToolchain();
    void test_removeRtosSuffixFromEnvironmentVariable_data();
    void test_removeRtosSuffixFromEnvironmentVariable();
    void test_useFallbackPathForToolchainWhenPathFromSettingsIsNotAvailable();
    void test_usePathFromSettingsForToolchainPath();

    void test_twoDotOneUsesLegacyImplementation();
    void test_addToolchainFileInfoToKit();
    void test_getFullToolchainFilePathFromTarget();
    void test_legacy_getPredefinedToolchainFilePackage();
    void test_legacy_createUnsupportedToolchainFilePackage();
    void test_legacy_supportMultipleToolchainVersions();

    void test_passExecutableVersionDetectorToToolchainPackage_data();
    void test_passExecutableVersionDetectorToToolchainPackage();
    void test_legacy_passExecutableVersionDetectorToToolchainPackage_data();
    void test_legacy_passExecutableVersionDetectorToToolchainPackage();
    void test_legacy_passXMLVersionDetectorToNxpAndStmBoardSdkPackage_data();
    void test_legacy_passXMLVersionDetectorToNxpAndStmBoardSdkPackage();
    void test_passXMLVersionDetectorToNxpAndStmBoardSdkPackage_data();
    void test_passXMLVersionDetectorToNxpAndStmBoardSdkPackage();
    void test_passDirectoryVersionDetectorToRenesasBoardSdkPackage();

    void test_legacy_createBoardSdk_data();
    void test_legacy_createBoardSdk();
    void test_createBoardSdk_data();
    void test_createBoardSdk();

    void test_legacy_doNOTcreateFreeRtosPackageForMetalVariants_data();
    void test_legacy_doNOTcreateFreeRtosPackageForMetalVariants();
    void test_legacy_createFreeRtosPackage_data();
    void test_legacy_createFreeRtosPackage();
    void test_createFreeRtosPackage_data();
    void test_createFreeRtosPackage();

    void test_resolveEnvironmentVariablesInDefaultPath();
    void test_resolveCmakeVariablesInDefaultPath();

    void test_legacy_createThirdPartyPackage_data();
    void test_legacy_createThirdPartyPackage();
    void test_createThirdPartyPackage_data();
    void test_createThirdPartyPackage();
    void test_legacy_createCypressProgrammer3rdPartyPackage();
    void test_createJLink3rdPartyPackage();

    void test_differentValueForEachOperationSystem();
    void test_addToSystemPathFlag();
    void test_processWildcards_data();
    void test_processWildcards();

    void test_nonemptyVersionDetector();
    void test_emptyVersionDetector();
    void test_emptyVersionDetectorFromJson();

    void test_expectedValueType();

private:
    QVersionNumber currentQulVersion{2, 0};
    PackageMock *freeRtosPackage{new PackageMock};
    PackageMock *sdkPackage{new PackageMock};
    McuPackagePtr freeRtosPackagePtr{freeRtosPackage};
    McuPackagePtr sdkPackagePtr{sdkPackage};

    QSharedPointer<SettingsHandlerMock> settingsMockPtr{new SettingsHandlerMock};
    McuTargetFactory targetFactory;
    PackageDescription compilerDescription;
    PackageDescription toochainFileDescription;
    McuTargetDescription targetDescription;
    McuToolChainPackagePtr toolchainPackagePtr;
    McuToolChainPackagePtr armGccToolchainPackagePtr;
    McuToolChainPackagePtr iarToolchainPackagePtr;
    PackageMock *armGccToolchainFilePackage{new PackageMock};
    McuPackagePtr armGccToolchainFilePackagePtr{armGccToolchainFilePackage};
    McuTarget::Platform platform;
    McuTarget mcuTarget;
    ProjectExplorer::Kit kit;

}; // class McuSupportTest

using PackageCreator = std::function<McuPackagePtr()>;
} // namespace McuSupport::Internal::Test

Q_DECLARE_METATYPE(McuSupport::Internal::Test::PackageCreator);
