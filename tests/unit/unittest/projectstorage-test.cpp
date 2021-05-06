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

#include "sqlitedatabasemock.h"

#include <modelnode.h>
#include <projectstorage/projectstorage.h>
#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitewritestatement.h>

namespace {

using QmlDesigner::PropertyDeclarationId;
using QmlDesigner::SourceContextId;
using QmlDesigner::SourceId;
using QmlDesigner::TypeAccessSemantics;
using QmlDesigner::TypeId;
using QmlDesigner::Cache::Source;
using QmlDesigner::Cache::SourceContext;

MATCHER_P2(IsSourceContext,
           id,
           value,
           std::string(negation ? "isn't " : "is ") + PrintToString(SourceContext{value, id}))
{
    const SourceContext &sourceContext = arg;

    return sourceContext.id == id && sourceContext.value == value;
}

MATCHER_P2(IsSourceNameAndSourceContextId,
           name,
           id,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(QmlDesigner::Cache::SourceNameAndSourceContextId{name, id}))
{
    const QmlDesigner::Cache::SourceNameAndSourceContextId &sourceNameAndSourceContextId = arg;

    return sourceNameAndSourceContextId.sourceName == name
           && sourceNameAndSourceContextId.sourceContextId == id;
}

class ProjectStorage : public testing::Test
{
public:
    ProjectStorage()
    {
        ON_CALL(selectSourceContextIdFromSourceContextsBySourceContextPathStatement,
                valueReturnsSourceContextId(A<Utils::SmallStringView>()))
            .WillByDefault(Return(SourceContextId()));
        ON_CALL(selectSourceContextIdFromSourceContextsBySourceContextPathStatement,
                valueReturnsSourceContextId(Utils::SmallStringView("")))
            .WillByDefault(Return(SourceContextId(0)));
        ON_CALL(selectSourceContextIdFromSourceContextsBySourceContextPathStatement,
                valueReturnsSourceContextId(Utils::SmallStringView("/path/to")))
            .WillByDefault(Return(SourceContextId(5)));
        ON_CALL(databaseMock, lastInsertedRowId()).WillByDefault(Return(12));
        ON_CALL(selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatment,
                valueReturnInt32(_, _))
            .WillByDefault(Return(Utils::optional<int>()));
        ON_CALL(selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatment,
                valueReturnInt32(0, Utils::SmallStringView("")))
            .WillByDefault(Return(Utils::optional<int>(0)));
        ON_CALL(selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatment,
                valueReturnInt32(5, Utils::SmallStringView("file.h")))
            .WillByDefault(Return(Utils::optional<int>(42)));
        ON_CALL(selectAllSourcesStatement, valuesReturnCacheSources(_))
            .WillByDefault(Return(std::vector<Source>{{"file.h", SourceContextId{1}, SourceId{1}},
                                                      {"file.cpp", SourceContextId{2}, SourceId{4}}}));
        ON_CALL(selectSourceContextPathFromSourceContextsBySourceContextIdStatement,
                valueReturnPathString(5))
            .WillByDefault(Return(Utils::optional<Utils::PathString>("/path/to")));
        ON_CALL(selectSourceNameAndSourceContextIdFromSourcesBySourceIdStatement,
                valueReturnCacheSourceNameAndSourceContextId(42))
            .WillByDefault(Return(QmlDesigner::Cache::SourceNameAndSourceContextId{"file.cpp", 5}));
        ON_CALL(selectSourceContextIdFromSourcesBySourceIdStatement,
                valueReturnInt32(TypedEq<int>(42)))
            .WillByDefault(Return(Utils::optional<int>(5)));
    }

protected:
    using Storage = QmlDesigner::ProjectStorage<SqliteDatabaseMock>;
    template<int ResultCount>
    using ReadStatement = Storage::ReadStatement<ResultCount>;
    using WriteStatement = Storage::WriteStatement;
    template<int ResultCount>
    using ReadWriteStatement = SqliteDatabaseMock::ReadWriteStatement<ResultCount>;

    NiceMock<SqliteDatabaseMock> databaseMock;
    Storage storage{databaseMock, true};
    ReadWriteStatement<1> &upsertTypeStatement = storage.upsertTypeStatement;
    ReadStatement<1> &selectTypeIdByQualifiedNameStatement = storage.selectTypeIdByQualifiedNameStatement;
    ReadWriteStatement<1> &upsertPropertyDeclarationStatement = storage.upsertPropertyDeclarationStatement;
    ReadStatement<1> &selectPropertyDeclarationByTypeIdAndNameStatement = storage.selectPropertyDeclarationByTypeIdAndNameStatement;
    WriteStatement &upsertQualifiedTypeNameStatement = storage.upsertQualifiedTypeNameStatement;
    ReadStatement<1> &selectSourceContextIdFromSourceContextsBySourceContextPathStatement
        = storage.selectSourceContextIdFromSourceContextsBySourceContextPathStatement;
    ReadStatement<1> &selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatment
        = storage.selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatement;
    ReadStatement<1> &selectSourceContextPathFromSourceContextsBySourceContextIdStatement
        = storage.selectSourceContextPathFromSourceContextsBySourceContextIdStatement;
    ReadStatement<2> &selectSourceNameAndSourceContextIdFromSourcesBySourceIdStatement
        = storage.selectSourceNameAndSourceContextIdFromSourcesBySourceIdStatement;
    ReadStatement<2> &selectAllSourceContextsStatement = storage.selectAllSourceContextsStatement;
    WriteStatement &insertIntoSourceContexts = storage.insertIntoSourceContextsStatement;
    WriteStatement &insertIntoSourcesStatement = storage.insertIntoSourcesStatement;
    ReadStatement<3> &selectAllSourcesStatement = storage.selectAllSourcesStatement;
    ReadStatement<1> &selectSourceContextIdFromSourcesBySourceIdStatement = storage.selectSourceContextIdFromSourcesBySourceIdStatement;
};

TEST_F(ProjectStorage, InsertTypeCalls)
{
    InSequence s;
    TypeId prototypeId{3};
    TypeId newTypeId{11};

    EXPECT_CALL(databaseMock, immediateBegin());
    EXPECT_CALL(upsertTypeStatement,
                valueReturnsTypeId(Eq("QObject"),
                                   TypedEq<long long>(
                                       static_cast<long long>(TypeAccessSemantics::Reference)),
                                   Eq(prototypeId.id)))
        .WillOnce(Return(newTypeId));
    EXPECT_CALL(upsertQualifiedTypeNameStatement,
                write(TypedEq<Utils::SmallStringView>("Qml.Object"), TypedEq<long long>(newTypeId.id)));
    EXPECT_CALL(upsertQualifiedTypeNameStatement,
                write(TypedEq<Utils::SmallStringView>("Quick.Object"),
                      TypedEq<long long>(newTypeId.id)));
    EXPECT_CALL(databaseMock, commit);

    storage.upsertType("QObject",
                       prototypeId,
                       TypeAccessSemantics::Reference,
                       std::vector<Utils::SmallString>{"Qml.Object", "Quick.Object"});
}

TEST_F(ProjectStorage, InsertTypeReturnsInternalPropertyId)
{
    TypeId prototypeId{3};
    TypeId newTypeId{11};
    ON_CALL(upsertTypeStatement, valueReturnsTypeId(Eq("QObject"), _, Eq(prototypeId.id)))
        .WillByDefault(Return(newTypeId));

    auto internalId = storage.upsertType("QObject",
                                         prototypeId,
                                         TypeAccessSemantics::Reference,
                                         std::vector<Utils::SmallString>{});

    ASSERT_THAT(internalId, Eq(newTypeId));
}

TEST_F(ProjectStorage, UpsertPropertyDeclaration)
{
    InSequence s;

    EXPECT_CALL(databaseMock, immediateBegin());
    EXPECT_CALL(upsertPropertyDeclarationStatement,
                valueReturnsPropertyDeclarationId(TypedEq<long long>(11),
                                                  TypedEq<Utils::SmallStringView>("boo"),
                                                  TypedEq<long long>(33)))
        .WillOnce(Return(PropertyDeclarationId{3}));
    EXPECT_CALL(databaseMock, commit);

    storage.upsertPropertyDeclaration(TypeId{11}, "boo", TypeId{33});
}

/*
TEST_F(ProjectStorage, ReadSourceContextIdForNotContainedPath)
{
    auto sourceContextId = storage.readSourceContextId("/some/not/known/path");

    ASSERT_FALSE(sourceContextId);
}

TEST_F(ProjectStorage, ReadSourceIdForNotContainedPathAndSourceContextId)
{
    auto sourceId = storage.readSourceId(23, "/some/not/known/path");

    ASSERT_FALSE(sourceId);
}

TEST_F(ProjectStorage, ReadSourceContextIdForEmptyPath)
{
    auto sourceContextId = storage.readSourceContextId("");

    ASSERT_THAT(sourceContextId.value(), 0);
}

TEST_F(ProjectStorage, ReadSourceIdForEmptyNameAndZeroSourceContextId)
{
    auto sourceId = storage.readSourceId(0, "");

    ASSERT_THAT(sourceId.value(), 0);
}

TEST_F(ProjectStorage, ReadSourceContextIdForPath)
{
    auto sourceContextId = storage.readSourceContextId("/path/to");

    ASSERT_THAT(sourceContextId.value(), 5);
}

TEST_F(ProjectStorage, ReadSourceIdForPathAndSourceContextId)
{
    auto sourceId = storage.readSourceId(5, "file.h");

    ASSERT_THAT(sourceId.value(), 42);
}

TEST_F(ProjectStorage, FetchSourceContextIdForEmptyPath)
{
    auto sourceContextId = storage.fetchSourceContextId("");

    ASSERT_THAT(sourceContextId, 0);
}

TEST_F(ProjectStorage, FetchSourceIdForEmptyNameAndZeroSourceContextId)
{
    auto sourceId = storage.fetchSourceId(0, "");

    ASSERT_THAT(sourceId, 0);
}

TEST_F(ProjectStorage, FetchSourceContextIdForPath)
{
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    ASSERT_THAT(sourceContextId, 5);
}

TEST_F(ProjectStorage, FetchSourceIdForPathAndSourceContextId)
{
    auto sourceId = storage.fetchSourceId(5, "file.h");

    ASSERT_THAT(sourceId, 42);
}

TEST_F(ProjectStorage, CallWriteForWriteDirectory)
{
    EXPECT_CALL(insertIntoSourceContexts, write(TypedEq<Utils::SmallStringView>("/some/not/known/path")));

    storage.writeSourceContextId("/some/not/known/path");
}

TEST_F(ProjectStorage, CallWriteForWriteSource)
{
    EXPECT_CALL(insertIntoSources,
                write(TypedEq<int>(5), TypedEq<Utils::SmallStringView>("unknownfile.h")));

    storage.writeSourceId(5, "unknownfile.h");
}

TEST_F(ProjectStorage, GetTheSourceContextIdBackAfterWritingANewEntryInSourceContexts)
{
    auto sourceContextId = storage.writeSourceContextId("/some/not/known/path");

    ASSERT_THAT(sourceContextId, 12);
}

TEST_F(ProjectStorage, GetTheSourceIdBackAfterWritingANewEntryInSources)
{
    auto sourceId = storage.writeSourceId(5, "unknownfile.h");

    ASSERT_THAT(sourceId, 12);
}

TEST_F(ProjectStorage, GetTheSourceContextIdBackAfterFetchingANewEntryFromSourceContexts)
{
    auto sourceContextId = storage.fetchSourceContextId("/some/not/known/path");

    ASSERT_THAT(sourceContextId, 12);
}

TEST_F(ProjectStorage, GetTheSourceIdBackAfterFetchingANewEntryFromSources)
{
    auto sourceId = storage.fetchSourceId(5, "unknownfile.h");

    ASSERT_THAT(sourceId, 12);
}

*/

TEST_F(ProjectStorage, SelectForFetchingSourceContextIdForKnownPathCalls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectSourceContextIdFromSourceContextsBySourceContextPathStatement,
                valueReturnsSourceContextId(TypedEq<Utils::SmallStringView>("/path/to")));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchSourceContextId("/path/to");
}

TEST_F(ProjectStorage, SelectForFetchingSourceIdForKnownPathCalls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatment,
                valueReturnsSourceId(5, Eq("file.h")));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchSourceId(SourceContextId{5}, "file.h");
}

TEST_F(ProjectStorage, NotWriteForFetchingSourceContextIdForKnownPathCalls)
{
    EXPECT_CALL(insertIntoSourceContexts, write(An<Utils::SmallStringView>())).Times(0);

    storage.fetchSourceContextId("/path/to");
}

TEST_F(ProjectStorage, NotWriteForFetchingSoureIdForKnownEntryCalls)
{
    EXPECT_CALL(insertIntoSourcesStatement, write(An<uint>(), An<Utils::SmallStringView>())).Times(0);

    storage.fetchSourceId(SourceContextId{5}, "file.h");
}

TEST_F(ProjectStorage, SelectAndWriteForFetchingSourceContextIdForUnknownPathCalls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectSourceContextIdFromSourceContextsBySourceContextPathStatement,
                valueReturnsSourceContextId(
                    TypedEq<Utils::SmallStringView>("/some/not/known/path")));
    EXPECT_CALL(insertIntoSourceContexts,
                write(TypedEq<Utils::SmallStringView>("/some/not/known/path")));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchSourceContextId("/some/not/known/path");
}

TEST_F(ProjectStorage, SelectAndWriteForFetchingSourceIdForUnknownEntryCalls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatment,
                valueReturnsSourceId(5, Eq("unknownfile.h")));
    EXPECT_CALL(insertIntoSourcesStatement,
                write(TypedEq<int>(5), TypedEq<Utils::SmallStringView>("unknownfile.h")));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchSourceId(SourceContextId{5}, "unknownfile.h");
}

TEST_F(ProjectStorage, ValueForFetchSourceContextForIdCalls)
{
    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectSourceContextPathFromSourceContextsBySourceContextIdStatement,
                valueReturnPathString(5));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchSourceContextPath(SourceContextId{5});
}

TEST_F(ProjectStorage, FetchSourceContextForId)
{
    auto path = storage.fetchSourceContextPath(SourceContextId{5});

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(ProjectStorage, ThrowAsFetchingSourceContextPathForNonExistingId)
{
    ASSERT_THROW(storage.fetchSourceContextPath(SourceContextId{12}),
                 QmlDesigner::SourceContextIdDoesNotExists);
}

TEST_F(ProjectStorage, FetchSourceContextIdForUnknownSourceId)
{
    ASSERT_THROW(storage.fetchSourceContextId(SourceId{1111}), QmlDesigner::SourceIdDoesNotExists);
}

TEST_F(ProjectStorage, FetchSourceContextIdThrows)
{
    ASSERT_THROW(storage.fetchSourceContextId(SourceId{41}), QmlDesigner::SourceIdDoesNotExists);
}

TEST_F(ProjectStorage, GetTheSourceContextIdBackAfterFetchingANewEntryFromSourceContextsUnguarded)
{
    auto sourceContextId = storage.fetchSourceContextIdUnguarded("/some/not/known/path");

    ASSERT_THAT(sourceContextId, SourceContextId{12});
}

TEST_F(ProjectStorage, GetTheSourceIdBackAfterFetchingANewEntryFromSourcesUnguarded)
{
    auto sourceId = storage.fetchSourceIdUnguarded(SourceContextId{5}, "unknownfile.h");

    ASSERT_THAT(sourceId, SourceId{12});
}

TEST_F(ProjectStorage, SelectForFetchingSourceContextIdForKnownPathUnguardedCalls)
{
    InSequence s;

    EXPECT_CALL(selectSourceContextIdFromSourceContextsBySourceContextPathStatement,
                valueReturnsSourceContextId(TypedEq<Utils::SmallStringView>("/path/to")));

    storage.fetchSourceContextIdUnguarded("/path/to");
}

TEST_F(ProjectStorage, SelectForFetchingSourceIdForKnownPathUnguardedCalls)
{
    EXPECT_CALL(selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatment,
                valueReturnsSourceId(5, Eq("file.h")));

    storage.fetchSourceIdUnguarded(SourceContextId{5}, "file.h");
}

TEST_F(ProjectStorage, NotWriteForFetchingSourceContextIdForKnownPathUnguardedCalls)
{
    EXPECT_CALL(insertIntoSourceContexts, write(An<Utils::SmallStringView>())).Times(0);

    storage.fetchSourceContextIdUnguarded("/path/to");
}

TEST_F(ProjectStorage, NotWriteForFetchingSoureIdForKnownEntryUnguardedCalls)
{
    EXPECT_CALL(insertIntoSourcesStatement, write(An<uint>(), An<Utils::SmallStringView>())).Times(0);

    storage.fetchSourceIdUnguarded(SourceContextId{5}, "file.h");
}

TEST_F(ProjectStorage, SelectAndWriteForFetchingSourceContextIdForUnknownPathUnguardedCalls)
{
    InSequence s;

    EXPECT_CALL(selectSourceContextIdFromSourceContextsBySourceContextPathStatement,
                valueReturnsSourceContextId(
                    TypedEq<Utils::SmallStringView>("/some/not/known/path")));
    EXPECT_CALL(insertIntoSourceContexts,
                write(TypedEq<Utils::SmallStringView>("/some/not/known/path")));

    storage.fetchSourceContextIdUnguarded("/some/not/known/path");
}

TEST_F(ProjectStorage, SelectAndWriteForFetchingSourceIdForUnknownEntryUnguardedCalls)
{
    InSequence s;

    EXPECT_CALL(selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatment,
                valueReturnsSourceId(5, Eq("unknownfile.h")));
    EXPECT_CALL(insertIntoSourcesStatement,
                write(TypedEq<int>(5), TypedEq<Utils::SmallStringView>("unknownfile.h")));

    storage.fetchSourceIdUnguarded(SourceContextId{5}, "unknownfile.h");
}

TEST_F(ProjectStorage,
       SelectAndWriteForFetchingSourceContextIdTwoTimesIfTheIndexIsConstraintBecauseTheEntryExistsAlreadyCalls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectSourceContextIdFromSourceContextsBySourceContextPathStatement,
                valueReturnsSourceContextId(TypedEq<Utils::SmallStringView>("/other/unknow/path")));
    EXPECT_CALL(insertIntoSourceContexts, write(TypedEq<Utils::SmallStringView>("/other/unknow/path")))
        .WillOnce(Throw(Sqlite::ConstraintPreventsModification("busy")));
    EXPECT_CALL(databaseMock, rollback());
    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectSourceContextIdFromSourceContextsBySourceContextPathStatement,
                valueReturnsSourceContextId(TypedEq<Utils::SmallStringView>("/other/unknow/path")));
    EXPECT_CALL(insertIntoSourceContexts,
                write(TypedEq<Utils::SmallStringView>("/other/unknow/path")));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchSourceContextId("/other/unknow/path");
}

class ProjectStorageSlowTest : public testing::Test
{
protected:
    template<typename Range>
    static auto toValues(Range &&range)
    {
        using Type = typename std::decay_t<Range>;

        return std::vector<typename Type::value_type>{range.begin(), range.end()};
    }

    void addSomeDummyData()
    {
        auto sourceContextId1 = storage.fetchSourceContextId("/path/dummy");
        auto sourceContextId2 = storage.fetchSourceContextId("/path/dummy2");
        auto sourceContextId3 = storage.fetchSourceContextId("/path/");

        storage.fetchSourceId(sourceContextId1, "foo");
        storage.fetchSourceId(sourceContextId1, "dummy");
        storage.fetchSourceId(sourceContextId2, "foo");
        storage.fetchSourceId(sourceContextId2, "bar");
        storage.fetchSourceId(sourceContextId3, "foo");
        storage.fetchSourceId(sourceContextId3, "bar");
        storage.fetchSourceId(sourceContextId1, "bar");
        storage.fetchSourceId(sourceContextId3, "bar");
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    QmlDesigner::ProjectStorage<Sqlite::Database> storage{database, database.isInitialized()};
};

TEST_F(ProjectStorageSlowTest, FetchTypeIdByName)
{
    storage.upsertType("Yi",
                       TypeId{},
                       TypeAccessSemantics::Reference,
                       std::vector<Utils::SmallString>{"Qml.Yi"});
    auto internalTypeId = storage.upsertType("Er",
                                             TypeId{},
                                             TypeAccessSemantics::Reference,
                                             std::vector<Utils::SmallString>{"Qml.Er"});
    storage.upsertType("San",
                       TypeId{},
                       TypeAccessSemantics::Reference,
                       std::vector<Utils::SmallString>{"Qml.San"});

    auto id = storage.fetchTypeIdByQualifiedName("Qml.Er");

    ASSERT_THAT(id, internalTypeId);
}

TEST_F(ProjectStorageSlowTest, InsertType)
{
    auto internalTypeId = storage.upsertType("Yi",
                                             TypeId{},
                                             TypeAccessSemantics::Reference,
                                             std::vector<Utils::SmallString>{"Qml.Yi"});

    ASSERT_THAT(storage.fetchTypeIdByQualifiedName("Qml.Yi"), internalTypeId);
}

TEST_F(ProjectStorageSlowTest, UpsertType)
{
    auto internalTypeId = storage.upsertType("Yi",
                                             TypeId{},
                                             TypeAccessSemantics::Reference,
                                             std::vector<Utils::SmallString>{"Qml.Yi"});

    auto internalTypeId2 = storage.upsertType("Yi",
                                              TypeId{},
                                              TypeAccessSemantics::Reference,
                                              std::vector<Utils::SmallString>{"Qml.Yi"});

    ASSERT_THAT(internalTypeId2, internalTypeId);
}

TEST_F(ProjectStorageSlowTest, InsertTypeIdAreUnique)
{
    auto internalTypeId = storage.upsertType("Yi",
                                             TypeId{},
                                             TypeAccessSemantics::Reference,
                                             std::vector<Utils::SmallString>{"Qml.Yi"});
    auto internalTypeId2 = storage.upsertType("Er",
                                              TypeId{},
                                              TypeAccessSemantics::Reference,
                                              std::vector<Utils::SmallString>{"Qml.Er"});

    ASSERT_TRUE(internalTypeId != internalTypeId2);
}

TEST_F(ProjectStorageSlowTest, IsConvertibleTypeToBase)
{
    auto baseId = storage.upsertType("Base",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Qml.Base"});
    auto objectId = storage.upsertType("QObject",
                                       baseId,
                                       TypeAccessSemantics::Reference,
                                       std::vector<Utils::SmallString>{"Qml.Object"});
    auto itemId = storage.upsertType("QQuickItem",
                                     objectId,
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Quick.Item"});

    auto isConvertible = storage.fetchIsProtype(itemId, baseId);

    ASSERT_TRUE(isConvertible);
}

TEST_F(ProjectStorageSlowTest, IsConvertibleTypeToSameType)
{
    auto baseId = storage.upsertType("Base",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Qml.Base"});
    auto objectId = storage.upsertType("QObject",
                                       baseId,
                                       TypeAccessSemantics::Reference,
                                       std::vector<Utils::SmallString>{"Qml.Object"});
    auto itemId = storage.upsertType("QQuickItem",
                                     objectId,
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Quick.Item"});

    auto isConvertible = storage.fetchIsProtype(itemId, itemId);

    ASSERT_TRUE(isConvertible);
}

TEST_F(ProjectStorageSlowTest, IsConvertibleTypeToSomeTypeInTheMiddle)
{
    auto baseId = storage.upsertType("Base",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Qml.Base"});
    auto objectId = storage.upsertType("QObject",
                                       baseId,
                                       TypeAccessSemantics::Reference,
                                       std::vector<Utils::SmallString>{"Qml.Object"});
    auto itemId = storage.upsertType("QQuickItem",
                                     objectId,
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Quick.Item"});

    auto isConvertible = storage.fetchIsProtype(itemId, objectId);

    ASSERT_TRUE(isConvertible);
}

TEST_F(ProjectStorageSlowTest, IsNotConvertibleToUnrelatedType)
{
    auto unrelatedId = storage.upsertType("Base",
                                          TypeId{},
                                          TypeAccessSemantics::Reference,
                                          std::vector<Utils::SmallString>{"Qml.Base"});
    auto objectId = storage.upsertType("QObject",
                                       TypeId{},
                                       TypeAccessSemantics::Reference,
                                       std::vector<Utils::SmallString>{"Qml.Object"});
    auto itemId = storage.upsertType("QQuickItem",
                                     objectId,
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Quick.Item"});

    auto isConvertible = storage.fetchIsProtype(itemId, unrelatedId);

    ASSERT_FALSE(isConvertible);
}

TEST_F(ProjectStorageSlowTest, IsNotPrototypeOrSameType)
{
    auto baseId = storage.upsertType("Base",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Qml.Base"});
    auto objectId = storage.upsertType("QObject",
                                       baseId,
                                       TypeAccessSemantics::Reference,
                                       std::vector<Utils::SmallString>{"Qml.Object"});
    auto itemId = storage.upsertType("QQuickItem",
                                     baseId,
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Quick.Item"});

    auto isPrototype = storage.fetchIsProtype(itemId, objectId);

    ASSERT_FALSE(isPrototype);
}

TEST_F(ProjectStorageSlowTest, IsNotConvertibleToDerivedType)
{
    auto baseId = storage.upsertType("Base",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Qml.Base"});
    auto objectId = storage.upsertType("QObject",
                                       baseId,
                                       TypeAccessSemantics::Reference,
                                       std::vector<Utils::SmallString>{"Qml.Object"});

    auto isConvertible = storage.fetchIsProtype(baseId, objectId);

    ASSERT_FALSE(isConvertible);
}

TEST_F(ProjectStorageSlowTest, InsertPropertyDeclaration)
{
    auto typeId = storage.upsertType("QObject",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Qml.Object"});
    auto propertyTypeId = storage.upsertType("double",
                                             TypeId{},
                                             TypeAccessSemantics::Value,
                                             std::vector<Utils::SmallString>{"Qml.doube"});

    auto propertyDeclarationId = storage.upsertPropertyDeclaration(typeId, "foo", propertyTypeId);

    ASSERT_THAT(storage.fetchPropertyDeclarationByTypeIdAndName(typeId, "foo"), propertyDeclarationId);
}

TEST_F(ProjectStorageSlowTest, UpsertPropertyDeclaration)
{
    auto typeId = storage.upsertType("QObject",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Qml.Object"});
    auto propertyTypeId = storage.upsertType("double",
                                             TypeId{},
                                             TypeAccessSemantics::Value,
                                             std::vector<Utils::SmallString>{"Qml.doube"});
    auto propertyDeclarationId = storage.upsertPropertyDeclaration(typeId, "foo", propertyTypeId);

    auto propertyDeclarationId2 = storage.upsertPropertyDeclaration(typeId, "foo", propertyTypeId);

    ASSERT_THAT(propertyDeclarationId2, Eq(propertyDeclarationId));
}

TEST_F(ProjectStorageSlowTest, FetchPropertyDeclarationByTypeIdAndNameFromSameType)
{
    auto typeId = storage.upsertType("QObject",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Qml.Object"});
    auto propertyTypeId = storage.upsertType("double",
                                             TypeId{},
                                             TypeAccessSemantics::Value,
                                             std::vector<Utils::SmallString>{"Qml.doube"});
    auto propertyDeclarationId = storage.upsertPropertyDeclaration(typeId, "foo", propertyTypeId);

    auto id = storage.fetchPropertyDeclarationByTypeIdAndName(typeId, "foo");

    ASSERT_THAT(id, propertyDeclarationId);
}

TEST_F(ProjectStorageSlowTest, CannotFetchPropertyDeclarationByTypeIdAndNameForNonExistingProperty)
{
    auto typeId = storage.upsertType("QObject",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Qml.Object"});
    auto propertyTypeId = storage.upsertType("double",
                                             TypeId{},
                                             TypeAccessSemantics::Value,
                                             std::vector<Utils::SmallString>{"Qml.doube"});
    storage.upsertPropertyDeclaration(typeId, "foo", propertyTypeId);

    auto id = storage.fetchPropertyDeclarationByTypeIdAndName(typeId, "bar");

    ASSERT_FALSE(id.isValid());
}

TEST_F(ProjectStorageSlowTest, FetchPropertyDeclarationByTypeIdAndNameFromDerivedType)
{
    auto baseTypeId = storage.upsertType("QObject",
                                         TypeId{},
                                         TypeAccessSemantics::Reference,
                                         std::vector<Utils::SmallString>{"Qml.Object"});
    auto propertyTypeId = storage.upsertType("double",
                                             TypeId{},
                                             TypeAccessSemantics::Value,
                                             std::vector<Utils::SmallString>{"Qml.doube"});
    auto derivedTypeId = storage.upsertType("Derived",
                                            baseTypeId,
                                            TypeAccessSemantics::Reference,
                                            std::vector<Utils::SmallString>{"Qml.Derived"});
    auto propertyDeclarationId = storage.upsertPropertyDeclaration(baseTypeId, "foo", propertyTypeId);

    auto id = storage.fetchPropertyDeclarationByTypeIdAndName(derivedTypeId, "foo");

    ASSERT_THAT(id, propertyDeclarationId);
}

TEST_F(ProjectStorageSlowTest, FetchPropertyDeclarationByTypeIdAndNameFromBaseType)
{
    auto baseTypeId = storage.upsertType("QObject",
                                         TypeId{},
                                         TypeAccessSemantics::Reference,
                                         std::vector<Utils::SmallString>{"Qml.Object"});
    auto propertyTypeId = storage.upsertType("double",
                                             TypeId{},
                                             TypeAccessSemantics::Value,
                                             std::vector<Utils::SmallString>{"Qml.doube"});
    auto derivedTypeId = storage.upsertType("Derived",
                                            baseTypeId,
                                            TypeAccessSemantics::Reference,
                                            std::vector<Utils::SmallString>{"Qml.Derived"});
    storage.upsertPropertyDeclaration(derivedTypeId, "foo", propertyTypeId);

    auto id = storage.fetchPropertyDeclarationByTypeIdAndName(baseTypeId, "foo");

    ASSERT_FALSE(id.isValid());
}

TEST_F(ProjectStorageSlowTest, DISABLED_FetchPrototypes)
{
    auto baseId = storage.upsertType("Base",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Qml.Base"});
    auto objectId = storage.upsertType("QObject",
                                       baseId,
                                       TypeAccessSemantics::Reference,
                                       std::vector<Utils::SmallString>{"Qml.Object"});
    auto itemId = storage.upsertType("QQuickItem",
                                     objectId,
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Quick.Item"});

    auto prototypeIds = storage.fetchPrototypes(itemId);

    ASSERT_THAT(prototypeIds, ElementsAre(itemId, objectId, baseId));
}

TEST_F(ProjectStorageSlowTest, FetchSourceContextIdReturnsAlwaysTheSameIdForTheSamePath)
{
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    auto newSourceContextId = storage.fetchSourceContextId("/path/to");

    ASSERT_THAT(newSourceContextId, Eq(sourceContextId));
}

TEST_F(ProjectStorageSlowTest, FetchSourceContextIdReturnsNotTheSameIdForDifferentPath)
{
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    auto newSourceContextId = storage.fetchSourceContextId("/path/to2");

    ASSERT_THAT(newSourceContextId, Ne(sourceContextId));
}

TEST_F(ProjectStorageSlowTest, FetchSourceContextPath)
{
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    auto path = storage.fetchSourceContextPath(sourceContextId);

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(ProjectStorageSlowTest, FetchUnknownSourceContextPathThrows)
{
    ASSERT_THROW(storage.fetchSourceContextPath(SourceContextId{323}),
                 QmlDesigner::SourceContextIdDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, FetchAllSourceContextsAreEmptyIfNoSourceContextsExists)
{
    auto sourceContexts = storage.fetchAllSourceContexts();

    ASSERT_THAT(toValues(sourceContexts), IsEmpty());
}

TEST_F(ProjectStorageSlowTest, FetchAllSourceContexts)
{
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceContextId2 = storage.fetchSourceContextId("/path/to2");

    auto sourceContexts = storage.fetchAllSourceContexts();

    ASSERT_THAT(toValues(sourceContexts),
                UnorderedElementsAre(IsSourceContext(sourceContextId, "/path/to"),
                                     IsSourceContext(sourceContextId2, "/path/to2")));
}

TEST_F(ProjectStorageSlowTest, FetchSourceIdFirstTime)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    auto sourceId = storage.fetchSourceId(sourceContextId, "foo");

    ASSERT_TRUE(sourceId.isValid());
}

TEST_F(ProjectStorageSlowTest, FetchExistingSourceId)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto createdSourceId = storage.fetchSourceId(sourceContextId, "foo");

    auto sourceId = storage.fetchSourceId(sourceContextId, "foo");

    ASSERT_THAT(sourceId, createdSourceId);
}

TEST_F(ProjectStorageSlowTest, FetchSourceIdWithDifferentContextIdAreNotEqual)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceContextId2 = storage.fetchSourceContextId("/path/to2");
    auto sourceId2 = storage.fetchSourceId(sourceContextId2, "foo");

    auto sourceId = storage.fetchSourceId(sourceContextId, "foo");

    ASSERT_THAT(sourceId, Ne(sourceId2));
}

TEST_F(ProjectStorageSlowTest, FetchSourceIdWithDifferentNameAreNotEqual)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceId2 = storage.fetchSourceId(sourceContextId, "foo");

    auto sourceId = storage.fetchSourceId(sourceContextId, "foo2");

    ASSERT_THAT(sourceId, Ne(sourceId2));
}

TEST_F(ProjectStorageSlowTest, FetchSourceIdWithNonExistingSourceContextIdThrows)
{
    ASSERT_THROW(storage.fetchSourceId(SourceContextId{42}, "foo"),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorageSlowTest, FetchSourceNameAndSourceContextIdForNonExistingSourceId)
{
    ASSERT_THROW(storage.fetchSourceNameAndSourceContextId(SourceId{212}),
                 QmlDesigner::SourceIdDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, FetchSourceNameAndSourceContextIdForNonExistingEntry)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceId = storage.fetchSourceId(sourceContextId, "foo");

    auto sourceNameAndSourceContextId = storage.fetchSourceNameAndSourceContextId(sourceId);

    ASSERT_THAT(sourceNameAndSourceContextId, IsSourceNameAndSourceContextId("foo", sourceContextId));
}

TEST_F(ProjectStorageSlowTest, FetchSourceContextIdForNonExistingSourceId)
{
    ASSERT_THROW(storage.fetchSourceContextId(SourceId{212}), QmlDesigner::SourceIdDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, FetchSourceContextIdForExistingSourceId)
{
    addSomeDummyData();
    auto originalSourceContextId = storage.fetchSourceContextId("/path/to3");
    auto sourceId = storage.fetchSourceId(originalSourceContextId, "foo");

    auto sourceContextId = storage.fetchSourceContextId(sourceId);

    ASSERT_THAT(sourceContextId, Eq(originalSourceContextId));
}

TEST_F(ProjectStorageSlowTest, FetchAllSources)
{
    auto sources = storage.fetchAllSources();

    ASSERT_THAT(toValues(sources), IsEmpty());
}

TEST_F(ProjectStorageSlowTest, FetchSourceIdUnguardedFirstTime)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    std::lock_guard lock{database};

    auto sourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    ASSERT_TRUE(sourceId.isValid());
}

TEST_F(ProjectStorageSlowTest, FetchExistingSourceIdUnguarded)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    std::lock_guard lock{database};
    auto createdSourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    auto sourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    ASSERT_THAT(sourceId, createdSourceId);
}

TEST_F(ProjectStorageSlowTest, FetchSourceIdUnguardedWithDifferentContextIdAreNotEqual)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceContextId2 = storage.fetchSourceContextId("/path/to2");
    std::lock_guard lock{database};
    auto sourceId2 = storage.fetchSourceIdUnguarded(sourceContextId2, "foo");

    auto sourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    ASSERT_THAT(sourceId, Ne(sourceId2));
}

TEST_F(ProjectStorageSlowTest, FetchSourceIdUnguardedWithDifferentNameAreNotEqual)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    std::lock_guard lock{database};
    auto sourceId2 = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    auto sourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo2");

    ASSERT_THAT(sourceId, Ne(sourceId2));
}

TEST_F(ProjectStorageSlowTest, FetchSourceIdUnguardedWithNonExistingSourceContextIdThrows)
{
    std::lock_guard lock{database};

    ASSERT_THROW(storage.fetchSourceIdUnguarded(SourceContextId{42}, "foo"),
                 Sqlite::ConstraintPreventsModification);
}

} // namespace
