// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <googletest.h>
#include <qmldesignerutils/version.h>

namespace {

TEST(Version, same_versions_are_equal)
{
    QmlDesigner::Version version1{6, 5, 2};
    QmlDesigner::Version version2{6, 5, 2};

    bool isEqual = version1 == version2;

    ASSERT_TRUE(isEqual);
}

TEST(Version, invalid_versions_are_equal)
{
    QmlDesigner::Version version1;
    QmlDesigner::Version version2;

    bool isEqual = version1 == version2;

    ASSERT_TRUE(isEqual);
}

TEST(Version, different_minor_versions_are_not_equal)
{
    QmlDesigner::Version version1{6, 4};
    QmlDesigner::Version version2{6, 5};

    bool isEqual = version1 == version2;

    ASSERT_FALSE(isEqual);
}

TEST(Version, different_major_versions_are_not_equal)
{
    QmlDesigner::Version version1{5, 5};
    QmlDesigner::Version version2{6, 5};

    bool isEqual = version1 == version2;

    ASSERT_FALSE(isEqual);
}

TEST(Version, different_patch_versions_are_not_equal)
{
    QmlDesigner::Version version1{6, 5, 1};
    QmlDesigner::Version version2{6, 5, 2};

    bool isEqual = version1 == version2;

    ASSERT_FALSE(isEqual);
}

TEST(Version, less_minor_versions_are_less)
{
    QmlDesigner::Version version1{6, 4};
    QmlDesigner::Version version2{6, 5};

    bool isLess = version1 < version2;

    ASSERT_TRUE(isLess);
}

TEST(Version, less_major_versions_are_less)
{
    QmlDesigner::Version version1{5, 15};
    QmlDesigner::Version version2{6, 5};

    bool isLess = version1 < version2;

    ASSERT_TRUE(isLess);
}

TEST(Version, less_patch_versions_are_less)
{
    QmlDesigner::Version version1{6, 5, 3};
    QmlDesigner::Version version2{6, 5, 15};

    bool isLess = version1 < version2;

    ASSERT_TRUE(isLess);
}

TEST(Version, same_versions_are_not_less)
{
    QmlDesigner::Version version1{6, 5, 1};
    QmlDesigner::Version version2{6, 5, 1};

    bool isLess = version1 < version2;

    ASSERT_FALSE(isLess);
}

TEST(Version, empty_version_is_not_less)
{
    QmlDesigner::Version version1;
    QmlDesigner::Version version2{6, 5};

    bool isLess = version1 < version2;

    ASSERT_FALSE(isLess);
}

TEST(Version, non_empty_version_is_is_less_than_empty_version)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2;

    bool isLess = version1 < version2;

    ASSERT_TRUE(isLess);
}

TEST(Version, greater_minor_versions_are_greater)
{
    QmlDesigner::Version version1{6, 6};
    QmlDesigner::Version version2{6, 5};

    bool isGreater = version1 > version2;

    ASSERT_TRUE(isGreater);
}

TEST(Version, greater_major_versions_are_greater)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2{5, 15};

    bool isGreater = version1 > version2;

    ASSERT_TRUE(isGreater);
}

TEST(Version, same_versions_are_not_greater)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2{6, 5};

    bool isGreater = version1 > version2;

    ASSERT_FALSE(isGreater);
}

TEST(Version, empty_version_is_greater)
{
    QmlDesigner::Version version1;
    QmlDesigner::Version version2{6, 5};

    bool isGreater = version1 > version2;

    ASSERT_TRUE(isGreater);
}

TEST(Version, empty_version_is_greater_than_non_empty_version)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2;

    bool isGreater = version1 > version2;

    ASSERT_FALSE(isGreater);
}

TEST(Version, less_equal_minor_versions_are_less_equal)
{
    QmlDesigner::Version version1{6, 4};
    QmlDesigner::Version version2{6, 5};

    bool isLessEqual = version1 <= version2;

    ASSERT_TRUE(isLessEqual);
}

TEST(Version, lessqual_major_versions_are_lessqual)
{
    QmlDesigner::Version version1{5, 15};
    QmlDesigner::Version version2{6, 5};

    bool isLessEqual = version1 <= version2;

    ASSERT_TRUE(isLessEqual);
}

TEST(Version, same_versions_are_lessqual)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2{6, 5};

    bool isLessEqual = version1 <= version2;

    ASSERT_TRUE(isLessEqual);
}

TEST(Version, empty_version_is_not_lessqual)
{
    QmlDesigner::Version version1;
    QmlDesigner::Version version2{6, 5};

    bool isLessEqual = version1 <= version2;

    ASSERT_FALSE(isLessEqual);
}

TEST(Version, non_empty_version_is_is_lessqual_than_empty_version)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2;

    bool isLessEqual = version1 <= version2;

    ASSERT_TRUE(isLessEqual);
}

TEST(Version, greater_equal_minor_versions_are_greater_equal)
{
    QmlDesigner::Version version1{6, 6};
    QmlDesigner::Version version2{6, 5};

    bool isGreaterEqual = version1 >= version2;

    ASSERT_TRUE(isGreaterEqual);
}

TEST(Version, greater_equal_major_versions_are_greater_equal)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2{5, 15};

    bool isGreaterEqual = version1 >= version2;

    ASSERT_TRUE(isGreaterEqual);
}

TEST(Version, same_versions_are_greater_equal)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2{6, 5};

    bool isGreaterEqual = version1 >= version2;

    ASSERT_TRUE(isGreaterEqual);
}

TEST(Version, empty_version_is_greater_equal)
{
    QmlDesigner::Version version1;
    QmlDesigner::Version version2{6, 5};

    bool isGreaterEqual = version1 >= version2;

    ASSERT_TRUE(isGreaterEqual);
}

TEST(Version, non_empty_version_is_is_not_greater_equal_than_empty_version)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2;

    bool isGreaterEqual = version1 >= version2;

    ASSERT_FALSE(isGreaterEqual);
}

TEST(Version, version_fromString_empty_string_makes_empty_version)
{
    QStringView versionStr = u"";

    QmlDesigner::Version version = QmlDesigner::Version::fromString(versionStr);

    ASSERT_TRUE(version.isEmpty());
}

TEST(Version, version_fromString_is_the_same_as_initialized_version)
{
    QStringView versionStr = u"6.9.2";

    QmlDesigner::Version version = QmlDesigner::Version::fromString(versionStr);

    ASSERT_THAT(version, Eq(QmlDesigner::Version{6, 9, 2}));
}

TEST(Version, version_fromString_sets_zero_for_minor_and_patch)
{
    QStringView versionStr = u"6";

    auto version = QmlDesigner::Version::fromString(versionStr);

    ASSERT_THAT(version, FieldsAre(6, 0, 0));
}

TEST(Version, version_fromString_sets_zero_for_patch)
{
    QStringView versionStr = u"6.3";

    auto version = QmlDesigner::Version::fromString(versionStr);

    ASSERT_THAT(version, FieldsAre(6, 3, 0));
}

TEST(Version, version_fromString_empty_parts_are_zero)
{
    QStringView versionStr = u"6.";

    auto version = QmlDesigner::Version::fromString(versionStr);

    ASSERT_THAT(version, FieldsAre(6, 0, 0));
}

TEST(Version, version_fromString_non_standard_version_is_empty)
{
    QStringView versionStr = u"1-beta1";

    auto version = QmlDesigner::Version::fromString(versionStr);

    ASSERT_TRUE(version.isEmpty());
}

TEST(Version, version_fromString_with_negative_major_is_empty)
{
    QStringView versionStr = u"-1";

    auto version = QmlDesigner::Version::fromString(versionStr);

    ASSERT_TRUE(version.isEmpty());
}

TEST(Version, version_fromString_negative_minor_will_be_changed_to_zero)
{
    QStringView versionStr = u"6.-1";

    auto version = QmlDesigner::Version::fromString(versionStr);

    ASSERT_THAT(version, FieldsAre(6, 0, 0));
}

} // namespace
