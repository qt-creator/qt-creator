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

#include <diagnosticset.h>
#include <projectpart.h>
#include <translationunit.h>
#include <unsavedfiles.h>

#include <clang-c/Index.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

using ::ClangBackEnd::DiagnosticSet;
using ::ClangBackEnd::TranslationUnit;
using ::ClangBackEnd::ProjectPart;
using ::ClangBackEnd::UnsavedFiles;

namespace {

class DiagnosticSet : public ::testing::Test
{
protected:
    ProjectPart projectPart{Utf8StringLiteral("projectPartId"), {Utf8StringLiteral("-pedantic")}};
    UnsavedFiles unsavedFiles;
    TranslationUnit translationUnit{Utf8StringLiteral(TESTDATA_DIR"/diagnostic_diagnosticset.cpp"),
                                    unsavedFiles,
                                    projectPart};
};

TEST_F(DiagnosticSet, SetHasContent)
{
    const auto set = translationUnit.diagnostics();

    ASSERT_THAT(set.size(), 1);
}

TEST_F(DiagnosticSet, MoveConstructor)
{
    auto set = translationUnit.diagnostics();

    const auto set2 = std::move(set);

    ASSERT_TRUE(set.isNull());
    ASSERT_FALSE(set2.isNull());
}

TEST_F(DiagnosticSet, MoveAssigment)
{
    auto set = translationUnit.diagnostics();

    auto set2 = std::move(set);
    set = std::move(set2);

    ASSERT_TRUE(set2.isNull());
    ASSERT_FALSE(set.isNull());
}

TEST_F(DiagnosticSet, MoveSelfAssigment)
{
    auto set = translationUnit.diagnostics();

    set = std::move(set);

    ASSERT_FALSE(set.isNull());
}

TEST_F(DiagnosticSet, FirstElementEqualBegin)
{
    auto set = translationUnit.diagnostics();

    ASSERT_TRUE(set.front() == *set.begin());
}

TEST_F(DiagnosticSet, BeginIsUnequalEnd)
{
    auto set = translationUnit.diagnostics();

    ASSERT_TRUE(set.begin() != set.end());
}

TEST_F(DiagnosticSet, BeginPlusOneIsEqualEnd)
{
    auto set = translationUnit.diagnostics();

    ASSERT_TRUE(++set.begin() == set.end());
}

}
