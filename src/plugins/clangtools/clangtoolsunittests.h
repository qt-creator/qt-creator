// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace CppEditor {
class ClangDiagnosticConfig;
namespace Tests { class TemporaryCopiedDir; }
} // namespace CppEditor

namespace ProjectExplorer { class Kit; }

namespace ClangTools {
namespace Internal {

class ClangToolsUnitTests : public QObject
{
    Q_OBJECT

public:
    ClangToolsUnitTests() = default;

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testProject();
    void testProject_data();

private:
    void addTestRow(const QByteArray &relativeFilePath,
                    int expectedDiagCountClangTidy, int expectedDiagCountClazy,
                    const CppEditor::ClangDiagnosticConfig &diagnosticConfig);

private:
    static int getTimeout();

    CppEditor::Tests::TemporaryCopiedDir *m_tmpDir = nullptr;
    ProjectExplorer::Kit *m_kit = nullptr;
    int m_timeout = getTimeout();
};

} // namespace Internal
} // namespace ClangTools
