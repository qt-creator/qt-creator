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

#include <commandlinebuilder.h>
#include <pchtask.h>
#include <projectpartcontainer.h>
#include <projectpartartefact.h>

namespace {
template<typename ProjectInfo>
using Builder = ClangBackEnd::CommandLineBuilder<ProjectInfo>;

using ClangBackEnd::IncludeSearchPathType;

template <typename ProjectInfo>
class CommandLineBuilder : public testing::Test
{
};

template <>
class CommandLineBuilder<ClangBackEnd::PchTask> : public testing::Test
{
public:
    CommandLineBuilder()
    {
        cppProjectInfo.language = Utils::Language::Cxx;
    }

public:
    ClangBackEnd::PchTask emptyProjectInfo{"empty", {}, {}, {}, {}, {}, {}, {}};
    ClangBackEnd::PchTask cppProjectInfo{"project1", {}, {}, {}, {}, {}, {}, {}};
};

template <>
class CommandLineBuilder<ClangBackEnd::ProjectPartContainer> : public testing::Test
{
public:
    CommandLineBuilder()
    {
        cppProjectInfo.language = Utils::Language::Cxx;
    }

public:
    ClangBackEnd::ProjectPartContainer emptyProjectInfo{"empty",
                                                        {},
                                                        {},
                                                        {},
                                                        {},
                                                        {},
                                                        {},
                                                        Utils::Language::Cxx,
                                                        Utils::LanguageVersion::CXX98,
                                                        Utils::LanguageExtension::None};
    ClangBackEnd::ProjectPartContainer cppProjectInfo{"project1",
                                                      {},
                                                      {},
                                                      {},
                                                      {},
                                                      {},
                                                      {},
                                                      Utils::Language::Cxx,
                                                      Utils::LanguageVersion::CXX98,
                                                      Utils::LanguageExtension::None};
};

template <>
class CommandLineBuilder<ClangBackEnd::ProjectPartArtefact> : public testing::Test
{
public:
    CommandLineBuilder()
    {
        cppProjectInfo.language = Utils::Language::Cxx;
    }

public:
    ClangBackEnd::ProjectPartArtefact emptyProjectInfo{{},
                                                       {},
                                                       {},
                                                       {},
                                                       {},
                                                       Utils::Language::Cxx,
                                                       Utils::LanguageVersion::CXX98,
                                                       Utils::LanguageExtension::None};
    ClangBackEnd::ProjectPartArtefact cppProjectInfo{{},
                                                     {},
                                                     {},
                                                     {},
                                                     {},
                                                     Utils::Language::Cxx,
                                                     Utils::LanguageVersion::CXX98,
                                                     Utils::LanguageExtension::None};
};

using ProjectInfos = testing::Types<ClangBackEnd::PchTask,
                                    ClangBackEnd::ProjectPartContainer,
                                    ClangBackEnd::ProjectPartArtefact>;
TYPED_TEST_SUITE(CommandLineBuilder, ProjectInfos);

TYPED_TEST(CommandLineBuilder, AddToolChainArguments)
{
    Builder<TypeParam> builder{this->emptyProjectInfo, {"-m64", "-PIC"}, {}};

    ASSERT_THAT(builder.commandLine, AllOf(Contains("-m64"), Contains("-PIC")));
}

TYPED_TEST(CommandLineBuilder, CTask)
{
    this->emptyProjectInfo.language = Utils::Language::C;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::C11;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.c"};

    ASSERT_THAT(
        builder.commandLine,
        ElementsAre("clang", "-x", "c-header", "-std=c11", "-nostdinc", "-nostdlibinc", "/source/file.c"));
}

TYPED_TEST(CommandLineBuilder, ObjectiveCTask)
{
    this->emptyProjectInfo.language = Utils::Language::C;
    this->emptyProjectInfo.languageExtension = Utils::LanguageExtension::ObjectiveC;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::C11;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.c"};

    ASSERT_THAT(builder.commandLine,
                ElementsAre("clang",
                            "-x",
                            "objective-c-header",
                            "-std=c11",
                            "-nostdinc",
                            "-nostdlibinc",
                            "/source/file.c"));
}

TYPED_TEST(CommandLineBuilder, CppTask)
{
    this->emptyProjectInfo.language = Utils::Language::Cxx;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::CXX98;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.cpp"};

    ASSERT_THAT(builder.commandLine,
                ElementsAre("clang++",
                            "-x",
                            "c++-header",
                            "-std=c++98",
                            "-nostdinc",
                            "-nostdlibinc",
                            "/source/file.cpp"));
}

TYPED_TEST(CommandLineBuilder, ObjectiveCppTask)
{
    this->emptyProjectInfo.language = Utils::Language::Cxx;
    this->emptyProjectInfo.languageExtension = Utils::LanguageExtension::ObjectiveC;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::CXX98;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.cpp"};

    ASSERT_THAT(builder.commandLine,
                ElementsAre("clang++",
                            "-x",
                            "objective-c++-header",
                            "-std=c++98",
                            "-nostdinc",
                            "-nostdlibinc",
                            "/source/file.cpp"));
}

TYPED_TEST(CommandLineBuilder, Cpp98)
{
    this->emptyProjectInfo.language = Utils::Language::Cxx;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::CXX98;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.cpp"};

    ASSERT_THAT(builder.commandLine, Contains("-std=c++98"));
}

TYPED_TEST(CommandLineBuilder, Cpp03)
{
    this->emptyProjectInfo.language = Utils::Language::Cxx;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::CXX03;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.cpp"};

    ASSERT_THAT(builder.commandLine, Contains("-std=c++03"));
}

TYPED_TEST(CommandLineBuilder, Cpp11)
{
    this->emptyProjectInfo.language = Utils::Language::Cxx;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::CXX11;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.cpp"};

    ASSERT_THAT(builder.commandLine, Contains("-std=c++11"));
}

TYPED_TEST(CommandLineBuilder, Cpp14)
{
    this->emptyProjectInfo.language = Utils::Language::Cxx;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::CXX14;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.cpp"};

    ASSERT_THAT(builder.commandLine, Contains("-std=c++14"));
}

TYPED_TEST(CommandLineBuilder, Cpp17)
{
    this->emptyProjectInfo.language = Utils::Language::Cxx;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::CXX17;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.cpp"};

    ASSERT_THAT(builder.commandLine, Contains("-std=c++17"));
}

TYPED_TEST(CommandLineBuilder, Cpp20)
{
    this->emptyProjectInfo.language = Utils::Language::Cxx;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::CXX2a;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.cpp"};

    ASSERT_THAT(builder.commandLine, Contains("-std=c++2a"));
}

TYPED_TEST(CommandLineBuilder, GnuCpp98)
{
    this->emptyProjectInfo.language = Utils::Language::Cxx;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::CXX98;
    this->emptyProjectInfo.languageExtension = Utils::LanguageExtension::Gnu;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.cpp"};

    ASSERT_THAT(builder.commandLine, Contains("-std=gnu++98"));
}

TYPED_TEST(CommandLineBuilder, GnuCpp03)
{
    this->emptyProjectInfo.language = Utils::Language::Cxx;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::CXX03;
    this->emptyProjectInfo.languageExtension = Utils::LanguageExtension::Gnu;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.cpp"};

    ASSERT_THAT(builder.commandLine, Contains("-std=gnu++03"));
}

TYPED_TEST(CommandLineBuilder, GnuCpp11)
{
    this->emptyProjectInfo.language = Utils::Language::Cxx;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::CXX11;
    this->emptyProjectInfo.languageExtension = Utils::LanguageExtension::Gnu;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.cpp"};

    ASSERT_THAT(builder.commandLine, Contains("-std=gnu++11"));
}

TYPED_TEST(CommandLineBuilder, GnuCpp14)
{
    this->emptyProjectInfo.language = Utils::Language::Cxx;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::CXX14;
    this->emptyProjectInfo.languageExtension = Utils::LanguageExtension::Gnu;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.cpp"};

    ASSERT_THAT(builder.commandLine, Contains("-std=gnu++14"));
}

TYPED_TEST(CommandLineBuilder, GnuCpp17)
{
    this->emptyProjectInfo.language = Utils::Language::Cxx;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::CXX17;
    this->emptyProjectInfo.languageExtension = Utils::LanguageExtension::Gnu;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.cpp"};

    ASSERT_THAT(builder.commandLine, Contains("-std=gnu++17"));
}

TYPED_TEST(CommandLineBuilder, GnuCpp20)
{
    this->emptyProjectInfo.language = Utils::Language::Cxx;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::CXX2a;
    this->emptyProjectInfo.languageExtension = Utils::LanguageExtension::Gnu;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.cpp"};

    ASSERT_THAT(builder.commandLine, Contains("-std=gnu++2a"));
}

TYPED_TEST(CommandLineBuilder, C89)
{
    this->emptyProjectInfo.language = Utils::Language::C;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::C89;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.c"};

    ASSERT_THAT(builder.commandLine, Contains("-std=c89"));
}

TYPED_TEST(CommandLineBuilder, C99)
{
    this->emptyProjectInfo.language = Utils::Language::C;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::C99;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.c"};

    ASSERT_THAT(builder.commandLine, Contains("-std=c99"));
}

TYPED_TEST(CommandLineBuilder, C11)
{
    this->emptyProjectInfo.language = Utils::Language::C;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::C11;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.c"};

    ASSERT_THAT(builder.commandLine, Contains("-std=c11"));
}

TYPED_TEST(CommandLineBuilder, C18)
{
    this->emptyProjectInfo.language = Utils::Language::C;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::C18;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.c"};

    ASSERT_THAT(builder.commandLine, Contains("-std=c18"));
}

TYPED_TEST(CommandLineBuilder, GnuC89)
{
    this->emptyProjectInfo.language = Utils::Language::C;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::C89;
    this->emptyProjectInfo.languageExtension = Utils::LanguageExtension::Gnu;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.c"};

    ASSERT_THAT(builder.commandLine, Contains("-std=gnu89"));
}

TYPED_TEST(CommandLineBuilder, GnuC99)
{
    this->emptyProjectInfo.language = Utils::Language::C;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::C99;
    this->emptyProjectInfo.languageExtension = Utils::LanguageExtension::Gnu;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.c"};

    ASSERT_THAT(builder.commandLine, Contains("-std=gnu99"));
}

TYPED_TEST(CommandLineBuilder, GnuC11)
{
    this->emptyProjectInfo.language = Utils::Language::C;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::C11;
    this->emptyProjectInfo.languageExtension = Utils::LanguageExtension::Gnu;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.c"};

    ASSERT_THAT(builder.commandLine, Contains("-std=gnu11"));
}

TYPED_TEST(CommandLineBuilder, GnuC18)
{
    this->emptyProjectInfo.language = Utils::Language::C;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::C18;
    this->emptyProjectInfo.languageExtension = Utils::LanguageExtension::Gnu;

    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.c"};

    ASSERT_THAT(builder.commandLine, Contains("-std=gnu18"));
}

TYPED_TEST(CommandLineBuilder, IncludesOrder)
{
    this->emptyProjectInfo.language = Utils::Language::Cxx;
    this->emptyProjectInfo.languageVersion = Utils::LanguageVersion::CXX11;
    this->emptyProjectInfo.projectIncludeSearchPaths = {{"/include/bar", 2, IncludeSearchPathType::User},
                                           {"/include/foo", 1, IncludeSearchPathType::User}};
    this->emptyProjectInfo.systemIncludeSearchPaths = {{"/system/bar", 4, IncludeSearchPathType::System},
                                          {"/system/foo", 3, IncludeSearchPathType::Framework},
                                          {"/builtin/bar", 2, IncludeSearchPathType::BuiltIn},
                                          {"/builtin/foo", 1, IncludeSearchPathType::BuiltIn}};
    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.cpp"};

    ASSERT_THAT(builder.commandLine,
                ElementsAre("clang++",
                            "-x",
                            "c++-header",
                            "-std=c++11",
                            "-nostdinc",
                            "-nostdlibinc",
                            "-I",
                            "/include/foo",
                            "-I",
                            "/include/bar",
                            "-F",
                            "/system/foo",
                            "-isystem",
                            "/system/bar",
                            "-isystem",
                            "/builtin/foo",
                            "-isystem",
                            "/builtin/bar",
                            "/source/file.cpp"));
}

TYPED_TEST(CommandLineBuilder, EmptySourceFile)
{
    Builder<TypeParam> builder{this->emptyProjectInfo, {}, {}};

    ASSERT_THAT(builder.commandLine,
                ElementsAre("clang++", "-x", "c++-header", "-std=c++98", "-nostdinc", "-nostdlibinc"));
}

TYPED_TEST(CommandLineBuilder, SourceFile)
{
    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.cpp"};

    ASSERT_THAT(builder.commandLine,
                ElementsAre("clang++",
                            "-x",
                            "c++-header",
                            "-std=c++98",
                            "-nostdinc",
                            "-nostdlibinc",
                            "/source/file.cpp"));
}


TYPED_TEST(CommandLineBuilder, EmptyOutputFile)
{
    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.cpp", ""};

    ASSERT_THAT(builder.commandLine,
                ElementsAre("clang++",
                            "-x",
                            "c++-header",
                            "-std=c++98",
                            "-nostdinc",
                            "-nostdlibinc",
                            "/source/file.cpp"));
}

TYPED_TEST(CommandLineBuilder, OutputFile)
{
    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.cpp", "/output/file.o"};

    ASSERT_THAT(builder.commandLine,
                ElementsAre("clang++",
                            "-x",
                            "c++-header",
                            "-std=c++98",
                            "-nostdinc",
                            "-nostdlibinc",
                            "-o",
                            "/output/file.o",
                            "/source/file.cpp"));
}

TYPED_TEST(CommandLineBuilder, IncludePchPath)
{
    Builder<TypeParam> builder{this->emptyProjectInfo, {}, "/source/file.cpp", "/output/file.o", "/pch/file.pch"};

    ASSERT_THAT(builder.commandLine,
                ElementsAre("clang++",
                            "-x",
                            "c++-header",
                            "-std=c++98",
                            "-nostdinc",
                            "-nostdlibinc",
                            "-Xclang",
                            "-include-pch",
                            "-Xclang",
                            "/pch/file.pch",
                            "-o",
                            "/output/file.o",
                            "/source/file.cpp"));
}

TYPED_TEST(CommandLineBuilder, CompilerMacros)
{
    this->emptyProjectInfo.compilerMacros = {{"YI", "1", 2}, {"ER", "2", 1}, {"SAN"}};

    Builder<TypeParam> builder{this->emptyProjectInfo};

    ASSERT_THAT(builder.commandLine,
                ElementsAre("clang++",
                            "-x",
                            "c++-header",
                            "-std=c++98",
                            "-nostdinc",
                            "-nostdlibinc",
                            "-DER=2",
                            "-DYI=1"));
}
} // namespace
