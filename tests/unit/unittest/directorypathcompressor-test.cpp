/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "filesystemmock.h"
#include "mocktimer.h"

#include <projectstorage/directorypathcompressor.h>

namespace {

using QmlDesigner::SourceContextId;
using QmlDesigner::SourceContextIds;

class DirectoryPathCompressor : public testing::Test
{
protected:
    void SetUp()
    {
        compressor.setCallback(mockCompressorCallback.AsStdFunction());
    }

protected:
    NiceMock<MockFunction<void(const SourceContextIds &sourceContextIds)>> mockCompressorCallback;
    QmlDesigner::DirectoryPathCompressor<NiceMock<MockTimer>> compressor;
    NiceMock<MockTimer> &mockTimer = compressor.timer();
    SourceContextId sourceContextId1{1};
    SourceContextId sourceContextId2{2};
};

TEST_F(DirectoryPathCompressor, AddFilePath)
{
    compressor.addSourceContextId(sourceContextId1);

    ASSERT_THAT(compressor.takeSourceContextIds(), ElementsAre(sourceContextId1));
}

TEST_F(DirectoryPathCompressor, NoFilePathsAferTakenThem)
{
    compressor.addSourceContextId(sourceContextId1);

    compressor.takeSourceContextIds();

    ASSERT_THAT(compressor.takeSourceContextIds(), IsEmpty());
}

TEST_F(DirectoryPathCompressor, CallRestartTimerAfterAddingPath)
{
    EXPECT_CALL(mockTimer, start(20));

    compressor.addSourceContextId(sourceContextId1);
}

TEST_F(DirectoryPathCompressor, CallTimeOutAfterAddingPath)
{
    EXPECT_CALL(mockCompressorCallback, Call(ElementsAre(sourceContextId1, sourceContextId2)));

    compressor.addSourceContextId(sourceContextId1);
    compressor.addSourceContextId(sourceContextId2);
}

TEST_F(DirectoryPathCompressor, RemoveDuplicates)
{
    EXPECT_CALL(mockCompressorCallback, Call(ElementsAre(sourceContextId1, sourceContextId2)));

    compressor.addSourceContextId(sourceContextId1);
    compressor.addSourceContextId(sourceContextId2);
    compressor.addSourceContextId(sourceContextId1);
}

}
