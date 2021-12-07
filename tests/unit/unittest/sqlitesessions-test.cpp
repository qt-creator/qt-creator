/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

    std::vector<Data> fetchData() { return selectData.values<Data>(8); }
    std::vector<Tag> fetchTags() { return selectTags.values<Tag>(8); }

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

TEST_F(SqliteSessions, DontThrowForCommittingWithoutSessionStart)
{
    ASSERT_NO_THROW(sessions.commit());
}

TEST_F(SqliteSessions, CreateEmptySession)
{
    sessions.create();
    sessions.commit();

    ASSERT_THAT(sessions.changeSets(), IsEmpty());
}

TEST_F(SqliteSessions, CreateSessionWithInsert)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();

    ASSERT_THAT(sessions.changeSets(), SizeIs(1));
}

TEST_F(SqliteSessions, CreateSessionWithUpdate)
{
    insertData.write("foo", 22, 3.14);

    sessions.create();
    updateNumber.write("foo", "bar");
    sessions.commit();

    ASSERT_THAT(sessions.changeSets(), SizeIs(1));
}

TEST_F(SqliteSessions, CreateSessionWithDelete)
{
    insertData.write("foo", 22, 3.14);

    sessions.create();
    deleteData.write("foo");
    sessions.commit();

    ASSERT_THAT(sessions.changeSets(), SizeIs(1));
}

TEST_F(SqliteSessions, CreateSessionWithInsertAndUpdate)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();

    sessions.create();
    updateNumber.write("foo", "bar");
    sessions.commit();

    ASSERT_THAT(sessions.changeSets(), SizeIs(2));
}

TEST_F(SqliteSessions, CreateSession)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);

    sessions.commit();

    ASSERT_THAT(sessions.changeSets(), SizeIs(1));
}

TEST_F(SqliteSessions, RevertSession)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();

    sessions.revert();

    ASSERT_THAT(fetchData(), IsEmpty());
}

TEST_F(SqliteSessions, RevertSessionToBase)
{
    insertData.write("bar", "foo", 99);
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();

    sessions.revert();

    ASSERT_THAT(fetchData(), ElementsAre(HasData("bar", "foo", 99)));
}

TEST_F(SqliteSessions, RevertMultipleSession)
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

TEST_F(SqliteSessions, ApplySession)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();

    sessions.apply();

    ASSERT_THAT(fetchData(), ElementsAre(HasData("foo", 22, 3.14)));
}

TEST_F(SqliteSessions, ApplySessionAfterAddingNewEntries)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();
    insertData.write("bar", "foo", 99);

    sessions.apply();

    ASSERT_THAT(fetchData(),
                UnorderedElementsAre(HasData("foo", 22, 3.14), HasData("bar", "foo", 99)));
}

TEST_F(SqliteSessions, ApplyOverridesEntriesWithUniqueConstraint)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();
    insertData.write("foo", "bar", 3.14);

    sessions.apply();

    ASSERT_THAT(fetchData(), ElementsAre(HasData("foo", 22, 3.14)));
}

TEST_F(SqliteSessions, ApplyDoesNotOverrideDeletedEntries)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();
    deleteData.write("foo");

    sessions.apply();

    ASSERT_THAT(fetchData(), IsEmpty());
}

TEST_F(SqliteSessions, ApplyDoesOnlyOverwriteUpdatedValues)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    updateValue.write("foo", 1234);
    sessions.commit();
    insertData.write("foo", "poo", 891);

    sessions.apply();

    ASSERT_THAT(fetchData(), ElementsAre(HasData("foo", "poo", 1234)));
}

TEST_F(SqliteSessions, ApplyDoesDoesNotOverrideForeignKeyIfReferenceIsDeleted)
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

TEST_F(SqliteSessions, ApplyDoesDoesNotOverrideIfConstraintsIsApplied)
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

TEST_F(SqliteSessions, ApplyDoesDoesNotOverrideForeignKeyIfReferenceIsDeletedDeferred)
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

TEST_F(SqliteSessions, EndSessionOnRollback)
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

TEST_F(SqliteSessions, EndSessionOnCommit)
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

TEST_F(SqliteSessions, DeleteSessions)
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

TEST_F(SqliteSessions, DeleteAllSessions)
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

TEST_F(SqliteSessions, ApplyAndUpdateSessions)
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

TEST_F(SqliteSessions, ApplyAndUpdateSessionsHasOnlyOneChangeSet)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    updateValue.write("foo", 99);
    sessions.commit();
    updateValue.write("foo", 99);

    sessions.applyAndUpdateSessions();

    ASSERT_THAT(sessions.changeSets(), SizeIs(1));
}

TEST_F(SqliteSessions, ForEmptySessionBeginEqualsEnd)
{
    auto changeSets = sessions.changeSets();

    auto begin = changeSets.begin();

    ASSERT_THAT(begin, Eq(changeSets.end()));
}

TEST_F(SqliteSessions, IteratorBeginUnequalsEndIfChangeSetHasContent)
{
    sessions.create();
    insertData.write("foo", "bar", 3.14);
    sessions.commit();
    auto changeSets = sessions.changeSets();
    auto &&changeSet = changeSets.front();

    auto begin = changeSet.begin();

    ASSERT_THAT(begin, Ne(changeSet.end()));
}

TEST_F(SqliteSessions, NextIteratorUnequalsBeginIfChangeSetHasContent)
{
    sessions.create();
    insertData.write("foo", "bar", 3.14);
    sessions.commit();
    auto changeSets = sessions.changeSets();
    auto &&changeSet = changeSets.front();

    auto next = std::next(changeSet.begin());

    ASSERT_NE(next, changeSet.begin());
}

TEST_F(SqliteSessions, NextIteratorEqualsEndIfChangeSetHasContent)
{
    sessions.create();
    insertData.write("foo", "bar", 3.14);
    sessions.commit();
    auto changeSets = sessions.changeSets();
    auto &&changeSet = changeSets.front();

    auto next = std::next(changeSet.begin());

    ASSERT_THAT(next, Eq(changeSet.end()));
}

TEST_F(SqliteSessions, NextIteratorNotUnqualsEndIfChangeSetHasContent)
{
    sessions.create();
    insertData.write("foo", "bar", 3.14);
    sessions.commit();
    auto changeSets = sessions.changeSets();
    auto &&changeSet = changeSets.front();

    auto next = std::next(changeSet.begin());

    ASSERT_THAT(next, Not(Ne(changeSet.end())));
}

TEST_F(SqliteSessions, BeginIteratorHasInsertOperation)
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

TEST_F(SqliteSessions, BeginIteratorHasUpdateOperation)
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

TEST_F(SqliteSessions, BeginIteratorHasDeleteOperation)
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

TEST_F(SqliteSessions, BeginIteratorHasDataTableName)
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

TEST_F(SqliteSessions, ConvertAllValueTypesInChangeSet)
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

TEST_F(SqliteSessions, InsertOneValueChangeSet)
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

TEST_F(SqliteSessions, UpdateOneValueChangeSet)
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

TEST_F(SqliteSessions, DeleteRowChangeSet)
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

TEST_F(SqliteSessions, EmptyChangeSet)
{
    sessions.create();
    sessions.commit();

    auto changeSets = sessions.changeSets();

    ASSERT_THAT(changeSets, ElementsAre());
}

TEST_F(SqliteSessions, AccessInsertOneValueChangeSet)
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

TEST_F(SqliteSessions, AccessUpdateOneValueChangeSet)
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

TEST_F(SqliteSessions, AccessDeleteRowChangeSet)
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
