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
    void testGccOutputParser_data();
    void testGccOutputParser();

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

    void testDeviceManager();

    void testUserFileAccessor_prepareToReadSettings();
    void testUserFileAccessor_prepareToWriteSettings();
    void testUserFileAccessor_mergeSettings();
    void testUserFileAccessor_mergeSettingsEmptyUser();
    void testUserFileAccessor_mergeSettingsEmptyShared();

    void testSessionSwitch();

private:
    friend class ::ProjectExplorer::ProjectExplorerPlugin;
};

} // ProjectExplorer::Internal

#endif // WITH_TESTS
