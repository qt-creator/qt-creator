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
           && Storage::ImportedTypeName{prototype} == type.prototype;
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
           && Storage::ImportedTypeName{typeName} == propertyDeclaration.typeName
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

MATCHER_P(IsModule,
          name,
          std::string(negation ? "isn't " : "is ") + PrintToString(Storage::Module{name}))
{
    const Storage::Module &module = arg;

    return module.name == name;
}

MATCHER_P2(IsModule,
           name,
           sourceId,
           std::string(negation ? "isn't " : "is ") + PrintToString(Storage::Module{name, sourceId}))
{
    const Storage::Module &module = arg;

    return module.name == name;
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
        imports.emplace_back("Qml", Storage::Version{}, sourceId1);
        imports.emplace_back("Qml", Storage::Version{}, sourceId2);
        imports.emplace_back("QtQuick", Storage::Version{}, sourceId1);

        importsSourceId1.emplace_back("Qml", Storage::Version{}, sourceId1);
        importsSourceId1.emplace_back("QtQuick", Storage::Version{}, sourceId1);

        importsSourceId2.emplace_back("Qml", Storage::Version{}, sourceId2);

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
                 Storage::SignalDeclaration{
                     "value0s",
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
            Storage::Type{Storage::Module{"Qml"},
                          "QObject",
                          Storage::NativeType{},
                          TypeAccessSemantics::Reference,
                          sourceId2,
                          {Storage::ExportedType{"Object", Storage::Version{2}},
                           Storage::ExportedType{"Obj", Storage::Version{2}}}}};
    }

    auto createVersionedTypes()
    {
        return Storage::Types{Storage::Type{Storage::Module{"Qml"},
                                            "QObject",
                                            Storage::NativeType{},
                                            TypeAccessSemantics::Reference,
                                            sourceId1,
                                            {Storage::ExportedType{"Object", Storage::Version{1}},
                                             Storage::ExportedType{"Obj", Storage::Version{1, 2}},
                                             Storage::ExportedType{"BuiltInObj", Storage::Version{}}}},
                              Storage::Type{Storage::Module{"Qml"},
                                            "QObject2",
                                            Storage::NativeType{},
                                            TypeAccessSemantics::Reference,
                                            sourceId1,
                                            {Storage::ExportedType{"Object", Storage::Version{2, 0}},
                                             Storage::ExportedType{"Obj", Storage::Version{2, 3}}}},
                              Storage::Type{Storage::Module{"Qml"},
                                            "QObject3",
                                            Storage::NativeType{},
                                            TypeAccessSemantics::Reference,
                                            sourceId1,
                                            {Storage::ExportedType{"Object", Storage::Version{2, 11}},
                                             Storage::ExportedType{"Obj", Storage::Version{2, 11}}}},
                              Storage::Type{Storage::Module{"Qml"},
                                            "QObject4",
                                            Storage::NativeType{},
                                            TypeAccessSemantics::Reference,
                                            sourceId1,
                                            {Storage::ExportedType{"Object", Storage::Version{3, 4}},
                                             Storage::ExportedType{"Obj", Storage::Version{3, 4}},
                                             Storage::ExportedType{"BuiltInObj",
                                                                   Storage::Version{3, 4}}}}};
    }

    auto createTypesWithExportedTypeNamesOnly()
    {
        auto types = createTypes();

        types[0].prototype = Storage::ImportedType{"Object"};
        types[0].propertyDeclarations[0].typeName = Storage::ImportedType{"Object"};

        return types;
    }

    auto createTypesWithAliases()
    {
        auto types = createTypes();

        imports.emplace_back("Qml", Storage::Version{}, sourceId3);
        imports.emplace_back("QtQuick", Storage::Version{}, sourceId3);

        imports.emplace_back("Qml", Storage::Version{}, sourceId4);
        imports.emplace_back("/path/to", Storage::Version{}, sourceId4);

        importsSourceId3.emplace_back("Qml", Storage::Version{}, sourceId3);
        importsSourceId3.emplace_back("QtQuick", Storage::Version{}, sourceId3);

        importsSourceId4.emplace_back("Qml", Storage::Version{}, sourceId4);
        importsSourceId4.emplace_back("/path/to", Storage::Version{}, sourceId4);

        types[1].propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"objects",
                                         Storage::NativeType{"QObject"},
                                         Storage::PropertyDeclarationTraits::IsList});

        types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                      "QAliasItem",
                                      Storage::ImportedType{"Item"},
                                      TypeAccessSemantics::Reference,
                                      sourceId3,
                                      {Storage::ExportedType{"AliasItem"}}});
        types.back().propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"data",
                                         Storage::NativeType{"QObject"},
                                         Storage::PropertyDeclarationTraits::IsList});
        types.back().propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"items", Storage::ImportedType{"Item"}, "children"});
        types.back().propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"objects", Storage::ImportedType{"Item"}, "objects"});

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
        imports.emplace_back("Qml", Storage::Version{}, sourceId5);
        imports.emplace_back("QtQuick", Storage::Version{}, sourceId5);

        auto types = createTypesWithAliases();
        types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                      "QAliasItem2",
                                      Storage::ImportedType{"Object"},
                                      TypeAccessSemantics::Reference,
                                      sourceId5,
                                      {Storage::ExportedType{"AliasItem2"}}});

        types.back().propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"objects", Storage::ImportedType{"AliasItem"}, "objects"});

        return types;
    }

    auto createTypesWithAliases2()
    {
        auto types = createTypesWithAliases();
        types[2].prototype = Storage::NativeType{"QObject"};
        types[2].propertyDeclarations.erase(std::next(types[2].propertyDeclarations.begin()));

        return types;
    }

    Storage::Modules createModules()
    {
        return Storage::Modules{Storage::Module{"Qml", moduleSourceId1},
                                Storage::Module{"QtQuick", moduleSourceId2},
                                Storage::Module{"/path/to", moduleSourceId3}};
    }

    Storage::Imports createImports(SourceId sourceId)
    {
        return Storage::Imports{Storage::Import{"Qml", Storage::Version{2}, sourceId},
                                Storage::Import{"QtQuick", Storage::Version{}, sourceId},
                                Storage::Import{"/path/to", Storage::Version{}, sourceId}};
    }

    static Storage::Imports createImports(const SourceIds &sourceIds)
    {
        Storage::Imports imports;
        imports.reserve(3 * sourceIds.size());

        for (SourceId sourceId : sourceIds) {
            imports.emplace_back("Qml", Storage::Version{2}, sourceId);
            imports.emplace_back("QtQuick", Storage::Version{}, sourceId);
            imports.emplace_back("/path/to", Storage::Version{}, sourceId);
        }

        return imports;
    }

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
    QmlDesigner::SourcePathView modulePath1{"/module/path1/to"};
    QmlDesigner::SourcePathView modulePath2{"/module/path2/to"};
    QmlDesigner::SourcePathView modulePath3{"/module/aaaa/to"};
    QmlDesigner::SourcePathView modulePath4{"/module/ooo/to"};
    QmlDesigner::SourcePathView modulePath5{"/module/xxx/to"};
    SourceId sourceId1{sourcePathCache.sourceId(path1)};
    SourceId sourceId2{sourcePathCache.sourceId(path2)};
    SourceId sourceId3{sourcePathCache.sourceId(path3)};
    SourceId sourceId4{sourcePathCache.sourceId(path4)};
    SourceId sourceId5{sourcePathCache.sourceId(path5)};
    SourceIds sourceIds{sourceId1, sourceId2, sourceId3, sourceId4, sourceId5};
    SourceId moduleSourceId1{sourcePathCache.sourceId(modulePath1)};
    SourceId moduleSourceId2{sourcePathCache.sourceId(modulePath2)};
    SourceId moduleSourceId3{sourcePathCache.sourceId(modulePath3)};
    SourceId moduleSourceId4{sourcePathCache.sourceId(modulePath4)};
    SourceId moduleSourceId5{sourcePathCache.sourceId(modulePath5)};
    Storage::Modules modules{createModules()};
    QmlDesigner::ModuleIds moduleIds{storage.fetchModuleIds(modules)};
    ModuleId moduleId1{moduleIds[0]};
    ModuleId moduleId2{moduleIds[1]};
    ModuleId moduleId3{moduleIds[2]};
    Storage::Imports imports;
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
    Storage::Types types{createTypes()};

    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml"},
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
    types[0].prototype = Storage::ImportedType{"Object"};

    storage.synchronize(modules,
                        importsSourceId1,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml"},
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

TEST_F(ProjectStorage, SynchronizeTypesAddsNewTypesThrowsWithWrongPrototypeName)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ImportedType{"Objec"};

    ASSERT_THROW(storage.synchronize(modules,
                                     imports,
                                     types,
                                     {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                                     {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesAddsNewTypesWithMissingModuleAndPrototypeName)
{
    Storage::Types types{createTypes()};
    types.push_back(Storage::Type{Storage::Module{"/path/to"},
                                  "QObject2",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object2"}, Storage::ExportedType{"Obj2"}}});
    storage.synchronize(modules,
                        imports,
                        {},
                        {sourceId1, sourceId2, sourceId3, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[1].prototype = Storage::ImportedType{"Object2"};

    ASSERT_THROW(storage.synchronize({}, importsSourceId2, {types[1]}, {sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesAddsNewTypesWithMissingModule)
{
    Storage::Types types{createTypes()};
    storage.synchronize(modules,
                        imports,
                        {},
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

    ASSERT_THROW(storage.synchronize({},
                                     imports,
                                     types,
                                     {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                                     {}),
                 QmlDesigner::ModuleDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesAddsNewTypesReverseOrder)
{
    Storage::Types types{createTypes()};
    std::reverse(types.begin(), types.end());

    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml"},
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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].accessSemantics = TypeAccessSemantics::Value;
    types[1].accessSemantics = TypeAccessSemantics::Value;

    storage.synchronize({}, imports, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml"},
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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].sourceId = sourceId3;
    types[1].sourceId = sourceId4;
    Storage::Imports newImports;
    newImports.emplace_back("Qml", Storage::Version{}, sourceId3);
    newImports.emplace_back("Qml", Storage::Version{}, sourceId4);
    newImports.emplace_back("QtQuick", Storage::Version{}, sourceId3);

    storage.synchronize({}, newImports, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml"},
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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].prototype = Storage::NativeType{"QQuickObject"};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{"QObject"},
                                  TypeAccessSemantics::Reference,
                                  sourceId1,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize({}, importsSourceId1, {types[0], types[2]}, {sourceId1}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml"},
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

TEST_F(ProjectStorage, SynchronizeTypesAddQualifiedPrototype)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::QualifiedImportedType{"Object",
                                                        Storage::Import{"QtQuick",
                                                                        Storage::Version{},
                                                                        sourceId1}};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{"QObject"},
                                  TypeAccessSemantics::Reference,
                                  sourceId1,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml"},
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
    sourceId1 = sourcePathCache.sourceId(path1);
    Storage::Types types{Storage::Type{Storage::Module{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1,
                                       {Storage::ExportedType{"Item"}}}};

    ASSERT_THROW(storage.synchronize(modules,
                                     imports,
                                     types,
                                     {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                                     {}),
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

    ASSERT_THROW(storage.synchronize({}, imports, types, {sourceId1}, {}),
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

    ASSERT_THROW(storage.synchronize(modules,
                                     imports,
                                     types,
                                     {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                                     {}),
                 QmlDesigner::TypeHasInvalidSourceId);
}

TEST_F(ProjectStorage, DeleteTypeIfSourceIdIsSynchronized)
{
    Storage::Types types{createTypes()};
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types.erase(types.begin());

    storage.synchronize({}, importsSourceId2, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml"},
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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types.pop_back();

    storage.synchronize({}, importsSourceId1, types, {sourceId1}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml"},
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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].typeName = "QQuickItem2";

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Module{"Qml"},
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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types.pop_back();

    ASSERT_THROW(storage.synchronize({}, importsSourceId1, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesAddPropertyDeclarations)
{
    Storage::Types types{createTypes()};

    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

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

TEST_F(ProjectStorage, SynchronizeTypesAddPropertyDeclarationsWithMissingImportsForNativeTypes)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations.pop_back();

    ASSERT_THROW(storage.synchronize(modules,
                                     {},
                                     types,
                                     {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                                     {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesAddPropertyDeclarationsWithMissingImportsForExportedTypes)
{
    Storage::Types types{createTypes()};
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].propertyDeclarations[0].typeName = Storage::ImportedType{"Obj"};

    ASSERT_THROW(storage.synchronize({}, {}, {types[0]}, {sourceId1}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesAddPropertyDeclarationQualifiedType)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object", Storage::Import{"QtQuick", Storage::Version{}, sourceId1}};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{"QObject"},
                                  TypeAccessSemantics::Reference,
                                  sourceId1,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].propertyDeclarations[0].typeName = Storage::NativeType{"QQuickItem"};

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});
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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsPointer;

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsPointer;
    types[0].propertyDeclarations[0].typeName = Storage::NativeType{"QQuickItem"};

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].propertyDeclarations.pop_back();

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"object",
                                     Storage::NativeType{"QObject"},
                                     Storage::PropertyDeclarationTraits::IsPointer});

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].propertyDeclarations[1].name = "objects";

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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

    ASSERT_THROW(storage.synchronize(modules,
                                     importsSourceId1,
                                     types,
                                     {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                                     {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, UsingNonExistingExportedPropertyTypeThrows)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ImportedType{"QObject2"};
    types.pop_back();

    ASSERT_THROW(storage.synchronize(modules,
                                     importsSourceId1,
                                     types,
                                     {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                                     {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, UsingNonExistingQualifiedExportedPropertyTypeWithWrongNameThrows)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "QObject2", Storage::Import{"Qml", Storage::Version{}, sourceId1}};
    types.pop_back();

    ASSERT_THROW(storage.synchronize(modules,
                                     importsSourceId1,
                                     types,
                                     {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                                     {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, UsingNonExistingQualifiedExportedPropertyTypeWithWrongModuleThrows)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "QObject", Storage::Import{"QtQuick", Storage::Version{}, sourceId1}};
    types.pop_back();

    ASSERT_THROW(storage.synchronize(modules,
                                     importsSourceId1,
                                     types,
                                     {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                                     {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, BreakingPropertyDeclarationTypeDependencyByDeletingTypeThrows)
{
    Storage::Types types{createTypes()};
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].prototype = Storage::NativeType{};
    types.pop_back();

    ASSERT_THROW(storage.synchronize(modules,
                                     importsSourceId1,
                                     types,
                                     {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                                     {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesAddFunctionDeclarations)
{
    Storage::Types types{createTypes()};

    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].functionDeclarations[1].returnTypeName = "item";

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].functionDeclarations[1].name = "name";

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].functionDeclarations[1].parameters.pop_back();

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].functionDeclarations[1].parameters.push_back(Storage::ParameterDeclaration{"arg4", "int"});

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].functionDeclarations[1].parameters[0].name = "other";

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].functionDeclarations[1].parameters[0].name = "long long";

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].functionDeclarations[1].parameters[0].traits = QmlDesigner::Storage::PropertyDeclarationTraits::IsList;

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].functionDeclarations.pop_back();

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].functionDeclarations.push_back(
        Storage::FunctionDeclaration{"name", "string", {Storage::ParameterDeclaration{"arg", "int"}}});

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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

    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].signalDeclarations[1].name = "name";

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].signalDeclarations[1].parameters.pop_back();

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].signalDeclarations[1].parameters.push_back(Storage::ParameterDeclaration{"arg4", "int"});

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].signalDeclarations[1].parameters[0].name = "other";

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].signalDeclarations[1].parameters[0].typeName = "long long";

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].signalDeclarations[1].parameters[0].traits = QmlDesigner::Storage::PropertyDeclarationTraits::IsList;

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].signalDeclarations.pop_back();

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].signalDeclarations.push_back(
        Storage::SignalDeclaration{"name", {Storage::ParameterDeclaration{"arg", "int"}}});

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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

    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].enumerationDeclarations[1].name = "Name";

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations.pop_back();

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations.push_back(
        Storage::EnumeratorDeclaration{"Haa", 54});

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations[0].name = "Hoo";

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations[1].value = 11;

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations[0].value = 11;
    types[0].enumerationDeclarations[1].enumeratorDeclarations[0].hasValue = true;

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations[0].hasValue = false;

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].enumerationDeclarations.pop_back();

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].enumerationDeclarations.push_back(
        Storage::EnumerationDeclaration{"name", {Storage::EnumeratorDeclaration{"Foo", 98, true}}});

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    Storage::Modules modules{createModules()};

    storage.synchronize(modules, {}, {}, {moduleSourceId1, moduleSourceId2, moduleSourceId5}, {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(IsModule("Qml", moduleSourceId1),
                                     IsModule("QtQuick", moduleSourceId2),
                                     IsModule("/path/to", moduleSourceId3)));
}

TEST_F(ProjectStorage, SynchronizeModulesAddModulesAgain)
{
    Storage::Modules modules{createModules()};
    storage.synchronize(modules, {}, {}, {moduleSourceId1, moduleSourceId2, moduleSourceId3}, {});

    storage.synchronize(modules, {}, {}, {moduleSourceId1, moduleSourceId2, moduleSourceId3}, {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(IsModule("Qml", moduleSourceId1),
                                     IsModule("QtQuick", moduleSourceId2),
                                     IsModule("/path/to", moduleSourceId3)));
}

TEST_F(ProjectStorage, SynchronizeModulesUpdateToMoreModules)
{
    Storage::Modules modules{createModules()};
    storage.synchronize(modules, {}, {}, {moduleSourceId1, moduleSourceId2, moduleSourceId3}, {});
    modules.push_back(Storage::Module{"QtQuick.Foo", moduleSourceId4});

    storage.synchronize(modules,
                        {},
                        {},
                        {moduleSourceId1, moduleSourceId2, moduleSourceId3, moduleSourceId4},
                        {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(IsModule("Qml", moduleSourceId1),
                                     IsModule("QtQuick", moduleSourceId2),
                                     IsModule("/path/to", moduleSourceId3),
                                     IsModule("QtQuick.Foo", moduleSourceId4)));
}

TEST_F(ProjectStorage, SynchronizeModulesAddOneMoreModules)
{
    Storage::Modules modules{createModules()};
    storage.synchronize(modules, {}, {}, {moduleSourceId1, moduleSourceId2, moduleSourceId3}, {});
    auto newModule = Storage::Module{"QtQuick.Foo", moduleSourceId4};

    storage.synchronize({newModule}, {}, {}, {moduleSourceId4}, {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(IsModule("Qml", moduleSourceId1),
                                     IsModule("QtQuick", moduleSourceId2),
                                     IsModule("/path/to", moduleSourceId3),
                                     IsModule("QtQuick.Foo", moduleSourceId4)));
}

TEST_F(ProjectStorage, SynchronizeModulesRemoveModule)
{
    Storage::Modules modules{createModules()};
    storage.synchronize(modules, {}, {}, {moduleSourceId1, moduleSourceId2, moduleSourceId3}, {});

    storage.synchronize({}, {}, {}, {moduleSourceId3}, {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(IsModule("Qml", moduleSourceId1),
                                     IsModule("QtQuick", moduleSourceId2)));
}

TEST_F(ProjectStorage, SynchronizeModulesChangeSourceId)
{
    Storage::Modules modules{createModules()};
    storage.synchronize(modules, {}, {}, {moduleSourceId1, moduleSourceId2, moduleSourceId3}, {});
    modules[1].sourceId = moduleSourceId4;

    storage.synchronize({modules[1]}, {}, {}, {moduleSourceId2, moduleSourceId4}, {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(IsModule("Qml", moduleSourceId1),
                                     IsModule("QtQuick", moduleSourceId4),
                                     IsModule("/path/to", moduleSourceId3)));
}

TEST_F(ProjectStorage, SynchronizeModulesChangeName)
{
    Storage::Modules modules{createModules()};
    storage.synchronize(modules, {}, {}, {moduleSourceId1, moduleSourceId2, moduleSourceId3}, {});
    modules[0].name = "Qml2";

    storage.synchronize(modules, {}, {}, {moduleSourceId1, moduleSourceId2, moduleSourceId3}, {});

    ASSERT_THAT(storage.fetchAllModules(),
                UnorderedElementsAre(IsModule("Qml2", moduleSourceId1),
                                     IsModule("QtQuick", moduleSourceId2),
                                     IsModule("/path/to", moduleSourceId3)));
}

TEST_F(ProjectStorage, RemovingModuleRemovesDependentTypesToo)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::NativeType{""};
    types[0].propertyDeclarations.clear();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

    storage.synchronize({}, {}, {}, {moduleSourceId2, moduleSourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(IsStorageType(Storage::Module{"Qml"},
                                                   "QObject",
                                                   Storage::NativeType{},
                                                   TypeAccessSemantics::Reference,
                                                   sourceId2)));
}

TEST_F(ProjectStorage, RemovingModuleThrowsForMissingType)
{
    Storage::Types types{createTypes()};
    imports.emplace_back("QtQuick", Storage::Version{}, sourceId2);
    types[0].prototype = Storage::NativeType{""};
    types[0].propertyDeclarations.clear();
    types[1].prototype = Storage::ImportedType{"Item"};
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

    ASSERT_THROW(storage.synchronize({}, {}, {}, {moduleSourceId2, moduleSourceId3}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, FetchTypeIdByModuleIdAndName)
{
    Storage::Types types{createTypes()};
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    auto qmlModuleId = storage.fetchModuleId("Qml");

    auto typeId = storage.fetchTypeIdByName(qmlModuleId, "QObject");

    ASSERT_THAT(storage.fetchTypeIdByExportedName("Object"), Eq(typeId));
}

TEST_F(ProjectStorage, FetchTypeIdByExportedName)
{
    Storage::Types types{createTypes()};
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    auto qmlModuleId = storage.fetchModuleId("Qml");

    auto typeId = storage.fetchTypeIdByExportedName("Object");

    ASSERT_THAT(storage.fetchTypeIdByName(qmlModuleId, "QObject"), Eq(typeId));
}

TEST_F(ProjectStorage, FetchTypeIdByImporIdsAndExportedName)
{
    Storage::Types types{createTypes()};
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    auto qmlModuleId = storage.fetchModuleId("Qml");
    auto qtQuickModuleId = storage.fetchModuleId("QtQuick");

    auto typeId = storage.fetchTypeIdByModuleIdsAndExportedName({qmlModuleId, qtQuickModuleId},
                                                                "Object");

    ASSERT_THAT(storage.fetchTypeIdByName(qmlModuleId, "QObject"), Eq(typeId));
}

TEST_F(ProjectStorage, FetchInvalidTypeIdByImporIdsAndExportedNameIfModuleIdsAreEmpty)
{
    Storage::Types types{createTypes()};
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    auto typeId = storage.fetchTypeIdByModuleIdsAndExportedName({}, "Object");

    ASSERT_FALSE(typeId.isValid());
}

TEST_F(ProjectStorage, FetchInvalidTypeIdByImporIdsAndExportedNameIfModuleIdsAreInvalid)
{
    Storage::Types types{createTypes()};
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    auto typeId = storage.fetchTypeIdByModuleIdsAndExportedName({ModuleId{}}, "Object");

    ASSERT_FALSE(typeId.isValid());
}

TEST_F(ProjectStorage, FetchInvalidTypeIdByImporIdsAndExportedNameIfNotInModule)
{
    Storage::Types types{createTypes()};
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    auto qtQuickModuleId = storage.fetchModuleId("QtQuick");
    auto typeId = storage.fetchTypeIdByModuleIdsAndExportedName({qtQuickModuleId}, "Object");

    ASSERT_FALSE(typeId.isValid());
}

TEST_F(ProjectStorage, SynchronizeTypesAddAliasDeclarations)
{
    Storage::Types types{createTypesWithAliases()};

    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});

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
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});

    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});

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
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});
    types[2].propertyDeclarations.pop_back();

    storage.synchronize({}, importsSourceId3, {types[2]}, {sourceId3}, {});

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

    ASSERT_THROW(storage.synchronize(modules, importsSourceId4, {types[2]}, {sourceId4}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}
TEST_F(ProjectStorage, SynchronizeTypesAddAliasDeclarationsThrowsForWrongPropertyName)
{
    Storage::Types types{createTypesWithAliases()};
    types[2].propertyDeclarations[1].aliasPropertyName = "childrenWrong";

    ASSERT_THROW(storage.synchronize(modules, imports, types, {sourceId4}, {}),
                 QmlDesigner::PropertyNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesChangeAliasDeclarationsTypeName)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});
    types[2].propertyDeclarations[2].typeName = Storage::ImportedType{"Obj2"};
    importsSourceId3.emplace_back("/path/to", Storage::Version{}, sourceId3);

    storage.synchronize({}, importsSourceId3, {types[2]}, {sourceId3}, {});

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
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});
    types[2].propertyDeclarations[2].aliasPropertyName = "children";

    storage.synchronize({}, importsSourceId3, {types[2]}, {sourceId3}, {});

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
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});
    types[2].propertyDeclarations.pop_back();
    types[2].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::NativeType{"QQuickItem"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize({}, importsSourceId3, {types[2]}, {sourceId3}, {});

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
    storage.synchronize(
        modules,
        imports,
        typesChanged,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});

    storage.synchronize({}, imports, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});

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
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});
    types[1].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsList
                                              | Storage::PropertyDeclarationTraits::IsReadOnly;

    storage.synchronize({}, importsSourceId2, {types[1]}, {sourceId2}, {});

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
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});
    types[1].propertyDeclarations[0].typeName = Storage::ImportedType{"Item"};
    importsSourceId2.emplace_back("QtQuick", Storage::Version{}, sourceId2);

    storage.synchronize({}, importsSourceId2, {types[1]}, {sourceId2}, {});

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
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});
    types[1].propertyDeclarations.pop_back();

    ASSERT_THROW(storage.synchronize({}, importsSourceId2, {types[1]}, {sourceId2}, {}),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorage, SynchronizeTypesRemovePropertyDeclarationAndAlias)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});
    types[1].propertyDeclarations.pop_back();
    types[2].propertyDeclarations.pop_back();

    storage.synchronize({},
                        importsSourceId2 + importsSourceId3,
                        {types[1], types[2]},
                        {sourceId2, sourceId3},
                        {});

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
    types[2].propertyDeclarations[2].typeName = Storage::ImportedType{"Object2"};
    imports.emplace_back("/path/to", Storage::Version{}, sourceId3);
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});

    ASSERT_THROW(storage.synchronize({}, {}, {}, {sourceId4}, {}), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, SynchronizeTypesRemoveTypeAndAliasPropertyDeclaration)
{
    Storage::Types types{createTypesWithAliases()};
    types[2].propertyDeclarations[2].typeName = Storage::ImportedType{"Object2"};
    imports.emplace_back("/path/to", Storage::Version{}, sourceId3);
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});
    types[2].propertyDeclarations.pop_back();

    storage.synchronize({},
                        importsSourceId1 + importsSourceId3,
                        {types[0], types[2]},
                        {sourceId1, sourceId3},
                        {});

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
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});
    types[0].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::NativeType{"QQuickItem"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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

    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});

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
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});
    types[0].propertyDeclarations.pop_back();

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    imports.emplace_back("/path/to", Storage::Version{}, sourceId2);
    imports.emplace_back("QtQuick", Storage::Version{}, sourceId2);
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});
    types[3].module = Storage::Module{"QtQuick"};

    storage.synchronize({}, importsSourceId4, {types[3]}, {sourceId4}, {});

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

TEST_F(ProjectStorage, DoNotRelinkAliasPropertyForQualifiedImportedTypeName)
{
    Storage::Types types{createTypesWithAliases()};
    types[1].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object2", Storage::Import{"/path/to", Storage::Version{}, sourceId2}};
    imports.emplace_back("/path/to", Storage::Version{}, sourceId2);
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});
    types[3].module = Storage::Module{"QtQuick"};
    importsSourceId4.emplace_back("QtQuick", Storage::Version{}, sourceId4);

    ASSERT_THROW(storage.synchronize({}, importsSourceId4, {types[3]}, {sourceId4}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage,
       DoRelinkAliasPropertyForQualifiedImportedTypeNameEvenIfAnOtherSimilarTimeNameExists)
{
    Storage::Types types{createTypesWithAliases()};
    types[1].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object2", Storage::Import{"/path/to", Storage::Version{}, sourceId2}};
    imports.emplace_back("/path/to", Storage::Version{}, sourceId2);
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QObject2",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId5,
                                  {Storage::ExportedType{"Object2"}, Storage::ExportedType{"Obj2"}}});

    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1,
                         sourceId2,
                         sourceId3,
                         sourceId4,
                         sourceId5,
                         moduleSourceId1,
                         moduleSourceId2,
                         moduleSourceId3},
                        {});

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
        Storage::PropertyDeclaration{"items", Storage::ImportedType{"Item"}, "children"});
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});
    types[0].typeName = "QQuickItem2";

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    imports.emplace_back("/path/to", Storage::Version{}, sourceId2);
    imports.emplace_back("QtQuick", Storage::Version{}, sourceId2);

    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});
    types[3].module = Storage::Module{"QtQuick"};

    storage.synchronize({}, importsSourceId4, {types[3]}, {sourceId3, sourceId4}, {});

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
    imports.emplace_back("/path/to", Storage::Version{}, sourceId2);
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});
    types[0].prototype = Storage::NativeType{};
    importsSourceId1.emplace_back("/path/to", Storage::Version{}, sourceId1);
    importsSourceId4.emplace_back("QtQuick", Storage::Version{}, sourceId4);
    types[0].propertyDeclarations[0].typeName = Storage::ImportedType{"Object2"};
    types[3].propertyDeclarations[0].typeName = Storage::ImportedType{"Item"};

    storage.synchronize({},
                        importsSourceId1 + importsSourceId4,
                        {types[0], types[3]},
                        {sourceId1, sourceId2, sourceId3, sourceId4},
                        {});

    ASSERT_THAT(storage.fetchTypes(), SizeIs(2));
}

TEST_F(ProjectStorage, DoNotRelinkAliasPropertyForDeletedTypeAndPropertyTypeNameChange)
{
    Storage::Types types{createTypesWithAliases()};
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject2"};
    imports.emplace_back("/path/to", Storage::Version{}, sourceId2);
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});
    types[3].module = Storage::Module{"QtQuick"};
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject"};
    importsSourceId4.emplace_back("QtQuick", Storage::Version{}, sourceId4);

    storage.synchronize({},
                        importsSourceId2 + importsSourceId4,
                        {types[1], types[3]},
                        {sourceId2, sourceId3, sourceId4},
                        {});

    ASSERT_THAT(storage.fetchTypes(),
                Not(Contains(IsStorageType(Storage::Module{"QtQuick"},
                                           "QAliasItem",
                                           Storage::NativeType{"QQuickItem"},
                                           TypeAccessSemantics::Reference,
                                           sourceId3))));
}

TEST_F(ProjectStorage, DoNotRelinkPropertyTypeDoesNotExists)
{
    Storage::Types types{createTypesWithAliases()};
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject2"};
    imports.emplace_back("/path/to", Storage::Version{}, sourceId2);
    imports.emplace_back("QtQuick", Storage::Version{}, sourceId2);
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});
    types.pop_back();

    ASSERT_THROW(storage.synchronize({}, {}, {}, {sourceId4}, {}), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, DoNotRelinkAliasPropertyTypeDoesNotExists)
{
    Storage::Types types{createTypesWithAliases2()};
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject2"};
    imports.emplace_back("/path/to", Storage::Version{}, sourceId2);
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});

    ASSERT_THROW(storage.synchronize({}, {}, {}, {sourceId1}, {}), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, ChangePrototypeTypeName)
{
    Storage::Types types{createTypesWithExportedTypeNamesOnly()};
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[1].typeName = "QObject3";

    storage.synchronize({}, importsSourceId2, {types[1]}, {sourceId2}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[1].module = Storage::Module{"QtQuick"};

    storage.synchronize({}, importsSourceId2, {types[1]}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorage, ChangeQualifiedPrototypeTypeModuleIdThows)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::QualifiedImportedType{"Object",
                                                        Storage::Import{"Qml",
                                                                        Storage::Version{},
                                                                        sourceId1}};
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[1].module = Storage::Module{"QtQuick"};

    ASSERT_THROW(storage.synchronize({}, importsSourceId2, {types[1]}, {sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, ChangeQualifiedPrototypeTypeModuleId)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::QualifiedImportedType{"Object",
                                                        Storage::Import{"Qml",
                                                                        Storage::Version{},
                                                                        sourceId1}};
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[1].module = Storage::Module{"QtQuick"};
    types[0].prototype = Storage::QualifiedImportedType{"Object",
                                                        Storage::Import{"QtQuick",
                                                                        Storage::Version{},
                                                                        sourceId1}};

    storage.synchronize({},
                        importsSourceId1 + importsSourceId2,
                        {types[0], types[1]},
                        {sourceId1, sourceId2},
                        {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[1].module = Storage::Module{"QtQuick"};
    types[1].typeName = "QObject3";

    storage.synchronize({}, importsSourceId2, {types[1]}, {sourceId2}, {});

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
    types[0].propertyDeclarations[0].typeName = Storage::ImportedType{"Object"};
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[1].typeName = "QObject3";

    ASSERT_THROW(storage.synchronize({}, importsSourceId2, {types[1]}, {sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, ThrowForPrototypeChainCycles)
{
    Storage::Types types{createTypes()};
    types[1].prototype = Storage::ImportedType{"Object2"};
    types.push_back(Storage::Type{Storage::Module{"/path/to"},
                                  "QObject2",
                                  Storage::ImportedType{"Item"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object2"}, Storage::ExportedType{"Obj2"}}});
    imports.emplace_back("/path/to", Storage::Version{}, sourceId2);
    imports.emplace_back("QtQuick", Storage::Version{}, sourceId3);
    imports.emplace_back("/path/to", Storage::Version{}, sourceId3);

    ASSERT_THROW(storage.synchronize(
                     modules,
                     imports,
                     types,
                     {sourceId1, sourceId2, sourceId3, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                     {}),
                 QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorage, ThrowForTypeIdAndPrototypeIdAreTheSame)
{
    Storage::Types types{createTypes()};
    types[1].prototype = Storage::ImportedType{"Object"};

    ASSERT_THROW(storage.synchronize(modules,
                                     imports,
                                     types,
                                     {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                                     {}),
                 QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorage, ThrowForTypeIdAndPrototypeIdAreTheSameForRelinking)
{
    Storage::Types types{createTypesWithExportedTypeNamesOnly()};
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[1].prototype = Storage::ImportedType{"Item"};
    types[1].typeName = "QObject2";
    importsSourceId2.emplace_back("QtQuick", Storage::Version{}, sourceId2);

    ASSERT_THROW(storage.synchronize({}, importsSourceId2, {types[1]}, {sourceId2}, {}),
                 QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorage, RecursiveAliases)
{
    Storage::Types types{createTypesWithRecursiveAliases()};

    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1,
                         sourceId2,
                         sourceId3,
                         sourceId4,
                         sourceId5,
                         moduleSourceId1,
                         moduleSourceId2,
                         moduleSourceId3},
                        {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1,
                         sourceId2,
                         sourceId3,
                         sourceId4,
                         sourceId5,
                         moduleSourceId1,
                         moduleSourceId2,
                         moduleSourceId3},
                        {});
    types[1].propertyDeclarations[0].typeName = Storage::ImportedType{"Object2"};
    importsSourceId2.emplace_back("/path/to", Storage::Version{}, sourceId2);

    storage.synchronize({}, importsSourceId2, {types[1]}, {sourceId2}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1,
                         sourceId2,
                         sourceId3,
                         sourceId4,
                         sourceId5,
                         moduleSourceId1,
                         moduleSourceId2,
                         moduleSourceId3},
                        {});
    types[0].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::ImportedType{"Item"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize({}, importsSourceId1, {types[0]}, {sourceId1}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1,
                         sourceId2,
                         sourceId3,
                         sourceId4,
                         sourceId5,
                         moduleSourceId1,
                         moduleSourceId2,
                         moduleSourceId3},
                        {});
    types[2].propertyDeclarations.clear();
    types[2].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::ImportedType{"Item"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize({}, importsSourceId3, {types[2]}, {sourceId3}, {});

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
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1,
                         sourceId2,
                         sourceId3,
                         sourceId4,
                         sourceId5,
                         moduleSourceId1,
                         moduleSourceId2,
                         moduleSourceId3},
                        {});
    types[1].propertyDeclarations.clear();
    types[1].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects", Storage::ImportedType{"Object2"}, "objects"});
    importsSourceId2.emplace_back("/path/to", Storage::Version{}, sourceId2);

    storage.synchronize({}, importsSourceId2, {types[1]}, {sourceId2}, {});

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
        Storage::PropertyDeclaration{"objects", Storage::ImportedType{"AliasItem2"}, "objects"});
    imports.emplace_back("QtQuick", Storage::Version{}, sourceId2);

    ASSERT_THROW(storage.synchronize(modules,
                                     imports,
                                     types,
                                     {sourceId1,
                                      sourceId2,
                                      sourceId3,
                                      sourceId4,
                                      sourceId5,
                                      moduleSourceId1,
                                      moduleSourceId2,
                                      moduleSourceId3},
                                     {}),
                 QmlDesigner::AliasChainCycle);
}

TEST_F(ProjectStorage, CheckForProtoTypeCycleAfterUpdateThrows)
{
    Storage::Types types{createTypesWithRecursiveAliases()};
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1,
                         sourceId2,
                         sourceId3,
                         sourceId4,
                         sourceId5,
                         moduleSourceId1,
                         moduleSourceId2,
                         moduleSourceId3},
                        {});
    types[1].propertyDeclarations.clear();
    types[1].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects", Storage::ImportedType{"AliasItem2"}, "objects"});
    importsSourceId2.emplace_back("QtQuick", Storage::Version{}, sourceId2);

    ASSERT_THROW(storage.synchronize({}, importsSourceId2, {types[1]}, {sourceId2}, {}),
                 QmlDesigner::AliasChainCycle);
}

TEST_F(ProjectStorage, QualifiedPrototype)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::QualifiedImportedType{"Object",
                                                        Storage::Import{"Qml",
                                                                        Storage::Version{},
                                                                        sourceId1}};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});
    imports.emplace_back("QtQuick", Storage::Version{}, sourceId3);

    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, sourceId3, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorage, QualifiedPrototypeUpperDownTheModuleChainThrows)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::QualifiedImportedType{"Object",
                                                        Storage::Import{"QtQuick",
                                                                        Storage::Version{},
                                                                        sourceId1}};

    ASSERT_THROW(storage.synchronize(modules,
                                     imports,
                                     types,
                                     {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                                     {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, QualifiedPrototypeUpperInTheModuleChain)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::QualifiedImportedType{"Object",
                                                        Storage::Import{"QtQuick",
                                                                        Storage::Version{},
                                                                        sourceId1}};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});
    imports.emplace_back("QtQuick", Storage::Version{}, sourceId3);

    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, sourceId3, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QQuickObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorage, QualifiedPrototypeWithWrongVersionThrows)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::QualifiedImportedType{"Object",
                                                        Storage::Import{"Qml",
                                                                        Storage::Version{4},
                                                                        sourceId1}};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});
    imports.emplace_back("QtQuick", Storage::Version{}, sourceId3);

    ASSERT_THROW(storage.synchronize(
                     modules,
                     imports,
                     types,
                     {sourceId1, sourceId2, sourceId3, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                     {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, QualifiedPrototypeWithVersion)
{
    Storage::Types types{createTypes()};
    imports[0].version = Storage::Version{2};
    types[0].prototype = Storage::QualifiedImportedType{"Object", imports[0]};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});
    imports.emplace_back("QtQuick", Storage::Version{}, sourceId3);

    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, sourceId3, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorage, QualifiedPrototypeWithVersionInTheProtoTypeChain)
{
    Storage::Types types{createTypes()};
    imports[2].version = Storage::Version{2};
    types[0].prototype = Storage::QualifiedImportedType{"Object", imports[2]};
    types[0].exportedTypes[0].version = Storage::Version{2};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object", Storage::Version{2}}}});
    imports.emplace_back("QtQuick", Storage::Version{}, sourceId3);

    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, sourceId3, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QQuickObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorage, QualifiedPrototypeWithVersionDownTheProtoTypeChainThrows)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::QualifiedImportedType{"Object",
                                                        Storage::Import{"QtQuick",
                                                                        Storage::Version{2},
                                                                        sourceId1}};

    ASSERT_THROW(storage.synchronize(
                     modules,
                     imports,
                     types,
                     {sourceId1, sourceId2, sourceId3, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                     {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, QualifiedPropertyDeclarationTypeName)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object", Storage::Import{"Qml", Storage::Version{}, sourceId1}};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});
    imports.emplace_back("QtQuick", Storage::Version{}, sourceId3);

    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, sourceId3, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    Field(&Storage::Type::propertyDeclarations,
                          Contains(IsPropertyDeclaration("data",
                                                         Storage::NativeType{"QObject"},
                                                         Storage::PropertyDeclarationTraits::IsList)))));
}

TEST_F(ProjectStorage, QualifiedPropertyDeclarationTypeNameDownTheModuleChainThrows)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object", Storage::Import{"QtQuick", Storage::Version{}, sourceId1}};

    ASSERT_THROW(storage.synchronize(
                     modules,
                     imports,
                     types,
                     {sourceId1, sourceId2, sourceId3, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                     {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, QualifiedPropertyDeclarationTypeNameInTheModuleChain)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object", Storage::Import{"QtQuick", Storage::Version{}, sourceId1}};
    types.push_back(Storage::Type{Storage::Module{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});
    imports.emplace_back("QtQuick", Storage::Version{}, sourceId3);

    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, sourceId3, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    Field(&Storage::Type::propertyDeclarations,
                          Contains(IsPropertyDeclaration("data",
                                                         Storage::NativeType{"QQuickObject"},
                                                         Storage::PropertyDeclarationTraits::IsList)))));
}

TEST_F(ProjectStorage, QualifiedPropertyDeclarationTypeNameWithVersion)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object", Storage::Import{"Qml", Storage::Version{2}, sourceId1}};
    imports.emplace_back("Qml", Storage::Version{2}, sourceId1);

    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    Field(&Storage::Type::propertyDeclarations,
                          Contains(IsPropertyDeclaration("data",
                                                         Storage::NativeType{"QObject"},
                                                         Storage::PropertyDeclarationTraits::IsList)))));
}

TEST_F(ProjectStorage, ChangePropertyTypeModuleIdWithQualifiedTypeThrows)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object", Storage::Import{"Qml", Storage::Version{}, sourceId1}};
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[1].module = Storage::Module{"QtQuick"};

    ASSERT_THROW(storage.synchronize({}, importsSourceId2, {types[1]}, {sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, ChangePropertyTypeModuleIdWithQualifiedType)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object", Storage::Import{"Qml", Storage::Version{}, sourceId1}};
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    types[0].propertyDeclarations[0].typeName = Storage::QualifiedImportedType{
        "Object", Storage::Import{"QtQuick", Storage::Version{}, sourceId1}};
    types[1].module = Storage::Module{"QtQuick"};
    imports.emplace_back("QtQuick", Storage::Version{}, sourceId2);

    storage.synchronize({}, imports, types, {sourceId1, sourceId2}, {});

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
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};

    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus1, fileStatus2});

    ASSERT_THAT(convert(storage.fetchAllFileStatuses()),
                UnorderedElementsAre(fileStatus1, fileStatus2));
}

TEST_F(ProjectStorage, RemoveFileStatus)
{
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus1, fileStatus2});

    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus1});

    ASSERT_THAT(convert(storage.fetchAllFileStatuses()), UnorderedElementsAre(fileStatus1));
}

TEST_F(ProjectStorage, UpdateFileStatus)
{
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
    FileStatus fileStatus1{SourceId{}, 100, 100};

    ASSERT_THROW(storage.synchronize({}, {}, {}, {sourceId1}, {fileStatus1}),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorage, FetchAllFileStatuses)
{
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus1, fileStatus2});

    auto fileStatuses = convert(storage.fetchAllFileStatuses());

    ASSERT_THAT(fileStatuses, ElementsAre(fileStatus1, fileStatus2));
}

TEST_F(ProjectStorage, FetchAllFileStatusesReverse)
{
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus2, fileStatus1});

    auto fileStatuses = convert(storage.fetchAllFileStatuses());

    ASSERT_THAT(fileStatuses, ElementsAre(fileStatus1, fileStatus2));
}

TEST_F(ProjectStorage, FetchFileStatus)
{
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus1, fileStatus2});

    auto fileStatus = storage.fetchFileStatus(sourceId1);

    ASSERT_THAT(fileStatus, Eq(fileStatus1));
}

TEST_F(ProjectStorage, SynchronizeTypesWithoutTypeName)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize(
        modules,
        imports,
        types,
        {sourceId1, sourceId2, sourceId3, sourceId4, moduleSourceId1, moduleSourceId2, moduleSourceId3},
        {});
    types[3].typeName.clear();
    types[3].module.name.clear();
    types[3].prototype = Storage::ImportedType{"Object"};

    storage.synchronize({}, importsSourceId4, {types[3]}, {sourceId4}, {});

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

TEST_F(ProjectStorage, FetchByMajorVersionForImportedType)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::ImportedType{"Object"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};
    Storage::Import import{"Qml", Storage::Version{1}, sourceId2};

    storage.synchronize({}, {import}, {type}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "Item",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId2)));
}

TEST_F(ProjectStorage, FetchByMajorVersionForQualifiedImportedType)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Import import{"Qml", Storage::Version{1}, sourceId2};
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::QualifiedImportedType{"Object", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};

    storage.synchronize({}, {import}, {type}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "Item",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId2)));
}

TEST_F(ProjectStorage, FetchByMajorVersionAndMinorVersionForImportedType)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::ImportedType{"Obj"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};
    Storage::Import import{"Qml", Storage::Version{1, 2}, sourceId2};

    storage.synchronize({}, {import}, {type}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "Item",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId2)));
}

TEST_F(ProjectStorage, FetchByMajorVersionAndMinorVersionForQualifiedImportedType)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Import import{"Qml", Storage::Version{1, 2}, sourceId2};
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::QualifiedImportedType{"Obj", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};

    storage.synchronize({}, {import}, {type}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "Item",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId2)));
}

TEST_F(ProjectStorage,
       FetchByMajorVersionAndMinorVersionForImportedTypeIfMinorVersionIsNotExportedThrows)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::ImportedType{"Object"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};
    Storage::Import import{"Qml", Storage::Version{1, 1}, sourceId2};

    ASSERT_THROW(storage.synchronize({}, {import}, {type}, {sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage,
       FetchByMajorVersionAndMinorVersionForQualifiedImportedTypeIfMinorVersionIsNotExportedThrows)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Import import{"Qml", Storage::Version{1, 1}, sourceId2};
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::QualifiedImportedType{"Object", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};

    ASSERT_THROW(storage.synchronize({}, {import}, {type}, {sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, FetchLowMinorVersionForImportedTypeThrows)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::ImportedType{"Obj"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};
    Storage::Import import{"Qml", Storage::Version{1, 1}, sourceId2};

    ASSERT_THROW(storage.synchronize({}, {import}, {type}, {sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, FetchLowMinorVersionForQualifiedImportedTypeThrows)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Import import{"Qml", Storage::Version{1, 1}, sourceId2};
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::QualifiedImportedType{"Obj", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};

    ASSERT_THROW(storage.synchronize({}, {import}, {type}, {sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, FetchHigherMinorVersionForImportedType)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::ImportedType{"Obj"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};
    Storage::Import import{"Qml", Storage::Version{1, 3}, sourceId2};

    storage.synchronize({}, {import}, {type}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "Item",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId2)));
}

TEST_F(ProjectStorage, FetchHigherMinorVersionForQualifiedImportedType)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Import import{"Qml", Storage::Version{1, 3}, sourceId2};
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::QualifiedImportedType{"Obj", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};

    storage.synchronize({}, {import}, {type}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "Item",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId2)));
}

TEST_F(ProjectStorage, FetchDifferentMajorVersionForImportedTypeThrows)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::ImportedType{"Obj"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};
    Storage::Import import{"Qml", Storage::Version{3, 1}, sourceId2};

    ASSERT_THROW(storage.synchronize({}, {import}, {type}, {sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, FetchDifferentMajorVersionForQualifiedImportedTypeThrows)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Import import{"Qml", Storage::Version{3, 1}, sourceId2};
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::QualifiedImportedType{"Obj", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};

    ASSERT_THROW(storage.synchronize({}, {import}, {type}, {sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, FetchOtherTypeByDifferentVersionForImportedType)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::ImportedType{"Obj"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};
    Storage::Import import{"Qml", Storage::Version{2, 3}, sourceId2};

    storage.synchronize({}, {import}, {type}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "Item",
                                       Storage::NativeType{"QObject2"},
                                       TypeAccessSemantics::Reference,
                                       sourceId2)));
}

TEST_F(ProjectStorage, FetchOtherTypeByDifferentVersionForQualifiedImportedType)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Import import{"Qml", Storage::Version{2, 3}, sourceId2};
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::QualifiedImportedType{"Obj", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};

    storage.synchronize({}, {import}, {type}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "Item",
                                       Storage::NativeType{"QObject2"},
                                       TypeAccessSemantics::Reference,
                                       sourceId2)));
}

TEST_F(ProjectStorage, FetchHighestVersionForImportWithoutVersionForImportedType)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::ImportedType{"Obj"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};
    Storage::Import import{"Qml", Storage::Version{}, sourceId2};

    storage.synchronize({}, {import}, {type}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "Item",
                                       Storage::NativeType{"QObject4"},
                                       TypeAccessSemantics::Reference,
                                       sourceId2)));
}

TEST_F(ProjectStorage, FetchHighestVersionForImportWithoutVersionForQualifiedImportedType)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Import import{"Qml", Storage::Version{}, sourceId2};
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::QualifiedImportedType{"Obj", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};

    storage.synchronize({}, {import}, {type}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "Item",
                                       Storage::NativeType{"QObject4"},
                                       TypeAccessSemantics::Reference,
                                       sourceId2)));
}

TEST_F(ProjectStorage, FetchHighestVersionForImportWithMajorVersionForImportedType)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::ImportedType{"Obj"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};
    Storage::Import import{"Qml", Storage::Version{2}, sourceId2};

    storage.synchronize({}, {import}, {type}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "Item",
                                       Storage::NativeType{"QObject3"},
                                       TypeAccessSemantics::Reference,
                                       sourceId2)));
}

TEST_F(ProjectStorage, FetchHighestVersionForImportWithMajorVersionForQualifiedImportedType)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Import import{"Qml", Storage::Version{2}, sourceId2};
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::QualifiedImportedType{"Obj", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};

    storage.synchronize({}, {import}, {type}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "Item",
                                       Storage::NativeType{"QObject3"},
                                       TypeAccessSemantics::Reference,
                                       sourceId2)));
}

TEST_F(ProjectStorage, FetchExportedTypeWithoutVersionFirstForImportedType)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::ImportedType{"BuiltInObj"},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};
    Storage::Import import{"Qml", Storage::Version{}, sourceId2};

    storage.synchronize({}, {import}, {type}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "Item",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId2)));
}

TEST_F(ProjectStorage, FetchExportedTypeWithoutVersionFirstForQualifiedImportedType)
{
    auto types = createVersionedTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});
    Storage::Import import{"Qml", Storage::Version{}, sourceId2};
    Storage::Type type{Storage::Module{"QtQuick"},
                       "Item",
                       Storage::QualifiedImportedType{"BuiltInObj", import},
                       TypeAccessSemantics::Reference,
                       sourceId2,
                       {Storage::ExportedType{"Item", Storage::Version{}}}};

    storage.synchronize({}, {import}, {type}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Module{"QtQuick"},
                                       "Item",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId2)));
}

TEST_F(ProjectStorage, EnsureThatPropertiesForRemovedTypesAreNotAnymoreRelinked)
{
    Storage::Type type{Storage::Module{"Qml"},
                       "QObject",
                       Storage::NativeType{""},
                       TypeAccessSemantics::Reference,
                       sourceId1,
                       {Storage::ExportedType{"Object", Storage::Version{}}},
                       {Storage::PropertyDeclaration{"data",
                                                     Storage::NativeType{"QObject"},
                                                     Storage::PropertyDeclarationTraits::IsList}}};
    Storage::Import import{"Qml", Storage::Version{}, sourceId1};
    storage.synchronize(modules,
                        {import},
                        {type},
                        {sourceId1, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

    ASSERT_NO_THROW(storage.synchronize({}, {}, {}, {sourceId1}, {}));
}

TEST_F(ProjectStorage, EnsureThatPrototypesForRemovedTypesAreNotAnymoreRelinked)
{
    auto types = createTypes();
    storage.synchronize(modules,
                        imports,
                        types,
                        {sourceId1, sourceId2, moduleSourceId1, moduleSourceId2, moduleSourceId3},
                        {});

    ASSERT_NO_THROW(storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {}));
}

} // namespace
