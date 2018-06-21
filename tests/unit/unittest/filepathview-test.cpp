/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <filepathview.h>

namespace {

using ClangBackEnd::FilePathView;

TEST(FilePathView, FilePathSlashForEmptyPath)
{
    FilePathView filePath("");

    ASSERT_THAT(filePath.slashIndex(), -1);
}

TEST(FilePathView, FilePathSlashForSingleSlash)
{
    FilePathView filePath("/");

    ASSERT_THAT(filePath.slashIndex(), 0);
}

TEST(FilePathView, FilePathSlashForFileInRoot)
{
    FilePathView filePath("/file.h");

    ASSERT_THAT(filePath.slashIndex(), 0);
}

TEST(FilePathView, FilePathSlashForSomeLongerPath)
{
    FilePathView filePath("/path/to/some/file.h");

    ASSERT_THAT(filePath.slashIndex(), 13);
}

TEST(FilePathView, FilePathSlashForFileNameOnly)
{
    FilePathView filePath("file.h");

    ASSERT_THAT(filePath.slashIndex(), -1);
}

TEST(FilePathView, DirectoryPathForEmptyPath)
{
    FilePathView filePath("");

    ASSERT_THAT(filePath.directory(), "");
}

TEST(FilePathView, DirectoryPathForSingleSlashPath)
{
    FilePathView filePath{"/"};

    ASSERT_THAT(filePath.directory(), "");
}

TEST(FilePathView, DirectoryPathForLongerPath)
{
    FilePathView filePath{"/path/to/some/file.h"};

    ASSERT_THAT(filePath.directory(), "/path/to/some");
}

TEST(FilePathView, DirectoryPathForFileNameOnly)
{
    FilePathView filePath{"file.h"};

    ASSERT_THAT(filePath.directory(), IsEmpty());
}

TEST(FilePathView, FileNameForEmptyPath)
{
    FilePathView filePath("");

    ASSERT_THAT(filePath.name(), "");
}

TEST(FilePathView, FileNameForSingleSlashPath)
{
    FilePathView filePath{"/"};

    ASSERT_THAT(filePath.name(), "");
}

TEST(FilePathView, FileNameForLongerPath)
{
    FilePathView filePath{"/path/to/some/file.h"};

    ASSERT_THAT(filePath.name(), "file.h");
}

TEST(FilePathView, FileNameForFileNameOnly)
{
    FilePathView filePath{"file.h"};

    ASSERT_THAT(filePath.name(), "file.h");
}

}
