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

#include "mocktimer.h"

#include <changedfilepathcompressor.h>
#include <filepathcaching.h>
#include <refactoringdatabaseinitializer.h>

namespace {

using testing::ElementsAre;
using testing::Invoke;
using testing::IsEmpty;
using testing::NiceMock;

using ClangBackEnd::FilePath;
using ClangBackEnd::FilePathId;

class ChangedFilePathCompressor : public testing::Test
{
protected:
    void SetUp()
    {
        compressor.setCallback(mockCompressorCallback.AsStdFunction());
    }

    FilePathId filePathId(const QString &filePath)
    {
        Utils::SmallString utf8FilePath{filePath};

        return filePathCache.filePathId(ClangBackEnd::FilePathView{utf8FilePath});
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    NiceMock<MockFunction<void (const ClangBackEnd::FilePathIds &filePathIds)>> mockCompressorCallback;
    ClangBackEnd::ChangedFilePathCompressor<NiceMock<MockTimer>> compressor{filePathCache};
    NiceMock<MockTimer> &mockTimer = compressor.timer();
    QString filePath1{"filePath1"};
    QString filePath2{"filePath2"};
    FilePathId filePathId1 = filePathId(filePath1);
    FilePathId filePathId2 = filePathId(filePath2);
};

TEST_F(ChangedFilePathCompressor, AddFilePath)
{
    compressor.addFilePath(filePath1);

    ASSERT_THAT(compressor.takeFilePathIds(), ElementsAre(filePathId(filePath1)));
}

TEST_F(ChangedFilePathCompressor, NoFilePathsAferTakenThem)
{
    compressor.addFilePath(filePath1);

    compressor.takeFilePathIds();

    ASSERT_THAT(compressor.takeFilePathIds(), IsEmpty());
}

TEST_F(ChangedFilePathCompressor, CallRestartTimerAfterAddingPath)
{
    EXPECT_CALL(mockTimer, start(20));

    compressor.addFilePath(filePath1);
}

TEST_F(ChangedFilePathCompressor, CallTimeOutAfterAddingPath)
{
    EXPECT_CALL(mockCompressorCallback, Call(ElementsAre(filePathId1, filePathId2)));

    compressor.addFilePath(filePath1);
    compressor.addFilePath(filePath2);
}

TEST_F(ChangedFilePathCompressor, RemoveDuplicates)
{
    EXPECT_CALL(mockCompressorCallback, Call(ElementsAre(filePathId1, filePathId2)));

    compressor.addFilePath(filePath1);
    compressor.addFilePath(filePath2);
    compressor.addFilePath(filePath1);
}

}
