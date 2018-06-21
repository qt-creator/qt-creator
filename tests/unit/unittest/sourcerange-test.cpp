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
#include "rundocumentparse-utility.h"
#include "testenvironment.h"

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

struct Data {
    ProjectPart projectPart{Utf8StringLiteral("projectPartId"),
                            TestEnvironment::addPlatformArguments({Utf8StringLiteral("-pedantic")})};
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{projects, unsavedFiles};
    Utf8String filePath{Utf8StringLiteral(TESTDATA_DIR"/diagnostic_source_range.cpp")};
    Document document{filePath,
                      projectPart,
                      Utf8StringVector(),
                      documents};
    UnitTest::RunDocumentParse _1{document};
    TranslationUnit translationUnit{filePath,
                                    filePath,
                                    document.translationUnit().cxIndex(),
                                    document.translationUnit().cxTranslationUnit()};
    DiagnosticSet diagnosticSet{document.translationUnit().diagnostics()};
    Diagnostic diagnostic{diagnosticSet.front()};
    Diagnostic diagnosticWithFilteredOutInvalidRange{diagnosticSet.at(1)};
    ClangBackEnd::SourceRange sourceRange{diagnostic.ranges().front()};
};

class SourceRange : public ::testing::Test
{
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

protected:
    static std::unique_ptr<const Data> data;
    const ::SourceRange &sourceRange = data->sourceRange;
    const Diagnostic &diagnostic = data->diagnostic;
    const Diagnostic &diagnosticWithFilteredOutInvalidRange = data->diagnosticWithFilteredOutInvalidRange;
    const TranslationUnit &translationUnit = data->translationUnit;
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

std::unique_ptr<const Data> SourceRange::data;

void SourceRange::SetUpTestCase()
{
    data = std::make_unique<const Data>();
}

void SourceRange::TearDownTestCase()
{
    data.reset();
}

}
