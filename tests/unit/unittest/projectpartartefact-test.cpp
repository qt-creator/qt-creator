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

TEST(ProjectPartArtefact, CompilerArguments)
{
    ClangBackEnd::ProjectPartArtefact artefact{"[\"-DFoo\",\"-DBar\"]", "", "", 1};

    ASSERT_THAT(artefact.compilerArguments, ElementsAre(Eq("-DFoo"), Eq("-DBar")));
}

TEST(ProjectPartArtefact, EmptyCompilerArguments)
{
    ClangBackEnd::ProjectPartArtefact artefact{"", "",  "", 1};

    ASSERT_THAT(artefact.compilerArguments, IsEmpty());
}

TEST(ProjectPartArtefact, CompilerArgumentsParseError)
{
    ASSERT_THROW(ClangBackEnd::ProjectPartArtefact("\"-DFoo\",\"-DBar\"]", "",  "", 1),
                 ClangBackEnd::ProjectPartArtefactParseError);
}

TEST(ProjectPartArtefact, CompilerMacros)
{
    ClangBackEnd::ProjectPartArtefact artefact{"", "{\"Foo\":\"1\",\"Bar\":\"42\"}",  "", 1};

    ASSERT_THAT(artefact.compilerMacros,
                UnorderedElementsAre(Eq(CompilerMacro{"Foo", "1"}), Eq(CompilerMacro{"Bar", "42"})));
}

TEST(ProjectPartArtefact, EmptyCompilerMacros)
{
    ClangBackEnd::ProjectPartArtefact artefact{"", "",  "", 1};

    ASSERT_THAT(artefact.compilerMacros, IsEmpty());
}

TEST(ProjectPartArtefact, CompilerMacrosParseError)
{
    ASSERT_THROW(ClangBackEnd::ProjectPartArtefact("", "\"Foo\":\"1\",\"Bar\":\"42\"}", "", 1),
                 ClangBackEnd::ProjectPartArtefactParseError);
}

TEST(ProjectPartArtefact, IncludeSearchPaths)
{
    ClangBackEnd::ProjectPartArtefact artefact{"", "",  "[\"/includes\",\"/other/includes\"]", 1};

    ASSERT_THAT(artefact.includeSearchPaths, ElementsAre(Eq("/includes"), Eq("/other/includes")));
}

TEST(ProjectPartArtefact, EmptyIncludeSearchPaths)
{
    ClangBackEnd::ProjectPartArtefact artefact{"", "",  "", 1};

    ASSERT_THAT(artefact.includeSearchPaths, IsEmpty());
}

TEST(ProjectPartArtefact, IncludeSearchPathsParseError)
{
    ASSERT_THROW(ClangBackEnd::ProjectPartArtefact("", "",  "\"/includes\",\"/other/includes\"]", 1),
                 ClangBackEnd::ProjectPartArtefactParseError);
}

}
