/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <diagnostic.h>
#include <diagnosticset.h>
#include <projectpart.h>
#include <translationunit.h>
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

using ClangBackEnd::DiagnosticSet;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::ProjectPart;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::Diagnostic;
using ClangBackEnd::SourceLocation;
using ClangBackEnd::DiagnosticSeverity;
using ClangBackEnd::TranslationUnits;
using testing::PrintToString;

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
                                    translationUnits};
    DiagnosticSet diagnosticSet{translationUnit.diagnostics()};
    ::Diagnostic diagnostic{diagnosticSet.back()};
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

}
