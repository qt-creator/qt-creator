// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace CppEditor::Internal {

class CompilerOptionsBuilderTest : public QObject
{
    Q_OBJECT

private slots:
    void testAddProjectMacros();
    void testUnknownFlagsAreForwarded();
    void testWarningsFlagsAreNotFilteredIfRequested();
    void testDiagnosticOptionsAreRemoved();
    void testCLanguageVersionIsRewritten();
    void testLanguageVersionIsExplicitlySetIfNotProvided();
    void testLanguageVersionIsExplicitlySetIfNotProvidedMsvc();
    void testAddWordWidth();
    void testHeaderPathOptionsOrder();
    void testHeaderPathOptionsOrderMsvc();
    void testUseSystemHeader();
    void testNoClangHeadersPath();
    void testClangHeadersAndCppIncludePathsOrderMacOs();
    void testClangHeadersAndCppIncludePathsOrderLinux();
    void testClangHeadersAndCppIncludePathsOrderNoVersion();
    void testClangHeadersAndCppIncludePathsOrderAndroidClang();
    void testNoPrecompiledHeader();
    void testUsePrecompiledHeader();
    void testUsePrecompiledHeaderMsvc();
    void testAddMacros();
    void testAddTargetTriple();
    void testEnableCExceptions();
    void testEnableCxxExceptions();
    void testInsertWrappedQtHeaders();
    void testInsertWrappedMingwHeadersWithNonMingwToolchain();
    void testInsertWrappedMingwHeadersWithMingwToolchain();
    void testSetLanguageVersion();
    void testSetLanguageVersionMsvc();
    void testHandleLanguageExtension();
    void testUpdateLanguageVersion();
    void testUpdateLanguageVersionMsvc();
    void testAddMsvcCompatibilityVersion();
    void testUndefineCppLanguageFeatureMacrosForMsvc2015();
    void testAddDefineFunctionMacrosMsvc();
    void testAddProjectConfigFileInclude();
    void testAddProjectConfigFileIncludeMsvc();
    void testNoUndefineClangVersionMacrosForNewMsvc();
    void testUndefineClangVersionMacrosForOldMsvc();
    void testBuildAllOptions();
    void testBuildAllOptionsMsvc();
    void testBuildAllOptionsMsvcWithExceptions();
};

} // namespace CppEditor::Internal
