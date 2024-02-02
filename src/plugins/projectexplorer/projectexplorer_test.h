// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifdef WITH_TESTS

#include <QObject>

namespace ProjectExplorer { class ProjectExplorerPlugin; }

namespace ProjectExplorer::Internal {

class ProjectExplorerTest final : public QObject
{
    Q_OBJECT

private slots:
    void testJsonWizardsEmptyWizard();
    void testJsonWizardsEmptyPage();
    void testJsonWizardsUnusedKeyAtFields_data();
    void testJsonWizardsUnusedKeyAtFields();
    void testJsonWizardsCheckBox();
    void testJsonWizardsLineEdit();
    void testJsonWizardsComboBox();
    void testJsonWizardsIconList();

    void testGccOutputParsers_data();
    void testGccOutputParsers();

    void testCustomOutputParsers_data();
    void testCustomOutputParsers();

    void testClangOutputParser_data();
    void testClangOutputParser();

    void testLinuxIccOutputParsers_data();
    void testLinuxIccOutputParsers();

    void testGnuMakeParserParsing_data();
    void testGnuMakeParserParsing();
    void testGnuMakeParserTaskMangling();

    void testXcodebuildParserParsing_data();
    void testXcodebuildParserParsing();

    void testMsvcOutputParsers_data();
    void testMsvcOutputParsers();

    void testClangClOutputParsers_data();
    void testClangClOutputParsers();

    void testGccAbiGuessing_data();
    void testGccAbiGuessing();

    void testAbiRoundTrips();
    void testAbiOfBinary_data();
    void testAbiOfBinary();
    void testAbiFromTargetTriplet_data();
    void testAbiFromTargetTriplet();
    void testAbiUserOsFlavor_data();
    void testAbiUserOsFlavor();

    void testDeviceManager();

    void testToolChainMerging_data();
    void testToolChainMerging();
    static void deleteTestToolchains();

    void testUserFileAccessor_prepareToReadSettings();
    void testUserFileAccessor_prepareToReadSettingsObsoleteVersion();
    void testUserFileAccessor_prepareToReadSettingsObsoleteVersionNewVersion();
    void testUserFileAccessor_prepareToWriteSettings();
    void testUserFileAccessor_mergeSettings();
    void testUserFileAccessor_mergeSettingsEmptyUser();
    void testUserFileAccessor_mergeSettingsEmptyShared();

    void testProject_setup();
    void testProject_changeDisplayName();
    void testProject_parsingSuccess();
    void testProject_parsingFail();
    void testProject_projectTree();
    void testProject_multipleBuildConfigs();

    void testSourceToBinaryMapping();
    void testSourceToBinaryMapping_data();

    void testSessionSwitch();

private:
    friend class ::ProjectExplorer::ProjectExplorerPlugin;
};

} // ProjectExplorer::Internal

#endif // WITH_TESTS
