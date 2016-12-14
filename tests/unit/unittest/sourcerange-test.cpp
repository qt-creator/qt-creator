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

#include <clangtranslationunit.h>
#include <diagnostic.h>
#include <diagnosticset.h>
#include <projectpart.h>
#include <clangdocument.h>
#include <clangdocuments.h>
#include <projects.h>
#include <unsavedfiles.h>
#include <sourcerange.h>

#include <clang-c/Index.h>

#include <memory>

using ClangBackEnd::DiagnosticSet;
using ClangBackEnd::Document;
using ClangBackEnd::Documents;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::ProjectPart;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::Diagnostic;
using ClangBackEnd::SourceRange;

using testing::PrintToString;
using testing::IsEmpty;

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
     || arg.offset() != offset) {
        return false;
    }

    return true;
}

struct SourceRangeData {
    SourceRangeData(Document &document)
        : diagnosticSet{document.translationUnit().diagnostics()}
        , diagnostic{diagnosticSet.front()}
        , diagnosticWithFilteredOutInvalidRange{diagnosticSet.at(1)}
        , sourceRange{diagnostic.ranges().front()}
    {
    }

    DiagnosticSet diagnosticSet;
    Diagnostic diagnostic;
    Diagnostic diagnosticWithFilteredOutInvalidRange;
    ::SourceRange sourceRange;
};

struct Data {
    Data()
    {
        document.parse();
        d.reset(new SourceRangeData(document));
    }

    ProjectPart projectPart{Utf8StringLiteral("projectPartId"), {Utf8StringLiteral("-pedantic")}};
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{projects, unsavedFiles};
    Utf8String filePath{Utf8StringLiteral(TESTDATA_DIR"/diagnostic_source_range.cpp")};
    Document document{filePath,
                      projectPart,
                      Utf8StringVector(),
                      documents};
    TranslationUnit translationUnit{filePath,
                                    filePath,
                                    document.translationUnit().cxIndex(),
                                    document.translationUnit().cxTranslationUnit()};

    std::unique_ptr<SourceRangeData> d;
};

class SourceRange : public ::testing::Test
{
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

protected:
    static Data *d;
    const ::SourceRange &sourceRange = d->d->sourceRange;
    const Diagnostic &diagnostic = d->d->diagnostic;
    const Diagnostic &diagnosticWithFilteredOutInvalidRange = d->d->diagnosticWithFilteredOutInvalidRange;
    const TranslationUnit &translationUnit = d->translationUnit;
};

TEST_F(SourceRange, IsNull)
{
    ::SourceRange sourceRange;

    ASSERT_TRUE(sourceRange.isNull());
}

TEST_F(SourceRange, IsNotNull)
{
    ::SourceRange sourceRange = diagnostic.ranges()[0];

    ASSERT_FALSE(sourceRange.isNull());
}

TEST_F(SourceRange, Size)
{
    ASSERT_THAT(diagnostic.ranges().size(), 2);
}

TEST_F(SourceRange, DISABLED_ON_WINDOWS(Start))
{
    ASSERT_THAT(sourceRange.start(), IsSourceLocation(Utf8StringLiteral("diagnostic_source_range.cpp"),
                                                      8u,
                                                      5u,
                                                      43u));
}

TEST_F(SourceRange, DISABLED_ON_WINDOWS(End))
{
    ASSERT_THAT(sourceRange.end(), IsSourceLocation(Utf8StringLiteral("diagnostic_source_range.cpp"),
                                                      8u,
                                                      6u,
                                                      44u));
}

TEST_F(SourceRange, Create)
{
    ASSERT_THAT(sourceRange, ::SourceRange(sourceRange.start(), sourceRange.end()));
}

TEST_F(SourceRange, SourceRangeFromTranslationUnit)
{
    auto sourceRangeFromTranslationUnit = translationUnit.sourceRange(8u, 5u, 8u, 6u);

    ASSERT_THAT(sourceRangeFromTranslationUnit, sourceRange);
}

TEST_F(SourceRange, InvalidRangeIsFilteredOut)
{
    ASSERT_THAT(diagnosticWithFilteredOutInvalidRange.ranges(), IsEmpty());
}

Data *SourceRange::d;

void SourceRange::SetUpTestCase()
{
    d = new Data;
}

void SourceRange::TearDownTestCase()
{
    delete d;
    d = nullptr;
}

}
