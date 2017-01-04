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

#include <clangcodecompleteresults.h>
#include <clangdocument.h>
#include <clangfilepath.h>
#include <clangtranslationunitupdater.h>
#include <projectpart.h>
#include <projects.h>
#include <clangtranslationunit.h>
#include <clangdocuments.h>
#include <unsavedfiles.h>
#include <utf8string.h>

#include <clang-c/Index.h>

namespace {

using ClangBackEnd::ClangCodeCompleteResults;
using ClangBackEnd::FilePath;
using ClangBackEnd::Document;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::ProjectPart;

static unsigned completionOptions()
{
    return ClangBackEnd::TranslationUnitUpdater::defaultParseOptions()
                & CXTranslationUnit_IncludeBriefCommentsInCodeCompletion
            ? CXCodeComplete_IncludeBriefComments
            : 0;
}

TEST(ClangCodeCompleteResultsSlowTest, GetData)
{
    ProjectPart projectPart(Utf8StringLiteral("projectPartId"));
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{projects, unsavedFiles};
    Document document(Utf8StringLiteral(TESTDATA_DIR"/complete_testfile_1.cpp"),
                      projectPart,
                      Utf8StringVector(),
                      documents);
    Utf8String nativeFilePath = FilePath::toNativeSeparators(document.filePath());
    document.parse();
    CXCodeCompleteResults *cxCodeCompleteResults =
            clang_codeCompleteAt(document.translationUnit().cxTranslationUnit(),
                                 nativeFilePath.constData(),
                                 49, 1, 0, 0,
                                 completionOptions());

    ClangCodeCompleteResults codeCompleteResults(cxCodeCompleteResults);

    ASSERT_THAT(codeCompleteResults.data(), cxCodeCompleteResults);
}

TEST(ClangCodeCompleteResults, GetInvalidData)
{
    CXCodeCompleteResults *cxCodeCompleteResults = clang_codeCompleteAt(nullptr, "file.cpp", 49, 1, 0, 0, 0);

    ClangCodeCompleteResults codeCompleteResults(cxCodeCompleteResults);

    ASSERT_THAT(codeCompleteResults.data(), cxCodeCompleteResults);
}

TEST(ClangCodeCompleteResultsSlowTest, MoveClangCodeCompleteResults)
{
    ProjectPart projectPart(Utf8StringLiteral("projectPartId"));
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{projects, unsavedFiles};
    Document document(Utf8StringLiteral(TESTDATA_DIR"/complete_testfile_1.cpp"),
                      projectPart,
                      Utf8StringVector(),
                      documents);
    Utf8String nativeFilePath = FilePath::toNativeSeparators(document.filePath());
    document.parse();
    CXCodeCompleteResults *cxCodeCompleteResults =
            clang_codeCompleteAt(document.translationUnit().cxTranslationUnit(),
                                 nativeFilePath.constData(),
                                 49, 1, 0, 0,
                                 completionOptions());

    ClangCodeCompleteResults codeCompleteResults(cxCodeCompleteResults);

    const ClangCodeCompleteResults codeCompleteResults2 = std::move(codeCompleteResults);

    ASSERT_TRUE(codeCompleteResults.isNull());
    ASSERT_FALSE(codeCompleteResults2.isNull());
}

}
