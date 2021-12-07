/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "googletest.h"

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

std::ostream &operator<<(std::ostream &out, KeyValueView keyValueView)
{
    return out << "(" << keyValueView.key << ", " << keyValueView.value << ")";
}

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

std::ostream &operator<<(std::ostream &out, KeyValue keyValue)
{
    return out << "(" << keyValue.key << ", " << keyValue.value << ")";
}

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

    auto fetchKeyValues() { return selectValuesStatement.values<KeyValue>(24); }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    Sqlite::ImmediateTransaction transaction{database};
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

TEST_F(SqliteAlgorithms, InsertValues)
{
    KeyValues keyValues = {{"one", 1}, {"oneone", 11}};

    Sqlite::insertUpdateDelete(select(), keyValues, compareKey, insert, update, remove);

    ASSERT_THAT(fetchKeyValues(), UnorderedElementsAre(KeyValue{"one", 1}, KeyValue{"oneone", 11}));
}

TEST_F(SqliteAlgorithms, InsertBeforeValues)
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

TEST_F(SqliteAlgorithms, InsertInBetweenValues)
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

TEST_F(SqliteAlgorithms, InsertTrailingValues)
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

TEST_F(SqliteAlgorithms, UpdateValues)
{
    KeyValues keyValues = {{"one", 1}, {"oneone", 11}};
    Sqlite::insertUpdateDelete(select(), keyValues, compareKey, insert, update, remove);
    KeyValues changedKeyValues = {{"one", 2}, {"oneone", 22}};

    Sqlite::insertUpdateDelete(select(), changedKeyValues, compareKey, insert, update, remove);

    ASSERT_THAT(fetchKeyValues(), UnorderedElementsAre(KeyValue{"one", 2}, KeyValue{"oneone", 22}));
}

TEST_F(SqliteAlgorithms, UpdateSomeValues)
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

TEST_F(SqliteAlgorithms, DeleteBeforeSqliteEntries)
{
    KeyValues keyValues = {{"one", 1}, {"oneone", 11}, {"two", 2}, {"twotwo", 22}};
    Sqlite::insertUpdateDelete(select(), keyValues, compareKey, insert, update, remove);
    KeyValues lessKeyValues = {{"two", 2}, {"twotwo", 22}};

    Sqlite::insertUpdateDelete(select(), lessKeyValues, compareKey, insert, update, remove);

    ASSERT_THAT(fetchKeyValues(), UnorderedElementsAre(KeyValue{"two", 2}, KeyValue{"twotwo", 22}));
}

TEST_F(SqliteAlgorithms, DeleteTrailingSqliteEntries2)
{
    KeyValues keyValues = {{"one", 1}, {"oneone", 11}, {"two", 2}, {"twotwo", 22}};
    Sqlite::insertUpdateDelete(select(), keyValues, compareKey, insert, update, remove);
    KeyValues lessKeyValues = {{"one", 1}, {"oneone", 11}};

    Sqlite::insertUpdateDelete(select(), lessKeyValues, compareKey, insert, update, remove);

    ASSERT_THAT(fetchKeyValues(), UnorderedElementsAre(KeyValue{"one", 1}, KeyValue{"oneone", 11}));
}

TEST_F(SqliteAlgorithms, DeleteTrailingSqliteEntries)
{
    KeyValues keyValues = {{"one", 1}, {"oneone", 11}, {"two", 2}, {"twotwo", 22}};
    Sqlite::insertUpdateDelete(select(), keyValues, compareKey, insert, update, remove);
    KeyValues lessKeyValues = {{"one", 1}, {"oneone", 11}};

    Sqlite::insertUpdateDelete(select(), lessKeyValues, compareKey, insert, update, remove);

    ASSERT_THAT(fetchKeyValues(), UnorderedElementsAre(KeyValue{"one", 1}, KeyValue{"oneone", 11}));
}

TEST_F(SqliteAlgorithms, DeleteSqliteEntries)
{
    KeyValues keyValues = {{"one", 1}, {"oneone", 11}, {"two", 2}, {"twotwo", 22}};
    Sqlite::insertUpdateDelete(select(), keyValues, compareKey, insert, update, remove);
    KeyValues emptyKeyValues = {};

    Sqlite::insertUpdateDelete(select(), emptyKeyValues, compareKey, insert, update, remove);

    ASSERT_THAT(fetchKeyValues(), IsEmpty());
}

TEST_F(SqliteAlgorithms, Synchonize)
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

TEST_F(SqliteAlgorithms, CompareEqual)
{
    auto compare = Sqlite::compare("one", "one");

    ASSERT_THAT(compare, Eq(0));
}

TEST_F(SqliteAlgorithms, CompareGreater)
{
    auto compare = Sqlite::compare("two", "one");

    ASSERT_THAT(compare, Gt(0));
}

TEST_F(SqliteAlgorithms, CompareGreaterForTrailingText)
{
    auto compare = Sqlite::compare("oneone", "one");

    ASSERT_THAT(compare, Gt(0));
}

TEST_F(SqliteAlgorithms, CompareLessForTrailingText)
{
    auto compare = Sqlite::compare("one", "oneone");

    ASSERT_THAT(compare, Lt(0));
}

} // namespace
