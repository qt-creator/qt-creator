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

#include <cursor.h>
#include <clangstring.h>
#include <projectpart.h>
#include <projects.h>
#include <skippedsourceranges.h>
#include <sourcelocation.h>
#include <sourcerange.h>
#include <clangtranslationunit.h>
#include <translationunits.h>
#include <unsavedfiles.h>

#include <sourcerangecontainer.h>

#include <QVector>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

using ClangBackEnd::Cursor;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::ProjectPart;
using ClangBackEnd::TranslationUnits;
using ClangBackEnd::ClangString;
using ClangBackEnd::SourceRange;
using ClangBackEnd::SkippedSourceRanges;

using testing::IsNull;
using testing::NotNull;
using testing::Gt;
using testing::Contains;
using testing::EndsWith;
using testing::AllOf;
using testing::Not;
using testing::IsEmpty;
using testing::SizeIs;
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
     || arg.offset() != offset) {
        return false;
    }

    return true;
}

struct Data {
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::TranslationUnits translationUnits{projects, unsavedFiles};
    Utf8String filePath = Utf8StringLiteral(TESTDATA_DIR"/skippedsourceranges.cpp");
    TranslationUnit translationUnit{filePath,
                ProjectPart(Utf8StringLiteral("projectPartId"),
                            {Utf8StringLiteral("-std=c++11"),Utf8StringLiteral("-DBLAH")}),
                {},
                translationUnits};
};

class SkippedSourceRanges : public ::testing::Test
{
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

protected:
    static Data *d;
    const TranslationUnit &translationUnit = d->translationUnit;
    const Utf8String &filePath = d->filePath;
    const ::SkippedSourceRanges skippedSourceRanges{d->translationUnit.skippedSourceRanges()};
};

Data *SkippedSourceRanges::d;

TEST_F(SkippedSourceRanges, RangeWithZero)
{
    auto ranges = skippedSourceRanges.sourceRanges();

    ASSERT_THAT(ranges, SizeIs(2));
}

TEST_F(SkippedSourceRanges, RangeOne)
{
    auto ranges = skippedSourceRanges.sourceRanges();

    ASSERT_THAT(ranges[0].start(), IsSourceLocation(filePath, 1, 2, 1));
    ASSERT_THAT(ranges[0].end(), IsSourceLocation(filePath, 5, 7, 24));
}

TEST_F(SkippedSourceRanges, RangeTwo)
{
    auto ranges = skippedSourceRanges.sourceRanges();

    ASSERT_THAT(ranges[1].start(), IsSourceLocation(filePath, 7, 2, 27));
    ASSERT_THAT(ranges[1].end(), IsSourceLocation(filePath, 12, 7, 63));
}

TEST_F(SkippedSourceRanges, RangeContainerSize)
{
    auto ranges = skippedSourceRanges.toSourceRangeContainers();

    ASSERT_THAT(ranges, SizeIs(2));
}

void SkippedSourceRanges::SetUpTestCase()
{
    d = new Data;
}

void SkippedSourceRanges::TearDownTestCase()
{
    delete d;
    d = nullptr;
}

}
