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

#include <filecontainer.h>
#include <unsavedfiles.h>
#include <unsavedfile.h>

#include <QVector>

using ClangBackEnd::UnsavedFile;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::FileContainer;

using ::testing::Eq;
using ::testing::Gt;
using ::testing::IsNull;
using ::testing::Ne;
using ::testing::NotNull;
using ::testing::PrintToString;

namespace {

bool operator==(const ClangBackEnd::FileContainer &fileContainer, const UnsavedFile &unsavedFile)
{
    return fileContainer.filePath() == unsavedFile.filePath()
        && fileContainer.unsavedFileContent() == unsavedFile.fileContent();
}

bool fileContainersContainsItemMatchingToCxUnsavedFile(const QVector<FileContainer> &fileContainers, const UnsavedFile &unsavedFile)
{
    for (const FileContainer &fileContainer : fileContainers) {
        if (fileContainer == unsavedFile)
            return true;
    }

    return false;
}

MATCHER_P(HasUnsavedFiles, fileContainers, "")
{
    ClangBackEnd::UnsavedFiles unsavedFiles = arg;
    if (unsavedFiles.count() != uint(fileContainers.size())) {
        *result_listener << "unsaved count is " << unsavedFiles.count() << " and not " << fileContainers.size();
        return false;
    }

    for (uint i = 0, to = unsavedFiles.count(); i < to; ++i) {
        UnsavedFile unsavedFile = unsavedFiles.at(i);
        if (!fileContainersContainsItemMatchingToCxUnsavedFile(fileContainers, unsavedFile))
            return false;
    }

    return true;
}

MATCHER_P3(IsUnsavedFile, fileName, contents, contentsLength,
          std::string(negation ? "isn't" : "is")
          + " file name " + PrintToString(fileName)
          + ", contents " + PrintToString(contents)
          + ", contents length " + PrintToString(contentsLength))
{
    return fileName == arg.filePath()
        && contents == arg.fileContent()
        && int(contentsLength) == arg.fileContent().byteSize();
}

class UnsavedFiles : public ::testing::Test
{
protected:
    ::UnsavedFiles unsavedFiles;
    Utf8String filePath{Utf8StringLiteral("file.cpp")};
    Utf8String projectPartId{Utf8StringLiteral("projectPartId")};

    Utf8String unsavedContent1{Utf8StringLiteral("foo")};
    Utf8String unsavedContent2{Utf8StringLiteral("bar")};
};

TEST_F(UnsavedFiles, ModifiedCopyIsDifferent)
{
    QVector<FileContainer> fileContainers({FileContainer(filePath, projectPartId, unsavedContent1, true)});
    unsavedFiles.createOrUpdate(fileContainers);

    ::UnsavedFiles copy = unsavedFiles;
    QVector<FileContainer> updatedFileContainers({FileContainer(filePath, projectPartId, unsavedContent2, true)});
    copy.createOrUpdate(updatedFileContainers);

    ASSERT_THAT(copy.at(0).fileContent(), Ne(unsavedFiles.at(0).fileContent()));
    ASSERT_THAT(copy.lastChangeTimePoint(), Gt(unsavedFiles.lastChangeTimePoint()));
}

TEST_F(UnsavedFiles, DoNothingForUpdateIfFileHasNoUnsavedContent)
{
    QVector<FileContainer> fileContainers({FileContainer(filePath, projectPartId)});

    unsavedFiles.createOrUpdate(fileContainers);

    ASSERT_THAT(unsavedFiles, HasUnsavedFiles(QVector<FileContainer>()));
}

TEST_F(UnsavedFiles, AddUnsavedFileForUpdateWithUnsavedContent)
{
    QVector<FileContainer> fileContainers({FileContainer(filePath, projectPartId),
                                           FileContainer(filePath, projectPartId, unsavedContent1, true)});
    unsavedFiles.createOrUpdate(fileContainers);

    ASSERT_THAT(unsavedFiles, HasUnsavedFiles(QVector<FileContainer>({FileContainer(filePath, projectPartId, unsavedContent1, true)})));
}

TEST_F(UnsavedFiles, RemoveUnsavedFileForUpdateWithUnsavedContent)
{
    QVector<FileContainer> fileContainers({FileContainer(filePath, projectPartId, unsavedContent1, true),
                                           FileContainer(filePath, projectPartId)});

    unsavedFiles.createOrUpdate(fileContainers);

    ASSERT_THAT(unsavedFiles, HasUnsavedFiles(QVector<FileContainer>()));
}

TEST_F(UnsavedFiles, ExchangeUnsavedFileForUpdateWithUnsavedContent)
{
    QVector<FileContainer> fileContainers({FileContainer(filePath, projectPartId, unsavedContent1, true),
                                           FileContainer(filePath, projectPartId, unsavedContent2, true)});
    unsavedFiles.createOrUpdate(fileContainers);

    ASSERT_THAT(unsavedFiles, HasUnsavedFiles(QVector<FileContainer>({FileContainer(filePath, projectPartId, unsavedContent2, true)})));
}

TEST_F(UnsavedFiles, TimeStampIsUpdatedAsUnsavedFilesChanged)
{
    QVector<FileContainer> fileContainers({FileContainer(filePath, projectPartId, unsavedContent1, true),
                                           FileContainer(filePath, projectPartId, unsavedContent2, true)});
    auto lastChangeTimePoint = unsavedFiles.lastChangeTimePoint();

    unsavedFiles.createOrUpdate(fileContainers);

    ASSERT_THAT(unsavedFiles.lastChangeTimePoint(), Gt(lastChangeTimePoint));
}

TEST_F(UnsavedFiles, RemoveUnsavedFiles)
{
    QVector<FileContainer> fileContainers({FileContainer(filePath, projectPartId, unsavedContent1, true)});
    unsavedFiles.createOrUpdate(fileContainers);

    unsavedFiles.remove(fileContainers);

    ASSERT_THAT(unsavedFiles, HasUnsavedFiles(QVector<FileContainer>()));
}

TEST_F(UnsavedFiles, FindUnsavedFile)
{
    QVector<FileContainer> fileContainers({FileContainer(filePath, projectPartId, unsavedContent1, true)});
    unsavedFiles.createOrUpdate(fileContainers);

    UnsavedFile &unsavedFile = unsavedFiles.unsavedFile(filePath);

    ASSERT_THAT(unsavedFile, IsUnsavedFile(filePath, unsavedContent1, unsavedContent1.byteSize()));
}

TEST_F(UnsavedFiles, FindNoUnsavedFile)
{
    QVector<FileContainer> fileContainers({FileContainer(filePath, projectPartId, unsavedContent1, true)});
    unsavedFiles.createOrUpdate(fileContainers);

    UnsavedFile &unsavedFile = unsavedFiles.unsavedFile(Utf8StringLiteral("nonExistingFilePath.cpp"));

    ASSERT_THAT(unsavedFile, IsUnsavedFile(Utf8String(), Utf8String(), 0UL));
}

}
