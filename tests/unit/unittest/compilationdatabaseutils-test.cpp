/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <compilationdatabaseprojectmanager/compilationdatabaseutils.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/projectmacro.h>
#include <utils/fileutils.h>

using namespace ProjectExplorer;
using namespace CompilationDatabaseProjectManager;

namespace {

class CompilationDatabaseUtils : public ::testing::Test
{
protected:
    HeaderPaths headerPaths;
    Macros macros;
    CppTools::ProjectFile::Kind fileKind = CppTools::ProjectFile::Unclassified;
    QStringList flags;
    QString fileName;
    QString workingDir;
};

TEST_F(CompilationDatabaseUtils, FilterEmptyFlags)
{
    filteredFlags(fileName, workingDir, flags, headerPaths, macros, fileKind);

    ASSERT_THAT(flags.isEmpty(), true);
}

TEST_F(CompilationDatabaseUtils, FilterFromFilename)
{
    flags = filterFromFileName(QStringList{"-o", "foo.o"}, "foo");

    ASSERT_THAT(flags, QStringList{"-o"});
}

TEST_F(CompilationDatabaseUtils, FilterArguments)
{
    fileName = "compileroptionsbuilder.cpp";
    workingDir = "C:/build-qtcreator-MinGW_32bit-Debug";
    flags = filterFromFileName(QStringList {
        "clang++",
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
        "-fPIC",
        "-I",
        "C:\\Qt\\5.9.2\\mingw53_32\\include",
        "-I",
        "C:\\Qt\\5.9.2\\mingw53_32\\include\\QtWidgets",
        "-x",
        "c++",
        "C:\\qt-creator\\src\\plugins\\cpptools\\compileroptionsbuilder.cpp"
    }, "compileroptionsbuilder");

    filteredFlags(fileName, workingDir, flags, headerPaths, macros, fileKind);

    ASSERT_THAT(flags, Eq(QStringList{"-m32",
                                      "-target",
                                      "i686-w64-mingw32",
                                      "-std=gnu++14",
                                      "-fcxx-exceptions",
                                      "-fexceptions"}));
    ASSERT_THAT(headerPaths, Eq(HeaderPaths{
                                    {"C:\\Qt\\5.9.2\\mingw53_32\\include", HeaderPathType::User},
                                    {"C:\\Qt\\5.9.2\\mingw53_32\\include\\QtWidgets", HeaderPathType::User}
                                }));
    ASSERT_THAT(macros, Eq(Macros{
                               {"UNICODE", "1"},
                               {"RELATIVE_PLUGIN_PATH", "\"../lib/qtcreator/plugins\""},
                               {"QT_CREATOR", "1"}
                           }));
    ASSERT_THAT(fileKind, CppTools::ProjectFile::Kind::CXXSource);
}

static QString kCmakeCommand = "C:\\PROGRA~2\\MICROS~2\\2017\\COMMUN~1\\VC\\Tools\\MSVC\\1415~1.267\\bin\\HostX64\\x64\\cl.exe "
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

TEST_F(CompilationDatabaseUtils, SplitFlags)
{
    flags = splitCommandLine(kCmakeCommand);

    ASSERT_THAT(flags.size(), 27);
}

TEST_F(CompilationDatabaseUtils, SplitFlagsWithEscapedQuotes)
{
    flags = splitCommandLine("-DRC_FILE_VERSION=\\\"7.0.0\\\" "
                             "-DRELATIVE_PLUGIN_PATH=\"../lib/qtcreator/plugins\"");

    ASSERT_THAT(flags.size(), 2);
}

TEST_F(CompilationDatabaseUtils, FilterCommand)
{
    fileName = "SemaCodeComplete.cpp";
    workingDir = "C:/build-qt_llvm-msvc2017_64bit-Debug";
    flags = filterFromFileName(splitCommandLine(kCmakeCommand), "SemaCodeComplete");

    filteredFlags(fileName, workingDir, flags, headerPaths, macros, fileKind);

    ASSERT_THAT(flags, Eq(QStringList{"/Zc:inline",
                                      "/Zc:strictStrings",
                                      "/Zc:rvalueCast",
                                      "/Zi"}));
    ASSERT_THAT(headerPaths, Eq(HeaderPaths{
                                    {"tools\\clang\\lib\\Sema", HeaderPathType::User}
                                }));
    ASSERT_THAT(macros, Eq(Macros{
                               {"UNICODE", "1"},
                               {"_HAS_EXCEPTIONS", "0"},
                               {"WIN32", "1"},
                               {"_WINDOWS", "1"}
                           }));
    ASSERT_THAT(fileKind, CppTools::ProjectFile::Kind::CXXSource);
}

TEST_F(CompilationDatabaseUtils, FileKindDifferentFromExtension)
{
    fileName = "foo.c";
    flags = QStringList{"-xc++"};

    filteredFlags(fileName, workingDir, flags, headerPaths, macros, fileKind);

    ASSERT_THAT(fileKind, CppTools::ProjectFile::Kind::CXXSource);
}

TEST_F(CompilationDatabaseUtils, FileKindDifferentFromExtension2)
{
    fileName = "foo.cpp";
    flags  = QStringList{"-x", "c"};

    filteredFlags(fileName, workingDir, flags, headerPaths, macros, fileKind);

    ASSERT_THAT(fileKind, CppTools::ProjectFile::Kind::CSource);
}

TEST_F(CompilationDatabaseUtils, SkipOutputFiles)
{
    flags = filterFromFileName(QStringList{"-o", "foo.o"}, "foo");

    filteredFlags(fileName, workingDir, flags, headerPaths, macros, fileKind);

    ASSERT_THAT(flags.isEmpty(), true);
}

}
