// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <memory>

namespace CppEditor { namespace Tests { class TemporaryCopiedDir; } }

namespace CompilationDatabaseProjectManager::Internal {

class CompilationDatabaseTests : public QObject
{
    Q_OBJECT
public:
    explicit CompilationDatabaseTests(QObject *parent = nullptr);
    ~CompilationDatabaseTests();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testProject();
    void testProject_data();
    void testFilterEmptyFlags();
    void testFilterFromFilename();
    void testFilterArguments();
    void testSplitFlags();
    void testSplitFlagsWithEscapedQuotes();
    void testFilterCommand();
    void testFileKindDifferentFromExtension();
    void testFileKindDifferentFromExtension2();
    void testSkipOutputFiles();

private:
    void addTestRow(const QString &relativeFilePath);

    std::unique_ptr<CppEditor::Tests::TemporaryCopiedDir> m_tmpDir;
};

} // CompilationDatabaseProjectManager::Internal
