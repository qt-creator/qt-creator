// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <projectstorage/sourcepath.h>

namespace {

TEST(SourcePath, create_from_path_string)
{
    QmlDesigner::SourcePath sourcePath{Utils::PathString{"/file/pathOne"}};

    ASSERT_THAT(sourcePath.directory(), "/file");
    ASSERT_THAT(sourcePath.name(), "pathOne");
}

TEST(FilePath, create_from_directory_and_file_name)
{
    QmlDesigner::SourcePath sourcePath{Utils::PathString{"/file"}, Utils::PathString{"pathOne"}};

    ASSERT_THAT(sourcePath.directory(), "/file");
    ASSERT_THAT(sourcePath.name(), "pathOne");
    ASSERT_THAT(sourcePath.path(), "/file/pathOne");
}

TEST(FilePath, create_from_c_string)
{
    QmlDesigner::SourcePath sourcePath{"/file/pathOne"};

    ASSERT_THAT(sourcePath.directory(), "/file");
    ASSERT_THAT(sourcePath.name(), "pathOne");
}

TEST(FilePath, create_from_file_path_view)
{
    QmlDesigner::SourcePath sourcePath{QmlDesigner::SourcePathView{"/file/pathOne"}};

    ASSERT_THAT(sourcePath.directory(), "/file");
    ASSERT_THAT(sourcePath.name(), "pathOne");
}

TEST(FilePath, create_from_q_string)
{
    QmlDesigner::SourcePath sourcePath{QString{"/file/pathOne"}};

    ASSERT_THAT(sourcePath.directory(), "/file");
    ASSERT_THAT(sourcePath.name(), "pathOne");
}

TEST(FilePath, default_file_path)
{
    QmlDesigner::SourcePath sourcePath;

    ASSERT_THAT(sourcePath.directory(), "");
    ASSERT_THAT(sourcePath.name(), "");
}

TEST(FilePath, empty_file_path)
{
    QmlDesigner::SourcePath sourcePath("");

    ASSERT_THAT(sourcePath.directory(), "");
    ASSERT_THAT(sourcePath.name(), "");
}

}
