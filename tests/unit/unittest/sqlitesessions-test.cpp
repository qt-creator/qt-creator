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

using Sqlite::SessionChangeSet;
using Sqlite::SessionChangeSets;

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

class Sessions : public testing::Test
{
protected:
    Sessions() { sessions.setAttachedTables({"data", "tags"}); }

    std::vector<Data> fetchData() { return selectData.values<Data, 3>(8); }
    std::vector<Tag> fetchTags() { return selectTags.values<Tag, 2>(8); }
    SessionChangeSets fetchChangeSets() { return selectChangeSets.values<SessionChangeSet>(8); }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    DatabaseExecute createTable{"CREATE TABLE data(id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT "
                                "UNIQUE, number NUMERIC, value NUMERIC)",
                                database};
    DatabaseExecute createTable2{"CREATE TABLE tags(id INTEGER PRIMARY KEY AUTOINCREMENT, dataId "
                                 "INTEGER NOT NULL REFERENCES data ON DELETE CASCADE DEFERRABLE "
                                 "INITIALLY DEFERRED, tag NUMERIC)",
                                 database};
    Sqlite::Sessions sessions{database, "main", "testsessions"};
    Sqlite::WriteStatement insertData{"INSERT INTO data(name, number, value) VALUES (?1, ?2, ?3) "
                                      "ON CONFLICT (name) DO UPDATE SET (number, value) = (?2, ?3)",
                                      database};
    Sqlite::WriteStatement updateNumber{"UPDATE data SET number = ?002 WHERE name=?001", database};
    Sqlite::WriteStatement updateValue{"UPDATE data SET value = ?002 WHERE name=?001", database};
    Sqlite::WriteStatement deleteData{"DELETE FROM data WHERE name=?", database};
    Sqlite::WriteStatement deleteTag{
        "DELETE FROM tags WHERE dataId=(SELECT id FROM data WHERE name=?)", database};
    Sqlite::WriteStatement insertTag{
        "INSERT INTO tags(dataId, tag) VALUES ((SELECT id FROM data WHERE name=?1), ?2) ", database};
    Sqlite::ReadStatement selectData{"SELECT name, number, value FROM data", database};
    Sqlite::ReadStatement selectTags{"SELECT name, tag FROM tags JOIN data ON data.id=tags.dataId",
                                     database};
    Sqlite::ReadStatement selectChangeSets{"SELECT changeset FROM testsessions", database};
};

TEST_F(Sessions, DontThrowForCommittingWithoutSessionStart)
{
    ASSERT_NO_THROW(sessions.commit());
}

TEST_F(Sessions, CreateEmptySession)
{
    sessions.create();
    sessions.commit();

    ASSERT_THAT(fetchChangeSets(), IsEmpty());
}

TEST_F(Sessions, CreateSessionWithInsert)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();

    ASSERT_THAT(fetchChangeSets(), SizeIs(1));
}

TEST_F(Sessions, CreateSessionWithUpdate)
{
    insertData.write("foo", 22, 3.14);

    sessions.create();
    updateNumber.write("foo", "bar");
    sessions.commit();

    ASSERT_THAT(fetchChangeSets(), SizeIs(1));
}

TEST_F(Sessions, CreateSessionWithDelete)
{
    insertData.write("foo", 22, 3.14);

    sessions.create();
    deleteData.write("foo");
    sessions.commit();

    ASSERT_THAT(fetchChangeSets(), SizeIs(1));
}

TEST_F(Sessions, CreateSessionWithInsertAndUpdate)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();

    sessions.create();
    updateNumber.write("foo", "bar");
    sessions.commit();

    ASSERT_THAT(fetchChangeSets(), SizeIs(2));
}

TEST_F(Sessions, CreateSession)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);

    sessions.commit();

    ASSERT_THAT(fetchChangeSets(), SizeIs(1));
}

TEST_F(Sessions, RevertSession)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();

    sessions.revert();

    ASSERT_THAT(fetchData(), IsEmpty());
}

TEST_F(Sessions, RevertSessionToBase)
{
    insertData.write("bar", "foo", 99);
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();

    sessions.revert();

    ASSERT_THAT(fetchData(), ElementsAre(HasData("bar", "foo", 99)));
}

TEST_F(Sessions, RevertMultipleSession)
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

TEST_F(Sessions, ApplySession)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();

    sessions.apply();

    ASSERT_THAT(fetchData(), ElementsAre(HasData("foo", 22, 3.14)));
}

TEST_F(Sessions, ApplySessionAfterAddingNewEntries)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();
    insertData.write("bar", "foo", 99);

    sessions.apply();

    ASSERT_THAT(fetchData(),
                UnorderedElementsAre(HasData("foo", 22, 3.14), HasData("bar", "foo", 99)));
}

TEST_F(Sessions, ApplyOverridesEntriesWithUniqueConstraint)
{
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();
    insertData.write("foo", "bar", 3.14);

    sessions.apply();

    ASSERT_THAT(fetchData(), ElementsAre(HasData("foo", 22, 3.14)));
}

TEST_F(Sessions, ApplyDoesNotOverrideDeletedEntries)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    insertData.write("foo", 22, 3.14);
    sessions.commit();
    deleteData.write("foo");

    sessions.apply();

    ASSERT_THAT(fetchData(), IsEmpty());
}

TEST_F(Sessions, ApplyDoesOnlyOverwriteUpdatedValues)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    updateValue.write("foo", 1234);
    sessions.commit();
    insertData.write("foo", "poo", 891);

    sessions.apply();

    ASSERT_THAT(fetchData(), ElementsAre(HasData("foo", "poo", 1234)));
}

TEST_F(Sessions, ApplyDoesDoesNotOverrideForeignKeyIfReferenceIsDeleted)
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

TEST_F(Sessions, ApplyDoesDoesNotOverrideIfConstraintsIsApplied)
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

TEST_F(Sessions, ApplyDoesDoesNotOverrideForeignKeyIfReferenceIsDeletedDeferred)
{
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
    ASSERT_THAT(fetchTags(), ElementsAre(HasTag("foo2", 4321)));
}

TEST_F(Sessions, EndSessionOnRollback)
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

TEST_F(Sessions, EndSessionOnCommit)
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

TEST_F(Sessions, DeleteSessions)
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

TEST_F(Sessions, DeleteAllSessions)
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

TEST_F(Sessions, ApplyAndUpdateSessions)
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

TEST_F(Sessions, ApplyAndUpdateSessionsHasOnlyOneChangeSet)
{
    insertData.write("foo", "bar", 3.14);
    sessions.create();
    updateValue.write("foo", 99);
    sessions.commit();
    updateValue.write("foo", 99);

    sessions.applyAndUpdateSessions();

    ASSERT_THAT(fetchChangeSets(), SizeIs(1));
}

} // namespace
