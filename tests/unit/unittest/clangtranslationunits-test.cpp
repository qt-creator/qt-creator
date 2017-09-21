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

#include <clangbackend_global.h>
#include <clangexceptions.h>
#include <clangtranslationunit.h>
#include <clangtranslationunits.h>
#include <utf8string.h>

#include <clang-c/Index.h>

using ClangBackEnd::Clock;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::TranslationUnits;
using ClangBackEnd::TranslationUnitDoesNotExist;
using ClangBackEnd::PreferredTranslationUnit;

using testing::Eq;

namespace {

class TranslationUnits : public ::testing::Test
{
protected:
    Utf8String someFilePath = Utf8StringLiteral("someFilePath");
    ClangBackEnd::TranslationUnits translationUnits{someFilePath};
};

TEST_F(TranslationUnits, CreatedUnitIsNull)
{
    TranslationUnit translationUnit = translationUnits.createAndAppend();

    ASSERT_TRUE(translationUnit.isNull());
}

TEST_F(TranslationUnits, GetThrowsForNotExisting)
{
    ASSERT_THROW(translationUnits.get(), TranslationUnitDoesNotExist);
}

TEST_F(TranslationUnits, GetForSingleTranslationUnit)
{
    const TranslationUnit created = translationUnits.createAndAppend();

    const TranslationUnit queried = translationUnits.get();

    ASSERT_THAT(queried.id(), Eq(created.id()));
}

TEST_F(TranslationUnits, GetFirstForMultipleTranslationUnits)
{
    const TranslationUnit created1 = translationUnits.createAndAppend();
    translationUnits.createAndAppend();

    const TranslationUnit queried = translationUnits.get();

    ASSERT_THAT(queried.id(), Eq(created1.id()));
}

TEST_F(TranslationUnits, GetFirstForMultipleTranslationUnitsAndOnlyFirstParsed)
{
    const TranslationUnit created1 = translationUnits.createAndAppend();
    translationUnits.updateParseTimePoint(created1.id(), Clock::now());
    translationUnits.createAndAppend();

    const TranslationUnit queried = translationUnits.get();

    ASSERT_THAT(queried.id(), Eq(created1.id()));
}

TEST_F(TranslationUnits, GetFirstForMultipleTranslationUnitsAndOnlySecondParsed)
{
    const TranslationUnit created1 = translationUnits.createAndAppend();
    const TranslationUnit created2 = translationUnits.createAndAppend();
    translationUnits.updateParseTimePoint(created2.id(), Clock::now());

    const TranslationUnit queried = translationUnits.get();

    ASSERT_THAT(queried.id(), Eq(created1.id()));
}

TEST_F(TranslationUnits, GetLastUnitializedForMultipleTranslationUnits)
{
    const TranslationUnit created1 = translationUnits.createAndAppend();
    translationUnits.updateParseTimePoint(created1.id(), Clock::now());
    const TranslationUnit created2 = translationUnits.createAndAppend();

    const TranslationUnit queried = translationUnits.get(PreferredTranslationUnit::LastUninitialized);

    ASSERT_THAT(queried.id(), Eq(created2.id()));
}

TEST_F(TranslationUnits, GetRecentForMultipleTranslationUnits)
{
    const TranslationUnit created1 = translationUnits.createAndAppend();
    translationUnits.updateParseTimePoint(created1.id(), Clock::now());
    const TranslationUnit created2 = translationUnits.createAndAppend();
    translationUnits.updateParseTimePoint(created2.id(), Clock::now());

    const TranslationUnit queried = translationUnits.get(PreferredTranslationUnit::RecentlyParsed);

    ASSERT_THAT(queried.id(), Eq(created2.id()));
}

TEST_F(TranslationUnits, GetPreviousForMultipleTranslationUnits)
{
    const TranslationUnit created1 = translationUnits.createAndAppend();
    translationUnits.updateParseTimePoint(created1.id(), Clock::now());
    const TranslationUnit created2 = translationUnits.createAndAppend();
    translationUnits.updateParseTimePoint(created2.id(), Clock::now());

    const TranslationUnit queried = translationUnits.get(PreferredTranslationUnit::PreviouslyParsed);

    ASSERT_THAT(queried.id(), Eq(created1.id()));
}

TEST_F(TranslationUnits, UpdateThrowsForNotExisting)
{
    ClangBackEnd::TranslationUnits otherTranslationUnits{someFilePath};
    const TranslationUnit translationUnit = otherTranslationUnits.createAndAppend();

    ASSERT_THROW(translationUnits.updateParseTimePoint(translationUnit.id(), Clock::now()),
                 TranslationUnitDoesNotExist);
}

} // anonymous namespace
