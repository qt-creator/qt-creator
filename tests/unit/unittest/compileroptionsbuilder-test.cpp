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

#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/cppprojectfile.h>
#include <cpptools/projectpart.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>

using CppTools::CompilerOptionsBuilder;
using CppTools::ProjectFile;
using CppTools::ProjectPart;
using ProjectExplorer::HeaderPath;
using ProjectExplorer::HeaderPathType;
using ProjectExplorer::Project;

MATCHER_P(IsPartOfHeader, headerPart, std::string(negation ? "isn't " : "is ") + headerPart)
{
    return arg.contains(QString::fromStdString(headerPart));
}
namespace {

class CompilerOptionsBuilder : public ::testing::Test
{
protected:
    void SetUp() final
    {
        projectPart.project = project.get();
        projectPart.toolchainType = ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID;
        projectPart.languageVersion = ProjectExplorer::LanguageVersion::CXX17;
        projectPart.toolChainWordWidth = CppTools::ProjectPart::WordWidth64Bit;
        projectPart.toolChainTargetTriple = "x86_64-apple-darwin10";
        projectPart.extraCodeModelFlags = QStringList{"-arch", "x86_64"};

        projectPart.precompiledHeaders = QStringList{TESTDATA_DIR "/compileroptionsbuilder.pch"};
        projectPart.toolChainMacros = {ProjectExplorer::Macro{"foo", "bar"},
                                       ProjectExplorer::Macro{"__cplusplus", "2"},
                                       ProjectExplorer::Macro{"__STDC_VERSION__", "2"},
                                       ProjectExplorer::Macro{"_MSVC_LANG", "2"},
                                       ProjectExplorer::Macro{"_MSC_BUILD", "2"},
                                       ProjectExplorer::Macro{"_MSC_FULL_VER", "1900"},
                                       ProjectExplorer::Macro{"_MSC_VER", "19"}};
        projectPart.projectMacros = {ProjectExplorer::Macro{"projectFoo", "projectBar"}};
        projectPart.qtVersion = ProjectPart::Qt5;

        projectPart.headerPaths = {HeaderPath{"/tmp/builtin_path", HeaderPathType::BuiltIn},
                                   HeaderPath{"/tmp/system_path", HeaderPathType::System},
                                   HeaderPath{"/tmp/path", HeaderPathType::User}};
    }

    std::unique_ptr<Project> project{std::make_unique<ProjectExplorer::Project>()};
    ProjectPart projectPart;
    CppTools::CompilerOptionsBuilder compilerOptionsBuilder{projectPart};
};

TEST_F(CompilerOptionsBuilder, AddToolchainAndProjectMacros)
{
    compilerOptionsBuilder.addToolchainAndProjectMacros();

    ASSERT_THAT(compilerOptionsBuilder.options(), ElementsAre("-Dfoo=bar", "-DprojectFoo=projectBar"));
}

TEST_F(CompilerOptionsBuilder, AddToolchainAndProjectMacrosWithoutSkipingLanguageDefines)
{
    CppTools::CompilerOptionsBuilder compilerOptionsBuilder{projectPart,
                                                            CppTools::UseSystemHeader::No,
                                                            CppTools::SkipBuiltIn::No,
                                                            CppTools:: SkipLanguageDefines::No};

    compilerOptionsBuilder.addToolchainAndProjectMacros();

    ASSERT_THAT(compilerOptionsBuilder.options(),
                ElementsAre("-Dfoo=bar",
                            "-D__cplusplus=2",
                            "-D__STDC_VERSION__=2",
                            "-D_MSVC_LANG=2",
                            "-D_MSC_BUILD=2",
                            "-D_MSC_FULL_VER=1900",
                            "-D_MSC_VER=19",
                            "-DprojectFoo=projectBar"));
}

TEST_F(CompilerOptionsBuilder, AddWordWidth)
{
    compilerOptionsBuilder.addWordWidth();

    ASSERT_THAT(compilerOptionsBuilder.options(), ElementsAre("-m64"));
}

TEST_F(CompilerOptionsBuilder, AddToolchainFlags)
{
    compilerOptionsBuilder.addToolchainFlags();

    ASSERT_THAT(compilerOptionsBuilder.options(), ElementsAre("-undef"));
}

TEST_F(CompilerOptionsBuilder, HeaderPathOptionsOrder)
{
    compilerOptionsBuilder.addHeaderPathOptions();

    ASSERT_THAT(compilerOptionsBuilder.options(),
                ElementsAre("-nostdlibinc",
                            "-I", QDir::toNativeSeparators("/tmp/path"),
                            "-I", QDir::toNativeSeparators("/tmp/system_path"),
                            "-isystem", QDir::toNativeSeparators("/tmp/builtin_path")));
}

TEST_F(CompilerOptionsBuilder, UseSystemHeader)
{
    CppTools::CompilerOptionsBuilder compilerOptionsBuilder(projectPart, CppTools::UseSystemHeader::Yes);

    compilerOptionsBuilder.addHeaderPathOptions();

    ASSERT_THAT(compilerOptionsBuilder.options(),
                ElementsAre("-nostdlibinc",
                            "-I", QDir::toNativeSeparators("/tmp/path"),
                            "-isystem", QDir::toNativeSeparators("/tmp/system_path"),
                            "-isystem", QDir::toNativeSeparators("/tmp/builtin_path")));
}

TEST_F(CompilerOptionsBuilder, ClangHeadersPath)
{
    CppTools::CompilerOptionsBuilder compilerOptionsBuilder(projectPart,
                                                            CppTools::UseSystemHeader::No,
                                                            CppTools::SkipBuiltIn::No,
                                                            CppTools::SkipLanguageDefines::Yes,
                                                            "7.0.0",
                                                            "");

    compilerOptionsBuilder.addHeaderPathOptions();

    ASSERT_THAT(compilerOptionsBuilder.options(),
                ElementsAre("-nostdinc",
                            "-nostdlibinc",
                            "-I", QDir::toNativeSeparators("/tmp/path"),
                            "-I", QDir::toNativeSeparators("/tmp/system_path"),
                            "-isystem", QDir::toNativeSeparators(CLANG_RESOURCE_DIR ""),
                            "-isystem", QDir::toNativeSeparators("/tmp/builtin_path")));
}

TEST_F(CompilerOptionsBuilder, ClangHeadersAndCppIncludesPathsOrderMacOs)
{
    auto defaultPaths = projectPart.headerPaths;
    projectPart.headerPaths = {HeaderPath{"/usr/include/c++/4.2.1", HeaderPathType::BuiltIn},
                               HeaderPath{"/usr/include/c++/4.2.1/backward", HeaderPathType::BuiltIn},
                               HeaderPath{"/usr/local/include", HeaderPathType::BuiltIn},
                               HeaderPath{"/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/../lib/clang/6.0/include", HeaderPathType::BuiltIn},
                               HeaderPath{"/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include", HeaderPathType::BuiltIn},
                               HeaderPath{"/usr/include", HeaderPathType::BuiltIn}
                              };
    projectPart.headerPaths.append(defaultPaths);
    CppTools::CompilerOptionsBuilder compilerOptionsBuilder(projectPart,
                                                            CppTools::UseSystemHeader::No,
                                                            CppTools::SkipBuiltIn::No,
                                                            CppTools::SkipLanguageDefines::Yes,
                                                            "7.0.0",
                                                            "");

    compilerOptionsBuilder.addHeaderPathOptions();

    ASSERT_THAT(compilerOptionsBuilder.options(),
                ElementsAre("-nostdinc",
                            "-nostdlibinc",
                            "-I", QDir::toNativeSeparators("/tmp/path"),
                            "-I", QDir::toNativeSeparators("/tmp/system_path"),
                            "-isystem", QDir::toNativeSeparators("/usr/include/c++/4.2.1"),
                            "-isystem", QDir::toNativeSeparators("/usr/include/c++/4.2.1/backward"),
                            "-isystem", QDir::toNativeSeparators("/usr/local/include"),
                            "-isystem", QDir::toNativeSeparators(CLANG_RESOURCE_DIR ""),
                            "-isystem", QDir::toNativeSeparators("/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include"),
                            "-isystem", QDir::toNativeSeparators("/usr/include"),
                            "-isystem", QDir::toNativeSeparators("/tmp/builtin_path")));
}

TEST_F(CompilerOptionsBuilder, ClangHeadersAndCppIncludesPathsOrderLinux)
{
    projectPart.headerPaths = {HeaderPath{"/usr/lib/gcc/x86_64-linux-gnu/4.8/../../../../include/c++/4.8", HeaderPathType::BuiltIn},
                               HeaderPath{"/usr/lib/gcc/x86_64-linux-gnu/4.8/../../../../include/c++/4.8/backward", HeaderPathType::BuiltIn},
                               HeaderPath{"/usr/lib/gcc/x86_64-linux-gnu/4.8/../../../../include/x86_64-linux-gnu/c++/4.8", HeaderPathType::BuiltIn},
                               HeaderPath{"/usr/local/include", HeaderPathType::BuiltIn},
                               HeaderPath{"/usr/lib/gcc/x86_64-linux-gnu/4.8/include", HeaderPathType::BuiltIn},
                               HeaderPath{"/usr/include/x86_64-linux-gnu", HeaderPathType::BuiltIn},
                               HeaderPath{"/usr/include", HeaderPathType::BuiltIn}
                              };
    projectPart.toolChainTargetTriple = "x86_64-linux-gnu";
    CppTools::CompilerOptionsBuilder compilerOptionsBuilder(projectPart,
                                                            CppTools::UseSystemHeader::No,
                                                            CppTools::SkipBuiltIn::No,
                                                            CppTools::SkipLanguageDefines::Yes,
                                                            "7.0.0",
                                                            "");

    compilerOptionsBuilder.addHeaderPathOptions();

    ASSERT_THAT(compilerOptionsBuilder.options(),
                ElementsAre("-nostdinc",
                            "-nostdlibinc",
                            "-isystem", QDir::toNativeSeparators("/usr/lib/gcc/x86_64-linux-gnu/4.8/../../../../include/c++/4.8"),
                            "-isystem", QDir::toNativeSeparators("/usr/lib/gcc/x86_64-linux-gnu/4.8/../../../../include/c++/4.8/backward"),
                            "-isystem", QDir::toNativeSeparators("/usr/lib/gcc/x86_64-linux-gnu/4.8/../../../../include/x86_64-linux-gnu/c++/4.8"),
                            "-isystem", QDir::toNativeSeparators(CLANG_RESOURCE_DIR ""),
                            "-isystem", QDir::toNativeSeparators("/usr/local/include"),
                            "-isystem", QDir::toNativeSeparators("/usr/lib/gcc/x86_64-linux-gnu/4.8/include"),
                            "-isystem", QDir::toNativeSeparators("/usr/include/x86_64-linux-gnu"),
                            "-isystem", QDir::toNativeSeparators("/usr/include")));
}

TEST_F(CompilerOptionsBuilder, ClangHeadersAndCppIncludesPathsOrderNoVersion)
{
    projectPart.headerPaths = {
        HeaderPath{"C:/Qt/Tools/mingw530_32/i686-w64-mingw32/include", HeaderPathType::BuiltIn},
        HeaderPath{"C:/Qt/Tools/mingw530_32/i686-w64-mingw32/include/c++",
                   HeaderPathType::BuiltIn},
        HeaderPath{"C:/Qt/Tools/mingw530_32/i686-w64-mingw32/include/c++/i686-w64-mingw32",
                   HeaderPathType::BuiltIn},
        HeaderPath{"C:/Qt/Tools/mingw530_32/i686-w64-mingw32/include/c++/backward",
                   HeaderPathType::BuiltIn}};
    projectPart.toolChainTargetTriple = "x86_64-w64-windows-gnu";
    CppTools::CompilerOptionsBuilder compilerOptionsBuilder(projectPart,
                                                            CppTools::UseSystemHeader::No,
                                                            CppTools::SkipBuiltIn::No,
                                                            CppTools::SkipLanguageDefines::Yes,
                                                            "7.0.0",
                                                            "");

    compilerOptionsBuilder.addHeaderPathOptions();

    ASSERT_THAT(
        compilerOptionsBuilder.options(),
        ElementsAre(
            "-nostdinc",
            "-nostdlibinc",
            "-isystem",
            QDir::toNativeSeparators("C:/Qt/Tools/mingw530_32/i686-w64-mingw32/include/c++"),
            "-isystem",
            QDir::toNativeSeparators(
                "C:/Qt/Tools/mingw530_32/i686-w64-mingw32/include/c++/i686-w64-mingw32"),
            "-isystem",
            QDir::toNativeSeparators(
                "C:/Qt/Tools/mingw530_32/i686-w64-mingw32/include/c++/backward"),
            "-isystem",
            QDir::toNativeSeparators(CLANG_RESOURCE_DIR ""),
            "-isystem",
            QDir::toNativeSeparators("C:/Qt/Tools/mingw530_32/i686-w64-mingw32/include")));
}

TEST_F(CompilerOptionsBuilder, ClangHeadersAndCppIncludesPathsOrderAndroidClang)
{
    projectPart.headerPaths
        = {HeaderPath{"C:/Users/test/AppData/Local/Android/sdk/ndk-"
                      "bundle/sysroot/usr/include/i686-linux-android",
                      HeaderPathType::BuiltIn},
           HeaderPath{"C:/Users/test/AppData/Local/Android/sdk/ndk-bundle/sources/cxx-"
                      "stl/llvm-libc++/include",
                      HeaderPathType::BuiltIn},
           HeaderPath{"C:/Users/test/AppData/Local/Android/sdk/ndk-"
                      "bundle/sources/android/support/include",
                      HeaderPathType::BuiltIn},
           HeaderPath{"C:/Users/test/AppData/Local/Android/sdk/ndk-bundle/sources/cxx-"
                      "stl/llvm-libc++abi/include",
                      HeaderPathType::BuiltIn},
           HeaderPath{
               "C:/Users/test/AppData/Local/Android/sdk/ndk-bundle/sysroot/usr/include",
               HeaderPathType::BuiltIn}};
    projectPart.toolChainTargetTriple = "i686-linux-android";
    CppTools::CompilerOptionsBuilder compilerOptionsBuilder(projectPart,
                                                            CppTools::UseSystemHeader::No,
                                                            CppTools::SkipBuiltIn::No,
                                                            CppTools::SkipLanguageDefines::Yes,
                                                            "7.0.0",
                                                            "");

    compilerOptionsBuilder.addHeaderPathOptions();

    ASSERT_THAT(
        compilerOptionsBuilder.options(),
        ElementsAre("-nostdinc",
                    "-nostdlibinc",
                    "-isystem",
                    QDir::toNativeSeparators("C:/Users/test/AppData/Local/Android/sdk/ndk-"
                                             "bundle/sources/cxx-stl/llvm-libc++/include"),
                    "-isystem",
                    QDir::toNativeSeparators("C:/Users/test/AppData/Local/Android/sdk/ndk-"
                                             "bundle/sources/cxx-stl/llvm-libc++abi/include"),
                    "-isystem",
                    QDir::toNativeSeparators(CLANG_RESOURCE_DIR ""),
                    "-isystem",
                    QDir::toNativeSeparators("C:/Users/test/AppData/Local/Android/sdk/ndk-"
                                             "bundle/sysroot/usr/include/i686-linux-android"),
                    "-isystem",
                    QDir::toNativeSeparators("C:/Users/test/AppData/Local/Android/sdk/ndk-"
                                             "bundle/sources/android/support/include"),
                    "-isystem",
                    QDir::toNativeSeparators("C:/Users/test/AppData/Local/Android/sdk/ndk-"
                                             "bundle/sysroot/usr/include")));
}

TEST_F(CompilerOptionsBuilder, NoPrecompiledHeader)
{
    compilerOptionsBuilder.addPrecompiledHeaderOptions(CppTools::CompilerOptionsBuilder::PchUsage::None);

    ASSERT_THAT(compilerOptionsBuilder.options().empty(), true);
}

TEST_F(CompilerOptionsBuilder, UsePrecompiledHeader)
{
    compilerOptionsBuilder.addPrecompiledHeaderOptions(CppTools::CompilerOptionsBuilder::PchUsage::Use);

    ASSERT_THAT(compilerOptionsBuilder.options(),
                ElementsAre("-include", QDir::toNativeSeparators(TESTDATA_DIR "/compileroptionsbuilder.pch")));
}

TEST_F(CompilerOptionsBuilder, AddMacros)
{
    compilerOptionsBuilder.addMacros(ProjectExplorer::Macros{ProjectExplorer::Macro{"key", "value"}});

    ASSERT_THAT(compilerOptionsBuilder.options(), ElementsAre("-Dkey=value"));
}

TEST_F(CompilerOptionsBuilder, AddTargetTriple)
{
    compilerOptionsBuilder.addTargetTriple();

    ASSERT_THAT(compilerOptionsBuilder.options(), ElementsAre("-target", "x86_64-apple-darwin10"));
}

TEST_F(CompilerOptionsBuilder, EnableCExceptions)
{
    projectPart.languageVersion = ProjectExplorer::LanguageVersion::C99;

    compilerOptionsBuilder.enableExceptions();

    ASSERT_THAT(compilerOptionsBuilder.options(), ElementsAre("-fexceptions"));
}

TEST_F(CompilerOptionsBuilder, EnableCXXExceptions)
{
    compilerOptionsBuilder.enableExceptions();

    ASSERT_THAT(compilerOptionsBuilder.options(), ElementsAre("-fcxx-exceptions", "-fexceptions"));
}

TEST_F(CompilerOptionsBuilder, InsertWrappedQtHeaders)
{
    compilerOptionsBuilder.insertWrappedQtHeaders();

    ASSERT_THAT(compilerOptionsBuilder.options(), Contains(IsPartOfHeader("wrappedQtHeaders")));
}

TEST_F(CompilerOptionsBuilder, SetLanguageVersion)
{
    compilerOptionsBuilder.updateLanguageOption(ProjectFile::CXXSource);

    ASSERT_THAT(compilerOptionsBuilder.options(), ElementsAre("-x", "c++"));
}

TEST_F(CompilerOptionsBuilder, HandleLanguageExtension)
{
    projectPart.languageExtensions = ProjectExplorer::LanguageExtension::ObjectiveC;

    compilerOptionsBuilder.updateLanguageOption(ProjectFile::CXXSource);

    ASSERT_THAT(compilerOptionsBuilder.options(), ElementsAre("-x", "objective-c++"));
}

TEST_F(CompilerOptionsBuilder, UpdateLanguageVersion)
{
    compilerOptionsBuilder.updateLanguageOption(ProjectFile::CXXSource);

    compilerOptionsBuilder.updateLanguageOption(ProjectFile::CXXHeader);

    ASSERT_THAT(compilerOptionsBuilder.options(), ElementsAre("-x", "c++-header"));
}

TEST_F(CompilerOptionsBuilder, AddMsvcCompatibilityVersion)
{
    projectPart.toolchainType = ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID;
    projectPart.toolChainMacros.append(ProjectExplorer::Macro{"_MSC_FULL_VER", "190000000"});

    compilerOptionsBuilder.addMsvcCompatibilityVersion();

    ASSERT_THAT(compilerOptionsBuilder.options(), ElementsAre("-fms-compatibility-version=19.00"));
}

TEST_F(CompilerOptionsBuilder, UndefineCppLanguageFeatureMacrosForMsvc2015)
{
    projectPart.toolchainType = ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID;
    projectPart.isMsvc2015Toolchain = true;

    compilerOptionsBuilder.undefineCppLanguageFeatureMacrosForMsvc2015();

    ASSERT_THAT(compilerOptionsBuilder.options(), Contains(QString{"-U__cpp_aggregate_bases"}));
}

TEST_F(CompilerOptionsBuilder, AddDefineFunctionMacrosMsvc)
{
    projectPart.toolchainType = ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID;

    compilerOptionsBuilder.addDefineFunctionMacrosMsvc();

    ASSERT_THAT(compilerOptionsBuilder.options(), Contains(QString{"-D__FUNCTION__=\"\""}));
}

TEST_F(CompilerOptionsBuilder, AddProjectConfigFileInclude)
{
    projectPart.projectConfigFile = "dummy_file.h";

    compilerOptionsBuilder.addProjectConfigFileInclude();

    ASSERT_THAT(compilerOptionsBuilder.options(), ElementsAre("-include", "dummy_file.h"));
}

TEST_F(CompilerOptionsBuilder, UndefineClangVersionMacrosForMsvc)
{
    projectPart.toolchainType = ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID;

    compilerOptionsBuilder.undefineClangVersionMacrosForMsvc();

    ASSERT_THAT(compilerOptionsBuilder.options(), Contains(QString{"-U__clang__"}));
}

TEST_F(CompilerOptionsBuilder, BuildAllOptions)
{
    compilerOptionsBuilder.build(ProjectFile::CXXSource, CppTools::CompilerOptionsBuilder::PchUsage::None);

    ASSERT_THAT(compilerOptionsBuilder.options(),
                ElementsAre(
                    "-nostdlibinc", "-c", "-m64", "-target", "x86_64-apple-darwin10",
                    "-arch", "x86_64", "-x", "c++", "-std=c++17", "-fcxx-exceptions",
                    "-fexceptions", "-Dfoo=bar", "-DprojectFoo=projectBar",
                    "-DBOOST_TYPE_INDEX_CTTI_USER_DEFINED_PARSING=(39, 1, true, \"T = \")",
                    "-undef",
                    "-I", IsPartOfHeader("wrappedQtHeaders"),
                    "-I", IsPartOfHeader(QDir::toNativeSeparators("wrappedQtHeaders/QtCore").toStdString()),
                    "-I", QDir::toNativeSeparators("/tmp/path"),
                    "-I", QDir::toNativeSeparators("/tmp/system_path"),
                    "-isystem", QDir::toNativeSeparators("/tmp/builtin_path")
                    ));
}

}
