// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QString>

namespace CppEditor::Tests { class TemporaryCopiedDir; }
namespace Utils { class FilePath; }

namespace ClangTools::Internal {

class ReadExportedDiagnosticsTest : public QObject
{
    Q_OBJECT
public:
    ReadExportedDiagnosticsTest();
    ~ReadExportedDiagnosticsTest();

private slots:
    void initTestCase();
    void init();

    void testNonExistingFile();
    void testEmptyFile();
    void testUnexpectedFileContents();
    void testTidy();
    void testAcceptDiagsFromFilePaths_None();
    void testTidy_ClangAnalyzer();
    void testClazy();

    void testOffsetInvalidText();
    void testOffsetInvalidOffset_EmptyInput();
    void testOffsetInvalidOffset_Before();
    void testOffsetInvalidOffset_After();
    void testOffsetInvalidOffset_NotFirstByteOfMultiByte();
    void testOffsetStartOfFirstLine();
    void testOffsetEndOfFirstLine();
    void testOffsetOffsetPointingToLineSeparator_unix();
    void testOffsetOffsetPointingToLineSeparator_dos();
    void testOffsetStartOfSecondLine();
    void testOffsetMultiByteCodePoint1();
    void testOffsetMultiByteCodePoint2();

private:
    Utils::FilePath createFile(const Utils::FilePath &yamlFilePath,
                               const Utils::FilePath &filePathToInject) const;
    Utils::FilePath filePath(const QString &fileName) const;

    CppEditor::Tests::TemporaryCopiedDir * const m_baseDir;
};

} // namespace ClangTools::Internal
