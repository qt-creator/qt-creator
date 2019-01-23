/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <projectpartartefact.h>

namespace {

using ClangBackEnd::CompilerMacro;
using ClangBackEnd::IncludeSearchPath;
using ClangBackEnd::IncludeSearchPathType;

TEST(ProjectPartArtefact, CompilerArguments)
{
    ClangBackEnd::ProjectPartArtefact artefact{"[\"-DFoo\",\"-DBar\"]",
                                               "",
                                               "",
                                               "",
                                               1,
                                               Utils::Language::Cxx,
                                               Utils::LanguageVersion::CXX11,
                                               Utils::LanguageExtension::None};

    ASSERT_THAT(artefact.toolChainArguments, ElementsAre(Eq("-DFoo"), Eq("-DBar")));
}

TEST(ProjectPartArtefact, EmptyCompilerArguments)
{
    ClangBackEnd::ProjectPartArtefact artefact{"",
                                               "",
                                               "",
                                               "",
                                               1,
                                               Utils::Language::Cxx,
                                               Utils::LanguageVersion::CXX11,
                                               Utils::LanguageExtension::None};

    ASSERT_THAT(artefact.toolChainArguments, IsEmpty());
}

TEST(ProjectPartArtefact, CompilerArgumentsParseError)
{
    ASSERT_THROW(ClangBackEnd::ProjectPartArtefact("\"-DFoo\",\"-DBar\"]",
                                                   "",
                                                   "",
                                                   "",
                                                   1,
                                                   Utils::Language::Cxx,
                                                   Utils::LanguageVersion::CXX11,
                                                   Utils::LanguageExtension::None),
                 ClangBackEnd::ProjectPartArtefactParseError);
}

TEST(ProjectPartArtefact, CompilerMacros)
{
    ClangBackEnd::ProjectPartArtefact artefact{"",
                                               R"([["Foo","1",1], ["Bar","42",2]])",
                                               "",
                                               "",
                                               1,
                                               Utils::Language::Cxx,
                                               Utils::LanguageVersion::CXX11,
                                               Utils::LanguageExtension::None};

    ASSERT_THAT(artefact.compilerMacros,
                UnorderedElementsAre(Eq(CompilerMacro{"Foo", "1", 1}), Eq(CompilerMacro{"Bar", "42", 2})));
}

TEST(ProjectPartArtefact, EmptyCompilerMacros)
{
    ClangBackEnd::ProjectPartArtefact artefact{"",
                                               "",
                                               "",
                                               "",
                                               1,
                                               Utils::Language::Cxx,
                                               Utils::LanguageVersion::CXX11,
                                               Utils::LanguageExtension::None};

    ASSERT_THAT(artefact.compilerMacros, IsEmpty());
}

TEST(ProjectPartArtefact, CompilerMacrosParseError)
{
    ASSERT_THROW(ClangBackEnd::ProjectPartArtefact("",
                                                   R"([["Foo":"1", 1], ["Bar", "42", 2]])",
                                                   "",
                                                   "",
                                                   1,
                                                   Utils::Language::Cxx,
                                                   Utils::LanguageVersion::CXX11,
                                                   Utils::LanguageExtension::None),
                 ClangBackEnd::ProjectPartArtefactParseError);
}

TEST(ProjectPartArtefact, SystemIncludeSearchPaths)
{
    ClangBackEnd::ProjectPartArtefact artefact{"",
                                               "",
                                               R"([["/includes", 1, 2], ["/other/includes", 2, 3]])",
                                               "",
                                               1,
                                               Utils::Language::Cxx,
                                               Utils::LanguageVersion::CXX11,
                                               Utils::LanguageExtension::None};

    ASSERT_THAT(
        artefact.systemIncludeSearchPaths,
        ElementsAre(Eq(IncludeSearchPath("/includes", 1, IncludeSearchPathType::BuiltIn)),
                    Eq(IncludeSearchPath("/other/includes", 2, IncludeSearchPathType::System))));
}

TEST(ProjectPartArtefact, EmptySystemIncludeSearchPaths)
{
    ClangBackEnd::ProjectPartArtefact artefact{"",
                                               "",
                                               "",
                                               "",
                                               1,
                                               Utils::Language::Cxx,
                                               Utils::LanguageVersion::CXX11,
                                               Utils::LanguageExtension::None};

    ASSERT_THAT(artefact.systemIncludeSearchPaths, IsEmpty());
}

TEST(ProjectPartArtefact, ProjectIncludeSearchPaths)
{
    ClangBackEnd::ProjectPartArtefact artefact{
        "",
        "",
        R"([["/project/includes", 1, 1], ["/other/project/includes", 2, 1]])",
        "",
        1,
        Utils::Language::Cxx,
        Utils::LanguageVersion::CXX11,
        Utils::LanguageExtension::None};

    ASSERT_THAT(
        artefact.systemIncludeSearchPaths,
        ElementsAre(
            Eq(IncludeSearchPath("/project/includes", 1, IncludeSearchPathType::User)),
            Eq(IncludeSearchPath("/other/project/includes", 2, IncludeSearchPathType::User))));
}

TEST(ProjectPartArtefact, EmptyProjectIncludeSearchPaths)
{
    ClangBackEnd::ProjectPartArtefact artefact{"",
                                               "",
                                               "",
                                               "",
                                               1,
                                               Utils::Language::Cxx,
                                               Utils::LanguageVersion::CXX11,
                                               Utils::LanguageExtension::None};

    ASSERT_THAT(artefact.projectIncludeSearchPaths, IsEmpty());
}

TEST(ProjectPartArtefact, IncludeSystemSearchPathsParseError)
{
    ASSERT_THROW(ClangBackEnd::ProjectPartArtefact("",
                                                   "",
                                                   R"(["/includes", 1, 3], ["/other/includes", 2, 2]])",
                                                   "",
                                                   1,
                                                   Utils::Language::Cxx,
                                                   Utils::LanguageVersion::CXX11,
                                                   Utils::LanguageExtension::None),
                 ClangBackEnd::ProjectPartArtefactParseError);
}

TEST(ProjectPartArtefact, IncludeProjectSearchPathsParseError)
{
    ASSERT_THROW(ClangBackEnd::ProjectPartArtefact(
                     "",
                     "",
                     R"(["/project/includes", 1, 1], ["/other/project/includes", 2, 1]])",
                     "",
                     1,
                     Utils::Language::Cxx,
                     Utils::LanguageVersion::CXX11,
                     Utils::LanguageExtension::None),
                 ClangBackEnd::ProjectPartArtefactParseError);
}
}
