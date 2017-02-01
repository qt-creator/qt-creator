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

#include <mockchangedfilepathcompressor.h>

namespace {

using testing::ElementsAre;
using testing::Invoke;
using testing::IsEmpty;
using testing::NiceMock;

class ChangedFilePathCompressor : public testing::Test
{
protected:
    void SetUp();

protected:
    NiceMock<MockChangedFilePathCompressor> mockCompressor;
    ClangBackEnd::ChangedFilePathCompressor<FakeTimer> compressor;
    QString filePath1{"filePath1"};
    QString filePath2{"filePath2"};
};

TEST_F(ChangedFilePathCompressor, AddFilePath)
{
    mockCompressor.addFilePath(filePath1);

    ASSERT_THAT(mockCompressor.takeFilePaths(), ElementsAre(filePath1));
}

TEST_F(ChangedFilePathCompressor, NoFilePathsAferTakenThem)
{
    mockCompressor.addFilePath(filePath1);

    mockCompressor.takeFilePaths();

    ASSERT_THAT(mockCompressor.takeFilePaths(), IsEmpty());
}

TEST_F(ChangedFilePathCompressor, CallRestartTimerAfterAddingPath)
{
    EXPECT_CALL(mockCompressor, restartTimer());

    mockCompressor.addFilePath(filePath1);
}

TEST_F(ChangedFilePathCompressor, CallTimeOutAfterAddingPath)
{
    EXPECT_CALL(mockCompressor, callbackCalled(ElementsAre(filePath1, filePath2)));

    compressor.addFilePath(filePath1);
    compressor.addFilePath(filePath2);
}

void ChangedFilePathCompressor::SetUp()
{
    compressor.setCallback([&] (Utils::PathStringVector &&filePaths) {
        mockCompressor.callbackCalled(filePaths);
    });
}

}
