// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compilationdatabasetests.h"

#include "compilationdatabaseutils.h"

#include <coreplugin/icore.h>
#include <cppeditor/cpptoolstestcase.h>
#include <cppeditor/projectinfo.h>

#include <projectexplorer/kitaspects.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>

#include <QtTest>

using namespace CppEditor;
using namespace ProjectExplorer;
using namespace Utils;

namespace CompilationDatabaseProjectManager::Internal {

CompilationDatabaseTests::CompilationDatabaseTests(QObject *parent)
    : QObject(parent)
{}

CompilationDatabaseTests::~CompilationDatabaseTests() = default;

void CompilationDatabaseTests::initTestCase()
{
    const QList<Kit *> allKits = KitManager::kits();
    if (allKits.empty())
        QSKIP("This test requires at least one kit to be present.");

    ToolChain *toolchain = ToolChainManager::toolChain([](const ToolChain *tc) {
        return tc->isValid() && tc->language() == ProjectExplorer::Constants::CXX_LANGUAGE_ID;
    });
    if (!toolchain)
        QSKIP("This test requires that there is at least one C++ toolchain present.");

    m_tmpDir = std::make_unique<CppEditor::Tests::TemporaryCopiedDir>(":/database_samples");
    QVERIFY(m_tmpDir->isValid());
}

void CompilationDatabaseTests::cleanupTestCase()
{
    m_tmpDir.reset();
}

void CompilationDatabaseTests::testProject()
{
    QFETCH(FilePath, projectFilePath);

    CppEditor::Tests::ProjectOpenerAndCloser projectManager;
    const CppEditor::ProjectInfo::ConstPtr projectInfo = projectManager.open(projectFilePath, true);
    QVERIFY(projectInfo);

    QVector<CppEditor::ProjectPart::ConstPtr> projectParts = projectInfo->projectParts();
    QVERIFY(!projectParts.isEmpty());

    const CppEditor::ProjectPart &projectPart = *projectParts.first();
    QVERIFY(!projectPart.headerPaths.isEmpty());
    QVERIFY(!projectPart.projectMacros.isEmpty());
    QVERIFY(!projectPart.toolChainMacros.isEmpty());
    QVERIFY(!projectPart.files.isEmpty());
}

void CompilationDatabaseTests::testProject_data()
{
    QTest::addColumn<FilePath>("projectFilePath");

    addTestRow("qtc/compile_commands.json");
    addTestRow("llvm/compile_commands.json");
}

namespace {
class CompilationDatabaseUtilsTestData
{
public:
    QStringList getSplitCommandLine(const QString &commandLine)
    {
        QSet<QString> fc;
        return splitCommandLine(commandLine, fc);
    }

    QStringList getFilteredFlags()
    {
        filteredFlags(FilePath::fromString(fileName),
                      FilePath::fromString(workingDir),
                      flags, headerPaths, macros, fileKind, sysRoot);
        return flags;
    }

    HeaderPaths headerPaths;
    Macros macros;
    CppEditor::ProjectFile::Kind fileKind = CppEditor::ProjectFile::Unclassified;
    QStringList flags;
    QString fileName;
    QString workingDir;
    FilePath sysRoot;
};
}

void CompilationDatabaseTests::testFilterEmptyFlags()
{
    QVERIFY(CompilationDatabaseUtilsTestData().getFilteredFlags().isEmpty());
}

void CompilationDatabaseTests::testFilterFromFilename()
{
    QCOMPARE(filterFromFileName(QStringList{"-o", "foo.o"}, "foo.c"), QStringList());
}

void CompilationDatabaseTests::testFilterArguments()
{
    using Utils::HostOsInfo;
    const char winPath1[] = "C:\\Qt\\5.9.2\\mingw53_32\\include";
    const char otherPath1[] = "/Qt/5.9.2/mingw53_32/include";
    const char winPath2[] = "C:\\Qt\\5.9.2\\mingw53_32\\include\\QtWidgets";
    const char otherPath2[] = "/Qt/5.9.2/mingw53_32/include/QtWidgets";
    CompilationDatabaseUtilsTestData testData;
    testData.fileName = "compileroptionsbuilder.cpp";
    testData.workingDir = "C:/build-qtcreator-MinGW_32bit-Debug";
    testData.flags = filterFromFileName(
        QStringList{"clang++",
                    "-c",
                    "-m32",
                    "-target",
                    "i686-w64-mingw32",
                    "-std=gnu++14",
                    "-fcxx-exceptions",
                    "-fexceptions",
                    "-DUNICODE",
                    "-DRELATIVE_PLUGIN_PATH=\"../lib/qtcreator/plugins\"",
                    "-DQT_CREATOR",
                    "-I",
                    QString::fromUtf8(HostOsInfo::isWindowsHost() ? winPath1 : otherPath1),
                    "-I",
                    QString::fromUtf8(HostOsInfo::isWindowsHost() ? winPath2 : otherPath2),
                    "-x",
                    "c++",
                    QString("--sysroot=") + (HostOsInfo::isWindowsHost()
                        ? "C:\\sysroot\\embedded" : "/opt/sysroot/embedded"),
                    QLatin1String(HostOsInfo::isWindowsHost()
                        ? "C:\\qt-creator\\src\\plugins\\cpptools\\compileroptionsbuilder.cpp"
                        : "/opt/qt-creator/src/plugins/cpptools/compileroptionsbuilder.cpp")},
        "compileroptionsbuilder.cpp");

    testData.getFilteredFlags();

    QCOMPARE(testData.flags, (QStringList{"-m32", "-target", "i686-w64-mingw32", "-std=gnu++14",
                                          "-fcxx-exceptions", "-fexceptions"}));
    QCOMPARE(testData.headerPaths,
             toUserHeaderPaths(QStringList{
                 QString::fromUtf8(HostOsInfo::isWindowsHost() ? winPath1 : otherPath1),
                 QString::fromUtf8(HostOsInfo::isWindowsHost() ? winPath2 : otherPath2)}));
    QCOMPARE(testData.macros, (Macros{{"UNICODE", "1"},
                                      {"RELATIVE_PLUGIN_PATH", "\"../lib/qtcreator/plugins\""},
                                      {"QT_CREATOR", "1"}}));
    QCOMPARE(testData.fileKind, CppEditor::ProjectFile::Kind::CXXSource);
    QCOMPARE(testData.sysRoot.toUserOutput(), HostOsInfo::isWindowsHost()
             ? QString("C:\\sysroot\\embedded") : QString("/opt/sysroot/embedded"));
}

static QString kCmakeCommand
    = "C:\\PROGRA~2\\MICROS~2\\2017\\COMMUN~1\\VC\\Tools\\MSVC\\1415~1.267\\bin\\HostX64\\x64\\cl."
      "exe "
      "/nologo "
      "/TP "
      "-DUNICODE "
      "-D_HAS_EXCEPTIONS=0 "
      "-Itools\\clang\\lib\\Sema "
      "/DWIN32 "
      "/D_WINDOWS "
      "/Zc:inline "
      "/Zc:strictStrings "
      "/Oi "
      "/Zc:rvalueCast "
      "/W4 "
      "-wd4141 "
      "-wd4146 "
      "/MDd "
      "/Zi "
      "/Ob0 "
      "/Od "
      "/RTC1 "
      "/EHs-c- "
      "/GR "
      "/Fotools\\clang\\lib\\Sema\\CMakeFiles\\clangSema.dir\\SemaCodeComplete.cpp.obj "
      "/FdTARGET_COMPILE_PDB "
      "/FS "
      "-c "
      "C:\\qt_llvm\\tools\\clang\\lib\\Sema\\SemaCodeComplete.cpp";

void CompilationDatabaseTests::testSplitFlags()
{
    CompilationDatabaseUtilsTestData testData;
    testData.flags = testData.getSplitCommandLine(kCmakeCommand);

    QCOMPARE(testData.flags.size(), 27);
}

void CompilationDatabaseTests::testSplitFlagsWithEscapedQuotes()
{
    CompilationDatabaseUtilsTestData testData;
    testData.flags = testData.getSplitCommandLine("-DRC_FILE_VERSION=\\\"7.0.0\\\" "
                             "-DRELATIVE_PLUGIN_PATH=\"../lib/qtcreator/plugins\"");

    QCOMPARE(testData.flags.size(), 2);
}

void CompilationDatabaseTests::testFilterCommand()
{
    CompilationDatabaseUtilsTestData testData;
    testData.fileName = "SemaCodeComplete.cpp";
    testData.workingDir = "C:/build-qt_llvm-msvc2017_64bit-Debug";
    testData.flags = filterFromFileName(testData.getSplitCommandLine(kCmakeCommand),
                                        "SemaCodeComplete.cpp");
    testData.getFilteredFlags();

    if (Utils::HostOsInfo::isWindowsHost()) {
        QCOMPARE(testData.flags,
                 (QStringList{"/Zc:inline", "/Zc:strictStrings", "/Zc:rvalueCast", "/Zi"}));
        QCOMPARE(testData.headerPaths,
                 toUserHeaderPaths(QStringList{"C:/build-qt_llvm-msvc2017_64bit-Debug/tools\\clang\\lib\\Sema"}));
        QCOMPARE(testData.macros, (Macros{{"UNICODE", "1"}, {"_HAS_EXCEPTIONS", "0"}, {"WIN32", "1"},
                                          {"_WINDOWS", "1"}}));
        QCOMPARE(testData.fileKind, CppEditor::ProjectFile::Kind::CXXSource);
    }
}

void CompilationDatabaseTests::testFileKindDifferentFromExtension()
{
    CompilationDatabaseUtilsTestData testData;
    testData.fileName = "foo.c";
    testData.flags = QStringList{"-xc++"};
    testData.getFilteredFlags();

    QCOMPARE(testData.fileKind, CppEditor::ProjectFile::Kind::CXXSource);
}

void CompilationDatabaseTests::testFileKindDifferentFromExtension2()
{
    CompilationDatabaseUtilsTestData testData;
    testData.fileName = "foo.cpp";
    testData.flags = QStringList{"-x", "c"};
    testData.getFilteredFlags();

    QCOMPARE(testData.fileKind, CppEditor::ProjectFile::Kind::CSource);
}

void CompilationDatabaseTests::testSkipOutputFiles()
{
    CompilationDatabaseUtilsTestData testData;
    testData.flags = filterFromFileName(QStringList{"-o", "foo.o"}, "foo.cpp");

    QVERIFY(testData.getFilteredFlags().isEmpty());
}

void CompilationDatabaseTests::addTestRow(const QString &relativeFilePath)
{
    const FilePath absoluteFilePath = m_tmpDir->absolutePath(relativeFilePath);

    QTest::newRow(qPrintable(relativeFilePath)) << absoluteFilePath;
}

} // CompilationDatabaseProjectManager::Internal
