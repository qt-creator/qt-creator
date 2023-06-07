// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <projectstorage/sourcepathview.h>

namespace {

TEST(SourcePathView, file_path_slash_for_empty_path)
{
    QmlDesigner::SourcePathView filePath("");

    ASSERT_THAT(filePath.slashIndex(), -1);
}

TEST(SourcePathView, file_path_slash_for_single_slash)
{
    QmlDesigner::SourcePathView filePath("/");

    ASSERT_THAT(filePath.slashIndex(), 0);
}

TEST(SourcePathView, file_path_slash_for_file_in_root)
{
    QmlDesigner::SourcePathView filePath("/file.h");

    ASSERT_THAT(filePath.slashIndex(), 0);
}

TEST(SourcePathView, file_path_slash_for_some_longer_path)
{
    QmlDesigner::SourcePathView filePath("/path/to/some/file.h");

    ASSERT_THAT(filePath.slashIndex(), 13);
}

TEST(SourcePathView, file_path_slash_for_file_name_only)
{
    QmlDesigner::SourcePathView filePath("file.h");

    ASSERT_THAT(filePath.slashIndex(), -1);
}

TEST(SourcePathView, directory_path_for_empty_path)
{
    QmlDesigner::SourcePathView filePath("");

    ASSERT_THAT(filePath.directory(), "");
}

TEST(SourcePathView, directory_path_for_single_slash_path)
{
    QmlDesigner::SourcePathView filePath{"/"};

    ASSERT_THAT(filePath.directory(), "");
}

TEST(SourcePathView, directory_path_for_longer_path)
{
    QmlDesigner::SourcePathView filePath{"/path/to/some/file.h"};

    ASSERT_THAT(filePath.directory(), "/path/to/some");
}

TEST(SourcePathView, directory_path_for_file_name_only)
{
    QmlDesigner::SourcePathView filePath{"file.h"};

    ASSERT_THAT(filePath.directory(), IsEmpty());
}

TEST(SourcePathView, file_name_for_empty_path)
{
    QmlDesigner::SourcePathView filePath("");

    ASSERT_THAT(filePath.name(), "");
}

TEST(SourcePathView, file_name_for_single_slash_path)
{
    QmlDesigner::SourcePathView filePath{"/"};

    ASSERT_THAT(filePath.name(), "");
}

TEST(SourcePathView, file_name_for_longer_path)
{
    QmlDesigner::SourcePathView filePath{"/path/to/some/file.h"};

    ASSERT_THAT(filePath.name(), "file.h");
}

TEST(SourcePathView, file_name_for_file_name_only)
{
    QmlDesigner::SourcePathView filePath{"file.h"};

    ASSERT_THAT(filePath.name(), "file.h");
}

}
