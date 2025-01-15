// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/filesystemmock.h"
#include "../mocks/mocktimer.h"

#include <projectstorage/directorypathcompressor.h>

#include <exception>

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

    ASSERT_THAT(compressor.sourceContextIds(), ElementsAre(sourceContextId1));
}

TEST_F(DirectoryPathCompressor, clear__after_calling_callback)
{
    compressor.addSourceContextId(sourceContextId1);

    compressor.timer().emitTimoutIfStarted();

    ASSERT_THAT(compressor.sourceContextIds(), IsEmpty());
}

TEST_F(DirectoryPathCompressor, dont_clear_for_thrown_exception)
{
    compressor.addSourceContextId(sourceContextId1);
    compressor.setCallback([](const SourceContextIds &) { throw std::exception{}; });

    compressor.timer().emitTimoutIfStarted();

    ASSERT_THAT(compressor.sourceContextIds(), ElementsAre(sourceContextId1));
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
