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

#include <cpptools/headerpathfilter.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>

namespace {

using ProjectExplorer::HeaderPath;
using ProjectExplorer::HeaderPathType;

MATCHER_P(HasBuiltIn,
          path,
          std::string(negation ? "isn't " : "is ")
              + PrintToString(HeaderPath{QString::fromUtf8(path), HeaderPathType::BuiltIn}))
{
    return arg.path == QString::fromUtf8(path) && arg.type == HeaderPathType::BuiltIn;
}

MATCHER_P(HasSystem,
          path,
          std::string(negation ? "isn't " : "is ")
              + PrintToString(HeaderPath{QString::fromUtf8(path), HeaderPathType::System}))
{
    return arg.path == QString::fromUtf8(path) && arg.type == HeaderPathType::System;
}

MATCHER_P(HasFramework,
          path,
          std::string(negation ? "isn't " : "is ")
              + PrintToString(HeaderPath{QString::fromUtf8(path), HeaderPathType::Framework}))
{
    return arg.path == QString::fromUtf8(path) && arg.type == HeaderPathType::Framework;
}

MATCHER_P(HasUser,
          path,
          std::string(negation ? "isn't " : "is ")
              + PrintToString(HeaderPath{QString::fromUtf8(path), HeaderPathType::User}))
{
    return arg.path == QString::fromUtf8(path) && arg.type == HeaderPathType::User;
}

class HeaderPathFilter : public testing::Test
{
protected:
    HeaderPathFilter()
    {
        auto headerPaths = {HeaderPath{"", HeaderPathType::BuiltIn},
                            HeaderPath{"/builtin_path", HeaderPathType::BuiltIn},
                            HeaderPath{"/system_path", HeaderPathType::System},
                            HeaderPath{"/framework_path", HeaderPathType::Framework},
                            HeaderPath{"/outside_project_user_path", HeaderPathType::User},
                            HeaderPath{"/build/user_path", HeaderPathType::User},
                            HeaderPath{"/buildb/user_path", HeaderPathType::User},
                            HeaderPath{"/projectb/user_path", HeaderPathType::User},
                            HeaderPath{"/project/user_path", HeaderPathType::User}};

        projectPart.headerPaths = headerPaths;
        projectPart.project = &project;
    }

    static HeaderPath builtIn(const QString &path)
    {
        return HeaderPath{path, HeaderPathType::BuiltIn};
    }

protected:
    ProjectExplorer::Project project;
    CppTools::ProjectPart projectPart;
    CppTools::HeaderPathFilter filter{
        projectPart, CppTools::UseTweakedHeaderPaths::No, {}, {}, "/project", "/build"};
};

TEST_F(HeaderPathFilter, BuiltIn)
{
    filter.process();

    ASSERT_THAT(filter.builtInHeaderPaths, ElementsAre(HasBuiltIn("/builtin_path")));
}

TEST_F(HeaderPathFilter, System)
{
    filter.process();

    ASSERT_THAT(filter.systemHeaderPaths,
                ElementsAre(HasSystem("/project/.pre_includes"),
                            HasSystem("/system_path"),
                            HasFramework("/framework_path"),
                            HasUser("/outside_project_user_path"),
                            HasUser("/buildb/user_path"),
                            HasUser("/projectb/user_path")));
}

TEST_F(HeaderPathFilter, User)
{
    filter.process();

    ASSERT_THAT(filter.userHeaderPaths,
                ElementsAre(HasUser("/build/user_path"), HasUser("/project/user_path")));
}

TEST_F(HeaderPathFilter, NoProjectPathSet)
{
    CppTools::HeaderPathFilter filter{projectPart, CppTools::UseTweakedHeaderPaths::No};

    filter.process();

    ASSERT_THAT(filter.userHeaderPaths,
                ElementsAre(HasUser("/outside_project_user_path"),
                            HasUser("/build/user_path"),
                            HasUser("/buildb/user_path"),
                            HasUser("/projectb/user_path"),
                            HasUser("/project/user_path")));
}

TEST_F(HeaderPathFilter, DontAddInvalidPath)
{
    filter.process();

    ASSERT_THAT(filter,
                AllOf(Field(&CppTools::HeaderPathFilter::builtInHeaderPaths,
                            ElementsAre(HasBuiltIn("/builtin_path"))),
                      Field(&CppTools::HeaderPathFilter::systemHeaderPaths,
                            ElementsAre(HasSystem("/project/.pre_includes"),
                                        HasSystem("/system_path"),
                                        HasFramework("/framework_path"),
                                        HasUser("/outside_project_user_path"),
                                        HasUser("/buildb/user_path"),
                                        HasUser("/projectb/user_path"))),
                      Field(&CppTools::HeaderPathFilter::userHeaderPaths,
                            ElementsAre(HasUser("/build/user_path"), HasUser("/project/user_path")))));
}

TEST_F(HeaderPathFilter, ClangHeadersPath)
{
    CppTools::HeaderPathFilter filter{projectPart,
                                      CppTools::UseTweakedHeaderPaths::Yes,
                                      "6.0",
                                      CLANG_RESOURCE_DIR};

    filter.process();

    ASSERT_THAT(filter.builtInHeaderPaths,
                ElementsAre(HasBuiltIn(CLANG_RESOURCE_DIR), HasBuiltIn("/builtin_path")));
}

TEST_F(HeaderPathFilter, ClangHeadersPathWitoutClangVersion)
{
    CppTools::HeaderPathFilter filter{projectPart,
                                      CppTools::UseTweakedHeaderPaths::Yes};

    filter.process();

    ASSERT_THAT(filter.builtInHeaderPaths,
                ElementsAre(HasBuiltIn("/builtin_path")));
}

TEST_F(HeaderPathFilter, ClangHeadersAndCppIncludesPathsOrderMacOs)
{
    auto builtIns = {
        builtIn("/usr/include/c++/4.2.1"),
        builtIn("/usr/include/c++/4.2.1/backward"),
        builtIn("/usr/local/include"),
        builtIn("/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/6.0/include"),
        builtIn("/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include"),
        builtIn("/usr/include")
    };
    projectPart.toolChainTargetTriple = "x86_64-apple-darwin10";
    std::copy(builtIns.begin(),
              builtIns.end(),
              std::inserter(projectPart.headerPaths, projectPart.headerPaths.begin()));
    CppTools::HeaderPathFilter filter{projectPart,
                                      CppTools::UseTweakedHeaderPaths::Yes,
                                      "6.0",
                                      CLANG_RESOURCE_DIR};

    filter.process();

    ASSERT_THAT(filter.builtInHeaderPaths,
                ElementsAre(HasBuiltIn("/usr/include/c++/4.2.1"),
                            HasBuiltIn("/usr/include/c++/4.2.1/backward"),
                            HasBuiltIn("/usr/local/include"),
                            HasBuiltIn(CLANG_RESOURCE_DIR),
                            HasBuiltIn("/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include"),
                            HasBuiltIn("/usr/include"),
                            HasBuiltIn("/builtin_path")));
}

TEST_F(HeaderPathFilter, ClangHeadersAndCppIncludesPathsOrderLinux)
{
    auto builtIns = {
        builtIn("/usr/include/c++/4.8"),
        builtIn("/usr/include/c++/4.8/backward"),
        builtIn("/usr/include/x86_64-linux-gnu/c++/4.8"),
        builtIn("/usr/local/include"),
        builtIn("/usr/lib/gcc/x86_64-linux-gnu/4.8/include"),
        builtIn("/usr/include/x86_64-linux-gnu"),
        builtIn("/usr/include")};
    std::copy(builtIns.begin(),
              builtIns.end(),
              std::inserter(projectPart.headerPaths, projectPart.headerPaths.begin()));
    projectPart.toolChainTargetTriple = "x86_64-linux-gnu";
    CppTools::HeaderPathFilter filter{projectPart,
                                      CppTools::UseTweakedHeaderPaths::Yes,
                                      "6.0",
                                      CLANG_RESOURCE_DIR};

    filter.process();

    ASSERT_THAT(filter.builtInHeaderPaths,
                ElementsAre(HasBuiltIn("/usr/include/c++/4.8"),
                            HasBuiltIn("/usr/include/c++/4.8/backward"),
                            HasBuiltIn("/usr/include/x86_64-linux-gnu/c++/4.8"),
                            HasBuiltIn("/usr/local/include"),
                            HasBuiltIn(CLANG_RESOURCE_DIR),
                            HasBuiltIn("/usr/lib/gcc/x86_64-linux-gnu/4.8/include"),
                            HasBuiltIn("/usr/include/x86_64-linux-gnu"),
                            HasBuiltIn("/usr/include"),
                            HasBuiltIn("/builtin_path")));
}

// GCC-internal include paths like <installdir>/include and <installdir/include-next> might confuse
// clang and should be filtered out. clang on the command line filters them out, too.
TEST_F(HeaderPathFilter, RemoveGccInternalPaths)
{
    projectPart.toolChainInstallDir = Utils::FilePath::fromUtf8("/usr/lib/gcc/x86_64-linux-gnu/7");
    projectPart.toolchainType = ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID;
    projectPart.headerPaths = {
        builtIn("/usr/lib/gcc/x86_64-linux-gnu/7/include"),
        builtIn("/usr/lib/gcc/x86_64-linux-gnu/7/include-fixed"),
    };
    CppTools::HeaderPathFilter filter{projectPart,
                                      CppTools::UseTweakedHeaderPaths::Yes,
                                      "6.0",
                                      CLANG_RESOURCE_DIR};

    filter.process();

    ASSERT_THAT(filter.builtInHeaderPaths, ElementsAre(HasBuiltIn(CLANG_RESOURCE_DIR)));
}

// Some distributions ship the standard library headers in "<installdir>/include/c++" (MinGW)
// or e.g. "<installdir>/include/g++-v8" (Gentoo).
// Ensure that we do not remove include paths pointing there.
TEST_F(HeaderPathFilter, RemoveGccInternalPathsExceptForStandardPaths)
{
    projectPart.toolChainInstallDir = Utils::FilePath::fromUtf8(
        "c:/mingw/lib/gcc/x86_64-w64-mingw32/7.3.0");
    projectPart.toolchainType = ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID;
    projectPart.headerPaths = {
        builtIn("c:/mingw/lib/gcc/x86_64-w64-mingw32/7.3.0/include/c++"),
        builtIn("c:/mingw/lib/gcc/x86_64-w64-mingw32/7.3.0/include/c++/x86_64-w64-mingw32"),
        builtIn("c:/mingw/lib/gcc/x86_64-w64-mingw32/7.3.0/include/c++/backward"),
    };

    auto expected = projectPart.headerPaths;
    expected << builtIn(CLANG_RESOURCE_DIR);

    CppTools::HeaderPathFilter filter{projectPart,
                                      CppTools::UseTweakedHeaderPaths::Yes,
                                      "6.0",
                                      CLANG_RESOURCE_DIR};

    filter.process();

    ASSERT_THAT(filter.builtInHeaderPaths, ContainerEq(expected));
}

TEST_F(HeaderPathFilter, ClangHeadersAndCppIncludesPathsOrderNoVersion)
{
    projectPart.headerPaths = {
        builtIn("C:/mingw/i686-w64-mingw32/include"),
        builtIn("C:/mingw/i686-w64-mingw32/include/c++"),
        builtIn("C:/mingw/i686-w64-mingw32/include/c++/i686-w64-mingw32"),
        builtIn("C:/mingw/i686-w64-mingw32/include/c++/backward"),
    };
    projectPart.toolChainTargetTriple = "x86_64-w64-windows-gnu";
    CppTools::HeaderPathFilter filter{projectPart,
                                      CppTools::UseTweakedHeaderPaths::Yes,
                                      "6.0",
                                      CLANG_RESOURCE_DIR};

    filter.process();

    ASSERT_THAT(
        filter.builtInHeaderPaths,
        ElementsAre(HasBuiltIn("C:/mingw/i686-w64-mingw32/include/c++"),
                    HasBuiltIn("C:/mingw/i686-w64-mingw32/include/c++/i686-w64-mingw32"),
                    HasBuiltIn("C:/mingw/i686-w64-mingw32/include/c++/backward"),
                    HasBuiltIn(CLANG_RESOURCE_DIR),
                    HasBuiltIn("C:/mingw/i686-w64-mingw32/include")));
}

TEST_F(HeaderPathFilter, ClangHeadersAndCppIncludesPathsOrderAndroidClang)
{
    projectPart.headerPaths = {
        builtIn("C:/Android/sdk/ndk-bundle/sysroot/usr/include/i686-linux-android"),
        builtIn("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++/include"),
        builtIn("C:/Android/sdk/ndk-bundle/sources/android/support/include"),
        builtIn("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++abi/include"),
        builtIn("C:/Android/sdk/ndk-bundle/sysroot/usr/include")
    };
    projectPart.toolChainTargetTriple = "i686-linux-android";
    CppTools::HeaderPathFilter filter{projectPart,
                                      CppTools::UseTweakedHeaderPaths::Yes,
                                      "6.0",
                                      CLANG_RESOURCE_DIR};

    filter.process();

    ASSERT_THAT(
        filter.builtInHeaderPaths,
        ElementsAre(HasBuiltIn("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++/include"),
                    HasBuiltIn("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++abi/include"),
                    HasBuiltIn(CLANG_RESOURCE_DIR),
                    HasBuiltIn("C:/Android/sdk/ndk-bundle/sysroot/usr/include/i686-linux-android"),
                    HasBuiltIn("C:/Android/sdk/ndk-bundle/sources/android/support/include"),
                    HasBuiltIn("C:/Android/sdk/ndk-bundle/sysroot/usr/include")));
}

} // namespace
