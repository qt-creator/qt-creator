// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <googletest.h>

#include <uniquename.h>

namespace {

namespace UniqueName = QmlDesigner::UniqueName;

TEST(UniqueName, generate_returns_same_input_if_predicate_returns_false)
{
    auto pred = [](const QString &) -> bool { return false; };
    QString name = "abc";

    QString uniqueName = UniqueName::generate(name, pred);

    ASSERT_THAT(uniqueName, "abc");
}

TEST(UniqueName, generateId_returns_properly_formatted_id_when_predicate_is_not_provided)
{
    QString id = "  A    bc   d _";

    QString uniqueId = UniqueName::generateId(id);

    ASSERT_THAT(uniqueId, "aBcD_");
}

TEST(UniqueName, generateId_returns_properly_formatted_id)
{
    auto pred = [](const QString &) -> bool { return false; };
    QString id = "  A    bc   d _";

    QString uniqueId = UniqueName::generateId(id, pred);

    ASSERT_THAT(uniqueId, "aBcD_");
}

TEST(UniqueName, generateId_returns_properly_formatted_unique_id_when_id_exists)
{
    QStringList existingIds = {"aBcD009", "aBcD010"};
    auto pred = [&] (const QString &id) -> bool {
        return existingIds.contains(id);
    };
    QString id = "  A    bc   d 0 \t 0 9\n";

    QString uniqueId = UniqueName::generateId(id, pred);

    ASSERT_THAT(uniqueId, "aBcD011");
}

TEST(UniqueName, generateId_properly_handles_dot_separated_words)
{
    auto pred = [&](const QString &) -> bool { return false; };
    QString id = "Foo.bar*foo";

    QString uniqueId = UniqueName::generateId(id, pred);

    ASSERT_THAT(uniqueId, "fooBarFoo");
}

TEST(UniqueName, generateId_prefixes_with_underscore_if_id_is_a_reserved_word)
{
    auto pred = [&](const QString &) -> bool { return false; };
    QString id = "for";

    QString uniqueId = UniqueName::generateId(id, pred);

    ASSERT_THAT(uniqueId, "_for");
}

TEST(UniqueName, generateId_prefixes_with_underscore_if_id_is_a_number)
{
    auto pred = [&](const QString &) -> bool { return false; };
    QString id = "436";

    QString uniqueId = UniqueName::generateId(id, pred);

    ASSERT_THAT(uniqueId, "_436");
}

TEST(UniqueName, generatePath_returns_same_path_when_path_doesnt_exist)
{
    QString path = "<<<non/existing/path>>>";

    QString uniquePath = UniqueName::generatePath(path);

    ASSERT_THAT(uniquePath, path);
}

TEST(UniqueName, generatePath_returns_unique_path_when_path_exists)
{
    QString path = UNITTEST_DIR;

    QString uniquePath = UniqueName::generatePath(path);

    ASSERT_THAT(uniquePath, QString(UNITTEST_DIR).append("1"));
}

} // namespace
