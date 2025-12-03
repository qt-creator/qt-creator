// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <sourcepathstorage/sourcepathview.h>

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

TEST(SourcePathView, common_path_for_same_path)
{
    std::string_view input{"/foo/"};

    auto matched = QmlDesigner::SourcePathView::commonPath(input, input);

    ASSERT_THAT(matched, FieldsAre("/foo/", IsEmpty(), IsEmpty()));
}

TEST(SourcePathView, common_path)
{
    std::string_view input1{"/foo/bar"};
    std::string_view input2{"/foo/hoo"};

    auto matched = QmlDesigner::SourcePathView::commonPath(input1, input2);

    ASSERT_THAT(matched, FieldsAre("/foo/", "bar", "hoo"));
}

TEST(SourcePathView, common_path_last_is_sub_path)
{
    std::string_view input1{"/foo/bar"};
    std::string_view input2{"/foo/"};

    auto matched = QmlDesigner::SourcePathView::commonPath(input1, input2);

    ASSERT_THAT(matched, FieldsAre("/foo/", "bar", IsEmpty()));
}

TEST(SourcePathView, common_path_first_is_sub_path)
{
    std::string_view input1{"/foo/"};
    std::string_view input2{"/foo/bar"};

    auto matched = QmlDesigner::SourcePathView::commonPath(input1, input2);

    ASSERT_THAT(matched, FieldsAre("/foo/", IsEmpty(), "bar"));
}

TEST(SourcePathView, common_path_last_is_file)
{
    std::string_view input1{"/foo/bar"};
    std::string_view input2{"/foo"};

    auto matched = QmlDesigner::SourcePathView::commonPath(input1, input2);

    ASSERT_THAT(matched, FieldsAre("/", "foo/bar", "foo"));
}

TEST(SourcePathView, common_path_first_is_file)
{
    std::string_view input1{"/foo"};
    std::string_view input2{"/foo/bar"};

    auto matched = QmlDesigner::SourcePathView::commonPath(input1, input2);

    ASSERT_THAT(matched, FieldsAre("/", "foo", "foo/bar"));
}
}
