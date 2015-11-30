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
using ClangBackEnd::SourceRange;
using ClangBackEnd::TranslationUnits;

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
    ProjectPart projectPart{Utf8StringLiteral("projectPartId"), {Utf8StringLiteral("-pedantic")}};
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::TranslationUnits translationUnits{projects, unsavedFiles};
    TranslationUnit translationUnit{Utf8StringLiteral(TESTDATA_DIR"/diagnostic_source_range.cpp"),
                                    projectPart,
                                    Utf8StringVector(),
                                    translationUnits};
    DiagnosticSet diagnosticSet{translationUnit.diagnostics()};
    Diagnostic diagnostic{diagnosticSet.front()};
    Diagnostic diagnosticWithFilteredOutInvalidRange{diagnosticSet.at(1)};
    ::SourceRange sourceRange{diagnostic.ranges().front()};
};

class SourceRange : public ::testing::Test
{
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

protected:
    static Data *d;
    const ::SourceRange &sourceRange = d->sourceRange;
    const Diagnostic &diagnostic = d->diagnostic;
    const Diagnostic &diagnosticWithFilteredOutInvalidRange = d->diagnosticWithFilteredOutInvalidRange;
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

TEST_F(SourceRange, Start)
{
    ASSERT_THAT(sourceRange.start(), IsSourceLocation(Utf8StringLiteral("diagnostic_source_range.cpp"),
                                                      8u,
                                                      5u,
                                                      43u));
}

TEST_F(SourceRange, End)
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
