// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <matchers/info_exportedtypenames-matcher.h>

#include <modelnode.h>
#include <projectstorage/projectstorage.h>
#include <projectstorage/sourcepathcache.h>
#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitewritestatement.h>

#include <random>

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
using QmlDesigner::Storage::TypeTraits;
using QmlDesigner::Storage::Synchronization::SynchronizationPackage;

namespace Storage = QmlDesigner::Storage;

Storage::Imports operator+(const Storage::Imports &first,
                                            const Storage::Imports &second)
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
           traits,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(
                   Storage::Synchronization::Type{typeName, prototypeId, TypeId{}, traits, sourceId}))
{
    const Storage::Synchronization::Type &type = arg;

    return type.sourceId == sourceId && type.typeName == typeName && type.traits == traits
           && compareInvalidAreTrue(prototypeId, type.prototypeId);
}

MATCHER_P5(IsStorageType,
           sourceId,
           typeName,
           prototypeId,
           extensionId,
           traits,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Synchronization::Type{
                   typeName, prototypeId, extensionId, traits, sourceId}))
{
    const Storage::Synchronization::Type &type = arg;

    return type.sourceId == sourceId && type.typeName == typeName && type.traits == traits
           && compareInvalidAreTrue(prototypeId, type.prototypeId)
           && compareInvalidAreTrue(extensionId, type.extensionId);
}

MATCHER_P(IsExportedType,
          name,
          std::string(negation ? "isn't " : "is ")
              + PrintToString(Storage::Synchronization::ExportedType{name}))
{
    const Storage::Synchronization::ExportedType &type = arg;

    return type.name == name;
}

MATCHER_P2(IsExportedType,
           moduleId,
           name,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Synchronization::ExportedType{moduleId, name}))
{
    const Storage::Synchronization::ExportedType &type = arg;

    return type.moduleId == moduleId && type.name == name;
}

MATCHER_P3(IsExportedType,
           name,
           majorVersion,
           minorVersion,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Synchronization::ExportedType{
                   name, Storage::Version{majorVersion, minorVersion}}))
{
    const Storage::Synchronization::ExportedType &type = arg;

    return type.name == name
           && type.version == Storage::Version{majorVersion, minorVersion};
}

MATCHER_P4(IsExportedType,
           moduleId,
           name,
           majorVersion,
           minorVersion,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Synchronization::ExportedType{
                   moduleId, name, Storage::Version{majorVersion, minorVersion}}))
{
    const Storage::Synchronization::ExportedType &type = arg;

    return type.moduleId == moduleId && type.name == name
           && type.version == Storage::Version{majorVersion, minorVersion};
}

MATCHER_P3(
    IsPropertyDeclaration,
    name,
    propertyTypeId,
    traits,
    std::string(negation ? "isn't " : "is ")
        + PrintToString(Storage::Synchronization::PropertyDeclaration{name, propertyTypeId, traits}))
{
    const Storage::Synchronization::PropertyDeclaration &propertyDeclaration = arg;

    return propertyDeclaration.name == name && propertyTypeId == propertyDeclaration.propertyTypeId
           && propertyDeclaration.traits == traits;
}

MATCHER_P4(IsPropertyDeclaration,
           name,
           propertyTypeId,
           traits,
           aliasPropertyName,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Synchronization::PropertyDeclaration{
                   name, propertyTypeId, traits, aliasPropertyName}))
{
    const Storage::Synchronization::PropertyDeclaration &propertyDeclaration = arg;

    return propertyDeclaration.name == name && propertyTypeId == propertyDeclaration.propertyTypeId
           && propertyDeclaration.aliasPropertyName == aliasPropertyName
           && propertyDeclaration.traits == traits;
}

MATCHER_P4(IsInfoPropertyDeclaration,
           typeId,
           name,
           traits,
           propertyTypeId,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Info::PropertyDeclaration{typeId, name, traits, propertyTypeId}))
{
    const Storage::Info::PropertyDeclaration &propertyDeclaration = arg;

    return propertyDeclaration.typeId == typeId && propertyDeclaration.name == name
           && propertyDeclaration.propertyTypeId == propertyTypeId
           && propertyDeclaration.traits == traits;
}

class HasNameMatcher
{
public:
    using is_gtest_matcher = void;

    HasNameMatcher(const QmlDesigner::ProjectStorage<Sqlite::Database> &storage,
                   Utils::SmallStringView name)
        : storage{storage}
        , name{name}
    {}

    bool MatchAndExplain(QmlDesigner::PropertyDeclarationId id, std::ostream *listener) const
    {
        auto propertyName = storage.propertyName(id);
        bool success = propertyName && *propertyName == name;

        if (success)
            return true;

        if (listener) {
            if (propertyName)
                *listener << "name is '" << *propertyName << "', not '" << name << "'";
            else
                *listener << "there is no '" << name << "'";
        }

        return false;
    }

    void DescribeTo(std::ostream *os) const { *os << "is '" << name << "'"; }

    void DescribeNegationTo(std::ostream *os) const { *os << "is not '" << name << "'"; }

private:
    const QmlDesigner::ProjectStorage<Sqlite::Database> &storage;
    Utils::SmallStringView name;
};

#define HasName(name) Matcher<QmlDesigner::PropertyDeclarationId>(HasNameMatcher{storage, name})

MATCHER(IsSorted, std::string(negation ? "isn't sorted" : "is sorted"))
{
    using std::begin;
    using std::end;
    return std::is_sorted(begin(arg), end(arg));
}

MATCHER(StringsAreSorted, std::string(negation ? "isn't sorted" : "is sorted"))
{
    using std::begin;
    using std::end;
    return std::is_sorted(begin(arg), end(arg), [](const auto &first, const auto &second) {
        return Sqlite::compare(first, second) < 0;
    });
}

MATCHER_P3(IsInfoType,
           defaultPropertyId,
           sourceId,
           traits,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Info::Type{defaultPropertyId, sourceId, traits}))
{
    const Storage::Info::Type &type = arg;

    return type.defaultPropertyId == defaultPropertyId && type.sourceId == sourceId
           && type.traits == traits;
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
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1);
        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId2);
        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId1);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId,
                                                Storage::Version{},
                                                sourceId1);
        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId2);
        package.updatedModuleDependencySourceIds.push_back(sourceId1);
        package.updatedModuleDependencySourceIds.push_back(sourceId2);

        importsSourceId1.emplace_back(qmlModuleId, Storage::Version{}, sourceId1);
        importsSourceId1.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1);
        moduleDependenciesSourceId1.emplace_back(qmlNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId1);
        moduleDependenciesSourceId1.emplace_back(qtQuickNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId1);

        importsSourceId2.emplace_back(qmlModuleId, Storage::Version{}, sourceId2);
        moduleDependenciesSourceId2.emplace_back(qmlNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId2);

        package.types.push_back(Storage::Synchronization::Type{
            "QQuickItem",
            Storage::Synchronization::ImportedType{"QObject"},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item"},
             Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QQuickItem"}},
            {Storage::Synchronization::PropertyDeclaration{"data",
                                                           Storage::Synchronization::ImportedType{
                                                               "QObject"},
                                                           Storage::PropertyDeclarationTraits::IsList},
             Storage::Synchronization::PropertyDeclaration{
                 "children",
                 Storage::Synchronization::ImportedType{"Item"},
                 Storage::PropertyDeclarationTraits::IsList
                     | Storage::PropertyDeclarationTraits::IsReadOnly}},
            {Storage::Synchronization::FunctionDeclaration{
                 "execute", "", {Storage::Synchronization::ParameterDeclaration{"arg", ""}}},
             Storage::Synchronization::FunctionDeclaration{
                 "values",
                 "Vector3D",
                 {Storage::Synchronization::ParameterDeclaration{"arg1", "int"},
                  Storage::Synchronization::ParameterDeclaration{
                      "arg2", "QObject", Storage::PropertyDeclarationTraits::IsPointer},
                  Storage::Synchronization::ParameterDeclaration{"arg3", "string"}}}},
            {Storage::Synchronization::SignalDeclaration{
                 "execute", {Storage::Synchronization::ParameterDeclaration{"arg", ""}}},
             Storage::Synchronization::SignalDeclaration{
                 "values",
                 {Storage::Synchronization::ParameterDeclaration{"arg1", "int"},
                  Storage::Synchronization::ParameterDeclaration{
                      "arg2", "QObject", Storage::PropertyDeclarationTraits::IsPointer},
                  Storage::Synchronization::ParameterDeclaration{"arg3", "string"}}}},
            {Storage::Synchronization::EnumerationDeclaration{
                 "Enum",
                 {Storage::Synchronization::EnumeratorDeclaration{"Foo"},
                  Storage::Synchronization::EnumeratorDeclaration{"Bar", 32}}},
             Storage::Synchronization::EnumerationDeclaration{
                 "Type",
                 {Storage::Synchronization::EnumeratorDeclaration{"Foo"},
                  Storage::Synchronization::EnumeratorDeclaration{"Poo", 12}}}},
            Storage::Synchronization::ChangeLevel::Full,
            "data"});
        package.types.push_back(Storage::Synchronization::Type{
            "QObject",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId2,
            {Storage::Synchronization::ExportedType{qmlModuleId,
                                                    "Object",
                                                    Storage::Version{2}},
             Storage::Synchronization::ExportedType{qmlModuleId,
                                                    "Obj",
                                                    Storage::Version{2}},
             Storage::Synchronization::ExportedType{qmlNativeModuleId, "QObject"}}});

        package.updatedSourceIds = {sourceId1, sourceId2};

        return package;
    }

    auto createBuiltinSynchronizationPackage()
    {
        SynchronizationPackage package;

        package.imports.emplace_back(QMLModuleId, Storage::Version{}, sourceId1);
        package.moduleDependencies.emplace_back(QMLModuleId,
                                                Storage::Version{},
                                                sourceId1);
        package.updatedModuleDependencySourceIds.push_back(sourceId1);

        importsSourceId1.emplace_back(QMLModuleId, Storage::Version{}, sourceId1);
        moduleDependenciesSourceId1.emplace_back(QMLModuleId, Storage::Version{}, sourceId1);

        package.types.push_back(
            Storage::Synchronization::Type{"bool",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraits::Value,
                                           sourceId1,
                                           {Storage::Synchronization::ExportedType{QMLModuleId,
                                                                                   "bool"}}});
        package.types.push_back(
            Storage::Synchronization::Type{"int",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraits::Value,
                                           sourceId1,
                                           {Storage::Synchronization::ExportedType{QMLModuleId,
                                                                                   "int"}}});
        package.types.push_back(
            Storage::Synchronization::Type{"uint",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraits::Value,
                                           sourceId1,
                                           {Storage::Synchronization::ExportedType{QMLNativeModuleId,
                                                                                   "uint"}}});
        package.types.push_back(
            Storage::Synchronization::Type{"double",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraits::Value,
                                           sourceId1,
                                           {Storage::Synchronization::ExportedType{QMLModuleId,
                                                                                   "double"}}});
        package.types.push_back(
            Storage::Synchronization::Type{"float",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraits::Value,
                                           sourceId1,
                                           {Storage::Synchronization::ExportedType{QMLNativeModuleId,
                                                                                   "float"}}});
        package.types.push_back(
            Storage::Synchronization::Type{"date",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraits::Value,
                                           sourceId1,
                                           {Storage::Synchronization::ExportedType{QMLModuleId,
                                                                                   "date"}}});
        package.types.push_back(
            Storage::Synchronization::Type{"string",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraits::Value,
                                           sourceId1,
                                           {Storage::Synchronization::ExportedType{QMLModuleId,
                                                                                   "string"}}});
        package.types.push_back(
            Storage::Synchronization::Type{"url",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraits::Value,
                                           sourceId1,
                                           {Storage::Synchronization::ExportedType{QMLModuleId,
                                                                                   "url"}}});
        package.types.push_back(
            Storage::Synchronization::Type{"var",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraits::Value,
                                           sourceId1,
                                           {Storage::Synchronization::ExportedType{QMLModuleId,
                                                                                   "var"}}});

        package.updatedSourceIds = {sourceId1};

        return package;
    }

    auto createSynchronizationPackageWithAliases()
    {
        auto package{createSimpleSynchronizationPackage()};

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId3);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId3);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId,
                                                Storage::Version{},
                                                sourceId3);
        package.updatedModuleDependencySourceIds.push_back(sourceId3);

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId4);
        package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId4);
        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId4);
        package.updatedModuleDependencySourceIds.push_back(sourceId4);

        importsSourceId3.emplace_back(qmlModuleId, Storage::Version{}, sourceId3);
        importsSourceId3.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
        moduleDependenciesSourceId3.emplace_back(qmlNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId3);
        moduleDependenciesSourceId3.emplace_back(qtQuickNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId3);

        importsSourceId4.emplace_back(qmlModuleId, Storage::Version{}, sourceId4);
        importsSourceId4.emplace_back(pathToModuleId, Storage::Version{}, sourceId4);
        moduleDependenciesSourceId4.emplace_back(qmlNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId4);

        package.types[1].propertyDeclarations.push_back(
            Storage::Synchronization::PropertyDeclaration{"objects",
                                                          Storage::Synchronization::ImportedType{
                                                              "QObject"},
                                                          Storage::PropertyDeclarationTraits::IsList});
        package.types.push_back(Storage::Synchronization::Type{
            "QAliasItem",
            Storage::Synchronization::ImportedType{"Item"},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId3,
            {Storage::Synchronization::ExportedType{qtQuickModuleId, "AliasItem"},
             Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QAliasItem"}}});
        package.types.back().propertyDeclarations.push_back(
            Storage::Synchronization::PropertyDeclaration{"data",
                                                          Storage::Synchronization::ImportedType{
                                                              "QObject"},
                                                          Storage::PropertyDeclarationTraits::IsList});
        package.types.back().propertyDeclarations.push_back(
            Storage::Synchronization::PropertyDeclaration{
                "items", Storage::Synchronization::ImportedType{"Item"}, "children"});
        package.types.back().propertyDeclarations.push_back(
            Storage::Synchronization::PropertyDeclaration{
                "objects", Storage::Synchronization::ImportedType{"Item"}, "objects"});

        package.types.push_back(Storage::Synchronization::Type{
            "QObject2",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId4,
            {Storage::Synchronization::ExportedType{pathToModuleId, "Object2"},
             Storage::Synchronization::ExportedType{pathToModuleId, "Obj2"}}});
        package.types[3].propertyDeclarations.push_back(
            Storage::Synchronization::PropertyDeclaration{"objects",
                                                          Storage::Synchronization::ImportedType{
                                                              "QObject"},
                                                          Storage::PropertyDeclarationTraits::IsList});

        package.updatedSourceIds.push_back(sourceId3);
        package.updatedSourceIds.push_back(sourceId4);

        return package;
    }

    auto createSynchronizationPackageWithIndirectAliases()
    {
        SynchronizationPackage package;

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1);
        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId2);

        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId1);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId,
                                                Storage::Version{},
                                                sourceId1);
        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId2);

        package.updatedModuleDependencySourceIds.push_back(sourceId1);
        package.updatedModuleDependencySourceIds.push_back(sourceId2);

        importsSourceId1.emplace_back(qmlModuleId, Storage::Version{}, sourceId1);
        importsSourceId1.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1);
        moduleDependenciesSourceId1.emplace_back(qmlNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId1);
        moduleDependenciesSourceId1.emplace_back(qtQuickNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId1);

        importsSourceId2.emplace_back(qmlModuleId, Storage::Version{}, sourceId2);
        moduleDependenciesSourceId2.emplace_back(qmlNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId2);

        package.types.push_back(Storage::Synchronization::Type{
            "QQuickItem",
            Storage::Synchronization::ImportedType{"QObject"},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item"},
             Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QQuickItem"}},
            {Storage::Synchronization::PropertyDeclaration{
                 "children",
                 Storage::Synchronization::ImportedType{"QChildren"},
                 Storage::PropertyDeclarationTraits::IsReadOnly},
             Storage::Synchronization::PropertyDeclaration{
                 "kids",
                 Storage::Synchronization::ImportedType{"QChildren2"},
                 Storage::PropertyDeclarationTraits::IsReadOnly}}});

        package.types.push_back(Storage::Synchronization::Type{
            "QObject",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId2,
            {Storage::Synchronization::ExportedType{qmlModuleId,
                                                    "Object",
                                                    Storage::Version{2}},
             Storage::Synchronization::ExportedType{qmlModuleId,
                                                    "Obj",
                                                    Storage::Version{2}},
             Storage::Synchronization::ExportedType{qmlNativeModuleId, "QObject"}}});

        package.updatedSourceIds = {sourceId1, sourceId2};

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId3);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId3);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId,
                                                Storage::Version{},
                                                sourceId3);
        package.updatedModuleDependencySourceIds.push_back(sourceId3);

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId4);
        package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId4);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId4);
        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId4);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId,
                                                Storage::Version{},
                                                sourceId4);
        package.updatedModuleDependencySourceIds.push_back(sourceId4);

        importsSourceId3.emplace_back(qmlModuleId, Storage::Version{}, sourceId3);
        importsSourceId3.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
        moduleDependenciesSourceId3.emplace_back(qmlNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId3);
        moduleDependenciesSourceId3.emplace_back(qtQuickNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId3);

        importsSourceId4.emplace_back(qmlModuleId, Storage::Version{}, sourceId4);
        importsSourceId4.emplace_back(pathToModuleId, Storage::Version{}, sourceId4);
        importsSourceId4.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId4);
        moduleDependenciesSourceId4.emplace_back(qmlNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId4);
        moduleDependenciesSourceId4.emplace_back(qtQuickNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId4);

        package.types[1].propertyDeclarations.push_back(
            Storage::Synchronization::PropertyDeclaration{"objects",
                                                          Storage::Synchronization::ImportedType{
                                                              "QObject"},
                                                          Storage::PropertyDeclarationTraits::IsList});
        package.types.push_back(Storage::Synchronization::Type{
            "QAliasItem",
            Storage::Synchronization::ImportedType{"Item"},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId3,
            {Storage::Synchronization::ExportedType{qtQuickModuleId, "AliasItem"},
             Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QAliasItem"}}});
        package.types.back().propertyDeclarations.push_back(
            Storage::Synchronization::PropertyDeclaration{
                "items", Storage::Synchronization::ImportedType{"Item"}, "children", "items"});
        package.types.back().propertyDeclarations.push_back(
            Storage::Synchronization::PropertyDeclaration{
                "objects", Storage::Synchronization::ImportedType{"Item"}, "children", "objects"});

        package.types.push_back(Storage::Synchronization::Type{
            "QObject2",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId4,
            {Storage::Synchronization::ExportedType{pathToModuleId, "Object2"},
             Storage::Synchronization::ExportedType{pathToModuleId, "Obj2"}}});
        package.types[3].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
            "children",
            Storage::Synchronization::ImportedType{"QChildren2"},
            Storage::PropertyDeclarationTraits::IsReadOnly});

        package.updatedSourceIds.push_back(sourceId3);
        package.updatedSourceIds.push_back(sourceId4);

        package.types.push_back(Storage::Synchronization::Type{
            "QChildren",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId5,
            {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                    "Children",
                                                    Storage::Version{2}},
             Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QChildren"}},
            {Storage::Synchronization::PropertyDeclaration{"items",
                                                           Storage::Synchronization::ImportedType{
                                                               "QQuickItem"},
                                                           Storage::PropertyDeclarationTraits::IsList},
             Storage::Synchronization::PropertyDeclaration{
                 "objects",
                 Storage::Synchronization::ImportedType{"QObject"},
                 Storage::PropertyDeclarationTraits::IsList
                     | Storage::PropertyDeclarationTraits::IsReadOnly}}});

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId5);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId5);
        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId5);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId,
                                                Storage::Version{},
                                                sourceId5);
        importsSourceId5.emplace_back(qmlModuleId, Storage::Version{}, sourceId5);
        importsSourceId5.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId5);
        moduleDependenciesSourceId5.emplace_back(qmlNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId5);
        moduleDependenciesSourceId5.emplace_back(qtQuickNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId5);
        package.updatedModuleDependencySourceIds.push_back(sourceId5);
        package.updatedSourceIds.push_back(sourceId5);

        package.types.push_back(Storage::Synchronization::Type{
            "QChildren2",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId6,
            {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                    "Children2",
                                                    Storage::Version{2}},
             Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QChildren2"}},
            {Storage::Synchronization::PropertyDeclaration{"items",
                                                           Storage::Synchronization::ImportedType{
                                                               "QQuickItem"},
                                                           Storage::PropertyDeclarationTraits::IsList},
             Storage::Synchronization::PropertyDeclaration{
                 "objects",
                 Storage::Synchronization::ImportedType{"Object2"},
                 Storage::PropertyDeclarationTraits::IsList
                     | Storage::PropertyDeclarationTraits::IsReadOnly}}});

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId6);
        package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId6);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId6);
        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId6);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId,
                                                Storage::Version{},
                                                sourceId6);
        package.updatedModuleDependencySourceIds.push_back(sourceId6);
        package.updatedSourceIds.push_back(sourceId6);

        return package;
    }

    auto createSynchronizationPackageWithAliases2()
    {
        auto package{createSynchronizationPackageWithAliases()};

        package.types[2].prototype = Storage::Synchronization::ImportedType{"Object"};
        package.types[2].propertyDeclarations.erase(
            std::next(package.types[2].propertyDeclarations.begin()));

        return package;
    }

    auto createSynchronizationPackageWithRecursiveAliases()
    {
        auto package{createSynchronizationPackageWithAliases()};

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId5);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId5);

        package.types.push_back(Storage::Synchronization::Type{
            "QAliasItem2",
            Storage::Synchronization::ImportedType{"Object"},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId5,
            {Storage::Synchronization::ExportedType{qtQuickModuleId, "AliasItem2"},
             Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QAliasItem2"}}});

        package.types.back().propertyDeclarations.push_back(
            Storage::Synchronization::PropertyDeclaration{
                "objects", Storage::Synchronization::ImportedType{"AliasItem"}, "objects"});

        package.updatedSourceIds.push_back(sourceId5);

        return package;
    }

    template<typename Container>
    static void shuffle(Container &container)
    {
        std::random_device randomDevice;
        std::mt19937 generator(randomDevice());

        std::shuffle(container.begin(), container.end(), generator);
    }

    auto createSynchronizationPackageWithVersions()
    {
        SynchronizationPackage package;

        package.types.push_back(Storage::Synchronization::Type{
            "QObject",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qmlModuleId,
                                                    "Object",
                                                    Storage::Version{1}},
             Storage::Synchronization::ExportedType{qmlModuleId,
                                                    "Obj",
                                                    Storage::Version{1, 2}},
             Storage::Synchronization::ExportedType{qmlModuleId, "BuiltInObj"},
             Storage::Synchronization::ExportedType{qmlNativeModuleId, "QObject"}}});
        package.types.push_back(Storage::Synchronization::Type{
            "QObject2",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qmlModuleId,
                                                    "Object",
                                                    Storage::Version{2, 0}},
             Storage::Synchronization::ExportedType{qmlModuleId,
                                                    "Obj",
                                                    Storage::Version{2, 3}},
             Storage::Synchronization::ExportedType{qmlNativeModuleId, "QObject2"}}});
        package.types.push_back(Storage::Synchronization::Type{
            "QObject3",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qmlModuleId,
                                                    "Object",
                                                    Storage::Version{2, 11}},
             Storage::Synchronization::ExportedType{qmlModuleId,
                                                    "Obj",
                                                    Storage::Version{2, 11}},
             Storage::Synchronization::ExportedType{qmlNativeModuleId, "QObject3"}}});
        package.types.push_back(Storage::Synchronization::Type{
            "QObject4",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qmlModuleId,
                                                    "Object",
                                                    Storage::Version{3, 4}},
             Storage::Synchronization::ExportedType{qmlModuleId,
                                                    "Obj",
                                                    Storage::Version{3, 4}},
             Storage::Synchronization::ExportedType{qmlModuleId,
                                                    "BuiltInObj",
                                                    Storage::Version{3, 4}},
             Storage::Synchronization::ExportedType{qmlNativeModuleId, "QObject4"}}});

        package.updatedSourceIds.push_back(sourceId1);

        shuffle(package.types);

        return package;
    }

    auto createPackageWithProperties()
    {
        SynchronizationPackage package;

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1);

        package.types.push_back(Storage::Synchronization::Type{
            "QObject",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qmlModuleId,
                                                    "Object",
                                                    Storage::Version{}}},
            {Storage::Synchronization::PropertyDeclaration{"data",
                                                           Storage::Synchronization::ImportedType{"Object"},
                                                           Storage::PropertyDeclarationTraits::IsList},
             Storage::Synchronization::PropertyDeclaration{
                 "children",
                 Storage::Synchronization::ImportedType{"Object"},
                 Storage::PropertyDeclarationTraits::IsList
                     | Storage::PropertyDeclarationTraits::IsReadOnly}},
            {Storage::Synchronization::FunctionDeclaration{"values", {}, {}}},
            {Storage::Synchronization::SignalDeclaration{"valuesChanged", {}}}});
        package.types.push_back(Storage::Synchronization::Type{
            "QObject2",
            Storage::Synchronization::ImportedType{"Object"},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qmlModuleId,
                                                    "Object2",
                                                    Storage::Version{}}},
            {Storage::Synchronization::PropertyDeclaration{
                 "data2",
                 Storage::Synchronization::ImportedType{"Object3"},
                 Storage::PropertyDeclarationTraits::IsReadOnly},
             Storage::Synchronization::PropertyDeclaration{
                 "children2",
                 Storage::Synchronization::ImportedType{"Object3"},
                 Storage::PropertyDeclarationTraits::IsList
                     | Storage::PropertyDeclarationTraits::IsReadOnly}},
            {Storage::Synchronization::FunctionDeclaration{"items", {}, {}}},
            {Storage::Synchronization::SignalDeclaration{"itemsChanged", {}}}});
        package.types.push_back(Storage::Synchronization::Type{
            "QObject3",
            Storage::Synchronization::ImportedType{"Object2"},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qmlModuleId,
                                                    "Object3",
                                                    Storage::Version{}}},
            {Storage::Synchronization::PropertyDeclaration{"data3",
                                                           Storage::Synchronization::ImportedType{
                                                               "Object2"},
                                                           Storage::PropertyDeclarationTraits::IsList},
             Storage::Synchronization::PropertyDeclaration{
                 "children3",
                 Storage::Synchronization::ImportedType{"Object2"},
                 Storage::PropertyDeclarationTraits::IsList
                     | Storage::PropertyDeclarationTraits::IsReadOnly}},
            {Storage::Synchronization::FunctionDeclaration{"objects", {}, {}}},
            {Storage::Synchronization::SignalDeclaration{"objectsChanged", {}}}});

        package.updatedSourceIds.push_back(sourceId1);

        shuffle(package.types);

        return package;
    }

    auto createModuleExportedImportSynchronizationPackage()
    {
        SynchronizationPackage package;

        package.imports.emplace_back(qmlModuleId, Storage::Version{1}, sourceId1);
        package.updatedModuleIds.push_back(qtQuickModuleId);
        package.types.push_back(Storage::Synchronization::Type{
            "QQuickItem",
            Storage::Synchronization::ImportedType{"Object"},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                    "Item",
                                                    Storage::Version{1, 0}}}});

        package.updatedModuleIds.push_back(qmlModuleId);
        package.moduleExportedImports.emplace_back(qtQuickModuleId,
                                                   qmlModuleId,
                                                   Storage::Version{},
                                                   Storage::Synchronization::IsAutoVersion::Yes);
        package.types.push_back(Storage::Synchronization::Type{
            "QObject",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId2,
            {Storage::Synchronization::ExportedType{qmlModuleId,
                                                    "Object",
                                                    Storage::Version{1, 0}}}});

        package.imports.emplace_back(qtQuickModuleId, Storage::Version{1}, sourceId3);
        package.moduleExportedImports.emplace_back(qtQuick3DModuleId,
                                                   qtQuickModuleId,
                                                   Storage::Version{},
                                                   Storage::Synchronization::IsAutoVersion::Yes);
        package.updatedModuleIds.push_back(qtQuick3DModuleId);
        package.types.push_back(Storage::Synchronization::Type{
            "QQuickItem3d",
            Storage::Synchronization::ImportedType{"Item"},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId3,
            {Storage::Synchronization::ExportedType{qtQuick3DModuleId,
                                                    "Item3D",
                                                    Storage::Version{1, 0}}}});

        package.imports.emplace_back(qtQuick3DModuleId, Storage::Version{1}, sourceId4);
        package.types.push_back(Storage::Synchronization::Type{
            "MyItem",
            Storage::Synchronization::ImportedType{"Object"},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId4,
            {Storage::Synchronization::ExportedType{myModuleModuleId,
                                                    "MyItem",
                                                    Storage::Version{1, 0}}}});

        package.updatedSourceIds = {sourceId1, sourceId2, sourceId3, sourceId4};

        return package;
    }

    auto createPropertyEditorPathsSynchronizationPackage()
    {
        SynchronizationPackage package;

        package.updatedModuleIds.push_back(qtQuickModuleId);
        package.types.push_back(Storage::Synchronization::Type{
            "QQuickItem",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraits::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item", Storage::Version{1, 0}}}});
        package.types.push_back(
            Storage::Synchronization::Type{"QObject",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraits::Reference,
                                           sourceId2,
                                           {Storage::Synchronization::ExportedType{
                                               qtQuickModuleId, "QtObject", Storage::Version{1, 0}}}});
        package.updatedModuleIds.push_back(qtQuick3DModuleId);
        package.types.push_back(
            Storage::Synchronization::Type{"QQuickItem3d",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraits::Reference,
                                           sourceId3,
                                           {Storage::Synchronization::ExportedType{
                                               qtQuickModuleId, "Item3D", Storage::Version{1, 0}}}});

        package.imports.emplace_back(qtQuick3DModuleId, Storage::Version{1}, sourceId4);

        package.updatedSourceIds = {sourceId1, sourceId2, sourceId3};

        package.propertyEditorQmlPaths.emplace_back(qtQuickModuleId, "QtObject", sourceId1, sourceIdPath);
        package.propertyEditorQmlPaths.emplace_back(qtQuickModuleId, "Item", sourceId2, sourceIdPath);
        package.propertyEditorQmlPaths.emplace_back(qtQuickModuleId, "Item3D", sourceId3, sourceIdPath);
        package.updatedPropertyEditorQmlPathSourceIds.emplace_back(sourceIdPath);

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

    static auto &findType(Storage::Synchronization::SynchronizationPackage &package,
                          Utils::SmallStringView name)
    {
        auto &types = package.types;

        return *std::find_if(types.begin(), types.end(), [=](const auto &type) {
            return type.typeName == name;
        });
    }

    static auto &findProperty(Storage::Synchronization::SynchronizationPackage &package,
                              Utils::SmallStringView typeName,
                              Utils::SmallStringView propertyName)
    {
        auto &type = findType(package, typeName);

        auto &properties = type.propertyDeclarations;

        return *std::find_if(properties.begin(), properties.end(), [=](const auto &property) {
            return property.name == propertyName;
        });
    }

    static void removeProperty(Storage::Synchronization::SynchronizationPackage &package,
                               Utils::SmallStringView typeName,
                               Utils::SmallStringView propertyName)
    {
        auto &type = findType(package, typeName);

        auto &properties = type.propertyDeclarations;

        properties.erase(std::remove_if(properties.begin(),
                                        properties.end(),
                                        [=](const auto &property) {
                                            return property.name == propertyName;
                                        }),
                         properties.end());
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    //Sqlite::Database database{"/tmp/aaaaa.db", Sqlite::JournalMode::Wal};
    QmlDesigner::ProjectStorage<Sqlite::Database> storage{database, database.isInitialized()};
    QmlDesigner::SourcePathCache<QmlDesigner::ProjectStorage<Sqlite::Database>> sourcePathCache{
        storage};
    QmlDesigner::SourcePathView path1{"/path1/to"};
    QmlDesigner::SourcePathView path2{"/path2/to"};
    QmlDesigner::SourcePathView path3{"/path3/to"};
    QmlDesigner::SourcePathView path4{"/path4/to"};
    QmlDesigner::SourcePathView path5{"/path5/to"};
    QmlDesigner::SourcePathView path6{"/path6/to"};
    QmlDesigner::SourcePathView pathPath{"/path6/."};
    SourceId sourceId1{sourcePathCache.sourceId(path1)};
    SourceId sourceId2{sourcePathCache.sourceId(path2)};
    SourceId sourceId3{sourcePathCache.sourceId(path3)};
    SourceId sourceId4{sourcePathCache.sourceId(path4)};
    SourceId sourceId5{sourcePathCache.sourceId(path5)};
    SourceId sourceId6{sourcePathCache.sourceId(path6)};
    SourceId sourceIdPath{sourcePathCache.sourceId(path6)};
    SourceId qmlProjectSourceId{sourcePathCache.sourceId("/path1/qmldir")};
    SourceId qtQuickProjectSourceId{sourcePathCache.sourceId("/path2/qmldir")};
    ModuleId qmlModuleId{storage.moduleId("Qml")};
    ModuleId qmlNativeModuleId{storage.moduleId("Qml-cppnative")};
    ModuleId qtQuickModuleId{storage.moduleId("QtQuick")};
    ModuleId qtQuickNativeModuleId{storage.moduleId("QtQuick-cppnative")};
    ModuleId pathToModuleId{storage.moduleId("/path/to")};
    ModuleId qtQuick3DModuleId{storage.moduleId("QtQuick3D")};
    ModuleId myModuleModuleId{storage.moduleId("MyModule")};
    ModuleId QMLModuleId{storage.moduleId("QML")};
    ModuleId QMLNativeModuleId{storage.moduleId("QML-cppnative")};
    Storage::Imports importsSourceId1;
    Storage::Imports importsSourceId2;
    Storage::Imports importsSourceId3;
    Storage::Imports importsSourceId4;
    Storage::Imports importsSourceId5;
    Storage::Imports moduleDependenciesSourceId1;
    Storage::Imports moduleDependenciesSourceId2;
    Storage::Imports moduleDependenciesSourceId3;
    Storage::Imports moduleDependenciesSourceId4;
    Storage::Imports moduleDependenciesSourceId5;
};

TEST_F(ProjectStorage, fetch_source_context_id_returns_always_the_same_id_for_the_same_path)
{
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    auto newSourceContextId = storage.fetchSourceContextId("/path/to");

    ASSERT_THAT(newSourceContextId, Eq(sourceContextId));
}

TEST_F(ProjectStorage, fetch_source_context_id_returns_not_the_same_id_for_different_path)
{
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    auto newSourceContextId = storage.fetchSourceContextId("/path/to2");

    ASSERT_THAT(newSourceContextId, Ne(sourceContextId));
}

TEST_F(ProjectStorage, fetch_source_context_path)
{
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    auto path = storage.fetchSourceContextPath(sourceContextId);

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(ProjectStorage, fetch_unknown_source_context_path_throws)
{
    ASSERT_THROW(storage.fetchSourceContextPath(SourceContextId::create(323)),
                 QmlDesigner::SourceContextIdDoesNotExists);
}

TEST_F(ProjectStorage, fetch_all_source_contexts_are_empty_if_no_source_contexts_exists)
{
    storage.clearSources();

    auto sourceContexts = storage.fetchAllSourceContexts();

    ASSERT_THAT(toValues(sourceContexts), IsEmpty());
}

TEST_F(ProjectStorage, fetch_all_source_contexts)
{
    storage.clearSources();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceContextId2 = storage.fetchSourceContextId("/path/to2");

    auto sourceContexts = storage.fetchAllSourceContexts();

    ASSERT_THAT(toValues(sourceContexts),
                UnorderedElementsAre(IsSourceContext(sourceContextId, "/path/to"),
                                     IsSourceContext(sourceContextId2, "/path/to2")));
}

TEST_F(ProjectStorage, fetch_source_id_first_time)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    auto sourceId = storage.fetchSourceId(sourceContextId, "foo");

    ASSERT_TRUE(sourceId.isValid());
}

TEST_F(ProjectStorage, fetch_existing_source_id)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto createdSourceId = storage.fetchSourceId(sourceContextId, "foo");

    auto sourceId = storage.fetchSourceId(sourceContextId, "foo");

    ASSERT_THAT(sourceId, createdSourceId);
}

TEST_F(ProjectStorage, fetch_source_id_with_different_context_id_are_not_equal)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceContextId2 = storage.fetchSourceContextId("/path/to2");
    auto sourceId2 = storage.fetchSourceId(sourceContextId2, "foo");

    auto sourceId = storage.fetchSourceId(sourceContextId, "foo");

    ASSERT_THAT(sourceId, Ne(sourceId2));
}

TEST_F(ProjectStorage, fetch_source_id_with_different_name_are_not_equal)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceId2 = storage.fetchSourceId(sourceContextId, "foo");

    auto sourceId = storage.fetchSourceId(sourceContextId, "foo2");

    ASSERT_THAT(sourceId, Ne(sourceId2));
}

TEST_F(ProjectStorage, fetch_source_id_with_non_existing_source_context_id_throws)
{
    ASSERT_THROW(storage.fetchSourceId(SourceContextId::create(42), "foo"),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorage, fetch_source_name_and_source_context_id_for_non_existing_source_id)
{
    ASSERT_THROW(storage.fetchSourceNameAndSourceContextId(SourceId::create(212)),
                 QmlDesigner::SourceIdDoesNotExists);
}

TEST_F(ProjectStorage, fetch_source_name_and_source_context_id_for_non_existing_entry)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceId = storage.fetchSourceId(sourceContextId, "foo");

    auto sourceNameAndSourceContextId = storage.fetchSourceNameAndSourceContextId(sourceId);

    ASSERT_THAT(sourceNameAndSourceContextId, IsSourceNameAndSourceContextId("foo", sourceContextId));
}

TEST_F(ProjectStorage, fetch_source_context_id_for_non_existing_source_id)
{
    ASSERT_THROW(storage.fetchSourceContextId(SourceId::create(212)),
                 QmlDesigner::SourceIdDoesNotExists);
}

TEST_F(ProjectStorage, fetch_source_context_id_for_existing_source_id)
{
    addSomeDummyData();
    auto originalSourceContextId = storage.fetchSourceContextId("/path/to3");
    auto sourceId = storage.fetchSourceId(originalSourceContextId, "foo");

    auto sourceContextId = storage.fetchSourceContextId(sourceId);

    ASSERT_THAT(sourceContextId, Eq(originalSourceContextId));
}

TEST_F(ProjectStorage, fetch_all_sources)
{
    storage.clearSources();

    auto sources = storage.fetchAllSources();

    ASSERT_THAT(toValues(sources), IsEmpty());
}

TEST_F(ProjectStorage, fetch_source_id_unguarded_first_time)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    std::lock_guard lock{database};

    auto sourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    ASSERT_TRUE(sourceId.isValid());
}

TEST_F(ProjectStorage, fetch_existing_source_id_unguarded)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    std::lock_guard lock{database};
    auto createdSourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    auto sourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    ASSERT_THAT(sourceId, createdSourceId);
}

TEST_F(ProjectStorage, fetch_source_id_unguarded_with_different_context_id_are_not_equal)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceContextId2 = storage.fetchSourceContextId("/path/to2");
    std::lock_guard lock{database};
    auto sourceId2 = storage.fetchSourceIdUnguarded(sourceContextId2, "foo");

    auto sourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    ASSERT_THAT(sourceId, Ne(sourceId2));
}

TEST_F(ProjectStorage, fetch_source_id_unguarded_with_different_name_are_not_equal)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    std::lock_guard lock{database};
    auto sourceId2 = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    auto sourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo2");

    ASSERT_THAT(sourceId, Ne(sourceId2));
}

TEST_F(ProjectStorage, fetch_source_id_unguarded_with_non_existing_source_context_id_throws)
{
    std::lock_guard lock{database};

    ASSERT_THROW(storage.fetchSourceIdUnguarded(SourceContextId::create(42), "foo"),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorage, synchronize_types_adds_new_types)
{
    auto package{createSimpleSynchronizationPackage()};

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                                     IsExportedType(qtQuickNativeModuleId,
                                                                    "QQuickItem"))))));
}

TEST_F(ProjectStorage, synchronize_types_adds_new_types_with_extension_chain)
{
    auto package{createSimpleSynchronizationPackage()};
    swap(package.types.front().extension, package.types.front().prototype);

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        TypeId{},
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                                     IsExportedType(qtQuickNativeModuleId,
                                                                    "QQuickItem"))))));
}

TEST_F(ProjectStorage, synchronize_types_adds_new_types_with_exported_prototype_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Object"};

    storage.synchronize(std::move(package));

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeId{}, TypeTraits::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                fetchTypeId(sourceId2, "QObject"),
                                TypeId{},
                                TypeTraits::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickItem"))))));
}

TEST_F(ProjectStorage, synchronize_types_adds_new_types_with_exported_extension_name)
{
    auto package{createSimpleSynchronizationPackage()};
    swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].extension = Storage::Synchronization::ImportedType{"Object"};

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        TypeId{},
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                                     IsExportedType(qtQuickNativeModuleId,
                                                                    "QQuickItem"))))));
}

TEST_F(ProjectStorage, synchronize_types_adds_new_types_throws_with_wrong_prototype_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Objec"};

    ASSERT_THROW(storage.synchronize(std::move(package)), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, synchronize_types_adds_new_types_throws_with_wrong_extension_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].extension = Storage::Synchronization::ImportedType{"Objec"};

    ASSERT_THROW(storage.synchronize(std::move(package)), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, synchronize_types_adds_new_types_with_missing_module)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.push_back(Storage::Synchronization::Type{
        "QObject2",
        Storage::Synchronization::ImportedType{},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId3,
        {Storage::Synchronization::ExportedType{ModuleId::create(22), "Object2"},
         Storage::Synchronization::ExportedType{pathToModuleId, "Obj2"}}});

    ASSERT_THROW(storage.synchronize(std::move(package)), QmlDesigner::ExportedTypeCannotBeInserted);
}

TEST_F(ProjectStorage, synchronize_types_adds_new_types_reverse_order)
{
    auto package{createSimpleSynchronizationPackage()};
    std::reverse(package.types.begin(), package.types.end());

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                                     IsExportedType(qtQuickNativeModuleId,
                                                                    "QQuickItem"))))));
}

TEST_F(ProjectStorage, synchronize_types_overwrites_type_traits)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].traits = TypeTraits::Value;
    package.types[1].traits = TypeTraits::Value;

    storage.synchronize(SynchronizationPackage{package.imports, package.types, {sourceId1, sourceId2}});

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraits::Value),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1, "QQuickItem", fetchTypeId(sourceId2, "QObject"), TypeTraits::Value),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickItem"))))));
}

TEST_F(ProjectStorage, synchronize_types_overwrites_sources)
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

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId4, "QObject", TypeId{}, TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId3,
                                        "QQuickItem",
                                        fetchTypeId(sourceId4, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                                     IsExportedType(qtQuickNativeModuleId,
                                                                    "QQuickItem"))))));
}

TEST_F(ProjectStorage, synchronize_types_insert_type_into_prototype_chain)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].prototype = Storage::Synchronization::ImportedType{"QQuickObject"};
    package.types.push_back(Storage::Synchronization::Type{
        "QQuickObject",
        Storage::Synchronization::ImportedType{"QObject"},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId1,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Object"},
         Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QQuickObject"}}});

    storage.synchronize(
        SynchronizationPackage{importsSourceId1, {package.types[0], package.types[2]}, {sourceId1}});

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeId{}, TypeTraits::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickObject",
                                fetchTypeId(sourceId2, "QObject"),
                                TypeId{},
                                TypeTraits::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Object"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                fetchTypeId(sourceId1, "QQuickObject"),
                                TypeId{},
                                TypeTraits::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickItem"))))));
}

TEST_F(ProjectStorage, synchronize_types_insert_type_into_extension_chain)
{
    auto package{createSimpleSynchronizationPackage()};
    swap(package.types.front().extension, package.types.front().prototype);
    storage.synchronize(package);
    package.types[0].extension = Storage::Synchronization::ImportedType{"QQuickObject"};
    package.types.push_back(Storage::Synchronization::Type{
        "QQuickObject",
        Storage::Synchronization::ImportedType{},
        Storage::Synchronization::ImportedType{"QObject"},
        TypeTraits::Reference,
        sourceId1,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Object"},
         Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QQuickObject"}}});

    storage.synchronize(
        SynchronizationPackage{importsSourceId1, {package.types[0], package.types[2]}, {sourceId1}});

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeId{}, TypeTraits::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickObject",
                                TypeId{},
                                fetchTypeId(sourceId2, "QObject"),
                                TypeTraits::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Object"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                TypeId{},
                                fetchTypeId(sourceId1, "QQuickObject"),
                                TypeTraits::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickItem"))))));
}

TEST_F(ProjectStorage, synchronize_types_add_qualified_prototype)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};
    package.types.push_back(Storage::Synchronization::Type{
        "QQuickObject",
        Storage::Synchronization::ImportedType{"QObject"},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId1,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Object"},
         Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QQuickObject"}}});

    storage.synchronize(package);

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeId{}, TypeTraits::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickObject",
                                fetchTypeId(sourceId2, "QObject"),
                                TypeId{},
                                TypeTraits::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Object"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                fetchTypeId(sourceId1, "QQuickObject"),
                                TypeId{},
                                TypeTraits::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickItem"))))));
}

TEST_F(ProjectStorage, synchronize_types_add_qualified_extension)
{
    auto package{createSimpleSynchronizationPackage()};
    swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};
    package.types.push_back(Storage::Synchronization::Type{
        "QQuickObject",
        Storage::Synchronization::ImportedType{},
        Storage::Synchronization::ImportedType{"QObject"},
        TypeTraits::Reference,
        sourceId1,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Object"},
         Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QQuickObject"}}});

    storage.synchronize(package);

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeId{}, TypeTraits::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickObject",
                                TypeId{},
                                fetchTypeId(sourceId2, "QObject"),
                                TypeTraits::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Object"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                TypeId{},
                                fetchTypeId(sourceId1, "QQuickObject"),
                                TypeTraits::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickItem"))))));
}

TEST_F(ProjectStorage, synchronize_types_throws_for_missing_prototype)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types = {Storage::Synchronization::Type{
        "QQuickItem",
        Storage::Synchronization::ImportedType{"QObject"},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId1,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item"},
         Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QQuickItem"}}}};

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, synchronize_types_throws_for_missing_extension)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types = {Storage::Synchronization::Type{
        "QQuickItem",
        Storage::Synchronization::ImportedType{},
        Storage::Synchronization::ImportedType{"QObject"},
        TypeTraits::Reference,
        sourceId1,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item"},
         Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QQuickItem"}}}};

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, synchronize_types_throws_for_invalid_module)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types = {
        Storage::Synchronization::Type{"QQuickItem",
                                       Storage::Synchronization::ImportedType{"QObject"},
                                       Storage::Synchronization::ImportedType{},
                                       TypeTraits::Reference,
                                       sourceId1,
                                       {Storage::Synchronization::ExportedType{ModuleId{}, "Item"}}}};

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::ModuleDoesNotExists);
}

TEST_F(ProjectStorage, type_with_invalid_source_id_throws)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types = {
        Storage::Synchronization::Type{"QQuickItem",
                                       Storage::Synchronization::ImportedType{""},
                                       Storage::Synchronization::ImportedType{},
                                       TypeTraits::Reference,
                                       SourceId{},
                                       {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                               "Item"}}}};

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeHasInvalidSourceId);
}

TEST_F(ProjectStorage, delete_type_if_source_id_is_synchronized)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types.erase(package.types.begin());

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject"))))));
}

TEST_F(ProjectStorage, dont_delete_type_if_source_id_is_not_synchronized)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                                     IsExportedType(qtQuickNativeModuleId,
                                                                    "QQuickItem"))))));
}

TEST_F(ProjectStorage, update_exported_types_if_type_name_changes)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].typeName = "QQuickItem2";

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem2",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                                     IsExportedType(qtQuickNativeModuleId,
                                                                    "QQuickItem"))))));
}

TEST_F(ProjectStorage, breaking_prototype_chain_by_deleting_base_component_throws)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{importsSourceId1,
                                                            {package.types[0]},
                                                            {sourceId1, sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, breaking_extension_chain_by_deleting_base_component_throws)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    storage.synchronize(package);

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{importsSourceId1,
                                                            {package.types[0]},
                                                            {sourceId1, sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, synchronize_types_add_property_declarations)
{
    auto package{createSimpleSynchronizationPackage()};

    storage.synchronize(package);

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId1, "QQuickItem", fetchTypeId(sourceId2, "QObject"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
                  UnorderedElementsAre(
                      IsPropertyDeclaration("data",
                                            fetchTypeId(sourceId2, "QObject"),
                                            Storage::PropertyDeclarationTraits::IsList),
                      IsPropertyDeclaration("children",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsList
                                                | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, synchronize_types_add_property_declarations_with_missing_imports)
{
    auto package{createSimpleSynchronizationPackage()};
    package.imports.clear();

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, synchronize_types_add_property_declaration_qualified_type)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};
    package.types.push_back(
        Storage::Synchronization::Type{"QQuickObject",
                                       Storage::Synchronization::ImportedType{"QObject"},
                                       Storage::Synchronization::ImportedType{},
                                       TypeTraits::Reference,
                                       sourceId1,
                                       {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                               "Object"}}});

    storage.synchronize(package);

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId1, "QQuickItem", fetchTypeId(sourceId2, "QObject"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
                  UnorderedElementsAre(
                      IsPropertyDeclaration("data",
                                            fetchTypeId(sourceId1, "QQuickObject"),
                                            Storage::PropertyDeclarationTraits::IsList),
                      IsPropertyDeclaration("children",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsList
                                                | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_property_declaration_type)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "QQuickItem"};

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId1, "QQuickItem", fetchTypeId(sourceId2, "QObject"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
                  UnorderedElementsAre(
                      IsPropertyDeclaration("data",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsList),
                      IsPropertyDeclaration("children",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsList
                                                | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_declaration_traits)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsPointer;

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId1, "QQuickItem", fetchTypeId(sourceId2, "QObject"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
                  UnorderedElementsAre(
                      IsPropertyDeclaration("data",
                                            fetchTypeId(sourceId2, "QObject"),
                                            Storage::PropertyDeclarationTraits::IsPointer),
                      IsPropertyDeclaration("children",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsList
                                                | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_declaration_traits_and_type)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsPointer;
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "QQuickItem"};

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId1, "QQuickItem", fetchTypeId(sourceId2, "QObject"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
                  UnorderedElementsAre(
                      IsPropertyDeclaration("data",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsPointer),
                      IsPropertyDeclaration("children",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsList
                                                | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, synchronize_types_removes_a_property_declaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations.pop_back();

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(sourceId1,
                                             "QQuickItem",
                                             fetchTypeId(sourceId2, "QObject"),
                                             TypeTraits::Reference),
                               Field(&Storage::Synchronization::Type::propertyDeclarations,
                                     UnorderedElementsAre(IsPropertyDeclaration(
                                         "data",
                                         fetchTypeId(sourceId2, "QObject"),
                                         Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, synchronize_types_adds_a_property_declaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations.push_back(
        Storage::Synchronization::PropertyDeclaration{"object",
                                                      Storage::Synchronization::ImportedType{
                                                          "QObject"},
                                                      Storage::PropertyDeclarationTraits::IsPointer});

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId1, "QQuickItem", fetchTypeId(sourceId2, "QObject"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
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

TEST_F(ProjectStorage, synchronize_types_rename_a_property_declaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[1].name = "objects";

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId1, "QQuickItem", fetchTypeId(sourceId2, "QObject"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
                  UnorderedElementsAre(
                      IsPropertyDeclaration("data",
                                            fetchTypeId(sourceId2, "QObject"),
                                            Storage::PropertyDeclarationTraits::IsList),
                      IsPropertyDeclaration("objects",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsList
                                                | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, using_non_existing_property_type_throws)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "QObject2"};
    package.types.pop_back();
    package.imports = importsSourceId1;

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, using_non_existing_qualified_exported_property_type_with_wrong_name_throws)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "QObject2", Storage::Import{qmlNativeModuleId, Storage::Version{}, sourceId1}};
    package.types.pop_back();
    package.imports = importsSourceId1;

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, using_non_existing_qualified_exported_property_type_with_wrong_module_throws)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "QObject", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};
    package.types.pop_back();
    package.imports = importsSourceId1;

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, breaking_property_declaration_type_dependency_by_deleting_type_throws)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].prototype = Storage::Synchronization::ImportedType{};
    package.types.pop_back();
    package.imports = importsSourceId1;

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, synchronize_types_add_function_declarations)
{
    auto package{createSimpleSynchronizationPackage()};

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_function_declaration_return_type)
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
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_function_declaration_name)
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
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_function_declaration_pop_parameters)
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
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_function_declaration_append_parameters)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations[1].parameters.push_back(
        Storage::Synchronization::ParameterDeclaration{"arg4", "int"});

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_function_declaration_change_parameter_name)
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
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_function_declaration_change_parameter_type_name)
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
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_function_declaration_change_parameter_traits)
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
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_removes_function_declaration)
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
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]))))));
}

TEST_F(ProjectStorage, synchronize_types_add_function_declaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations.push_back(Storage::Synchronization::FunctionDeclaration{
        "name", "string", {Storage::Synchronization::ParameterDeclaration{"arg", "int"}}});

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]),
                                                     Eq(package.types[0].functionDeclarations[2]))))));
}

TEST_F(ProjectStorage, synchronize_types_add_function_declarations_with_overloads)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].functionDeclarations.push_back(
        Storage::Synchronization::FunctionDeclaration{"execute", "", {}});

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]),
                                                     Eq(package.types[0].functionDeclarations[2]))))));
}

TEST_F(ProjectStorage, synchronize_types_function_declarations_adding_overload)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations.push_back(
        Storage::Synchronization::FunctionDeclaration{"execute", "", {}});

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]),
                                                     Eq(package.types[0].functionDeclarations[2]))))));
}

TEST_F(ProjectStorage, synchronize_types_function_declarations_removing_overload)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].functionDeclarations.push_back(
        Storage::Synchronization::FunctionDeclaration{"execute", "", {}});
    storage.synchronize(package);
    package.types[0].functionDeclarations.pop_back();

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::functionDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                                     Eq(package.types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_add_signal_declarations)
{
    auto package{createSimpleSynchronizationPackage()};

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                                     Eq(package.types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_signal_declaration_name)
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
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                                     Eq(package.types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_signal_declaration_pop_parameters)
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
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                                     Eq(package.types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_signal_declaration_append_parameters)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations[1].parameters.push_back(
        Storage::Synchronization::ParameterDeclaration{"arg4", "int"});

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                                     Eq(package.types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_signal_declaration_change_parameter_name)
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
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                                     Eq(package.types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_signal_declaration_change_parameter_type_name)
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
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                                     Eq(package.types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_signal_declaration_change_parameter_traits)
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
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                                     Eq(package.types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_removes_signal_declaration)
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
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]))))));
}

TEST_F(ProjectStorage, synchronize_types_add_signal_declaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations.push_back(Storage::Synchronization::SignalDeclaration{
        "name", {Storage::Synchronization::ParameterDeclaration{"arg", "int"}}});

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                                     Eq(package.types[0].signalDeclarations[1]),
                                                     Eq(package.types[0].signalDeclarations[2]))))));
}

TEST_F(ProjectStorage, synchronize_types_add_signal_declarations_with_overloads)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].signalDeclarations.push_back(
        Storage::Synchronization::SignalDeclaration{"execute", {}});

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                                     Eq(package.types[0].signalDeclarations[1]),
                                                     Eq(package.types[0].signalDeclarations[2]))))));
}

TEST_F(ProjectStorage, synchronize_types_signal_declarations_adding_overload)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations.push_back(
        Storage::Synchronization::SignalDeclaration{"execute", {}});

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                                     Eq(package.types[0].signalDeclarations[1]),
                                                     Eq(package.types[0].signalDeclarations[2]))))));
}

TEST_F(ProjectStorage, synchronize_types_signal_declarations_removing_overload)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].signalDeclarations.push_back(
        Storage::Synchronization::SignalDeclaration{"execute", {}});
    storage.synchronize(package);
    package.types[0].signalDeclarations.pop_back();

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::signalDeclarations,
                                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                                     Eq(package.types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_add_enumeration_declarations)
{
    auto package{createSimpleSynchronizationPackage()};

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId1,
                                  "QQuickItem",
                                  fetchTypeId(sourceId2, "QObject"),
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_enumeration_declaration_name)
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
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_enumeration_declaration_pop_enumerator_declaration)
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
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_enumeration_declaration_append_enumerator_declaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations[1].enumeratorDeclarations.push_back(
        Storage::Synchronization::EnumeratorDeclaration{"Haa", 54});

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId1,
                                  "QQuickItem",
                                  fetchTypeId(sourceId2, "QObject"),
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_enumeration_declaration_change_enumerator_declaration_name)
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
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_changes_enumeration_declaration_change_enumerator_declaration_value)
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
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage,
       synchronize_types_changes_enumeration_declaration_add_that_enumerator_declaration_has_value)
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
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage,
       synchronize_types_changes_enumeration_declaration_remove_that_enumerator_declaration_has_value)
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
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage, synchronize_types_removes_enumeration_declaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations.pop_back();

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(sourceId1,
                                             "QQuickItem",
                                             fetchTypeId(sourceId2, "QObject"),
                                             TypeTraits::Reference),
                               Field(&Storage::Synchronization::Type::enumerationDeclarations,
                                     UnorderedElementsAre(
                                         Eq(package.types[0].enumerationDeclarations[0]))))));
}

TEST_F(ProjectStorage, synchronize_types_add_enumeration_declaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations.push_back(Storage::Synchronization::EnumerationDeclaration{
        "name", {Storage::Synchronization::EnumeratorDeclaration{"Foo", 98, true}}});

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId1,
                                  "QQuickItem",
                                  fetchTypeId(sourceId2, "QObject"),
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]),
                                               Eq(package.types[0].enumerationDeclarations[2]))))));
}

TEST_F(ProjectStorage, fetch_type_id_by_source_id_and_name)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    auto typeId = storage.fetchTypeIdByName(sourceId2, "QObject");

    ASSERT_THAT(storage.fetchTypeIdByExportedName("Object"), Eq(typeId));
}

TEST_F(ProjectStorage, fetch_type_id_by_exported_name)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    auto typeId = storage.fetchTypeIdByExportedName("Object");

    ASSERT_THAT(storage.fetchTypeIdByName(sourceId2, "QObject"), Eq(typeId));
}

TEST_F(ProjectStorage, fetch_type_id_by_impor_ids_and_exported_name)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    auto typeId = storage.fetchTypeIdByModuleIdsAndExportedName({qmlModuleId, qtQuickModuleId},
                                                                "Object");

    ASSERT_THAT(storage.fetchTypeIdByName(sourceId2, "QObject"), Eq(typeId));
}

TEST_F(ProjectStorage, fetch_invalid_type_id_by_impor_ids_and_exported_name_if_module_ids_are_empty)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    auto typeId = storage.fetchTypeIdByModuleIdsAndExportedName({}, "Object");

    ASSERT_FALSE(typeId.isValid());
}

TEST_F(ProjectStorage, fetch_invalid_type_id_by_impor_ids_and_exported_name_if_module_ids_are_invalid)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    auto typeId = storage.fetchTypeIdByModuleIdsAndExportedName({ModuleId{}}, "Object");

    ASSERT_FALSE(typeId.isValid());
}

TEST_F(ProjectStorage, fetch_invalid_type_id_by_impor_ids_and_exported_name_if_not_in_module)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    auto qtQuickModuleId = storage.moduleId("QtQuick");

    auto typeId = storage.fetchTypeIdByModuleIdsAndExportedName({qtQuickModuleId}, "Object");

    ASSERT_FALSE(typeId.isValid());
}

TEST_F(ProjectStorage, synchronize_types_add_alias_declarations)
{
    auto package{createSynchronizationPackageWithAliases()};

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId1, "QQuickItem"),
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::propertyDeclarations,
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

TEST_F(ProjectStorage, synchronize_types_add_alias_declarations_again)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId1, "QQuickItem"),
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::propertyDeclarations,
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

TEST_F(ProjectStorage, synchronize_types_remove_alias_declarations)
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
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    fetchTypeId(sourceId1, "QQuickItem"),
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("data",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, synchronize_types_add_alias_declarations_throws_for_wrong_type_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "QQuickItemWrong"};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{importsSourceId4,
                                                            {package.types[2]},
                                                            {sourceId4},
                                                            moduleDependenciesSourceId4,
                                                            {sourceId4}}),
                 QmlDesigner::TypeNameDoesNotExists);
}
TEST_F(ProjectStorage, synchronize_types_add_alias_declarations_throws_for_wrong_property_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[1].aliasPropertyName = "childrenWrong";

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{package.imports,
                                                            package.types,
                                                            {sourceId4},
                                                            package.moduleDependencies,
                                                            {sourceId4}}),
                 QmlDesigner::PropertyNameDoesNotExists);
}

TEST_F(ProjectStorage, synchronize_types_change_alias_declarations_type_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[2].typeName = Storage::Synchronization::ImportedType{
        "Obj2"};
    importsSourceId3.emplace_back(pathToModuleId, Storage::Version{}, sourceId3);

    storage.synchronize(SynchronizationPackage{importsSourceId3, {package.types[2]}, {sourceId3}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId1, "QQuickItem"),
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::propertyDeclarations,
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

TEST_F(ProjectStorage, synchronize_types_change_alias_declarations_property_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[2].aliasPropertyName = "children";

    storage.synchronize(SynchronizationPackage{importsSourceId3, {package.types[2]}, {sourceId3}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId3, "QAliasItem", fetchTypeId(sourceId1, "QQuickItem"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
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

TEST_F(ProjectStorage, synchronize_types_change_alias_declarations_to_property_declaration)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations.pop_back();
    package.types[2].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects",
        Storage::Synchronization::ImportedType{"QQuickItem"},
        Storage::PropertyDeclarationTraits::IsList | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize(SynchronizationPackage{importsSourceId3, {package.types[2]}, {sourceId3}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId3, "QAliasItem", fetchTypeId(sourceId1, "QQuickItem"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
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

TEST_F(ProjectStorage, synchronize_types_change_property_declarations_to_alias_declaration)
{
    auto package{createSynchronizationPackageWithAliases()};
    auto packageChanged = package;
    packageChanged.types[2].propertyDeclarations.pop_back();
    packageChanged.types[2].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects",
        Storage::Synchronization::ImportedType{"QQuickItem"},
        Storage::PropertyDeclarationTraits::IsList | Storage::PropertyDeclarationTraits::IsReadOnly});
    storage.synchronize(packageChanged);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId1, "QQuickItem"),
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::propertyDeclarations,
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

TEST_F(ProjectStorage, synchronize_types_change_alias_target_property_declaration_traits)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[1].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly;

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId3, "QAliasItem", fetchTypeId(sourceId1, "QQuickItem"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
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

TEST_F(ProjectStorage, synchronize_types_change_alias_target_property_declaration_type_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Item"};
    importsSourceId2.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId1, "QQuickItem"),
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::propertyDeclarations,
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

TEST_F(ProjectStorage, synchronize_types_remove_property_declaration_with_an_alias_throws)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[1].propertyDeclarations.pop_back();

    ASSERT_THROW(storage.synchronize(
                     SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}}),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorage, synchronize_types_remove_property_declaration_and_alias)
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
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    fetchTypeId(sourceId1, "QQuickItem"),
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("data",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, synchronize_types_remove_type_with_alias_target_property_declaration_throws)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[2].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId3);
    storage.synchronize(package);

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{sourceId4}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, synchronize_types_remove_type_and_alias_property_declaration)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[2].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
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
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    fetchTypeId(sourceId1, "QQuickItem"),
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("data",
                                                    fetchTypeId(sourceId2, "QObject"),
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, update_alias_property_if_property_is_overloaded)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects",
        Storage::Synchronization::ImportedType{"QQuickItem"},
        Storage::PropertyDeclarationTraits::IsList | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId3, "QAliasItem", fetchTypeId(sourceId1, "QQuickItem"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
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

TEST_F(ProjectStorage, alias_property_is_overloaded)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[0].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects",
        Storage::Synchronization::ImportedType{"QQuickItem"},
        Storage::PropertyDeclarationTraits::IsList | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize(package);

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId3, "QAliasItem", fetchTypeId(sourceId1, "QQuickItem"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
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

TEST_F(ProjectStorage, update_alias_property_if_overloaded_property_is_removed)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[0].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects",
        Storage::Synchronization::ImportedType{"QQuickItem"},
        Storage::PropertyDeclarationTraits::IsList | Storage::PropertyDeclarationTraits::IsReadOnly});
    storage.synchronize(package);
    package.types[0].propertyDeclarations.pop_back();

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId1, "QQuickItem"),
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::propertyDeclarations,
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

TEST_F(ProjectStorage, relink_alias_property)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
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
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::propertyDeclarations,
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

TEST_F(ProjectStorage, do_not_relink_alias_property_for_qualified_imported_type_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
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
       do_relink_alias_property_for_qualified_imported_type_name_even_if_an_other_similar_time_name_exists)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object2", Storage::Import{pathToModuleId, Storage::Version{}, sourceId2}};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    package.types.push_back(Storage::Synchronization::Type{
        "QObject2",
        Storage::Synchronization::ImportedType{},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId5,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Object2"},
         Storage::Synchronization::ExportedType{qtQuickModuleId, "Obj2"}}});
    package.updatedSourceIds.push_back(sourceId5);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId1, "QQuickItem"),
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::propertyDeclarations,
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

TEST_F(ProjectStorage, relink_alias_property_react_to_type_name_change)
{
    auto package{createSynchronizationPackageWithAliases2()};
    package.types[2].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "items", Storage::Synchronization::ImportedType{"Item"}, "children"});
    storage.synchronize(package);
    package.types[0].typeName = "QQuickItem2";

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId3, "QAliasItem", fetchTypeId(sourceId2, "QObject"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
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

TEST_F(ProjectStorage, do_not_relink_alias_property_for_deleted_type)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
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
                                           TypeTraits::Reference))));
}

TEST_F(ProjectStorage, do_not_relink_alias_property_for_deleted_type_and_property_type)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);
    package.types[0].prototype = Storage::Synchronization::ImportedType{};
    importsSourceId1.emplace_back(pathToModuleId, Storage::Version{}, sourceId1);
    importsSourceId4.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId4);
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.types[3].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Item"};

    storage.synchronize(SynchronizationPackage{importsSourceId1 + importsSourceId4,
                                               {package.types[0], package.types[3]},
                                               {sourceId1, sourceId2, sourceId3, sourceId4}});

    ASSERT_THAT(storage.fetchTypes(), SizeIs(2));
}

TEST_F(ProjectStorage, do_not_relink_alias_property_for_deleted_type_and_property_type_name_change)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);
    package.types[3].exportedTypes[0].moduleId = qtQuickModuleId;
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "QObject"};
    importsSourceId4.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId4);

    storage.synchronize(SynchronizationPackage{importsSourceId2 + importsSourceId4,
                                               {package.types[1], package.types[3]},
                                               {sourceId2, sourceId3, sourceId4}});

    ASSERT_THAT(storage.fetchTypes(),
                Not(Contains(IsStorageType(sourceId3,
                                           "QAliasItem",
                                           fetchTypeId(sourceId1, "QQuickItem"),
                                           TypeTraits::Reference))));
}

TEST_F(ProjectStorage, do_not_relink_property_type_does_not_exists)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{sourceId4}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, do_not_relink_alias_property_type_does_not_exists)
{
    auto package{createSynchronizationPackageWithAliases2()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{sourceId1}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, change_prototype_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[1].typeName = "QObject3";

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       fetchTypeId(sourceId2, "QObject3"),
                                       TypeId{},
                                       TypeTraits::Reference)));
}

TEST_F(ProjectStorage, change_extension_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    storage.synchronize(package);
    package.types[1].typeName = "QObject3";

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       TypeId{},
                                       fetchTypeId(sourceId2, "QObject3"),
                                       TypeTraits::Reference)));
}

TEST_F(ProjectStorage, change_prototype_type_module_id)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[1].exportedTypes[2].moduleId = qtQuickModuleId;

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       fetchTypeId(sourceId2, "QObject"),
                                       TypeId{},
                                       TypeTraits::Reference)));
}

TEST_F(ProjectStorage, change_extension_type_module_id)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    storage.synchronize(package);
    package.types[1].exportedTypes[2].moduleId = qtQuickModuleId;

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       TypeId{},
                                       fetchTypeId(sourceId2, "QObject"),
                                       TypeTraits::Reference)));
}

TEST_F(ProjectStorage, change_qualified_prototype_type_module_id_throws)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};
    storage.synchronize(package);
    package.types[1].exportedTypes[0].moduleId = qtQuickModuleId;

    ASSERT_THROW(storage.synchronize(
                     SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, change_qualified_extension_type_module_id_throws)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};
    storage.synchronize(package);
    package.types[1].exportedTypes[0].moduleId = qtQuickModuleId;

    ASSERT_THROW(storage.synchronize(
                     SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, change_qualified_prototype_type_module_id)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};
    storage.synchronize(package);
    package.types[1].exportedTypes[0].moduleId = qtQuickModuleId;
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};

    storage.synchronize(SynchronizationPackage{importsSourceId1 + importsSourceId2,
                                               {package.types[0], package.types[1]},
                                               {sourceId1, sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       fetchTypeId(sourceId2, "QObject"),
                                       TypeId{},
                                       TypeTraits::Reference)));
}

TEST_F(ProjectStorage, change_qualified_extension_type_module_id)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};
    storage.synchronize(package);
    package.types[1].exportedTypes[0].moduleId = qtQuickModuleId;
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};

    storage.synchronize(SynchronizationPackage{importsSourceId1 + importsSourceId2,
                                               {package.types[0], package.types[1]},
                                               {sourceId1, sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       TypeId{},
                                       fetchTypeId(sourceId2, "QObject"),
                                       TypeTraits::Reference)));
}

TEST_F(ProjectStorage, change_prototype_type_name_and_module_id)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Object"};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object"};
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
                                       TypeId{},
                                       TypeTraits::Reference)));
}

TEST_F(ProjectStorage, change_extension_type_name_and_module_id)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].extension = Storage::Synchronization::ImportedType{"Object"};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object"};
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
                                       TypeId{},
                                       fetchTypeId(sourceId2, "QObject3"),
                                       TypeTraits::Reference)));
}

TEST_F(ProjectStorage, change_prototype_type_name_throws_for_wrong_native_prototupe_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object"};
    storage.synchronize(package);
    package.types[1].exportedTypes[2].name = "QObject3";
    package.types[1].typeName = "QObject3";

    ASSERT_THROW(storage.synchronize(
                     SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, change_extension_type_name_throws_for_wrong_native_extension_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object"};
    storage.synchronize(package);
    package.types[1].exportedTypes[2].name = "QObject3";
    package.types[1].typeName = "QObject3";

    ASSERT_THROW(storage.synchronize(
                     SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, throw_for_prototype_chain_cycles)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[1].prototype = Storage::Synchronization::ImportedType{"Object2"};
    package.types.push_back(Storage::Synchronization::Type{
        "QObject2",
        Storage::Synchronization::ImportedType{"Item"},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId3,
        {Storage::Synchronization::ExportedType{pathToModuleId, "Object2"},
         Storage::Synchronization::ExportedType{pathToModuleId, "Obj2"}}});
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId3);

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{package.imports,
                                                            package.types,
                                                            {sourceId1, sourceId2, sourceId3},
                                                            package.moduleDependencies,
                                                            package.updatedModuleDependencySourceIds}),
                 QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorage, throw_for_extension_chain_cycles)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[1].extension = Storage::Synchronization::ImportedType{"Object2"};
    package.types.push_back(Storage::Synchronization::Type{
        "QObject2",
        Storage::Synchronization::ImportedType{"Item"},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId3,
        {Storage::Synchronization::ExportedType{pathToModuleId, "Object2"},
         Storage::Synchronization::ExportedType{pathToModuleId, "Obj2"}}});
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId3);

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{package.imports,
                                                            package.types,
                                                            {sourceId1, sourceId2, sourceId3},
                                                            package.moduleDependencies,
                                                            package.updatedModuleDependencySourceIds}),
                 QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorage, throw_for_type_id_and_prototype_id_are_the_same)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[1].prototype = Storage::Synchronization::ImportedType{"Object"};

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorage, throw_for_type_id_and_extension_id_are_the_same)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[1].prototype = Storage::Synchronization::ImportedType{"Object"};

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorage, throw_for_type_id_and_prototype_id_are_the_same_for_relinking)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[1].prototype = Storage::Synchronization::ImportedType{"Item"};
    package.types[1].typeName = "QObject2";
    importsSourceId2.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);

    ASSERT_THROW(storage.synchronize(
                     SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}}),
                 QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorage, throw_for_type_id_and_extenssion_id_are_the_same_for_relinking)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    storage.synchronize(package);
    package.types[1].extension = Storage::Synchronization::ImportedType{"Item"};
    package.types[1].typeName = "QObject2";
    importsSourceId2.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);

    ASSERT_THROW(storage.synchronize(
                     SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}}),
                 QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorage, recursive_aliases)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(sourceId5,
                                             "QAliasItem2",
                                             fetchTypeId(sourceId2, "QObject"),
                                             TypeTraits::Reference),
                               Field(&Storage::Synchronization::Type::propertyDeclarations,
                                     ElementsAre(IsPropertyDeclaration(
                                         "objects",
                                         fetchTypeId(sourceId2, "QObject"),
                                         Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, recursive_aliases_change_property_type)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};
    storage.synchronize(package);
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    importsSourceId2.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(sourceId5,
                                             "QAliasItem2",
                                             fetchTypeId(sourceId2, "QObject"),
                                             TypeTraits::Reference),
                               Field(&Storage::Synchronization::Type::propertyDeclarations,
                                     ElementsAre(IsPropertyDeclaration(
                                         "objects",
                                         fetchTypeId(sourceId4, "QObject2"),
                                         Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, update_aliases_after_injecting_property)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects",
        Storage::Synchronization::ImportedType{"Item"},
        Storage::PropertyDeclarationTraits::IsList | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(sourceId5,
                                             "QAliasItem2",
                                             fetchTypeId(sourceId2, "QObject"),
                                             TypeTraits::Reference),
                               Field(&Storage::Synchronization::Type::propertyDeclarations,
                                     ElementsAre(IsPropertyDeclaration(
                                         "objects",
                                         fetchTypeId(sourceId1, "QQuickItem"),
                                         Storage::PropertyDeclarationTraits::IsList
                                             | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, update_aliases_after_change_alias_to_property)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations.clear();
    package.types[2].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects",
        Storage::Synchronization::ImportedType{"Item"},
        Storage::PropertyDeclarationTraits::IsList | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize(SynchronizationPackage{importsSourceId3, {package.types[2]}, {sourceId3}});

    ASSERT_THAT(storage.fetchTypes(),
                AllOf(Contains(AllOf(IsStorageType(sourceId5,
                                                   "QAliasItem2",
                                                   fetchTypeId(sourceId2, "QObject"),
                                                   TypeTraits::Reference),
                                     Field(&Storage::Synchronization::Type::propertyDeclarations,
                                           ElementsAre(IsPropertyDeclaration(
                                               "objects",
                                               fetchTypeId(sourceId1, "QQuickItem"),
                                               Storage::PropertyDeclarationTraits::IsList
                                                   | Storage::PropertyDeclarationTraits::IsReadOnly,
                                               "objects"))))),
                      Contains(AllOf(IsStorageType(sourceId3,
                                                   "QAliasItem",
                                                   fetchTypeId(sourceId1, "QQuickItem"),
                                                   TypeTraits::Reference),
                                     Field(&Storage::Synchronization::Type::propertyDeclarations,
                                           ElementsAre(IsPropertyDeclaration(
                                               "objects",
                                               fetchTypeId(sourceId1, "QQuickItem"),
                                               Storage::PropertyDeclarationTraits::IsList
                                                   | Storage::PropertyDeclarationTraits::IsReadOnly,
                                               "")))))));
}

TEST_F(ProjectStorage, update_aliases_after_change_property_to_alias)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};
    package.types[3].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly;
    storage.synchronize(package);
    package.types[1].propertyDeclarations.clear();
    package.types[1].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects", Storage::Synchronization::ImportedType{"Object2"}, "objects"});
    importsSourceId2.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(sourceId5,
                                             "QAliasItem2",
                                             fetchTypeId(sourceId2, "QObject"),
                                             TypeTraits::Reference),
                               Field(&Storage::Synchronization::Type::propertyDeclarations,
                                     ElementsAre(IsPropertyDeclaration(
                                         "objects",
                                         fetchTypeId(sourceId2, "QObject"),
                                         Storage::PropertyDeclarationTraits::IsList
                                             | Storage::PropertyDeclarationTraits::IsReadOnly,
                                         "objects"))))));
}

TEST_F(ProjectStorage, check_for_proto_type_cycle_throws)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};
    package.types[1].propertyDeclarations.clear();
    package.types[1].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects", Storage::Synchronization::ImportedType{"AliasItem2"}, "objects"});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::AliasChainCycle);
}

TEST_F(ProjectStorage, check_for_proto_type_cycle_after_update_throws)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};
    storage.synchronize(package);
    package.types[1].propertyDeclarations.clear();
    package.types[1].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects", Storage::Synchronization::ImportedType{"AliasItem2"}, "objects"});
    importsSourceId2.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);

    ASSERT_THROW(storage.synchronize(
                     SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}}),
                 QmlDesigner::AliasChainCycle);
}

TEST_F(ProjectStorage, qualified_prototype)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};
    package.types.push_back(
        Storage::Synchronization::Type{"QQuickObject",
                                       Storage::Synchronization::ImportedType{},
                                       Storage::Synchronization::ImportedType{},
                                       TypeTraits::Reference,
                                       sourceId3,
                                       {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                               "Object"}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       fetchTypeId(sourceId2, "QObject"),
                                       TypeId{},
                                       TypeTraits::Reference)));
}

TEST_F(ProjectStorage, qualified_extension)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};
    package.types.push_back(
        Storage::Synchronization::Type{"QQuickObject",
                                       Storage::Synchronization::ImportedType{},
                                       Storage::Synchronization::ImportedType{},
                                       TypeTraits::Reference,
                                       sourceId3,
                                       {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                               "Object"}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       TypeId{},
                                       fetchTypeId(sourceId2, "QObject"),
                                       TypeTraits::Reference)));
}

TEST_F(ProjectStorage, qualified_prototype_upper_down_the_module_chain_throws)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, qualified_extension_upper_down_the_module_chain_throws)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, qualified_prototype_upper_in_the_module_chain)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};
    package.types.push_back(
        Storage::Synchronization::Type{"QQuickObject",
                                       Storage::Synchronization::ImportedType{},
                                       Storage::Synchronization::ImportedType{},
                                       TypeTraits::Reference,
                                       sourceId3,
                                       {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                               "Object"}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       fetchTypeId(sourceId3, "QQuickObject"),
                                       TypeId{},
                                       TypeTraits::Reference)));
}

TEST_F(ProjectStorage, qualified_extension_upper_in_the_module_chain)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};
    package.types.push_back(
        Storage::Synchronization::Type{"QQuickObject",
                                       Storage::Synchronization::ImportedType{},
                                       Storage::Synchronization::ImportedType{},
                                       TypeTraits::Reference,
                                       sourceId3,
                                       {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                               "Object"}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       TypeId{},
                                       fetchTypeId(sourceId3, "QQuickObject"),
                                       TypeTraits::Reference)));
}

TEST_F(ProjectStorage, qualified_prototype_with_wrong_version_throws)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{4}, sourceId1}};
    package.types.push_back(
        Storage::Synchronization::Type{"QQuickObject",
                                       Storage::Synchronization::ImportedType{},
                                       Storage::Synchronization::ImportedType{},
                                       TypeTraits::Reference,
                                       sourceId3,
                                       {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                               "Object"}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, qualified_extension_with_wrong_version_throws)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{4}, sourceId1}};
    package.types.push_back(
        Storage::Synchronization::Type{"QQuickObject",
                                       Storage::Synchronization::ImportedType{},
                                       Storage::Synchronization::ImportedType{},
                                       TypeTraits::Reference,
                                       sourceId3,
                                       {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                               "Object"}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, qualified_prototype_with_version)
{
    auto package{createSimpleSynchronizationPackage()};
    package.imports[0].version = Storage::Version{2};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{"Object",
                                                                                 package.imports[0]};
    package.types.push_back(
        Storage::Synchronization::Type{"QQuickObject",
                                       Storage::Synchronization::ImportedType{},
                                       Storage::Synchronization::ImportedType{},
                                       TypeTraits::Reference,
                                       sourceId3,
                                       {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                               "Object"}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       fetchTypeId(sourceId2, "QObject"),
                                       TypeId{},
                                       TypeTraits::Reference)));
}

TEST_F(ProjectStorage, qualified_extension_with_version)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.imports[0].version = Storage::Version{2};
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{"Object",
                                                                                 package.imports[0]};
    package.types.push_back(
        Storage::Synchronization::Type{"QQuickObject",
                                       Storage::Synchronization::ImportedType{},
                                       Storage::Synchronization::ImportedType{},
                                       TypeTraits::Reference,
                                       sourceId3,
                                       {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                               "Object"}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       TypeId{},
                                       fetchTypeId(sourceId2, "QObject"),
                                       TypeTraits::Reference)));
}

TEST_F(ProjectStorage, qualified_prototype_with_version_in_the_proto_type_chain)
{
    auto package{createSimpleSynchronizationPackage()};
    package.imports[1].version = Storage::Version{2};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{"Object",
                                                                                 package.imports[1]};
    package.types[0].exportedTypes[0].version = Storage::Version{2};
    package.types.push_back(Storage::Synchronization::Type{
        "QQuickObject",
        Storage::Synchronization::ImportedType{},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId3,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Object",
                                                Storage::Version{2}}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       fetchTypeId(sourceId3, "QQuickObject"),
                                       TypeId{},
                                       TypeTraits::Reference)));
}

TEST_F(ProjectStorage, qualified_extension_with_version_in_the_proto_type_chain)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.imports[1].version = Storage::Version{2};
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{"Object",
                                                                                 package.imports[1]};
    package.types[0].exportedTypes[0].version = Storage::Version{2};
    package.types.push_back(Storage::Synchronization::Type{
        "QQuickObject",
        Storage::Synchronization::ImportedType{},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId3,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Object",
                                                Storage::Version{2}}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       TypeId{},
                                       fetchTypeId(sourceId3, "QQuickObject"),
                                       TypeTraits::Reference)));
}

TEST_F(ProjectStorage, qualified_prototype_with_version_down_the_proto_type_chain_throws)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{2}, sourceId1}};

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, qualified_extension_with_version_down_the_proto_type_chain_throws)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{2}, sourceId1}};

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, qualified_property_declaration_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};
    package.types.push_back(
        Storage::Synchronization::Type{"QQuickObject",
                                       Storage::Synchronization::ImportedType{},
                                       Storage::Synchronization::ImportedType{},
                                       TypeTraits::Reference,
                                       sourceId3,
                                       {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                               "Object"}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    Field(&Storage::Synchronization::Type::propertyDeclarations,
                          Contains(IsPropertyDeclaration("data",
                                                         fetchTypeId(sourceId2, "QObject"),
                                                         Storage::PropertyDeclarationTraits::IsList)))));
}

TEST_F(ProjectStorage, qualified_property_declaration_type_name_down_the_module_chain_throws)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, qualified_property_declaration_type_name_in_the_module_chain)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};
    package.types.push_back(
        Storage::Synchronization::Type{"QQuickObject",
                                       Storage::Synchronization::ImportedType{},
                                       Storage::Synchronization::ImportedType{},
                                       TypeTraits::Reference,
                                       sourceId3,
                                       {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                               "Object"}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    Field(&Storage::Synchronization::Type::propertyDeclarations,
                          Contains(IsPropertyDeclaration("data",
                                                         fetchTypeId(sourceId3, "QQuickObject"),
                                                         Storage::PropertyDeclarationTraits::IsList)))));
}

TEST_F(ProjectStorage, qualified_property_declaration_type_name_with_version)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{2}, sourceId1}};
    package.imports.emplace_back(qmlModuleId, Storage::Version{2}, sourceId1);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    Field(&Storage::Synchronization::Type::propertyDeclarations,
                          Contains(IsPropertyDeclaration("data",
                                                         fetchTypeId(sourceId2, "QObject"),
                                                         Storage::PropertyDeclarationTraits::IsList)))));
}

TEST_F(ProjectStorage, change_property_type_module_id_with_qualified_type_throws)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};
    storage.synchronize(package);
    package.types[1].exportedTypes[0].moduleId = qtQuickModuleId;

    ASSERT_THROW(storage.synchronize(
                     SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, change_property_type_module_id_with_qualified_type)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};
    package.types[1].exportedTypes[0].moduleId = qtQuickModuleId;
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);

    storage.synchronize(package);

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId1, "QQuickItem", fetchTypeId(sourceId2, "QObject"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
                  Contains(IsPropertyDeclaration("data",
                                                 fetchTypeId(sourceId2, "QObject"),
                                                 Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, add_file_statuses)
{
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};

    storage.synchronize(SynchronizationPackage{{sourceId1, sourceId2}, {fileStatus1, fileStatus2}});

    ASSERT_THAT(convert(storage.fetchAllFileStatuses()),
                UnorderedElementsAre(fileStatus1, fileStatus2));
}

TEST_F(ProjectStorage, remove_file_status)
{
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    storage.synchronize(SynchronizationPackage{{sourceId1, sourceId2}, {fileStatus1, fileStatus2}});

    storage.synchronize(SynchronizationPackage{{sourceId1, sourceId2}, {fileStatus1}});

    ASSERT_THAT(convert(storage.fetchAllFileStatuses()), UnorderedElementsAre(fileStatus1));
}

TEST_F(ProjectStorage, update_file_status)
{
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    FileStatus fileStatus2b{sourceId2, 102, 102};
    storage.synchronize(SynchronizationPackage{{sourceId1, sourceId2}, {fileStatus1, fileStatus2}});

    storage.synchronize(SynchronizationPackage{{sourceId1, sourceId2}, {fileStatus1, fileStatus2b}});

    ASSERT_THAT(convert(storage.fetchAllFileStatuses()),
                UnorderedElementsAre(fileStatus1, fileStatus2b));
}

TEST_F(ProjectStorage, throw_for_invalid_source_id_in_file_status)
{
    FileStatus fileStatus1{SourceId{}, 100, 100};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{sourceId1}, {fileStatus1}}),
                 QmlDesigner::FileStatusHasInvalidSourceId);
}

TEST_F(ProjectStorage, fetch_all_file_statuses)
{
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    storage.synchronize(SynchronizationPackage{{sourceId1, sourceId2}, {fileStatus1, fileStatus2}});

    auto fileStatuses = convert(storage.fetchAllFileStatuses());

    ASSERT_THAT(fileStatuses, ElementsAre(fileStatus1, fileStatus2));
}

TEST_F(ProjectStorage, fetch_all_file_statuses_reverse)
{
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    storage.synchronize(SynchronizationPackage{{sourceId1, sourceId2}, {fileStatus2, fileStatus1}});

    auto fileStatuses = convert(storage.fetchAllFileStatuses());

    ASSERT_THAT(fileStatuses, ElementsAre(fileStatus1, fileStatus2));
}

TEST_F(ProjectStorage, fetch_file_status)
{
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    storage.synchronize(SynchronizationPackage{{sourceId1, sourceId2}, {fileStatus1, fileStatus2}});

    auto fileStatus = storage.fetchFileStatus(sourceId1);

    ASSERT_THAT(fileStatus, Eq(fileStatus1));
}

TEST_F(ProjectStorage, synchronize_types_without_type_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[3].typeName.clear();
    package.types[3].prototype = Storage::Synchronization::ImportedType{"Object"};

    storage.synchronize(SynchronizationPackage{importsSourceId4, {package.types[3]}, {sourceId4}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(sourceId4,
                                             "QObject2",
                                             fetchTypeId(sourceId2, "QObject"),
                                             TypeTraits::Reference),
                               Field(&Storage::Synchronization::Type::exportedTypes,
                                     UnorderedElementsAre(IsExportedType("Object2"),
                                                          IsExportedType("Obj2"))))));
}

TEST_F(ProjectStorage, fetch_by_major_version_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::ImportedType{"Object"},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{1}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(
                    sourceId2, "Item", fetchTypeId(sourceId1, "QObject"), TypeTraits::Reference)));
}

TEST_F(ProjectStorage, fetch_by_major_version_for_qualified_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{1}, sourceId2};
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::QualifiedImportedType{"Object", import},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(
                    sourceId2, "Item", fetchTypeId(sourceId1, "QObject"), TypeTraits::Reference)));
}

TEST_F(ProjectStorage, fetch_by_major_version_and_minor_version_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::ImportedType{"Obj"},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{1, 2}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(
                    sourceId2, "Item", fetchTypeId(sourceId1, "QObject"), TypeTraits::Reference)));
}

TEST_F(ProjectStorage, fetch_by_major_version_and_minor_version_for_qualified_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{1, 2}, sourceId2};
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::QualifiedImportedType{"Obj", import},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(
                    sourceId2, "Item", fetchTypeId(sourceId1, "QObject"), TypeTraits::Reference)));
}

TEST_F(ProjectStorage,
       fetch_by_major_version_and_minor_version_for_imported_type_if_minor_version_is_not_exported_throws)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::ImportedType{"Object"},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{1, 1}, sourceId2};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage,
       fetch_by_major_version_and_minor_version_for_qualified_imported_type_if_minor_version_is_not_exported_throws)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{1, 1}, sourceId2};
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::QualifiedImportedType{"Object", import},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, fetch_low_minor_version_for_imported_type_throws)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::ImportedType{"Obj"},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{1, 1}, sourceId2};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, fetch_low_minor_version_for_qualified_imported_type_throws)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{1, 1}, sourceId2};
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::QualifiedImportedType{"Obj", import},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, fetch_higher_minor_version_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::ImportedType{"Obj"},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{1, 3}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(
                    sourceId2, "Item", fetchTypeId(sourceId1, "QObject"), TypeTraits::Reference)));
}

TEST_F(ProjectStorage, fetch_higher_minor_version_for_qualified_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{1, 3}, sourceId2};
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::QualifiedImportedType{"Obj", import},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(
                    sourceId2, "Item", fetchTypeId(sourceId1, "QObject"), TypeTraits::Reference)));
}

TEST_F(ProjectStorage, fetch_different_major_version_for_imported_type_throws)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::ImportedType{"Obj"},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{3, 1}, sourceId2};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, fetch_different_major_version_for_qualified_imported_type_throws)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{3, 1}, sourceId2};
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::QualifiedImportedType{"Obj", import},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, fetch_other_type_by_different_version_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::ImportedType{"Obj"},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{2, 3}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(
                    sourceId2, "Item", fetchTypeId(sourceId1, "QObject2"), TypeTraits::Reference)));
}

TEST_F(ProjectStorage, fetch_other_type_by_different_version_for_qualified_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{2, 3}, sourceId2};
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::QualifiedImportedType{"Obj", import},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(
                    sourceId2, "Item", fetchTypeId(sourceId1, "QObject2"), TypeTraits::Reference)));
}

TEST_F(ProjectStorage, fetch_highest_version_for_import_without_version_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::ImportedType{"Obj"},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(
                    sourceId2, "Item", fetchTypeId(sourceId1, "QObject4"), TypeTraits::Reference)));
}

TEST_F(ProjectStorage, fetch_highest_version_for_import_without_version_for_qualified_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{}, sourceId2};
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::QualifiedImportedType{"Obj", import},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(
                    sourceId2, "Item", fetchTypeId(sourceId1, "QObject4"), TypeTraits::Reference)));
}

TEST_F(ProjectStorage, fetch_highest_version_for_import_with_major_version_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::ImportedType{"Obj"},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{2}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(
                    sourceId2, "Item", fetchTypeId(sourceId1, "QObject3"), TypeTraits::Reference)));
}

TEST_F(ProjectStorage, fetch_highest_version_for_import_with_major_version_for_qualified_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{2}, sourceId2};
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::QualifiedImportedType{"Obj", import},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(
                    sourceId2, "Item", fetchTypeId(sourceId1, "QObject3"), TypeTraits::Reference)));
}

TEST_F(ProjectStorage, fetch_exported_type_without_version_first_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::ImportedType{"BuiltInObj"},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(
                    sourceId2, "Item", fetchTypeId(sourceId1, "QObject"), TypeTraits::Reference)));
}

TEST_F(ProjectStorage, fetch_exported_type_without_version_first_for_qualified_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{}, sourceId2};
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::QualifiedImportedType{"BuiltInObj", import},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(
                    sourceId2, "Item", fetchTypeId(sourceId1, "QObject"), TypeTraits::Reference)));
}

TEST_F(ProjectStorage, ensure_that_properties_for_removed_types_are_not_anymore_relinked)
{
    Storage::Synchronization::Type type{
        "QObject",
        Storage::Synchronization::ImportedType{""},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId1,
        {Storage::Synchronization::ExportedType{qmlModuleId,
                                                "Object",
                                                Storage::Version{}}},
        {Storage::Synchronization::PropertyDeclaration{"data",
                                                       Storage::Synchronization::ImportedType{
                                                           "Object"},
                                                       Storage::PropertyDeclarationTraits::IsList}}};
    Storage::Import import{qmlModuleId, Storage::Version{}, sourceId1};
    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId1}});

    ASSERT_NO_THROW(storage.synchronize(SynchronizationPackage{{sourceId1}}));
}

TEST_F(ProjectStorage, ensure_that_prototypes_for_removed_types_are_not_anymore_relinked)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    ASSERT_NO_THROW(storage.synchronize(SynchronizationPackage{{sourceId1, sourceId2}}));
}

TEST_F(ProjectStorage, ensure_that_extensions_for_removed_types_are_not_anymore_relinked)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    storage.synchronize(package);

    ASSERT_NO_THROW(storage.synchronize(SynchronizationPackage{{sourceId1, sourceId2}}));
}

TEST_F(ProjectStorage, minimal_updates)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    Storage::Synchronization::Type quickType{
        "QQuickItem",
        {},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId1,
        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                "Item",
                                                Storage::Version{2, 0}},
         Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QQuickItem"}},
        {},
        {},
        {},
        {},
        Storage::Synchronization::ChangeLevel::Minimal};

    storage.synchronize(SynchronizationPackage{{quickType}});

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraits::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                fetchTypeId(sourceId2, "QObject"),
                                TypeTraits::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item", 2, 0),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickItem"))),
                  Field(&Storage::Synchronization::Type::propertyDeclarations, Not(IsEmpty())),
                  Field(&Storage::Synchronization::Type::functionDeclarations, Not(IsEmpty())),
                  Field(&Storage::Synchronization::Type::signalDeclarations, Not(IsEmpty())),
                  Field(&Storage::Synchronization::Type::enumerationDeclarations, Not(IsEmpty())))));
}

TEST_F(ProjectStorage, get_module_id)
{
    auto id = storage.moduleId("Qml");

    ASSERT_TRUE(id);
}

TEST_F(ProjectStorage, get_same_module_id_again)
{
    auto initialId = storage.moduleId("Qml");

    auto id = storage.moduleId("Qml");

    ASSERT_THAT(id, Eq(initialId));
}

TEST_F(ProjectStorage, module_name_throws_if_id_is_invalid)
{
    ASSERT_THROW(storage.moduleName(ModuleId{}), QmlDesigner::ModuleDoesNotExists);
}

TEST_F(ProjectStorage, module_name_throws_if_id_does_not_exists)
{
    ASSERT_THROW(storage.moduleName(ModuleId::create(222)), QmlDesigner::ModuleDoesNotExists);
}

TEST_F(ProjectStorage, get_module_name)
{
    auto id = storage.moduleId("Qml");

    auto name = storage.moduleName(id);

    ASSERT_THAT(name, Eq("Qml"));
}

TEST_F(ProjectStorage, populate_module_cache)
{
    auto id = storage.moduleId("Qml");

    QmlDesigner::ProjectStorage<Sqlite::Database> newStorage{database, database.isInitialized()};

    ASSERT_THAT(newStorage.moduleName(id), Eq("Qml"));
}

TEST_F(ProjectStorage, add_project_dataes)
{
    Storage::Synchronization::ProjectData projectData1{qmlProjectSourceId,
                                                       sourceId1,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectData projectData2{qmlProjectSourceId,
                                                       sourceId2,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectData projectData3{qtQuickProjectSourceId,
                                                       sourceId3,
                                                       qtQuickModuleId,
                                                       Storage::Synchronization::FileType::QmlTypes};

    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {projectData1, projectData2, projectData3}});

    ASSERT_THAT(storage.fetchProjectDatas({qmlProjectSourceId, qtQuickProjectSourceId}),
                UnorderedElementsAre(projectData1, projectData2, projectData3));
}

TEST_F(ProjectStorage, remove_project_data)
{
    Storage::Synchronization::ProjectData projectData1{qmlProjectSourceId,
                                                       sourceId1,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectData projectData2{qmlProjectSourceId,
                                                       sourceId2,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectData projectData3{qtQuickProjectSourceId,
                                                       sourceId3,
                                                       qtQuickModuleId,
                                                       Storage::Synchronization::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {projectData1, projectData2, projectData3}});

    storage.synchronize(
        SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId}, {projectData1}});

    ASSERT_THAT(storage.fetchProjectDatas({qmlProjectSourceId, qtQuickProjectSourceId}),
                UnorderedElementsAre(projectData1));
}

TEST_F(ProjectStorage, update_project_data_file_type)
{
    Storage::Synchronization::ProjectData projectData1{qmlProjectSourceId,
                                                       sourceId1,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectData projectData2{qmlProjectSourceId,
                                                       sourceId2,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectData projectData2b{qmlProjectSourceId,
                                                        sourceId2,
                                                        qmlModuleId,
                                                        Storage::Synchronization::FileType::QmlTypes};
    Storage::Synchronization::ProjectData projectData3{qtQuickProjectSourceId,
                                                       sourceId3,
                                                       qtQuickModuleId,
                                                       Storage::Synchronization::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {projectData1, projectData2, projectData3}});

    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {projectData1, projectData2b}});

    ASSERT_THAT(storage.fetchProjectDatas({qmlProjectSourceId, qtQuickProjectSourceId}),
                UnorderedElementsAre(projectData1, projectData2b, projectData3));
}

TEST_F(ProjectStorage, update_project_data_module_id)
{
    Storage::Synchronization::ProjectData projectData1{qmlProjectSourceId,
                                                       sourceId1,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectData projectData2{qmlProjectSourceId,
                                                       sourceId3,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectData projectData2b{qmlProjectSourceId,
                                                        sourceId3,
                                                        qtQuickModuleId,
                                                        Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectData projectData3{qtQuickProjectSourceId,
                                                       sourceId2,
                                                       qtQuickModuleId,
                                                       Storage::Synchronization::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {projectData1, projectData2, projectData3}});

    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {projectData1, projectData2b}});

    ASSERT_THAT(storage.fetchProjectDatas({qmlProjectSourceId, qtQuickProjectSourceId}),
                UnorderedElementsAre(projectData1, projectData2b, projectData3));
}

TEST_F(ProjectStorage, throw_for_invalid_source_id_in_project_data)
{
    Storage::Synchronization::ProjectData projectData1{qmlProjectSourceId,
                                                       SourceId{},
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {projectData1}}),
                 QmlDesigner::ProjectDataHasInvalidSourceId);
}

TEST_F(ProjectStorage, insert_project_data_with_invalid_module_id)
{
    Storage::Synchronization::ProjectData projectData1{qmlProjectSourceId,
                                                       sourceId1,
                                                       ModuleId{},
                                                       Storage::Synchronization::FileType::QmlDocument};

    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {projectData1}});

    ASSERT_THAT(storage.fetchProjectDatas({qmlProjectSourceId, qtQuickProjectSourceId}),
                UnorderedElementsAre(projectData1));
}

TEST_F(ProjectStorage, update_project_data_with_invalid_module_id)
{
    Storage::Synchronization::ProjectData projectData1{qmlProjectSourceId,
                                                       sourceId1,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {projectData1}});
    projectData1.moduleId = ModuleId{};

    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {projectData1}});

    ASSERT_THAT(storage.fetchProjectDatas({qmlProjectSourceId, qtQuickProjectSourceId}),
                UnorderedElementsAre(projectData1));
}

TEST_F(ProjectStorage, throw_for_updating_with_invalid_project_source_id_in_project_data)
{
    Storage::Synchronization::ProjectData projectData1{SourceId{},
                                                       sourceId1,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {projectData1}}),
                 QmlDesigner::ProjectDataHasInvalidProjectSourceId);
}

TEST_F(ProjectStorage, fetch_project_datas_by_directory_source_ids)
{
    Storage::Synchronization::ProjectData projectData1{qmlProjectSourceId,
                                                       sourceId1,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectData projectData2{qmlProjectSourceId,
                                                       sourceId2,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectData projectData3{qtQuickProjectSourceId,
                                                       sourceId3,
                                                       qtQuickModuleId,
                                                       Storage::Synchronization::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {projectData1, projectData2, projectData3}});

    auto projectDatas = storage.fetchProjectDatas({qmlProjectSourceId, qtQuickProjectSourceId});

    ASSERT_THAT(projectDatas, UnorderedElementsAre(projectData1, projectData2, projectData3));
}

TEST_F(ProjectStorage, fetch_project_datas_by_directory_source_id)
{
    Storage::Synchronization::ProjectData projectData1{qmlProjectSourceId,
                                                       sourceId1,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectData projectData2{qmlProjectSourceId,
                                                       sourceId2,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectData projectData3{qtQuickProjectSourceId,
                                                       sourceId3,
                                                       qtQuickModuleId,
                                                       Storage::Synchronization::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {projectData1, projectData2, projectData3}});

    auto projectData = storage.fetchProjectDatas(qmlProjectSourceId);

    ASSERT_THAT(projectData, UnorderedElementsAre(projectData1, projectData2));
}

TEST_F(ProjectStorage, fetch_project_data_by_source_ids)
{
    Storage::Synchronization::ProjectData projectData1{qmlProjectSourceId,
                                                       sourceId1,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectData projectData2{qmlProjectSourceId,
                                                       sourceId2,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectData projectData3{qtQuickProjectSourceId,
                                                       sourceId3,
                                                       qtQuickModuleId,
                                                       Storage::Synchronization::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {projectData1, projectData2, projectData3}});

    auto projectData = storage.fetchProjectData({sourceId2});

    ASSERT_THAT(projectData, Eq(projectData2));
}

TEST_F(ProjectStorage, exclude_exported_types)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].exportedTypes.clear();
    package.types[0].changeLevel = Storage::Synchronization::ChangeLevel::ExcludeExportedTypes;

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                                     IsExportedType(qtQuickNativeModuleId,
                                                                    "QQuickItem"))))));
}

TEST_F(ProjectStorage, module_exported_import)
{
    auto package{createModuleExportedImportSynchronizationPackage()};

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item")))),
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object")))),
                    AllOf(IsStorageType(sourceId3,
                                        "QQuickItem3d",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuick3DModuleId, "Item3D")))),
                    AllOf(IsStorageType(sourceId4,
                                        "MyItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(myModuleModuleId, "MyItem"))))));
}

TEST_F(ProjectStorage, module_exported_import_deletes_indirect_imports_too)
{
    auto package{createModuleExportedImportSynchronizationPackage()};
    storage.synchronize(package);
    SynchronizationPackage packageWhichDeletesImports;
    packageWhichDeletesImports.updatedSourceIds = package.updatedSourceIds;
    storage.synchronize(packageWhichDeletesImports);

    auto imports = storage.fetchDocumentImports();

    ASSERT_THAT(imports, IsEmpty());
}

TEST_F(ProjectStorage, module_exported_import_with_different_versions)
{
    auto package{createModuleExportedImportSynchronizationPackage()};
    package.imports.back().version.major.value = 2;
    package.types[2].exportedTypes.front().version.major.value = 2;
    package.moduleExportedImports.back().isAutoVersion = Storage::Synchronization::IsAutoVersion::No;
    package.moduleExportedImports.back().version = Storage::Version{1};

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item")))),
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object")))),
                    AllOf(IsStorageType(sourceId3,
                                        "QQuickItem3d",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuick3DModuleId, "Item3D")))),
                    AllOf(IsStorageType(sourceId4,
                                        "MyItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(myModuleModuleId, "MyItem"))))));
}

TEST_F(ProjectStorage, module_exported_import_with_indirect_different_versions)
{
    auto package{createModuleExportedImportSynchronizationPackage()};
    package.imports[1].version.major.value = 2;
    package.imports.back().version.major.value = 2;
    package.types[0].exportedTypes.front().version.major.value = 2;
    package.types[2].exportedTypes.front().version.major.value = 2;
    package.moduleExportedImports[0].isAutoVersion = Storage::Synchronization::IsAutoVersion::No;
    package.moduleExportedImports[0].version = Storage::Version{1};

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item")))),
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object")))),
                    AllOf(IsStorageType(sourceId3,
                                        "QQuickItem3d",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuick3DModuleId, "Item3D")))),
                    AllOf(IsStorageType(sourceId4,
                                        "MyItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(myModuleModuleId, "MyItem"))))));
}

TEST_F(ProjectStorage, module_exported_import_prevent_collision_if_module_is_indirectly_reexported_multiple_times)
{
    ModuleId qtQuick4DModuleId{storage.moduleId("QtQuick4D")};
    auto package{createModuleExportedImportSynchronizationPackage()};
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{1}, sourceId5);
    package.moduleExportedImports.emplace_back(qtQuick4DModuleId,
                                               qtQuickModuleId,
                                               Storage::Version{},
                                               Storage::Synchronization::IsAutoVersion::Yes);
    package.moduleExportedImports.emplace_back(qtQuick4DModuleId,
                                               qmlModuleId,
                                               Storage::Version{},
                                               Storage::Synchronization::IsAutoVersion::Yes);
    package.updatedModuleIds.push_back(qtQuick4DModuleId);
    package.types.push_back(Storage::Synchronization::Type{
        "QQuickItem4d",
        Storage::Synchronization::ImportedType{"Item"},
        Storage::Synchronization::ImportedType{},
        TypeTraits::Reference,
        sourceId5,
        {Storage::Synchronization::ExportedType{qtQuick4DModuleId,
                                                "Item4D",
                                                Storage::Version{1, 0}}}});
    package.imports.emplace_back(qtQuick4DModuleId, Storage::Version{1}, sourceId4);

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item")))),
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object")))),
                    AllOf(IsStorageType(sourceId3,
                                        "QQuickItem3d",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuick3DModuleId, "Item3D")))),
                    AllOf(IsStorageType(sourceId5,
                                        "QQuickItem4d",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuick4DModuleId, "Item4D")))),
                    AllOf(IsStorageType(sourceId4,
                                        "MyItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(myModuleModuleId, "MyItem"))))));
}

TEST_F(ProjectStorage, distinguish_between_import_kinds)
{
    ModuleId qml1ModuleId{storage.moduleId("Qml1")};
    ModuleId qml11ModuleId{storage.moduleId("Qml11")};
    auto package{createSimpleSynchronizationPackage()};
    package.moduleDependencies.emplace_back(qmlModuleId, Storage::Version{}, sourceId1);
    package.moduleDependencies.emplace_back(qml1ModuleId,
                                            Storage::Version{1},
                                            sourceId1);
    package.imports.emplace_back(qml1ModuleId, Storage::Version{}, sourceId1);
    package.moduleDependencies.emplace_back(qml11ModuleId,
                                            Storage::Version{1, 1},
                                            sourceId1);
    package.imports.emplace_back(qml11ModuleId, Storage::Version{1, 1}, sourceId1);

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                                     IsExportedType(qtQuickNativeModuleId,
                                                                    "QQuickItem"))))));
}

TEST_F(ProjectStorage, module_exported_import_distinguish_between_dependency_and_import_re_exports)
{
    auto package{createModuleExportedImportSynchronizationPackage()};
    package.moduleDependencies.emplace_back(qtQuick3DModuleId,
                                            Storage::Version{1},
                                            sourceId4);

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item")))),
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object")))),
                    AllOf(IsStorageType(sourceId3,
                                        "QQuickItem3d",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuick3DModuleId, "Item3D")))),
                    AllOf(IsStorageType(sourceId4,
                                        "MyItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(myModuleModuleId, "MyItem"))))));
}

TEST_F(ProjectStorage, module_exported_import_with_qualified_imported_type)
{
    auto package{createModuleExportedImportSynchronizationPackage()};
    package.types.back().prototype = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qtQuick3DModuleId, Storage::Version{1}, sourceId4}};

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item")))),
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object")))),
                    AllOf(IsStorageType(sourceId3,
                                        "QQuickItem3d",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuick3DModuleId, "Item3D")))),
                    AllOf(IsStorageType(sourceId4,
                                        "MyItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(myModuleModuleId, "MyItem"))))));
}

TEST_F(ProjectStorage, synchronize_types_add_indirect_alias_declarations)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};

    storage.synchronize(package);

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId3, "QAliasItem", fetchTypeId(sourceId1, "QQuickItem"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
                  UnorderedElementsAre(
                      IsPropertyDeclaration("items",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsList),
                      IsPropertyDeclaration("objects",
                                            fetchTypeId(sourceId2, "QObject"),
                                            Storage::PropertyDeclarationTraits::IsList
                                                | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, synchronize_types_add_indirect_alias_declarations_again)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);

    storage.synchronize(package);

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId3, "QAliasItem", fetchTypeId(sourceId1, "QQuickItem"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
                  UnorderedElementsAre(
                      IsPropertyDeclaration("items",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsList),
                      IsPropertyDeclaration("objects",
                                            fetchTypeId(sourceId2, "QObject"),
                                            Storage::PropertyDeclarationTraits::IsList
                                                | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, synchronize_types_remove_indirect_alias_declaration)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations.pop_back();

    storage.synchronize(SynchronizationPackage{importsSourceId3, {package.types[2]}, {sourceId3}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(sourceId3,
                                             "QAliasItem",
                                             fetchTypeId(sourceId1, "QQuickItem"),
                                             TypeTraits::Reference),
                               Field(&Storage::Synchronization::Type::propertyDeclarations,
                                     UnorderedElementsAre(IsPropertyDeclaration(
                                         "items",
                                         fetchTypeId(sourceId1, "QQuickItem"),
                                         Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, synchronize_types_add_indirect_alias_declarations_throws_for_wrong_type_name)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "QQuickItemWrong"};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{importsSourceId3,
                                                            {package.types[2]},
                                                            {sourceId3},
                                                            moduleDependenciesSourceId3,
                                                            {sourceId3}}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorage, synchronize_types_add_indirect_alias_declarations_throws_for_wrong_property_name)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].aliasPropertyName = "childrenWrong";

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{importsSourceId3,
                                                            {package.types[2]},
                                                            {sourceId3},
                                                            moduleDependenciesSourceId3,
                                                            {sourceId3}}),
                 QmlDesigner::PropertyNameDoesNotExists);
}

TEST_F(ProjectStorage, synchronize_types_add_indirect_alias_declarations_throws_for_wrong_property_name_tail)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].aliasPropertyNameTail = "objectsWrong";

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{importsSourceId3,
                                                            {package.types[2]},
                                                            {sourceId3},
                                                            moduleDependenciesSourceId3,
                                                            {sourceId3}}),
                 QmlDesigner::PropertyNameDoesNotExists);
}

TEST_F(ProjectStorage, synchronize_types_change_indirect_alias_declaration_type_name)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "Obj2"};
    importsSourceId3.emplace_back(pathToModuleId, Storage::Version{}, sourceId3);

    storage.synchronize(SynchronizationPackage{
        importsSourceId3, {package.types[2]}, {sourceId3}, moduleDependenciesSourceId3, {sourceId3}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId3, "QAliasItem", fetchTypeId(sourceId1, "QQuickItem"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
                  UnorderedElementsAre(
                      IsPropertyDeclaration("items",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsList),
                      IsPropertyDeclaration("objects",
                                            fetchTypeId(sourceId4, "QObject2"),
                                            Storage::PropertyDeclarationTraits::IsList
                                                | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, synchronize_types_change_indirect_alias_declaration_tails_type_name)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[4].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "Obj2"};
    importsSourceId5.emplace_back(pathToModuleId, Storage::Version{}, sourceId5);

    storage.synchronize(SynchronizationPackage{
        importsSourceId5, {package.types[4]}, {sourceId5}, moduleDependenciesSourceId5, {sourceId5}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId3, "QAliasItem", fetchTypeId(sourceId1, "QQuickItem"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
                  UnorderedElementsAre(
                      IsPropertyDeclaration("items",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsList),
                      IsPropertyDeclaration("objects",
                                            fetchTypeId(sourceId4, "QObject2"),
                                            Storage::PropertyDeclarationTraits::IsList
                                                | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, synchronize_types_change_indirect_alias_declarations_property_name)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].aliasPropertyName = "kids";

    storage.synchronize(SynchronizationPackage{
        importsSourceId3, {package.types[2]}, {sourceId3}, moduleDependenciesSourceId3, {sourceId3}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId3, "QAliasItem", fetchTypeId(sourceId1, "QQuickItem"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
                  UnorderedElementsAre(
                      IsPropertyDeclaration("items",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsList),
                      IsPropertyDeclaration("objects",
                                            fetchTypeId(sourceId4, "QObject2"),
                                            Storage::PropertyDeclarationTraits::IsList
                                                | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, synchronize_types_change_indirect_alias_declarations_property_name_tail)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].aliasPropertyNameTail = "items";

    storage.synchronize(SynchronizationPackage{
        importsSourceId3, {package.types[2]}, {sourceId3}, moduleDependenciesSourceId3, {sourceId3}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId1, "QQuickItem"),
                                  TypeTraits::Reference),
                    Field(&Storage::Synchronization::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    fetchTypeId(sourceId1, "QQuickItem"),
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("objects",
                                                    fetchTypeId(sourceId1, "QQuickItem"),
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, synchronize_types_change_indirect_alias_declarations_to_property_declaration)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations.pop_back();
    package.types[2].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects",
        Storage::Synchronization::ImportedType{"QQuickItem"},
        Storage::PropertyDeclarationTraits::IsList | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize(SynchronizationPackage{
        importsSourceId3, {package.types[2]}, {sourceId3}, moduleDependenciesSourceId3, {sourceId3}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId3, "QAliasItem", fetchTypeId(sourceId1, "QQuickItem"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
                  UnorderedElementsAre(
                      IsPropertyDeclaration("items",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsList),
                      IsPropertyDeclaration("objects",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsList
                                                | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, synchronize_types_change_property_declarations_to_indirect_alias_declaration)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    auto packageChanged = package;
    packageChanged.types[2].propertyDeclarations.pop_back();
    packageChanged.types[2].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects",
        Storage::Synchronization::ImportedType{"QQuickItem"},
        Storage::PropertyDeclarationTraits::IsList | Storage::PropertyDeclarationTraits::IsReadOnly});
    storage.synchronize(SynchronizationPackage{importsSourceId3,
                                               {packageChanged.types[2]},
                                               {sourceId3},
                                               moduleDependenciesSourceId3,
                                               {sourceId3}});

    storage.synchronize(SynchronizationPackage{
        importsSourceId3, {package.types[2]}, {sourceId3}, moduleDependenciesSourceId3, {sourceId3}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId3, "QAliasItem", fetchTypeId(sourceId1, "QQuickItem"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
                  UnorderedElementsAre(
                      IsPropertyDeclaration("items",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsList),
                      IsPropertyDeclaration("objects",
                                            fetchTypeId(sourceId2, "QObject"),
                                            Storage::PropertyDeclarationTraits::IsList
                                                | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, synchronize_types_change_indirect_alias_target_property_declaration_traits)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[4].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly;

    storage.synchronize(SynchronizationPackage{
        importsSourceId5, {package.types[4]}, {sourceId5}, moduleDependenciesSourceId5, {sourceId5}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId3, "QAliasItem", fetchTypeId(sourceId1, "QQuickItem"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
                  UnorderedElementsAre(
                      IsPropertyDeclaration("items",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsList
                                                | Storage::PropertyDeclarationTraits::IsReadOnly),
                      IsPropertyDeclaration("objects",
                                            fetchTypeId(sourceId2, "QObject"),
                                            Storage::PropertyDeclarationTraits::IsList
                                                | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, synchronize_types_change_indirect_alias_target_property_declaration_type_name)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[4].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "Item"};

    storage.synchronize(SynchronizationPackage{
        importsSourceId5, {package.types[4]}, {sourceId5}, moduleDependenciesSourceId5, {sourceId5}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId3, "QAliasItem", fetchTypeId(sourceId1, "QQuickItem"), TypeTraits::Reference),
            Field(&Storage::Synchronization::Type::propertyDeclarations,
                  UnorderedElementsAre(
                      IsPropertyDeclaration("items",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsList),
                      IsPropertyDeclaration("objects",
                                            fetchTypeId(sourceId1, "QQuickItem"),
                                            Storage::PropertyDeclarationTraits::IsList
                                                | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, synchronize_types_remove_property_declaration_with_an_indirect_alias_throws)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[4].propertyDeclarations.pop_back();

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{importsSourceId5,
                                                            {package.types[4]},
                                                            {sourceId5},
                                                            moduleDependenciesSourceId5,
                                                            {sourceId5}}),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorage, DISABLED_synchronize_types_remove_stem_property_declaration_with_an_indirect_alias_throws)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations.erase(package.types[0].propertyDeclarations.begin());

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{importsSourceId1,
                                                            {package.types[0]},
                                                            {sourceId1},
                                                            moduleDependenciesSourceId1,
                                                            {sourceId1}}),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorage, synchronize_types_remove_property_declaration_and_indirect_alias)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations.pop_back();
    package.types[4].propertyDeclarations.pop_back();

    storage.synchronize(SynchronizationPackage{importsSourceId3 + importsSourceId5,
                                               {package.types[2], package.types[4]},
                                               {sourceId3, sourceId5},
                                               moduleDependenciesSourceId3 + moduleDependenciesSourceId5,
                                               {sourceId3, sourceId5}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(sourceId3,
                                             "QAliasItem",
                                             fetchTypeId(sourceId1, "QQuickItem"),
                                             TypeTraits::Reference),
                               Field(&Storage::Synchronization::Type::propertyDeclarations,
                                     UnorderedElementsAre(IsPropertyDeclaration(
                                         "items",
                                         fetchTypeId(sourceId1, "QQuickItem"),
                                         Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage, synchronize_types_remove_property_declaration_and_indirect_alias_steam)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations.clear();
    package.types[2].propertyDeclarations.clear();

    storage.synchronize(SynchronizationPackage{importsSourceId1 + importsSourceId3,
                                               {package.types[0], package.types[2]},
                                               {sourceId1, sourceId3},
                                               moduleDependenciesSourceId1 + moduleDependenciesSourceId3,
                                               {sourceId1, sourceId3}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId3,
                                        "QAliasItem",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraits::Reference),
                          Field(&Storage::Synchronization::Type::propertyDeclarations, IsEmpty()))));
}

TEST_F(ProjectStorage, get_type_id)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);

    auto typeId = storage.typeId(qmlModuleId, "Object", Storage::Version{});

    ASSERT_THAT(typeId, fetchTypeId(sourceId1, "QObject4"));
}

TEST_F(ProjectStorage, get_no_type_id_for_non_existing_type_name)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);

    auto typeId = storage.typeId(qmlModuleId, "Object2", Storage::Version{});

    ASSERT_FALSE(typeId);
}

TEST_F(ProjectStorage, get_no_type_id_for_invalid_module_id)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);

    auto typeId = storage.typeId(ModuleId{}, "Object", Storage::Version{});

    ASSERT_FALSE(typeId);
}

TEST_F(ProjectStorage, get_no_type_id_for_wrong_module_id)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);

    auto typeId = storage.typeId(qtQuick3DModuleId, "Object", Storage::Version{});

    ASSERT_FALSE(typeId);
}

TEST_F(ProjectStorage, get_type_id_with_major_version)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);

    auto typeId = storage.typeId(qmlModuleId, "Object", Storage::Version{2});

    ASSERT_THAT(typeId, fetchTypeId(sourceId1, "QObject3"));
}

TEST_F(ProjectStorage, get_no_type_id_with_major_version_for_non_existing_type_name)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);

    auto typeId = storage.typeId(qmlModuleId, "Object2", Storage::Version{2});

    ASSERT_FALSE(typeId);
}

TEST_F(ProjectStorage, get_no_type_id_with_major_version_for_invalid_module_id)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);

    auto typeId = storage.typeId(ModuleId{}, "Object", Storage::Version{2});

    ASSERT_FALSE(typeId);
}

TEST_F(ProjectStorage, get_no_type_id_with_major_version_for_wrong_module_id)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);

    auto typeId = storage.typeId(qtQuick3DModuleId, "Object", Storage::Version{2});

    ASSERT_FALSE(typeId);
}

TEST_F(ProjectStorage, get_no_type_id_with_major_version_for_wrong_version)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);

    auto typeId = storage.typeId(qmlModuleId, "Object", Storage::Version{4});

    ASSERT_FALSE(typeId);
}

TEST_F(ProjectStorage, get_type_id_with_complete_version)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);

    auto typeId = storage.typeId(qmlModuleId, "Object", Storage::Version{2, 0});

    ASSERT_THAT(typeId, fetchTypeId(sourceId1, "QObject2"));
}

TEST_F(ProjectStorage, get_no_type_id_with_complete_version_with_higher_minor_version)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);

    auto typeId = storage.typeId(qmlModuleId, "Object", Storage::Version{2, 12});

    ASSERT_THAT(typeId, fetchTypeId(sourceId1, "QObject3"));
}

TEST_F(ProjectStorage, get_no_type_id_with_complete_version_for_non_existing_type_name)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);

    auto typeId = storage.typeId(qmlModuleId, "Object2", Storage::Version{2, 0});

    ASSERT_FALSE(typeId);
}

TEST_F(ProjectStorage, get_no_type_id_with_complete_version_for_invalid_module_id)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);

    auto typeId = storage.typeId(ModuleId{}, "Object", Storage::Version{2, 0});

    ASSERT_FALSE(typeId);
}

TEST_F(ProjectStorage, get_no_type_id_with_complete_version_for_wrong_module_id)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);

    auto typeId = storage.typeId(qtQuick3DModuleId, "Object", Storage::Version{2, 0});

    ASSERT_FALSE(typeId);
}

TEST_F(ProjectStorage, get_no_type_id_with_complete_version_for_wrong_major_version)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);

    auto typeId = storage.typeId(qmlModuleId, "Object", Storage::Version{4, 0});

    ASSERT_FALSE(typeId);
}

TEST_F(ProjectStorage, get_property_declaration_ids_over_prototype_chain)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");

    auto propertyIds = storage.propertyDeclarationIds(typeId);

    ASSERT_THAT(propertyIds,
                UnorderedElementsAre(HasName("data"),
                                     HasName("children"),
                                     HasName("data2"),
                                     HasName("children2"),
                                     HasName("data3"),
                                     HasName("children3")));
}

TEST_F(ProjectStorage, get_property_declaration_ids_over_extension_chain)
{
    auto package{createPackageWithProperties()};
    std::swap(package.types[1].extension, package.types[1].prototype);
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");

    auto propertyIds = storage.propertyDeclarationIds(typeId);

    ASSERT_THAT(propertyIds,
                UnorderedElementsAre(HasName("data"),
                                     HasName("children"),
                                     HasName("data2"),
                                     HasName("children2"),
                                     HasName("data3"),
                                     HasName("children3")));
}

TEST_F(ProjectStorage, get_property_declaration_ids_are_returned_sorted)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");

    auto propertyIds = storage.propertyDeclarationIds(typeId);

    ASSERT_THAT(propertyIds, IsSorted());
}

TEST_F(ProjectStorage, get_no_property_declaration_ids_properties_from_derived_types)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject2");

    auto propertyIds = storage.propertyDeclarationIds(typeId);

    ASSERT_THAT(propertyIds,
                UnorderedElementsAre(HasName("data"),
                                     HasName("children"),
                                     HasName("data2"),
                                     HasName("children2")));
}

TEST_F(ProjectStorage, get_no_property_declaration_ids_for_wrong_type_id)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "WrongObject");

    auto propertyIds = storage.propertyDeclarationIds(typeId);

    ASSERT_THAT(propertyIds, IsEmpty());
}

TEST_F(ProjectStorage, get_local_property_declaration_ids)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject2");

    auto propertyIds = storage.localPropertyDeclarationIds(typeId);

    ASSERT_THAT(propertyIds, UnorderedElementsAre(HasName("data2"), HasName("children2")));
}

TEST_F(ProjectStorage, get_local_property_declaration_ids_are_returned_sorted)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject2");

    auto propertyIds = storage.localPropertyDeclarationIds(typeId);

    ASSERT_THAT(propertyIds, IsSorted());
}

TEST_F(ProjectStorage, get_property_declaration_id_over_prototype_chain)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");

    auto propertyId = storage.propertyDeclarationId(typeId, "data");

    ASSERT_THAT(propertyId, HasName("data"));
}

TEST_F(ProjectStorage, get_property_declaration_id_over_extension_chain)
{
    auto package{createPackageWithProperties()};
    std::swap(package.types[1].extension, package.types[1].prototype);
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");

    auto propertyId = storage.propertyDeclarationId(typeId, "data");

    ASSERT_THAT(propertyId, HasName("data"));
}

TEST_F(ProjectStorage, get_latest_property_declaration_id)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");
    auto oldPropertyId = storage.propertyDeclarationId(typeId, "data");
    auto &type = findType(package, "QObject3");
    type.propertyDeclarations.push_back(
        Storage::Synchronization::PropertyDeclaration{"data",
                                                      Storage::Synchronization::ImportedType{
                                                          "Object"},
                                                      Storage::PropertyDeclarationTraits::IsList});
    storage.synchronize(package);

    auto propertyId = storage.propertyDeclarationId(typeId, "data");

    ASSERT_THAT(propertyId, AllOf(Not(oldPropertyId), HasName("data")));
    ASSERT_THAT(oldPropertyId, HasName("data"));
}

TEST_F(ProjectStorage, get_invalid_property_declaration_id_for_invalid_type_id)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "WrongObject");

    auto propertyId = storage.propertyDeclarationId(typeId, "data");

    ASSERT_FALSE(propertyId);
}

TEST_F(ProjectStorage, get_invalid_property_declaration_id_for_wrong_property_name)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "Object3");

    auto propertyId = storage.propertyDeclarationId(typeId, "wrongName");

    ASSERT_FALSE(propertyId);
}

TEST_F(ProjectStorage, get_local_property_declaration_id)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject");

    auto propertyId = storage.localPropertyDeclarationId(typeId, "data");

    ASSERT_THAT(propertyId, HasName("data"));
}

TEST_F(ProjectStorage, get_invalid_local_property_declaration_id_for_wrong_type)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject2");

    auto propertyId = storage.localPropertyDeclarationId(typeId, "data");

    ASSERT_FALSE(propertyId);
}

TEST_F(ProjectStorage, get_invalid_local_property_declaration_id_for_invalid_type_id)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "WrongQObject");

    auto propertyId = storage.localPropertyDeclarationId(typeId, "data");

    ASSERT_FALSE(propertyId);
}

TEST_F(ProjectStorage, get_invalid_local_property_declaration_id_for_wrong_property_name)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject");

    auto propertyId = storage.localPropertyDeclarationId(typeId, "wrongName");

    ASSERT_FALSE(propertyId);
}

TEST_F(ProjectStorage, get_property_declaration)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId2 = fetchTypeId(sourceId1, "QObject2");
    auto typeId3 = fetchTypeId(sourceId1, "QObject3");
    auto propertyId = storage.propertyDeclarationId(typeId3, "data2");

    auto property = storage.propertyDeclaration(propertyId);

    ASSERT_THAT(property,
                Optional(IsInfoPropertyDeclaration(
                    typeId2, "data2", Storage::PropertyDeclarationTraits::IsReadOnly, typeId3)));
}

TEST_F(ProjectStorage, get_invalid_optional_property_declaration_for_invalid_property_declaration_id)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);

    auto property = storage.propertyDeclaration(PropertyDeclarationId{});

    ASSERT_THAT(property, Eq(std::nullopt));
}

TEST_F(ProjectStorage, get_signal_declaration_names)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");

    auto signalNames = storage.signalDeclarationNames(typeId);

    ASSERT_THAT(signalNames, ElementsAre("itemsChanged", "objectsChanged", "valuesChanged"));
}

TEST_F(ProjectStorage, get_signal_declaration_names_are_ordered)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");

    auto signalNames = storage.signalDeclarationNames(typeId);

    ASSERT_THAT(signalNames, StringsAreSorted());
}

TEST_F(ProjectStorage, get_no_signal_declaration_names_for_invalid_type_id)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "WrongObject");

    auto signalNames = storage.signalDeclarationNames(typeId);

    ASSERT_THAT(signalNames, IsEmpty());
}

TEST_F(ProjectStorage, get_only_signal_declaration_names_from_up_into_the_prototype_chain)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject2");

    auto signalNames = storage.signalDeclarationNames(typeId);

    ASSERT_THAT(signalNames, ElementsAre("itemsChanged", "valuesChanged"));
}

TEST_F(ProjectStorage, get_only_signal_declaration_names_from_up_into_the_extension_chain)
{
    auto package{createPackageWithProperties()};
    std::swap(package.types[1].extension, package.types[1].prototype);
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject2");

    auto signalNames = storage.signalDeclarationNames(typeId);

    ASSERT_THAT(signalNames, ElementsAre("itemsChanged", "valuesChanged"));
}

TEST_F(ProjectStorage, get_function_declaration_names)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");

    auto functionNames = storage.functionDeclarationNames(typeId);

    ASSERT_THAT(functionNames, ElementsAre("items", "objects", "values"));
}

TEST_F(ProjectStorage, get_function_declaration_names_are_ordered)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");

    auto functionNames = storage.functionDeclarationNames(typeId);

    ASSERT_THAT(functionNames, StringsAreSorted());
}

TEST_F(ProjectStorage, get_no_function_declaration_names_for_invalid_type_id)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "WrongObject");

    auto functionNames = storage.functionDeclarationNames(typeId);

    ASSERT_THAT(functionNames, IsEmpty());
}

TEST_F(ProjectStorage, get_only_function_declaration_names_from_up_into_the_prototype_chain)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject2");

    auto functionNames = storage.functionDeclarationNames(typeId);

    ASSERT_THAT(functionNames, ElementsAre("items", "values"));
}

TEST_F(ProjectStorage, get_only_function_declaration_names_from_up_into_the_extension_chain)
{
    auto package{createPackageWithProperties()};
    std::swap(package.types[1].extension, package.types[1].prototype);
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject2");

    auto functionNames = storage.functionDeclarationNames(typeId);

    ASSERT_THAT(functionNames, ElementsAre("items", "values"));
}

TEST_F(ProjectStorage, synchronize_default_property)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.front().defaultPropertyName = "children";

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(Field(&Storage::Synchronization::Type::typeName, Eq("QQuickItem")),
                               Field(&Storage::Synchronization::Type::defaultPropertyName,
                                     Eq("children")))));
}

TEST_F(ProjectStorage, synchronize_default_property_to_a_different_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.front().defaultPropertyName = "children";
    storage.synchronize(package);
    package.types.front().defaultPropertyName = "data";

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(Field(&Storage::Synchronization::Type::typeName, Eq("QQuickItem")),
                          Field(&Storage::Synchronization::Type::defaultPropertyName, Eq("data")))));
}

TEST_F(ProjectStorage, synchronize_to_removed_default_property)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.front().defaultPropertyName = "children";
    storage.synchronize(package);
    package.types.front().defaultPropertyName = {};

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(Field(&Storage::Synchronization::Type::typeName, Eq("QQuickItem")),
                          Field(&Storage::Synchronization::Type::defaultPropertyName, IsEmpty()))));
}

TEST_F(ProjectStorage, synchronize_default_property_throws_for_missing_default_property)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.front().defaultPropertyName = "child";

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::PropertyNameDoesNotExists);
}

TEST_F(ProjectStorage,
       synchronize_default_property_throws_for_removing_property_without_changing_default_property)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.front().defaultPropertyName = "children";
    storage.synchronize(package);
    removeProperty(package, "QQuickItem", "children");

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::PropertyNameDoesNotExists);
}

TEST_F(ProjectStorage, synchronize_changes_default_property_and_removes_old_default_property)
{
    auto package{createSimpleSynchronizationPackage()};
    auto &type = findType(package, "QQuickItem");
    type.defaultPropertyName = "children";
    storage.synchronize(package);
    removeProperty(package, "QQuickItem", "children");
    type.defaultPropertyName = "data";

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(Field(&Storage::Synchronization::Type::typeName, Eq("QQuickItem")),
                          Field(&Storage::Synchronization::Type::defaultPropertyName, Eq("data")))));
}

TEST_F(ProjectStorage, synchronize_add_new_default_property_and_removes_old_default_property)
{
    auto package{createSimpleSynchronizationPackage()};
    auto &type = findType(package, "QQuickItem");
    type.defaultPropertyName = "children";
    storage.synchronize(package);
    removeProperty(package, "QQuickItem", "children");
    type.defaultPropertyName = "data2";
    type.propertyDeclarations.push_back(
        Storage::Synchronization::PropertyDeclaration{"data2",
                                                      Storage::Synchronization::ImportedType{
                                                          "QObject"},
                                                      Storage::PropertyDeclarationTraits::IsList});

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(Field(&Storage::Synchronization::Type::typeName, Eq("QQuickItem")),
                          Field(&Storage::Synchronization::Type::defaultPropertyName, Eq("data2")))));
}

TEST_F(ProjectStorage, synchronize_default_property_to_the_prototype_property)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.front().defaultPropertyName = "children";
    removeProperty(package, "QQuickItem", "children");
    auto &type = findType(package, "QObject");
    type.propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "children", Storage::Synchronization::ImportedType{"Object"}, {}});

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(Field(&Storage::Synchronization::Type::typeName, Eq("QQuickItem")),
                               Field(&Storage::Synchronization::Type::defaultPropertyName,
                                     Eq("children")))));
}

TEST_F(ProjectStorage, synchronize_default_property_to_the_extension_property)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types.front().defaultPropertyName = "children";
    removeProperty(package, "QQuickItem", "children");
    auto &type = findType(package, "QObject");
    type.propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "children", Storage::Synchronization::ImportedType{"Object"}, {}});

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(Field(&Storage::Synchronization::Type::typeName, Eq("QQuickItem")),
                               Field(&Storage::Synchronization::Type::defaultPropertyName,
                                     Eq("children")))));
}

TEST_F(ProjectStorage, synchronize_move_the_default_property_to_the_prototype_property)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.front().defaultPropertyName = "children";
    storage.synchronize(package);
    removeProperty(package, "QQuickItem", "children");
    auto &type = findType(package, "QObject");
    type.propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "children", Storage::Synchronization::ImportedType{"Object"}, {}});

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(Field(&Storage::Synchronization::Type::typeName, Eq("QQuickItem")),
                               Field(&Storage::Synchronization::Type::defaultPropertyName,
                                     Eq("children")))));
}

TEST_F(ProjectStorage, synchronize_move_the_default_property_to_the_extension_property)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types.front().defaultPropertyName = "children";
    storage.synchronize(package);
    removeProperty(package, "QQuickItem", "children");
    auto &type = findType(package, "QObject");
    type.propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "children", Storage::Synchronization::ImportedType{"Object"}, {}});

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(Field(&Storage::Synchronization::Type::typeName, Eq("QQuickItem")),
                               Field(&Storage::Synchronization::Type::defaultPropertyName,
                                     Eq("children")))));
}

TEST_F(ProjectStorage, get_type)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QQuickItem");
    auto defaultPropertyName = storage.fetchTypeByTypeId(typeId).defaultPropertyName;
    auto defaultPropertyId = storage.propertyDeclarationId(typeId, defaultPropertyName);

    auto type = storage.type(typeId);

    ASSERT_THAT(type, Optional(IsInfoType(defaultPropertyId, sourceId1, TypeTraits::Reference)));
}

TEST_F(ProjectStorage, dont_get_type_for_invalid_id)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    auto type = storage.type(TypeId());

    ASSERT_THAT(type, Eq(std::nullopt));
}

TEST_F(ProjectStorage, get_common_type)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    auto typeId = storage.commonTypeId<QmlDesigner::Storage::Info::QtQuick,
                                       QmlDesigner::Storage::Info::Item>();

    ASSERT_THAT(typeId, fetchTypeId(sourceId1, "QQuickItem"));
}

TEST_F(ProjectStorage, get_common_type_again)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    auto firstTypeId = storage.commonTypeId<QmlDesigner::Storage::Info::QtQuick,
                                            QmlDesigner::Storage::Info::Item>();

    auto typeId = storage.commonTypeId<QmlDesigner::Storage::Info::QtQuick,
                                       QmlDesigner::Storage::Info::Item>();

    ASSERT_THAT(typeId, firstTypeId);
}

TEST_F(ProjectStorage, get_common_type_after_changing_type)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    auto oldTypeId = storage.commonTypeId<QmlDesigner::Storage::Info::QtQuick,
                                          QmlDesigner::Storage::Info::Item>();
    package.types.front().typeName = "QQuickItem2";
    storage.synchronize(package);

    auto typeId = storage.commonTypeId<QmlDesigner::Storage::Info::QtQuick,
                                       QmlDesigner::Storage::Info::Item>();

    ASSERT_THAT(typeId, Ne(oldTypeId));
    ASSERT_THAT(typeId, fetchTypeId(sourceId1, "QQuickItem2"));
}

TEST_F(ProjectStorage, type_ids_without_properties_get_initialized)
{
    auto package{createBuiltinSynchronizationPackage()};

    storage.synchronize(package);

    ASSERT_THAT(storage.commonTypeCache().typeIdsWithoutProperties(), Each(IsTrue()));
}

TEST_F(ProjectStorage, get_builtin_type)
{
    auto package{createBuiltinSynchronizationPackage()};
    storage.synchronize(package);

    auto typeId = storage.builtinTypeId<double>();

    ASSERT_THAT(typeId, fetchTypeId(sourceId1, "double"));
}

TEST_F(ProjectStorage, get_builtin_type_again)
{
    auto package{createBuiltinSynchronizationPackage()};
    storage.synchronize(package);
    auto firstTypeId = storage.builtinTypeId<double>();

    auto typeId = storage.builtinTypeId<double>();

    ASSERT_THAT(typeId, firstTypeId);
}

TEST_F(ProjectStorage, get_builtin_type_after_changing_type)
{
    auto package{createBuiltinSynchronizationPackage()};
    storage.synchronize(package);
    auto oldTypeId = storage.builtinTypeId<bool>();
    package.types.front().typeName = "bool2";
    storage.synchronize(package);

    auto typeId = storage.builtinTypeId<bool>();

    ASSERT_THAT(typeId, Ne(oldTypeId));
    ASSERT_THAT(typeId, fetchTypeId(sourceId1, "bool2"));
}

TEST_F(ProjectStorage, get_builtin_string_type)
{
    auto package{createBuiltinSynchronizationPackage()};
    storage.synchronize(package);

    auto typeId = storage.builtinTypeId<QmlDesigner::Storage::Info::var>();

    ASSERT_THAT(typeId, fetchTypeId(sourceId1, "var"));
}

TEST_F(ProjectStorage, get_builtin_string_type_again)
{
    auto package{createBuiltinSynchronizationPackage()};
    storage.synchronize(package);
    auto firstTypeId = storage.builtinTypeId<QmlDesigner::Storage::Info::var>();

    auto typeId = storage.builtinTypeId<QmlDesigner::Storage::Info::var>();

    ASSERT_THAT(typeId, firstTypeId);
}

TEST_F(ProjectStorage, get_builtin_string_type_after_changing_type)
{
    auto package{createBuiltinSynchronizationPackage()};
    storage.synchronize(package);
    auto oldTypeId = storage.builtinTypeId<QmlDesigner::Storage::Info::var>();
    package.types.back().typeName = "variant";
    storage.synchronize(package);

    auto typeId = storage.builtinTypeId<QmlDesigner::Storage::Info::var>();

    ASSERT_THAT(typeId, Ne(oldTypeId));
    ASSERT_THAT(typeId, fetchTypeId(sourceId1, "variant"));
}

TEST_F(ProjectStorage, get_prototype_ids)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");

    auto prototypeIds = storage.prototypeIds(typeId);

    ASSERT_THAT(prototypeIds,
                ElementsAre(fetchTypeId(sourceId1, "QObject2"), fetchTypeId(sourceId1, "QObject")));
}

TEST_F(ProjectStorage, get_no_prototype_ids_for_no_prototype)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject");

    auto prototypeIds = storage.prototypeIds(typeId);

    ASSERT_THAT(prototypeIds, IsEmpty());
}

TEST_F(ProjectStorage, get_prototype_ids_with_extension)
{
    auto package{createPackageWithProperties()};
    std::swap(package.types[1].extension, package.types[1].prototype);
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");

    auto prototypeIds = storage.prototypeIds(typeId);

    ASSERT_THAT(prototypeIds,
                ElementsAre(fetchTypeId(sourceId1, "QObject2"), fetchTypeId(sourceId1, "QObject")));
}

TEST_F(ProjectStorage, get_prototype_and_self_ids)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");

    auto prototypeAndSelfIds = storage.prototypeAndSelfIds(typeId);

    ASSERT_THAT(prototypeAndSelfIds,
                ElementsAre(fetchTypeId(sourceId1, "QObject3"),
                            fetchTypeId(sourceId1, "QObject2"),
                            fetchTypeId(sourceId1, "QObject")));
}

TEST_F(ProjectStorage, get_self_for_no_prototype_ids)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject");

    auto prototypeAndSelfIds = storage.prototypeAndSelfIds(typeId);

    ASSERT_THAT(prototypeAndSelfIds, ElementsAre(fetchTypeId(sourceId1, "QObject")));
}

TEST_F(ProjectStorage, get_prototype_and_self_ids_with_extension)
{
    auto package{createPackageWithProperties()};
    std::swap(package.types[1].extension, package.types[1].prototype);
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");

    auto prototypeAndSelfIds = storage.prototypeAndSelfIds(typeId);

    ASSERT_THAT(prototypeAndSelfIds,
                ElementsAre(fetchTypeId(sourceId1, "QObject3"),
                            fetchTypeId(sourceId1, "QObject2"),
                            fetchTypeId(sourceId1, "QObject")));
}

TEST_F(ProjectStorage, is_based_on_for_direct_prototype)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject2");
    auto baseTypeId = fetchTypeId(sourceId1, "QObject");
    auto baseTypeId2 = fetchTypeId(sourceId1, "QObject3");

    bool isBasedOn = storage.isBasedOn(typeId, baseTypeId, baseTypeId2, TypeId{});

    ASSERT_TRUE(isBasedOn);
}

TEST_F(ProjectStorage, is_based_on_for_indirect_prototype)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");
    auto baseTypeId = fetchTypeId(sourceId1, "QObject");

    bool isBasedOn = storage.isBasedOn(typeId, baseTypeId);

    ASSERT_TRUE(isBasedOn);
}

TEST_F(ProjectStorage, is_based_on_for_direct_extension)
{
    auto package{createPackageWithProperties()};
    std::swap(package.types[1].extension, package.types[1].prototype);
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject2");
    auto baseTypeId = fetchTypeId(sourceId1, "QObject");

    bool isBasedOn = storage.isBasedOn(typeId, baseTypeId);

    ASSERT_TRUE(isBasedOn);
}

TEST_F(ProjectStorage, is_based_on_for_indirect_extension)
{
    auto package{createPackageWithProperties()};
    std::swap(package.types[1].extension, package.types[1].prototype);
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");
    auto baseTypeId = fetchTypeId(sourceId1, "QObject");

    bool isBasedOn = storage.isBasedOn(typeId, baseTypeId);

    ASSERT_TRUE(isBasedOn);
}

TEST_F(ProjectStorage, is_based_on_for_self)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject2");
    auto baseTypeId = fetchTypeId(sourceId1, "QObject2");

    bool isBasedOn = storage.isBasedOn(typeId, baseTypeId);

    ASSERT_TRUE(isBasedOn);
}

TEST_F(ProjectStorage, is_not_based_on)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject");
    auto baseTypeId = fetchTypeId(sourceId1, "QObject2");
    auto baseTypeId2 = fetchTypeId(sourceId1, "QObject3");

    bool isBasedOn = storage.isBasedOn(typeId, baseTypeId, baseTypeId2, TypeId{});

    ASSERT_FALSE(isBasedOn);
}

TEST_F(ProjectStorage, is_not_based_on_if_no_base_type_is_given)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject");

    bool isBasedOn = storage.isBasedOn(typeId);

    ASSERT_FALSE(isBasedOn);
}

TEST_F(ProjectStorage, get_imported_type_name_id_for_source_id)
{
    auto sourceId = sourcePathCache.sourceId("/path/foo.qml");

    auto importedTypeNameId = storage.importedTypeNameId(sourceId, "Item");

    ASSERT_TRUE(importedTypeNameId);
}

TEST_F(ProjectStorage,
       get_imported_type_name_id_for_source_id_returns_the_same_id_for_the_same_arguments)
{
    auto sourceId = sourcePathCache.sourceId("/path/foo.qml");
    auto expectedImportedTypeNameId = storage.importedTypeNameId(sourceId, "Item");

    auto importedTypeNameId = storage.importedTypeNameId(sourceId, "Item");

    ASSERT_THAT(importedTypeNameId, expectedImportedTypeNameId);
}

TEST_F(ProjectStorage,
       get_imported_type_name_id_for_source_id_returns_different_id_for_different_sourceId)
{
    auto sourceId = sourcePathCache.sourceId("/path/foo.qml");
    auto expectedImportedTypeNameId = storage.importedTypeNameId(sourceId, "Item");
    auto sourceId2 = sourcePathCache.sourceId("/path/foo2.qml");

    auto importedTypeNameId = storage.importedTypeNameId(sourceId2, "Item");

    ASSERT_THAT(importedTypeNameId, Not(expectedImportedTypeNameId));
}

TEST_F(ProjectStorage, get_imported_type_name_id_returns_different_id_for_different_name)
{
    auto sourceId = sourcePathCache.sourceId("/path/foo.qml");
    auto expectedImportedTypeNameId = storage.importedTypeNameId(sourceId, "Item");

    auto importedTypeNameId = storage.importedTypeNameId(sourceId, "Item2");

    ASSERT_THAT(importedTypeNameId, Not(expectedImportedTypeNameId));
}

TEST_F(ProjectStorage, get_import_id)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    auto importId = storage.importId(Storage::Import{qmlModuleId, Storage::Version{}, sourceId1});

    ASSERT_TRUE(importId);
}

TEST_F(ProjectStorage, get_invalid_import_id_if_not_exists)
{
    auto importId = storage.importId(Storage::Import{qmlModuleId, Storage::Version{}, sourceId1});

    ASSERT_FALSE(importId);
}

TEST_F(ProjectStorage, get_imported_type_name_id_for_import_id)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    auto importId = storage.importId(Storage::Import{qmlModuleId, Storage::Version{}, sourceId1});

    auto importedTypeNameId = storage.importedTypeNameId(importId, "Item");

    ASSERT_TRUE(importedTypeNameId);
}

TEST_F(ProjectStorage,
       get_imported_type_name_id_for_import_id_returns_different_id_for_different_importId)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    auto importId = storage.importId(Storage::Import{qmlModuleId, Storage::Version{}, sourceId1});
    auto expectedImportedTypeNameId = storage.importedTypeNameId(importId, "Item");
    auto importId2 = storage.importId(
        Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1});

    auto importedTypeNameId = storage.importedTypeNameId(importId2, "Item");

    ASSERT_THAT(importedTypeNameId, Not(expectedImportedTypeNameId));
}

TEST_F(ProjectStorage,
       get_imported_type_name_id_for_import_id_returns_different_id_for_different_name)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    auto importId = storage.importId(Storage::Import{qmlModuleId, Storage::Version{}, sourceId1});
    auto expectedImportedTypeNameId = storage.importedTypeNameId(importId, "Item");

    auto importedTypeNameId = storage.importedTypeNameId(importId, "Item2");

    ASSERT_THAT(importedTypeNameId, Not(expectedImportedTypeNameId));
}

TEST_F(ProjectStorage, synchronize_document_imports)
{
    Storage::Imports imports;
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1);
    imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1);

    storage.synchronizeDocumentImports(imports, sourceId1);

    ASSERT_TRUE(storage.importId(imports.back()));
}

TEST_F(ProjectStorage, synchronize_document_imports_removes_import)
{
    Storage::Imports imports;
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1);
    imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1);
    storage.synchronizeDocumentImports(imports, sourceId1);
    auto removedImport = imports.back();
    imports.pop_back();

    storage.synchronizeDocumentImports(imports, sourceId1);

    ASSERT_FALSE(storage.importId(removedImport));
}

TEST_F(ProjectStorage, synchronize_document_imports_adds_import)
{
    Storage::Imports imports;
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1);
    storage.synchronizeDocumentImports(imports, sourceId1);
    imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1);

    storage.synchronizeDocumentImports(imports, sourceId1);

    ASSERT_TRUE(storage.importId(imports.back()));
}

TEST_F(ProjectStorage, get_exported_type_names)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId2, "QObject");

    auto exportedTypeNames = storage.exportedTypeNames(typeId);

    ASSERT_THAT(exportedTypeNames,
                UnorderedElementsAre(IsInfoExportTypeNames(qmlModuleId, "Object", 2, -1),
                                     IsInfoExportTypeNames(qmlModuleId, "Obj", 2, -1),
                                     IsInfoExportTypeNames(qmlNativeModuleId, "QObject", -1, -1)));
}

TEST_F(ProjectStorage, get_no_exported_type_names_if_type_id_is_invalid)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    TypeId typeId;

    auto exportedTypeNames = storage.exportedTypeNames(typeId);

    ASSERT_THAT(exportedTypeNames, IsEmpty());
}

TEST_F(ProjectStorage, get_exported_type_names_for_source_id)
{
    auto package{createSimpleSynchronizationPackage()};
    package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId3);
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId2, "QObject");

    auto exportedTypeNames = storage.exportedTypeNames(typeId, sourceId3);

    ASSERT_THAT(exportedTypeNames,
                UnorderedElementsAre(IsInfoExportTypeNames(qmlModuleId, "Object", 2, -1),
                                     IsInfoExportTypeNames(qmlModuleId, "Obj", 2, -1)));
}

TEST_F(ProjectStorage, get_no_exported_type_names_for_source_id_for_invalid_type_id)
{
    auto package{createSimpleSynchronizationPackage()};
    package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId3);
    storage.synchronize(package);
    TypeId typeId;

    auto exportedTypeNames = storage.exportedTypeNames(typeId, sourceId3);

    ASSERT_THAT(exportedTypeNames, IsEmpty());
}

TEST_F(ProjectStorage, get_no_exported_type_names_for_source_id_for_non_matching_import)
{
    auto package{createSimpleSynchronizationPackage()};
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId2, "QObject");

    auto exportedTypeNames = storage.exportedTypeNames(typeId, sourceId3);

    ASSERT_THAT(exportedTypeNames, IsEmpty());
}

TEST_F(ProjectStorage, get_property_editor_path_is)
{
    TypeId typeId = TypeId::create(21);
    SourceId sourceId = SourceId::create(5);
    storage.setPropertyEditorPathId(typeId, sourceId);

    auto id = storage.propertyEditorPathId(typeId);

    ASSERT_THAT(id, sourceId);
}

TEST_F(ProjectStorage, get_empty_property_editor_specifics_path_id_if_not_exists)
{
    TypeId typeId = TypeId::create(21);

    auto id = storage.propertyEditorPathId(typeId);

    ASSERT_THAT(id, IsFalse());
}

TEST_F(ProjectStorage, synchronize_property_editor_paths)
{
    auto package{createPropertyEditorPathsSynchronizationPackage()};

    storage.synchronize(package);

    ASSERT_THAT(storage.propertyEditorPathId(fetchTypeId(sourceId2, "QObject")), sourceId1);
    ASSERT_THAT(storage.propertyEditorPathId(fetchTypeId(sourceId1, "QQuickItem")), sourceId2);
    ASSERT_THAT(storage.propertyEditorPathId(fetchTypeId(sourceId3, "QQuickItem3d")), sourceId3);
}

TEST_F(ProjectStorage, synchronize_property_editor_paths_removes_path)
{
    auto package{createPropertyEditorPathsSynchronizationPackage()};
    storage.synchronize(package);
    package.propertyEditorQmlPaths.pop_back();

    storage.synchronize(package);

    ASSERT_THAT(storage.propertyEditorPathId(fetchTypeId(sourceId2, "QObject")), sourceId1);
    ASSERT_THAT(storage.propertyEditorPathId(fetchTypeId(sourceId1, "QQuickItem")), sourceId2);
    ASSERT_THAT(storage.propertyEditorPathId(fetchTypeId(sourceId3, "QQuickItem3d")), IsFalse());
}

TEST_F(ProjectStorage, synchronize_property_editor_adds_path)
{
    auto package{createPropertyEditorPathsSynchronizationPackage()};
    package.propertyEditorQmlPaths.pop_back();
    storage.synchronize(package);
    package.propertyEditorQmlPaths.emplace_back(qtQuickModuleId, "Item3D", sourceId3, sourceIdPath);

    storage.synchronize(package);

    ASSERT_THAT(storage.propertyEditorPathId(fetchTypeId(sourceId2, "QObject")), sourceId1);
    ASSERT_THAT(storage.propertyEditorPathId(fetchTypeId(sourceId1, "QQuickItem")), sourceId2);
    ASSERT_THAT(storage.propertyEditorPathId(fetchTypeId(sourceId3, "QQuickItem3d")), sourceId3);
}

TEST_F(ProjectStorage, synchronize_property_editor_with_non_existing_type_name)
{
    auto package{createPropertyEditorPathsSynchronizationPackage()};
    package.propertyEditorQmlPaths.emplace_back(qtQuickModuleId, "Item4D", sourceId4, sourceIdPath);

    storage.synchronize(package);

    ASSERT_THAT(storage.propertyEditorPathId(fetchTypeId(sourceId4, "Item4D")), IsFalse());
}

TEST_F(ProjectStorage, call_refresh_callback_after_synchronization)
{
    auto package{createSimpleSynchronizationPackage()};
    MockFunction<void(const QmlDesigner::TypeIds &)> callbackMock;
    auto callback = callbackMock.AsStdFunction();
    storage.addRefreshCallback(&callback);

    EXPECT_CALL(callbackMock, Call(_));

    storage.synchronize(package);
}

} // namespace
