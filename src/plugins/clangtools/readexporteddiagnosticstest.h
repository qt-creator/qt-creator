/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
    QString createFile(const QString &yamlFilePath, const QString &filePathToInject) const;
    Utils::FilePath filePath(const QString &fileName) const;

    CppEditor::Tests::TemporaryCopiedDir * const m_baseDir;
    QString m_message;
};

} // namespace ClangTools::Internal
