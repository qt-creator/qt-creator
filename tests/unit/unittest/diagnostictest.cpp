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

#include <diagnostic.h>
#include <diagnosticcontainer.h>
#include <diagnosticset.h>
#include <fixitcontainer.h>
#include <projectpart.h>
#include <clangtranslationunit.h>
#include <translationunits.h>
#include <projects.h>
#include <unsavedfiles.h>
#include <sourcelocation.h>
#include <sourcerange.h>

#include <clang-c/Index.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "gtest-qt-printing.h"
#include "matcher-diagnosticcontainer.h"

using ::testing::Contains;
using ::testing::Not;
using ::testing::PrintToString;

using ClangBackEnd::DiagnosticSet;
using ClangBackEnd::DiagnosticContainer;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::ProjectPart;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::Diagnostic;
using ClangBackEnd::SourceLocation;
using ClangBackEnd::DiagnosticSeverity;
using ClangBackEnd::TranslationUnits;
using ClangBackEnd::FixItContainer;
using ClangBackEnd::SourceLocationContainer;

namespace {

MATCHER_P4(IsSourceLocation, filePath, line, column, offset,
           std::string(negation ? "isn't" : "is")
           + " source location with file path "+ PrintToString(filePath)
           + ", line " + PrintToString(line)
           + ", column " + PrintToString(column)
           + " and offset " + PrintToString(offset)
           )
{
    if (!arg.filePath().endsWith(filePath)
            || arg.line() != line
            || arg.column() != column
            || arg.offset() != offset)
        return false;

    return true;
}

class Diagnostic : public ::testing::Test
{
protected:
    ProjectPart projectPart{Utf8StringLiteral("projectPartId"), {Utf8StringLiteral("-std=c++11")}};
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::TranslationUnits translationUnits{projects, unsavedFiles};
    TranslationUnit translationUnit{Utf8StringLiteral(TESTDATA_DIR"/diagnostic_diagnostic.cpp"),
                                    projectPart,
                                    Utf8StringVector(),
                                    translationUnits};
    DiagnosticSet diagnosticSet{translationUnit.diagnostics()};
    ::Diagnostic diagnostic{diagnosticSet.back()};

protected:
    enum ChildMode { WithChild, WithoutChild };
    DiagnosticContainer expectedDiagnostic(ChildMode childMode) const;
};

TEST_F(Diagnostic, MoveContructor)
{
    const auto diagnostic2 = std::move(diagnostic);

    ASSERT_TRUE(diagnostic.isNull());
    ASSERT_FALSE(diagnostic2.isNull());
}

TEST_F(Diagnostic, MoveAssigment)
{
    auto diagnostic2 = std::move(diagnostic);
    diagnostic = std::move(diagnostic2);

    ASSERT_TRUE(diagnostic2.isNull());
    ASSERT_FALSE(diagnostic.isNull());
}

TEST_F(Diagnostic, MoveSelfAssigment)
{
    diagnostic = std::move(diagnostic);

    ASSERT_FALSE(diagnostic.isNull());
}

TEST_F(Diagnostic, Text)
{
    ASSERT_THAT(diagnostic.text(), Utf8StringLiteral("warning: control reaches end of non-void function"));
}

TEST_F(Diagnostic, Category)
{
    ASSERT_THAT(diagnostic.category(), Utf8StringLiteral("Semantic Issue"));
}

TEST_F(Diagnostic, EnableOption)
{
    ASSERT_THAT(diagnostic.options().first, Utf8StringLiteral("-Wreturn-type"));
}

TEST_F(Diagnostic, DisableOption)
{
    ASSERT_THAT(diagnostic.options().second, Utf8StringLiteral("-Wno-return-type"));
}

TEST_F(Diagnostic, Severity)
{
    ASSERT_THAT(diagnostic.severity(), DiagnosticSeverity::Warning);
}

TEST_F(Diagnostic, ChildDiagnosticsSize)
{
    auto diagnostic = diagnosticSet.front();

    ASSERT_THAT(diagnostic.childDiagnostics().size(), 1);
}

TEST_F(Diagnostic, ChildDiagnosticsText)
{
    auto childDiagnostic = diagnosticSet.front().childDiagnostics().front();

    ASSERT_THAT(childDiagnostic.text(), Utf8StringLiteral("note: previous declaration is here"));
}

TEST_F(Diagnostic, toDiagnosticContainerLetChildrenThroughByDefault)
{
    const auto diagnosticWithChild = expectedDiagnostic(WithChild);

    const auto diagnostic = diagnosticSet.front().toDiagnosticContainer();

    ASSERT_THAT(diagnostic, IsDiagnosticContainer(diagnosticWithChild));
}

DiagnosticContainer Diagnostic::expectedDiagnostic(Diagnostic::ChildMode childMode) const
{
    QVector<DiagnosticContainer> children;
    if (childMode == WithChild) {
        const auto child = DiagnosticContainer(
            Utf8StringLiteral("note: previous declaration is here"),
            Utf8StringLiteral("Semantic Issue"),
            {Utf8String(), Utf8String()},
            ClangBackEnd::DiagnosticSeverity::Note,
            SourceLocationContainer(translationUnit.filePath(), 2, 5),
            {},
            {},
            {}
        );
        children.append(child);
    }

    return
        DiagnosticContainer(
            Utf8StringLiteral("warning: 'X' is missing exception specification 'noexcept'"),
            Utf8StringLiteral("Semantic Issue"),
            {Utf8String(), Utf8String()},
            ClangBackEnd::DiagnosticSeverity::Warning,
            SourceLocationContainer(translationUnit.filePath(), 5, 4),
            {},
            {},
            children
        );
}

}
