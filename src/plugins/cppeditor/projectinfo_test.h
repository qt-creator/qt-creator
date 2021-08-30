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

class ProjectPartChooserTest : public QObject
{
    Q_OBJECT

private slots:
    void testChooseManuallySet();
    void testIndicateManuallySet();
    void testIndicateManuallySetForFallbackToProjectPartFromDependencies();
    void testDoNotIndicateNotManuallySet();
    void testForMultipleChooseFromActiveProject();
    void testForMultiplePreferSelectedForBuilding();
    void testForMultipleFromDependenciesChooseFromActiveProject();
    void testForMultipleCheckIfActiveProjectChanged();
    void testForMultipleAndAmbigiousHeaderPreferCProjectPart();
    void testForMultipleAndAmbigiousHeaderPreferCxxProjectPart();
    void testIndicateMultiple();
    void testIndicateMultipleForFallbackToProjectPartFromDependencies();
    void testForMultipleChooseNewIfPreviousIsGone();
    void testFallbackToProjectPartFromDependencies();
    void testFallbackToProjectPartFromModelManager();
    void testContinueUsingFallbackFromModelManagerIfProjectDoesNotChange();
    void testStopUsingFallbackFromModelManagerIfProjectChanges1();
    void testStopUsingFallbackFromModelManagerIfProjectChanges2();
    void testIndicateFallbacktoProjectPartFromModelManager();
    void testIndicateFromDependencies();
    void testDoNotIndicateFromDependencies();
};

class ProjectInfoGeneratorTest : public QObject
{
    Q_OBJECT

private slots:
    void testCreateNoProjectPartsForEmptyFileList();
    void testCreateSingleProjectPart();
    void testCreateMultipleProjectParts();
    void testProjectPartIndicatesObjectiveCExtensionsByDefault();
    void testProjectPartHasLatestLanguageVersionByDefault();
    void testUseMacroInspectionReportForLanguageVersion();
    void testUseCompilerFlagsForLanguageExtensions();
    void testProjectFileKindsMatchProjectPartVersion();
};

class HeaderPathFilterTest : public QObject
{
    Q_OBJECT

private slots:
    void testBuiltin();
    void testSystem();
    void testUser();
    void testNoProjectPathSet();
    void testDontAddInvalidPath();
    void testClangHeadersPath();
    void testClangHeadersPathWitoutClangVersion();
    void testClangHeadersAndCppIncludesPathsOrderMacOs();
    void testClangHeadersAndCppIncludesPathsOrderLinux();
    void testRemoveGccInternalPaths();
    void testRemoveGccInternalPathsExceptForStandardPaths();
    void testClangHeadersAndCppIncludesPathsOrderNoVersion();
    void testClangHeadersAndCppIncludesPathsOrderAndroidClang();
};

class ProjectFileCategorizerTest : public QObject
{
    Q_OBJECT

private slots:
    void testC();
    void testCxxWithUnambiguousHeaderSuffix();
    void testCxxWithAmbiguousHeaderSuffix();
    void testObjectiveC();
    void testObjectiveCxx();
    void testMixedCAndCxx();
    void testAmbiguousHeaderOnly();
};

} // namespace CppEditor::Internal
