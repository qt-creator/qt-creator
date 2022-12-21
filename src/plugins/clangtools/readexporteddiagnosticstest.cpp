// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "readexporteddiagnosticstest.h"

#include "clangtoolslogfilereader.h"

#include <cppeditor/cpptoolstestcase.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QtTest>

using namespace CppEditor::Tests;
using namespace Debugger;
using namespace Utils;

namespace ClangTools::Internal {

const char asciiWord[] = "FOO";
const char asciiMultiLine[] = "FOO\nBAR";
const char asciiMultiLine_dos[] = "FOO\r\nBAR";
const char nonAsciiMultiLine[] = "\xc3\xbc" "\n"
                                 "\xe4\xba\x8c" "\n"
                                 "\xf0\x90\x8c\x82" "X";

ReadExportedDiagnosticsTest::ReadExportedDiagnosticsTest()
    : m_baseDir(new TemporaryCopiedDir(":/clangtools/unit-tests/exported-diagnostics")) {}

ReadExportedDiagnosticsTest::~ReadExportedDiagnosticsTest() { delete m_baseDir; }

void ReadExportedDiagnosticsTest::initTestCase() { QVERIFY(m_baseDir->isValid()); }
void ReadExportedDiagnosticsTest::init() { m_message.clear(); }

void ReadExportedDiagnosticsTest::testNonExistingFile()
{
    const Diagnostics diags = readExportedDiagnostics("nonExistingFile.yaml", {}, &m_message);
    QVERIFY(diags.isEmpty());
    QVERIFY(!m_message.isEmpty());
}

void ReadExportedDiagnosticsTest::testEmptyFile()
{
    const Diagnostics diags = readExportedDiagnostics(filePath("empty.yaml"), {}, &m_message);
    QVERIFY(diags.isEmpty());
    QVERIFY2(m_message.isEmpty(), qPrintable(m_message));
}

void ReadExportedDiagnosticsTest::testUnexpectedFileContents()
{
    const Diagnostics diags = readExportedDiagnostics(filePath("tidy.modernize-use-nullptr.cpp"),
                                                      {}, &m_message);
    QVERIFY(!m_message.isEmpty());
    QVERIFY(diags.isEmpty());
}

static QString appendYamlSuffix(const char *filePathFragment)
{
    const QString yamlSuffix = QLatin1String(Utils::HostOsInfo::isWindowsHost()
                                             ? "_win.yaml" : ".yaml");
    return filePathFragment + yamlSuffix;
}

void ReadExportedDiagnosticsTest::testTidy()
{
    const FilePath sourceFile = filePath("tidy.modernize-use-nullptr.cpp");
    const QString exportedFile = createFile(
                filePath(appendYamlSuffix("tidy.modernize-use-nullptr")).toString(),
                sourceFile.toString());
    Diagnostic expectedDiag;
    expectedDiag.name = "modernize-use-nullptr";
    expectedDiag.location = {sourceFile, 2, 25};
    expectedDiag.description = "use nullptr [modernize-use-nullptr]";
    expectedDiag.type = "warning";
    expectedDiag.hasFixits = true;
    expectedDiag.explainingSteps = {
        ExplainingStep{"nullptr",
                       expectedDiag.location,
                       {expectedDiag.location, {sourceFile, 2, 26}},
                       true}};
    const Diagnostics diags = readExportedDiagnostics(Utils::FilePath::fromString(exportedFile),
                                                      {}, &m_message);

    QVERIFY2(m_message.isEmpty(), qPrintable(m_message));
    QCOMPARE(diags, {expectedDiag});
}

void ReadExportedDiagnosticsTest::testAcceptDiagsFromFilePaths_None()
{
    const QString sourceFile = filePath("tidy.modernize-use-nullptr.cpp").toString();
    const QString exportedFile = createFile(filePath("tidy.modernize-use-nullptr.yaml").toString(),
                                            sourceFile);
    const auto acceptNone = [](const Utils::FilePath &) { return false; };
    const Diagnostics diags = readExportedDiagnostics(FilePath::fromString(exportedFile),
                                                      acceptNone, &m_message);
    QVERIFY2(m_message.isEmpty(), qPrintable(m_message));
    QVERIFY(diags.isEmpty());
}

// Diagnostics from clang (static) analyzer passed through via clang-tidy
void ReadExportedDiagnosticsTest::testTidy_ClangAnalyzer()
{
    const FilePath sourceFile = filePath("clang-analyzer.dividezero.cpp");
    const QString exportedFile = createFile(
                filePath(appendYamlSuffix("clang-analyzer.dividezero")).toString(),
                sourceFile.toString());
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
    const Diagnostics diags = readExportedDiagnostics(Utils::FilePath::fromString(exportedFile),
                                                      {}, &m_message);
    QVERIFY2(m_message.isEmpty(), qPrintable(m_message));
    QCOMPARE(diags, {expectedDiag});
}

void ReadExportedDiagnosticsTest::testClazy()
{
    const FilePath sourceFile = filePath("clazy.qgetenv.cpp");
    const QString exportedFile = createFile(filePath(appendYamlSuffix("clazy.qgetenv")).toString(),
                                            sourceFile.toString());
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
    const Diagnostics diags = readExportedDiagnostics(Utils::FilePath::fromString(exportedFile),
                                                      {}, &m_message);
    QVERIFY2(m_message.isEmpty(), qPrintable(m_message));
    QCOMPARE(diags, {expectedDiag});
}

void ReadExportedDiagnosticsTest::testOffsetInvalidText()
{
    QVERIFY(!byteOffsetInUtf8TextToLineColumn(nullptr, 0));
}

void ReadExportedDiagnosticsTest::testOffsetInvalidOffset_EmptyInput()
{
    QVERIFY(!byteOffsetInUtf8TextToLineColumn("", 0));
}

void ReadExportedDiagnosticsTest::testOffsetInvalidOffset_Before()
{
    QVERIFY(!byteOffsetInUtf8TextToLineColumn(asciiWord, -1));
}

void ReadExportedDiagnosticsTest::testOffsetInvalidOffset_After()
{
    QVERIFY(!byteOffsetInUtf8TextToLineColumn(asciiWord, 3));
}

void ReadExportedDiagnosticsTest::testOffsetInvalidOffset_NotFirstByteOfMultiByte()
{
    QVERIFY(!byteOffsetInUtf8TextToLineColumn(nonAsciiMultiLine, 1));
}

void ReadExportedDiagnosticsTest::testOffsetStartOfFirstLine()
{
    const auto info = byteOffsetInUtf8TextToLineColumn(asciiWord, 0);
    QVERIFY(info);
    QCOMPARE(info->line, 1);
    QCOMPARE(info->column, 1);
}

void ReadExportedDiagnosticsTest::testOffsetEndOfFirstLine()
{
    const auto info = byteOffsetInUtf8TextToLineColumn(asciiWord, 2);
    QVERIFY(info);
    QCOMPARE(info->line, 1);
    QCOMPARE(info->column, 3);
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
// The newline in the if-line is considered as column 11, which is normally not visible in the
// editor.
void ReadExportedDiagnosticsTest::testOffsetOffsetPointingToLineSeparator_unix()
{
    const auto info = byteOffsetInUtf8TextToLineColumn(asciiMultiLine, 3);
    QVERIFY(info);
    QCOMPARE(info->line, 1);
    QCOMPARE(info->column, 4);
}

// For a file with dos style line endings ("\r\n"), clang-tidy points to '\r'.
void ReadExportedDiagnosticsTest::testOffsetOffsetPointingToLineSeparator_dos()
{
    const auto info = byteOffsetInUtf8TextToLineColumn(asciiMultiLine_dos, 3);
    QVERIFY(info);
    QCOMPARE(info->line, 1);
    QCOMPARE(info->column, 4);
}

void ReadExportedDiagnosticsTest::testOffsetStartOfSecondLine()
{
    const auto info = byteOffsetInUtf8TextToLineColumn(asciiMultiLine, 4);
    QVERIFY(info);
    QCOMPARE(info->line, 2);
    QCOMPARE(info->column, 1);
}

void ReadExportedDiagnosticsTest::testOffsetMultiByteCodePoint1()
{
    const auto info = byteOffsetInUtf8TextToLineColumn(nonAsciiMultiLine, 3);
    QVERIFY(info);
    QCOMPARE(info->line, 2);
    QCOMPARE(info->column, 1);
}

void ReadExportedDiagnosticsTest::testOffsetMultiByteCodePoint2()
{
    const auto info = byteOffsetInUtf8TextToLineColumn(nonAsciiMultiLine, 11);
    QVERIFY(info);
    QCOMPARE(info->line, 3);
    QCOMPARE(info->column, 2);
}

// Replace FILE_PATH with a real absolute file path in the *.yaml files.
QString ReadExportedDiagnosticsTest::createFile(const QString &yamlFilePath,
                                                const QString &filePathToInject) const
{
    QTC_ASSERT(QDir::isAbsolutePath(filePathToInject), return QString());
    const Utils::FilePath newFileName = FilePath::fromString(m_baseDir->absolutePath(
                QFileInfo(yamlFilePath).fileName().toLocal8Bit()));

    Utils::FileReader reader;
    if (QTC_GUARD(reader.fetch(Utils::FilePath::fromString(yamlFilePath),
                               QIODevice::ReadOnly | QIODevice::Text))) {
        QByteArray contents = reader.data();
        contents.replace("FILE_PATH", filePathToInject.toLocal8Bit());

        Utils::FileSaver fileSaver(newFileName, QIODevice::WriteOnly | QIODevice::Text);
        QTC_CHECK(fileSaver.write(contents));
        QTC_CHECK(fileSaver.finalize());
    }

    return newFileName.toString();
}

FilePath ReadExportedDiagnosticsTest::filePath(const QString &fileName) const
{
    return FilePath::fromString(m_baseDir->absolutePath(fileName.toUtf8()));
}

} // namespace ClangTools::Internal
