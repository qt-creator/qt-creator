// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitesessionchangeset.h>
#include <sqlitesessions.h>
#include <sqlitetransaction.h>
#include <sqlitewritestatement.h>

#include <ostream>

namespace {

using Sqlite::Operation;
using Sqlite::SessionChangeSet;
using Sqlite::SessionChangeSets;
using Sqlite::SessionChangeSetInternal::ValueViews;

class DatabaseExecute
{
public:
    DatabaseExecute(Utils::SmallStringView sqlStatement, Sqlite::Database &database)
    {
        database.execute(sqlStatement);
    }
};

class Data
{
public:
    Data(Sqlite::ValueView name, Sqlite::ValueView number, Sqlite::ValueView value)
        : name(name)
        , number(number)
        , value(value)
    {}

    Sqlite::Value name;
    Sqlite::Value number;
    Sqlite::Value value;
};

std::ostream &operator<<(std::ostream &out, const Data &data)
{
    return out << "(" << data.name << ", " << data.number << " " << data.value << ")";
}

MATCHER_P3(HasData,
           name,
           number,
           value,
           std::string(negation ? "hasn't " : "has ") + PrintToString(name) + ", "
               + PrintToString(number) + ", " + PrintToString(value))
{
    const Data &data = arg;

    return data.name == name && data.number == number && data.value == value;
}

MATCHER_P2(HasValues,
           newValue,
           oldValue,
           std::string(negation ? "hasn't " : "has ") + PrintToString(newValue) + ", "
               + PrintToString(oldValue))
{
    const ValueViews &values = arg;

    return values.newValue == newValue && values.oldValue == oldValue;
}

class Tag
{
public:
    Tag(Sqlite::ValueView name, Sqlite::ValueView tag)
        : name(name)
        , tag(tag)
    {}

    Sqlite::Value name;
    Sqlite::Value tag;
};

std::ostream &operator<<(std::ostream &out, const Tag &tag)
{
    return out << "(" << tag.name << ", " << tag.tag << ")";
}

MATCHER_P2(HasTag,
           name,
           tag,
           std::string(negation ? "hasn't " : "has ") + PrintToString(name) + ", " + PrintToString(tag))
{
    const Tag &t = arg;

    return t.name == name && t.tag == tag;
}

class SqliteSessions : public testing::Test
{
protected:
    SqliteSessions() { sessions.setAttachedTables({"data", "tags"}); }

    std::vector<Data> fetchData() { return selectData.values<Data>(); }

    std::vector<Tag> fetchTags() { return selectTags.values<Tag>(); }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    std::lock_guard<Sqlite::Database> lock{database};
    DatabaseExecute createTable{"CREATE TABLE data(id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT "
                                "UNIQUE, number NUMERIC, value NUMERIC)",
                                database};
    DatabaseExecute createTable2{"CREATE TABLE tags(id INTEGER PRIMARY KEY AUTOINCREMENT, dataId "
                                 "INTEGER NOT NULL REFERENCES data ON DELETE CASCADE DEFERRABLE "
                                 "INITIALLY DEFERRED, tag NUMERIC)",
                                 database};
    Sqlite::Sessions sessions{database, "main", "testsessions"};
    Sqlite::WriteStatement<3> insertData{
        "INSERT INTO data(name, number, value) VALUES (?1, ?2, ?3) "
        "ON CONFLICT (name) DO UPDATE SET (number, value) = (?2, ?3)",
        database};
    Sqlite::WriteStatement<2> insertOneDatum{"INSERT INTO data(value, name) VALUES (?1, ?2) "
                                             "ON CONFLICT (name) DO UPDATE SET (value) = (?2)",
                                             database};
    Sqlite::WriteStatement<2> updateNumber{"UPDATE data SET number = ?2 WHERE name=?1", database};
    Sqlite::WriteStatement<2> updateValue{"UPDATE data SET value = ?2 WHERE name=?1", database};
    Sqlite::WriteStatement<1> deleteData{"DELETE FROM data WHERE name=?", database};
    Sqlite::WriteStatement<1> deleteTag{
        "DELETE FROM tags WHERE dataId=(SELECT id FROM data WHERE name=?)", database};
    Sqlite::WriteStatement<2> insertTag{
        "INSERT INTO tags(dataId, tag) VALUES ((SELECT id FROM data WHERE name=?1), ?2) ", database};
    Sqlite::ReadStatement<3> selectData{"SELECT name, number, value FROM data", database};
    Sqlite::ReadStatement<2> selectTags{
        "SELECT name, tag FROM tags JOIN data ON data.id=tags.dataId", database};
    Sqlite::ReadStatement<1> selectChangeSets{"SELECT changeset FROM testsessions", database};
};

TEST_F(SqliteSessions, dont_throw_for_committing_without_session_start)
{
    ASSERT_NO_THROW(sessions.commit());
}

TEST_F(SqliteSessions, create_empty_session)
{
    sessions.create();
    sessions.commit();

    ASSERT_THAT(sessions.changeSets(), IsEmpty());
}

TEST_F(SqliteSessions, create_session_with_insert)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();

    ASSERT_THAT(sessions.changeSets(), SizeIs(1));
}

TEST_F(SqliteSessions, create_session_with_update)
{
    insertData.write("foo", 22, 3.14);

    sessions.create();
    updateNumber.write("foo", "bar");
    sessions.commit();

    ASSERT_THAT(sessions.changeSets(), SizeIs(1));
}

TEST_F(SqliteSessions, create_session_with_delete)
{
    insertData.write("foo", 22, 3.14);

    sessions.create();
    deleteData.write("foo");
    sessions.commit();

    ASSERT_THAT(sessions.changeSets(), SizeIs(1));
}

TEST_F(SqliteSessions, create_session_with_insert_and_update)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();

    sessions.create();
    updateNumber.write("foo", "bar");
    sessions.commit();

    ASSERT_THAT(sessions.changeSets(), SizeIs(2));
}

TEST_F(SqliteSessions, create_session)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);

    sessions.commit();

    ASSERT_THAT(sessions.changeSets(), SizeIs(1));
}

TEST_F(SqliteSessions, revert_session)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();

    sessions.revert();

    ASSERT_THAT(fetchData(), IsEmpty());
}

TEST_F(SqliteSessions, revert_session_to_base)
{
    insertData.write("bar", "foo", 99);
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();

    sessions.revert();

    ASSERT_THAT(fetchData(), ElementsAre(HasData("bar", "foo", 99)));
}

TEST_F(SqliteSessions, revert_multiple_session)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();
    sessions.create();
    updateNumber.write("foo", "bar");
    sessions.commit();

    sessions.revert();

    ASSERT_THAT(fetchData(), IsEmpty());
}

TEST_F(SqliteSessions, apply_session)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();

    sessions.apply();

    ASSERT_THAT(fetchData(), ElementsAre(HasData("foo", 22, 3.14)));
}

TEST_F(SqliteSessions, apply_session_after_adding_new_entries)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();
    insertData.write("bar", "foo", 99);

    sessions.apply();

    ASSERT_THAT(fetchData(),
                UnorderedElementsAre(HasData("foo", 22, 3.14), HasData("bar", "foo", 99)));
}

TEST_F(SqliteSessions, apply_overrides_entries_with_unique_constraint)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();
    insertData.write("foo", "bar", 3.14);

    sessions.apply();

    ASSERT_THAT(fetchData(), ElementsAre(HasData("foo", 22, 3.14)));
}

TEST_F(SqliteSessions, apply_does_not_override_deleted_entries)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();
    deleteData.write("foo");

    sessions.apply();

    ASSERT_THAT(fetchData(), IsEmpty());
}

TEST_F(SqliteSessions, apply_does_only_overwrite_updated_values)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    updateValue.write("foo", 1234);
    sessions.commit();
    insertData.write("foo", "poo", 891);

    sessions.apply();

    ASSERT_THAT(fetchData(), ElementsAre(HasData("foo", "poo", 1234)));
}

TEST_F(SqliteSessions, apply_does_does_not_override_foreign_key_if_reference_is_deleted)
{
    insertData.write("foo2", "bar", 3.14);
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    insertTag.write("foo2", 4321);
    insertTag.write("foo", 1234);
    sessions.commit();
    deleteData.write("foo");

    sessions.apply();

    ASSERT_THAT(fetchTags(), ElementsAre(HasTag("foo2", 4321)));
}

TEST_F(SqliteSessions, apply_does_does_not_override_if_constraints_is_applied)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    deleteData.write("foo");
    sessions.commit();
    sessions.revert();
    insertTag.write("foo", 1234);

    sessions.apply();

    ASSERT_THAT(fetchTags(), IsEmpty());
}

TEST_F(SqliteSessions, apply_does_does_not_override_foreign_key_if_reference_is_deleted_deferred)
{
    database.unlock();
    Sqlite::DeferredTransaction transaction{database};
    insertData.write("foo2", "bar", 3.14);
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    insertTag.write("foo2", 4321);
    insertTag.write("foo", 1234);
    sessions.commit();
    deleteData.write("foo");

    sessions.apply();

    transaction.commit();
    database.lock();
    ASSERT_THAT(fetchTags(), ElementsAre(HasTag("foo2", 4321)));
}

TEST_F(SqliteSessions, end_session_on_rollback)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    updateValue.write("foo", 99);
    sessions.rollback();
    sessions.commit();
    sessions.create();
    updateNumber.write("foo", 333);
    sessions.commit();
    updateValue.write("foo", 666);

    sessions.apply();

    ASSERT_THAT(fetchData(), ElementsAre(HasData("foo", 333, 666)));
}

TEST_F(SqliteSessions, end_session_on_commit)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    updateValue.write("foo", 99);
    sessions.commit();
    updateValue.write("foo", 666);
    sessions.commit();

    sessions.apply();

    ASSERT_THAT(fetchData(), ElementsAre(HasData("foo", "bar", 99)));
}

TEST_F(SqliteSessions, delete_sessions)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    updateValue.write("foo", 99);
    sessions.commit();
    sessions.revert();

    sessions.deleteAll();

    sessions.apply();
    ASSERT_THAT(fetchData(), ElementsAre(HasData("foo", "bar", 3.14)));
}

TEST_F(SqliteSessions, delete_all_sessions)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    updateValue.write("foo", 99);
    sessions.commit();
    sessions.revert();

    sessions.deleteAll();

    sessions.apply();
    ASSERT_THAT(fetchData(), ElementsAre(HasData("foo", "bar", 3.14)));
}

TEST_F(SqliteSessions, apply_and_update_sessions)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    updateValue.write("foo", 99);
    sessions.commit();
    updateValue.write("foo", 99);

    sessions.applyAndUpdateSessions();

    updateValue.write("foo", 22);
    sessions.apply();
    ASSERT_THAT(fetchData(), ElementsAre(HasData("foo", "bar", 22)));
}

TEST_F(SqliteSessions, apply_and_update_sessions_has_only_one_change_set)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    updateValue.write("foo", 99);
    sessions.commit();
    updateValue.write("foo", 99);

    sessions.applyAndUpdateSessions();

    ASSERT_THAT(sessions.changeSets(), SizeIs(1));
}

TEST_F(SqliteSessions, for_empty_session_begin_equals_end)
{
    auto changeSets = sessions.changeSets();

    auto begin = changeSets.begin();

    ASSERT_THAT(begin, Eq(changeSets.end()));
}

TEST_F(SqliteSessions, iterator_begin_unequals_end_if_change_set_has_content)
{
    sessions.create();
    insertData.write("foo", "bar", 3.14);
    sessions.commit();
    auto changeSets = sessions.changeSets();
    auto &&changeSet = changeSets.front();

    auto begin = changeSet.begin();

    ASSERT_THAT(begin, Ne(changeSet.end()));
}

TEST_F(SqliteSessions, next_iterator_unequals_begin_if_change_set_has_content)
{
    sessions.create();
    insertData.write("foo", "bar", 3.14);
    sessions.commit();
    auto changeSets = sessions.changeSets();
    auto &&changeSet = changeSets.front();

    auto next = std::next(changeSet.begin());

    ASSERT_NE(next, changeSet.begin());
}

TEST_F(SqliteSessions, next_iterator_equals_end_if_change_set_has_content)
{
    sessions.create();
    insertData.write("foo", "bar", 3.14);
    sessions.commit();
    auto changeSets = sessions.changeSets();
    auto &&changeSet = changeSets.front();

    auto next = std::next(changeSet.begin());

    ASSERT_THAT(next, Eq(changeSet.end()));
}

TEST_F(SqliteSessions, next_iterator_not_unquals_end_if_change_set_has_content)
{
    sessions.create();
    insertData.write("foo", "bar", 3.14);
    sessions.commit();
    auto changeSets = sessions.changeSets();
    auto &&changeSet = changeSets.front();

    auto next = std::next(changeSet.begin());

    ASSERT_THAT(next, Not(Ne(changeSet.end())));
}

TEST_F(SqliteSessions, begin_iterator_has_insert_operation)
{
    sessions.create();
    insertData.write("foo", "bar", 3.14);
    sessions.commit();
    auto changeSets = sessions.changeSets();
    auto &&changeSet = changeSets.front();
    auto begin = changeSet.begin();

    auto tuple = (*begin);

    ASSERT_THAT(tuple.operation, Eq(Operation::Insert));
}

TEST_F(SqliteSessions, begin_iterator_has_update_operation)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    updateValue.write("foo", 99);
    sessions.commit();
    auto changeSets = sessions.changeSets();
    auto &&changeSet = changeSets.front();
    auto begin = changeSet.begin();

    auto tuple = (*begin);

    ASSERT_THAT(tuple.operation, Eq(Operation::Update));
}

TEST_F(SqliteSessions, begin_iterator_has_delete_operation)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    deleteData.write("foo");
    sessions.commit();
    auto changeSets = sessions.changeSets();
    auto &&changeSet = changeSets.front();
    auto begin = changeSet.begin();

    auto tuple = (*begin);

    ASSERT_THAT(tuple.operation, Eq(Operation::Delete));
}

TEST_F(SqliteSessions, begin_iterator_has_data_table_name)
{
    sessions.create();
    insertData.write("foo", "bar", 3.14);
    sessions.commit();
    auto changeSets = sessions.changeSets();
    auto &&changeSet = changeSets.front();
    auto begin = changeSet.begin();

    auto tuple = (*begin);

    ASSERT_THAT(tuple.table, Eq("data"));
}

TEST_F(SqliteSessions, convert_all_value_types_in_change_set)
{
    sessions.create();
    insertData.write("foo", "bar", 3.14);
    sessions.commit();
    auto changeSets = sessions.changeSets();
    auto &&changeSet = changeSets.front();
    auto begin = changeSet.begin();
    auto tuple = (*begin);

    std::vector<ValueViews> values{tuple.begin(), tuple.end()};

    ASSERT_THAT(values,
                ElementsAre(HasValues(1, nullptr),
                            HasValues("foo", nullptr),
                            HasValues("bar", nullptr),
                            HasValues(3.14, nullptr)));
}

TEST_F(SqliteSessions, insert_one_value_change_set)
{
    sessions.create();
    insertOneDatum.write("foo", Sqlite::NullValue{});
    sessions.commit();
    auto changeSets = sessions.changeSets();
    auto &&changeSet = changeSets.front();
    auto begin = changeSet.begin();
    auto tuple = (*begin);

    std::vector<ValueViews> values{tuple.begin(), tuple.end()};

    ASSERT_THAT(values,
                ElementsAre(HasValues(1, nullptr),
                            HasValues(nullptr, nullptr),
                            HasValues(nullptr, nullptr),
                            HasValues("foo", nullptr)));
}

TEST_F(SqliteSessions, update_one_value_change_set)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    updateValue.write("foo", 99);
    sessions.commit();
    auto changeSets = sessions.changeSets();
    auto &&changeSet = changeSets.front();
    auto begin = changeSet.begin();
    auto tuple = (*begin);

    std::vector<ValueViews> values{tuple.begin(), tuple.end()};

    ASSERT_THAT(values,
                ElementsAre(HasValues(nullptr, 1),
                            HasValues(nullptr, nullptr),
                            HasValues(nullptr, nullptr),
                            HasValues(99, 3.14)));
}

TEST_F(SqliteSessions, delete_row_change_set)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    deleteData.write("foo");
    sessions.commit();
    auto changeSets = sessions.changeSets();
    auto &&changeSet = changeSets.front();
    auto begin = changeSet.begin();

    auto tuple = (*begin);

    std::vector<ValueViews> values{tuple.begin(), tuple.end()};

    ASSERT_THAT(values,
                ElementsAre(HasValues(nullptr, 1),
                            HasValues(nullptr, "foo"),
                            HasValues(nullptr, "bar"),
                            HasValues(nullptr, 3.14)));
}

TEST_F(SqliteSessions, empty_change_set)
{
    sessions.create();
    sessions.commit();

    auto changeSets = sessions.changeSets();

    ASSERT_THAT(changeSets, ElementsAre());
}

TEST_F(SqliteSessions, access_insert_one_value_change_set)
{
    sessions.create();
    insertOneDatum.write("foo", Sqlite::NullValue{});
    sessions.commit();
    auto changeSets = sessions.changeSets();
    auto &&changeSet = changeSets.front();
    auto begin = changeSet.begin();
    auto tuple = (*begin);

    ValueViews value = tuple[3];

    ASSERT_THAT(value, HasValues("foo", nullptr));
}

TEST_F(SqliteSessions, access_update_one_value_change_set)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    updateValue.write("foo", 99);
    sessions.commit();
    auto changeSets = sessions.changeSets();
    auto &&changeSet = changeSets.front();
    auto begin = changeSet.begin();
    auto tuple = (*begin);

    ValueViews value = tuple[3];

    ASSERT_THAT(value, HasValues(99, 3.14));
}

TEST_F(SqliteSessions, access_delete_row_change_set)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    deleteData.write("foo");
    sessions.commit();
    auto changeSets = sessions.changeSets();
    auto &&changeSet = changeSets.front();
    auto begin = changeSet.begin();
    auto tuple = (*begin);

    ValueViews value = tuple[0];

    ASSERT_THAT(value, HasValues(nullptr, 1));
}
} // namespace
