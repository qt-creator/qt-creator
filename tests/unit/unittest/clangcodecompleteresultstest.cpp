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
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <clangcodecompleteresults.h>
#include <projectpart.h>
#include <translationunit.h>
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
    UnsavedFiles unsavedFiles;
    TranslationUnit translationUnit(Utf8StringLiteral(TESTDATA_DIR"/complete_testfile_1.cpp"), unsavedFiles, projectPart);
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
    UnsavedFiles unsavedFiles;
    TranslationUnit translationUnit(Utf8StringLiteral(TESTDATA_DIR"/complete_testfile_1.cpp"), unsavedFiles, projectPart);
    CXCodeCompleteResults *cxCodeCompleteResults = clang_codeCompleteAt(translationUnit.cxTranslationUnit(), translationUnit.filePath().constData(), 49, 1, 0, 0, 0);

    ClangCodeCompleteResults codeCompleteResults(cxCodeCompleteResults);

    const ClangCodeCompleteResults codeCompleteResults2 = std::move(codeCompleteResults);

    ASSERT_TRUE(codeCompleteResults.isNull());
    ASSERT_FALSE(codeCompleteResults2.isNull());
}

}
