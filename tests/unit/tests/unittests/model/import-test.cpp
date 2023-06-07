// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <import.h>

namespace {

TEST(Import, parse_version)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml", "6.5");

    auto version = import.toVersion();

    ASSERT_THAT(version, FieldsAre(6, 5));
}

TEST(Import, parse_major_version)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml", "6.5");

    auto version = import.majorVersion();

    ASSERT_THAT(version, 6);
}

TEST(Import, major_version_is_invalid_for_empty_string)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml");

    auto version = import.majorVersion();

    ASSERT_THAT(version, -1);
}

TEST(Import, major_version_is_invalid_for_broken_string)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml", "6,5");

    auto version = import.majorVersion();

    ASSERT_THAT(version, -1);
}

TEST(Import, parse_minor_version)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml", "6.5");

    auto version = import.minorVersion();

    ASSERT_THAT(version, 5);
}

TEST(Import, minor_version_is_invalid_for_empty_string)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml");

    auto version = import.minorVersion();

    ASSERT_THAT(version, -1);
}

TEST(Import, minor_version_is_invalid_for_broken_string)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml", "6,5");

    auto version = import.minorVersion();

    ASSERT_THAT(version, -1);
}

TEST(Import, version_is_not_empty)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml", "6.5");

    auto version = import.toVersion();

    ASSERT_FALSE(version.isEmpty());
}

TEST(Import, broken_version_string_is_empty_version)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml", "6");

    auto version = import.toVersion();

    ASSERT_TRUE(version.isEmpty());
}

TEST(Import, empty_version_string_is_empty_version)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml");

    auto version = import.toVersion();

    ASSERT_TRUE(version.isEmpty());
}

TEST(Import, same_versions_are_equal)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2{6, 5};

    bool isEqual = version1 == version2;

    ASSERT_TRUE(isEqual);
}

TEST(Import, invalid_versions_are_equal)
{
    QmlDesigner::Version version1;
    QmlDesigner::Version version2;

    bool isEqual = version1 == version2;

    ASSERT_TRUE(isEqual);
}

TEST(Import, different_minor_versions_are_not_equal)
{
    QmlDesigner::Version version1{6, 4};
    QmlDesigner::Version version2{6, 5};

    bool isEqual = version1 == version2;

    ASSERT_FALSE(isEqual);
}

TEST(Import, different_major_versions_are_not_equal)
{
    QmlDesigner::Version version1{5, 5};
    QmlDesigner::Version version2{6, 5};

    bool isEqual = version1 == version2;

    ASSERT_FALSE(isEqual);
}

TEST(Import, less_minor_versions_are_less)
{
    QmlDesigner::Version version1{6, 4};
    QmlDesigner::Version version2{6, 5};

    bool isLess = version1 < version2;

    ASSERT_TRUE(isLess);
}

TEST(Import, less_major_versions_are_less)
{
    QmlDesigner::Version version1{5, 15};
    QmlDesigner::Version version2{6, 5};

    bool isLess = version1 < version2;

    ASSERT_TRUE(isLess);
}

TEST(Import, same_versions_are_not_less)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2{6, 5};

    bool isLess = version1 < version2;

    ASSERT_FALSE(isLess);
}

TEST(Import, empty_version_is_not_less)
{
    QmlDesigner::Version version1;
    QmlDesigner::Version version2{6, 5};

    bool isLess = version1 < version2;

    ASSERT_FALSE(isLess);
}

TEST(Import, non_empty_version_is_is_less_than_empty_version)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2;

    bool isLess = version1 < version2;

    ASSERT_TRUE(isLess);
}

TEST(Import, greater_minor_versions_are_greater)
{
    QmlDesigner::Version version1{6, 6};
    QmlDesigner::Version version2{6, 5};

    bool isGreater = version1 > version2;

    ASSERT_TRUE(isGreater);
}

TEST(Import, greater_major_versions_are_greater)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2{5, 15};

    bool isGreater = version1 > version2;

    ASSERT_TRUE(isGreater);
}

TEST(Import, same_versions_are_not_greater)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2{6, 5};

    bool isGreater = version1 > version2;

    ASSERT_FALSE(isGreater);
}

TEST(Import, empty_version_is_greater)
{
    QmlDesigner::Version version1;
    QmlDesigner::Version version2{6, 5};

    bool isGreater = version1 > version2;

    ASSERT_TRUE(isGreater);
}

TEST(Import, non_empty_version_is_is_not_greater_than_empty_version)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2;

    bool isGreater = version1 > version2;

    ASSERT_FALSE(isGreater);
}

TEST(Import, less_equal_minor_versions_are_less_equal)
{
    QmlDesigner::Version version1{6, 4};
    QmlDesigner::Version version2{6, 5};

    bool isLessEqual = version1 <= version2;

    ASSERT_TRUE(isLessEqual);
}

TEST(Import, lessqual_major_versions_are_lessqual)
{
    QmlDesigner::Version version1{5, 15};
    QmlDesigner::Version version2{6, 5};

    bool isLessEqual = version1 <= version2;

    ASSERT_TRUE(isLessEqual);
}

TEST(Import, same_versions_are_lessqual)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2{6, 5};

    bool isLessEqual = version1 <= version2;

    ASSERT_TRUE(isLessEqual);
}

TEST(Import, empty_version_is_not_lessqual)
{
    QmlDesigner::Version version1;
    QmlDesigner::Version version2{6, 5};

    bool isLessEqual = version1 <= version2;

    ASSERT_FALSE(isLessEqual);
}

TEST(Import, non_empty_version_is_is_lessqual_than_empty_version)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2;

    bool isLessEqual = version1 <= version2;

    ASSERT_TRUE(isLessEqual);
}

TEST(Import, greater_equal_minor_versions_are_greater_equal)
{
    QmlDesigner::Version version1{6, 6};
    QmlDesigner::Version version2{6, 5};

    bool isGreaterEqual = version1 >= version2;

    ASSERT_TRUE(isGreaterEqual);
}

TEST(Import, greater_equal_major_versions_are_greater_equal)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2{5, 15};

    bool isGreaterEqual = version1 >= version2;

    ASSERT_TRUE(isGreaterEqual);
}

TEST(Import, same_versions_are_greater_equal)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2{6, 5};

    bool isGreaterEqual = version1 >= version2;

    ASSERT_TRUE(isGreaterEqual);
}

TEST(Import, empty_version_is_greater_equal)
{
    QmlDesigner::Version version1;
    QmlDesigner::Version version2{6, 5};

    bool isGreaterEqual = version1 >= version2;

    ASSERT_TRUE(isGreaterEqual);
}

TEST(Import, non_empty_version_is_is_not_greater_equal_than_empty_version)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2;

    bool isGreaterEqual = version1 >= version2;

    ASSERT_FALSE(isGreaterEqual);
}

} // namespace
