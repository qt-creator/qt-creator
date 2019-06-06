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

#include "mockfilesystem.h"
#include "mocktimer.h"

#include <directorypathcompressor.h>

namespace {

using testing::ElementsAre;
using testing::Invoke;
using testing::IsEmpty;
using testing::NiceMock;

using ClangBackEnd::DirectoryPathId;
using ClangBackEnd::DirectoryPathIds;

class DirectoryPathCompressor : public testing::Test
{
protected:
    void SetUp()
    {
        compressor.setCallback(mockCompressorCallback.AsStdFunction());
    }

protected:
    NiceMock<MockFunction<void(const DirectoryPathIds &directoryPathIds)>> mockCompressorCallback;
    ClangBackEnd::DirectoryPathCompressor<NiceMock<MockTimer>> compressor;
    NiceMock<MockTimer> &mockTimer = compressor.timer();
    DirectoryPathId directoryPathId1{1};
    DirectoryPathId directoryPathId2{2};
};

TEST_F(DirectoryPathCompressor, AddFilePath)
{
    compressor.addDirectoryPathId(directoryPathId1);

    ASSERT_THAT(compressor.takeDirectoryPathIds(), ElementsAre(directoryPathId1));
}

TEST_F(DirectoryPathCompressor, NoFilePathsAferTakenThem)
{
    compressor.addDirectoryPathId(directoryPathId1);

    compressor.takeDirectoryPathIds();

    ASSERT_THAT(compressor.takeDirectoryPathIds(), IsEmpty());
}

TEST_F(DirectoryPathCompressor, CallRestartTimerAfterAddingPath)
{
    EXPECT_CALL(mockTimer, start(20));

    compressor.addDirectoryPathId(directoryPathId1);
}

TEST_F(DirectoryPathCompressor, CallTimeOutAfterAddingPath)
{
    EXPECT_CALL(mockCompressorCallback, Call(ElementsAre(directoryPathId1, directoryPathId2)));

    compressor.addDirectoryPathId(directoryPathId1);
    compressor.addDirectoryPathId(directoryPathId2);
}

TEST_F(DirectoryPathCompressor, RemoveDuplicates)
{
    EXPECT_CALL(mockCompressorCallback, Call(ElementsAre(directoryPathId1, directoryPathId2)));

    compressor.addDirectoryPathId(directoryPathId1);
    compressor.addDirectoryPathId(directoryPathId2);
    compressor.addDirectoryPathId(directoryPathId1);
}

}
