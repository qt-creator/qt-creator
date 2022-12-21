// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "googletest.h"

#include <projectstorage/sourcepath.h>

namespace {

TEST(SourcePath, CreateFromPathString)
{
    QmlDesigner::SourcePath sourcePath{Utils::PathString{"/file/pathOne"}};

    ASSERT_THAT(sourcePath.directory(), "/file");
    ASSERT_THAT(sourcePath.name(), "pathOne");
}

TEST(FilePath, CreateFromDirectoryAndFileName)
{
    QmlDesigner::SourcePath sourcePath{Utils::PathString{"/file"}, Utils::PathString{"pathOne"}};

    ASSERT_THAT(sourcePath.directory(), "/file");
    ASSERT_THAT(sourcePath.name(), "pathOne");
    ASSERT_THAT(sourcePath.path(), "/file/pathOne");
}

TEST(FilePath, CreateFromCString)
{
    QmlDesigner::SourcePath sourcePath{"/file/pathOne"};

    ASSERT_THAT(sourcePath.directory(), "/file");
    ASSERT_THAT(sourcePath.name(), "pathOne");
}

TEST(FilePath, CreateFromFilePathView)
{
    QmlDesigner::SourcePath sourcePath{QmlDesigner::SourcePathView{"/file/pathOne"}};

    ASSERT_THAT(sourcePath.directory(), "/file");
    ASSERT_THAT(sourcePath.name(), "pathOne");
}

TEST(FilePath, CreateFromQString)
{
    QmlDesigner::SourcePath sourcePath{QString{"/file/pathOne"}};

    ASSERT_THAT(sourcePath.directory(), "/file");
    ASSERT_THAT(sourcePath.name(), "pathOne");
}

TEST(FilePath, DefaultFilePath)
{
    QmlDesigner::SourcePath sourcePath;

    ASSERT_THAT(sourcePath.directory(), "");
    ASSERT_THAT(sourcePath.name(), "");
}

TEST(FilePath, EmptyFilePath)
{
    QmlDesigner::SourcePath sourcePath("");

    ASSERT_THAT(sourcePath.directory(), "");
    ASSERT_THAT(sourcePath.name(), "");
}

}
