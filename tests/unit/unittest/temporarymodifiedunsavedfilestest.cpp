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

#include <filecontainer.h>
#include <temporarymodifiedunsavedfiles.h>
#include <unsavedfiles.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

using ClangBackEnd::FileContainer;
using ClangBackEnd::TemporaryModifiedUnsavedFiles;
using ClangBackEnd::UnsavedFiles;

using testing::Eq;

namespace {

class TemporaryModifiedUnsavedFiles : public ::testing::Test
{
protected:
    ClangBackEnd::UnsavedFiles unsavedFiles;

    FileContainer fileContainer{Utf8StringLiteral("file1.h"),
                                Utf8StringLiteral("projectPartId"),
                                Utf8StringLiteral("void f() { foo. }"),
                                true};

    Utf8String someNotExistingFilePath{Utf8StringLiteral("nonExistingFile.cpp")};
};

TEST_F(TemporaryModifiedUnsavedFiles, HasZeroFilesOnCreation)
{
    ::TemporaryModifiedUnsavedFiles files(unsavedFiles.cxUnsavedFileVector());

    ASSERT_THAT(files.count(), Eq(0L));
}

TEST_F(TemporaryModifiedUnsavedFiles, HasOneFileAfterCreatingOne)
{
    unsavedFiles.createOrUpdate({fileContainer});

    ::TemporaryModifiedUnsavedFiles files(unsavedFiles.cxUnsavedFileVector());

    ASSERT_THAT(files.count(), Eq(1L));
}

TEST_F(TemporaryModifiedUnsavedFiles, ReplaceIndicatesFailureOnNonExistingUnsavedFile)
{
    ::TemporaryModifiedUnsavedFiles files(unsavedFiles.cxUnsavedFileVector());
    const uint someOffset = 0;
    const uint someLength = 1;
    const auto someReplacement = Utf8StringLiteral("->");

    const bool replaced = files.replaceInFile(someNotExistingFilePath,
                                              someOffset,
                                              someLength,
                                              someReplacement);

    ASSERT_FALSE(replaced);
}

TEST_F(TemporaryModifiedUnsavedFiles, ReplaceDotWithArrow)
{
    unsavedFiles.createOrUpdate({fileContainer});
    ::TemporaryModifiedUnsavedFiles files(unsavedFiles.cxUnsavedFileVector());
    const uint offset = 14;
    const uint length = 1;
    const auto replacement = Utf8StringLiteral("->");

    const bool replacedSuccessfully = files.replaceInFile(fileContainer.filePath(),
                                                          offset,
                                                          length,
                                                          replacement);

    CXUnsavedFile cxUnsavedFile = files.cxUnsavedFileAt(0);
    ASSERT_TRUE(replacedSuccessfully);
    ASSERT_THAT(Utf8String::fromUtf8(cxUnsavedFile.Contents),
                Eq(Utf8StringLiteral("void f() { foo-> }")));
}

} // anonymous
