// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
