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
#include <projectstorage/sourcepathcache.h>
#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitewritestatement.h>

namespace {

using QmlDesigner::PropertyDeclarationId;
using QmlDesigner::SourceContextId;
using QmlDesigner::SourceId;
using QmlDesigner::TypeId;
using QmlDesigner::Cache::Source;
using QmlDesigner::Cache::SourceContext;
using QmlDesigner::Storage::TypeAccessSemantics;

namespace Storage = QmlDesigner::Storage;

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

MATCHER_P4(IsStorageType,
           typeName,
           prototype,
           accessSemantics,
           sourceId,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Type{typeName, prototype, accessSemantics, sourceId}))
{
    const Storage::Type &type = arg;

    return type.typeName == typeName && type.prototype == prototype
           && type.accessSemantics == accessSemantics && type.sourceId == sourceId;
}

MATCHER_P3(IsStorageTypeWithInvalidSourceId,
           typeName,
           prototype,
           accessSemantics,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Type{typeName, prototype, accessSemantics, SourceId{}}))
{
    const Storage::Type &type = arg;

    return type.typeName == typeName && type.prototype == prototype
           && type.accessSemantics == accessSemantics && !type.sourceId.isValid();
}

MATCHER_P2(IsExportedType,
           qualifiedTypeName,
           version,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::ExportedType{qualifiedTypeName, version}))
{
    const Storage::ExportedType &type = arg;

    return type.qualifiedTypeName == qualifiedTypeName && type.version == version;
}

MATCHER_P3(IsPropertyDeclaration,
           name,
           typeName,
           traits,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::PropertyDeclaration{name, typeName, traits}))
{
    const Storage::PropertyDeclaration &propertyDeclaration = arg;

    return propertyDeclaration.name == name && propertyDeclaration.typeName == typeName
           && propertyDeclaration.traits == traits;
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
    using ProjectStorageMock = QmlDesigner::ProjectStorage<SqliteDatabaseMock>;
    template<int ResultCount>
    using ReadStatement = ProjectStorageMock::ReadStatement<ResultCount>;
    using WriteStatement = ProjectStorageMock::WriteStatement;
    template<int ResultCount>
    using ReadWriteStatement = ProjectStorageMock::ReadWriteStatement<ResultCount>;

    NiceMock<SqliteDatabaseMock> databaseMock;
    ProjectStorageMock storage{databaseMock, true};
    ReadWriteStatement<1> &upsertTypeStatement = storage.upsertTypeStatement;
    ReadStatement<1> &selectTypeIdByQualifiedNameStatement = storage.selectTypeIdByQualifiedNameStatement;
    ReadWriteStatement<1> &upsertPropertyDeclarationStatement = storage.upsertPropertyDeclarationStatement;
    ReadStatement<1> &selectPropertyDeclarationByTypeIdAndNameStatement = storage.selectPropertyDeclarationByTypeIdAndNameStatement;
    WriteStatement &upsertExportedTypesStatement = storage.upsertExportedTypesStatement;
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
    ReadWriteStatement<1> &insertTypeStatement = storage.insertTypeStatement;
    ReadStatement<1> &selectTypeIdByNameStatement = storage.selectTypeIdByNameStatement;
    ReadStatement<4> &selectTypeByTypeIdStatement = storage.selectTypeByTypeIdStatement;
    ReadStatement<3> &selectExportedTypesByTypeIdStatement = storage.selectExportedTypesByTypeIdStatement;
    ReadStatement<5> &selectTypesStatement = storage.selectTypesStatement;
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
    EXPECT_CALL(upsertExportedTypesStatement,
                write(TypedEq<Utils::SmallStringView>("QML.Object"),
                      TypedEq<int>(5),
                      TypedEq<int>(15),
                      TypedEq<long long>(newTypeId.id)));
    EXPECT_CALL(upsertExportedTypesStatement,
                write(TypedEq<Utils::SmallStringView>("QML.Object"),
                      TypedEq<int>(5),
                      TypedEq<int>(12),
                      TypedEq<long long>(newTypeId.id)));
    EXPECT_CALL(databaseMock, commit);

    storage.upsertType("QObject",
                       prototypeId,
                       TypeAccessSemantics::Reference,
                       {Storage::ExportedType{"QML.Object", Storage::Version{5, 15}},
                        Storage::ExportedType{"QML.Object", Storage::Version{5, 12}}});
}

TEST_F(ProjectStorage, InsertTypeReturnsInternalPropertyId)
{
    TypeId prototypeId{3};
    TypeId newTypeId{11};
    ON_CALL(upsertTypeStatement, valueReturnsTypeId(Eq("QObject"), _, Eq(prototypeId.id)))
        .WillByDefault(Return(newTypeId));

    auto internalId = storage.upsertType("QObject", prototypeId, TypeAccessSemantics::Reference, {});

    ASSERT_THAT(internalId, Eq(newTypeId));
}

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

TEST_F(ProjectStorage, FetchTypeByTypeIdCalls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectTypeByTypeIdStatement, valueReturnsStorageType(Eq(21)));
    EXPECT_CALL(selectExportedTypesByTypeIdStatement, valuesReturnsStorageExportedTypes(_, Eq(21)));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchTypeByTypeId(TypeId{21});
}

TEST_F(ProjectStorage, FetchTypesCalls)
{
    InSequence s;
    Storage::Type type{{}, {}, {}, SourceId{}, {}, {}, {}, {}, {}, TypeId{55}};
    Storage::Types types{type};

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectTypesStatement, valuesReturnsStorageTypes(_)).WillOnce(Return(types));
    EXPECT_CALL(selectExportedTypesByTypeIdStatement, valuesReturnsStorageExportedTypes(_, Eq(55)));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchTypes();
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

    auto createTypes()
    {
        sourceId1 = sourcePathCache.sourceId(path1);
        sourceId2 = sourcePathCache.sourceId(path2);
        sourceId3 = sourcePathCache.sourceId(path3);
        sourceId4 = sourcePathCache.sourceId(path4);

        return Storage::Types{
            Storage::Type{
                "QQuickItem",
                "QObject",
                TypeAccessSemantics::Reference,
                sourceId1,
                {Storage::ExportedType{"QtQuick.Item", Storage::Version{5, 15}},
                 Storage::ExportedType{"QtQuick.Item", Storage::Version{5, 1}}},
                {Storage::PropertyDeclaration{"data", "QObject", Storage::DeclarationTraits::IsList},
                 Storage::PropertyDeclaration{"children",
                                              "QQuickItem",
                                              Storage::DeclarationTraits::IsList
                                                  | Storage::DeclarationTraits::IsReadOnly}},
                {Storage::FunctionDeclaration{"execute", "", {Storage::ParameterDeclaration{"arg", ""}}},
                 Storage::FunctionDeclaration{
                     "values",
                     "Vector3D",
                     {Storage::ParameterDeclaration{"arg1", "int"},
                      Storage::ParameterDeclaration{"arg2", "QObject", Storage::DeclarationTraits::IsPointer},
                      Storage::ParameterDeclaration{"arg3", "string"}}}},
                {Storage::SignalDeclaration{"execute", {Storage::ParameterDeclaration{"arg", ""}}},
                 Storage::SignalDeclaration{"values",
                                            {Storage::ParameterDeclaration{"arg1", "int"},
                                             Storage::ParameterDeclaration{
                                                 "arg2", "QObject", Storage::DeclarationTraits::IsPointer},
                                             Storage::ParameterDeclaration{"arg3", "string"}}}},
                {Storage::EnumerationDeclaration{"Enum",
                                                 {Storage::EnumeratorDeclaration{"Foo"},
                                                  Storage::EnumeratorDeclaration{"Bar", 32}}},
                 Storage::EnumerationDeclaration{"Type",
                                                 {Storage::EnumeratorDeclaration{"Foo"},
                                                  Storage::EnumeratorDeclaration{"Poo", 12}}}}},
            Storage::Type{"QObject",
                          {},
                          TypeAccessSemantics::Reference,
                          sourceId2,
                          {Storage::ExportedType{"QML.Object", Storage::Version{5, 14}},
                           Storage::ExportedType{"Qml.Object", Storage::Version{5, 1}}}}};
    }

protected:
    using ProjectStorage = QmlDesigner::ProjectStorage<Sqlite::Database>;
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ProjectStorage storage{database, database.isInitialized()};
    QmlDesigner::SourcePathCache<ProjectStorage> sourcePathCache{storage};
    QmlDesigner::SourcePathView path1{"/path1/to"};
    QmlDesigner::SourcePathView path2{"/path2/to"};
    QmlDesigner::SourcePathView path3{"/path3/to"};
    QmlDesigner::SourcePathView path4{"/path4/to"};
    SourceId sourceId1;
    SourceId sourceId2;
    SourceId sourceId3;
    SourceId sourceId4;
};

TEST_F(ProjectStorageSlowTest, FetchTypeIdByName)
{
    storage.upsertType("Yi",
                       TypeId{},
                       TypeAccessSemantics::Reference,
                       {Storage::ExportedType{"Qml.Yi"}});
    auto internalTypeId = storage.upsertType("Er",
                                             TypeId{},
                                             TypeAccessSemantics::Reference,
                                             {Storage::ExportedType{"Qml.Er"}});
    storage.upsertType("San",
                       TypeId{},
                       TypeAccessSemantics::Reference,
                       {Storage::ExportedType{"Qml.San"}});

    auto id = storage.fetchTypeIdByQualifiedNameAndVersion("Qml.Er");

    ASSERT_THAT(id, internalTypeId);
}

TEST_F(ProjectStorageSlowTest, FetchTypeIdByNameAndVersion)
{
    auto internalTypeId = storage.upsertType("Yi",
                                             TypeId{},
                                             TypeAccessSemantics::Reference,
                                             {Storage::ExportedType{"Qml.Yi", Storage::Version{5, 15}},
                                              Storage::ExportedType{"Qml.Yi", Storage::Version{5, 1}}});

    auto id = storage.fetchTypeIdByQualifiedNameAndVersion("Qml.Yi", Storage::Version{5, 1});

    ASSERT_THAT(id, internalTypeId);
}

TEST_F(ProjectStorageSlowTest, DontFetchTypeIdByWrongName)
{
    storage.upsertType("Yi",
                       TypeId{},
                       TypeAccessSemantics::Reference,
                       {Storage::ExportedType{"Qml.Yi", Storage::Version{5, 15}},
                        Storage::ExportedType{"Qml.Yi", Storage::Version{5, 1}}});

    auto id = storage.fetchTypeIdByQualifiedNameAndVersion("Qml.Er", Storage::Version{5, 1});

    ASSERT_FALSE(id.isValid());
}

TEST_F(ProjectStorageSlowTest, DontFetchTypeIdByWrongMajorVersion)
{
    storage.upsertType("Yi",
                       TypeId{},
                       TypeAccessSemantics::Reference,
                       {Storage::ExportedType{"Qml.Yi", Storage::Version{3, 15}},
                        Storage::ExportedType{"Qml.Yi", Storage::Version{5, 7}},
                        Storage::ExportedType{"Qml.Yi", Storage::Version{8, 1}}});

    auto id = storage.fetchTypeIdByQualifiedNameAndVersion("Qml.Yi", Storage::Version{6, 1});

    ASSERT_FALSE(id.isValid());
}

TEST_F(ProjectStorageSlowTest, DontFetchTypeIdByLowerMinorVersion)
{
    storage.upsertType("Yi",
                       TypeId{},
                       TypeAccessSemantics::Reference,
                       {Storage::ExportedType{"Qml.Yi", Storage::Version{5, 15}},
                        Storage::ExportedType{"Qml.Yi", Storage::Version{5, 7}},
                        Storage::ExportedType{"Qml.Yi", Storage::Version{5, 1}}});

    auto id = storage.fetchTypeIdByQualifiedNameAndVersion("Qml.Yi", Storage::Version{5, 0});

    ASSERT_FALSE(id.isValid());
}

TEST_F(ProjectStorageSlowTest, FetchTypeIdByHigherMinorVersion)
{
    storage.upsertType("Yi",
                       TypeId{},
                       TypeAccessSemantics::Reference,
                       {Storage::ExportedType{"Qml.Yi", Storage::Version{5, 7}},
                        Storage::ExportedType{"Qml.Yi", Storage::Version{5, 1}}});
    auto internalTypeId = storage.upsertType("Yi2",
                                             TypeId{},
                                             TypeAccessSemantics::Reference,
                                             {Storage::ExportedType{"Qml.Yi", Storage::Version{5, 15}},
                                              Storage::ExportedType{"Qml.Yi",
                                                                    Storage::Version{5, 12}}});

    auto id = storage.fetchTypeIdByQualifiedNameAndVersion("Qml.Yi", Storage::Version{5, 13});

    ASSERT_THAT(id, internalTypeId);
}

TEST_F(ProjectStorageSlowTest, FetchTypeIdByHigherMiddleVersion)
{
    auto internalTypeId = storage.upsertType("Yi",
                                             TypeId{},
                                             TypeAccessSemantics::Reference,
                                             {Storage::ExportedType{"Qml.Yi", Storage::Version{5, 7}},
                                              Storage::ExportedType{"Qml.Yi", Storage::Version{5, 1}}});
    storage.upsertType("Yi2",
                       TypeId{},
                       TypeAccessSemantics::Reference,
                       {Storage::ExportedType{"Qml.Yi", Storage::Version{5, 15}},
                        Storage::ExportedType{"Qml.Yi", Storage::Version{5, 12}}});

    auto id = storage.fetchTypeIdByQualifiedNameAndVersion("Qml.Yi", Storage::Version{5, 11});

    ASSERT_THAT(id, internalTypeId);
}

TEST_F(ProjectStorageSlowTest, InsertType)
{
    auto internalTypeId = storage.upsertType("Yi",
                                             TypeId{},
                                             TypeAccessSemantics::Reference,
                                             {Storage::ExportedType{"Qml.Yi"}});

    ASSERT_THAT(storage.fetchTypeIdByQualifiedNameAndVersion("Qml.Yi"), internalTypeId);
}

TEST_F(ProjectStorageSlowTest, UpsertType)
{
    auto internalTypeId = storage.upsertType("Yi",
                                             TypeId{},
                                             TypeAccessSemantics::Reference,
                                             {Storage::ExportedType{"Qml.Yi"}});

    auto internalTypeId2 = storage.upsertType("Yi",
                                              TypeId{},
                                              TypeAccessSemantics::Reference,
                                              {Storage::ExportedType{"Qml.Yi"}});

    ASSERT_THAT(internalTypeId2, internalTypeId);
}

TEST_F(ProjectStorageSlowTest, InsertTypeIdAreUnique)
{
    auto internalTypeId = storage.upsertType("Yi",
                                             TypeId{},
                                             TypeAccessSemantics::Reference,
                                             {Storage::ExportedType{"Qml.Yi"}});
    auto internalTypeId2 = storage.upsertType("Er",
                                              TypeId{},
                                              TypeAccessSemantics::Reference,
                                              {Storage::ExportedType{"Qml.Er"}});

    ASSERT_TRUE(internalTypeId != internalTypeId2);
}

TEST_F(ProjectStorageSlowTest, IsConvertibleTypeToBase)
{
    auto baseId = storage.upsertType("Base",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     {Storage::ExportedType{"Qml.Base"}});
    auto objectId = storage.upsertType("QObject",
                                       baseId,
                                       TypeAccessSemantics::Reference,
                                       {Storage::ExportedType{"Qml.Object"}});
    auto itemId = storage.upsertType("QQuickItem",
                                     objectId,
                                     TypeAccessSemantics::Reference,
                                     {Storage::ExportedType{"Quick.Item"}});

    auto isConvertible = storage.fetchIsProtype(itemId, baseId);

    ASSERT_TRUE(isConvertible);
}

TEST_F(ProjectStorageSlowTest, IsConvertibleTypeToSameType)
{
    auto baseId = storage.upsertType("Base",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     {Storage::ExportedType{"Qml.Base"}});
    auto objectId = storage.upsertType("QObject",
                                       baseId,
                                       TypeAccessSemantics::Reference,
                                       {Storage::ExportedType{"Qml.Object"}});
    auto itemId = storage.upsertType("QQuickItem",
                                     objectId,
                                     TypeAccessSemantics::Reference,
                                     {Storage::ExportedType{"Quick.Item"}});

    auto isConvertible = storage.fetchIsProtype(itemId, itemId);

    ASSERT_TRUE(isConvertible);
}

TEST_F(ProjectStorageSlowTest, IsConvertibleTypeToSomeTypeInTheMiddle)
{
    auto baseId = storage.upsertType("Base",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     {Storage::ExportedType{"Qml.Base"}});
    auto objectId = storage.upsertType("QObject",
                                       baseId,
                                       TypeAccessSemantics::Reference,
                                       {Storage::ExportedType{"Qml.Object"}});
    auto itemId = storage.upsertType("QQuickItem",
                                     objectId,
                                     TypeAccessSemantics::Reference,
                                     {Storage::ExportedType{"Quick.Item"}});

    auto isConvertible = storage.fetchIsProtype(itemId, objectId);

    ASSERT_TRUE(isConvertible);
}

TEST_F(ProjectStorageSlowTest, IsNotConvertibleToUnrelatedType)
{
    auto unrelatedId = storage.upsertType("Base",
                                          TypeId{},
                                          TypeAccessSemantics::Reference,
                                          {Storage::ExportedType{"Qml.Base"}});
    auto objectId = storage.upsertType("QObject",
                                       TypeId{},
                                       TypeAccessSemantics::Reference,
                                       {Storage::ExportedType{"Qml.Object"}});
    auto itemId = storage.upsertType("QQuickItem",
                                     objectId,
                                     TypeAccessSemantics::Reference,
                                     {Storage::ExportedType{"Quick.Item"}});

    auto isConvertible = storage.fetchIsProtype(itemId, unrelatedId);

    ASSERT_FALSE(isConvertible);
}

TEST_F(ProjectStorageSlowTest, IsNotPrototypeOrSameType)
{
    auto baseId = storage.upsertType("Base",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     {Storage::ExportedType{"Qml.Base"}});
    auto objectId = storage.upsertType("QObject",
                                       baseId,
                                       TypeAccessSemantics::Reference,
                                       {Storage::ExportedType{"Qml.Object"}});
    auto itemId = storage.upsertType("QQuickItem",
                                     baseId,
                                     TypeAccessSemantics::Reference,
                                     {Storage::ExportedType{"Quick.Item"}});

    auto isPrototype = storage.fetchIsProtype(itemId, objectId);

    ASSERT_FALSE(isPrototype);
}

TEST_F(ProjectStorageSlowTest, IsNotConvertibleToDerivedType)
{
    auto baseId = storage.upsertType("Base",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     {Storage::ExportedType{"Qml.Base"}});
    auto objectId = storage.upsertType("QObject",
                                       baseId,
                                       TypeAccessSemantics::Reference,
                                       {Storage::ExportedType{"Qml.Object"}});

    auto isConvertible = storage.fetchIsProtype(baseId, objectId);

    ASSERT_FALSE(isConvertible);
}

TEST_F(ProjectStorageSlowTest, InsertPropertyDeclaration)
{
    auto typeId = storage.upsertType("QObject",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     {Storage::ExportedType{"Qml.Object"}});
    auto propertyTypeId = storage.upsertType("double",
                                             TypeId{},
                                             TypeAccessSemantics::Value,
                                             {Storage::ExportedType{"Qml.doube"}});

    auto propertyDeclarationId = storage.upsertPropertyDeclaration(typeId, "foo", propertyTypeId);

    ASSERT_THAT(storage.fetchPropertyDeclarationByTypeIdAndName(typeId, "foo"), propertyDeclarationId);
}

TEST_F(ProjectStorageSlowTest, UpsertPropertyDeclaration)
{
    auto typeId = storage.upsertType("QObject",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     {Storage::ExportedType{"Qml.Object"}});
    auto propertyTypeId = storage.upsertType("double",
                                             TypeId{},
                                             TypeAccessSemantics::Value,
                                             {Storage::ExportedType{"Qml.doube"}});
    auto propertyDeclarationId = storage.upsertPropertyDeclaration(typeId, "foo", propertyTypeId);

    auto propertyDeclarationId2 = storage.upsertPropertyDeclaration(typeId, "foo", propertyTypeId);

    ASSERT_THAT(propertyDeclarationId2, Eq(propertyDeclarationId));
}

TEST_F(ProjectStorageSlowTest, FetchPropertyDeclarationByTypeIdAndNameFromSameType)
{
    auto typeId = storage.upsertType("QObject",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     {Storage::ExportedType{"Qml.Object"}});
    auto propertyTypeId = storage.upsertType("double",
                                             TypeId{},
                                             TypeAccessSemantics::Value,
                                             {Storage::ExportedType{"Qml.doube"}});
    auto propertyDeclarationId = storage.upsertPropertyDeclaration(typeId, "foo", propertyTypeId);

    auto id = storage.fetchPropertyDeclarationByTypeIdAndName(typeId, "foo");

    ASSERT_THAT(id, propertyDeclarationId);
}

TEST_F(ProjectStorageSlowTest, CannotFetchPropertyDeclarationByTypeIdAndNameForNonExistingProperty)
{
    auto typeId = storage.upsertType("QObject",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     {Storage::ExportedType{"Qml.Object"}});
    auto propertyTypeId = storage.upsertType("double",
                                             TypeId{},
                                             TypeAccessSemantics::Value,
                                             {Storage::ExportedType{"Qml.doube"}});
    storage.upsertPropertyDeclaration(typeId, "foo", propertyTypeId);

    auto id = storage.fetchPropertyDeclarationByTypeIdAndName(typeId, "bar");

    ASSERT_FALSE(id.isValid());
}

TEST_F(ProjectStorageSlowTest, FetchPropertyDeclarationByTypeIdAndNameFromDerivedType)
{
    auto baseTypeId = storage.upsertType("QObject",
                                         TypeId{},
                                         TypeAccessSemantics::Reference,
                                         {Storage::ExportedType{"Qml.Object"}});
    auto propertyTypeId = storage.upsertType("double",
                                             TypeId{},
                                             TypeAccessSemantics::Value,
                                             {Storage::ExportedType{"Qml.doube"}});
    auto derivedTypeId = storage.upsertType("Derived",
                                            baseTypeId,
                                            TypeAccessSemantics::Reference,
                                            {Storage::ExportedType{"Qml.Derived"}});
    auto propertyDeclarationId = storage.upsertPropertyDeclaration(baseTypeId, "foo", propertyTypeId);

    auto id = storage.fetchPropertyDeclarationByTypeIdAndName(derivedTypeId, "foo");

    ASSERT_THAT(id, propertyDeclarationId);
}

TEST_F(ProjectStorageSlowTest, FetchPropertyDeclarationByTypeIdAndNameFromBaseType)
{
    auto baseTypeId = storage.upsertType("QObject",
                                         TypeId{},
                                         TypeAccessSemantics::Reference,
                                         {Storage::ExportedType{"Qml.Object"}});
    auto propertyTypeId = storage.upsertType("double",
                                             TypeId{},
                                             TypeAccessSemantics::Value,
                                             {Storage::ExportedType{"Qml.doube"}});
    auto derivedTypeId = storage.upsertType("Derived",
                                            baseTypeId,
                                            TypeAccessSemantics::Reference,
                                            {Storage::ExportedType{"Qml.Derived"}});
    storage.upsertPropertyDeclaration(derivedTypeId, "foo", propertyTypeId);

    auto id = storage.fetchPropertyDeclarationByTypeIdAndName(baseTypeId, "foo");

    ASSERT_FALSE(id.isValid());
}

TEST_F(ProjectStorageSlowTest, DISABLED_FetchPrototypes)
{
    auto baseId = storage.upsertType("Base",
                                     TypeId{},
                                     TypeAccessSemantics::Reference,
                                     {Storage::ExportedType{"Qml.Base"}});
    auto objectId = storage.upsertType("QObject",
                                       baseId,
                                       TypeAccessSemantics::Reference,
                                       {Storage::ExportedType{"Qml.Object"}});
    auto itemId = storage.upsertType("QQuickItem",
                                     objectId,
                                     TypeAccessSemantics::Reference,
                                     {Storage::ExportedType{"Quick.Item"}});

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

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddsNewTypes)
{
    Storage::Types types{createTypes()};

    storage.synchronizeTypes(types, {sourceId1, sourceId2});

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType("QObject", "", TypeAccessSemantics::Reference, sourceId2),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType("Qml.Object", Storage::Version{5, 1}),
                                             IsExportedType("QML.Object", Storage::Version{5, 14})))),
            AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType("QtQuick.Item", Storage::Version{5, 1}),
                                             IsExportedType("QtQuick.Item", Storage::Version{5, 15}))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddsNewTypesReverseOrder)
{
    Storage::Types types{createTypes()};
    std::reverse(types.begin(), types.end());

    storage.synchronizeTypes(types, {sourceId1, sourceId2});

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType("QObject", "", TypeAccessSemantics::Reference, sourceId2),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType("Qml.Object", Storage::Version{5, 1}),
                                             IsExportedType("QML.Object", Storage::Version{5, 14})))),
            AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType("QtQuick.Item", Storage::Version{5, 1}),
                                             IsExportedType("QtQuick.Item", Storage::Version{5, 15}))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesOverwritesTypeAccessSemantics)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {sourceId1, sourceId2});
    types[0].accessSemantics = TypeAccessSemantics::Value;
    types[1].accessSemantics = TypeAccessSemantics::Value;

    storage.synchronizeTypes(types, {sourceId1, sourceId2});

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType("QObject", "", TypeAccessSemantics::Value, sourceId2),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType("Qml.Object", Storage::Version{5, 1}),
                                             IsExportedType("QML.Object", Storage::Version{5, 14})))),
            AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Value, sourceId1),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType("QtQuick.Item", Storage::Version{5, 1}),
                                             IsExportedType("QtQuick.Item", Storage::Version{5, 15}))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesOverwritesSources)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {sourceId1, sourceId2});
    types[0].sourceId = sourceId3;
    types[1].sourceId = sourceId4;

    storage.synchronizeTypes(types, {sourceId1, sourceId2});

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType("QObject", "", TypeAccessSemantics::Reference, sourceId4),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType("Qml.Object", Storage::Version{5, 1}),
                                             IsExportedType("QML.Object", Storage::Version{5, 14})))),
            AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId3),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType("QtQuick.Item", Storage::Version{5, 1}),
                                             IsExportedType("QtQuick.Item", Storage::Version{5, 15}))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesOverwritesTypes)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {sourceId1, sourceId2});
    types[0].exportedTypes.push_back(Storage::ExportedType{"QtQuick.Item", Storage::Version{5, 19}});
    types[0].exportedTypes.push_back(Storage::ExportedType{"QtQuick.Item", Storage::Version{6, 1}});
    types[1].exportedTypes.push_back(Storage::ExportedType{"Qml.Object", Storage::Version{6, 1}});

    storage.synchronizeTypes(types, {sourceId3, sourceId4});

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType("QObject", "", TypeAccessSemantics::Reference, sourceId2),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType("Qml.Object", Storage::Version{5, 1}),
                                             IsExportedType("QML.Object", Storage::Version{5, 14}),
                                             IsExportedType("Qml.Object", Storage::Version{6, 1})))),
            AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType("QtQuick.Item", Storage::Version{5, 1}),
                                             IsExportedType("QtQuick.Item", Storage::Version{5, 15}),
                                             IsExportedType("QtQuick.Item", Storage::Version{5, 19}),
                                             IsExportedType("QtQuick.Item", Storage::Version{6, 1}))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesInsertTypeIntoPrototypeChain)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {sourceId1, sourceId2});
    types[0].prototype = "QQuickObject";
    types.push_back(Storage::Type{"QQuickObject",
                                  "QObject",
                                  TypeAccessSemantics::Reference,
                                  sourceId1,
                                  {Storage::ExportedType{"QtQuick.Object", Storage::Version{5, 15}}}});

    storage.synchronizeTypes(types, {sourceId1, sourceId2});

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType("QObject", "", TypeAccessSemantics::Reference, sourceId2),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType("Qml.Object", Storage::Version{5, 1}),
                                             IsExportedType("QML.Object", Storage::Version{5, 14})))),
            AllOf(IsStorageType("QQuickObject", "QObject", TypeAccessSemantics::Reference, sourceId1),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType("QtQuick.Object", Storage::Version{5, 15})))),
            AllOf(IsStorageType("QQuickItem", "QQuickObject", TypeAccessSemantics::Reference, sourceId1),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType("QtQuick.Item", Storage::Version{5, 1}),
                                             IsExportedType("QtQuick.Item", Storage::Version{5, 15}))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesDontThrowsForMissingPrototype)
{
    sourceId1 = sourcePathCache.sourceId(path1);
    Storage::Types types{
        Storage::Type{"QQuickItem",
                      "QObject",
                      TypeAccessSemantics::Reference,
                      sourceId1,
                      {Storage::ExportedType{"QtQuick.Item", Storage::Version{5, 15}}}}};

    ASSERT_NO_THROW(storage.synchronizeTypes(types, {sourceId1}));
}

TEST_F(ProjectStorageSlowTest, TypeWithInvalidSourceIdThrows)
{
    Storage::Types types{
        Storage::Type{"QQuickItem",
                      "",
                      TypeAccessSemantics::Reference,
                      SourceId{},
                      {Storage::ExportedType{"QtQuick.Item", Storage::Version{5, 15}}}}};

    ASSERT_THROW(storage.synchronizeTypes(types, {}), QmlDesigner::TypeHasInvalidSourceId);
}

TEST_F(ProjectStorageSlowTest, ResetsTypeIfTypeIsRemoved)
{
    Storage::Types types{createTypes()};
    types[1].enumerationDeclarations = types[0].enumerationDeclarations;
    types[1].propertyDeclarations = types[0].propertyDeclarations;
    types[1].functionDeclarations = types[0].functionDeclarations;
    types[1].signalDeclarations = types[0].signalDeclarations;
    storage.synchronizeTypes(types, {sourceId1, sourceId2});
    types.pop_back();

    storage.synchronizeTypes(types, {sourceId1, sourceId2});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageTypeWithInvalidSourceId("QObject", "", TypeAccessSemantics::Invalid),
                          Field(&Storage::Type::exportedTypes, IsEmpty()),
                          Field(&Storage::Type::enumerationDeclarations, IsEmpty()),
                          Field(&Storage::Type::propertyDeclarations, IsEmpty()),
                          Field(&Storage::Type::functionDeclarations, IsEmpty()),
                          Field(&Storage::Type::signalDeclarations, IsEmpty()))));
}

TEST_F(ProjectStorageSlowTest, DontResetsTypeIfSourceIdIsNotSynchronized)
{
    Storage::Types types{createTypes()};
    types[1].enumerationDeclarations = types[0].enumerationDeclarations;
    types[1].propertyDeclarations = types[0].propertyDeclarations;
    types[1].functionDeclarations = types[0].functionDeclarations;
    types[1].signalDeclarations = types[0].signalDeclarations;
    storage.synchronizeTypes(types, {sourceId1, sourceId2});
    types.pop_back();

    storage.synchronizeTypes(types, {sourceId1});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType("QObject", "", TypeAccessSemantics::Reference, sourceId2),
                               Field(&Storage::Type::exportedTypes, Not(IsEmpty())),
                               Field(&Storage::Type::exportedTypes, Not(IsEmpty())),
                               Field(&Storage::Type::enumerationDeclarations, Not(IsEmpty())),
                               Field(&Storage::Type::propertyDeclarations, Not(IsEmpty())),
                               Field(&Storage::Type::functionDeclarations, Not(IsEmpty())),
                               Field(&Storage::Type::signalDeclarations, Not(IsEmpty())))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddPropertyDeclarations)
{
    Storage::Types types{createTypes()};

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("data", "QObject", Storage::DeclarationTraits::IsList),
                            IsPropertyDeclaration("children",
                                                  "QQuickItem",
                                                  Storage::DeclarationTraits::IsList
                                                      | Storage::DeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesPropertyDeclarationType)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].propertyDeclarations[0].typeName = "QQuickItem";

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("data", "QQuickItem", Storage::DeclarationTraits::IsList),
                            IsPropertyDeclaration("children",
                                                  "QQuickItem",
                                                  Storage::DeclarationTraits::IsList
                                                      | Storage::DeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesDeclarationTraits)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].propertyDeclarations[0].traits = Storage::DeclarationTraits::IsPointer;

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("data", "QObject", Storage::DeclarationTraits::IsPointer),
                            IsPropertyDeclaration("children",
                                                  "QQuickItem",
                                                  Storage::DeclarationTraits::IsList
                                                      | Storage::DeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesDeclarationTraitsAndType)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].propertyDeclarations[0].traits = Storage::DeclarationTraits::IsPointer;
    types[0].propertyDeclarations[0].typeName = "QQuickItem";

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("data", "QQuickItem", Storage::DeclarationTraits::IsPointer),
                            IsPropertyDeclaration("children",
                                                  "QQuickItem",
                                                  Storage::DeclarationTraits::IsList
                                                      | Storage::DeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesRemovesAPropertyDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].propertyDeclarations.pop_back();

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::propertyDeclarations,
                                UnorderedElementsAre(IsPropertyDeclaration(
                                    "data", "QObject", Storage::DeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddsAPropertyDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"object", "QObject", Storage::DeclarationTraits::IsPointer});

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
            Field(&Storage::Type::propertyDeclarations,
                  UnorderedElementsAre(
                      IsPropertyDeclaration("object", "QObject", Storage::DeclarationTraits::IsPointer),
                      IsPropertyDeclaration("data", "QObject", Storage::DeclarationTraits::IsList),
                      IsPropertyDeclaration("children",
                                            "QQuickItem",
                                            Storage::DeclarationTraits::IsList
                                                | Storage::DeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesRenameAPropertyDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].propertyDeclarations[1].name = "objects";

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("data", "QObject", Storage::DeclarationTraits::IsList),
                            IsPropertyDeclaration("objects",
                                                  "QQuickItem",
                                                  Storage::DeclarationTraits::IsList
                                                      | Storage::DeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddFunctionDeclarations)
{
    Storage::Types types{createTypes()};

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                     Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesFunctionDeclarationReturnType)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].functionDeclarations[1].returnTypeName = "item";

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                     Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesFunctionDeclarationName)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].functionDeclarations[1].name = "name";

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                     Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesFunctionDeclarationPopParameters)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].functionDeclarations[1].parameters.pop_back();

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                     Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesFunctionDeclarationAppendParameters)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].functionDeclarations[1].parameters.push_back(Storage::ParameterDeclaration{"arg4", "int"});

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                     Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesFunctionDeclarationChangeParameterName)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].functionDeclarations[1].parameters[0].name = "other";

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                     Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesFunctionDeclarationChangeParameterTypeName)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].functionDeclarations[1].parameters[0].name = "long long";

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                     Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesFunctionDeclarationChangeParameterTraits)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].functionDeclarations[1].parameters[0].traits = QmlDesigner::Storage::DeclarationTraits::IsList;

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                     Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesRemovesFunctionDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].functionDeclarations.pop_back();

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(types[0].functionDeclarations[0]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddFunctionDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].functionDeclarations.push_back(
        Storage::FunctionDeclaration{"name", "string", {Storage::ParameterDeclaration{"arg", "int"}}});

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                     Eq(types[0].functionDeclarations[1]),
                                                     Eq(types[0].functionDeclarations[2]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddSignalDeclarations)
{
    Storage::Types types{createTypes()};

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                     Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesSignalDeclarationName)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].signalDeclarations[1].name = "name";

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                     Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesSignalDeclarationPopParameters)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].signalDeclarations[1].parameters.pop_back();

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                     Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesSignalDeclarationAppendParameters)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].signalDeclarations[1].parameters.push_back(Storage::ParameterDeclaration{"arg4", "int"});

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                     Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesSignalDeclarationChangeParameterName)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].signalDeclarations[1].parameters[0].name = "other";

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                     Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesSignalDeclarationChangeParameterTypeName)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].signalDeclarations[1].parameters[0].typeName = "long long";

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                     Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesSignalDeclarationChangeParameterTraits)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].signalDeclarations[1].parameters[0].traits = QmlDesigner::Storage::DeclarationTraits::IsList;

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                     Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesRemovesSignalDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].signalDeclarations.pop_back();

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(types[0].signalDeclarations[0]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddSignalDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].signalDeclarations.push_back(
        Storage::SignalDeclaration{"name", {Storage::ParameterDeclaration{"arg", "int"}}});

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                     Eq(types[0].signalDeclarations[1]),
                                                     Eq(types[0].signalDeclarations[2]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddEnumerationDeclarations)
{
    Storage::Types types{createTypes()};

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::enumerationDeclarations,
                                UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                     Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesEnumerationDeclarationName)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].enumerationDeclarations[1].name = "Name";

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::enumerationDeclarations,
                                UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                     Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesEnumerationDeclarationPopEnumeratorDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations.pop_back();

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::enumerationDeclarations,
                                UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                     Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesEnumerationDeclarationAppendEnumeratorDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations.push_back(
        Storage::EnumeratorDeclaration{"Haa", 54});

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::enumerationDeclarations,
                                UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                     Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest,
       SynchronizeTypesChangesEnumerationDeclarationChangeEnumeratorDeclarationName)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations[0].name = "Hoo";

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::enumerationDeclarations,
                                UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                     Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest,
       SynchronizeTypesChangesEnumerationDeclarationChangeEnumeratorDeclarationValue)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations[1].value = 11;

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::enumerationDeclarations,
                                UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                     Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest,
       SynchronizeTypesChangesEnumerationDeclarationAddThatEnumeratorDeclarationHasValue)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations[0].value = 11;
    types[0].enumerationDeclarations[1].enumeratorDeclarations[0].hasValue = true;

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::enumerationDeclarations,
                                UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                     Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest,
       SynchronizeTypesChangesEnumerationDeclarationRemoveThatEnumeratorDeclarationHasValue)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations[0].hasValue = false;

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::enumerationDeclarations,
                                UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                     Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesRemovesEnumerationDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].enumerationDeclarations.pop_back();

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::enumerationDeclarations,
                                UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddEnumerationDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronizeTypes(types, {});
    types[0].enumerationDeclarations.push_back(
        Storage::EnumerationDeclaration{"name", {Storage::EnumeratorDeclaration{"Foo", 98, true}}});

    storage.synchronizeTypes(types, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType("QQuickItem", "QObject", TypeAccessSemantics::Reference, sourceId1),
                          Field(&Storage::Type::enumerationDeclarations,
                                UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                     Eq(types[0].enumerationDeclarations[1]),
                                                     Eq(types[0].enumerationDeclarations[2]))))));
}

} // namespace
