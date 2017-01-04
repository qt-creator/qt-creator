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
#include "diagnosticcontainer-matcher.h"

#include <diagnostic.h>
#include <diagnosticcontainer.h>
#include <diagnosticset.h>
#include <fixitcontainer.h>
#include <projectpart.h>
#include <clangdocument.h>
#include <clangdocuments.h>
#include <clangtranslationunit.h>
#include <projects.h>
#include <unsavedfiles.h>
#include <sourcelocation.h>
#include <sourcerange.h>

#include <clang-c/Index.h>


using ::testing::Contains;
using ::testing::Not;
using ::testing::PrintToString;

using ClangBackEnd::DiagnosticSet;
using ClangBackEnd::DiagnosticContainer;
using ClangBackEnd::Document;
using ClangBackEnd::Documents;
using ClangBackEnd::ProjectPart;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::Diagnostic;
using ClangBackEnd::SourceLocation;
using ClangBackEnd::DiagnosticSeverity;
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

struct DiagnosticData {
    DiagnosticData(Document &document)
        : diagnosticSet{document.translationUnit().diagnostics()}
        , diagnostic{diagnosticSet.front()}
    {
    }

    DiagnosticSet diagnosticSet;
    ::Diagnostic diagnostic;
};

struct Data {
    Data()
    {
        document.parse();
        d.reset(new DiagnosticData(document));
    }

    ProjectPart projectPart{Utf8StringLiteral("projectPartId"), {Utf8StringLiteral("-std=c++11")}};
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{projects, unsavedFiles};
    Document document{Utf8StringLiteral(TESTDATA_DIR"/diagnostic_diagnostic.cpp"),
                      projectPart,
                      Utf8StringVector(),
                      documents};
    std::unique_ptr<DiagnosticData> d;
};

class Diagnostic : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

protected:
    Data *d;

protected:
    enum ChildMode { WithChild, WithoutChild };
    DiagnosticContainer expectedDiagnostic(ChildMode childMode) const;
};

using DiagnosticSlowTest = Diagnostic;

TEST_F(DiagnosticSlowTest, MoveContructor)
{
    const auto diagnostic2 = std::move(d->d->diagnostic);

    ASSERT_TRUE(d->d->diagnostic.isNull());
    ASSERT_FALSE(diagnostic2.isNull());
}

TEST_F(DiagnosticSlowTest, MoveAssigment)
{
    auto diagnostic2 = std::move(d->d->diagnostic);
    d->d->diagnostic = std::move(diagnostic2);

    ASSERT_TRUE(diagnostic2.isNull());
    ASSERT_FALSE(d->d->diagnostic.isNull());
}

TEST_F(DiagnosticSlowTest, MoveSelfAssigment)
{
    d->d->diagnostic = std::move(d->d->diagnostic);

    ASSERT_FALSE(d->d->diagnostic.isNull());
}

TEST_F(DiagnosticSlowTest, Text)
{
    ASSERT_THAT(d->d->diagnostic.text(), Utf8StringLiteral("warning: control reaches end of non-void function"));
}

TEST_F(DiagnosticSlowTest, Category)
{
    ASSERT_THAT(d->d->diagnostic.category(), Utf8StringLiteral("Semantic Issue"));
}

TEST_F(DiagnosticSlowTest, EnableOption)
{
    ASSERT_THAT(d->d->diagnostic.options().first, Utf8StringLiteral("-Wreturn-type"));
}

TEST_F(DiagnosticSlowTest, DisableOption)
{
    ASSERT_THAT(d->d->diagnostic.options().second, Utf8StringLiteral("-Wno-return-type"));
}

TEST_F(DiagnosticSlowTest, Severity)
{
    ASSERT_THAT(d->d->diagnostic.severity(), DiagnosticSeverity::Warning);
}

TEST_F(DiagnosticSlowTest, ChildDiagnosticsSize)
{
    auto diagnostic = d->d->diagnosticSet.back();

    ASSERT_THAT(diagnostic.childDiagnostics().size(), 1);
}

TEST_F(DiagnosticSlowTest, ChildDiagnosticsText)
{
    auto childDiagnostic = d->d->diagnosticSet.back().childDiagnostics().front();

    ASSERT_THAT(childDiagnostic.text(), Utf8StringLiteral("note: candidate function not viable: requires 1 argument, but 0 were provided"));
}

TEST_F(DiagnosticSlowTest, toDiagnosticContainerLetChildrenThroughByDefault)
{
    const auto diagnosticWithChild = expectedDiagnostic(WithChild);

    const auto diagnostic = d->d->diagnosticSet.back().toDiagnosticContainer();

    ASSERT_THAT(diagnostic, IsDiagnosticContainer(diagnosticWithChild));
}

void Diagnostic::SetUp()
{
    d = new Data;
}

void Diagnostic::TearDown()
{
    delete d;
    d = nullptr;
}

DiagnosticContainer Diagnostic::expectedDiagnostic(Diagnostic::ChildMode childMode) const
{
    QVector<DiagnosticContainer> children;
    if (childMode == WithChild) {
        const auto child = DiagnosticContainer(
            Utf8StringLiteral("note: candidate function not viable: requires 1 argument, but 0 were provided"),
            Utf8StringLiteral("Semantic Issue"),
            {Utf8String(), Utf8String()},
            ClangBackEnd::DiagnosticSeverity::Note,
            SourceLocationContainer(d->document.filePath(), 5, 6),
            {},
            {},
            {}
        );
        children.append(child);
    }

    return
        DiagnosticContainer(
            Utf8StringLiteral("error: no matching function for call to 'f'"),
            Utf8StringLiteral("Semantic Issue"),
            {Utf8String(), Utf8String()},
            ClangBackEnd::DiagnosticSeverity::Error,
            SourceLocationContainer(d->document.filePath(), 7, 5),
            {},
            {},
            children
        );
}

}
