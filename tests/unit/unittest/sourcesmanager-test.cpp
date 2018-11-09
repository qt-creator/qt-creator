/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <sourcesmanager.h>

namespace {

class SourcesManager : public testing::Test
{
protected:
    ClangBackEnd::SourcesManager sources;
};

TEST_F(SourcesManager, TouchFilePathIdFirstTime)
{
    ASSERT_FALSE(sources.alreadyParsed(1, 56));
}

TEST_F(SourcesManager, TouchFilePathIdTwoTimesWithSameTime)
{
    sources.alreadyParsed(1, 56);

    ASSERT_FALSE(sources.alreadyParsed(1, 56));
}

TEST_F(SourcesManager, TouchFilePathIdSecondTimeWithSameTime)
{
    sources.alreadyParsed(1, 56);

    sources.updateModifiedTimeStamps();

    ASSERT_TRUE(sources.alreadyParsed(1, 56));
}

TEST_F(SourcesManager, TouchFilePathIdSecondTimeWithOlderTime)
{
    sources.alreadyParsed(1, 56);

    sources.updateModifiedTimeStamps();

    ASSERT_TRUE(sources.alreadyParsed(1, 55));
}

TEST_F(SourcesManager, TouchFilePathIdSecondTimeWithNewerTime)
{
    sources.alreadyParsed(1, 56);

    sources.updateModifiedTimeStamps();

    ASSERT_FALSE(sources.alreadyParsed(1, 57));
}

TEST_F(SourcesManager, MultipleFileIds)
{
    sources.alreadyParsed(1, 455);
    sources.alreadyParsed(4, 56);
    sources.alreadyParsed(3, 85);
    sources.alreadyParsed(6, 56);
    sources.alreadyParsed(2, 45);

    sources.updateModifiedTimeStamps();

    ASSERT_TRUE(sources.alreadyParsed(3, 85));
}

TEST_F(SourcesManager, UpdateModifiedTimeStampsWithNewerTimeStamp)
{
    sources.alreadyParsed(1, 455);
    sources.alreadyParsed(4, 56);
    sources.alreadyParsed(3, 85);
    sources.alreadyParsed(6, 56);
    sources.alreadyParsed(2, 45);
    sources.updateModifiedTimeStamps();

    sources.alreadyParsed(3, 86);
    sources.updateModifiedTimeStamps();

    ASSERT_TRUE(sources.alreadyParsed(3, 86));
}

TEST_F(SourcesManager, DontUpdateModifiedTimeStampsWithOlderTimeStamp)
{
    sources.alreadyParsed(1, 455);
    sources.alreadyParsed(4, 56);
    sources.alreadyParsed(3, 85);
    sources.alreadyParsed(6, 56);
    sources.alreadyParsed(2, 45);
    sources.updateModifiedTimeStamps();

    sources.alreadyParsed(3, 84);
    sources.updateModifiedTimeStamps();

    ASSERT_TRUE(sources.alreadyParsed(3, 85));
}

TEST_F(SourcesManager, ZeroTime)
{
    sources.alreadyParsed(1, 0);

    sources.updateModifiedTimeStamps();

    ASSERT_TRUE(sources.alreadyParsed(1, 0));
}

TEST_F(SourcesManager, TimeIsUpdated)
{
    sources.alreadyParsed(1, 56);
    sources.alreadyParsed(1, 57);

    sources.updateModifiedTimeStamps();

    ASSERT_TRUE(sources.alreadyParsed(1, 57));
}

TEST_F(SourcesManager, AnyDependFileIsNotModifiedAfterInitialization)
{
    ASSERT_FALSE(sources.dependentFilesModified());
}

TEST_F(SourcesManager, AnyDependFileIsModified)
{
    sources.alreadyParsed(1, 56);

    ASSERT_TRUE(sources.dependentFilesModified());
}

TEST_F(SourcesManager, AnyDependFileIsModifiedAfterParsingTwoTimesSameTimeStamp)
{
    sources.alreadyParsed(1, 56);
    sources.alreadyParsed(1, 56);

    ASSERT_TRUE(sources.dependentFilesModified());
}

TEST_F(SourcesManager, AnyDependFileIsNotModifiedAfterUpdate)
{
    sources.alreadyParsed(1, 56);
    sources.updateModifiedTimeStamps();

    ASSERT_FALSE(sources.dependentFilesModified());
}

TEST_F(SourcesManager, AnyDependFileIsNotModifiedAfterNotAlreadyPared)
{
    sources.alreadyParsed(1, 56);
    sources.updateModifiedTimeStamps();

    sources.alreadyParsed(1, 56);

    ASSERT_FALSE(sources.dependentFilesModified());
}

TEST_F(SourcesManager, AnyDependFileIsNotModifiedAfterAlreadyPared)
{
    sources.alreadyParsed(1, 56);
    sources.updateModifiedTimeStamps();

    sources.alreadyParsed(1, 57);

    ASSERT_TRUE(sources.dependentFilesModified());
}

TEST_F(SourcesManager, AnyDependFileIsModifiedAfterUpdateNewTimeStamp)
{
    sources.alreadyParsed(1, 56);
    sources.alreadyParsed(2, 56);
    sources.updateModifiedTimeStamps();
    sources.alreadyParsed(1, 57);

    sources.alreadyParsed(2, 56);

    ASSERT_TRUE(sources.dependentFilesModified());
}

TEST_F(SourcesManager, AlreadyParsedWithDependencyAfterUpdateNewTimeStamp)
{
    sources.alreadyParsedAllDependentFiles(1, 56);
    sources.alreadyParsedAllDependentFiles(2, 56);
    sources.updateModifiedTimeStamps();
    sources.alreadyParsedAllDependentFiles(1, 57);

    bool alreadyParsed = sources.alreadyParsedAllDependentFiles(2, 56);

    ASSERT_FALSE(alreadyParsed);
}

TEST_F(SourcesManager, AlreadyParsedWithDependencyAfterUpdateNewSecondTimeStamp)
{
    sources.alreadyParsedAllDependentFiles(1, 56);
    sources.alreadyParsedAllDependentFiles(2, 56);
    sources.updateModifiedTimeStamps();
    sources.alreadyParsedAllDependentFiles(1, 56);

    bool alreadyParsed = sources.alreadyParsedAllDependentFiles(2, 57);

    ASSERT_FALSE(alreadyParsed);
}

TEST_F(SourcesManager, AlreadyParsedWithDependencyAfterUpdateSameTimeStamps)
{
    sources.alreadyParsedAllDependentFiles(1, 56);
    sources.alreadyParsedAllDependentFiles(2, 56);
    sources.updateModifiedTimeStamps();
    sources.alreadyParsedAllDependentFiles(1, 56);

    bool alreadyParsed = sources.alreadyParsedAllDependentFiles(2, 56);

    ASSERT_TRUE(alreadyParsed);
}

}
