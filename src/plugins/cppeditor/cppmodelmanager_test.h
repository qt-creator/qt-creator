// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace CppEditor::Internal {

class ModelManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void testPathsAreClean();
    void testFrameworkHeaders();
    void testRefreshAlsoIncludesOfProjectFiles();
    void testRefreshSeveralTimes();
    void testRefreshTestForChanges();
    void testRefreshAddedAndPurgeRemoved();
    void testRefreshTimeStampModifiedIfSourcefilesChange();
    void testRefreshTimeStampModifiedIfSourcefilesChange_data();
    void testSnapshotAfterTwoProjects();
    void testExtraeditorsupportUiFiles();
    void testGcIfLastCppeditorClosed();
    void testDontGcOpenedFiles();
    void testDefinesPerProject();
    void testDefinesPerEditor();
    void testStaleHeaderReuseAcrossConfigs_QTCREATORBUG_18800();
    void testStaleHeaderReuseViaNestedDependency();
    void testStaleHeaderReuseViaDiamondIncludeGuard();
    void testUpdateEditorsAfterProjectUpdate();
    void testPrecompiledHeaders();
    void testRenameIncludes_data();
    void testRenameIncludes();
    void testMoveIncludingSources_data();
    void testMoveIncludingSources();
    void testRenamingAFormRewritesTheIncludesOfItsHeader();
    void testRenameIncludesInEditor();
    void testDocumentsAndRevisions();
    void testSettingsChanges();
    void testOptionalIndexing_data();
    void testOptionalIndexing();
    void testTheHeaderPathsOfSeveralProjectsComeInOneOrder();
    void testTheMacrosOfSeveralProjectsComeInOneOrder();
    void testAFileSeveralProjectsListIsReadWithOneOfThem();
    void testAHeaderAppearingBetweenSessionsIsNoticed_data();
    void testAHeaderAppearingBetweenSessionsIsNoticed();
    void testAHeaderShadowingAnotherBetweenSessionsIsNoticed();
    void testAHeaderAppearingReachesWhoeverIncludesIt();
    void testAHeaderAppearingDoesNotReviveUnreachableFiles();
    void testASecondSessionsSymbolSearchFindsTheSameSymbols();
    void testASecondSessionFindsTheSameDefinition();
    void testAFileChangedInASecondSessionIsIndexedAgain();
    void testASwitchedConfigurationIsNotAnsweredWithTheOldOne();
    void testAnUpdateThatChangesNothingKeepsTheNextOneIncremental();
    void testAFileJustOverTheSizeLimitIsNotIndexed();
    void testAFileTheIndexerDoesNotDeliverIsReadOnRequest();
    void testAFileIsNotReadOnRequestWhileItsProjectIsBeingIndexed();
    void testReadingOnRequestKeepsTheDocumentsThatAreThere();
    void testReadingOnRequestFollowsTheIndexersRules_data();
    void testReadingOnRequestFollowsTheIndexersRules();
    void testADefinitionInAFileTheIndexerDoesNotDeliverIsFound();
    void testOnlyFilesThatCanHoldTheDefinitionAreRead_data();
    void testOnlyFilesThatCanHoldTheDefinitionAreRead();
    void testAllDefinitionsIncludeThoseInFilesTheIndexerDoesNotDeliver();
    void testACopyTakenBeforeAReadOnRequestStillFindsTheDefinition();
    void testTheClosestDefinitionOutsideTheSnapshotIsFound();
    void testASourceThatLeftTheSnapshotIsStillSearched();
    void testAFileThatCannotBeReadOnRequestIsSkipped();
};

} // namespace CppEditor::Internal
