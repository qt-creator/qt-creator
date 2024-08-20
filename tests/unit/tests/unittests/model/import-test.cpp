// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <import.h>
#include <qmldesignerutils/version.h>

namespace {

TEST(Import, parse_version)
{
    auto import = QmlDesigner::Import::createLibraryImport("Qml", "6.5");

    auto version = import.toVersion();

    ASSERT_THAT(version, FieldsAre(6, 5, 0));
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

} // namespace
