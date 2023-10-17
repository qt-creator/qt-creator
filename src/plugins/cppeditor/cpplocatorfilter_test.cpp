// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpplocatorfilter_test.h"

#include "cpptoolstestcase.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/locator/locatorfiltertest.h>
#include <coreplugin/testdatadir.h>
#include <utils/environment.h>

#include <QDebug>
#include <QFileInfo>
#include <QtTest>

using namespace Core;
using namespace Core::Tests;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

const bool debug = qtcEnvironmentVariable("QTC_DEBUG_CPPLOCATORFILTERTESTCASE") == "1";

QTC_DECLARE_MYTESTDATADIR("../../../tests/cpplocators/")

class CppLocatorFilterTestCase : public CppEditor::Tests::TestCase
{
public:
    CppLocatorFilterTestCase(const QList<LocatorMatcherTask> &matchers,
                             const QString &fileName,
                             const QString &searchText,
                             const ResultDataList &expectedResults)
    {
        QVERIFY(succeededSoFar());
        QVERIFY(!fileName.isEmpty());
        QVERIFY(garbageCollectGlobalSnapshot());

        QVERIFY(parseFiles(fileName));
        const LocatorFilterEntries entries = LocatorMatcher::runBlocking(matchers, searchText);
        QVERIFY(garbageCollectGlobalSnapshot());
        const ResultDataList results = ResultData::fromFilterEntryList(entries);
        if (debug) {
            ResultData::printFilterEntries(expectedResults, "Expected:");
            ResultData::printFilterEntries(results, "Results:");
        }
        QVERIFY(!results.isEmpty());
        QCOMPARE(results, expectedResults);
    }
};

class CppCurrentDocumentFilterTestCase : public CppEditor::Tests::TestCase
{
public:
    CppCurrentDocumentFilterTestCase(const FilePath &filePath,
                                     const QList<LocatorMatcherTask> &matchers,
                                     const ResultDataList &expectedResults,
                                     const QString &searchText = QString())
    {
        QVERIFY(succeededSoFar());
        QVERIFY(!filePath.isEmpty());

        QVERIFY(DocumentModel::openedDocuments().isEmpty());
        QVERIFY(garbageCollectGlobalSnapshot());

        const auto editor = EditorManager::openEditor(filePath);
        QVERIFY(editor);

        QVERIFY(waitForFileInGlobalSnapshot(filePath));
        const LocatorFilterEntries entries = LocatorMatcher::runBlocking(matchers, searchText);
        QVERIFY(closeEditorWithoutGarbageCollectorInvocation(editor));
        QCoreApplication::processEvents();
        QVERIFY(DocumentModel::openedDocuments().isEmpty());
        QVERIFY(garbageCollectGlobalSnapshot());
        const ResultDataList results = ResultData::fromFilterEntryList(entries);
        if (debug) {
            ResultData::printFilterEntries(expectedResults, "Expected:");
            ResultData::printFilterEntries(results, "Results:");
        }
        QVERIFY(!results.isEmpty());
        QCOMPARE(results, expectedResults);
    }
};

} // anonymous namespace

void LocatorFilterTest::testLocatorFilter()
{
    QFETCH(QString, testFile);
    QFETCH(MatcherType, matcherType);
    QFETCH(QString, searchText);
    QFETCH(ResultDataList, expectedResults);

    Tests::VerifyCleanCppModelManager verify;
    CppLocatorFilterTestCase(LocatorMatcher::matchers(matcherType), testFile, searchText,
                             expectedResults);
}

void LocatorFilterTest::testLocatorFilter_data()
{
    QTest::addColumn<QString>("testFile");
    QTest::addColumn<MatcherType>("matcherType");
    QTest::addColumn<QString>("searchText");
    QTest::addColumn<ResultDataList>("expectedResults");

    MyTestDataDir testDirectory("testdata_basic");
    QString testFile = testDirectory.file("file1.cpp");
    testFile[0] = testFile[0].toLower(); // Ensure Windows path sorts after scope names.
    const QString objTestFile = testDirectory.file("file1.mm");
    const QString testFileShort = FilePath::fromString(testFile).shortNativePath();
    const QString objTestFileShort = FilePath::fromString(objTestFile).shortNativePath();

    QTest::newRow("CppFunctionsFilter")
        << testFile
        << MatcherType::Functions
        << "function"
        << ResultDataList{
               ResultData("functionDefinedInClass(bool, int)",
                          "<anonymous namespace>::MyClass (file1.cpp)"),
               ResultData("functionDefinedInClass(bool, int)", "MyClass (file1.cpp)"),
               ResultData("functionDefinedInClass(bool, int)",
                          "MyNamespace::MyClass (file1.cpp)"),
               ResultData("functionDefinedOutSideClass(char)",
                          "<anonymous namespace>::MyClass (file1.cpp)"),
               ResultData("functionDefinedOutSideClass(char)", "MyClass (file1.cpp)"),
               ResultData("functionDefinedOutSideClass(char)",
                          "MyNamespace::MyClass (file1.cpp)"),
               ResultData("functionDefinedOutSideClassAndNamespace(float)",
                          "MyNamespace::MyClass (file1.cpp)"),
               ResultData("myFunction(bool, int)", "<anonymous namespace> (file1.cpp)"),
               ResultData("myFunction(bool, int)", "MyNamespace (file1.cpp)"),
               ResultData("myFunction(bool, int)", testFileShort)
           };

    QTest::newRow("CppFunctionsFilter-Sorting")
        << testFile
        << MatcherType::Functions
        << "pos"
        << ResultDataList{
               ResultData("positiveNumber()", testFileShort),
               ResultData("somePositionWithin()", testFileShort),
               ResultData("pointOfService()", testFileShort),
               ResultData("matchArgument(Pos)", testFileShort)
           };

    QTest::newRow("CppFunctionsFilter-arguments")
        << testFile
        << MatcherType::Functions
        << "function*bool"
        << ResultDataList{
               ResultData("functionDefinedInClass(bool, int)",
                          "<anonymous namespace>::MyClass (file1.cpp)"),
               ResultData("functionDefinedInClass(bool, int)",
                          "MyClass (file1.cpp)"),
               ResultData("functionDefinedInClass(bool, int)",
                          "MyNamespace::MyClass (file1.cpp)"),
               ResultData("myFunction(bool, int)", "<anonymous namespace> (file1.cpp)"),
               ResultData("myFunction(bool, int)", "MyNamespace (file1.cpp)"),
               ResultData("myFunction(bool, int)", testFileShort)
           };

    QTest::newRow("CppFunctionsFilter-WithNamespacePrefix")
        << testFile
        << MatcherType::Functions
        << "mynamespace::"
        << ResultDataList{
               ResultData("MyClass()", "MyNamespace::MyClass (file1.cpp)"),
               ResultData("functionDefinedInClass(bool, int)",
                          "MyNamespace::MyClass (file1.cpp)"),
               ResultData("functionDefinedOutSideClass(char)",
                          "MyNamespace::MyClass (file1.cpp)"),
               ResultData("functionDefinedOutSideClassAndNamespace(float)",
                          "MyNamespace::MyClass (file1.cpp)"),
               ResultData("myFunction(bool, int)", "MyNamespace (file1.cpp)"),
           };

    QTest::newRow("CppFunctionsFilter-WithClassPrefix")
        << testFile
        << MatcherType::Functions
        << "MyClass::func"
        << ResultDataList{
               ResultData("functionDefinedInClass(bool, int)",
                          "<anonymous namespace>::MyClass (file1.cpp)"),
               ResultData("functionDefinedInClass(bool, int)",
                          "MyClass (file1.cpp)"),
               ResultData("functionDefinedInClass(bool, int)",
                          "MyNamespace::MyClass (file1.cpp)"),
               ResultData("functionDefinedOutSideClass(char)",
                          "<anonymous namespace>::MyClass (file1.cpp)"),
               ResultData("functionDefinedOutSideClass(char)",
                          "MyClass (file1.cpp)"),
               ResultData("functionDefinedOutSideClass(char)",
                          "MyNamespace::MyClass (file1.cpp)"),
               ResultData("functionDefinedOutSideClassAndNamespace(float)",
                          "MyNamespace::MyClass (file1.cpp)"),
           };

    QTest::newRow("CppClassesFilter")
        << testFile
        << MatcherType::Classes
        << "myclass"
        << ResultDataList{
               ResultData("MyClass", "<anonymous namespace>"),
               ResultData("MyClass", "MyNamespace"),
               ResultData("MyClass", testFileShort)
           };

    QTest::newRow("CppClassesFilter-WithNamespacePrefix")
        << testFile
        << MatcherType::Classes
        << "mynamespace::"
        << ResultDataList{
               ResultData("MyClass", "MyNamespace")
           };

    // all symbols in the left column are expected to be fully qualified.
    QTest::newRow("CppLocatorFilter-filtered")
        << testFile
        << MatcherType::AllSymbols
        << "my"
        << ResultDataList{
               ResultData("MyClass", "<anonymous namespace>"),
               ResultData("MyClass", "<anonymous namespace>::MyClass"),
               ResultData("MyClass", "MyClass"),
               ResultData("MyClass", "MyNamespace"),
               ResultData("MyClass", "MyNamespace::MyClass"),
               ResultData("MyClass", testFileShort),
               ResultData("MyEnum", "<anonymous namespace>"),
               ResultData("MyEnum", "MyNamespace"),
               ResultData("MyEnum", testFileShort),
               ResultData("myFunction", "(bool, int)"),
               ResultData("myFunction", "<anonymous namespace>"),
               ResultData("myFunction", "MyNamespace"),
           };

    QTest::newRow("CppClassesFilter-ObjC")
        << objTestFile
        << MatcherType::Classes
        << "M"
        << ResultDataList{
               ResultData("MyClass", objTestFileShort),
               ResultData("MyClass", objTestFileShort),
               ResultData("MyClass", objTestFileShort),
               ResultData("MyProtocol", objTestFileShort),
           };

    QTest::newRow("CppFunctionsFilter-ObjC")
        << objTestFile
        << MatcherType::Functions
        << "M"
        << ResultDataList{
               ResultData("anotherMethod", "MyClass (file1.mm)"),
               ResultData("anotherMethod:", "MyClass (file1.mm)"),
               ResultData("someMethod", "MyClass (file1.mm)")
           };
}

void LocatorFilterTest::testCurrentDocumentFilter()
{
    MyTestDataDir testDirectory("testdata_basic");
    const FilePath testFile = testDirectory.filePath("file1.cpp");

    auto expectedResults = ResultDataList{
        ResultData("int myVariable", ""),
        ResultData("myFunction(bool, int)", ""),
        ResultData("Pos", ""),
        ResultData("somePositionWithin()", ""),
        ResultData("pointOfService()", ""),
        ResultData("matchArgument(Pos)", ""),
        ResultData("positiveNumber()", ""),
        ResultData("MyEnum", ""),
        ResultData("int V1", "MyEnum"),
        ResultData("int V2", "MyEnum"),
        ResultData("MyClass", ""),
        ResultData("MyClass()", "MyClass"),
        ResultData("functionDeclaredOnly()", "MyClass"),
        ResultData("functionDefinedInClass(bool, int)", "MyClass"),
        ResultData("functionDefinedOutSideClass(char)", "MyClass"),
        ResultData("int myVariable", "MyNamespace"),
        ResultData("myFunction(bool, int)", "MyNamespace"),
        ResultData("MyEnum", "MyNamespace"),
        ResultData("int V1", "MyNamespace::MyEnum"),
        ResultData("int V2", "MyNamespace::MyEnum"),
        ResultData("MyClass", "MyNamespace"),
        ResultData("MyClass()", "MyNamespace::MyClass"),
        ResultData("functionDeclaredOnly()", "MyNamespace::MyClass"),
        ResultData("functionDefinedInClass(bool, int)", "MyNamespace::MyClass"),
        ResultData("functionDefinedOutSideClass(char)", "MyNamespace::MyClass"),
        ResultData("functionDefinedOutSideClassAndNamespace(float)",
                   "MyNamespace::MyClass"),
        ResultData("int myVariable", "<anonymous namespace>"),
        ResultData("myFunction(bool, int)", "<anonymous namespace>"),
        ResultData("MyEnum", "<anonymous namespace>"),
        ResultData("int V1", "<anonymous namespace>::MyEnum"),
        ResultData("int V2", "<anonymous namespace>::MyEnum"),
        ResultData("MyClass", "<anonymous namespace>"),
        ResultData("MyClass()", "<anonymous namespace>::MyClass"),
        ResultData("functionDeclaredOnly()", "<anonymous namespace>::MyClass"),
        ResultData("functionDefinedInClass(bool, int)", "<anonymous namespace>::MyClass"),
        ResultData("functionDefinedOutSideClass(char)", "<anonymous namespace>::MyClass"),
        ResultData("main()", ""),
    };

    Tests::VerifyCleanCppModelManager verify;
    CppCurrentDocumentFilterTestCase(
        testFile, LocatorMatcher::matchers(MatcherType::CurrentDocumentSymbols), expectedResults);
}

void LocatorFilterTest::testCurrentDocumentHighlighting()
{
    MyTestDataDir testDirectory("testdata_basic");
    const FilePath testFile = testDirectory.filePath("file1.cpp");

    const QString searchText = "pos";
    const ResultDataList expectedResults{
        ResultData("Pos", "",
                   "~~~"),
        ResultData("pointOfService()", "",
                   "~    ~ ~        "),
        ResultData("positiveNumber()", "",
                   "~~~             "),
        ResultData("somePositionWithin()", "",
                   "    ~~~             "),
        ResultData("matchArgument(Pos)", "",
                   "              ~~~ ")
       };

    Tests::VerifyCleanCppModelManager verify;
    CppCurrentDocumentFilterTestCase(testFile,
        LocatorMatcher::matchers(MatcherType::CurrentDocumentSymbols), expectedResults, searchText);
}

void LocatorFilterTest::testFunctionsFilterHighlighting()
{
    MyTestDataDir testDirectory("testdata_basic");
    const QString testFile = testDirectory.file("file1.cpp");
    const QString testFileShort = FilePath::fromString(testFile).shortNativePath();

    const QString searchText = "pos";
    const ResultDataList expectedResults{
        ResultData("positiveNumber()", testFileShort,
                   "~~~             "),
        ResultData("somePositionWithin()", testFileShort,
                   "    ~~~             "),
        ResultData("pointOfService()", testFileShort,
                   "~    ~ ~        "),
        ResultData("matchArgument(Pos)", testFileShort,
                   "              ~~~ ")
       };

    Tests::VerifyCleanCppModelManager verify;
    CppLocatorFilterTestCase(LocatorMatcher::matchers(MatcherType::Functions), testFile,
                             searchText, expectedResults);
}

} // namespace CppEditor::Internal
