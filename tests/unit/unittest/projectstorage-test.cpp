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

MATCHER_P5(IsStorageType,
           module,
           typeName,
           prototype,
           accessSemantics,
           sourceId,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Type{module, typeName, prototype, accessSemantics, sourceId}))
{
    const Storage::Type &type = arg;

    return type.module == module && type.typeName == typeName
           && type.accessSemantics == accessSemantics && type.sourceId == sourceId
           && Storage::TypeName{prototype} == type.prototype;
}

MATCHER_P4(IsStorageTypeWithInvalidSourceId,
           module,
           typeName,
           prototype,
           accessSemantics,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Type{module, typeName, prototype, accessSemantics, SourceId{}}))
{
    const Storage::Type &type = arg;

    return type.module == module && type.typeName == typeName && type.prototype == prototype
           && type.accessSemantics == accessSemantics && !type.sourceId.isValid();
}

MATCHER_P(IsExportedType,
          name,
          std::string(negation ? "isn't " : "is ") + PrintToString(Storage::ExportedType{name}))
{
    const Storage::ExportedType &type = arg;

    return type.name == name;
}

MATCHER_P3(IsPropertyDeclaration,
           name,
           typeName,
           traits,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::PropertyDeclaration{name, typeName, traits}))
{
    const Storage::PropertyDeclaration &propertyDeclaration = arg;

    return propertyDeclaration.name == name
           && Storage::TypeName{typeName} == propertyDeclaration.typeName
           && propertyDeclaration.traits == traits;
}

MATCHER_P4(IsPropertyDeclaration,
           name,
           typeName,
           traits,
           aliasPropertyName,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::PropertyDeclaration(name, typeName, traits, aliasPropertyName)))
{
    const Storage::PropertyDeclaration &propertyDeclaration = arg;

    return propertyDeclaration.name == name
           && Utils::visit([&](auto &&v) -> bool { return v.name == typeName.name; },
                           propertyDeclaration.typeName)
           && propertyDeclaration.aliasPropertyName == aliasPropertyName
           && propertyDeclaration.traits == traits;
}

MATCHER_P2(IsModule,
           name,
           version,
           std::string(negation ? "isn't " : "is ") + PrintToString(Storage::Module{name, version}))
{
    const Storage::Module &module = arg;

    return module.name == name && module.version == version;
}

MATCHER_P3(IsModuleDependency,
           name,
           version,
           sourceId,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::ModuleDependency{name, version, sourceId}))
{
    const Storage::ModuleDependency &module = arg;

    return module.name == name && module.version == version && &module.sourceId == &sourceId;
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

    auto createTypes()
    {
        setUpModuleDependenciesAndDocuments();

        return Storage::Types{
            Storage::Type{
                Storage::Module{"QtQuick"},
                "QQuickItem",
                Storage::NativeType{"QObject"},
                TypeAccessSemantics::Reference,
                sourceId1,
                {Storage::ExportedType{"Item"}},
                {Storage::PropertyDeclaration{"data",
                                              Storage::NativeType{"QObject"},
                                              Storage::PropertyDeclarationTraits::IsList},
                 Storage::PropertyDeclaration{"children",
                                              Storage::ExportedType{"Item"},
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
                 Storage::SignalDeclaration{
                     "values",
                     {Storage::ParameterDeclaration{"arg1", "int"},
                      Storage::ParameterDeclaration{"arg2",
                                                    "QObject",
                                                    Storage::PropertyDeclarationTraits::IsPointer},
                      Storage::ParameterDeclaration{"arg3", "string"}}}},
                {Storage::EnumerationDeclaration{"Enum",
                                                 {Storage::EnumeratorDeclaration{"Foo"},
                                                  Storage::EnumeratorDeclaration{"Bar", 32}}},
                 Storage::EnumerationDeclaration{"Type",
                                                 {Storage::EnumeratorDeclaration{"Foo"},
                                                  Storage::EnumeratorDeclaration{"Poo", 12}}}}},
            Storage::Type{Storage::Module{"Qml", 2},
                          "QObject",
                          Storage::NativeType{},
                          TypeAccessSemantics::Reference,
                          sourceId2,
                          {Storage::ExportedType{"Object"}, Storage::ExportedType{"Obj"}}}};
    }

    auto createTypesWithExportedTypeNamesOnly()
    {
        auto types = createTypes();

        types[0].prototype = Storage::ExportedType{"Object"};
        types[0].propertyDeclarations[0].typeName = Storage::ExportedType{"Object"};

        return types;
    }

    auto createTypesWithAliases()
    {
        auto types = createTypes();

        types[1].propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"objects",
                                         Storage::NativeType{"QObject"},
                                         Storage::PropertyDeclarationTraits::IsList});

        types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                      "QAliasItem",
                                      Storage::ExportedType{"Item"},
                                      TypeAccessSemantics::Reference,
                                      sourceId3,
                                      {Storage::ExportedType{"AliasItem"}}});
        types.back().propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"data",
                                         Storage::NativeType{"QObject"},
                                         Storage::PropertyDeclarationTraits::IsList});
        types.back().propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"items", Storage::ExportedType{"Item"}, "children"});
        types.back().propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"objects", Storage::ExportedType{"Item"}, "objects"});

        types.push_back(
            Storage::Type{Storage::Module{"/path/to"},
                          "QObject2",
                          Storage::NativeType{},
                          TypeAccessSemantics::Reference,
                          sourceId4,
                          {Storage::ExportedType{"Object2"}, Storage::ExportedType{"Obj2"}}});
        types[3].propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"objects",
                                         Storage::NativeType{"QObject"},
                                         Storage::PropertyDeclarationTraits::IsList});

        return types;
    }

    auto createTypesWithRecursiveAliases()
    {
        auto types = createTypesWithAliases();
        types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                      "QAliasItem2",
                                      Storage::ExportedType{"Object"},
                                      TypeAccessSemantics::Reference,
                                      sourceId5,
                                      {Storage::ExportedType{"AliasItem2"}}});

        types.back().propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"objects", Storage::ExportedType{"AliasItem"}, "objects"});

        return types;
    }

    auto createTypesWithAliases2()
    {
        auto types = createTypesWithAliases();
        types[2].prototype = Storage::NativeType{"QObject"};
        types[2].propertyDeclarations.erase(std::next(types[2].propertyDeclarations.begin()));

        return types;
    }

    auto createModuleDependencies()
    {
        moduleSourceId1 = sourcePathCache.sourceId(modulePath1);
        moduleSourceId2 = sourcePathCache.sourceId(modulePath2);
        moduleSourceId3 = sourcePathCache.sourceId(modulePath3);
        moduleSourceId5 = sourcePathCache.sourceId("/path/to/.");

        return Storage::ModuleDependencies{
            Storage::ModuleDependency{"Qml", Storage::VersionNumber{2}, moduleSourceId1, {}},
            Storage::ModuleDependency{"QtQuick",
                                      Storage::VersionNumber{},
                                      moduleSourceId2,
                                      {Storage::Module{"Qml", Storage::VersionNumber{2}}}},
            Storage::ModuleDependency{"/path/to",
                                      Storage::VersionNumber{},
                                      moduleSourceId5,
                                      {Storage::Module{"QtQuick"},
                                       Storage::Module{"Qml", Storage::VersionNumber{2}}}}};
    }

    Storage::Modules createModules()
    {
        return Storage::Modules{Storage::Module{"Qml", Storage::VersionNumber{2}},
                                Storage::Module{"QtQuick", Storage::VersionNumber{}},
                                Storage::Module{"/path/to", Storage::VersionNumber{}}};
    }

    auto createModuleDependencies2()
    {
        moduleSourceId4 = sourcePathCache.sourceId(modulePath4);

        auto newModuleDependencies = createModuleDependencies();
        newModuleDependencies.push_back(
            Storage::ModuleDependency{"Qml2", Storage::VersionNumber{3}, moduleSourceId4, {}});

        return newModuleDependencies;
    }

    void setUpSourceIds()
    {
        sourceId1 = sourcePathCache.sourceId(path1);
        sourceId2 = sourcePathCache.sourceId(path2);
        sourceId3 = sourcePathCache.sourceId(path3);
        sourceId4 = sourcePathCache.sourceId(path4);
        sourceId5 = sourcePathCache.sourceId(path5);
    }

    void setUpModuleDependenciesAndDocuments()
    {
        setUpModules();
        setUpSourceIds();

        moduleDependencies = createModuleDependencies();

        documents = {Storage::Document{sourceId1, modules},
                     Storage::Document{sourceId2, modules},
                     Storage::Document{sourceId3, modules},
                     Storage::Document{sourceId4, modules},
                     Storage::Document{sourceId5, modules}};

        storage.synchronize(moduleDependencies,
                            documents,
                            {},
                            {sourceId1,
                             sourceId2,
                             sourceId3,
                             sourceId4,
                             sourceId5,
                             moduleSourceId1,
                             moduleSourceId2,
                             moduleSourceId5},
                            {});
        moduleIds = storage.fetchModuleIds(modules);
        moduleId1 = moduleIds[0];
        moduleId2 = moduleIds[1];
        moduleId3 = moduleIds[2];
    }

    void setUpModules() { modules = createModules(); }

    template<typename Range>
    static FileStatuses convert(const Range &range)
    {
        return FileStatuses(range.begin(), range.end());
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    QmlDesigner::ProjectStorage<Sqlite::Database> storage{database, database.isInitialized()};
    QmlDesigner::SourcePathCache<QmlDesigner::ProjectStorage<Sqlite::Database>> sourcePathCache{
        storage};
    QmlDesigner::SourcePathView path1{"/path1/to"};
    QmlDesigner::SourcePathView path2{"/path2/to"};
    QmlDesigner::SourcePathView path3{"/path3/to"};
    QmlDesigner::SourcePathView path4{"/path4/to"};
    QmlDesigner::SourcePathView path5{"/path5/to"};
    SourceId sourceId1;
    SourceId sourceId2;
    SourceId sourceId3;
    SourceId sourceId4;
    SourceId sourceId5;
    QmlDesigner::SourcePathView modulePath1{"/module/path1/to"};
    QmlDesigner::SourcePathView modulePath2{"/module/path2/to"};
    QmlDesigner::SourcePathView modulePath3{"/module/aaaa/to"};
    QmlDesigner::SourcePathView modulePath4{"/module/ooo/to"};
    SourceId moduleSourceId1;
    SourceId moduleSourceId2;
    SourceId moduleSourceId3;
    SourceId moduleSourceId4;
    SourceId moduleSourceId5;
    Storage::Modules modules;
    ModuleId moduleId1;
    ModuleId moduleId2;
    ModuleId moduleId3;
    Storage::ModuleDependencies moduleDependencies;
    Storage::Documents documents;
    QmlDesigner::ModuleIds moduleIds;
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
    auto sourceContexts = storage.fetchAllSourceContexts();

    ASSERT_THAT(toValues(sourceContexts), IsEmpty());
}

TEST_F(ProjectStorage, FetchAllSourceContexts)
{
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
    Storage::Types types{createTypes()};

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj")))),
                                     AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                                         "QQuickItem",
                                                         Storage::NativeType{"QObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Item"))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddsNewTypesWithExportedPrototypeName)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExportedType{"Object"};

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj")))),
                                     AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                                         "QQuickItem",
                                                         Storage::NativeType{"QObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Item"))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddsNewTypesThrowsWithWrongExportedPrototypeName)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExportedType{"Objec"};

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesAddsNewTypesWithMissingModuleAndExportedPrototypeName)
{
    Storage::Types types{createTypes()};
    types.push_back(Storage::Type{Storage::Module{"/path/to"},
                                  "QObject2",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId4,
                                  {Storage::ExportedType{"Object2"}, Storage::ExportedType{"Obj2"}}});
    storage.synchronize({}, {Storage::Document{sourceId1, {modules[0]}}}, {}, {sourceId1}, {});
    types[1].prototype = Storage::ExportedType{"Object2"};

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesAddsNewTypesWithMissingModule)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {Storage::Document{sourceId1, {modules[0]}}}, {}, {sourceId1}, {});

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesAddsNewTypesReverseOrder)
{
    Storage::Types types{createTypes()};
    std::reverse(types.begin(), types.end());

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj")))),
                                     AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                                         "QQuickItem",
                                                         Storage::NativeType{"QObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Item"))))));
}

TEST_F(ProjectStorage, SynchronizeTypesOverwritesTypeAccessSemantics)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[0].accessSemantics = TypeAccessSemantics::Value;
    types[1].accessSemantics = TypeAccessSemantics::Value;

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Value,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj")))),
                                     AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                                         "QQuickItem",
                                                         Storage::NativeType{"QObject"},
                                                         TypeAccessSemantics::Value,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Item"))))));
}

TEST_F(ProjectStorage, SynchronizeTypesOverwritesSources)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[0].sourceId = sourceId3;
    types[1].sourceId = sourceId4;

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId4),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj")))),
                                     AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                                         "QQuickItem",
                                                         Storage::NativeType{"QObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId3),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Item"))))));
}

TEST_F(ProjectStorage, SynchronizeTypesInsertTypeIntoPrototypeChain)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[0].prototype = Storage::NativeType{"QQuickObject"};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{"QObject"},
                                  TypeAccessSemantics::Reference,
                                  sourceId1,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj")))),
                                     AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                                         "QQuickObject",
                                                         Storage::NativeType{"QObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object")))),
                                     AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                                         "QQuickItem",
                                                         Storage::NativeType{"QQuickObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Item"))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddExplicitPrototype)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Module{"QtQuick"}};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{"QObject"},
                                  TypeAccessSemantics::Reference,
                                  sourceId1,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj")))),
                                     AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                                         "QQuickObject",
                                                         Storage::NativeType{"QObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object")))),
                                     AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                                         "QQuickItem",
                                                         Storage::NativeType{"QQuickObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Item"))))));
}

TEST_F(ProjectStorage, SynchronizeTypesThrowsForMissingPrototype)
{
    setUpModuleDependenciesAndDocuments();
    sourceId1 = sourcePathCache.sourceId(path1);
    Storage::Types types{Storage::Type{Storage::Module{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1,
                                       {Storage::ExportedType{"Item"}}}};

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesThrowsForMissingModule)
{
    sourceId1 = sourcePathCache.sourceId(path1);
    Storage::Types types{Storage::Type{Storage::Module{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1,
                                       {Storage::ExportedType{"Item"}}}};

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1}, {}),
                 QmlDesigner::ModuleDoesNotExists);
}

TEST_F(ProjectStorage, TypeWithInvalidSourceIdThrows)
{
    Storage::Types types{Storage::Type{Storage::Module{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{""},
                                       TypeAccessSemantics::Reference,
                                       SourceId{},
                                       {Storage::ExportedType{"Item"}}}};

    ASSERT_THROW(storage.synchronize({}, {}, types, {}, {}), QmlDesigner::TypeHasInvalidSourceId);
}

TEST_F(ProjectStorage, DeleteTypeIfSourceIdIsSynchronized)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types.erase(types.begin());

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj"))))));
}

TEST_F(ProjectStorage, DontDeleteTypeIfSourceIdIsNotSynchronized)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types.pop_back();

    storage.synchronize({}, {}, types, {sourceId1}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj")))),
                                     AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                                         "QQuickItem",
                                                         Storage::NativeType{"QObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Item"))))));
}

TEST_F(ProjectStorage, UpdateExportedTypesIfTypeNameChanges)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[0].typeName = "QQuickItem2";

    storage.synchronize({}, {}, {types[0]}, {sourceId1}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj")))),
                                     AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                                         "QQuickItem2",
                                                         Storage::NativeType{"QObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Item"))))));
}

TEST_F(ProjectStorage, BreakingPrototypeChainByDeletingBaseComponentThrows)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types.pop_back();

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesAddPropertyDeclarations)
{
    Storage::Types types{createTypes()};

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                        "QQuickItem",
                                        Storage::NativeType{"QObject"},
                                        TypeAccessSemantics::Reference,
                                        sourceId1),
                          Field(&Storage::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("data",
                                                          Storage::NativeType{"QObject"},
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "children",
                                        Storage::NativeType{"QQuickItem"},
                                        Storage::PropertyDeclarationTraits::IsList
                                            | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddPropertyDeclarationsWithMissingModuleIdsForNativeTypes)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {Storage::Document{sourceId1, {}}}, {}, {sourceId1}, {});
    types[0].propertyDeclarations.pop_back();

    ASSERT_THROW(storage.synchronize({}, {}, types, {}, {}), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesAddPropertyDeclarationsWithMissingModuleIdsForExportedTypes)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {Storage::Document{sourceId1, {modules[0]}}}, {}, {sourceId1}, {});
    types[0].propertyDeclarations[0].typeName = Storage::ExportedType{"Obj"};

    ASSERT_THROW(storage.synchronize({}, {}, types, {}, {}), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesAddPropertyDeclarationExplicitType)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object",
                                                                              Storage::Module{
                                                                                  "QtQuick"}};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{"QObject"},
                                  TypeAccessSemantics::Reference,
                                  sourceId1,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                        "QQuickItem",
                                        Storage::NativeType{"QObject"},
                                        TypeAccessSemantics::Reference,
                                        sourceId1),
                          Field(&Storage::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("data",
                                                          Storage::NativeType{"QQuickObject"},
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "children",
                                        Storage::NativeType{"QQuickItem"},
                                        Storage::PropertyDeclarationTraits::IsList
                                            | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesPropertyDeclarationType)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].propertyDeclarations[0].typeName = Storage::NativeType{"QQuickItem"};

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                        "QQuickItem",
                                        Storage::NativeType{"QObject"},
                                        TypeAccessSemantics::Reference,
                                        sourceId1),
                          Field(&Storage::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("data",
                                                          Storage::NativeType{"QQuickItem"},
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "children",
                                        Storage::NativeType{"QQuickItem"},
                                        Storage::PropertyDeclarationTraits::IsList
                                            | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesDeclarationTraits)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsPointer;

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                        "QQuickItem",
                                        Storage::NativeType{"QObject"},
                                        TypeAccessSemantics::Reference,
                                        sourceId1),
                          Field(&Storage::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("data",
                                                          Storage::NativeType{"QObject"},
                                                          Storage::PropertyDeclarationTraits::IsPointer),
                                    IsPropertyDeclaration(
                                        "children",
                                        Storage::NativeType{"QQuickItem"},
                                        Storage::PropertyDeclarationTraits::IsList
                                            | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesDeclarationTraitsAndType)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsPointer;
    types[0].propertyDeclarations[0].typeName = Storage::NativeType{"QQuickItem"};

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                        "QQuickItem",
                                        Storage::NativeType{"QObject"},
                                        TypeAccessSemantics::Reference,
                                        sourceId1),
                          Field(&Storage::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("data",
                                                          Storage::NativeType{"QQuickItem"},
                                                          Storage::PropertyDeclarationTraits::IsPointer),
                                    IsPropertyDeclaration(
                                        "children",
                                        Storage::NativeType{"QQuickItem"},
                                        Storage::PropertyDeclarationTraits::IsList
                                            | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, SynchronizeTypesRemovesAPropertyDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].propertyDeclarations.pop_back();

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::propertyDeclarations,
                                     UnorderedElementsAre(IsPropertyDeclaration(
                                         "data",
                                         Storage::NativeType{"QObject"},
                                         Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddsAPropertyDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"object",
                                     Storage::NativeType{"QObject"},
                                     Storage::PropertyDeclarationTraits::IsPointer});

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(Storage::Module{"QtQuick"},
                          "QQuickItem",
                          Storage::NativeType{"QObject"},
                          TypeAccessSemantics::Reference,
                          sourceId1),
            Field(&Storage::Type::propertyDeclarations,
                  UnorderedElementsAre(
                      IsPropertyDeclaration("object",
                                            Storage::NativeType{"QObject"},
                                            Storage::PropertyDeclarationTraits::IsPointer),
                      IsPropertyDeclaration("data",
                                            Storage::NativeType{"QObject"},
                                            Storage::PropertyDeclarationTraits::IsList),
                      IsPropertyDeclaration("children",
                                            Storage::NativeType{"QQuickItem"},
                                            Storage::PropertyDeclarationTraits::IsList
                                                | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, SynchronizeTypesRenameAPropertyDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].propertyDeclarations[1].name = "objects";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                        "QQuickItem",
                                        Storage::NativeType{"QObject"},
                                        TypeAccessSemantics::Reference,
                                        sourceId1),
                          Field(&Storage::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("data",
                                                          Storage::NativeType{"QObject"},
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "objects",
                                        Storage::NativeType{"QQuickItem"},
                                        Storage::PropertyDeclarationTraits::IsList
                                            | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, UsingNonExistingNativePropertyTypeThrows)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::NativeType{"QObject2"};
    types.pop_back();

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, UsingNonExistingExportedPropertyTypeThrows)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExportedType{"QObject2"};
    types.pop_back();

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, UsingNonExistingExplicitExportedPropertyTypeWithWrongNameThrows)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"QObject2",
                                                                              Storage::Module{
                                                                                  "Qml"}};
    types.pop_back();

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, UsingNonExistingExplicitExportedPropertyTypeWithWrongModuleThrows)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"QObject",
                                                                              Storage::Module{
                                                                                  "QtQuick"}};
    types.pop_back();

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, BreakingPropertyDeclarationTypeDependencyByDeletingTypeThrows)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[0].prototype = Storage::NativeType{};
    types.pop_back();

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesAddFunctionDeclarations)
{
    Storage::Types types{createTypes()};

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                          Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesFunctionDeclarationReturnType)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].functionDeclarations[1].returnTypeName = "item";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                          Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesFunctionDeclarationName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].functionDeclarations[1].name = "name";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                          Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesFunctionDeclarationPopParameters)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].functionDeclarations[1].parameters.pop_back();

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                          Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesFunctionDeclarationAppendParameters)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].functionDeclarations[1].parameters.push_back(Storage::ParameterDeclaration{"arg4", "int"});

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                          Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesFunctionDeclarationChangeParameterName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].functionDeclarations[1].parameters[0].name = "other";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                          Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesFunctionDeclarationChangeParameterTypeName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].functionDeclarations[1].parameters[0].name = "long long";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                          Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesFunctionDeclarationChangeParameterTraits)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].functionDeclarations[1].parameters[0].traits = QmlDesigner::Storage::PropertyDeclarationTraits::IsList;

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                          Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesRemovesFunctionDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].functionDeclarations.pop_back();

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddFunctionDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].functionDeclarations.push_back(
        Storage::FunctionDeclaration{"name", "string", {Storage::ParameterDeclaration{"arg", "int"}}});

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                          Eq(types[0].functionDeclarations[1]),
                                                          Eq(types[0].functionDeclarations[2]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddSignalDeclarations)
{
    Storage::Types types{createTypes()};

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::signalDeclarations,
                                     UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                          Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesSignalDeclarationName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].signalDeclarations[1].name = "name";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::signalDeclarations,
                                     UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                          Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesSignalDeclarationPopParameters)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].signalDeclarations[1].parameters.pop_back();

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::signalDeclarations,
                                     UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                          Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesSignalDeclarationAppendParameters)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].signalDeclarations[1].parameters.push_back(Storage::ParameterDeclaration{"arg4", "int"});

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::signalDeclarations,
                                     UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                          Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesSignalDeclarationChangeParameterName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].signalDeclarations[1].parameters[0].name = "other";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::signalDeclarations,
                                     UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                          Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesSignalDeclarationChangeParameterTypeName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].signalDeclarations[1].parameters[0].typeName = "long long";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::signalDeclarations,
                                     UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                          Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesSignalDeclarationChangeParameterTraits)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].signalDeclarations[1].parameters[0].traits = QmlDesigner::Storage::PropertyDeclarationTraits::IsList;

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::signalDeclarations,
                                     UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                          Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesRemovesSignalDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].signalDeclarations.pop_back();

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::signalDeclarations,
                                     UnorderedElementsAre(Eq(types[0].signalDeclarations[0]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddSignalDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].signalDeclarations.push_back(
        Storage::SignalDeclaration{"name", {Storage::ParameterDeclaration{"arg", "int"}}});

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::signalDeclarations,
                                     UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                          Eq(types[0].signalDeclarations[1]),
                                                          Eq(types[0].signalDeclarations[2]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddEnumerationDeclarations)
{
    Storage::Types types{createTypes()};

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                          Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesEnumerationDeclarationName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].enumerationDeclarations[1].name = "Name";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                          Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesEnumerationDeclarationPopEnumeratorDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations.pop_back();

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                          Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesEnumerationDeclarationAppendEnumeratorDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations.push_back(
        Storage::EnumeratorDeclaration{"Haa", 54});

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                          Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesEnumerationDeclarationChangeEnumeratorDeclarationName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations[0].name = "Hoo";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                          Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangesEnumerationDeclarationChangeEnumeratorDeclarationValue)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations[1].value = 11;

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                          Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage,
       SynchronizeTypesChangesEnumerationDeclarationAddThatEnumeratorDeclarationHasValue)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations[0].value = 11;
    types[0].enumerationDeclarations[1].enumeratorDeclarations[0].hasValue = true;

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                          Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage,
       SynchronizeTypesChangesEnumerationDeclarationRemoveThatEnumeratorDeclarationHasValue)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations[0].hasValue = false;

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                          Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesRemovesEnumerationDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].enumerationDeclarations.pop_back();

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddEnumerationDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].enumerationDeclarations.push_back(
        Storage::EnumerationDeclaration{"name", {Storage::EnumeratorDeclaration{"Foo", 98, true}}});

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                          Eq(types[0].enumerationDeclarations[1]),
                                                          Eq(types[0].enumerationDeclarations[2]))))));
}

TEST_F(ProjectStorage, SynchronizeModulesAddModules)
{
    Storage::ModuleDependencies moduleDependencies{createModuleDependencies()};

    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                        {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(
                    IsModuleDependency("Qml", Storage::VersionNumber{2}, moduleSourceId1),
                    IsModuleDependency("QtQuick", Storage::VersionNumber{}, moduleSourceId2),
                    IsModuleDependency("/path/to", Storage::VersionNumber{}, moduleSourceId5)));
}

TEST_F(ProjectStorage, SynchronizeModulesAddModulesAgain)
{
    Storage::ModuleDependencies moduleDependencies{createModuleDependencies()};
    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                        {});

    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                        {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(
                    IsModuleDependency("Qml", Storage::VersionNumber{2}, moduleSourceId1),
                    IsModuleDependency("QtQuick", Storage::VersionNumber{}, moduleSourceId2),
                    IsModuleDependency("/path/to", Storage::VersionNumber{}, moduleSourceId5)));
}

TEST_F(ProjectStorage, SynchronizeModulesUpdateToMoreModules)
{
    Storage::ModuleDependencies moduleDependencies{createModuleDependencies()};
    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                        {});
    moduleDependencies.push_back(
        Storage::ModuleDependency{"QtQuick.Foo", Storage::VersionNumber{1}, moduleSourceId3});

    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId3, moduleSourceId5},
                        {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(
                    IsModuleDependency("Qml", Storage::VersionNumber{2}, moduleSourceId1),
                    IsModuleDependency("QtQuick", Storage::VersionNumber{}, moduleSourceId2),
                    IsModuleDependency("/path/to", Storage::VersionNumber{}, moduleSourceId5),
                    IsModuleDependency("QtQuick.Foo", Storage::VersionNumber{1}, moduleSourceId3)));
}

TEST_F(ProjectStorage, SynchronizeModulesAddOneMoreModules)
{
    Storage::ModuleDependencies moduleDependencies{createModuleDependencies()};
    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                        {});
    auto newModuleDependency = Storage::ModuleDependency{"QtQuick.Foo",
                                                         Storage::VersionNumber{1},
                                                         moduleSourceId3};

    storage.synchronize({newModuleDependency}, {}, {}, {moduleSourceId3}, {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(
                    IsModuleDependency("Qml", Storage::VersionNumber{2}, moduleSourceId1),
                    IsModuleDependency("QtQuick", Storage::VersionNumber{}, moduleSourceId2),
                    IsModuleDependency("/path/to", Storage::VersionNumber{}, moduleSourceId5),
                    IsModuleDependency("QtQuick.Foo", Storage::VersionNumber{1}, moduleSourceId3)));
}

TEST_F(ProjectStorage, SynchronizeModulesAddSameModuleNameButDifferentVersion)
{
    auto moduleSourceIdQml4 = sourcePathCache.sourceId("/path/Qml.4");
    auto moduleSourceIdQml3 = sourcePathCache.sourceId("/path/Qml.3");
    Storage::ModuleDependencies moduleDependencies{createModuleDependencies()};
    moduleDependencies.push_back(
        Storage::ModuleDependency{"Qml", Storage::VersionNumber{4}, moduleSourceIdQml4});
    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                        {});
    auto newModuleDependency = Storage::ModuleDependency{"Qml",
                                                         Storage::VersionNumber{3},
                                                         moduleSourceIdQml3};

    storage.synchronize({newModuleDependency}, {}, {}, {moduleSourceIdQml4, moduleSourceIdQml3}, {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(
                    IsModuleDependency("Qml", Storage::VersionNumber{2}, moduleSourceId1),
                    IsModuleDependency("Qml", Storage::VersionNumber{3}, moduleSourceIdQml3),
                    IsModuleDependency("QtQuick", Storage::VersionNumber{}, moduleSourceId2),
                    IsModuleDependency("/path/to", Storage::VersionNumber{}, moduleSourceId5)));
}

TEST_F(ProjectStorage, SynchronizeModulesRemoveModule)
{
    Storage::ModuleDependencies moduleDependencies{createModuleDependencies()};
    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                        {});

    storage.synchronize({}, {}, {}, {moduleSourceId5}, {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(
                    IsModuleDependency("Qml", Storage::VersionNumber{2}, moduleSourceId1),
                    IsModuleDependency("QtQuick", Storage::VersionNumber{}, moduleSourceId2)));
}

TEST_F(ProjectStorage, SynchronizeModulesChangeSourceId)
{
    Storage::ModuleDependencies moduleDependencies{createModuleDependencies()};
    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                        {});
    moduleDependencies[1].sourceId = moduleSourceId3;

    storage.synchronize({moduleDependencies[1]}, {}, {}, {moduleSourceId2, moduleSourceId3}, {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(
                    IsModuleDependency("Qml", Storage::VersionNumber{2}, moduleSourceId1),
                    IsModuleDependency("QtQuick", Storage::VersionNumber{}, moduleSourceId3),
                    IsModuleDependency("/path/to", Storage::VersionNumber{}, moduleSourceId5)));
}

TEST_F(ProjectStorage, SynchronizeModulesChangeName)
{
    Storage::ModuleDependencies moduleDependencies{createModuleDependencies()};
    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                        {});
    moduleDependencies[0].name = "Qml2";
    moduleDependencies[1].dependencies[0].name = "Qml2";
    moduleDependencies[2].dependencies[1].name = "Qml2";

    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                        {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(
                    IsModuleDependency("Qml2", Storage::VersionNumber{2}, moduleSourceId1),
                    IsModuleDependency("QtQuick", Storage::VersionNumber{}, moduleSourceId2),
                    IsModuleDependency("/path/to", Storage::VersionNumber{}, moduleSourceId5)));
}

TEST_F(ProjectStorage, SynchronizeModulesChangeVersion)
{
    Storage::ModuleDependencies moduleDependencies{createModuleDependencies()};
    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                        {});
    moduleDependencies[0].version = Storage::VersionNumber{3};
    moduleDependencies[1].dependencies[0].version = Storage::VersionNumber{3};
    moduleDependencies[2].dependencies[1].version = Storage::VersionNumber{3};

    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                        {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(
                    IsModuleDependency("Qml", Storage::VersionNumber{3}, moduleSourceId1),
                    IsModuleDependency("QtQuick", Storage::VersionNumber{}, moduleSourceId2),
                    IsModuleDependency("/path/to", Storage::VersionNumber{}, moduleSourceId5)));
}

TEST_F(ProjectStorage, SynchronizeModulesAddModuleDependecies)
{
    Storage::ModuleDependencies moduleDependencies{createModuleDependencies()};

    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                        {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(
                    AllOf(IsModuleDependency("Qml", Storage::VersionNumber{2}, moduleSourceId1),
                          Field(&Storage::ModuleDependency::dependencies, IsEmpty())),
                    AllOf(IsModuleDependency("QtQuick", Storage::VersionNumber{}, moduleSourceId2),
                          Field(&Storage::ModuleDependency::dependencies,
                                ElementsAre(IsModule("Qml", Storage::VersionNumber{2})))),
                    AllOf(IsModuleDependency("/path/to", Storage::VersionNumber{}, moduleSourceId5),
                          Field(&Storage::ModuleDependency::dependencies,
                                UnorderedElementsAre(IsModule("Qml", Storage::VersionNumber{2}),
                                                     IsModule("QtQuick", Storage::VersionNumber{}))))));
}

TEST_F(ProjectStorage, SynchronizeModulesAddModuleDependeciesWhichDoesNotExitsThrows)
{
    Storage::ModuleDependencies moduleDependencies{createModuleDependencies()};
    moduleDependencies[1].dependencies.push_back(Storage::Module{"QmlBase", Storage::VersionNumber{2}});

    ASSERT_THROW(storage.synchronize(moduleDependencies,
                                     {},
                                     {},
                                     {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                                     {}),
                 QmlDesigner::ModuleDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeModulesRemovesDependeciesForRemovedModules)
{
    Storage::ModuleDependencies moduleDependencies{createModuleDependencies()};
    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                        {});
    auto last = moduleDependencies.back();
    moduleDependencies.pop_back();
    storage.synchronize({}, {}, {}, {moduleSourceId5}, {});
    last.dependencies.pop_back();
    moduleDependencies.push_back(last);

    storage.synchronize({moduleDependencies[2]}, {}, {}, {moduleSourceId5}, {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(
                    AllOf(IsModuleDependency("Qml", Storage::VersionNumber{2}, moduleSourceId1),
                          Field(&Storage::ModuleDependency::dependencies, IsEmpty())),
                    AllOf(IsModuleDependency("QtQuick", Storage::VersionNumber{}, moduleSourceId2),
                          Field(&Storage::ModuleDependency::dependencies,
                                ElementsAre(IsModule("Qml", Storage::VersionNumber{2})))),
                    AllOf(IsModuleDependency("/path/to", Storage::VersionNumber{}, moduleSourceId5),
                          Field(&Storage::ModuleDependency::dependencies,
                                UnorderedElementsAre(IsModule("QtQuick", Storage::VersionNumber{}))))));
}

TEST_F(ProjectStorage, SynchronizeModulesAddMoreModuleDependecies)
{
    Storage::ModuleDependencies moduleDependencies{createModuleDependencies()};
    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                        {});
    auto moduleSourceIdQmlBase = sourcePathCache.sourceId("/path/QmlBase");
    moduleDependencies.push_back(
        Storage::ModuleDependency{"QmlBase", Storage::VersionNumber{2}, moduleSourceIdQmlBase});
    moduleDependencies[1].dependencies.push_back(Storage::Module{"QmlBase", Storage::VersionNumber{2}});

    storage.synchronize({moduleDependencies[1], moduleDependencies[3]},
                        {},
                        {},
                        {moduleSourceId2, moduleSourceIdQmlBase},
                        {});

    ASSERT_THAT(
        storage.fetchAllModules(),
        UnorderedElementsAre(
            AllOf(IsModuleDependency("Qml", Storage::VersionNumber{2}, moduleSourceId1),
                  Field(&Storage::ModuleDependency::dependencies, IsEmpty())),
            AllOf(IsModuleDependency("QmlBase", Storage::VersionNumber{2}, moduleSourceIdQmlBase),
                  Field(&Storage::ModuleDependency::dependencies, IsEmpty())),
            AllOf(IsModuleDependency("QtQuick", Storage::VersionNumber{}, moduleSourceId2),
                  Field(&Storage::ModuleDependency::dependencies,
                        UnorderedElementsAre(IsModule("Qml", Storage::VersionNumber{2}),
                                             IsModule("QmlBase", Storage::VersionNumber{2})))),
            AllOf(IsModuleDependency("/path/to", Storage::VersionNumber{}, moduleSourceId5),
                  Field(&Storage::ModuleDependency::dependencies,
                        UnorderedElementsAre(IsModule("Qml", Storage::VersionNumber{2}),
                                             IsModule("QtQuick", Storage::VersionNumber{}))))));
}

TEST_F(ProjectStorage, SynchronizeModulesAddMoreModuleDependeciesWithDifferentVersionNumber)
{
    Storage::ModuleDependencies moduleDependencies{createModuleDependencies()};
    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                        {});
    auto moduleSourceIdQml3 = sourcePathCache.sourceId("/path/Qml.3");
    moduleDependencies.push_back(
        Storage::ModuleDependency{"Qml", Storage::VersionNumber{3}, moduleSourceIdQml3, {}});
    moduleDependencies[1].dependencies.push_back(Storage::Module{"Qml", Storage::VersionNumber{3}});

    storage.synchronize({moduleDependencies[1], moduleDependencies[3]},
                        {},
                        {},
                        {moduleSourceId2, moduleSourceIdQml3},
                        {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(
                    AllOf(IsModuleDependency("Qml", Storage::VersionNumber{2}, moduleSourceId1),
                          Field(&Storage::ModuleDependency::dependencies, IsEmpty())),
                    AllOf(IsModuleDependency("Qml", Storage::VersionNumber{3}, moduleSourceIdQml3),
                          Field(&Storage::ModuleDependency::dependencies, IsEmpty())),
                    AllOf(IsModuleDependency("QtQuick", Storage::VersionNumber{}, moduleSourceId2),
                          Field(&Storage::ModuleDependency::dependencies,
                                UnorderedElementsAre(IsModule("Qml", Storage::VersionNumber{2}),
                                                     IsModule("Qml", Storage::VersionNumber{3})))),
                    AllOf(IsModuleDependency("/path/to", Storage::VersionNumber{}, moduleSourceId5),
                          Field(&Storage::ModuleDependency::dependencies,
                                UnorderedElementsAre(IsModule("Qml", Storage::VersionNumber{2}),
                                                     IsModule("QtQuick", Storage::VersionNumber{}))))));
}

TEST_F(ProjectStorage, SynchronizeModulesDependencyGetsHighestVersionIfNoVersionIsSupplied)
{
    Storage::ModuleDependencies moduleDependencies{createModuleDependencies()};
    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                        {});
    auto moduleSourceIdQml3 = sourcePathCache.sourceId("/path/Qml.3");
    moduleDependencies.push_back(
        Storage::ModuleDependency{"Qml", Storage::VersionNumber{3}, moduleSourceIdQml3, {}});
    moduleDependencies[1].dependencies.push_back(Storage::Module{"Qml"});

    storage.synchronize({moduleDependencies[1], moduleDependencies[3]},
                        {},
                        {},
                        {moduleSourceId2, moduleSourceIdQml3},
                        {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(
                    AllOf(IsModuleDependency("Qml", Storage::VersionNumber{2}, moduleSourceId1),
                          Field(&Storage::ModuleDependency::dependencies, IsEmpty())),
                    AllOf(IsModuleDependency("Qml", Storage::VersionNumber{3}, moduleSourceIdQml3),
                          Field(&Storage::ModuleDependency::dependencies, IsEmpty())),
                    AllOf(IsModuleDependency("QtQuick", Storage::VersionNumber{}, moduleSourceId2),
                          Field(&Storage::ModuleDependency::dependencies,
                                UnorderedElementsAre(IsModule("Qml", Storage::VersionNumber{2}),
                                                     IsModule("Qml", Storage::VersionNumber{3})))),
                    AllOf(IsModuleDependency("/path/to", Storage::VersionNumber{}, moduleSourceId5),
                          Field(&Storage::ModuleDependency::dependencies,
                                UnorderedElementsAre(IsModule("Qml", Storage::VersionNumber{2}),
                                                     IsModule("QtQuick", Storage::VersionNumber{}))))));
}

TEST_F(ProjectStorage, SynchronizeModulesDependencyGetsOnlyTheHighestDependency)
{
    Storage::ModuleDependencies moduleDependencies{createModuleDependencies()};
    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                        {});
    auto moduleSourceIdQml1 = sourcePathCache.sourceId("/path/Qml.1");
    moduleDependencies.push_back(
        Storage::ModuleDependency{"Qml", Storage::VersionNumber{1}, moduleSourceIdQml1, {}});
    moduleDependencies[1].dependencies.push_back(Storage::Module{"Qml"});

    storage.synchronize({moduleDependencies[1], moduleDependencies[3]},
                        {},
                        {},
                        {moduleSourceId2, moduleSourceIdQml1},
                        {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(
                    AllOf(IsModuleDependency("Qml", Storage::VersionNumber{2}, moduleSourceId1),
                          Field(&Storage::ModuleDependency::dependencies, IsEmpty())),
                    AllOf(IsModuleDependency("Qml", Storage::VersionNumber{1}, moduleSourceIdQml1),
                          Field(&Storage::ModuleDependency::dependencies, IsEmpty())),
                    AllOf(IsModuleDependency("QtQuick", Storage::VersionNumber{}, moduleSourceId2),
                          Field(&Storage::ModuleDependency::dependencies,
                                UnorderedElementsAre(IsModule("Qml", Storage::VersionNumber{2})))),
                    AllOf(IsModuleDependency("/path/to", Storage::VersionNumber{}, moduleSourceId5),
                          Field(&Storage::ModuleDependency::dependencies,
                                UnorderedElementsAre(IsModule("Qml", Storage::VersionNumber{2}),
                                                     IsModule("QtQuick", Storage::VersionNumber{}))))));
}

TEST_F(ProjectStorage, SynchronizeModulesDependencyRemoveDuplicateDependencies)
{
    Storage::ModuleDependencies moduleDependencies{createModuleDependencies()};
    storage.synchronize(moduleDependencies,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId5},
                        {});
    auto moduleSourceIdQml3 = sourcePathCache.sourceId("/path/Qml.3");
    moduleDependencies.push_back(
        Storage::ModuleDependency{"Qml", Storage::VersionNumber{3}, moduleSourceIdQml3});
    moduleDependencies[2].dependencies.push_back(Storage::Module{"Qml", Storage::VersionNumber{3}});
    moduleDependencies[2].dependencies.push_back(Storage::Module{"Qml", Storage::VersionNumber{2}});
    moduleDependencies[2].dependencies.push_back(Storage::Module{"Qml", Storage::VersionNumber{3}});
    moduleDependencies[2].dependencies.push_back(Storage::Module{"Qml", Storage::VersionNumber{2}});

    storage.synchronize({moduleDependencies[2], moduleDependencies[3]},
                        {},
                        {},
                        {moduleSourceId5, moduleSourceIdQml3},
                        {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(
                    AllOf(IsModuleDependency("Qml", Storage::VersionNumber{2}, moduleSourceId1),
                          Field(&Storage::ModuleDependency::dependencies, IsEmpty())),
                    AllOf(IsModuleDependency("Qml", Storage::VersionNumber{3}, moduleSourceIdQml3),
                          Field(&Storage::ModuleDependency::dependencies, IsEmpty())),
                    AllOf(IsModuleDependency("QtQuick", Storage::VersionNumber{}, moduleSourceId2),
                          Field(&Storage::ModuleDependency::dependencies,
                                UnorderedElementsAre(IsModule("Qml", Storage::VersionNumber{2})))),
                    AllOf(IsModuleDependency("/path/to", Storage::VersionNumber{}, moduleSourceId5),
                          Field(&Storage::ModuleDependency::dependencies,
                                UnorderedElementsAre(IsModule("Qml", Storage::VersionNumber{2}),
                                                     IsModule("Qml", Storage::VersionNumber{3}),
                                                     IsModule("QtQuick", Storage::VersionNumber{}))))));
}

TEST_F(ProjectStorage, RemovingModuleRemovesDependentTypesToo)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    Storage::ModuleDependencies moduleDependencies{createModuleDependencies()};
    moduleDependencies.pop_back();
    moduleDependencies.pop_back();
    storage.synchronize({}, {}, {}, {moduleSourceId2, moduleSourceId5}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj"))))));
}

TEST_F(ProjectStorage, FetchTypeIdByModuleIdAndName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    auto typeId = storage.fetchTypeIdByName(moduleId1, "QObject");

    ASSERT_THAT(storage.fetchTypeIdByExportedName("Object"), Eq(typeId));
}

TEST_F(ProjectStorage, FetchTypeIdByExportedName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    auto typeId = storage.fetchTypeIdByExportedName("Object");

    ASSERT_THAT(storage.fetchTypeIdByName(moduleId1, "QObject"), Eq(typeId));
}

TEST_F(ProjectStorage, FetchTypeIdByImporIdsAndExportedName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    auto typeId = storage.fetchTypeIdByModuleIdsAndExportedName({moduleId1, moduleId2}, "Object");

    ASSERT_THAT(storage.fetchTypeIdByName(moduleId1, "QObject"), Eq(typeId));
}

TEST_F(ProjectStorage, FetchInvalidTypeIdByImporIdsAndExportedNameIfModuleIdsAreEmpty)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    auto typeId = storage.fetchTypeIdByModuleIdsAndExportedName({}, "Object");

    ASSERT_FALSE(typeId.isValid());
}

TEST_F(ProjectStorage, FetchInvalidTypeIdByImporIdsAndExportedNameIfModuleIdsAreInvalid)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    auto typeId = storage.fetchTypeIdByModuleIdsAndExportedName({ModuleId{}}, "Object");

    ASSERT_FALSE(typeId.isValid());
}

TEST_F(ProjectStorage, FetchInvalidTypeIdByImporIdsAndExportedNameIfNotInModule)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    auto typeId = storage.fetchTypeIdByModuleIdsAndExportedName({moduleId2, moduleId3}, "Object");

    ASSERT_FALSE(typeId.isValid());
}

TEST_F(ProjectStorage, FetchModuleDepencencyIds)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    auto moduleIds = storage.fetchModuleDependencyIds({moduleId3});

    ASSERT_THAT(moduleIds, UnorderedElementsAre(moduleId1, moduleId2, moduleId3));
}

TEST_F(ProjectStorage, FetchModuleDepencencyIdsForRootDepencency)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    auto moduleIds = storage.fetchModuleDependencyIds({moduleId1});

    ASSERT_THAT(moduleIds, ElementsAre(moduleId1));
}

TEST_F(ProjectStorage, SynchronizeTypesAddAliasDeclarations)
{
    Storage::Types types{createTypesWithAliases()};

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Module{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddAliasDeclarationsAgain)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Module{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesRemoveAliasDeclarations)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[2].propertyDeclarations.pop_back();

    storage.synchronize({}, {}, {types[2]}, {sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Module{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesAddAliasDeclarationsThrowsForWrongTypeName)
{
    Storage::Types types{createTypesWithAliases()};
    types[2].propertyDeclarations[1].typeName = Storage::NativeType{"QQuickItemWrong"};

    ASSERT_THROW(storage.synchronize({}, {}, {types[2]}, {sourceId4}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}
TEST_F(ProjectStorage, SynchronizeTypesAddAliasDeclarationsThrowsForWrongPropertyName)
{
    Storage::Types types{createTypesWithAliases()};
    types[2].propertyDeclarations[1].aliasPropertyName = "childrenWrong";

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId4}, {}),
                 QmlDesigner::PropertyNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesChangeAliasDeclarationsTypeName)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[2].propertyDeclarations[2].typeName = Storage::ExportedType{"Obj2"};

    storage.synchronize({}, {}, {types[2]}, {sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Module{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangeAliasDeclarationsPropertyName)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[2].propertyDeclarations[2].aliasPropertyName = "children";

    storage.synchronize({}, {}, {types[2]}, {sourceId3}, {});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                "QAliasItem",
                                Storage::NativeType{"QQuickItem"},
                                TypeAccessSemantics::Reference,
                                sourceId3),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("items",
                                                  Storage::NativeType{"QQuickItem"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("objects",
                                                  Storage::NativeType{"QQuickItem"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("data",
                                                  Storage::NativeType{"QObject"},
                                                  Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangeAliasDeclarationsToPropertyDeclaration)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[2].propertyDeclarations.pop_back();
    types[2].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::NativeType{"QQuickItem"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize({}, {}, {types[2]}, {sourceId3}, {});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                "QAliasItem",
                                Storage::NativeType{"QQuickItem"},
                                TypeAccessSemantics::Reference,
                                sourceId3),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("items",
                                                  Storage::NativeType{"QQuickItem"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("objects",
                                                  Storage::NativeType{"QQuickItem"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("data",
                                                  Storage::NativeType{"QObject"},
                                                  Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangePropertyDeclarationsToAliasDeclaration)
{
    Storage::Types types{createTypesWithAliases()};
    auto typesChanged = types;
    typesChanged[2].propertyDeclarations.pop_back();
    typesChanged[2].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::NativeType{"QQuickItem"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});
    storage.synchronize({}, {}, typesChanged, {sourceId1, sourceId2, sourceId3, sourceId4}, {});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Module{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangeAliasTargetPropertyDeclarationTraits)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[1].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsList
                                              | Storage::PropertyDeclarationTraits::IsReadOnly;

    storage.synchronize({}, {}, {types[1]}, {sourceId2}, {});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                "QAliasItem",
                                Storage::NativeType{"QQuickItem"},
                                TypeAccessSemantics::Reference,
                                sourceId3),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("items",
                                                  Storage::NativeType{"QQuickItem"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("objects",
                                                  Storage::NativeType{"QObject"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("data",
                                                  Storage::NativeType{"QObject"},
                                                  Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesChangeAliasTargetPropertyDeclarationTypeName)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[1].propertyDeclarations[0].typeName = Storage::ExportedType{"Item"};

    storage.synchronize({}, {}, {types[1]}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Module{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesRemovePropertyDeclarationWithAnAliasThrows)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[1].propertyDeclarations.pop_back();

    ASSERT_THROW(storage.synchronize({}, {}, {types[1]}, {sourceId2}, {}),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorage, SynchronizeTypesRemovePropertyDeclarationAndAlias)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[1].propertyDeclarations.pop_back();
    types[2].propertyDeclarations.pop_back();

    storage.synchronize({}, {}, {types[1], types[2]}, {sourceId2, sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Module{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, SynchronizeTypesRemoveTypeWithAliasTargetPropertyDeclarationThrows)
{
    Storage::Types types{createTypesWithAliases()};
    types[2].propertyDeclarations[2].typeName = Storage::ExportedType{"Object2"};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});

    ASSERT_THROW(storage.synchronize({}, {}, {}, {sourceId4}, {}), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesRemoveTypeAndAliasPropertyDeclaration)
{
    Storage::Types types{createTypesWithAliases()};
    types[2].propertyDeclarations[2].typeName = Storage::ExportedType{"Object2"};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[2].propertyDeclarations.pop_back();

    storage.synchronize({}, {}, {types[0], types[2]}, {sourceId1, sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Module{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, UpdateAliasPropertyIfPropertyIsOverloaded)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[0].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::NativeType{"QQuickItem"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize({}, {}, {types[0]}, {sourceId1}, {});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                "QAliasItem",
                                Storage::NativeType{"QQuickItem"},
                                TypeAccessSemantics::Reference,
                                sourceId3),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("items",
                                                  Storage::NativeType{"QQuickItem"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("objects",
                                                  Storage::NativeType{"QQuickItem"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("data",
                                                  Storage::NativeType{"QObject"},
                                                  Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, AliasPropertyIsOverloaded)
{
    Storage::Types types{createTypesWithAliases()};
    types[0].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::NativeType{"QQuickItem"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                "QAliasItem",
                                Storage::NativeType{"QQuickItem"},
                                TypeAccessSemantics::Reference,
                                sourceId3),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("items",
                                                  Storage::NativeType{"QQuickItem"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("objects",
                                                  Storage::NativeType{"QQuickItem"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("data",
                                                  Storage::NativeType{"QObject"},
                                                  Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, UpdateAliasPropertyIfOverloadedPropertyIsRemoved)
{
    Storage::Types types{createTypesWithAliases()};
    types[0].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::NativeType{"QQuickItem"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[0].propertyDeclarations.pop_back();

    storage.synchronize({}, {}, {types[0]}, {sourceId1}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Module{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, RelinkAliasProperty)
{
    Storage::Types types{createTypesWithAliases()};
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject2"};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[3].module = Storage::Module{"QtQuick"};

    storage.synchronize({}, {}, {types[3]}, {sourceId4}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Module{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    Storage::NativeType{"QObject2"},
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, DoNotRelinkAliasPropertyForExplicitExportedTypeName)
{
    Storage::Types types{createTypesWithAliases()};
    types[1].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object2",
                                                                              Storage::Module{
                                                                                  "/path/to"}};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[3].module = Storage::Module{"QtQuick"};

    ASSERT_THROW(storage.synchronize({}, {}, {types[3]}, {sourceId4}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage,
       DoRelinkAliasPropertyForExplicitExportedTypeNameEvenIfAnOtherSimilarTimeNameExists)
{
    Storage::Types types{createTypesWithAliases()};
    types[1].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object2",
                                                                              Storage::Module{
                                                                                  "/path/to"}};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QObject2",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId5,
                                  {Storage::ExportedType{"Object2"}, Storage::ExportedType{"Obj2"}}});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4, sourceId5}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Module{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    Storage::NativeType{"QObject2"},
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, RelinkAliasPropertyReactToTypeNameChange)
{
    Storage::Types types{createTypesWithAliases2()};
    types[2].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"items", Storage::ExportedType{"Item"}, "children"});
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[0].typeName = "QQuickItem2";

    storage.synchronize({}, {}, {types[0]}, {sourceId1}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Module{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QObject"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem2"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, DoNotRelinkAliasPropertyForDeletedType)
{
    Storage::Types types{createTypesWithAliases()};
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject2"};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types.erase(std::next(types.begin(), 2));
    types[2].module = Storage::Module{"QtQuick"};

    storage.synchronize({}, {}, {types[2]}, {sourceId3, sourceId4}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Not(Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                                 "QAliasItem",
                                                 Storage::NativeType{"QQuickItem"},
                                                 TypeAccessSemantics::Reference,
                                                 sourceId3)))));
}

TEST_F(ProjectStorage, DoNotRelinkAliasPropertyForDeletedTypeAndPropertyType)
{
    Storage::Types types{createTypesWithAliases()};
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject2"};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types.pop_back();
    types.pop_back();
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject"};

    storage.synchronize({}, {}, {types[1]}, {sourceId2, sourceId3, sourceId4}, {});

    ASSERT_THAT(storage.fetchTypes(), SizeIs(2));
}

TEST_F(ProjectStorage, DoNotRelinkAliasPropertyForDeletedTypeAndPropertyTypeNameChange)
{
    Storage::Types types{createTypesWithAliases()};
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject2"};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types.erase(std::next(types.begin(), 2));
    types[2].module = Storage::Module{"QtQuick"};
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject"};

    storage.synchronize({}, {}, {types[2]}, {sourceId3, sourceId4}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Not(Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                                 "QAliasItem",
                                                 Storage::NativeType{"QQuickItem"},
                                                 TypeAccessSemantics::Reference,
                                                 sourceId3)))));
}

TEST_F(ProjectStorage, DoNotRelinkPropertyTypeDoesNotExists)
{
    Storage::Types types{createTypesWithAliases()};
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject2"};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types.pop_back();

    ASSERT_THROW(storage.synchronize({}, {}, {}, {sourceId4}, {}), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, DoNotRelinkAliasPropertyTypeDoesNotExists)
{
    Storage::Types types{createTypesWithAliases2()};
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject2"};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types.erase(types.begin());

    ASSERT_THROW(storage.synchronize({}, {}, {}, {sourceId1}, {}), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, ChangePrototypeTypeName)
{
    Storage::Types types{createTypesWithExportedTypeNamesOnly()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[1].typeName = "QObject3";

    storage.synchronize({}, {}, {types[1]}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject3"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorage, ChangePrototypeTypeModuleId)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[1].module = Storage::Module{"QtQuick"};

    storage.synchronize({}, {}, {types[1]}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorage, ChangeExplicitPrototypeTypeModuleIdThows)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Module{"Qml"}};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[1].module = Storage::Module{"QtQuick"};

    ASSERT_THROW(storage.synchronize({}, {}, {types[1]}, {sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, ChangeExplicitPrototypeTypeModuleId)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Module{"Qml"}};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[1].module = Storage::Module{"QtQuick"};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Module{"QtQuick"}};

    storage.synchronize({}, {}, {types[0], types[1]}, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorage, ChangePrototypeTypeNameAndModuleId)
{
    Storage::Types types{createTypesWithExportedTypeNamesOnly()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[1].module = Storage::Module{"QtQuick"};
    types[1].typeName = "QObject3";

    storage.synchronize({}, {}, {types[1]}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject3"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorage, ChangePrototypeTypeNameThrowsForWrongNativePrototupeTypeName)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExportedType{"Object"};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[1].typeName = "QObject3";

    ASSERT_THROW(storage.synchronize({}, {}, {types[1]}, {sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, ThrowForPrototypeChainCycles)
{
    Storage::Types types{createTypes()};
    types[1].prototype = Storage::ExportedType{"Object2"};
    types.push_back(Storage::Type{Storage::Module{"/path/to"},
                                  "QObject2",
                                  Storage::ExportedType{"Item"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object2"}, Storage::ExportedType{"Obj2"}}});

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {}),
                 QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorage, ThrowForTypeIdAndPrototypeIdAreTheSame)
{
    Storage::Types types{createTypes()};
    types[1].prototype = Storage::ExportedType{"Object"};

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorage, ThrowForTypeIdAndPrototypeIdAreTheSameForRelinking)
{
    Storage::Types types{createTypesWithExportedTypeNamesOnly()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[1].prototype = Storage::ExportedType{"Item"};
    types[1].typeName = "QObject2";

    ASSERT_THROW(storage.synchronize({}, {}, {types[1]}, {sourceId2}, {}),
                 QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorage, RecursiveAliases)
{
    Storage::Types types{createTypesWithRecursiveAliases()};

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4, sourceId5}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QAliasItem2",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId5),
                               Field(&Storage::Type::propertyDeclarations,
                                     ElementsAre(IsPropertyDeclaration(
                                         "objects",
                                         Storage::NativeType{"QObject"},
                                         Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, RecursiveAliasesChangePropertyType)
{
    Storage::Types types{createTypesWithRecursiveAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4, sourceId5}, {});
    types[1].propertyDeclarations[0].typeName = Storage::ExportedType{"Object2"};

    storage.synchronize({}, {}, {types[1]}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QAliasItem2",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId5),
                               Field(&Storage::Type::propertyDeclarations,
                                     ElementsAre(IsPropertyDeclaration(
                                         "objects",
                                         Storage::NativeType{"QObject2"},
                                         Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, UpdateAliasesAfterInjectingProperty)
{
    Storage::Types types{createTypesWithRecursiveAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4, sourceId5}, {});
    types[0].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::ExportedType{"Item"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize({}, {}, {types[0]}, {sourceId1}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QAliasItem2",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId5),
                               Field(&Storage::Type::propertyDeclarations,
                                     ElementsAre(IsPropertyDeclaration(
                                         "objects",
                                         Storage::NativeType{"QQuickItem"},
                                         Storage::PropertyDeclarationTraits::IsList
                                             | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, UpdateAliasesAfterChangeAliasToProperty)
{
    Storage::Types types{createTypesWithRecursiveAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4, sourceId5}, {});
    types[2].propertyDeclarations.clear();
    types[2].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::ExportedType{"Item"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize({}, {}, {types[2]}, {sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                AllOf(Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                                   "QAliasItem2",
                                                   Storage::NativeType{"QObject"},
                                                   TypeAccessSemantics::Reference,
                                                   sourceId5),
                                     Field(&Storage::Type::propertyDeclarations,
                                           ElementsAre(IsPropertyDeclaration(
                                               "objects",
                                               Storage::NativeType{"QQuickItem"},
                                               Storage::PropertyDeclarationTraits::IsList
                                                   | Storage::PropertyDeclarationTraits::IsReadOnly,
                                               "objects"))))),
                      Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                                   "QAliasItem",
                                                   Storage::NativeType{"QQuickItem"},
                                                   TypeAccessSemantics::Reference,
                                                   sourceId3),
                                     Field(&Storage::Type::propertyDeclarations,
                                           ElementsAre(IsPropertyDeclaration(
                                               "objects",
                                               Storage::NativeType{"QQuickItem"},
                                               Storage::PropertyDeclarationTraits::IsList
                                                   | Storage::PropertyDeclarationTraits::IsReadOnly,
                                               "")))))));
}

TEST_F(ProjectStorage, UpdateAliasesAfterChangePropertyToAlias)
{
    Storage::Types types{createTypesWithRecursiveAliases()};
    types[3].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsList
                                              | Storage::PropertyDeclarationTraits::IsReadOnly;
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4, sourceId5}, {});
    types[1].propertyDeclarations.clear();
    types[1].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects", Storage::ExportedType{"Object2"}, "objects"});

    storage.synchronize({}, {}, {types[1]}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QAliasItem2",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId5),
                               Field(&Storage::Type::propertyDeclarations,
                                     ElementsAre(IsPropertyDeclaration(
                                         "objects",
                                         Storage::NativeType{"QObject"},
                                         Storage::PropertyDeclarationTraits::IsList
                                             | Storage::PropertyDeclarationTraits::IsReadOnly,
                                         "objects"))))));
}

TEST_F(ProjectStorage, CheckForProtoTypeCycleThrows)
{
    Storage::Types types{createTypesWithRecursiveAliases()};
    types[1].propertyDeclarations.clear();
    types[1].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects", Storage::ExportedType{"AliasItem2"}, "objects"});

    ASSERT_THROW(storage.synchronize(
                     {}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4, sourceId5}, {}),
                 QmlDesigner::AliasChainCycle);
}

TEST_F(ProjectStorage, CheckForProtoTypeCycleAfterUpdateThrows)
{
    Storage::Types types{createTypesWithRecursiveAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4, sourceId5}, {});
    types[1].propertyDeclarations.clear();
    types[1].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects", Storage::ExportedType{"AliasItem2"}, "objects"});

    ASSERT_THROW(storage.synchronize({}, {}, {types[1]}, {sourceId2}, {}),
                 QmlDesigner::AliasChainCycle);
}

TEST_F(ProjectStorage, ExplicitPrototype)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Module{"Qml"}};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorage, ExplicitPrototypeUpperDownTheModuleChainThrows)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Module{"QtQuick"}};

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, ExplicitPrototypeUpperInTheModuleChain)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Module{"QtQuick"}};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QQuickObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorage, ExplicitPrototypeWithWrongVersionThrows)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Module{"Qml", 4}};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, ExplicitPrototypeWithVersion)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Module{"Qml", 2}};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorage, ExplicitPrototypeWithVersionInTheProtoTypeChain)
{
    Storage::Types types{createTypes()};
    auto moduleDependencyQtQuick2 = Storage::ModuleDependency{
        "QtQuick",
        Storage::VersionNumber{2},
        moduleSourceId2,
        {Storage::Module{"Qml", Storage::VersionNumber{2}}}};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Module{"QtQuick", 2}};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize({moduleDependencyQtQuick2},
                        {},
                        types,
                        {sourceId1, sourceId2, sourceId3, moduleSourceId2},
                        {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick", 2},
                                       "QQuickItem",
                                       Storage::NativeType{"QQuickObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorage, ExplicitPrototypeWithVersionDownTheProtoTypeChainThrows)
{
    Storage::Types types{createTypes()};
    auto moduleDependencyQtQuick2 = Storage::ModuleDependency{
        "QtQuick",
        Storage::VersionNumber{2},
        moduleSourceId2,
        {Storage::Module{"Qml", Storage::VersionNumber{2}}}};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Module{"QtQuick", 2}};

    ASSERT_THROW(storage.synchronize({moduleDependencyQtQuick2},
                                     {},
                                     types,
                                     {sourceId1, sourceId2, sourceId3, moduleSourceId2},
                                     {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, ExplicitPropertyDeclarationTypeName)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object",
                                                                              Storage::Module{
                                                                                  "Qml"}};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    Field(&Storage::Type::propertyDeclarations,
                          Contains(IsPropertyDeclaration("data",
                                                         Storage::NativeType{"QObject"},
                                                         Storage::PropertyDeclarationTraits::IsList)))));
}

TEST_F(ProjectStorage, ExplicitPropertyDeclarationTypeNameDownTheModuleChainThrows)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object",
                                                                              Storage::Module{
                                                                                  "QtQuick"}};

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, ExplicitPropertyDeclarationTypeNameInTheModuleChain)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object",
                                                                              Storage::Module{
                                                                                  "QtQuick"}};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    Field(&Storage::Type::propertyDeclarations,
                          Contains(IsPropertyDeclaration("data",
                                                         Storage::NativeType{"QQuickObject"},
                                                         Storage::PropertyDeclarationTraits::IsList)))));
}

TEST_F(ProjectStorage, ExplicitPropertyDeclarationTypeNameWithVersion)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object",
                                                                              Storage::Module{"Qml",
                                                                                              2}};

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    Field(&Storage::Type::propertyDeclarations,
                          Contains(IsPropertyDeclaration("data",
                                                         Storage::NativeType{"QObject"},
                                                         Storage::PropertyDeclarationTraits::IsList)))));
}

TEST_F(ProjectStorage, PrototypeWithVersionDownTheProtoTypeChainThrows)
{
    Storage::Types types{createTypes()};
    auto moduleDependencyQtQuick2 = Storage::ModuleDependency{
        "QtQuick",
        Storage::VersionNumber{2},
        moduleSourceId2,
        {Storage::Module{"Qml", Storage::VersionNumber{2}}}};
    types[0].prototype = Storage::ExportedType{"Object"};
    auto document = Storage::Document{sourceId1, {Storage::Module{"QtQuick"}}};

    ASSERT_THROW(storage.synchronize({moduleDependencyQtQuick2},
                                     {document},
                                     types,
                                     {sourceId1, sourceId2, sourceId3, moduleSourceId2},
                                     {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, ChangePropertyTypeModuleIdWithExplicitTypeThrows)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object",
                                                                              Storage::Module{
                                                                                  "Qml"}};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[1].module = Storage::Module{"QtQuick"};

    ASSERT_THROW(storage.synchronize({}, {}, {types[1]}, {sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, ChangePropertyTypeModuleIdWithExplicitType)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object",
                                                                              Storage::Module{
                                                                                  "Qml"}};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object",
                                                                              Storage::Module{
                                                                                  "QtQuick"}};
    types[1].module = Storage::Module{"QtQuick"};

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::propertyDeclarations,
                                     Contains(IsPropertyDeclaration(
                                         "data",
                                         Storage::NativeType{"QObject"},
                                         Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, AddFileStatuses)
{
    setUpSourceIds();
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};

    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus1, fileStatus2});

    ASSERT_THAT(convert(storage.fetchAllFileStatuses()),
                UnorderedElementsAre(fileStatus1, fileStatus2));
}

TEST_F(ProjectStorage, RemoveFileStatus)
{
    setUpSourceIds();
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus1, fileStatus2});

    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus1});

    ASSERT_THAT(convert(storage.fetchAllFileStatuses()), UnorderedElementsAre(fileStatus1));
}

TEST_F(ProjectStorage, UpdateFileStatus)
{
    setUpSourceIds();
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    FileStatus fileStatus2b{sourceId2, 102, 102};
    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus1, fileStatus2});

    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus1, fileStatus2b});

    ASSERT_THAT(convert(storage.fetchAllFileStatuses()),
                UnorderedElementsAre(fileStatus1, fileStatus2b));
}

TEST_F(ProjectStorage, ThrowForInvalidSourceId)
{
    setUpSourceIds();
    FileStatus fileStatus1{SourceId{}, 100, 100};

    ASSERT_THROW(storage.synchronize({}, {}, {}, {sourceId1}, {fileStatus1}),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorage, FetchAllFileStatuses)
{
    setUpSourceIds();
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus1, fileStatus2});

    auto fileStatuses = convert(storage.fetchAllFileStatuses());

    ASSERT_THAT(fileStatuses, ElementsAre(fileStatus1, fileStatus2));
}

TEST_F(ProjectStorage, FetchAllFileStatusesReverse)
{
    setUpSourceIds();
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus2, fileStatus1});

    auto fileStatuses = convert(storage.fetchAllFileStatuses());

    ASSERT_THAT(fileStatuses, ElementsAre(fileStatus1, fileStatus2));
}

TEST_F(ProjectStorage, FetchFileStatus)
{
    setUpSourceIds();
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus1, fileStatus2});

    auto fileStatus = storage.fetchFileStatus(sourceId1);

    ASSERT_THAT(fileStatus, Eq(fileStatus1));
}

TEST_F(ProjectStorage, SynchronizeTypesWithoutTypeName)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[3].typeName.clear();
    types[3].module.name.clear();
    types[3].prototype = Storage::ExportedType{"Object"};

    storage.synchronize({}, {}, {types[3]}, {sourceId4}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Module{"/path/to"},
                                             "QObject2",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId4),
                               Field(&Storage::Type::exportedTypes,
                                     UnorderedElementsAre(IsExportedType("Object2"),
                                                          IsExportedType("Obj2"))))));
}

} // namespace
