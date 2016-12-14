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

#include <diagnostic.h>
#include <diagnosticset.h>
#include <projectpart.h>
#include <projects.h>
#include <clangdocument.h>
#include <clangdocuments.h>
#include <clangtranslationunit.h>
#include <unsavedfiles.h>
#include <sourcelocation.h>

#include <clang-c/Index.h>

using ClangBackEnd::Diagnostic;
using ClangBackEnd::DiagnosticSet;
using ClangBackEnd::ProjectPart;
using ClangBackEnd::SourceLocation;
using ClangBackEnd::Document;
using ClangBackEnd::UnsavedFiles;

using testing::EndsWith;
using testing::Not;

namespace {

struct SourceLocationData {
    SourceLocationData(Document &document)
        : diagnosticSet{document.translationUnit().diagnostics()}
        , diagnostic{diagnosticSet.front()}
        , sourceLocation{diagnostic.location()}
    {
    }

    DiagnosticSet diagnosticSet;
    Diagnostic diagnostic;
    ::SourceLocation sourceLocation;
};

struct Data {
    Data()
    {
        document.parse();
        d.reset(new SourceLocationData(document));
    }

    ProjectPart projectPart{Utf8StringLiteral("projectPartId")};
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{projects, unsavedFiles};
    Document document{Utf8StringLiteral(TESTDATA_DIR"/diagnostic_source_location.cpp"),
                      projectPart,
                      Utf8StringVector(),
                      documents};
    std::unique_ptr<SourceLocationData> d;
};

class SourceLocation : public ::testing::Test
{
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

protected:
    static Data *d;
    Document &document = d->document;
    ::SourceLocation &sourceLocation = d->d->sourceLocation;
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

TEST_F(SourceLocation, DISABLED_ON_WINDOWS(Offset))
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

Data *SourceLocation::d;

void SourceLocation::SetUpTestCase()
{
    d = new Data;
}

void SourceLocation::TearDownTestCase()
{
    delete d;
    d = nullptr;
}

}
