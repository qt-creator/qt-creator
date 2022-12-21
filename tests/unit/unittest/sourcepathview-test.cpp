// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "googletest.h"

#include <projectstorage/sourcepathview.h>

namespace {

TEST(SourcePathView, FilePathSlashForEmptyPath)
{
    QmlDesigner::SourcePathView filePath("");

    ASSERT_THAT(filePath.slashIndex(), -1);
}

TEST(SourcePathView, FilePathSlashForSingleSlash)
{
    QmlDesigner::SourcePathView filePath("/");

    ASSERT_THAT(filePath.slashIndex(), 0);
}

TEST(SourcePathView, FilePathSlashForFileInRoot)
{
    QmlDesigner::SourcePathView filePath("/file.h");

    ASSERT_THAT(filePath.slashIndex(), 0);
}

TEST(SourcePathView, FilePathSlashForSomeLongerPath)
{
    QmlDesigner::SourcePathView filePath("/path/to/some/file.h");

    ASSERT_THAT(filePath.slashIndex(), 13);
}

TEST(SourcePathView, FilePathSlashForFileNameOnly)
{
    QmlDesigner::SourcePathView filePath("file.h");

    ASSERT_THAT(filePath.slashIndex(), -1);
}

TEST(SourcePathView, DirectoryPathForEmptyPath)
{
    QmlDesigner::SourcePathView filePath("");

    ASSERT_THAT(filePath.directory(), "");
}

TEST(SourcePathView, DirectoryPathForSingleSlashPath)
{
    QmlDesigner::SourcePathView filePath{"/"};

    ASSERT_THAT(filePath.directory(), "");
}

TEST(SourcePathView, DirectoryPathForLongerPath)
{
    QmlDesigner::SourcePathView filePath{"/path/to/some/file.h"};

    ASSERT_THAT(filePath.directory(), "/path/to/some");
}

TEST(SourcePathView, DirectoryPathForFileNameOnly)
{
    QmlDesigner::SourcePathView filePath{"file.h"};

    ASSERT_THAT(filePath.directory(), IsEmpty());
}

TEST(SourcePathView, FileNameForEmptyPath)
{
    QmlDesigner::SourcePathView filePath("");

    ASSERT_THAT(filePath.name(), "");
}

TEST(SourcePathView, FileNameForSingleSlashPath)
{
    QmlDesigner::SourcePathView filePath{"/"};

    ASSERT_THAT(filePath.name(), "");
}

TEST(SourcePathView, FileNameForLongerPath)
{
    QmlDesigner::SourcePathView filePath{"/path/to/some/file.h"};

    ASSERT_THAT(filePath.name(), "file.h");
}

TEST(SourcePathView, FileNameForFileNameOnly)
{
    QmlDesigner::SourcePathView filePath{"file.h"};

    ASSERT_THAT(filePath.name(), "file.h");
}

}
