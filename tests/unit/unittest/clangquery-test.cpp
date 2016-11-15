/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include "sourcerangecontainer-matcher.h"
#include "dynamicastmatcherdiagnosticcontainer-matcher.h"

#include <clangquery.h>

using ClangBackEnd::ClangQuery;

using testing::IsEmpty;
using testing::Not;
using testing::AllOf;

namespace {

class ClangQuery : public ::testing::Test
{
protected:
    void SetUp() override;

protected:
    ::ClangQuery simpleFunctionQuery;
    ::ClangQuery simpleClassQuery;
};


TEST_F(ClangQuery, NoSourceRangesForDefaultConstruction)
{
    auto sourceRanges = simpleFunctionQuery.takeSourceRanges();

    ASSERT_THAT(sourceRanges.sourceRangeWithTextContainers(), IsEmpty());
}

TEST_F(ClangQuery, SourceRangesForSimpleFunctionDeclarationAreNotEmpty)
{
    simpleFunctionQuery.setQuery("functionDecl()");

    simpleFunctionQuery.findLocations();

    ASSERT_THAT(simpleFunctionQuery.takeSourceRanges().sourceRangeWithTextContainers(), Not(IsEmpty()));
}

TEST_F(ClangQuery, RootSourceRangeForSimpleFunctionDeclarationRange)
{
    simpleFunctionQuery.setQuery("functionDecl()");

    simpleFunctionQuery.findLocations();

    ASSERT_THAT(simpleFunctionQuery.takeSourceRanges().sourceRangeWithTextContainers().at(0),
                IsSourceRangeWithText(1, 1, 8, 1, "int function(int* pointer, int value) { if (pointer == nullptr) { return value + 1; } else { return value - 1; } }"));
}

TEST_F(ClangQuery, RootSourceRangeForSimpleFieldDeclarationRange)
{
    simpleClassQuery.setQuery("fieldDecl(hasType(isInteger()))");

    simpleClassQuery.findLocations();

    ASSERT_THAT(simpleClassQuery.takeSourceRanges().sourceRangeWithTextContainers().at(0),
                IsSourceRangeWithText(4, 5, 4, 9, "int x"));
}

TEST_F(ClangQuery, NoSourceRangesForEmptyQuery)
{
    simpleClassQuery.findLocations();

    ASSERT_THAT(simpleClassQuery.takeSourceRanges().sourceRangeWithTextContainers(), IsEmpty());
}

TEST_F(ClangQuery, NoSourceRangesForWrongQuery)
{
    simpleClassQuery.setQuery("wrongQuery()");

    simpleClassQuery.findLocations();

    ASSERT_THAT(simpleClassQuery.takeSourceRanges().sourceRangeWithTextContainers(), IsEmpty());
}

TEST_F(ClangQuery, NoDiagnosticsForDefaultConstruction)
{
    auto diagnostics = simpleFunctionQuery.takeDiagnosticContainers();

    ASSERT_THAT(diagnostics, IsEmpty());
}

TEST_F(ClangQuery, DiagnosticsForEmptyQuery)
{
    simpleFunctionQuery.findLocations();

    ASSERT_THAT(simpleFunctionQuery.takeDiagnosticContainers(),
                Not(IsEmpty()));
}

TEST_F(ClangQuery, DiagnosticsForWrongQuery)
{
    simpleClassQuery.setQuery("wrongQuery()");

    simpleClassQuery.findLocations();

    ASSERT_THAT(simpleClassQuery.takeDiagnosticContainers(),
                Not(IsEmpty()));
}

TEST_F(ClangQuery, NoDiagnosticsForAccurateQuery)
{
    simpleFunctionQuery.setQuery("functionDecl()");

    simpleFunctionQuery.findLocations();

    ASSERT_THAT(simpleFunctionQuery.takeDiagnosticContainers(),
                IsEmpty());
}

TEST_F(ClangQuery, DiagnosticForWrongQuery)
{
    simpleClassQuery.setQuery("wrongQuery()");

    simpleClassQuery.findLocations();

    ASSERT_THAT(simpleClassQuery.takeDiagnosticContainers(),
                HasDiagnosticMessage("RegistryMatcherNotFound", 1, 1, 1, 11));
}

TEST_F(ClangQuery, DiagnosticForWrongArgumenType)
{
    simpleFunctionQuery.setQuery("functionDecl(1)");

    simpleFunctionQuery.findLocations();

    ASSERT_THAT(simpleFunctionQuery.takeDiagnosticContainers(),
                AllOf(HasDiagnosticMessage("RegistryWrongArgType", 1, 14, 1, 15),
                      HasDiagnosticContext("MatcherConstruct", 1, 1, 1, 13)));
}

void ClangQuery::SetUp()
{
    simpleFunctionQuery.addFile(TESTDATA_DIR, "query_simplefunction.cpp", "", {"cc", "query_simplefunction.cpp", "-std=c++14"});
    simpleClassQuery.addFile(TESTDATA_DIR, "query_simpleclass.cpp", "", {"cc", "query_simpleclass.cpp", "-std=c++14"});

}
}
