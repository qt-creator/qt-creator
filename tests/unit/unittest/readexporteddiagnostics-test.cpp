/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "googletest.h"

#include <clangtools/clangtoolslogfilereader.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#define TESTDATA TESTDATA_DIR "/clangtools/"

using namespace ClangTools::Internal;
using Debugger::DiagnosticLocation;

namespace {

class ReadExportedDiagnostics : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_TRUE(temporaryDir.isValid());
    }

    // Replace FILE_PATH with a real absolute file path in the *.yaml files.
    QString createFile(const QString &yamlFilePath, const QString &filePathToInject)
    {
        QTC_ASSERT(QDir::isAbsolutePath(filePathToInject), return QString());
        const QString newFileName = temporaryDir.filePath(QFileInfo(yamlFilePath).fileName());

        Utils::FileReader reader;
        if (QTC_GUARD(reader.fetch(yamlFilePath, QIODevice::ReadOnly | QIODevice::Text))) {
            QByteArray contents = reader.data();
            contents.replace("FILE_PATH", filePathToInject.toLocal8Bit());

            Utils::FileSaver fileSaver(newFileName, QIODevice::WriteOnly | QIODevice::Text);
            QTC_CHECK(fileSaver.write(contents));
            QTC_CHECK(fileSaver.finalize());
        }

        return newFileName;
    }

protected:
    QString errorMessage;
    Utils::TemporaryDirectory temporaryDir{"clangtools-tests-XXXXXX"};
};

TEST_F(ReadExportedDiagnostics, NotExistingFile)
{
    Diagnostics diags = readExportedDiagnostics(Utils::FilePath::fromString("notExistingFile.yaml"),
                                                {},
                                                &errorMessage);

    ASSERT_THAT(diags, IsEmpty());
    ASSERT_FALSE(errorMessage.isEmpty());
}

TEST_F(ReadExportedDiagnostics, EmptyFile)
{
    Diagnostics diags = readExportedDiagnostics(Utils::FilePath::fromString(TESTDATA "empty.yaml"),
                                                {},
                                                &errorMessage);

    ASSERT_THAT(diags, IsEmpty());
    ASSERT_TRUE(errorMessage.isEmpty());
}

TEST_F(ReadExportedDiagnostics, UnexpectedFileContents)
{
    const QString sourceFile = TESTDATA "tidy.modernize-use-nullptr.cpp";

    Diagnostics diags = readExportedDiagnostics(Utils::FilePath::fromString(sourceFile),
                                                {},
                                                &errorMessage);

    ASSERT_FALSE(errorMessage.isEmpty());
    ASSERT_THAT(diags, IsEmpty());
}

TEST_F(ReadExportedDiagnostics, Tidy)
{
    const QString sourceFile = TESTDATA "tidy.modernize-use-nullptr.cpp";
    const QString exportedFile = createFile(TESTDATA "tidy.modernize-use-nullptr.yaml", sourceFile);
    Diagnostic expectedDiag;
    expectedDiag.name = "modernize-use-nullptr";
    expectedDiag.location = {sourceFile, 2, 25};
    expectedDiag.description = "use nullptr [modernize-use-nullptr]";
    expectedDiag.type = "warning";
    expectedDiag.hasFixits = true;
    expectedDiag.explainingSteps = {ExplainingStep{"nullptr",
                                                   expectedDiag.location,
                                                   {expectedDiag.location, {sourceFile, 2, 26}},
                                                   true}};

    Diagnostics diags = readExportedDiagnostics(Utils::FilePath::fromString(exportedFile),
                                                {},
                                                &errorMessage);

    ASSERT_TRUE(errorMessage.isEmpty());
    ASSERT_THAT(diags, ElementsAre(expectedDiag));
}

TEST_F(ReadExportedDiagnostics, AcceptDiagsFromFilePaths_None)
{
    const QString sourceFile = TESTDATA "tidy.modernize-use-nullptr.cpp";
    const QString exportedFile = createFile(TESTDATA "tidy.modernize-use-nullptr.yaml", sourceFile);
    const auto acceptNone = [](const Utils::FilePath &) { return false; };

    Diagnostics diags = readExportedDiagnostics(Utils::FilePath::fromString(exportedFile),
                                                acceptNone,
                                                &errorMessage);

    ASSERT_TRUE(errorMessage.isEmpty());
    ASSERT_THAT(diags, IsEmpty());
}

// Diagnostics from clang passed through via clang-tidy
TEST_F(ReadExportedDiagnostics, Tidy_Clang)
{
    const QString sourceFile = TESTDATA "clang.unused-parameter.cpp";
    const QString exportedFile = createFile(TESTDATA "clang.unused-parameter.yaml", sourceFile);
    Diagnostic expectedDiag;
    expectedDiag.name = "clang-diagnostic-unused-parameter";
    expectedDiag.location = {sourceFile, 4, 12};
    expectedDiag.description = "unused parameter 'g' [clang-diagnostic-unused-parameter]";
    expectedDiag.type = "warning";
    expectedDiag.hasFixits = false;

    Diagnostics diags = readExportedDiagnostics(Utils::FilePath::fromString(exportedFile),
                                                {},
                                                &errorMessage);

    ASSERT_TRUE(errorMessage.isEmpty());
    ASSERT_THAT(diags, ElementsAre(expectedDiag));
}

// Diagnostics from clang (static) analyzer passed through via clang-tidy
TEST_F(ReadExportedDiagnostics, Tidy_ClangAnalyzer)
{
    const QString sourceFile = TESTDATA "clang-analyzer.dividezero.cpp";
    const QString exportedFile = createFile(TESTDATA "clang-analyzer.dividezero.yaml", sourceFile);
    Diagnostic expectedDiag;
    expectedDiag.name = "clang-analyzer-core.DivideZero";
    expectedDiag.location = {sourceFile, 4, 15};
    expectedDiag.description = "Division by zero [clang-analyzer-core.DivideZero]";
    expectedDiag.type = "warning";
    expectedDiag.hasFixits = false;
    expectedDiag.explainingSteps = {
        ExplainingStep{"Assuming 'z' is equal to 0",
                       {sourceFile, 3, 7},
                       {},
                       false,
        },
        ExplainingStep{"Taking true branch",
                       {sourceFile, 3, 3},
                       {},
                       false,
        },
        ExplainingStep{"Division by zero",
                       {sourceFile, 4, 15},
                       {},
                       false,
        },
    };

    Diagnostics diags = readExportedDiagnostics(Utils::FilePath::fromString(exportedFile),
                                                {},
                                                &errorMessage);

    ASSERT_TRUE(errorMessage.isEmpty());
    ASSERT_THAT(diags, ElementsAre(expectedDiag));
}

TEST_F(ReadExportedDiagnostics, Clazy)
{
    const QString sourceFile = TESTDATA "clazy.qgetenv.cpp";
    const QString exportedFile = createFile(TESTDATA "clazy.qgetenv.yaml", sourceFile);
    Diagnostic expectedDiag;
    expectedDiag.name = "clazy-qgetenv";
    expectedDiag.location = {sourceFile, 7, 5};
    expectedDiag.description = "qgetenv().isEmpty() allocates. Use qEnvironmentVariableIsEmpty() instead [clazy-qgetenv]";
    expectedDiag.type = "warning";
    expectedDiag.hasFixits = true;
    expectedDiag.explainingSteps = {
        ExplainingStep{"qEnvironmentVariableIsEmpty",
                       expectedDiag.location,
                       {expectedDiag.location, {sourceFile, 7, 12}},
                       true
        },
        ExplainingStep{")",
                       {sourceFile, 7, 18},
                       {{sourceFile, 7, 18}, {sourceFile, 7, 29}},
                       true},
    };

    Diagnostics diags = readExportedDiagnostics(Utils::FilePath::fromString(exportedFile),
                                                {},
                                                &errorMessage);

    ASSERT_TRUE(errorMessage.isEmpty());
    ASSERT_THAT(diags, ElementsAre(expectedDiag));
}

class ByteOffsetInUtf8TextToLineColumn : public ::testing::Test
{
protected:
    const char *empty = "";
    const char *asciiWord = "FOO";
    const char *asciiMultiLine = "FOO\nBAR";
    const char *asciiMultiLine_dos = "FOO\r\nBAR";
    const char *asciiEmptyMultiLine = "\n\n";
    // U+00FC  - 2 code units in UTF8, 1 in UTF16 - LATIN SMALL LETTER U WITH DIAERESIS
    // U+4E8C  - 3 code units in UTF8, 1 in UTF16 - CJK UNIFIED IDEOGRAPH-4E8C
    // U+10302 - 4 code units in UTF8, 2 in UTF16 - OLD ITALIC LETTER KE
    const char *nonAsciiMultiLine = "\xc3\xbc" "\n"
                                    "\xe4\xba\x8c" "\n"
                                    "\xf0\x90\x8c\x82" "X";

    // Convenience
    const char *text = nullptr;
};

TEST_F(ByteOffsetInUtf8TextToLineColumn, InvalidText)
{
    ASSERT_FALSE(byteOffsetInUtf8TextToLineColumn(nullptr, 0));
}

TEST_F(ByteOffsetInUtf8TextToLineColumn, InvalidOffset_EmptyInput)
{
    ASSERT_FALSE(byteOffsetInUtf8TextToLineColumn(empty, 0));
}

TEST_F(ByteOffsetInUtf8TextToLineColumn, InvalidOffset_Before)
{
    ASSERT_FALSE(byteOffsetInUtf8TextToLineColumn(asciiWord, -1));
}

TEST_F(ByteOffsetInUtf8TextToLineColumn, InvalidOffset_After)
{
    ASSERT_FALSE(byteOffsetInUtf8TextToLineColumn(asciiWord, 3));
}

TEST_F(ByteOffsetInUtf8TextToLineColumn, InvalidOffset_NotFirstByteOfMultiByte)
{
    ASSERT_FALSE(byteOffsetInUtf8TextToLineColumn(nonAsciiMultiLine, 1));
}

TEST_F(ByteOffsetInUtf8TextToLineColumn, StartOfFirstLine)
{
    auto info = byteOffsetInUtf8TextToLineColumn(asciiWord, 0);

    ASSERT_TRUE(info);
    ASSERT_THAT(info->line, Eq(1));
    ASSERT_THAT(info->column, Eq(1));
}

TEST_F(ByteOffsetInUtf8TextToLineColumn, EndOfFirstLine)
{
    auto info = byteOffsetInUtf8TextToLineColumn(asciiWord, 2);

    ASSERT_TRUE(info);
    ASSERT_THAT(info->line, Eq(1));
    ASSERT_THAT(info->column, Eq(3));
}

// The invocation
//
//   clang-tidy "-checks=-*,readability-braces-around-statements" /path/to/file
//
// for the code
//
//   void f(bool b)
//   {
//       if (b)
//           f(b);
//   }
//
// emits
//
//   3:11: warning: statement should be inside braces [readability-braces-around-statements]
//
// The new line in the if-line is considered as column 11, which is normally not visible in the
// editor.
TEST_F(ByteOffsetInUtf8TextToLineColumn, OffsetPointingToLineSeparator_unix)
{
    auto info = byteOffsetInUtf8TextToLineColumn(asciiMultiLine, 3);

    ASSERT_TRUE(info);
    ASSERT_THAT(info->line, Eq(1));
    ASSERT_THAT(info->column, Eq(4));
}

// For a file with dos style line endings ("\r\n"), clang-tidy points to '\r'.
TEST_F(ByteOffsetInUtf8TextToLineColumn, OffsetPointingToLineSeparator_dos)
{
    auto info = byteOffsetInUtf8TextToLineColumn(asciiMultiLine_dos, 3);

    ASSERT_TRUE(info);
    ASSERT_THAT(info->line, Eq(1));
    ASSERT_THAT(info->column, Eq(4));
}

TEST_F(ByteOffsetInUtf8TextToLineColumn, StartOfSecondLine)
{
    auto info = byteOffsetInUtf8TextToLineColumn(asciiMultiLine, 4);

    ASSERT_TRUE(info);
    ASSERT_THAT(info->line, Eq(2));
    ASSERT_THAT(info->column, Eq(1));
}

TEST_F(ByteOffsetInUtf8TextToLineColumn, MultiByteCodePoint1)
{
    auto info = byteOffsetInUtf8TextToLineColumn(nonAsciiMultiLine, 3);

    ASSERT_TRUE(info);
    ASSERT_THAT(info->line, Eq(2));
    ASSERT_THAT(info->column, Eq(1));
}

TEST_F(ByteOffsetInUtf8TextToLineColumn, MultiByteCodePoint2)
{
    auto info = byteOffsetInUtf8TextToLineColumn(nonAsciiMultiLine, 11);

    ASSERT_TRUE(info);
    ASSERT_THAT(info->line, Eq(3));
    ASSERT_THAT(info->column, Eq(2));
}

} // namespace

#undef TESTDATA
