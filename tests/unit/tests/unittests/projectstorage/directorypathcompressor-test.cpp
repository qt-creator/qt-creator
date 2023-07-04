// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/filesystemmock.h"
#include "../mocks/mocktimer.h"

#include <projectstorage/directorypathcompressor.h>

namespace {

using QmlDesigner::SourceContextId;
using QmlDesigner::SourceContextIds;

class DirectoryPathCompressor : public testing::Test
{
protected:
    void SetUp() { compressor.setCallback(mockCompressorCallback.AsStdFunction()); }

protected:
    NiceMock<MockFunction<void(const SourceContextIds &sourceContextIds)>> mockCompressorCallback;
    QmlDesigner::DirectoryPathCompressor<NiceMock<MockTimer>> compressor;
    NiceMock<MockTimer> &mockTimer = compressor.timer();
    SourceContextId sourceContextId1{SourceContextId::create(1)};
    SourceContextId sourceContextId2{SourceContextId::create(2)};
};

TEST_F(DirectoryPathCompressor, add_file_path)
{
    compressor.addSourceContextId(sourceContextId1);

    ASSERT_THAT(compressor.takeSourceContextIds(), ElementsAre(sourceContextId1));
}

TEST_F(DirectoryPathCompressor, no_file_paths_afer_taken_them)
{
    compressor.addSourceContextId(sourceContextId1);

    compressor.takeSourceContextIds();

    ASSERT_THAT(compressor.takeSourceContextIds(), IsEmpty());
}

TEST_F(DirectoryPathCompressor, call_restart_timer_after_adding_path)
{
    EXPECT_CALL(mockTimer, start(20));

    compressor.addSourceContextId(sourceContextId1);
}

TEST_F(DirectoryPathCompressor, call_time_out_after_adding_path)
{
    EXPECT_CALL(mockCompressorCallback, Call(ElementsAre(sourceContextId1, sourceContextId2)));

    compressor.addSourceContextId(sourceContextId1);
    compressor.addSourceContextId(sourceContextId2);
}

TEST_F(DirectoryPathCompressor, remove_duplicates)
{
    EXPECT_CALL(mockCompressorCallback, Call(ElementsAre(sourceContextId1, sourceContextId2)));

    compressor.addSourceContextId(sourceContextId1);
    compressor.addSourceContextId(sourceContextId2);
    compressor.addSourceContextId(sourceContextId1);
}

} // namespace
