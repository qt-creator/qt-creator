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

#include <clang-c/Index.h>

using ClangBackEnd::Diagnostic;
using ClangBackEnd::DiagnosticSet;
using ClangBackEnd::SourceLocation;
using ClangBackEnd::Document;
using ClangBackEnd::UnsavedFiles;

using testing::EndsWith;
using testing::Not;

namespace {

struct Data {
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{unsavedFiles};
    Document document{Utf8StringLiteral(TESTDATA_DIR"/diagnostic_source_location.cpp"),
                      {},
                      {},
                      documents};
    UnitTest::RunDocumentParse _1{document};
    DiagnosticSet diagnosticSet{document.translationUnit().diagnostics()};
    Diagnostic diagnostic{diagnosticSet.front()};
    ClangBackEnd::SourceLocation sourceLocation{diagnostic.location()};
};

class SourceLocation : public ::testing::Test
{
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

protected:
    static std::unique_ptr<const Data> data;
    const Document &document = data->document;
    const ClangBackEnd::SourceLocation &sourceLocation = data->sourceLocation;
};

TEST_F(SourceLocation, FilePath)
{
    ASSERT_THAT(sourceLocation.filePath().constData(), EndsWith("diagnostic_source_location.cpp"));
}

TEST_F(SourceLocation, Line)
{
    ASSERT_THAT(sourceLocation.line(), 4);
}

TEST_F(SourceLocation, Column)
{
    ASSERT_THAT(sourceLocation.column(), 1);
}

TEST_F(SourceLocation, Offset)
{
    ASSERT_THAT(sourceLocation.offset(), 18);
}

TEST_F(SourceLocation, Create)
{
    ASSERT_THAT(document.translationUnit().sourceLocationAt(4, 1), sourceLocation);
}

TEST_F(SourceLocation, NotEqual)
{
    ASSERT_THAT(document.translationUnit().sourceLocationAt(3, 1), Not(sourceLocation));
}

TEST_F(SourceLocation, BeforeMultibyteCharacter)
{
    ClangBackEnd::SourceLocation sourceLocation(
                document.translationUnit().cxTranslationUnit(),
                clang_getLocation(document.translationUnit().cxTranslationUnit(),
                                  clang_getFile(document.translationUnit().cxTranslationUnit(),
                                                document.filePath().constData()),
                                  8, 10));

    ASSERT_THAT(document.translationUnit().sourceLocationAt(8, 10).column(), sourceLocation.column());
}

TEST_F(SourceLocation, AfterMultibyteCharacter)
{
    ClangBackEnd::SourceLocation sourceLocation(
                document.translationUnit().cxTranslationUnit(),
                clang_getLocation(document.translationUnit().cxTranslationUnit(),
                                  clang_getFile(document.translationUnit().cxTranslationUnit(),
                                                document.filePath().constData()),
                                  8, 12));

    ASSERT_THAT(document.translationUnit().sourceLocationAt(8, 13).column(), sourceLocation.column());
}

std::unique_ptr<const Data> SourceLocation::data;

void SourceLocation::SetUpTestCase()
{
    data = std::make_unique<const Data>();
}

void SourceLocation::TearDownTestCase()
{
    data.reset();
}

}
