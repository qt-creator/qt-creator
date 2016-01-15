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

#include <clangcodecompleteresults.h>
#include <projectpart.h>
#include <projects.h>
#include <clangtranslationunit.h>
#include <translationunits.h>
#include <unsavedfiles.h>
#include <utf8string.h>

#include <clang-c/Index.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

namespace {

using ClangBackEnd::ClangCodeCompleteResults;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::ProjectPart;

TEST(ClangCodeCompleteResults, GetData)
{
    ProjectPart projectPart(Utf8StringLiteral("projectPartId"));
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::TranslationUnits translationUnits{projects, unsavedFiles};
    TranslationUnit translationUnit(Utf8StringLiteral(TESTDATA_DIR"/complete_testfile_1.cpp"),
                                    projectPart,
                                    Utf8StringVector(),
                                    translationUnits);
    CXCodeCompleteResults *cxCodeCompleteResults = clang_codeCompleteAt(translationUnit.cxTranslationUnit(), translationUnit.filePath().constData(), 49, 1, 0, 0, 0);

    ClangCodeCompleteResults codeCompleteResults(cxCodeCompleteResults);

    ASSERT_THAT(codeCompleteResults.data(), cxCodeCompleteResults);
}

TEST(ClangCodeCompleteResults, GetInvalidData)
{
    CXCodeCompleteResults *cxCodeCompleteResults = clang_codeCompleteAt(nullptr, "file.cpp", 49, 1, 0, 0, 0);

    ClangCodeCompleteResults codeCompleteResults(cxCodeCompleteResults);

    ASSERT_THAT(codeCompleteResults.data(), cxCodeCompleteResults);
}

TEST(ClangCodeCompleteResults, MoveClangCodeCompleteResults)
{
    ProjectPart projectPart(Utf8StringLiteral("projectPartId"));
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::TranslationUnits translationUnits{projects, unsavedFiles};
    TranslationUnit translationUnit(Utf8StringLiteral(TESTDATA_DIR"/complete_testfile_1.cpp"),
                                    projectPart,
                                    Utf8StringVector(),
                                    translationUnits);
    CXCodeCompleteResults *cxCodeCompleteResults = clang_codeCompleteAt(translationUnit.cxTranslationUnit(), translationUnit.filePath().constData(), 49, 1, 0, 0, 0);

    ClangCodeCompleteResults codeCompleteResults(cxCodeCompleteResults);

    const ClangCodeCompleteResults codeCompleteResults2 = std::move(codeCompleteResults);

    ASSERT_TRUE(codeCompleteResults.isNull());
    ASSERT_FALSE(codeCompleteResults2.isNull());
}

}
