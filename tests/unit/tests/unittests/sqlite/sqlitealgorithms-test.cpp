// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <sqlitealgorithms.h>
#include <sqlitedatabase.h>
#include <sqlitetable.h>

#include <functional>

namespace {

class KeyValueView
{
public:
    KeyValueView(Utils::SmallStringView key, long long value)
        : key{key}
        , value{value}
    {}

public:
    Utils::SmallStringView key;
    long long value = 0;
};

class KeyValue
{
public:
    KeyValue(Utils::SmallStringView key, long long value)
        : key{key}
        , value{value}
    {}

    friend bool operator==(const KeyValue &first, const KeyValue &second)
    {
        return first.key == second.key && first.value == second.value;
    }

    friend bool operator==(KeyValueView first, const KeyValue &second)
    {
        return first.key == second.key && first.value == second.value;
    }

public:
    Utils::SmallString key;
    long long value = 0;
};

using KeyValues = std::vector<KeyValue>;

MATCHER_P2(IsKeyValue,
           key,
           value,
           std::string(negation ? "isn't " : "is ") + PrintToString(KeyValue{key, value}))
{
    return arg.key == key && arg.value == value;
}

class SqliteAlgorithms : public testing::Test
{
public:
    ~SqliteAlgorithms() noexcept { transaction.commit(); }

    class Initializer
    {
    public:
        Initializer(Sqlite::Database &database)
        {
            Sqlite::Table table;
            table.setName("data");
            table.setUseWithoutRowId(true);
            table.addColumn("key", Sqlite::ColumnType::None, {Sqlite::PrimaryKey{}});
            table.addColumn("value");

            table.initialize(database);
        }
    };

    auto select() { return selectViewsStatement.range<KeyValueView>(); }

    auto fetchKeyValues() { return selectValuesStatement.values<KeyValue>(); }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    Sqlite::ImmediateTransaction<Sqlite::Database> transaction{database};
    Initializer initializer{database};
    Sqlite::ReadStatement<2> selectViewsStatement{"SELECT key, value FROM data ORDER BY key",
                                                  database};
    Sqlite::ReadStatement<2> selectValuesStatement{"SELECT key, value FROM data ORDER BY key",
                                                   database};
    Sqlite::WriteStatement<2> insertStatement{"INSERT INTO data(key, value) VALUES (?1, ?2)",
                                              database};
    Sqlite::WriteStatement<2> updateStatement{"UPDATE data SET value = ?2 WHERE key=?1", database};
    Sqlite::WriteStatement<1> deleteStatement{"DELETE FROM data WHERE key=?", database};
    std::function<void(const KeyValue &keyValue)> insert{
        [&](const KeyValue &keyValue) { insertStatement.write(keyValue.key, keyValue.value); }};
    std::function<Sqlite::UpdateChange(KeyValueView keyValueView, const KeyValue &keyValue)> update{
        [&](KeyValueView keyValueView, const KeyValue &keyValue) {
            if (!(keyValueView == keyValue)) {
                updateStatement.write(keyValueView.key, keyValue.value);
                return Sqlite::UpdateChange::Update;
            }

            return Sqlite::UpdateChange::No;
        }};
    std::function<void(KeyValueView keyValueView)> remove{
        [&](KeyValueView keyValueView) { deleteStatement.write(keyValueView.key); }};
};

auto compareKey = [](KeyValueView keyValueView, const KeyValue &keyValue) {
    return Sqlite::compare(keyValueView.key, keyValue.key);
};

TEST_F(SqliteAlgorithms, insert_values)
{
    KeyValues keyValues = {{"one", 1}, {"oneone", 11}};

    Sqlite::insertUpdateDelete(select(), keyValues, compareKey, insert, update, remove);

    ASSERT_THAT(fetchKeyValues(), UnorderedElementsAre(KeyValue{"one", 1}, KeyValue{"oneone", 11}));
}

TEST_F(SqliteAlgorithms, insert_before_values)
{
    KeyValues keyValues = {{"two", 2}, {"twotwo", 22}};
    Sqlite::insertUpdateDelete(select(), keyValues, compareKey, insert, update, remove);
    KeyValues moreKeyValues = {{"one", 1}, {"oneone", 11}, {"two", 2}, {"twotwo", 22}};

    Sqlite::insertUpdateDelete(select(), moreKeyValues, compareKey, insert, update, remove);

    ASSERT_THAT(fetchKeyValues(),
                UnorderedElementsAre(KeyValue{"one", 1},
                                     KeyValue{"oneone", 11},
                                     KeyValue{"two", 2},
                                     KeyValue{"twotwo", 22}));
}

TEST_F(SqliteAlgorithms, insert_in_between_values)
{
    KeyValues keyValues = {{"one", 1}, {"two", 2}};
    Sqlite::insertUpdateDelete(select(), keyValues, compareKey, insert, update, remove);
    KeyValues moreKeyValues = {{"one", 1}, {"oneone", 11}, {"two", 2}, {"twotwo", 22}};

    Sqlite::insertUpdateDelete(select(), moreKeyValues, compareKey, insert, update, remove);

    ASSERT_THAT(fetchKeyValues(),
                UnorderedElementsAre(KeyValue{"one", 1},
                                     KeyValue{"oneone", 11},
                                     KeyValue{"two", 2},
                                     KeyValue{"twotwo", 22}));
}

TEST_F(SqliteAlgorithms, insert_trailing_values)
{
    KeyValues keyValues = {{"one", 1}, {"oneone", 11}};
    Sqlite::insertUpdateDelete(select(), keyValues, compareKey, insert, update, remove);
    KeyValues moreKeyValues = {{"one", 1}, {"oneone", 11}, {"two", 2}, {"twotwo", 22}};

    Sqlite::insertUpdateDelete(select(), moreKeyValues, compareKey, insert, update, remove);

    ASSERT_THAT(fetchKeyValues(),
                UnorderedElementsAre(KeyValue{"one", 1},
                                     KeyValue{"oneone", 11},
                                     KeyValue{"two", 2},
                                     KeyValue{"twotwo", 22}));
}

TEST_F(SqliteAlgorithms, update_values)
{
    KeyValues keyValues = {{"one", 1}, {"oneone", 11}};
    Sqlite::insertUpdateDelete(select(), keyValues, compareKey, insert, update, remove);
    KeyValues changedKeyValues = {{"one", 2}, {"oneone", 22}};

    Sqlite::insertUpdateDelete(select(), changedKeyValues, compareKey, insert, update, remove);

    ASSERT_THAT(fetchKeyValues(), UnorderedElementsAre(KeyValue{"one", 2}, KeyValue{"oneone", 22}));
}

TEST_F(SqliteAlgorithms, update_some_values)
{
    KeyValues keyValues = {{"one", 1}, {"oneone", 11}, {"two", 2}, {"twotwo", 22}};
    Sqlite::insertUpdateDelete(select(), keyValues, compareKey, insert, update, remove);
    KeyValues moreKeyValues = {{"one", 101}, {"oneone", 11}, {"two", 202}, {"twotwo", 22}};

    Sqlite::insertUpdateDelete(select(), moreKeyValues, compareKey, insert, update, remove);

    ASSERT_THAT(fetchKeyValues(),
                UnorderedElementsAre(KeyValue{"one", 101},
                                     KeyValue{"oneone", 11},
                                     KeyValue{"two", 202},
                                     KeyValue{"twotwo", 22}));
}

TEST_F(SqliteAlgorithms, delete_before_sqlite_entries)
{
    KeyValues keyValues = {{"one", 1}, {"oneone", 11}, {"two", 2}, {"twotwo", 22}};
    Sqlite::insertUpdateDelete(select(), keyValues, compareKey, insert, update, remove);
    KeyValues lessKeyValues = {{"two", 2}, {"twotwo", 22}};

    Sqlite::insertUpdateDelete(select(), lessKeyValues, compareKey, insert, update, remove);

    ASSERT_THAT(fetchKeyValues(), UnorderedElementsAre(KeyValue{"two", 2}, KeyValue{"twotwo", 22}));
}

TEST_F(SqliteAlgorithms, delete_trailing_sqlite_entries2)
{
    KeyValues keyValues = {{"one", 1}, {"oneone", 11}, {"two", 2}, {"twotwo", 22}};
    Sqlite::insertUpdateDelete(select(), keyValues, compareKey, insert, update, remove);
    KeyValues lessKeyValues = {{"one", 1}, {"oneone", 11}};

    Sqlite::insertUpdateDelete(select(), lessKeyValues, compareKey, insert, update, remove);

    ASSERT_THAT(fetchKeyValues(), UnorderedElementsAre(KeyValue{"one", 1}, KeyValue{"oneone", 11}));
}

TEST_F(SqliteAlgorithms, delete_trailing_sqlite_entries)
{
    KeyValues keyValues = {{"one", 1}, {"oneone", 11}, {"two", 2}, {"twotwo", 22}};
    Sqlite::insertUpdateDelete(select(), keyValues, compareKey, insert, update, remove);
    KeyValues lessKeyValues = {{"one", 1}, {"oneone", 11}};

    Sqlite::insertUpdateDelete(select(), lessKeyValues, compareKey, insert, update, remove);

    ASSERT_THAT(fetchKeyValues(), UnorderedElementsAre(KeyValue{"one", 1}, KeyValue{"oneone", 11}));
}

TEST_F(SqliteAlgorithms, delete_sqlite_entries)
{
    KeyValues keyValues = {{"one", 1}, {"oneone", 11}, {"two", 2}, {"twotwo", 22}};
    Sqlite::insertUpdateDelete(select(), keyValues, compareKey, insert, update, remove);
    KeyValues emptyKeyValues = {};

    Sqlite::insertUpdateDelete(select(), emptyKeyValues, compareKey, insert, update, remove);

    ASSERT_THAT(fetchKeyValues(), IsEmpty());
}

TEST_F(SqliteAlgorithms, synchonize)
{
    KeyValues keyValues = {{"one", 1}, {"oneone", 11}, {"two", 2}, {"twotwo", 22}};
    Sqlite::insertUpdateDelete(select(), keyValues, compareKey, insert, update, remove);
    KeyValues emptyKeyValues = {{"oneone", 11}, {"three", 3}, {"twotwo", 202}};

    Sqlite::insertUpdateDelete(select(), emptyKeyValues, compareKey, insert, update, remove);

    ASSERT_THAT(fetchKeyValues(),
                UnorderedElementsAre(KeyValue{"oneone", 11},
                                     KeyValue{"three", 3},
                                     KeyValue{"twotwo", 202}));
}

TEST_F(SqliteAlgorithms, compare_equal)
{
    auto compare = Sqlite::compare("one", "one");

    ASSERT_THAT(compare, Eq(0));
}

TEST_F(SqliteAlgorithms, compare_greater)
{
    auto compare = Sqlite::compare("two", "one");

    ASSERT_THAT(compare, Gt(0));
}

TEST_F(SqliteAlgorithms, compare_greater_for_trailing_text)
{
    auto compare = Sqlite::compare("oneone", "one");

    ASSERT_THAT(compare, Gt(0));
}

TEST_F(SqliteAlgorithms, compare_less_for_trailing_text)
{
    auto compare = Sqlite::compare("one", "oneone");

    ASSERT_THAT(compare, Lt(0));
}

} // namespace
