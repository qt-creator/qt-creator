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

#include <clangclock.h>
#include <clangtranslationunitupdater.h>

#include <clang-c/Index.h>

using ClangBackEnd::Clock;
using ClangBackEnd::TimePoint;
using ClangBackEnd::TranslationUnitUpdater;
using ClangBackEnd::TranslationUnitUpdateInput;
using ClangBackEnd::TranslationUnitUpdateResult;

using testing::Eq;
using testing::Gt;

namespace {

class TranslationUnitUpdater : public ::testing::Test
{
protected:
    void TearDown() override;

    ::TranslationUnitUpdater createUpdater(const TranslationUnitUpdateInput &input,
                                           const Utf8String &translationUnitId = Utf8String());

    enum ReparseMode { SetReparseNeeded, DoNotSetReparseNeeded };
    TranslationUnitUpdateInput createInput(ReparseMode reparseMode = DoNotSetReparseNeeded);

protected:
    CXIndex cxIndex = nullptr;
    CXTranslationUnit cxTranslationUnit = nullptr;

    Utf8String filePath = Utf8StringLiteral(TESTDATA_DIR"/translationunits.cpp");
};

using TranslationUnitUpdaterSlowTest = TranslationUnitUpdater;

TEST_F(TranslationUnitUpdaterSlowTest, ParsesIfNeeded)
{
    ::TranslationUnitUpdater updater = createUpdater(createInput());

    TranslationUnitUpdateResult result = updater.update(::TranslationUnitUpdater::UpdateMode::AsNeeded);

    ASSERT_TRUE(cxTranslationUnit);
    ASSERT_FALSE(result.hasReparsed());
}

TEST_F(TranslationUnitUpdaterSlowTest, ReparsesIfNeeded)
{
    ::TranslationUnitUpdater updater = createUpdater(createInput(SetReparseNeeded));

    TranslationUnitUpdateResult result = updater.update(::TranslationUnitUpdater::UpdateMode::AsNeeded);

    ASSERT_TRUE(result.hasReparsed());
}

TEST_F(TranslationUnitUpdaterSlowTest, PropagatesTranslationUnitId)
{
    const Utf8String translationUnitId = Utf8StringLiteral("myId");
    ::TranslationUnitUpdater updater = createUpdater(createInput(SetReparseNeeded), translationUnitId);

    TranslationUnitUpdateResult result = updater.update(::TranslationUnitUpdater::UpdateMode::AsNeeded);

    ASSERT_THAT(result.translationUnitId, Eq(translationUnitId));
}

TEST_F(TranslationUnitUpdaterSlowTest, UpdatesParseTimePoint)
{
    ::TranslationUnitUpdater updater = createUpdater(createInput());
    const TimePoint now = Clock::now();

    TranslationUnitUpdateResult result = updater.update(::TranslationUnitUpdater::UpdateMode::AsNeeded);

    ASSERT_TRUE(result.hasParsed());
    ASSERT_THAT(result.parseTimePoint, Gt(now));
}

TEST_F(TranslationUnitUpdaterSlowTest, NotUpdatingParseTimePointForReparseOnly)
{
    ::TranslationUnitUpdater updater = createUpdater(createInput());
    TranslationUnitUpdateResult result = updater.update(::TranslationUnitUpdater::UpdateMode::AsNeeded);

    ::TranslationUnitUpdater reparseUpdater = createUpdater(createInput(SetReparseNeeded));
    result = reparseUpdater.update(::TranslationUnitUpdater::UpdateMode::AsNeeded);

    ASSERT_TRUE(result.hasReparsed());
    ASSERT_FALSE(result.hasParsed());
}

TEST_F(TranslationUnitUpdaterSlowTest, UpdatesDependendOnFilesOnParse)
{
    ::TranslationUnitUpdater updater = createUpdater(createInput());

    TranslationUnitUpdateResult result = updater.update(::TranslationUnitUpdater::UpdateMode::AsNeeded);

    ASSERT_FALSE(result.dependedOnFilePaths.isEmpty());
}

void TranslationUnitUpdater::TearDown()
{
    clang_disposeIndex(cxIndex);
}

::TranslationUnitUpdater
TranslationUnitUpdater::createUpdater(const TranslationUnitUpdateInput &input,
                                      const Utf8String &translationUnitId)
{
    return ::TranslationUnitUpdater(translationUnitId, cxIndex, cxTranslationUnit, input);
}

TranslationUnitUpdateInput
TranslationUnitUpdater::createInput(ReparseMode reparseMode)
{
    TranslationUnitUpdateInput updateInput;
    updateInput.filePath = filePath;
    updateInput.reparseNeeded = reparseMode == SetReparseNeeded;

    return updateInput;
}

} // anonymous
