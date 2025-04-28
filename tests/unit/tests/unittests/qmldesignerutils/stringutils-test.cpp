// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <googletest.h>
#include <qmldesignerutils/stringutils.h>

namespace {

using QmlDesigner::StringUtils::split_last;

TEST(StringUtils_split_last, leaf_is_empty_for_empty_input)
{
    QString text;

    auto [steam, leaf] = split_last(text, u'.');

    ASSERT_THAT(leaf, IsEmpty());
}

TEST(StringUtils_split_last, leaf_is_empty_with_ending_dot)
{
    QString text;

    auto [steam, leaf] = split_last(text, u'.');

    ASSERT_THAT(leaf, IsEmpty());
}

TEST(StringUtils_split_last, steam_is_empty_for_last_beginning_dot)
{
    QString text = ".bar";

    auto [steam, leaf] = split_last(text, u'.');

    ASSERT_THAT(steam, IsEmpty());
}

TEST(StringUtils_split_last, steam_is_empty)
{
    QString text = ".";

    auto [steam, leaf] = split_last(text, u'.');

    ASSERT_THAT(steam, IsEmpty());
}

TEST(StringUtils_split_last, leaf)
{
    QString text = "foo.foo.bar";

    auto [steam, leaf] = split_last(text, u'.');

    ASSERT_THAT(leaf, u"bar");
}

TEST(StringUtils_split_last, leaf_for_not_dot)
{
    QString text = "bar";

    auto [steam, leaf] = split_last(text, u'.');

    ASSERT_THAT(leaf, u"bar");
}

TEST(StringUtils_split_last, steam)
{
    QString text = "foo.foo.bar";

    auto [steam, leaf] = split_last(text, u'.');

    ASSERT_THAT(steam, u"foo.foo");
}

TEST(StringUtils_split_last, no_steam_for_not_dot)
{
    QString text = "bar";

    auto [steam, leaf] = split_last(text, u'.');

    ASSERT_THAT(steam, IsEmpty());
}
} // namespace
