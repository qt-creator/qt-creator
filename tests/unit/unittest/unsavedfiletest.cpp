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

#include "gtest/gtest.h"
#include "gmock/gmock-matchers.h"
#include "gmock/gmock.h"
#include "gtest-qt-printing.h"

#include <unsavedfile.h>
#include <unsavedfiles.h>

using ClangBackEnd::UnsavedFile;
using ClangBackEnd::UnsavedFiles;

using ::testing::PrintToString;

namespace {

MATCHER_P3(IsUnsavedFile, fileName, contents, contentsLength,
          std::string(negation ? "isn't" : "is")
          + " file name " + PrintToString(fileName)
          + ", contents " + PrintToString(contents)
          + ", contents length " + PrintToString(contentsLength))
{
    CXUnsavedFile unsavedFile = arg.cxUnsavedFile;

    return fileName == unsavedFile.Filename
        && contents == unsavedFile.Contents
        && size_t(contentsLength) == unsavedFile.Length;
}

class UnsavedFile : public ::testing::Test
{
protected:
    Utf8String filePath = Utf8StringLiteral("path");
    Utf8String fileContent = Utf8StringLiteral("content");

    Utf8String otherFilePath = Utf8StringLiteral("otherpath");
    Utf8String otherFileContent = Utf8StringLiteral("othercontent");

    uint aLength = 2;
    Utf8String aReplacement = Utf8StringLiteral("replacement");
};

TEST_F(UnsavedFile, Create)
{
    ::UnsavedFile unsavedFile(filePath, fileContent);

    ASSERT_THAT(unsavedFile, IsUnsavedFile(filePath,
                                           fileContent,
                                           fileContent.byteSize()));
}

TEST_F(UnsavedFile, Destruct)
{
    auto *unsavedFile = new ::UnsavedFile(filePath, fileContent);
    unsavedFile->~UnsavedFile();

    ASSERT_THAT(*unsavedFile, IsUnsavedFile(nullptr, nullptr, 0UL));
}

TEST_F(UnsavedFile, ConstructMoveToIsValid)
{
    ::UnsavedFile movedFrom(filePath, fileContent);

    ::UnsavedFile movedTo = std::move(movedFrom);

    ASSERT_THAT(movedTo, IsUnsavedFile(filePath,
                                       fileContent,
                                       fileContent.byteSize()));
}

TEST_F(UnsavedFile, ConstructMoveFromIsNull)
{
    ::UnsavedFile movedFrom(filePath, fileContent);

    ::UnsavedFile movedTo = std::move(movedFrom);

    ASSERT_THAT(movedFrom, IsUnsavedFile(nullptr, nullptr, 0UL));
}

TEST_F(UnsavedFile, AssignMoveToIsValid)
{
    ::UnsavedFile movedFrom(filePath, fileContent);
    ::UnsavedFile movedTo(otherFilePath, otherFileContent);

    movedTo = std::move(movedFrom);

    ASSERT_THAT(movedTo, IsUnsavedFile(filePath,
                                       fileContent,
                                       fileContent.byteSize()));
}

TEST_F(UnsavedFile, AssignMoveFromIsSwapped)
{
    ::UnsavedFile movedFrom(filePath, fileContent);
    ::UnsavedFile movedTo(otherFilePath, otherFileContent);

    movedTo = std::move(movedFrom);

    ASSERT_THAT(movedFrom, IsUnsavedFile(otherFilePath,
                                         otherFileContent,
                                         otherFileContent.byteSize()));
}

TEST_F(UnsavedFile, DoNotReplaceWithOffsetZeroInEmptyContent)
{
    ::UnsavedFile unsavedFile(filePath, QStringLiteral(""));

    ASSERT_FALSE(unsavedFile.replaceAt(0, aLength, aReplacement));
}

TEST_F(UnsavedFile, DoNotReplaceWithOffsetOneInEmptyContent)
{
    ::UnsavedFile unsavedFile(filePath, QStringLiteral(""));

    ASSERT_FALSE(unsavedFile.replaceAt(1, aLength, aReplacement));
}

TEST_F(UnsavedFile, Replace)
{
    const Utf8String expectedContent = Utf8StringLiteral("hello replacement!");
    ::UnsavedFile unsavedFile(filePath, Utf8StringLiteral("hello world!"));

    const bool hasReplaced = unsavedFile.replaceAt(6, 5, aReplacement);

    ASSERT_TRUE(hasReplaced);
    ASSERT_THAT(unsavedFile, IsUnsavedFile(filePath, expectedContent, expectedContent.byteSize()));
}

TEST_F(UnsavedFile, HasNoCharacterForTooBigOffset)
{
    ::UnsavedFile unsavedFile(filePath, fileContent);

    ASSERT_FALSE(unsavedFile.hasCharacterAt(100, 'x'));
}

TEST_F(UnsavedFile, HasNoCharacterForWrongOffset)
{
    ::UnsavedFile unsavedFile(filePath, fileContent);

    ASSERT_FALSE(unsavedFile.hasCharacterAt(0, 'x'));
}

TEST_F(UnsavedFile, HasCharacterForCorrectOffset)
{
    ::UnsavedFile unsavedFile(filePath, fileContent);

    ASSERT_TRUE(unsavedFile.hasCharacterAt(0, 'c'));
}

TEST_F(UnsavedFile, HasCharacterForLastLineColumn)
{
    ::UnsavedFile unsavedFile(filePath, fileContent);

    ASSERT_TRUE(unsavedFile.hasCharacterAt(1, 7, 't'));
}

} // anonymous namespace
