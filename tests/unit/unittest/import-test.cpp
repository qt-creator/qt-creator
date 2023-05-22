// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "googletest.h"

#include <import.h>

namespace {

TEST(Import, ParseVersion)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml", "6.5");

    auto version = import.toVersion();

    ASSERT_THAT(version, FieldsAre(6, 5));
}

TEST(Import, ParseMajorVersion)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml", "6.5");

    auto version = import.majorVersion();

    ASSERT_THAT(version, 6);
}

TEST(Import, MajorVersionIsInvalidForEmptyString)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml");

    auto version = import.majorVersion();

    ASSERT_THAT(version, -1);
}

TEST(Import, MajorVersionIsInvalidForBrokenString)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml", "6,5");

    auto version = import.majorVersion();

    ASSERT_THAT(version, -1);
}

TEST(Import, ParseMinorVersion)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml", "6.5");

    auto version = import.minorVersion();

    ASSERT_THAT(version, 5);
}

TEST(Import, MinorVersionIsInvalidForEmptyString)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml");

    auto version = import.minorVersion();

    ASSERT_THAT(version, -1);
}

TEST(Import, MinorVersionIsInvalidForBrokenString)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml", "6,5");

    auto version = import.minorVersion();

    ASSERT_THAT(version, -1);
}

TEST(Import, VersionIsNotEmpty)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml", "6.5");

    auto version = import.toVersion();

    ASSERT_FALSE(version.isEmpty());
}

TEST(Import, BrokenVersionStringIsEmptyVersion)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml", "6");

    auto version = import.toVersion();

    ASSERT_TRUE(version.isEmpty());
}

TEST(Import, EmptyVersionStringIsEmptyVersion)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml");

    auto version = import.toVersion();

    ASSERT_TRUE(version.isEmpty());
}

TEST(Import, SameVersionsAreEqual)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2{6, 5};

    bool isEqual = version1 == version2;

    ASSERT_TRUE(isEqual);
}

TEST(Import, InvalidVersionsAreEqual)
{
    QmlDesigner::Version version1;
    QmlDesigner::Version version2;

    bool isEqual = version1 == version2;

    ASSERT_TRUE(isEqual);
}

TEST(Import, DifferentMinorVersionsAreNotEqual)
{
    QmlDesigner::Version version1{6, 4};
    QmlDesigner::Version version2{6, 5};

    bool isEqual = version1 == version2;

    ASSERT_FALSE(isEqual);
}

TEST(Import, DifferentMajorVersionsAreNotEqual)
{
    QmlDesigner::Version version1{5, 5};
    QmlDesigner::Version version2{6, 5};

    bool isEqual = version1 == version2;

    ASSERT_FALSE(isEqual);
}

TEST(Import, LessMinorVersionsAreLess)
{
    QmlDesigner::Version version1{6, 4};
    QmlDesigner::Version version2{6, 5};

    bool isLess = version1 < version2;

    ASSERT_TRUE(isLess);
}

TEST(Import, LessMajorVersionsAreLess)
{
    QmlDesigner::Version version1{5, 15};
    QmlDesigner::Version version2{6, 5};

    bool isLess = version1 < version2;

    ASSERT_TRUE(isLess);
}

TEST(Import, SameVersionsAreNotLess)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2{6, 5};

    bool isLess = version1 < version2;

    ASSERT_FALSE(isLess);
}

TEST(Import, EmptyVersionIsNotLess)
{
    QmlDesigner::Version version1;
    QmlDesigner::Version version2{6, 5};

    bool isLess = version1 < version2;

    ASSERT_FALSE(isLess);
}

TEST(Import, NonEmptyVersionIsIsLessThanEmptyVersion)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2;

    bool isLess = version1 < version2;

    ASSERT_TRUE(isLess);
}

TEST(Import, GreaterMinorVersionsAreGreater)
{
    QmlDesigner::Version version1{6, 6};
    QmlDesigner::Version version2{6, 5};

    bool isGreater = version1 > version2;

    ASSERT_TRUE(isGreater);
}

TEST(Import, GreaterMajorVersionsAreGreater)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2{5, 15};

    bool isGreater = version1 > version2;

    ASSERT_TRUE(isGreater);
}

TEST(Import, SameVersionsAreNotGreater)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2{6, 5};

    bool isGreater = version1 > version2;

    ASSERT_FALSE(isGreater);
}

TEST(Import, EmptyVersionIsGreater)
{
    QmlDesigner::Version version1;
    QmlDesigner::Version version2{6, 5};

    bool isGreater = version1 > version2;

    ASSERT_TRUE(isGreater);
}

TEST(Import, NonEmptyVersionIsIsNotGreaterThanEmptyVersion)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2;

    bool isGreater = version1 > version2;

    ASSERT_FALSE(isGreater);
}

TEST(Import, LessEqualMinorVersionsAreLessEqual)
{
    QmlDesigner::Version version1{6, 4};
    QmlDesigner::Version version2{6, 5};

    bool isLessEqual = version1 <= version2;

    ASSERT_TRUE(isLessEqual);
}

TEST(Import, LessqualMajorVersionsAreLessqual)
{
    QmlDesigner::Version version1{5, 15};
    QmlDesigner::Version version2{6, 5};

    bool isLessEqual = version1 <= version2;

    ASSERT_TRUE(isLessEqual);
}

TEST(Import, SameVersionsAreLessqual)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2{6, 5};

    bool isLessEqual = version1 <= version2;

    ASSERT_TRUE(isLessEqual);
}

TEST(Import, EmptyVersionIsNotLessqual)
{
    QmlDesigner::Version version1;
    QmlDesigner::Version version2{6, 5};

    bool isLessEqual = version1 <= version2;

    ASSERT_FALSE(isLessEqual);
}

TEST(Import, NonEmptyVersionIsIsLessqualThanEmptyVersion)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2;

    bool isLessEqual = version1 <= version2;

    ASSERT_TRUE(isLessEqual);
}

TEST(Import, GreaterEqualMinorVersionsAreGreaterEqual)
{
    QmlDesigner::Version version1{6, 6};
    QmlDesigner::Version version2{6, 5};

    bool isGreaterEqual = version1 >= version2;

    ASSERT_TRUE(isGreaterEqual);
}

TEST(Import, GreaterEqualMajorVersionsAreGreaterEqual)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2{5, 15};

    bool isGreaterEqual = version1 >= version2;

    ASSERT_TRUE(isGreaterEqual);
}

TEST(Import, SameVersionsAreGreaterEqual)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2{6, 5};

    bool isGreaterEqual = version1 >= version2;

    ASSERT_TRUE(isGreaterEqual);
}

TEST(Import, EmptyVersionIsGreaterEqual)
{
    QmlDesigner::Version version1;
    QmlDesigner::Version version2{6, 5};

    bool isGreaterEqual = version1 >= version2;

    ASSERT_TRUE(isGreaterEqual);
}

TEST(Import, NonEmptyVersionIsIsNotGreaterEqualThanEmptyVersion)
{
    QmlDesigner::Version version1{6, 5};
    QmlDesigner::Version version2;

    bool isGreaterEqual = version1 >= version2;

    ASSERT_FALSE(isGreaterEqual);
}

} // namespace
