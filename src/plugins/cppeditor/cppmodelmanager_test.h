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
    void testUpdateEditorsAfterProjectUpdate();
    void testPrecompiledHeaders();
    void testRenameIncludes();
    void testRenameIncludesInEditor();
    void testDocumentsAndRevisions();
};

} // namespace CppEditor::Internal
