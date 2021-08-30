/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
