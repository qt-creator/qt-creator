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

TEST(UniqueName, generateId_stable_captilzation)
{
    QString id = "A CaMeL*cAsE";

    QString uniqueId = UniqueName::generateId(id);

    ASSERT_THAT(uniqueId, "aCaMeLCAsE");
}

TEST(UniqueName, generateId_contains_number_in_prefix_and_suffix)
{
    QString id = "_4KScartchySurface6";
    auto pred = [&](const QString &str) -> bool { return str == id; };

    QString uniqueId = UniqueName::generateId(id, pred);

    ASSERT_THAT(uniqueId, "_4KScartchySurface7");
}

TEST(UniqueName, generateId_begins_with_non_latin)
{
    QString id = "😂_saneId";

    QString uniqueId = UniqueName::generateId(id);

    ASSERT_THAT(uniqueId, "_saneId");
}

TEST(UniqueName, generateId_non_latin_chars)
{
    QString id = "😂1😂test😅*chars";

    QString uniqueId = UniqueName::generateId(id);

    ASSERT_THAT(uniqueId, "_1TestChars");
}

TEST(UniqueName, generateId_use_fallback_id)
{
    QString id = "😂😂  😅*    ";

    QString uniqueId = UniqueName::generateId(id, "validFallbackId");

    ASSERT_THAT(uniqueId, "validFallbackId");
}

TEST(UniqueName, generateId_unused_fallback_id)
{
    QString id = "saneId";

    QString uniqueId = UniqueName::generateId(id, "fallbackId");

    ASSERT_THAT(uniqueId, "saneId");
}

TEST(UniqueName, generateId_use_emtpy_fallback)
{
    QString id = "😂😂  😅*    ";

    QString uniqueId = UniqueName::generateId(id, QString{});

    ASSERT_TRUE(uniqueId.isEmpty());
}

TEST(UniqueName, generateId_use_fallback_when_id_exists)
{
    QStringList existingIds = {"validFallbackId", "validFallbackId1"};
    auto pred = [&](const QString &id) -> bool { return existingIds.contains(id); };
    QString id = "😂😂  😅*    ";

    QString uniqueId = UniqueName::generateId(id, "validFallbackId", pred);

    ASSERT_THAT(uniqueId, "validFallbackId2");
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

TEST(UniqueName, generateTypeName_returns_valid_type_name_unaltered)
{
    QString input = "MyType";

    QString validTypeName = UniqueName::generateTypeName(input, "Gadget");

    ASSERT_THAT(validTypeName, "MyType");
}

TEST(UniqueName, generateTypeName_returns_valid_type_name_without_predicate)
{
    QString input = "ab c";

    QString validTypeName = UniqueName::generateTypeName(input, "Gadget");

    ASSERT_THAT(validTypeName, "AbC");
}

TEST(UniqueName, generateTypeName_returns_fallback_with_empty_input)
{
    QString input = "";

    QString validTypeName = UniqueName::generateTypeName(input, "Gadget");

    ASSERT_THAT(validTypeName, "Gadget");
}

TEST(UniqueName, generateTypeName_returns_fallback_with_all_invalid_chars)
{
    QString input = "😂😂😅*";

    QString validTypeName = UniqueName::generateTypeName(input, "Gadget");

    ASSERT_THAT(validTypeName, "Gadget");
}

TEST(UniqueName, generateTypeName_returns_valid_type_name_with_predicate)
{
    auto pred = [](const QString &) -> bool { return false; };
    QString input = "abc";

    QString validTypeName = UniqueName::generateTypeName(input, "Gadget", pred);

    ASSERT_THAT(validTypeName, "Abc");
}

TEST(UniqueName, generateTypeName_returns_valid_unique_type_name)
{
    QStringList existingIds = {"Abc"};
    auto pred = [&existingIds](const QString &t) { return existingIds.contains(t); };
    QString input = "abc";

    QString validTypeName = UniqueName::generateTypeName(input, "Gadget", pred);

    ASSERT_THAT(validTypeName, "Abc1");
}

TEST(UniqueName, generateTypeName_filters_invalid_chars_and_capitalize)
{
    QString input = "😂a😂b😅*#c";

    QString validTypeName = UniqueName::generateTypeName(input, "Gadget");

    ASSERT_THAT(validTypeName, "ABC");
}

TEST(UniqueName, generateTypeName_removes_underscore_from_front)
{
    QString input = "_ab_c";

    QString validTypeName = UniqueName::generateTypeName(input, "Gadget");

    ASSERT_THAT(validTypeName, "Ab_c");
}

TEST(UniqueName, generateTypeName_removes_numbers_from_front)
{
    QString input = "12ab_c3";

    QString validTypeName = UniqueName::generateTypeName(input, "Gadget");

    ASSERT_THAT(validTypeName, "Ab_c3");
}

TEST(UniqueName, generateTypeName_removes_dots)
{
    QString input = ".ab.c3";

    QString validTypeName = UniqueName::generateTypeName(input, "Gadget");

    ASSERT_THAT(validTypeName, "AbC3");
}

TEST(UniqueName, generateTypeName_removes_spaces_and_capitalize)
{
    QString input = "ab c ";

    QString validTypeName = UniqueName::generateTypeName(input, "Gadget");

    ASSERT_THAT(validTypeName, "AbC");
}

TEST(UniqueName, generateTypeName_return_unique_type_name_with_stable_count)
{
    QStringList existingIds = {"Abc3", "Abc4"};
    auto pred = [&existingIds](const QString &t) { return existingIds.contains(t); };
    QString input = "abc3";

    QString validTypeName = UniqueName::generateTypeName(input, "Gadget", pred);

    ASSERT_THAT(validTypeName, "Abc5");
}

TEST(UniqueName, generateTypeName_returns_valid_type_name_with_stable_capitalization)
{
    QString input = "myTypENAme x";

    QString validTypeName = UniqueName::generateTypeName(input, "Gadget");

    ASSERT_THAT(validTypeName, "MyTypENAmeX");
}

} // namespace
