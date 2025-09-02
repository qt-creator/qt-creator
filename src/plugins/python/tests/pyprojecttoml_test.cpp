// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "pyprojecttoml_test.h"
#include "../pyprojecttoml.h"

#include <QtTest/QtTest>

#include <coreplugin/textdocument.h>

#include <utils/filepath.h>

using namespace Utils;

namespace Python::Internal {

/*
    \brief Reads a file from the testfiles folder
    \param relativeFilePath The relative path to the file from the testfiles folder
    \returns The contents of the file
*/
static Result<QString> readTestFile(const QString &relativeFilePath)
{
    const auto filePath = FilePath::fromUserInput(":/unittests/Python/" + relativeFilePath);
    Core::BaseTextDocument projectFile;
    QString fileContent;
    const Core::BaseTextDocument::ReadResult result = projectFile.read(filePath, &fileContent);
    if (result.code != TextFileFormat::ReadSuccess)
        return ResultError(result.error);

    return fileContent;
}

void PyProjectTomlTest::testCorrectPyProjectParsing()
{
    const auto projectFile = Utils::FilePath::fromUserInput(":/unittests/Python/correct.toml");

    PyProjectTomlParseResult result = parsePyProjectToml(projectFile);

    QVERIFY(result.errors.empty());
    QCOMPARE(result.projectName, QString("CorrectTest"));
    QCOMPARE(result.projectFiles.size(), 1);
    QVERIFY(result.projectFiles.contains(QString("main.py")));
}

void PyProjectTomlTest::testNonExistingPyProjectParsing()
{
    const auto projectFile = Utils::FilePath::fromUserInput("nonexisting.toml");

    PyProjectTomlParseResult result = parsePyProjectToml(projectFile);

    QCOMPARE(result.errors.size(), 1);
    auto expectedError = PyProjectTomlError::FileNotFoundError("nonexisting.toml");
    QCOMPARE(result.errors.first(), expectedError);
}

void PyProjectTomlTest::testEmptyPyProjectParsing()
{
    const auto projectFile = Utils::FilePath::fromUserInput(":/unittests/Python/empty.toml");

    PyProjectTomlParseResult result = parsePyProjectToml(projectFile);

    QCOMPARE(result.errors.size(), 1);
    QCOMPARE(result.errors.first(), PyProjectTomlError::MissingNodeError("root", "project"));
}

void PyProjectTomlTest::testFilesBlankPyProjectParsing()
{
    const auto projectFile = Utils::FilePath::fromUserInput(":/unittests/Python/filesblank.toml");

    PyProjectTomlParseResult result = parsePyProjectToml(projectFile);

    QCOMPARE(result.errors.size(), 1);
    auto firstError = result.errors.first();
    QCOMPARE(firstError.type, PyProjectTomlErrorType::ParsingError);
    QCOMPARE(firstError.line, 5);
}

void PyProjectTomlTest::testFilesWrongTypePyProjectParsing()
{
    const auto projectFile = Utils::FilePath::fromUserInput(":/unittests/Python/fileswrongtype.toml");

    PyProjectTomlParseResult result = parsePyProjectToml(projectFile);

    QCOMPARE(result.errors.size(), 1);
    QCOMPARE(result.errors.first(), PyProjectTomlError::TypeError("files", "array", "integer", 5));
}

void PyProjectTomlTest::testFileMissingPyProjectParsing()
{
    // main.py exists in testfiles folder, but "non_existing_file.py" does not
    const auto projectFile = Utils::FilePath::fromUserInput(":/unittests/Python/filemissing.toml");

    PyProjectTomlParseResult result = parsePyProjectToml(projectFile);

    QCOMPARE(result.errors.size(), 1);
    QCOMPARE(result.errors.first(), PyProjectTomlError::FileNotFoundError("non_existing_file.py", 5));
}

void PyProjectTomlTest::testFileWrongTypePyProjectParsing()
{
    const auto projectFile = Utils::FilePath::fromUserInput(":/unittests/Python/filewrongtype.toml");

    PyProjectTomlParseResult result = parsePyProjectToml(projectFile);

    QCOMPARE(result.errors.size(), 1);
    auto expectedError = PyProjectTomlError::TypeError("file", "string", "integer", 5);
    QCOMPARE(result.errors.first(), expectedError);
}

void PyProjectTomlTest::testProjectEmptyPyProjectParsing()
{
    const auto projectFile = Utils::FilePath::fromUserInput(":/unittests/Python/projectempty.toml");

    PyProjectTomlParseResult result = parsePyProjectToml(projectFile);

    QCOMPARE(result.errors.size(), 1);
    auto expectedError = PyProjectTomlError::MissingNodeError("project", "name", 1);
    QCOMPARE(result.errors.first(), expectedError);
}

void PyProjectTomlTest::testProjectMissingPyProjectParsing()
{
    const auto projectFile = Utils::FilePath::fromUserInput(":/unittests/Python/projectmissing.toml");

    PyProjectTomlParseResult result = parsePyProjectToml(projectFile);

    QCOMPARE(result.errors.size(), 1);
    auto expectedError = PyProjectTomlError::MissingNodeError("root", "project");
    QCOMPARE(result.errors.first(), expectedError);
}

void PyProjectTomlTest::testProjectWrongTypePyProjectParsing()
{
    const auto projectFile = Utils::FilePath::fromUserInput(":/unittests/Python/projectwrongtype.toml");

    PyProjectTomlParseResult result = parsePyProjectToml(projectFile);

    QCOMPARE(result.errors.size(), 1);
    auto expectedError = PyProjectTomlError::TypeError("project", "table", "integer", 1);
    QCOMPARE(result.errors.first(), expectedError);
}

void PyProjectTomlTest::testPySideEmptyPyProjectParsing()
{
    const auto projectFile = Utils::FilePath::fromUserInput(":/unittests/Python/pysideempty.toml");

    PyProjectTomlParseResult result = parsePyProjectToml(projectFile);

    QCOMPARE(result.errors.size(), 1);
    auto expectedError = PyProjectTomlError::MissingNodeError("pyside6-project", "files", 4);
    QCOMPARE(result.errors.first(), expectedError);
}

void PyProjectTomlTest::testPySideMissingPyProjectParsing()
{
    const auto projectFile = Utils::FilePath::fromUserInput(":/unittests/Python/pysidemissing.toml");

    PyProjectTomlParseResult result = parsePyProjectToml(projectFile);

    QCOMPARE(result.errors.size(), 1);
    auto expectedError = PyProjectTomlError::MissingNodeError("tool", "pyside6-project", 4);
    QCOMPARE(result.errors.first(), expectedError);
}

void PyProjectTomlTest::testPySideWrongTypePyProjectParsing()
{
    const auto projectFile = Utils::FilePath::fromUserInput(":/unittests/Python/pysidewrongtype.toml");

    PyProjectTomlParseResult result = parsePyProjectToml(projectFile);

    QCOMPARE(result.errors.size(), 1);
    auto expectedError = PyProjectTomlError::TypeError("pyside6-project", "table", "array", 5);
    QCOMPARE(result.errors.first(), expectedError);
}

void PyProjectTomlTest::testToolEmptyPyProjectParsing()
{
    const auto projectFile = Utils::FilePath::fromUserInput(":/unittests/Python/toolempty.toml");

    PyProjectTomlParseResult result = parsePyProjectToml(projectFile);

    QCOMPARE(result.errors.size(), 1);
    auto expectedError = PyProjectTomlError::MissingNodeError("tool", "pyside6-project", 4);
    QCOMPARE(result.errors.first(), expectedError);
}

void PyProjectTomlTest::testToolMissingPyProjectParsing()
{
    const auto projectFile = Utils::FilePath::fromUserInput(":/unittests/Python/toolmissing.toml");

    PyProjectTomlParseResult result = parsePyProjectToml(projectFile);

    QCOMPARE(result.errors.size(), 1);
    auto expectedError = PyProjectTomlError::MissingNodeError("root", "tool");
    QCOMPARE(result.errors.first(), expectedError);
}

void PyProjectTomlTest::testToolWrongTypePyProjectParsing()
{
    const auto projectFile = Utils::FilePath::fromUserInput(":/unittests/Python/toolwrongtype.toml");

    PyProjectTomlParseResult result = parsePyProjectToml(projectFile);

    QCOMPARE(result.errors.size(), 1);
    auto expectedError = PyProjectTomlError::TypeError("tool", "table", "integer", 1);
    QCOMPARE(result.errors.first(), expectedError);
}

void PyProjectTomlTest::testUpdatePyProject()
{
    const auto projectFileContents = readTestFile("correct.toml");
    QVERIFY2(projectFileContents, "Can not read correct.toml");
    const QStringList
        newProjectFiles{"folder/file_in_folder.py", "new_file.py", "main.py", "another_file.py"};
    const auto newProjectFileContent
        = updatePyProjectTomlContent(projectFileContents.value(), newProjectFiles);
    QVERIFY(newProjectFileContent);

    const auto expectedProjectFileContent = readTestFile("correct_after_update.toml");
    QCOMPARE(newProjectFileContent.value(), expectedProjectFileContent);
}

QObject *createPyProjectTomlTest()
{
    return new PyProjectTomlTest;
}

} // namespace Python::Internal

#endif // WITH_TESTS
