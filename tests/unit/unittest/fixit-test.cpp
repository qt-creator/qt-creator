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

#include <diagnostic.h>
#include <diagnosticset.h>
#include <clangdocument.h>
#include <clangdocuments.h>
#include <clangtranslationunit.h>
#include <unsavedfiles.h>
#include <sourcelocation.h>
#include <sourcerange.h>
#include <fixit.h>

#include <clang-c/Index.h>

using ClangBackEnd::DiagnosticSet;
using ClangBackEnd::Document;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::Diagnostic;
using ClangBackEnd::FixIt;
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

struct Data
{
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{unsavedFiles};
    Document document{Utf8StringLiteral(TESTDATA_DIR"/diagnostic_semicolon_fixit.cpp"),
                      {},
                      {},
                      documents};
    UnitTest::RunDocumentParse _1{document};
    TranslationUnit translationUnit{document.translationUnit()};
    DiagnosticSet diagnosticSet{translationUnit.diagnostics()};
    Diagnostic diagnostic{diagnosticSet.front()};
    ClangBackEnd::FixIt fixIt{diagnostic.fixIts().front()};
};

class FixIt : public ::testing::Test
{
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

protected:
    static  std::unique_ptr<const Data> data;
    const Diagnostic &diagnostic{data->diagnostic};
    const ClangBackEnd::FixIt &fixIt{data->fixIt};
};

std::unique_ptr<const Data> FixIt::data;

TEST_F(FixIt, Size)
{
    ASSERT_THAT(diagnostic.fixIts().size(), 1);
}

TEST_F(FixIt, Text)
{
    ASSERT_THAT(fixIt.text(), Utf8StringLiteral(";"));
}


TEST_F(FixIt, DISABLED_ON_WINDOWS(Start))
{
    ASSERT_THAT(fixIt.range().start(), IsSourceLocation(Utf8StringLiteral("diagnostic_semicolon_fixit.cpp"),
                                                        3u,
                                                        13u,
                                                        29u));
}

TEST_F(FixIt, DISABLED_ON_WINDOWS(End))
{
    ASSERT_THAT(fixIt.range().end(), IsSourceLocation(Utf8StringLiteral("diagnostic_semicolon_fixit.cpp"),
                                                      3u,
                                                      13u,
                                                      29u));
}

void FixIt::SetUpTestCase()
{
    data = std::make_unique<const Data>();
}

void FixIt::TearDownTestCase()
{
    data.reset();
}

} // anonymous
