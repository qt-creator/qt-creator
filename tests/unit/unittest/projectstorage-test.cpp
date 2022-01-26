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

using QmlDesigner::FileStatus;
using QmlDesigner::FileStatuses;
using QmlDesigner::ModuleId;
using QmlDesigner::PropertyDeclarationId;
using QmlDesigner::SourceContextId;
using QmlDesigner::SourceId;
using QmlDesigner::SourceIds;
using QmlDesigner::TypeId;
using QmlDesigner::Cache::Source;
using QmlDesigner::Cache::SourceContext;
using QmlDesigner::Storage::SynchronizationPackage;
using QmlDesigner::Storage::TypeAccessSemantics;

namespace Storage = QmlDesigner::Storage;

Storage::Imports operator+(const Storage::Imports &first, const Storage::Imports &second)
{
    Storage::Imports imports;
    imports.reserve(first.size() + second.size());

    imports.insert(imports.end(), first.begin(), first.end());
    imports.insert(imports.end(), second.begin(), second.end());

    return imports;
}

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
           sourceId,
           typeName,
           prototypeId,
           accessSemantics,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Type{typeName, prototypeId, accessSemantics, sourceId}))
{
    const Storage::Type &type = arg;

    return type.sourceId == sourceId && type.typeName == typeName
           && type.accessSemantics == accessSemantics && &prototypeId == &type.prototypeId;
}

MATCHER_P(IsExportedType,
          name,
          std::string(negation ? "isn't " : "is ") + PrintToString(Storage::ExportedType{name}))
{
    const Storage::ExportedType &type = arg;

    return type.name == name;
}

MATCHER_P2(IsExportedType,
           moduleId,
           name,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::ExportedType{moduleId, name}))
{
    const Storage::ExportedType &type = arg;

    return type.moduleId == moduleId && type.name == name;
}

MATCHER_P3(IsExportedType,
           name,
           majorVersion,
           minorVersion,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::ExportedType{name,
                                                     Storage::Version{majorVersion, minorVersion}}))
{
    const Storage::ExportedType &type = arg;

    return type.name == name && type.version == Storage::Version{majorVersion, minorVersion};
}

MATCHER_P4(IsExportedType,
           moduleId,
           name,
           majorVersion,
           minorVersion,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::ExportedType{moduleId,
                                                     name,
                                                     Storage::Version{majorVersion, minorVersion}}))
{
    const Storage::ExportedType &type = arg;

    return type.moduleId == moduleId && type.name == name
           && type.version == Storage::Version{majorVersion, minorVersion};
}

MATCHER_P3(IsPropertyDeclaration,
           name,
           propertyTypeId,
           traits,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::PropertyDeclaration{name, propertyTypeId, traits}))
{
    const Storage::PropertyDeclaration &propertyDeclaration = arg;

    return propertyDeclaration.name == name && propertyTypeId == propertyDeclaration.propertyTypeId
           && propertyDeclaration.traits == traits;
}

MATCHER_P4(IsPropertyDeclaration,
           name,
           propertyTypeId,
           traits,
           aliasPropertyName,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(
                   Storage::PropertyDeclaration{name, propertyTypeId, traits, aliasPropertyName}))
{
    const Storage::PropertyDeclaration &propertyDeclaration = arg;

    return propertyDeclaration.name == name && propertyTypeId == propertyDeclaration.propertyTypeId
           && propertyDeclaration.aliasPropertyName == aliasPropertyName
           && propertyDeclaration.traits == traits;
}

class ProjectStorage : public testing::Test
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

    auto createSimpleSynchronizationPackage()
    {
        SynchronizationPackage package;

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1);
        package.imports.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId1);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1);
        package.imports.emplace_back(qtQuickNativeModuleId, Storage::Version{}, sourceId1);
        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId2);
        package.imports.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId2);

        importsSourceId1.emplace_back(qmlModuleId, Storage::Version{}, sourceId1);
        importsSourceId1.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId1);
        importsSourceId1.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1);
        importsSourceId1.emplace_back(qtQuickNativeModuleId, Storage::Version{}, sourceId1);

        importsSourceId2.emplace_back(qmlModuleId, Storage::Version{}, sourceId2);
        importsSourceId2.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId2);

        package.types.push_back(Storage::Type{
            "QQuickItem",
            Storage::ImportedType{"QObject"},
            TypeAccessSemantics::Reference,
            sourceId1,
            {Storage::ExportedType{qtQuickModuleId, "Item"},
             Storage::ExportedType{qtQuickNativeModuleId, "QQuickItem"}},
            {Storage::PropertyDeclaration{"data",
                                          Storage::ImportedType{"QObject"},
                                          Storage::PropertyDeclarationTraits::IsList},
             Storage::PropertyDeclaration{"children",
                                          Storage::ImportedType{"Item"},
                                          Storage::PropertyDeclarationTraits::IsList
                                              | Storage::PropertyDeclarationTraits::IsReadOnly}},
            {Storage::FunctionDeclaration{"execute", "", {Storage::ParameterDeclaration{"arg", ""}}},
             Storage::FunctionDeclaration{
                 "values",
                 "Vector3D",
                 {Storage::ParameterDeclaration{"arg1", "int"},
                  Storage::ParameterDeclaration{"arg2",
                                                "QObject",
                                                Storage::PropertyDeclarationTraits::IsPointer},
                  Storage::ParameterDeclaration{"arg3", "string"}}}},
            {Storage::SignalDeclaration{"execute", {Storage::ParameterDeclaration{"arg", ""}}},
             Storage::SignalDeclaration{"value0s",
                                        {Storage::ParameterDeclaration{"arg1", "int"},
                                         Storage::ParameterDeclaration{
                                             "arg2", "QObject", Storage::PropertyDeclarationTraits::IsPointer},
                                         Storage::ParameterDeclaration{"arg3", "string"}}}},
            {Storage::EnumerationDeclaration{"Enum",
                                             {Storage::EnumeratorDeclaration{"Foo"},
                                              Storage::EnumeratorDeclaration{"Bar", 32}}},
             Storage::EnumerationDeclaration{"Type",
                                             {Storage::EnumeratorDeclaration{"Foo"},
                                              Storage::EnumeratorDeclaration{"Poo", 12}}}}});
        package.types.push_back(
            Storage::Type{"QObject",
                          Storage::ImportedType{},
                          TypeAccessSemantics::Reference,
                          sourceId2,
                          {Storage::ExportedType{qmlModuleId, "Object", Storage::Version{2}},
                           Storage::ExportedType{qmlModuleId, "Obj", Storage::Version{2}},
                           Storage::ExportedType{qmlNativeModuleId, "QObject"}}});

        package.updatedSourceIds = {sourceId1, sourceId2};

        return package;
    }

    auto createSynchronizationPackageWithAliases()
    {
        auto package{createSimpleSynchronizationPackage()};

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId3);
        package.imports.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId3);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
        package.imports.emplace_back(qtQuickNativeModuleId, Storage::Version{}, sourceId3);

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId4);
        package.imports.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId4);
        package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId4);

        importsSourceId3.emplace_back(qmlModuleId, Storage::Version{}, sourceId3);
        importsSourceId3.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId3);
        importsSourceId3.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
        importsSourceId3.emplace_back(qtQuickNativeModuleId, Storage::Version{}, sourceId3);

        importsSourceId4.emplace_back(qmlModuleId, Storage::Version{}, sourceId4);
        importsSourceId4.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId4);
        importsSourceId4.emplace_back(pathToModuleId, Storage::Version{}, sourceId4);

        package.types[1].propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"objects",
                                         Storage::ImportedType{"QObject"},
                                         Storage::PropertyDeclarationTraits::IsList});
        package.types.push_back(
            Storage::Type{"QAliasItem",
                          Storage::ImportedType{"Item"},
                          TypeAccessSemantics::Reference,
                          sourceId3,
                          {Storage::ExportedType{qtQuickModuleId, "AliasItem"},
                           Storage::ExportedType{qtQuickNativeModuleId, "QAliasItem"}}});
        package.types.back().propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"data",
                                         Storage::ImportedType{"QObject"},
                                         Storage::PropertyDeclarationTraits::IsList});
        package.types.back().propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"items", Storage::ImportedType{"Item"}, "children"});
        package.types.back().propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"objects", Storage::ImportedType{"Item"}, "objects"});

        package.types.push_back(Storage::Type{"QObject2",
                                              Storage::ImportedType{},
                                              TypeAccessSemantics::Reference,
                                              sourceId4,
                                              {Storage::ExportedType{pathToModuleId, "Object2"},
                                               Storage::ExportedType{pathToModuleId, "Obj2"}}});
        package.types[3].propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"objects",
                                         Storage::ImportedType{"QObject"},
                                         Storage::PropertyDeclarationTraits::IsList});

        package.updatedSourceIds.push_back(sourceId3);
        package.updatedSourceIds.push_back(sourceId4);

        return package;
    }

    auto createSynchronizationPackageWithAliases2()
    {
        auto package{createSynchronizationPackageWithAliases()};

        package.types[2].prototype = Storage::ImportedType{"Object"};
        package.types[2].propertyDeclarations.erase(
            std::next(package.types[2].propertyDeclarations.begin()));

        return package;
    }

    auto createSynchronizationPackageWithRecursiveAliases()
    {
        auto package{createSynchronizationPackageWithAliases()};

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId5);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId5);

        package.types.push_back(
            Storage::Type{"QAliasItem2",
                          Storage::ImportedType{"Object"},
                          TypeAccessSemantics::Reference,
                          sourceId5,
                          {Storage::ExportedType{qtQuickModuleId, "AliasItem2"},
                           Storage::ExportedType{qtQuickNativeModuleId, "QAliasItem2"}}});

        package.types.back().propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"objects", Storage::ImportedType{"AliasItem"}, "objects"});

        package.updatedSourceIds.push_back(sourceId5);

        return package;
    }

    auto createSynchronizationPackageWithVersions()
    {
        SynchronizationPackage package;

        package.types.push_back(
            Storage::Type{"QObject",
                          Storage::ImportedType{},
                          TypeAccessSemantics::Reference,
                          sourceId1,
                          {Storage::ExportedType{qmlModuleId, "Object", Storage::Version{1}},
                           Storage::ExportedType{qmlModuleId, "Obj", Storage::Version{1, 2}},
                           Storage::ExportedType{qmlModuleId, "BuiltInObj"},
                           Storage::ExportedType{qmlNativeModuleId, "QObject"}}});
        package.types.push_back(
            Storage::Type{"QObject2",
                          Storage::ImportedType{},
                          TypeAccessSemantics::Reference,
                          sourceId1,
                          {Storage::ExportedType{qmlModuleId, "Object", Storage::Version{2, 0}},
                           Storage::ExportedType{qmlModuleId, "Obj", Storage::Version{2, 3}},
                           Storage::ExportedType{qmlNativeModuleId, "QObject2"}}});
        package.types.push_back(
            Storage::Type{"QObject3",
                          Storage::ImportedType{},
                          TypeAccessSemantics::Reference,
                          sourceId1,
                          {Storage::ExportedType{qmlModuleId, "Object", Storage::Version{2, 11}},
                           Storage::ExportedType{qmlModuleId, "Obj", Storage::Version{2, 11}},
                           Storage::ExportedType{qmlNativeModuleId, "QObject3"}}});
        package.types.push_back(
            Storage::Type{"QObject4",
                          Storage::ImportedType{},
                          TypeAccessSemantics::Reference,
                          sourceId1,
                          {Storage::ExportedType{qmlModuleId, "Object", Storage::Version{3, 4}},
                           Storage::ExportedType{qmlModuleId, "Obj", Storage::Version{3, 4}},
                           Storage::ExportedType{qmlModuleId, "BuiltInObj", Storage::Version{3, 4}},
                           Storage::ExportedType{qmlNativeModuleId, "QObject4"}}});

        package.updatedSourceIds.push_back(sourceId1);

        return package;
    }

    template<typename Range>
    static FileStatuses convert(const Range &range)
    {
        return FileStatuses(range.begin(), range.end());
    }

    TypeId fetchTypeId(SourceId sourceId, Utils::SmallStringView name)
    {
        return storage.fetchTypeIdByName(sourceId, name);
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    //Sqlite::Database database{TESTDATA_DIR "/aaaaa.db", Sqlite::JournalMode::Wal};
    QmlDesigner::ProjectStorage<Sqlite::Database> storage{database, database.isInitialized()};
    QmlDesigner::SourcePathCache<QmlDesigner::ProjectStorage<Sqlite::Database>> sourcePathCache{
        storage};
    QmlDesigner::SourcePathView path1{"/path1/to"};
    QmlDesigner::SourcePathView path2{"/path2/to"};
    QmlDesigner::SourcePathView path3{"/path3/to"};
    QmlDesigner::SourcePathView path4{"/path4/to"};
    QmlDesigner::SourcePathView path5{"/path5/to"};
    SourceId sourceId1{sourcePathCache.sourceId(path1)};
    SourceId sourceId2{sourcePathCache.sourceId(path2)};
    SourceId sourceId3{sourcePathCache.sourceId(path3)};
    SourceId sourceId4{sourcePathCache.sourceId(path4)};
    SourceId sourceId5{sourcePathCache.sourceId(path5)};
    SourceId qmlProjectSourceId{sourcePathCache.sourceId("/path1/qmldir")};
    SourceId qtQuickProjectSourceId{sourcePathCache.sourceId("/path2/qmldir")};
    ModuleId qmlModuleId{storage.moduleId("Qml")};
    ModuleId qmlNativeModuleId{storage.moduleId("Qml-cppnative")};
    ModuleId qtQuickModuleId{storage.moduleId("QtQuick")};
    ModuleId qtQuickNativeModuleId{storage.moduleId("QtQuick-cppnative")};
    ModuleId pathToModuleId{storage.moduleId("/path/to")};
    Storage::Imports importsSourceId1;
    Storage::Imports importsSourceId2;
    Storage::Imports importsSourceId3;
    Storage::Imports importsSourceId4;
};

TEST_F(ProjectStorage, FetchSourceContextIdReturnsAlwaysTheSameIdForTheSamePath)
{
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    auto newSourceContextId = storage.fetchSourceContextId("/path/to");

    ASSERT_THAT(newSourceContextId, Eq(sourceContextId));
}

TEST_F(ProjectStorage, FetchSourceContextIdReturnsNotTheSameIdForDifferentPath)
{
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    auto newSourceContextId = storage.fetchSourceContextId("/path/to2");

    ASSERT_THAT(newSourceContextId, Ne(sourceContextId));
}

TEST_F(ProjectStorage, FetchSourceContextPath)
{
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    auto path = storage.fetchSourceContextPath(sourceContextId);

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(ProjectStorage, FetchUnknownSourceContextPathThrows)
{
    ASSERT_THROW(storage.fetchSourceContextPath(SourceContextId{323}),
                 QmlDesigner::SourceContextIdDoesNotExists);
}

TEST_F(ProjectStorage, FetchAllSourceContextsAreEmptyIfNoSourceContextsExists)
{
    storage.clearSources();

    auto sourceContexts = storage.fetchAllSourceContexts();

    ASSERT_THAT(toValues(sourceContexts), IsEmpty());
}

TEST_F(ProjectStorage, FetchAllSourceContexts)
{
    storage.clearSources();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceContextId2 = storage.fetchSourceContextId("/path/to2");

    auto sourceContexts = storage.fetchAllSourceContexts();

    ASSERT_THAT(toValues(sourceContexts),
                UnorderedElementsAre(IsSourceContext(sourceContextId, "/path/to"),
                                     IsSourceContext(sourceContextId2, "/path/to2")));
}

TEST_F(ProjectStorage, FetchSourceIdFirstTime)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    auto sourceId = storage.fetchSourceId(sourceContextId, "foo");

    ASSERT_TRUE(sourceId.isValid());
}

TEST_F(ProjectStorage, FetchExistingSourceId)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto createdSourceId = storage.fetchSourceId(sourceContextId, "foo");

    auto sourceId = storage.fetchSourceId(sourceContextId, "foo");

    ASSERT_THAT(sourceId, createdSourceId);
}

TEST_F(ProjectStorage, FetchSourceIdWithDifferentContextIdAreNotEqual)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceContextId2 = storage.fetchSourceContextId("/path/to2");
    auto sourceId2 = storage.fetchSourceId(sourceContextId2, "foo");

    auto sourceId = storage.fetchSourceId(sourceContextId, "foo");

    ASSERT_THAT(sourceId, Ne(sourceId2));
}

TEST_F(ProjectStorage, FetchSourceIdWithDifferentNameAreNotEqual)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceId2 = storage.fetchSourceId(sourceContextId, "foo");

    auto sourceId = storage.fetchSourceId(sourceContextId, "foo2");

    ASSERT_THAT(sourceId, Ne(sourceId2));
}

TEST_F(ProjectStorage, FetchSourceIdWithNonExistingSourceContextIdThrows)
{
    ASSERT_THROW(storage.fetchSourceId(SourceContextId{42}, "foo"),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorage, FetchSourceNameAndSourceContextIdForNonExistingSourceId)
{
    ASSERT_THROW(storage.fetchSourceNameAndSourceContextId(SourceId{212}),
                 QmlDesigner::SourceIdDoesNotExists);
}

TEST_F(ProjectStorage, FetchSourceNameAndSourceContextIdForNonExistingEntry)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceId = storage.fetchSourceId(sourceContextId, "foo");

    auto sourceNameAndSourceContextId = storage.fetchSourceNameAndSourceContextId(sourceId);

    ASSERT_THAT(sourceNameAndSourceContextId, IsSourceNameAndSourceContextId("foo", sourceContextId));
}

TEST_F(ProjectStorage, FetchSourceContextIdForNonExistingSourceId)
{
    ASSERT_THROW(storage.fetchSourceContextId(SourceId{212}), QmlDesigner::SourceIdDoesNotExists);
}

TEST_F(ProjectStorage, FetchSourceContextIdForExistingSourceId)
{
    addSomeDummyData();
    auto originalSourceContextId = storage.fetchSourceContextId("/path/to3");
    auto sourceId = storage.fetchSourceId(originalSourceContextId, "foo");

    auto sourceContextId = storage.fetchSourceContextId(sourceId);

    ASSERT_THAT(sourceContextId, Eq(originalSourceContextId));
}

TEST_F(ProjectStorage, FetchAllSources)
{
    storage.clearSources();

    auto sources = storage.fetchAllSources();

    ASSERT_THAT(toValues(sources), IsEmpty());
}

TEST_F(ProjectStorage, FetchSourceIdUnguardedFirstTime)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    std::lock_guard lock{database};

    auto sourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    ASSERT_TRUE(sourceId.isValid());
}

TEST_F(ProjectStorage, FetchExistingSourceIdUnguarded)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    std::lock_guard lock{database};
    auto createdSourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    auto sourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    ASSERT_THAT(sourceId, createdSourceId);
}

TEST_F(ProjectStorage, FetchSourceIdUnguardedWithDifferentContextIdAreNotEqual)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceContextId2 = storage.fetchSourceContextId("/path/to2");
    std::lock_guard lock{database};
    auto sourceId2 = storage.fetchSourceIdUnguarded(sourceContextId2, "foo");

    auto sourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    ASSERT_THAT(sourceId, Ne(sourceId2));
}

TEST_F(ProjectStorage, FetchSourceIdUnguardedWithDifferentNameAreNotEqual)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    std::lock_guard lock{database};
    auto sourceId2 = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    auto sourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo2");

    ASSERT_THAT(sourceId, Ne(sourceId2));
}

TEST_F(ProjectStorage, FetchSourceIdUnguardedWithNonExistingSourceContextIdThrows)
{
    std::lock_guard lock{database};

    ASSERT_THROW(storage.fetchSourceIdUnguarded(SourceContextId{42}, "foo"),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorage, SynchronizeTypesAddsNewTypes)
{
    auto package{createSimpleSynchronizationPackage()};

    storage.synchronize(std::move(package));

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                fetchTypeId(sourceId2, "QObject"),
                                TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickItem"))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddsNewTypesWithExportedPrototypeName)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::ImportedType{"Object"};

    storage.synchronize(std::move(package));

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                fetchTypeId(sourceId2, "QObject"),
                                TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickItem"))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddsNewTypesThrowsWithWrongPrototypeName)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::ImportedType{"Objec"};

    ASSERT_THROW(storage.synchronize(std::move(package)), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesAddsNewTypesWithMissingModule)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.push_back(Storage::Type{"QObject2",
                                          Storage::ImportedType{},
                                          TypeAccessSemantics::Reference,
                                          sourceId3,
                                          {Storage::ExportedType{ModuleId{22}, "Object2"},
                                           Storage::ExportedType{pathToModuleId, "Obj2"}}});

    ASSERT_THROW(storage.synchronize(std::move(package)), QmlDesigner::ExportedTypeCannotBeInserted);
}

TEST_F(ProjectStorage, SynchronizeTypesAddsNewTypesReverseOrder)
{
    auto package{createSimpleSynchronizationPackage()};
    std::reverse(package.types.begin(), package.types.end());

    storage.synchronize(std::move(package));

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                fetchTypeId(sourceId2, "QObject"),
                                TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickItem"))))));
}

TEST_F(ProjectStorage, SynchronizeTypesOverwritesTypeAccessSemantics)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].accessSemantics = TypeAccessSemantics::Value;
    package.types[1].accessSemantics = TypeAccessSemantics::Value;

    storage.synchronize(SynchronizationPackage{package.imports, package.types, {sourceId1, sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeAccessSemantics::Value),
                          Field(&Storage::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Value),
                          Field(&Storage::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                                     IsExportedType(qtQuickNativeModuleId,
                                                                    "QQuickItem"))))));
}

TEST_F(ProjectStorage, SynchronizeTypesOverwritesSources)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].sourceId = sourceId3;
    package.types[1].sourceId = sourceId4;
    Storage::Imports newImports;
    newImports.emplace_back(qmlModuleId, Storage::Version{}, sourceId3);
    newImports.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId3);
    newImports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    newImports.emplace_back(qtQuickNativeModuleId, Storage::Version{}, sourceId3);
    newImports.emplace_back(qmlModuleId, Storage::Version{}, sourceId4);
    newImports.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId4);

    storage.synchronize(SynchronizationPackage{newImports,
                                               package.types,
                                               {sourceId1, sourceId2, sourceId3, sourceId4}});

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId4, "QObject", TypeId{}, TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId3,
                                "QQuickItem",
                                fetchTypeId(sourceId4, "QObject"),
                                TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickItem"))))));
}

TEST_F(ProjectStorage, SynchronizeTypesInsertTypeIntoPrototypeChain)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].prototype = Storage::ImportedType{"QQuickObject"};
    package.types.push_back(
        Storage::Type{"QQuickObject",
                      Storage::ImportedType{"QObject"},
                      TypeAccessSemantics::Reference,
                      sourceId1,
                      {Storage::ExportedType{qtQuickModuleId, "Object"},
                       Storage::ExportedType{qtQuickNativeModuleId, "QQuickObject"}}});

    storage.synchronize(
        SynchronizationPackage{importsSourceId1, {package.types[0], package.types[2]}, {sourceId1}});

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickObject",
                                fetchTypeId(sourceId2, "QObject"),
                                TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Object"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                fetchTypeId(sourceId1, "QQuickObject"),
                                TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickItem"))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddQualifiedPrototype)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::QualifiedImportedType{"Object",
                                                                Storage::Import{qtQuickModuleId,
                                                                                Storage::Version{},
                                                                                sourceId1}};
    package.types.push_back(
        Storage::Type{"QQuickObject",
                      Storage::ImportedType{"QObject"},
                      TypeAccessSemantics::Reference,
                      sourceId1,
                      {Storage::ExportedType{qtQuickModuleId, "Object"},
                       Storage::ExportedType{qtQuickNativeModuleId, "QQuickObject"}}});

    storage.synchronize(package);

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickObject",
                                fetchTypeId(sourceId2, "QObject"),
                                TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Object"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                fetchTypeId(sourceId1, "QQuickObject"),
                                TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickItem"))))));
}

TEST_F(ProjectStorage, SynchronizeTypesThrowsForMissingPrototype)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types = {Storage::Type{"QQuickItem",
                                   Storage::ImportedType{"QObject"},
                                   TypeAccessSemantics::Reference,
                                   sourceId1,
                                   {Storage::ExportedType{qtQuickModuleId, "Item"},
                                    Storage::ExportedType{qtQuickNativeModuleId, "QQuickItem"}}}};

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesThrowsForInvalidModule)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types = {Storage::Type{"QQuickItem",
                                   Storage::ImportedType{"QObject"},
                                   TypeAccessSemantics::Reference,
                                   sourceId1,
                                   {Storage::ExportedType{ModuleId{}, "Item"}}}};

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::ModuleDoesNotExists);
}

TEST_F(ProjectStorage, TypeWithInvalidSourceIdThrows)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types = {Storage::Type{"QQuickItem",
                                   Storage::ImportedType{""},
                                   TypeAccessSemantics::Reference,
                                   SourceId{},
                                   {Storage::ExportedType{qtQuickModuleId, "Item"}}}};

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeHasInvalidSourceId);
}

TEST_F(ProjectStorage, DeleteTypeIfSourceIdIsSynchronized)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types.erase(package.types.begin());

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeAccessSemantics::Reference),
                          Field(&Storage::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject"))))));
}

TEST_F(ProjectStorage, DontDeleteTypeIfSourceIdIsNotSynchronized)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                fetchTypeId(sourceId2, "QObject"),
                                TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickItem"))))));
}

TEST_F(ProjectStorage, UpdateExportedTypesIfTypeNameChanges)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].typeName = "QQuickItem2";

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem2",
                                fetchTypeId(sourceId2, "QObject"),
                                TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickItem"))))));
}

TEST_F(ProjectStorage, BreakingPrototypeChainByDeletingBaseComponentThrows)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{importsSourceId1,
                                                            {package.types[0]},
                                                            {sourceId1, sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesAddPropertyDeclarations)
{
    auto package{createSimpleSynchronizationPackage()};

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("data",
                                                          fetchTypeId(sourceId2, "QObject"),
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "children",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        Storage::PropertyDeclarationTraits::IsList
                                            | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddPropertyDeclarationsWithMissingImports)
{
    auto package{createSimpleSynchronizationPackage()};
    package.imports.clear();

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesAddPropertyDeclarationQualifiedType)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};
    package.types.push_back(Storage::Type{"QQuickObject",
                                          Storage::ImportedType{"QObject"},
                                          TypeAccessSemantics::Reference,
                                          sourceId1,
                                          {Storage::ExportedType{qtQuickModuleId, "Object"}}});

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("data",
                                                          fetchTypeId(sourceId1, "QQuickObject"),
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "children",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        Storage::PropertyDeclarationTraits::IsList
                                            | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesPropertyDeclarationType)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].typeName = Storage::ImportedType{"QQuickItem"};

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("data",
                                                          fetchTypeId(sourceId1, "QQuickItem"),
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "children",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        Storage::PropertyDeclarationTraits::IsList
                                            | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesDeclarationTraits)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsPointer;

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("data",
                                                          fetchTypeId(sourceId2, "QObject"),
                                                          Storage::PropertyDeclarationTraits::IsPointer),
                                    IsPropertyDeclaration(
                                        "children",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        Storage::PropertyDeclarationTraits::IsList
                                            | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesDeclarationTraitsAndType)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsPointer;
    package.types[0].propertyDeclarations[0].typeName = Storage::ImportedType{"QQuickItem"};

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("data",
                                                          fetchTypeId(sourceId1, "QQuickItem"),
                                                          Storage::PropertyDeclarationTraits::IsPointer),
                                    IsPropertyDeclaration(
                                        "children",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        Storage::PropertyDeclarationTraits::IsList
                                            | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, SynchronizeTypesRemovesAPropertyDeclaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations.pop_back();

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(sourceId1,
                                             "QQuickItem",
                                             fetchTypeId(sourceId2, "QObject"),
                                             TypeAccessSemantics::Reference),
                               Field(&Storage::Type::propertyDeclarations,
                                     UnorderedElementsAre(IsPropertyDeclaration(
                                         "data",
                                         fetchTypeId(sourceId2, "QObject"),
                                         Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddsAPropertyDeclaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"object",
                                     Storage::ImportedType{"QObject"},
                                     Storage::PropertyDeclarationTraits::IsPointer});

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId1,
                          "QQuickItem",
                          fetchTypeId(sourceId2, "QObject"),
                          TypeAccessSemantics::Reference),
            Field(&Storage::Type::propertyDeclarations,
                  UnorderedElementsAre(
                      IsPropertyDeclaration("object",
                                            fetchTypeId(sourceId2, "QObject"),
                                            Storage::PropertyDeclarationTraits::IsPointer),
                      IsPropertyDeclaration("data",
                                            fetchTypeId(sourceId2, "QObject"),
                                            Storage::PropertyDeclarationTraits::IsList),
                      IsPropertyDeclaration("children",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsList
                                                | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, SynchronizeTypesRenameAPropertyDeclaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[1].name = "objects";

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("data",
                                                          fetchTypeId(sourceId2, "QObject"),
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "objects",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        Storage::PropertyDeclarationTraits::IsList
                                            | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, UsingNonExistingPropertyTypeThrows)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::ImportedType{"QObject2"};
    package.types.pop_back();
    package.imports = importsSourceId1;

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, UsingNonExistingQualifiedExportedPropertyTypeWithWrongNameThrows)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "QObject2", Storage::Import{qmlNativeModuleId, Storage::Version{}, sourceId1}};
    package.types.pop_back();
    package.imports = importsSourceId1;

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, UsingNonExistingQualifiedExportedPropertyTypeWithWrongModuleThrows)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "QObject", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};
    package.types.pop_back();
    package.imports = importsSourceId1;

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, BreakingPropertyDeclarationTypeDependencyByDeletingTypeThrows)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].prototype = Storage::ImportedType{};
    package.types.pop_back();
    package.imports = importsSourceId1;

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesAddFunctionDeclarations)
{
    auto package{createSimpleSynchronizationPackage()};

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesFunctionDeclarationReturnType)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations[1].returnTypeName = "item";

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesFunctionDeclarationName)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations[1].name = "name";

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesFunctionDeclarationPopParameters)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations[1].parameters.pop_back();

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesFunctionDeclarationAppendParameters)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations[1].parameters.push_back(
        Storage::ParameterDeclaration{"arg4", "int"});

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesFunctionDeclarationChangeParameterName)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations[1].parameters[0].name = "other";

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesFunctionDeclarationChangeParameterTypeName)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations[1].parameters[0].name = "long long";

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesFunctionDeclarationChangeParameterTraits)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations[1].parameters[0].traits = QmlDesigner::Storage::
        PropertyDeclarationTraits::IsList;

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesRemovesFunctionDeclaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations.pop_back();

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddFunctionDeclaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations.push_back(
        Storage::FunctionDeclaration{"name", "string", {Storage::ParameterDeclaration{"arg", "int"}}});

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]),
                                                     Eq(package.types[0].functionDeclarations[2]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddSignalDeclarations)
{
    auto package{createSimpleSynchronizationPackage()};

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                                     Eq(package.types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesSignalDeclarationName)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations[1].name = "name";

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                                     Eq(package.types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesSignalDeclarationPopParameters)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations[1].parameters.pop_back();

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                                     Eq(package.types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesSignalDeclarationAppendParameters)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations[1].parameters.push_back(
        Storage::ParameterDeclaration{"arg4", "int"});

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                                     Eq(package.types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesSignalDeclarationChangeParameterName)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations[1].parameters[0].name = "other";

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                                     Eq(package.types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesSignalDeclarationChangeParameterTypeName)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations[1].parameters[0].typeName = "long long";

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                                     Eq(package.types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesSignalDeclarationChangeParameterTraits)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations[1].parameters[0].traits = QmlDesigner::Storage::PropertyDeclarationTraits::IsList;

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                                     Eq(package.types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesRemovesSignalDeclaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations.pop_back();

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddSignalDeclaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations.push_back(
        Storage::SignalDeclaration{"name", {Storage::ParameterDeclaration{"arg", "int"}}});

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeAccessSemantics::Reference),
                          Field(&Storage::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                                     Eq(package.types[0].signalDeclarations[1]),
                                                     Eq(package.types[0].signalDeclarations[2]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddEnumerationDeclarations)
{
    auto package{createSimpleSynchronizationPackage()};

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId1,
                                  "QQuickItem",
                                  fetchTypeId(sourceId2, "QObject"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesEnumerationDeclarationName)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations[1].name = "Name";

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId1,
                                  "QQuickItem",
                                  fetchTypeId(sourceId2, "QObject"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesEnumerationDeclarationPopEnumeratorDeclaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations[1].enumeratorDeclarations.pop_back();

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId1,
                                  "QQuickItem",
                                  fetchTypeId(sourceId2, "QObject"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesEnumerationDeclarationAppendEnumeratorDeclaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations[1].enumeratorDeclarations.push_back(
        Storage::EnumeratorDeclaration{"Haa", 54});

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId1,
                                  "QQuickItem",
                                  fetchTypeId(sourceId2, "QObject"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesEnumerationDeclarationChangeEnumeratorDeclarationName)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations[1].enumeratorDeclarations[0].name = "Hoo";

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId1,
                                  "QQuickItem",
                                  fetchTypeId(sourceId2, "QObject"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesEnumerationDeclarationChangeEnumeratorDeclarationValue)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations[1].enumeratorDeclarations[1].value = 11;

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId1,
                                  "QQuickItem",
                                  fetchTypeId(sourceId2, "QObject"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage,
       SynchronizeTypesChangesEnumerationDeclarationAddThatEnumeratorDeclarationHasValue)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations[1].enumeratorDeclarations[0].value = 11;
    package.types[0].enumerationDeclarations[1].enumeratorDeclarations[0].hasValue = true;

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId1,
                                  "QQuickItem",
                                  fetchTypeId(sourceId2, "QObject"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage,
       SynchronizeTypesChangesEnumerationDeclarationRemoveThatEnumeratorDeclarationHasValue)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations[1].enumeratorDeclarations[0].hasValue = false;

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId1,
                                  "QQuickItem",
                                  fetchTypeId(sourceId2, "QObject"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesRemovesEnumerationDeclaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations.pop_back();

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(sourceId1,
                                             "QQuickItem",
                                             fetchTypeId(sourceId2, "QObject"),
                                             TypeAccessSemantics::Reference),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(
                                         Eq(package.types[0].enumerationDeclarations[0]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddEnumerationDeclaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations.push_back(
        Storage::EnumerationDeclaration{"name", {Storage::EnumeratorDeclaration{"Foo", 98, true}}});

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId1,
                                  "QQuickItem",
                                  fetchTypeId(sourceId2, "QObject"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]),
                                               Eq(package.types[0].enumerationDeclarations[2]))))));
}

TEST_F(ProjectStorage, FetchTypeIdBySourceIdAndName)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    auto typeId = storage.fetchTypeIdByName(sourceId2, "QObject");

    ASSERT_THAT(storage.fetchTypeIdByExportedName("Object"), Eq(typeId));
}

TEST_F(ProjectStorage, FetchTypeIdByExportedName)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    auto typeId = storage.fetchTypeIdByExportedName("Object");

    ASSERT_THAT(storage.fetchTypeIdByName(sourceId2, "QObject"), Eq(typeId));
}

TEST_F(ProjectStorage, FetchTypeIdByImporIdsAndExportedName)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    auto typeId = storage.fetchTypeIdByModuleIdsAndExportedName({qmlModuleId, qtQuickModuleId},
                                                                "Object");

    ASSERT_THAT(storage.fetchTypeIdByName(sourceId2, "QObject"), Eq(typeId));
}

TEST_F(ProjectStorage, FetchInvalidTypeIdByImporIdsAndExportedNameIfModuleIdsAreEmpty)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    auto typeId = storage.fetchTypeIdByModuleIdsAndExportedName({}, "Object");

    ASSERT_FALSE(typeId.isValid());
}

TEST_F(ProjectStorage, FetchInvalidTypeIdByImporIdsAndExportedNameIfModuleIdsAreInvalid)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    auto typeId = storage.fetchTypeIdByModuleIdsAndExportedName({ModuleId{}}, "Object");

    ASSERT_FALSE(typeId.isValid());
}

TEST_F(ProjectStorage, FetchInvalidTypeIdByImporIdsAndExportedNameIfNotInModule)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    auto qtQuickModuleId = storage.moduleId("QtQuick");

    auto typeId = storage.fetchTypeIdByModuleIdsAndExportedName({qtQuickModuleId}, "Object");

    ASSERT_FALSE(typeId.isValid());
}

TEST_F(ProjectStorage, SynchronizeTypesAddAliasDeclarations)
{
    auto package{createSynchronizationPackageWithAliases()};

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId1, "QQuickItem"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    fetchTypeId(sourceId1, "QQuickItem"),
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddAliasDeclarationsAgain)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId1, "QQuickItem"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    fetchTypeId(sourceId1, "QQuickItem"),
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesRemoveAliasDeclarations)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations.pop_back();

    storage.synchronize(SynchronizationPackage{importsSourceId3, {package.types[2]}, {sourceId3}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId1, "QQuickItem"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    fetchTypeId(sourceId1, "QQuickItem"),
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("data",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddAliasDeclarationsThrowsForWrongTypeName)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[1].typeName = Storage::ImportedType{"QQuickItemWrong"};

    ASSERT_THROW(storage.synchronize(
                     SynchronizationPackage{importsSourceId4, {package.types[2]}, {sourceId4}}),
                 QmlDesigner::TypeNameDoesNotExists);
}
TEST_F(ProjectStorage, SynchronizeTypesAddAliasDeclarationsThrowsForWrongPropertyName)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[1].aliasPropertyName = "childrenWrong";

    ASSERT_THROW(storage.synchronize(
                     SynchronizationPackage{package.imports, package.types, {sourceId4}}),
                 QmlDesigner::PropertyNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesChangeAliasDeclarationsTypeName)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[2].typeName = Storage::ImportedType{"Obj2"};
    importsSourceId3.emplace_back(pathToModuleId, Storage::Version{}, sourceId3);

    storage.synchronize(SynchronizationPackage{importsSourceId3, {package.types[2]}, {sourceId3}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId1, "QQuickItem"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    fetchTypeId(sourceId1, "QQuickItem"),
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangeAliasDeclarationsPropertyName)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[2].aliasPropertyName = "children";

    storage.synchronize(SynchronizationPackage{importsSourceId3, {package.types[2]}, {sourceId3}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType(sourceId3,
                                "QAliasItem",
                                fetchTypeId(sourceId1, "QQuickItem"),
                                TypeAccessSemantics::Reference),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("items",
                                                  fetchTypeId(sourceId1, "QQuickItem"),
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("objects",
                                                  fetchTypeId(sourceId1, "QQuickItem"),
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("data",
                                                  fetchTypeId(sourceId2, "QObject"),
                                                  Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangeAliasDeclarationsToPropertyDeclaration)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations.pop_back();
    package.types[2].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::ImportedType{"QQuickItem"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize(SynchronizationPackage{importsSourceId3, {package.types[2]}, {sourceId3}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType(sourceId3,
                                "QAliasItem",
                                fetchTypeId(sourceId1, "QQuickItem"),
                                TypeAccessSemantics::Reference),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("items",
                                                  fetchTypeId(sourceId1, "QQuickItem"),
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("objects",
                                                  fetchTypeId(sourceId1, "QQuickItem"),
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("data",
                                                  fetchTypeId(sourceId2, "QObject"),
                                                  Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangePropertyDeclarationsToAliasDeclaration)
{
    auto package{createSynchronizationPackageWithAliases()};
    auto packageChanged = package;
    packageChanged.types[2].propertyDeclarations.pop_back();
    packageChanged.types[2].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::ImportedType{"QQuickItem"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});
    storage.synchronize(packageChanged);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId1, "QQuickItem"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    fetchTypeId(sourceId1, "QQuickItem"),
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangeAliasTargetPropertyDeclarationTraits)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[1].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly;

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType(sourceId3,
                                "QAliasItem",
                                fetchTypeId(sourceId1, "QQuickItem"),
                                TypeAccessSemantics::Reference),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("items",
                                                  fetchTypeId(sourceId1, "QQuickItem"),
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("objects",
                                                  fetchTypeId(sourceId2, "QObject"),
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("data",
                                                  fetchTypeId(sourceId2, "QObject"),
                                                  Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangeAliasTargetPropertyDeclarationTypeName)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[1].propertyDeclarations[0].typeName = Storage::ImportedType{"Item"};
    importsSourceId2.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId1, "QQuickItem"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    fetchTypeId(sourceId1, "QQuickItem"),
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    fetchTypeId(sourceId1, "QQuickItem"),
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesRemovePropertyDeclarationWithAnAliasThrows)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[1].propertyDeclarations.pop_back();

    ASSERT_THROW(storage.synchronize(
                     SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}}),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorage, SynchronizeTypesRemovePropertyDeclarationAndAlias)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[1].propertyDeclarations.pop_back();
    package.types[2].propertyDeclarations.pop_back();

    storage.synchronize(SynchronizationPackage{importsSourceId2 + importsSourceId3,
                                               {package.types[1], package.types[2]},
                                               {sourceId2, sourceId3}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId1, "QQuickItem"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    fetchTypeId(sourceId1, "QQuickItem"),
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("data",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesRemoveTypeWithAliasTargetPropertyDeclarationThrows)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[2].typeName = Storage::ImportedType{"Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId3);
    storage.synchronize(package);

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{sourceId4}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesRemoveTypeAndAliasPropertyDeclaration)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[2].typeName = Storage::ImportedType{"Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId3);
    storage.synchronize(package);
    package.types[2].propertyDeclarations.pop_back();

    storage.synchronize(SynchronizationPackage{importsSourceId1 + importsSourceId3,
                                               {package.types[0], package.types[2]},
                                               {sourceId1, sourceId3}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId1, "QQuickItem"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    fetchTypeId(sourceId1, "QQuickItem"),
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("data",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, UpdateAliasPropertyIfPropertyIsOverloaded)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::ImportedType{"QQuickItem"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType(sourceId3,
                                "QAliasItem",
                                fetchTypeId(sourceId1, "QQuickItem"),
                                TypeAccessSemantics::Reference),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("items",
                                                  fetchTypeId(sourceId1, "QQuickItem"),
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("objects",
                                                  fetchTypeId(sourceId1, "QQuickItem"),
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("data",
                                                  fetchTypeId(sourceId2, "QObject"),
                                                  Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, AliasPropertyIsOverloaded)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[0].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::ImportedType{"QQuickItem"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize(package);

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType(sourceId3,
                                "QAliasItem",
                                fetchTypeId(sourceId1, "QQuickItem"),
                                TypeAccessSemantics::Reference),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("items",
                                                  fetchTypeId(sourceId1, "QQuickItem"),
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("objects",
                                                  fetchTypeId(sourceId1, "QQuickItem"),
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("data",
                                                  fetchTypeId(sourceId2, "QObject"),
                                                  Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, UpdateAliasPropertyIfOverloadedPropertyIsRemoved)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[0].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::ImportedType{"QQuickItem"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});
    storage.synchronize(package);
    package.types[0].propertyDeclarations.pop_back();

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId1, "QQuickItem"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    fetchTypeId(sourceId1, "QQuickItem"),
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, RelinkAliasProperty)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::ImportedType{"Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);
    package.types[3].exportedTypes[0].moduleId = qtQuickModuleId;

    storage.synchronize(SynchronizationPackage{importsSourceId4, {package.types[3]}, {sourceId4}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId1, "QQuickItem"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    fetchTypeId(sourceId1, "QQuickItem"),
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    fetchTypeId(sourceId4, "QObject2"),
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, DoNotRelinkAliasPropertyForQualifiedImportedTypeName)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object2", Storage::Import{pathToModuleId, Storage::Version{}, sourceId2}};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);
    package.types[3].exportedTypes[0].moduleId = qtQuickModuleId;
    importsSourceId4.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId4);

    ASSERT_THROW(storage.synchronize(
                     SynchronizationPackage{importsSourceId4, {package.types[3]}, {sourceId4}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage,
       DoRelinkAliasPropertyForQualifiedImportedTypeNameEvenIfAnOtherSimilarTimeNameExists)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object2", Storage::Import{pathToModuleId, Storage::Version{}, sourceId2}};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    package.types.push_back(Storage::Type{"QObject2",
                                          Storage::ImportedType{},
                                          TypeAccessSemantics::Reference,
                                          sourceId5,
                                          {Storage::ExportedType{qtQuickModuleId, "Object2"},
                                           Storage::ExportedType{qtQuickModuleId, "Obj2"}}});
    package.updatedSourceIds.push_back(sourceId5);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId1, "QQuickItem"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    fetchTypeId(sourceId1, "QQuickItem"),
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    fetchTypeId(sourceId4, "QObject2"),
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, RelinkAliasPropertyReactToTypeNameChange)
{
    auto package{createSynchronizationPackageWithAliases2()};
    package.types[2].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"items", Storage::ImportedType{"Item"}, "children"});
    storage.synchronize(package);
    package.types[0].typeName = "QQuickItem2";

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId2, "QObject"),
                                  TypeAccessSemantics::Reference),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    fetchTypeId(sourceId1, "QQuickItem2"),
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, DoNotRelinkAliasPropertyForDeletedType)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::ImportedType{"Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);
    package.types[3].exportedTypes[0].moduleId = qtQuickModuleId;

    storage.synchronize(
        SynchronizationPackage{importsSourceId4, {package.types[3]}, {sourceId3, sourceId4}});

    ASSERT_THAT(storage.fetchTypes(),
                Not(Contains(IsStorageType(sourceId3,
                                           "QAliasItem",
                                           fetchTypeId(sourceId1, "QQuickItem"),
                                           TypeAccessSemantics::Reference))));
}

TEST_F(ProjectStorage, DoNotRelinkAliasPropertyForDeletedTypeAndPropertyType)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::ImportedType{"Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);
    package.types[0].prototype = Storage::ImportedType{};
    importsSourceId1.emplace_back(pathToModuleId, Storage::Version{}, sourceId1);
    importsSourceId4.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId4);
    package.types[0].propertyDeclarations[0].typeName = Storage::ImportedType{"Object2"};
    package.types[3].propertyDeclarations[0].typeName = Storage::ImportedType{"Item"};

    storage.synchronize(SynchronizationPackage{importsSourceId1 + importsSourceId4,
                                               {package.types[0], package.types[3]},
                                               {sourceId1, sourceId2, sourceId3, sourceId4}});

    ASSERT_THAT(storage.fetchTypes(), SizeIs(2));
}

TEST_F(ProjectStorage, DoNotRelinkAliasPropertyForDeletedTypeAndPropertyTypeNameChange)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::ImportedType{"Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);
    package.types[3].exportedTypes[0].moduleId = qtQuickModuleId;
    package.types[1].propertyDeclarations[0].typeName = Storage::ImportedType{"QObject"};
    importsSourceId4.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId4);

    storage.synchronize(SynchronizationPackage{importsSourceId2 + importsSourceId4,
                                               {package.types[1], package.types[3]},
                                               {sourceId2, sourceId3, sourceId4}});

    ASSERT_THAT(storage.fetchTypes(),
                Not(Contains(IsStorageType(sourceId3,
                                           "QAliasItem",
                                           fetchTypeId(sourceId1, "QQuickItem"),
                                           TypeAccessSemantics::Reference))));
}

TEST_F(ProjectStorage, DoNotRelinkPropertyTypeDoesNotExists)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::ImportedType{"Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{sourceId4}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, DoNotRelinkAliasPropertyTypeDoesNotExists)
{
    auto package{createSynchronizationPackageWithAliases2()};
    package.types[1].propertyDeclarations[0].typeName = Storage::ImportedType{"Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{sourceId1}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, ChangePrototypeTypeName)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[1].typeName = "QObject3";

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       fetchTypeId(sourceId2, "QObject3"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, ChangePrototypeTypeModuleId)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[1].exportedTypes[2].moduleId = qtQuickModuleId;

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       fetchTypeId(sourceId2, "QObject"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, ChangeQualifiedPrototypeTypeModuleIdThrows)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::QualifiedImportedType{"Object",
                                                                Storage::Import{qmlModuleId,
                                                                                Storage::Version{},
                                                                                sourceId1}};
    storage.synchronize(package);
    package.types[1].exportedTypes[0].moduleId = qtQuickModuleId;

    ASSERT_THROW(storage.synchronize(
                     SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, ChangeQualifiedPrototypeTypeModuleId)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::QualifiedImportedType{"Object",
                                                                Storage::Import{qmlModuleId,
                                                                                Storage::Version{},
                                                                                sourceId1}};
    storage.synchronize(package);
    package.types[1].exportedTypes[0].moduleId = qtQuickModuleId;
    package.types[0].prototype = Storage::QualifiedImportedType{"Object",
                                                                Storage::Import{qtQuickModuleId,
                                                                                Storage::Version{},
                                                                                sourceId1}};

    storage.synchronize(SynchronizationPackage{importsSourceId1 + importsSourceId2,
                                               {package.types[0], package.types[1]},
                                               {sourceId1, sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       fetchTypeId(sourceId2, "QObject"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, ChangePrototypeTypeNameAndModuleId)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::ImportedType{"Object"};
    package.types[0].propertyDeclarations[0].typeName = Storage::ImportedType{"Object"};
    storage.synchronize(package);
    package.types[1].exportedTypes[0].moduleId = qtQuickModuleId;
    package.types[1].exportedTypes[1].moduleId = qtQuickModuleId;
    package.types[1].exportedTypes[2].moduleId = qtQuickModuleId;
    package.types[1].exportedTypes[2].name = "QObject3";
    package.types[1].typeName = "QObject3";

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       fetchTypeId(sourceId2, "QObject3"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, ChangePrototypeTypeNameThrowsForWrongNativePrototupeTypeName)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::ImportedType{"Object"};
    storage.synchronize(package);
    package.types[1].exportedTypes[2].name = "QObject3";
    package.types[1].typeName = "QObject3";

    ASSERT_THROW(storage.synchronize(
                     SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, ThrowForPrototypeChainCycles)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[1].prototype = Storage::ImportedType{"Object2"};
    package.types.push_back(Storage::Type{"QObject2",
                                          Storage::ImportedType{"Item"},
                                          TypeAccessSemantics::Reference,
                                          sourceId3,
                                          {Storage::ExportedType{pathToModuleId, "Object2"},
                                           Storage::ExportedType{pathToModuleId, "Obj2"}}});
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId3);

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{package.imports,
                                                            package.types,
                                                            {sourceId1, sourceId2, sourceId3}}),
                 QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorage, ThrowForTypeIdAndPrototypeIdAreTheSame)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[1].prototype = Storage::ImportedType{"Object"};

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorage, ThrowForTypeIdAndPrototypeIdAreTheSameForRelinking)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[1].prototype = Storage::ImportedType{"Item"};
    package.types[1].typeName = "QObject2";
    importsSourceId2.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);

    ASSERT_THROW(storage.synchronize(
                     SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}}),
                 QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorage, RecursiveAliases)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(sourceId5,
                                             "QAliasItem2",
                                             fetchTypeId(sourceId2, "QObject"),
                                             TypeAccessSemantics::Reference),
                               Field(&Storage::Type::propertyDeclarations,
                                     ElementsAre(IsPropertyDeclaration(
                                         "objects",
                                         fetchTypeId(sourceId2, "QObject"),
                                         Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, RecursiveAliasesChangePropertyType)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};
    storage.synchronize(package);
    package.types[1].propertyDeclarations[0].typeName = Storage::ImportedType{"Object2"};
    importsSourceId2.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(sourceId5,
                                             "QAliasItem2",
                                             fetchTypeId(sourceId2, "QObject"),
                                             TypeAccessSemantics::Reference),
                               Field(&Storage::Type::propertyDeclarations,
                                     ElementsAre(IsPropertyDeclaration(
                                         "objects",
                                         fetchTypeId(sourceId4, "QObject2"),
                                         Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, UpdateAliasesAfterInjectingProperty)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::ImportedType{"Item"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(sourceId5,
                                             "QAliasItem2",
                                             fetchTypeId(sourceId2, "QObject"),
                                             TypeAccessSemantics::Reference),
                               Field(&Storage::Type::propertyDeclarations,
                                     ElementsAre(IsPropertyDeclaration(
                                         "objects",
                                         fetchTypeId(sourceId1, "QQuickItem"),
                                         Storage::PropertyDeclarationTraits::IsList
                                             | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, UpdateAliasesAfterChangeAliasToProperty)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations.clear();
    package.types[2].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::ImportedType{"Item"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize(SynchronizationPackage{importsSourceId3, {package.types[2]}, {sourceId3}});

    ASSERT_THAT(storage.fetchTypes(),
                AllOf(Contains(AllOf(IsStorageType(sourceId5,
                                                   "QAliasItem2",
                                                   fetchTypeId(sourceId2, "QObject"),
                                                   TypeAccessSemantics::Reference),
                                     Field(&Storage::Type::propertyDeclarations,
                                           ElementsAre(IsPropertyDeclaration(
                                               "objects",
                                               fetchTypeId(sourceId1, "QQuickItem"),
                                               Storage::PropertyDeclarationTraits::IsList
                                                   | Storage::PropertyDeclarationTraits::IsReadOnly,
                                               "objects"))))),
                      Contains(AllOf(IsStorageType(sourceId3,
                                                   "QAliasItem",
                                                   fetchTypeId(sourceId1, "QQuickItem"),
                                                   TypeAccessSemantics::Reference),
                                     Field(&Storage::Type::propertyDeclarations,
                                           ElementsAre(IsPropertyDeclaration(
                                               "objects",
                                               fetchTypeId(sourceId1, "QQuickItem"),
                                               Storage::PropertyDeclarationTraits::IsList
                                                   | Storage::PropertyDeclarationTraits::IsReadOnly,
                                               "")))))));
}

TEST_F(ProjectStorage, UpdateAliasesAfterChangePropertyToAlias)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};
    package.types[3].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly;
    storage.synchronize(package);
    package.types[1].propertyDeclarations.clear();
    package.types[1].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects", Storage::ImportedType{"Object2"}, "objects"});
    importsSourceId2.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(sourceId5,
                                             "QAliasItem2",
                                             fetchTypeId(sourceId2, "QObject"),
                                             TypeAccessSemantics::Reference),
                               Field(&Storage::Type::propertyDeclarations,
                                     ElementsAre(IsPropertyDeclaration(
                                         "objects",
                                         fetchTypeId(sourceId2, "QObject"),
                                         Storage::PropertyDeclarationTraits::IsList
                                             | Storage::PropertyDeclarationTraits::IsReadOnly,
                                         "objects"))))));
}

TEST_F(ProjectStorage, CheckForProtoTypeCycleThrows)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};
    package.types[1].propertyDeclarations.clear();
    package.types[1].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects", Storage::ImportedType{"AliasItem2"}, "objects"});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::AliasChainCycle);
}

TEST_F(ProjectStorage, CheckForProtoTypeCycleAfterUpdateThrows)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};
    storage.synchronize(package);
    package.types[1].propertyDeclarations.clear();
    package.types[1].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects", Storage::ImportedType{"AliasItem2"}, "objects"});
    importsSourceId2.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);

    ASSERT_THROW(storage.synchronize(
                     SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}}),
                 QmlDesigner::AliasChainCycle);
}

TEST_F(ProjectStorage, QualifiedPrototype)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::QualifiedImportedType{"Object",
                                                                Storage::Import{qmlModuleId,
                                                                                Storage::Version{},
                                                                                sourceId1}};
    package.types.push_back(Storage::Type{"QQuickObject",
                                          Storage::ImportedType{},
                                          TypeAccessSemantics::Reference,
                                          sourceId3,
                                          {Storage::ExportedType{qtQuickModuleId, "Object"}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       fetchTypeId(sourceId2, "QObject"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, QualifiedPrototypeUpperDownTheModuleChainThrows)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::QualifiedImportedType{"Object",
                                                                Storage::Import{qtQuickModuleId,
                                                                                Storage::Version{},
                                                                                sourceId1}};

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, QualifiedPrototypeUpperInTheModuleChain)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::QualifiedImportedType{"Object",
                                                                Storage::Import{qtQuickModuleId,
                                                                                Storage::Version{},
                                                                                sourceId1}};
    package.types.push_back(Storage::Type{"QQuickObject",
                                          Storage::ImportedType{},
                                          TypeAccessSemantics::Reference,
                                          sourceId3,
                                          {Storage::ExportedType{qtQuickModuleId, "Object"}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       fetchTypeId(sourceId3, "QQuickObject"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, QualifiedPrototypeWithWrongVersionThrows)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::QualifiedImportedType{"Object",
                                                                Storage::Import{qmlModuleId,
                                                                                Storage::Version{4},
                                                                                sourceId1}};
    package.types.push_back(Storage::Type{"QQuickObject",
                                          Storage::ImportedType{},
                                          TypeAccessSemantics::Reference,
                                          sourceId3,
                                          {Storage::ExportedType{qtQuickModuleId, "Object"}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, QualifiedPrototypeWithVersion)
{
    auto package{createSimpleSynchronizationPackage()};
    package.imports[0].version = Storage::Version{2};
    package.types[0].prototype = Storage::QualifiedImportedType{"Object", package.imports[0]};
    package.types.push_back(Storage::Type{"QQuickObject",
                                          Storage::ImportedType{},
                                          TypeAccessSemantics::Reference,
                                          sourceId3,
                                          {Storage::ExportedType{qtQuickModuleId, "Object"}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       fetchTypeId(sourceId2, "QObject"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, QualifiedPrototypeWithVersionInTheProtoTypeChain)
{
    auto package{createSimpleSynchronizationPackage()};
    package.imports[2].version = Storage::Version{2};
    package.types[0].prototype = Storage::QualifiedImportedType{"Object", package.imports[2]};
    package.types[0].exportedTypes[0].version = Storage::Version{2};
    package.types.push_back(
        Storage::Type{"QQuickObject",
                      Storage::ImportedType{},
                      TypeAccessSemantics::Reference,
                      sourceId3,
                      {Storage::ExportedType{qtQuickModuleId, "Object", Storage::Version{2}}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       fetchTypeId(sourceId3, "QQuickObject"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, QualifiedPrototypeWithVersionDownTheProtoTypeChainThrows)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::QualifiedImportedType{"Object",
                                                                Storage::Import{qtQuickModuleId,
                                                                                Storage::Version{2},
                                                                                sourceId1}};

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, QualifiedPropertyDeclarationTypeName)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};
    package.types.push_back(Storage::Type{"QQuickObject",
                                          Storage::ImportedType{},
                                          TypeAccessSemantics::Reference,
                                          sourceId3,
                                          {Storage::ExportedType{qtQuickModuleId, "Object"}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    Field(&Storage::Type::propertyDeclarations,
                          Contains(IsPropertyDeclaration("data",
                                                         fetchTypeId(sourceId2, "QObject"),
                                                         Storage::PropertyDeclarationTraits::IsList)))));
}

TEST_F(ProjectStorage, QualifiedPropertyDeclarationTypeNameDownTheModuleChainThrows)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, QualifiedPropertyDeclarationTypeNameInTheModuleChain)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};
    package.types.push_back(Storage::Type{"QQuickObject",
                                          Storage::ImportedType{},
                                          TypeAccessSemantics::Reference,
                                          sourceId3,
                                          {Storage::ExportedType{qtQuickModuleId, "Object"}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    Field(&Storage::Type::propertyDeclarations,
                          Contains(IsPropertyDeclaration("data",
                                                         fetchTypeId(sourceId3, "QQuickObject"),
                                                         Storage::PropertyDeclarationTraits::IsList)))));
}

TEST_F(ProjectStorage, QualifiedPropertyDeclarationTypeNameWithVersion)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{2}, sourceId1}};
    package.imports.emplace_back(qmlModuleId, Storage::Version{2}, sourceId1);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    Field(&Storage::Type::propertyDeclarations,
                          Contains(IsPropertyDeclaration("data",
                                                         fetchTypeId(sourceId2, "QObject"),
                                                         Storage::PropertyDeclarationTraits::IsList)))));
}

TEST_F(ProjectStorage, ChangePropertyTypeModuleIdWithQualifiedTypeThrows)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};
    storage.synchronize(package);
    package.types[1].exportedTypes[0].moduleId = qtQuickModuleId;

    ASSERT_THROW(storage.synchronize(
                     SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, ChangePropertyTypeModuleIdWithQualifiedType)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};
    package.types[1].exportedTypes[0].moduleId = qtQuickModuleId;
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(sourceId1,
                                             "QQuickItem",
                                             fetchTypeId(sourceId2, "QObject"),
                                             TypeAccessSemantics::Reference),
                               Field(&Storage::Type::propertyDeclarations,
                                     Contains(IsPropertyDeclaration(
                                         "data",
                                         fetchTypeId(sourceId2, "QObject"),
                                         Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, AddFileStatuses)
{
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};

    storage.synchronize(SynchronizationPackage{{sourceId1, sourceId2}, {fileStatus1, fileStatus2}});

    ASSERT_THAT(convert(storage.fetchAllFileStatuses()),
                UnorderedElementsAre(fileStatus1, fileStatus2));
}

TEST_F(ProjectStorage, RemoveFileStatus)
{
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    storage.synchronize(SynchronizationPackage{{sourceId1, sourceId2}, {fileStatus1, fileStatus2}});

    storage.synchronize(SynchronizationPackage{{sourceId1, sourceId2}, {fileStatus1}});

    ASSERT_THAT(convert(storage.fetchAllFileStatuses()), UnorderedElementsAre(fileStatus1));
}

TEST_F(ProjectStorage, UpdateFileStatus)
{
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    FileStatus fileStatus2b{sourceId2, 102, 102};
    storage.synchronize(SynchronizationPackage{{sourceId1, sourceId2}, {fileStatus1, fileStatus2}});

    storage.synchronize(SynchronizationPackage{{sourceId1, sourceId2}, {fileStatus1, fileStatus2b}});

    ASSERT_THAT(convert(storage.fetchAllFileStatuses()),
                UnorderedElementsAre(fileStatus1, fileStatus2b));
}

TEST_F(ProjectStorage, ThrowForInvalidSourceIdInFileStatus)
{
    FileStatus fileStatus1{SourceId{}, 100, 100};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{sourceId1}, {fileStatus1}}),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorage, FetchAllFileStatuses)
{
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    storage.synchronize(SynchronizationPackage{{sourceId1, sourceId2}, {fileStatus1, fileStatus2}});

    auto fileStatuses = convert(storage.fetchAllFileStatuses());

    ASSERT_THAT(fileStatuses, ElementsAre(fileStatus1, fileStatus2));
}

TEST_F(ProjectStorage, FetchAllFileStatusesReverse)
{
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    storage.synchronize(SynchronizationPackage{{sourceId1, sourceId2}, {fileStatus2, fileStatus1}});

    auto fileStatuses = convert(storage.fetchAllFileStatuses());

    ASSERT_THAT(fileStatuses, ElementsAre(fileStatus1, fileStatus2));
}

TEST_F(ProjectStorage, FetchFileStatus)
{
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    storage.synchronize(SynchronizationPackage{{sourceId1, sourceId2}, {fileStatus1, fileStatus2}});

    auto fileStatus = storage.fetchFileStatus(sourceId1);

    ASSERT_THAT(fileStatus, Eq(fileStatus1));
}

TEST_F(ProjectStorage, SynchronizeTypesWithoutTypeName)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[3].typeName.clear();
    package.types[3].prototype = Storage::ImportedType{"Object"};

    storage.synchronize(SynchronizationPackage{importsSourceId4, {package.types[3]}, {sourceId4}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(sourceId4,
                                             "QObject2",
                                             fetchTypeId(sourceId2, "QObject"),
                                             TypeAccessSemantics::Reference),
                               Field(&Storage::Type::exportedTypes,
                                     UnorderedElementsAre(IsExportedType("Object2"),
                                                          IsExportedType("Obj2"))))));
}

TEST_F(ProjectStorage, FetchByMajorVersionForImportedType)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Type type{"Item",
                       Storage::ImportedType{"Object"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{1}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, FetchByMajorVersionForQualifiedImportedType)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{1}, sourceId2};
    Storage::Type type{"Item",
                       Storage::QualifiedImportedType{"Object", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, FetchByMajorVersionAndMinorVersionForImportedType)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Type type{"Item",
                       Storage::ImportedType{"Obj"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{1, 2}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, FetchByMajorVersionAndMinorVersionForQualifiedImportedType)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{1, 2}, sourceId2};
    Storage::Type type{"Item",
                       Storage::QualifiedImportedType{"Obj", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage,
       FetchByMajorVersionAndMinorVersionForImportedTypeIfMinorVersionIsNotExportedThrows)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Type type{"Item",
                       Storage::ImportedType{"Object"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{1, 1}, sourceId2};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage,
       FetchByMajorVersionAndMinorVersionForQualifiedImportedTypeIfMinorVersionIsNotExportedThrows)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{1, 1}, sourceId2};
    Storage::Type type{"Item",
                       Storage::QualifiedImportedType{"Object", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, FetchLowMinorVersionForImportedTypeThrows)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Type type{"Item",
                       Storage::ImportedType{"Obj"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{1, 1}, sourceId2};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, FetchLowMinorVersionForQualifiedImportedTypeThrows)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{1, 1}, sourceId2};
    Storage::Type type{"Item",
                       Storage::QualifiedImportedType{"Obj", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, FetchHigherMinorVersionForImportedType)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Type type{"Item",
                       Storage::ImportedType{"Obj"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{1, 3}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, FetchHigherMinorVersionForQualifiedImportedType)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{1, 3}, sourceId2};
    Storage::Type type{"Item",
                       Storage::QualifiedImportedType{"Obj", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, FetchDifferentMajorVersionForImportedTypeThrows)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Type type{"Item",
                       Storage::ImportedType{"Obj"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{3, 1}, sourceId2};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, FetchDifferentMajorVersionForQualifiedImportedTypeThrows)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{3, 1}, sourceId2};
    Storage::Type type{"Item",
                       Storage::QualifiedImportedType{"Obj", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, FetchOtherTypeByDifferentVersionForImportedType)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Type type{"Item",
                       Storage::ImportedType{"Obj"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{2, 3}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject2"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, FetchOtherTypeByDifferentVersionForQualifiedImportedType)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{2, 3}, sourceId2};
    Storage::Type type{"Item",
                       Storage::QualifiedImportedType{"Obj", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject2"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, FetchHighestVersionForImportWithoutVersionForImportedType)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Type type{"Item",
                       Storage::ImportedType{"Obj"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject4"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, FetchHighestVersionForImportWithoutVersionForQualifiedImportedType)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{}, sourceId2};
    Storage::Type type{"Item",
                       Storage::QualifiedImportedType{"Obj", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject4"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, FetchHighestVersionForImportWithMajorVersionForImportedType)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Type type{"Item",
                       Storage::ImportedType{"Obj"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{2}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject3"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, FetchHighestVersionForImportWithMajorVersionForQualifiedImportedType)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{2}, sourceId2};
    Storage::Type type{"Item",
                       Storage::QualifiedImportedType{"Obj", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject3"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, FetchExportedTypeWithoutVersionFirstForImportedType)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Type type{"Item",
                       Storage::ImportedType{"BuiltInObj"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, FetchExportedTypeWithoutVersionFirstForQualifiedImportedType)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{}, sourceId2};
    Storage::Type type{"Item",
                       Storage::QualifiedImportedType{"BuiltInObj", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject"),
                                       TypeAccessSemantics::Reference)));
}

TEST_F(ProjectStorage, EnsureThatPropertiesForRemovedTypesAreNotAnymoreRelinked)
{
    Storage::Type type{"QObject",
                       Storage::ImportedType{""},
                       TypeAccessSemantics::Reference,
                       sourceId1,
                       {Storage::ExportedType{qmlModuleId, "Object", Storage::Version{}}},
                       {Storage::PropertyDeclaration{"data",
                                                     Storage::ImportedType{"Object"},
                                                     Storage::PropertyDeclarationTraits::IsList}}};
    Storage::Import import{qmlModuleId, Storage::Version{}, sourceId1};
    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId1}});

    ASSERT_NO_THROW(storage.synchronize(SynchronizationPackage{{sourceId1}}));
}

TEST_F(ProjectStorage, EnsureThatPrototypesForRemovedTypesAreNotAnymoreRelinked)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    ASSERT_NO_THROW(storage.synchronize(SynchronizationPackage{{sourceId1, sourceId2}}));
}

TEST_F(ProjectStorage, MinimalUpdates)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    Storage::Type quickType{"QQuickItem",
                            {},
                            TypeAccessSemantics::Reference,
                            sourceId1,
                            {Storage::ExportedType{qtQuickModuleId, "Item", Storage::Version{2, 0}},
                             Storage::ExportedType{qtQuickNativeModuleId, "QQuickItem"}},
                            {},
                            {},
                            {},
                            {},
                            Storage::ChangeLevel::Minimal};

    storage.synchronize(SynchronizationPackage{{quickType}});

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                fetchTypeId(sourceId2, "QObject"),
                                TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item", 2, 0),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickItem"))),
                  Field(&Storage::Type::propertyDeclarations, Not(IsEmpty())),
                  Field(&Storage::Type::functionDeclarations, Not(IsEmpty())),
                  Field(&Storage::Type::signalDeclarations, Not(IsEmpty())),
                  Field(&Storage::Type::enumerationDeclarations, Not(IsEmpty())))));
}

TEST_F(ProjectStorage, GetModuleId)
{
    auto id = storage.moduleId("Qml");

    ASSERT_TRUE(id);
}

TEST_F(ProjectStorage, GetSameModuleIdAgain)
{
    auto initialId = storage.moduleId("Qml");

    auto id = storage.moduleId("Qml");

    ASSERT_THAT(id, Eq(initialId));
}

TEST_F(ProjectStorage, ModuleNameThrowsIfIdIsInvalid)
{
    ASSERT_THROW(storage.moduleName(ModuleId{}), QmlDesigner::ModuleDoesNotExists);
}

TEST_F(ProjectStorage, ModuleNameThrowsIfIdDoesNotExists)
{
    ASSERT_THROW(storage.moduleName(ModuleId{222}), QmlDesigner::ModuleDoesNotExists);
}

TEST_F(ProjectStorage, GetModuleName)
{
    auto id = storage.moduleId("Qml");

    auto name = storage.moduleName(id);

    ASSERT_THAT(name, Eq("Qml"));
}

TEST_F(ProjectStorage, PopulateModuleCache)
{
    auto id = storage.moduleId("Qml");

    QmlDesigner::ProjectStorage<Sqlite::Database> newStorage{database, database.isInitialized()};

    ASSERT_THAT(newStorage.moduleName(id), Eq("Qml"));
}

TEST_F(ProjectStorage, AddProjectDataes)
{
    Storage::ProjectData projectData1{qmlProjectSourceId,
                                      sourceId1,
                                      qmlModuleId,
                                      Storage::FileType::QmlDocument};
    Storage::ProjectData projectData2{qmlProjectSourceId,
                                      sourceId2,
                                      qmlModuleId,
                                      Storage::FileType::QmlDocument};
    Storage::ProjectData projectData3{qtQuickProjectSourceId,
                                      sourceId3,
                                      qtQuickModuleId,
                                      Storage::FileType::QmlTypes};

    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {projectData1, projectData2, projectData3}});

    ASSERT_THAT(storage.fetchProjectDatas({qmlProjectSourceId, qtQuickProjectSourceId}),
                UnorderedElementsAre(projectData1, projectData2, projectData3));
}

TEST_F(ProjectStorage, RemoveProjectData)
{
    Storage::ProjectData projectData1{qmlProjectSourceId,
                                      sourceId1,
                                      qmlModuleId,
                                      Storage::FileType::QmlDocument};
    Storage::ProjectData projectData2{qmlProjectSourceId,
                                      sourceId2,
                                      qmlModuleId,
                                      Storage::FileType::QmlDocument};
    Storage::ProjectData projectData3{qtQuickProjectSourceId,
                                      sourceId3,
                                      qtQuickModuleId,
                                      Storage::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {projectData1, projectData2, projectData3}});

    storage.synchronize(
        SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId}, {projectData1}});

    ASSERT_THAT(storage.fetchProjectDatas({qmlProjectSourceId, qtQuickProjectSourceId}),
                UnorderedElementsAre(projectData1));
}

TEST_F(ProjectStorage, UpdateProjectDataFileType)
{
    Storage::ProjectData projectData1{qmlProjectSourceId,
                                      sourceId1,
                                      qmlModuleId,
                                      Storage::FileType::QmlDocument};
    Storage::ProjectData projectData2{qmlProjectSourceId,
                                      sourceId2,
                                      qmlModuleId,
                                      Storage::FileType::QmlDocument};
    Storage::ProjectData projectData2b{qmlProjectSourceId,
                                       sourceId2,
                                       qmlModuleId,
                                       Storage::FileType::QmlTypes};
    Storage::ProjectData projectData3{qtQuickProjectSourceId,
                                      sourceId3,
                                      qtQuickModuleId,
                                      Storage::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {projectData1, projectData2, projectData3}});

    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {projectData1, projectData2b}});

    ASSERT_THAT(storage.fetchProjectDatas({qmlProjectSourceId, qtQuickProjectSourceId}),
                UnorderedElementsAre(projectData1, projectData2b, projectData3));
}

TEST_F(ProjectStorage, UpdateProjectDataModuleId)
{
    Storage::ProjectData projectData1{qmlProjectSourceId,
                                      sourceId1,
                                      qmlModuleId,
                                      Storage::FileType::QmlDocument};
    Storage::ProjectData projectData2{qmlProjectSourceId,
                                      sourceId3,
                                      qmlModuleId,
                                      Storage::FileType::QmlDocument};
    Storage::ProjectData projectData2b{qmlProjectSourceId,
                                       sourceId3,
                                       qtQuickModuleId,
                                       Storage::FileType::QmlDocument};
    Storage::ProjectData projectData3{qtQuickProjectSourceId,
                                      sourceId2,
                                      qtQuickModuleId,
                                      Storage::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {projectData1, projectData2, projectData3}});

    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {projectData1, projectData2b}});

    ASSERT_THAT(storage.fetchProjectDatas({qmlProjectSourceId, qtQuickProjectSourceId}),
                UnorderedElementsAre(projectData1, projectData2b, projectData3));
}

TEST_F(ProjectStorage, ThrowForInvalidSourceIdInProjectData)
{
    Storage::ProjectData projectData1{qmlProjectSourceId,
                                      SourceId{},
                                      qmlModuleId,
                                      Storage::FileType::QmlDocument};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {projectData1}}),
                 QmlDesigner::ProjectDataHasInvalidSourceId);
}

TEST_F(ProjectStorage, ThrowForInvalidModuleIdInProjectData)
{
    Storage::ProjectData projectData1{qmlProjectSourceId,
                                      sourceId1,
                                      ModuleId{},
                                      Storage::FileType::QmlDocument};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {projectData1}}),
                 QmlDesigner::ProjectDataHasInvalidModuleId);
}

TEST_F(ProjectStorage, ThrowForUpdatingWithInvalidModuleIdInProjectData)
{
    Storage::ProjectData projectData1{qmlProjectSourceId,
                                      sourceId1,
                                      qmlModuleId,
                                      Storage::FileType::QmlDocument};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {projectData1}});
    projectData1.moduleId = ModuleId{};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {projectData1}}),
                 QmlDesigner::ProjectDataHasInvalidModuleId);
}

TEST_F(ProjectStorage, ThrowForUpdatingWithInvalidProjectSourceIdInProjectData)
{
    Storage::ProjectData projectData1{SourceId{}, sourceId1, qmlModuleId, Storage::FileType::QmlDocument};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {projectData1}}),
                 QmlDesigner::ProjectDataHasInvalidProjectSourceId);
}

TEST_F(ProjectStorage, FetchProjectDatasByModuleIds)
{
    Storage::ProjectData projectData1{qmlProjectSourceId,
                                      sourceId1,
                                      qmlModuleId,
                                      Storage::FileType::QmlDocument};
    Storage::ProjectData projectData2{qmlProjectSourceId,
                                      sourceId2,
                                      qmlModuleId,
                                      Storage::FileType::QmlDocument};
    Storage::ProjectData projectData3{qtQuickProjectSourceId,
                                      sourceId3,
                                      qtQuickModuleId,
                                      Storage::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {projectData1, projectData2, projectData3}});

    auto projectDatas = storage.fetchProjectDatas({qmlProjectSourceId, qtQuickProjectSourceId});

    ASSERT_THAT(projectDatas, UnorderedElementsAre(projectData1, projectData2, projectData3));
}

TEST_F(ProjectStorage, FetchProjectDatasByModuleId)
{
    Storage::ProjectData projectData1{qmlProjectSourceId,
                                      sourceId1,
                                      qmlModuleId,
                                      Storage::FileType::QmlDocument};
    Storage::ProjectData projectData2{qmlProjectSourceId,
                                      sourceId2,
                                      qmlModuleId,
                                      Storage::FileType::QmlDocument};
    Storage::ProjectData projectData3{qtQuickProjectSourceId,
                                      sourceId3,
                                      qtQuickModuleId,
                                      Storage::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {projectData1, projectData2, projectData3}});

    auto projectData = storage.fetchProjectDatas(qmlProjectSourceId);

    ASSERT_THAT(projectData, UnorderedElementsAre(projectData1, projectData2));
}

TEST_F(ProjectStorage, ExcludeExportedTypes)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].exportedTypes.clear();
    package.types[0].changeLevel = Storage::ChangeLevel::ExcludeExportedTypes;

    storage.synchronize(std::move(package));

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                fetchTypeId(sourceId2, "QObject"),
                                TypeAccessSemantics::Reference),
                  Field(&Storage::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickItem"))))));
}

} // namespace
