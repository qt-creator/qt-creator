// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/filesystemmock.h"
#include "../mocks/mocktimer.h"

#include <projectstorage/directorypathcompressor.h>

#include <exception>

namespace {

using QmlDesigner::DirectoryPathId;
using QmlDesigner::DirectoryPathIds;

class DirectoryPathCompressor : public testing::Test
{
protected:
    void SetUp() { compressor.setCallback(mockCompressorCallback.AsStdFunction()); }

protected:
    NiceMock<MockFunction<void(const DirectoryPathIds &directoryPathIds)>> mockCompressorCallback;
    QmlDesigner::DirectoryPathCompressor<NiceMock<MockTimer>> compressor;
    NiceMock<MockTimer> &mockTimer = compressor.timer();
    DirectoryPathId directoryPathId1{DirectoryPathId::create(1)};
    DirectoryPathId directoryPathId2{DirectoryPathId::create(2)};
};

TEST_F(DirectoryPathCompressor, add_file_path)
{
    compressor.addDirectoryPathId(directoryPathId1);

    ASSERT_THAT(compressor.directoryPathIds(), ElementsAre(directoryPathId1));
}

TEST_F(DirectoryPathCompressor, clear__after_calling_callback)
{
    compressor.addDirectoryPathId(directoryPathId1);

    compressor.timer().emitTimoutIfStarted();

    ASSERT_THAT(compressor.directoryPathIds(), IsEmpty());
}

TEST_F(DirectoryPathCompressor, dont_clear_for_thrown_exception)
{
    compressor.addDirectoryPathId(directoryPathId1);
    compressor.setCallback([](const DirectoryPathIds &) { throw std::exception{}; });

    compressor.timer().emitTimoutIfStarted();

    ASSERT_THAT(compressor.directoryPathIds(), ElementsAre(directoryPathId1));
}

TEST_F(DirectoryPathCompressor, call_restart_timer_after_adding_path)
{
    EXPECT_CALL(mockTimer, start(20));

    compressor.addDirectoryPathId(directoryPathId1);
}

TEST_F(DirectoryPathCompressor, call_time_out_after_adding_path)
{
    EXPECT_CALL(mockCompressorCallback, Call(ElementsAre(directoryPathId1, directoryPathId2)));

    compressor.addDirectoryPathId(directoryPathId1);
    compressor.addDirectoryPathId(directoryPathId2);
}

TEST_F(DirectoryPathCompressor, remove_duplicates)
{
    EXPECT_CALL(mockCompressorCallback, Call(ElementsAre(directoryPathId1, directoryPathId2)));

    compressor.addDirectoryPathId(directoryPathId1);
    compressor.addDirectoryPathId(directoryPathId2);
    compressor.addDirectoryPathId(directoryPathId1);
}

} // namespace
