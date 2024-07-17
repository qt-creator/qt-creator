// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <matchers/info_exportedtypenames-matcher.h>
#include <matchers/projectstorage-matcher.h>
#include <projectstorageerrornotifiermock.h>
#include <projectstorageobservermock.h>

#include <modelnode.h>
#include <projectstorage/projectstorage.h>
#include <projectstorage/sourcepathcache.h>
#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitewritestatement.h>

#include <random>

namespace {

using QmlDesigner::Cache::Source;
using QmlDesigner::Cache::SourceContext;
using QmlDesigner::FileStatus;
using QmlDesigner::FileStatuses;
using QmlDesigner::FlagIs;
using QmlDesigner::ModuleId;
using QmlDesigner::PropertyDeclarationId;
using QmlDesigner::SourceContextId;
using QmlDesigner::SourceId;
using QmlDesigner::SourceIds;
using QmlDesigner::Storage::ModuleKind;
using QmlDesigner::Storage::Synchronization::SynchronizationPackage;
using QmlDesigner::Storage::Synchronization::TypeAnnotations;
using QmlDesigner::Storage::TypeTraits;
using QmlDesigner::Storage::TypeTraitsKind;
using QmlDesigner::TypeId;

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

auto IsModule(Utils::SmallStringView name, ModuleKind kind)
{
    return AllOf(Field(&QmlDesigner::Storage::Module::name, name),
                 Field(&QmlDesigner::Storage::Module::kind, kind));
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

template<typename PropertyTypeIdMatcher>
auto IsPropertyDeclaration(Utils::SmallStringView name,
                           const PropertyTypeIdMatcher &propertyTypeIdMatcher)
{
    return AllOf(Field(&Storage::Synchronization::PropertyDeclaration::name, name),
                 Field(&Storage::Synchronization::PropertyDeclaration::propertyTypeId,
                       propertyTypeIdMatcher));
}

template<typename PropertyTypeIdMatcher, typename TraitsMatcher>
auto IsPropertyDeclaration(Utils::SmallStringView name,
                           const PropertyTypeIdMatcher &propertyTypeIdMatcher,
                           TraitsMatcher traitsMatcher)
{
    return AllOf(Field(&Storage::Synchronization::PropertyDeclaration::name, name),
                 Field(&Storage::Synchronization::PropertyDeclaration::propertyTypeId,
                       propertyTypeIdMatcher),
                 Field(&Storage::Synchronization::PropertyDeclaration::traits, traitsMatcher));
}

template<typename PropertyTypeIdMatcher, typename TraitsMatcher>
auto IsPropertyDeclaration(Utils::SmallStringView name,
                           const PropertyTypeIdMatcher &propertyTypeIdMatcher,
                           TraitsMatcher traitsMatcher,
                           Utils::SmallStringView aliasPropertyName)
{
    return AllOf(Field(&Storage::Synchronization::PropertyDeclaration::name, name),
                 Field(&Storage::Synchronization::PropertyDeclaration::propertyTypeId,
                       propertyTypeIdMatcher),
                 Field(&Storage::Synchronization::PropertyDeclaration::traits, traitsMatcher),
                 Field(&Storage::Synchronization::PropertyDeclaration::aliasPropertyName,
                       aliasPropertyName));
}

template<typename PropertyTypeIdMatcher, typename TraitsMatcher>
auto IsInfoPropertyDeclaration(TypeId typeId,
                               Utils::SmallStringView name,
                               TraitsMatcher traitsMatcher,
                               const PropertyTypeIdMatcher &propertyTypeIdMatcher)
{
    return AllOf(Field(&Storage::Info::PropertyDeclaration::typeId, typeId),
                 Field(&Storage::Info::PropertyDeclaration::name, name),
                 Field(&Storage::Info::PropertyDeclaration::propertyTypeId, propertyTypeIdMatcher),
                 Field(&Storage::Info::PropertyDeclaration::traits, traitsMatcher));
}

auto IsUnresolvedTypeId()
{
    return Property(&QmlDesigner::TypeId::internalId, -1);
}

auto IsNullTypeId()
{
    return Property(&QmlDesigner::TypeId::isNull, true);
}

auto IsNullPropertyDeclarationId()
{
    return Property(&QmlDesigner::PropertyDeclarationId::isNull, true);
}

template<typename Matcher>
auto HasPrototypeId(const Matcher &matcher)
{
    return Field(&Storage::Synchronization::Type::prototypeId, matcher);
}

template<typename Matcher>
auto HasExtensionId(const Matcher &matcher)
{
    return Field(&Storage::Synchronization::Type::extensionId, matcher);
}

template<typename Matcher>
auto PropertyDeclarations(const Matcher &matcher)
{
    return Field(&Storage::Synchronization::Type::propertyDeclarations, matcher);
}

template<typename Matcher>
auto HasPropertyTypeId(const Matcher &matcher)
{
    return Field(&Storage::Synchronization::PropertyDeclaration::propertyTypeId, matcher);
}

template<typename Matcher>
auto HasTypeName(const Matcher &matcher)
{
    return Field(&Storage::Synchronization::Type::typeName, matcher);
}

template<typename Matcher>
auto HasDefaultPropertyName(const Matcher &matcher)
{
    return Field(&Storage::Synchronization::Type::defaultPropertyName, matcher);
}

class HasNameMatcher
{
public:
    using is_gtest_matcher = void;

    HasNameMatcher(const QmlDesigner::ProjectStorage &storage,
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
    const QmlDesigner::ProjectStorage &storage;
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

MATCHER_P2(IsInfoType,
           sourceId,
           traits,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Info::Type{sourceId, traits}))
{
    const Storage::Info::Type &type = arg;

    return type.sourceId == sourceId && type.traits == traits;
}

class ProjectStorage : public testing::Test
{
protected:
    struct StaticData
    {
        Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
        NiceMock<ProjectStorageErrorNotifierMock> errorNotifierMock;
        QmlDesigner::ProjectStorage storage{database, errorNotifierMock, database.isInitialized()};
    };

    static void SetUpTestSuite() { staticData = std::make_unique<StaticData>(); }

    static void TearDownTestSuite() { staticData.reset(); }

    ProjectStorage() { storage.setErrorNotifier(errorNotifierMock); }

    ~ProjectStorage() { storage.resetForTestsOnly(); }

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

    auto createVerySimpleSynchronizationPackage()
    {
        SynchronizationPackage package;

        importsSourceId1.emplace_back(qmlModuleId, Storage::Version{}, sourceId1);
        importsSourceId1.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1);

        package.types.push_back(Storage::Synchronization::Type{
            "QQuickItem",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraitsKind::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item"},
             Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QQuickItem"}}});
        package.types.push_back(Storage::Synchronization::Type{
            "QObject",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraitsKind::Reference,
            sourceId2,
            {Storage::Synchronization::ExportedType{qmlModuleId, "Object"},
             Storage::Synchronization::ExportedType{qmlNativeModuleId, "QObject"}}});

        package.updatedSourceIds = {sourceId1, sourceId2};

        return package;
    }

    auto createSimpleSynchronizationPackage()
    {
        SynchronizationPackage package;

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1);
        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId2);
        package.moduleDependencies.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId1);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId, Storage::Version{}, sourceId1);
        package.moduleDependencies.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId2);
        package.updatedModuleDependencySourceIds.push_back(sourceId1);
        package.updatedModuleDependencySourceIds.push_back(sourceId2);

        importsSourceId1.emplace_back(qmlModuleId, Storage::Version{}, sourceId1);
        importsSourceId1.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1);
        moduleDependenciesSourceId1.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId1);
        moduleDependenciesSourceId1.emplace_back(qtQuickNativeModuleId, Storage::Version{}, sourceId1);

        importsSourceId2.emplace_back(qmlModuleId, Storage::Version{}, sourceId2);
        moduleDependenciesSourceId2.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId2);

        package.types.push_back(Storage::Synchronization::Type{
            "QQuickItem",
            Storage::Synchronization::ImportedType{"QObject"},
            Storage::Synchronization::ImportedType{},
            TypeTraitsKind::Reference,
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
            TypeTraitsKind::Reference,
            sourceId2,
            {Storage::Synchronization::ExportedType{qmlModuleId, "Object", Storage::Version{2}},
             Storage::Synchronization::ExportedType{qmlModuleId, "Obj", Storage::Version{2}},
             Storage::Synchronization::ExportedType{qmlNativeModuleId, "QObject"}}});

        package.updatedSourceIds = {sourceId1, sourceId2};

        return package;
    }

    auto createBuiltinSynchronizationPackage()
    {
        SynchronizationPackage package;

        package.imports.emplace_back(QMLModuleId, Storage::Version{}, sourceId1);
        package.moduleDependencies.emplace_back(QMLModuleId, Storage::Version{}, sourceId1);
        package.updatedModuleDependencySourceIds.push_back(sourceId1);

        importsSourceId1.emplace_back(QMLModuleId, Storage::Version{}, sourceId1);
        moduleDependenciesSourceId1.emplace_back(QMLModuleId, Storage::Version{}, sourceId1);

        package.types.push_back(
            Storage::Synchronization::Type{"bool",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraitsKind::Value,
                                           sourceId1,
                                           {Storage::Synchronization::ExportedType{QMLModuleId,
                                                                                   "bool"}}});
        package.types.push_back(
            Storage::Synchronization::Type{"int",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraitsKind::Value,
                                           sourceId1,
                                           {Storage::Synchronization::ExportedType{QMLModuleId,
                                                                                   "int"}}});
        package.types.push_back(
            Storage::Synchronization::Type{"uint",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraitsKind::Value,
                                           sourceId1,
                                           {Storage::Synchronization::ExportedType{QMLNativeModuleId,
                                                                                   "uint"}}});
        package.types.push_back(Storage::Synchronization::Type{
            "double",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraitsKind::Value,
            sourceId1,
            {Storage::Synchronization::ExportedType{QMLModuleId, "double"},
             Storage::Synchronization::ExportedType{QMLModuleId, "real"}}});
        package.types.push_back(
            Storage::Synchronization::Type{"float",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraitsKind::Value,
                                           sourceId1,
                                           {Storage::Synchronization::ExportedType{QMLNativeModuleId,
                                                                                   "float"}}});
        package.types.push_back(
            Storage::Synchronization::Type{"date",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraitsKind::Value,
                                           sourceId1,
                                           {Storage::Synchronization::ExportedType{QMLModuleId,
                                                                                   "date"}}});
        package.types.push_back(
            Storage::Synchronization::Type{"string",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraitsKind::Value,
                                           sourceId1,
                                           {Storage::Synchronization::ExportedType{QMLModuleId,
                                                                                   "string"}}});
        package.types.push_back(
            Storage::Synchronization::Type{"url",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraitsKind::Value,
                                           sourceId1,
                                           {Storage::Synchronization::ExportedType{QMLModuleId,
                                                                                   "url"}}});
        package.types.push_back(
            Storage::Synchronization::Type{"var",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraitsKind::Value,
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
        package.moduleDependencies.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId3);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId, Storage::Version{}, sourceId3);
        package.updatedModuleDependencySourceIds.push_back(sourceId3);

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId4);
        package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId4);
        package.moduleDependencies.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId4);
        package.updatedModuleDependencySourceIds.push_back(sourceId4);

        importsSourceId3.emplace_back(qmlModuleId, Storage::Version{}, sourceId3);
        importsSourceId3.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
        moduleDependenciesSourceId3.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId3);
        moduleDependenciesSourceId3.emplace_back(qtQuickNativeModuleId, Storage::Version{}, sourceId3);

        importsSourceId4.emplace_back(qmlModuleId, Storage::Version{}, sourceId4);
        importsSourceId4.emplace_back(pathToModuleId, Storage::Version{}, sourceId4);
        moduleDependenciesSourceId4.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId4);

        package.types[1].propertyDeclarations.push_back(
            Storage::Synchronization::PropertyDeclaration{"objects",
                                                          Storage::Synchronization::ImportedType{
                                                              "QObject"},
                                                          Storage::PropertyDeclarationTraits::IsList});
        package.types.push_back(Storage::Synchronization::Type{
            "QAliasItem",
            Storage::Synchronization::ImportedType{"Item"},
            Storage::Synchronization::ImportedType{},
            TypeTraitsKind::Reference,
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
            TypeTraitsKind::Reference,
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

        package.moduleDependencies.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId1);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId, Storage::Version{}, sourceId1);
        package.moduleDependencies.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId2);

        package.updatedModuleDependencySourceIds.push_back(sourceId1);
        package.updatedModuleDependencySourceIds.push_back(sourceId2);

        importsSourceId1.emplace_back(qmlModuleId, Storage::Version{}, sourceId1);
        importsSourceId1.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1);
        moduleDependenciesSourceId1.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId1);
        moduleDependenciesSourceId1.emplace_back(qtQuickNativeModuleId, Storage::Version{}, sourceId1);

        importsSourceId2.emplace_back(qmlModuleId, Storage::Version{}, sourceId2);
        moduleDependenciesSourceId2.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId2);

        package.types.push_back(Storage::Synchronization::Type{
            "QQuickItem",
            Storage::Synchronization::ImportedType{"QObject"},
            Storage::Synchronization::ImportedType{},
            TypeTraitsKind::Reference,
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
            TypeTraitsKind::Reference,
            sourceId2,
            {Storage::Synchronization::ExportedType{qmlModuleId, "Object", Storage::Version{2}},
             Storage::Synchronization::ExportedType{qmlModuleId, "Obj", Storage::Version{2}},
             Storage::Synchronization::ExportedType{qmlNativeModuleId, "QObject"}}});

        package.updatedSourceIds = {sourceId1, sourceId2};

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId3);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
        package.moduleDependencies.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId3);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId, Storage::Version{}, sourceId3);
        package.updatedModuleDependencySourceIds.push_back(sourceId3);

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId4);
        package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId4);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId4);
        package.moduleDependencies.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId4);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId, Storage::Version{}, sourceId4);
        package.updatedModuleDependencySourceIds.push_back(sourceId4);

        importsSourceId3.emplace_back(qmlModuleId, Storage::Version{}, sourceId3);
        importsSourceId3.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
        moduleDependenciesSourceId3.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId3);
        moduleDependenciesSourceId3.emplace_back(qtQuickNativeModuleId, Storage::Version{}, sourceId3);

        importsSourceId4.emplace_back(qmlModuleId, Storage::Version{}, sourceId4);
        importsSourceId4.emplace_back(pathToModuleId, Storage::Version{}, sourceId4);
        importsSourceId4.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId4);
        moduleDependenciesSourceId4.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId4);
        moduleDependenciesSourceId4.emplace_back(qtQuickNativeModuleId, Storage::Version{}, sourceId4);

        package.types[1].propertyDeclarations.push_back(
            Storage::Synchronization::PropertyDeclaration{"objects",
                                                          Storage::Synchronization::ImportedType{
                                                              "QObject"},
                                                          Storage::PropertyDeclarationTraits::IsList});
        package.types.push_back(Storage::Synchronization::Type{
            "QAliasItem",
            Storage::Synchronization::ImportedType{"Item"},
            Storage::Synchronization::ImportedType{},
            TypeTraitsKind::Reference,
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
            TypeTraitsKind::Reference,
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
            TypeTraitsKind::Reference,
            sourceId5,
            {Storage::Synchronization::ExportedType{qtQuickModuleId, "Children", Storage::Version{2}},
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
        package.moduleDependencies.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId5);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId, Storage::Version{}, sourceId5);
        importsSourceId5.emplace_back(qmlModuleId, Storage::Version{}, sourceId5);
        importsSourceId5.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId5);
        moduleDependenciesSourceId5.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId5);
        moduleDependenciesSourceId5.emplace_back(qtQuickNativeModuleId, Storage::Version{}, sourceId5);
        package.updatedModuleDependencySourceIds.push_back(sourceId5);
        package.updatedSourceIds.push_back(sourceId5);

        package.types.push_back(Storage::Synchronization::Type{
            "QChildren2",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraitsKind::Reference,
            sourceId6,
            {Storage::Synchronization::ExportedType{qtQuickModuleId, "Children2", Storage::Version{2}},
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
        package.moduleDependencies.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId6);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId, Storage::Version{}, sourceId6);
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
            TypeTraitsKind::Reference,
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
            TypeTraitsKind::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qmlModuleId, "Object", Storage::Version{1}},
             Storage::Synchronization::ExportedType{qmlModuleId, "Obj", Storage::Version{1, 2}},
             Storage::Synchronization::ExportedType{qmlModuleId, "BuiltInObj"},
             Storage::Synchronization::ExportedType{qmlNativeModuleId, "QObject"}}});
        package.types.push_back(Storage::Synchronization::Type{
            "QObject2",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraitsKind::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qmlModuleId, "Object", Storage::Version{2, 0}},
             Storage::Synchronization::ExportedType{qmlModuleId, "Obj", Storage::Version{2, 3}},
             Storage::Synchronization::ExportedType{qmlNativeModuleId, "QObject2"}}});
        package.types.push_back(Storage::Synchronization::Type{
            "QObject3",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraitsKind::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qmlModuleId, "Object", Storage::Version{2, 11}},
             Storage::Synchronization::ExportedType{qmlModuleId, "Obj", Storage::Version{2, 11}},
             Storage::Synchronization::ExportedType{qmlNativeModuleId, "QObject3"}}});
        package.types.push_back(Storage::Synchronization::Type{
            "QObject4",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraitsKind::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qmlModuleId, "Object", Storage::Version{3, 4}},
             Storage::Synchronization::ExportedType{qmlModuleId, "Obj", Storage::Version{3, 4}},
             Storage::Synchronization::ExportedType{qmlModuleId, "BuiltInObj", Storage::Version{3, 4}},
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
            TypeTraitsKind::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qmlModuleId, "Object", Storage::Version{}}},
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
            TypeTraitsKind::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qmlModuleId, "Object2", Storage::Version{}}},
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
            TypeTraitsKind::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qmlModuleId, "Object3", Storage::Version{}}},
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

    auto createHeirPackage()
    {
        auto package = createPackageWithProperties();

        package.types.push_back(
            Storage::Synchronization::Type{"QObject4",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{"Object2"},
                                           TypeTraitsKind::Reference,
                                           sourceId1,
                                           {},
                                           {},
                                           {},
                                           {}});
        package.types.push_back(
            Storage::Synchronization::Type{"QObject5",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{"Object2"},
                                           TypeTraitsKind::Reference,
                                           sourceId1,
                                           {},
                                           {},
                                           {},
                                           {}});

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
            TypeTraitsKind::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item", Storage::Version{1, 0}}}});

        package.updatedModuleIds.push_back(qmlModuleId);
        package.moduleExportedImports.emplace_back(qtQuickModuleId,
                                                   qmlModuleId,
                                                   Storage::Version{},
                                                   Storage::Synchronization::IsAutoVersion::Yes);
        package.types.push_back(Storage::Synchronization::Type{
            "QObject",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraitsKind::Reference,
            sourceId2,
            {Storage::Synchronization::ExportedType{qmlModuleId, "Object", Storage::Version{1, 0}}}});

        package.imports.emplace_back(qtQuickModuleId, Storage::Version{1}, sourceId3);
        package.moduleExportedImports.emplace_back(qtQuick3DModuleId,
                                                   qtQuickModuleId,
                                                   Storage::Version{},
                                                   Storage::Synchronization::IsAutoVersion::Yes);
        package.updatedModuleIds.push_back(qtQuick3DModuleId);
        package.types.push_back(
            Storage::Synchronization::Type{"QQuickItem3d",
                                           Storage::Synchronization::ImportedType{"Item"},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraitsKind::Reference,
                                           sourceId3,
                                           {Storage::Synchronization::ExportedType{
                                               qtQuick3DModuleId, "Item3D", Storage::Version{1, 0}}}});

        package.imports.emplace_back(qtQuick3DModuleId, Storage::Version{1}, sourceId4);
        package.types.push_back(
            Storage::Synchronization::Type{"MyItem",
                                           Storage::Synchronization::ImportedType{"Object"},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraitsKind::Reference,
                                           sourceId4,
                                           {Storage::Synchronization::ExportedType{
                                               myModuleModuleId, "MyItem", Storage::Version{1, 0}}}});

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
            TypeTraitsKind::Reference,
            sourceId1,
            {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item", Storage::Version{1, 0}}}});
        package.types.push_back(
            Storage::Synchronization::Type{"QObject",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraitsKind::Reference,
                                           sourceId2,
                                           {Storage::Synchronization::ExportedType{
                                               qtQuickModuleId, "QtObject", Storage::Version{1, 0}}}});
        package.updatedModuleIds.push_back(qtQuick3DModuleId);
        package.types.push_back(
            Storage::Synchronization::Type{"QQuickItem3d",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraitsKind::Reference,
                                           sourceId3,
                                           {Storage::Synchronization::ExportedType{
                                               qtQuickModuleId, "Item3D", Storage::Version{1, 0}}}});

        package.imports.emplace_back(qtQuick3DModuleId, Storage::Version{1}, sourceId4);

        package.updatedSourceIds = {sourceId1, sourceId2, sourceId3};

        package.propertyEditorQmlPaths.emplace_back(qtQuickModuleId, "QtObject", sourceId1, sourceIdPath6);
        package.propertyEditorQmlPaths.emplace_back(qtQuickModuleId, "Item", sourceId2, sourceIdPath6);
        package.propertyEditorQmlPaths.emplace_back(qtQuickModuleId, "Item3D", sourceId3, sourceIdPath6);
        package.updatedPropertyEditorQmlPathSourceIds.emplace_back(sourceIdPath6);

        return package;
    }

    auto createTypeAnnotions() const
    {
        TypeAnnotations annotations;

        TypeTraits traits{TypeTraitsKind::Reference};
        traits.canBeContainer = FlagIs::True;
        traits.visibleInLibrary = FlagIs::True;

        annotations.emplace_back(sourceId4,
                                 sourceIdPath6,
                                 "Object",
                                 qmlModuleId,
                                 "/path/to/icon.png",
                                 traits,
                                 R"xy({"canBeContainer": "true", "hints": "false"})xy",
                                 R"xy([{"name":"Foo",
                                        "iconPath":"/path/icon",
                                        "category":"Basic Items",
                                        "import":"QtQuick",
                                        "toolTip":"Foo is a Item",
                                        "templatePath":"/path/templates/item.qml",
                                        "properties":[["x", "double", 32.1],["y", "double", 12.3]],
                                        "extraFilePaths":["/path/templates/frame.png", "/path/templates/frame.frag"]},
                                       {"name":"Bar",
                                        "iconPath":"/path/icon2",
                                        "category":"Basic Items",
                                        "import":"QtQuick",
                                        "toolTip":"Bar is a Item",
                                        "properties":[["color", "color", "#blue"]]}])xy");

        annotations.emplace_back(sourceId5,
                                 sourceIdPath6,
                                 "Item",
                                 qtQuickModuleId,
                                 "/path/to/quick.png",
                                 traits,
                                 R"xy({"canBeContainer": "true", "forceClip": "false"})xy",
                                 R"xy([{"name":"Item",
                                        "iconPath":"/path/icon3",
                                        "category":"Advanced Items",
                                        "import":"QtQuick",
                                        "toolTip":"Item is an Object",
                                        "properties":[["x", "double", 1], ["y", "double", 2]]}])xy");

        return annotations;
    }

    auto createExtendedTypeAnnotations() const
    {
        auto annotations = createTypeAnnotions();
        annotations.pop_back();
        TypeTraits traits{TypeTraitsKind::Reference};
        traits.canBeContainer = FlagIs::True;
        traits.visibleInLibrary = FlagIs::True;

        annotations.emplace_back(sourceId5,
                                 sourceIdPath1,
                                 "Item",
                                 qtQuickModuleId,
                                 "/path/to/quick.png",
                                 traits,
                                 R"xy({"canBeContainer": "true", "forceClip": "false"})xy",
                                 R"xy([{"name":"Item",
                                        "iconPath":"/path/icon3",
                                        "category":"Advanced Items",
                                        "import":"QtQuick",
                                        "toolTip":"Item is an Object",
                                        "properties":[["x", "double", 1], ["y", "double", 2]]}])xy");

        return annotations;
    }

    static auto createUpdatedTypeAnnotionSourceIds(const TypeAnnotations &annotations)
    {
        return Utils::transform<SourceIds>(annotations, [](const auto &annotation) {
            return annotation.sourceId;
        });
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

    auto fetchType(SourceId sourceId, Utils::SmallStringView name)
    {
        return storage.fetchTypeByTypeId(storage.fetchTypeIdByName(sourceId, name));
    }

    auto defaultPropertyDeclarationId(SourceId sourceId, Utils::SmallStringView typeName)
    {
        return storage.defaultPropertyDeclarationId(storage.fetchTypeIdByName(sourceId, typeName));
    }

    auto propertyDeclarationId(SourceId sourceId,
                               Utils::SmallStringView typeName,
                               Utils::SmallStringView propertyName)
    {
        return storage.propertyDeclarationId(storage.fetchTypeIdByName(sourceId, typeName),
                                             propertyName);
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
    inline static std::unique_ptr<StaticData> staticData;
    Sqlite::Database &database = staticData->database;
    QmlDesigner::ProjectStorage &storage = staticData->storage;
    NiceMock<ProjectStorageErrorNotifierMock> errorNotifierMock;
    QmlDesigner::SourcePathCache<QmlDesigner::ProjectStorage> sourcePathCache{storage};
    QmlDesigner::SourcePathView path1{"/path1/to"};
    QmlDesigner::SourcePathView path2{"/path2/to"};
    QmlDesigner::SourcePathView path3{"/path3/to"};
    QmlDesigner::SourcePathView path4{"/path4/to"};
    QmlDesigner::SourcePathView path5{"/path5/to"};
    QmlDesigner::SourcePathView path6{"/path6/to"};
    QmlDesigner::SourcePathView pathPath1{"/path1/."};
    QmlDesigner::SourcePathView pathPath6{"/path6/."};
    SourceId sourceId1{sourcePathCache.sourceId(path1)};
    SourceId sourceId2{sourcePathCache.sourceId(path2)};
    SourceId sourceId3{sourcePathCache.sourceId(path3)};
    SourceId sourceId4{sourcePathCache.sourceId(path4)};
    SourceId sourceId5{sourcePathCache.sourceId(path5)};
    SourceId sourceId6{sourcePathCache.sourceId(path6)};
    SourceId sourceIdPath1{sourcePathCache.sourceId(pathPath1)};
    SourceId sourceIdPath6{sourcePathCache.sourceId(pathPath6)};
    SourceId qmlProjectSourceId{sourcePathCache.sourceId("/path1/qmldir")};
    SourceId qtQuickProjectSourceId{sourcePathCache.sourceId("/path2/qmldir")};
    ModuleId qmlModuleId{storage.moduleId("Qml", ModuleKind::QmlLibrary)};
    ModuleId qmlNativeModuleId{storage.moduleId("Qml", ModuleKind::CppLibrary)};
    ModuleId qtQuickModuleId{storage.moduleId("QtQuick", ModuleKind::QmlLibrary)};
    ModuleId qtQuickNativeModuleId{storage.moduleId("QtQuick", ModuleKind::CppLibrary)};
    ModuleId pathToModuleId{storage.moduleId("/path/to", ModuleKind::PathLibrary)};
    ModuleId qtQuick3DModuleId{storage.moduleId("QtQuick3D", ModuleKind::QmlLibrary)};
    ModuleId myModuleModuleId{storage.moduleId("MyModule", ModuleKind::QmlLibrary)};
    ModuleId QMLModuleId{storage.moduleId("QML", ModuleKind::QmlLibrary)};
    ModuleId QMLNativeModuleId{storage.moduleId("QML", ModuleKind::CppLibrary)};
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
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
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
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        TypeId{},
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
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
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeId{}, TypeTraitsKind::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                fetchTypeId(sourceId2, "QObject"),
                                TypeId{},
                                TypeTraitsKind::Reference),
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
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        TypeId{},
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                                     IsExportedType(qtQuickNativeModuleId,
                                                                    "QQuickItem"))))));
}

TEST_F(ProjectStorage,
       synchronize_types_adds_unknown_prototype_which_notifies_about_unresolved_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Objec"};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Objec"), sourceId1));

    storage.synchronize(std::move(package));
}

TEST_F(ProjectStorage, synchronize_types_adds_unknown_prototype_as_unresolved_type_id)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Objec"};

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(IsUnresolvedTypeId()));
}

TEST_F(ProjectStorage, synchronize_types_updates_unresolved_prototype_after_exported_type_name_is_added)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Objec"};
    storage.synchronize(package);
    package.types[1].exportedTypes.emplace_back(qmlNativeModuleId, "Objec");

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(fetchTypeId(sourceId2, "QObject")));
}

TEST_F(ProjectStorage,
       synchronize_types_updates_prototype_to_unresolved_after_exported_type_name_is_removed)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Objec"};
    package.types[1].exportedTypes.emplace_back(qmlNativeModuleId, "Objec");
    storage.synchronize(package);
    package.types[1].exportedTypes.pop_back();

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(IsUnresolvedTypeId()));
}

TEST_F(ProjectStorage,
       synchronize_types_updates_unresolved_prototype_indirectly_after_exported_type_name_is_added)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Objec"};
    storage.synchronize(package);
    package.types[1].exportedTypes.emplace_back(qmlNativeModuleId, "Objec");
    package.types.erase(package.types.begin());
    package.updatedSourceIds = {sourceId2};

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(fetchTypeId(sourceId2, "QObject")));
}

TEST_F(ProjectStorage,
       synchronize_types_updates_prototype_indirectly_to_unresolved_after_exported_type_name_is_removed)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Objec"};
    package.types[1].exportedTypes.emplace_back(qmlNativeModuleId, "Objec");
    storage.synchronize(package);
    package.types[1].exportedTypes.pop_back();
    package.types.erase(package.types.begin());
    package.updatedSourceIds = {sourceId2};

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(IsUnresolvedTypeId()));
}

TEST_F(ProjectStorage,
       synchronize_types_updates_prototype_indirectly_to_unresolved_after_exported_type_name_is_removed_notifies_type_name_cannot_be_resolved)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Objec"};
    package.types[1].exportedTypes.emplace_back(qmlNativeModuleId, "Objec");
    storage.synchronize(package);
    package.types[1].exportedTypes.pop_back();
    package.types.erase(package.types.begin());
    package.updatedSourceIds = {sourceId2};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Objec"), sourceId1));

    storage.synchronize(std::move(package));
}

TEST_F(ProjectStorage, synchronize_types_updates_unresolved_extension_after_exported_type_name_is_added)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].extension = Storage::Synchronization::ImportedType{"Objec"};
    storage.synchronize(package);
    package.types[1].exportedTypes.emplace_back(qmlNativeModuleId, "Objec");

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasExtensionId(fetchTypeId(sourceId2, "QObject")));
}

TEST_F(ProjectStorage,
       synchronize_types_updates_extension_to_unresolved_after_exported_type_name_is_removed)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].extension = Storage::Synchronization::ImportedType{"Objec"};
    package.types[1].exportedTypes.emplace_back(qmlNativeModuleId, "Objec");
    storage.synchronize(package);
    package.types[1].exportedTypes.pop_back();

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasExtensionId(IsUnresolvedTypeId()));
}

TEST_F(ProjectStorage,
       synchronize_types_updates_unresolved_extension_indirectly_after_exported_type_name_is_added)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].extension = Storage::Synchronization::ImportedType{"Objec"};
    storage.synchronize(package);
    package.types[1].exportedTypes.emplace_back(qmlNativeModuleId, "Objec");
    package.types.erase(package.types.begin());
    package.updatedSourceIds = {sourceId2};

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasExtensionId(fetchTypeId(sourceId2, "QObject")));
}

TEST_F(ProjectStorage,
       synchronize_types_updates_invalid_extension_indirectly_after_exported_type_name_is_removed)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].extension = Storage::Synchronization::ImportedType{"Objec"};
    package.types[1].exportedTypes.emplace_back(qmlNativeModuleId, "Objec");
    storage.synchronize(package);
    package.types[1].exportedTypes.pop_back();
    package.types.erase(package.types.begin());
    package.updatedSourceIds = {sourceId2};

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasExtensionId(IsUnresolvedTypeId()));
}

TEST_F(ProjectStorage,
       synchronize_types_updates_extension_indirectly_to_unresolved_after_exported_type_name_is_removed_notifies_type_name_cannot_be_resolved)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].extension = Storage::Synchronization::ImportedType{"Objec"};
    package.types[1].exportedTypes.emplace_back(qmlNativeModuleId, "Objec");
    storage.synchronize(package);
    package.types[1].exportedTypes.pop_back();
    package.types.erase(package.types.begin());
    package.updatedSourceIds = {sourceId2};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Objec"), sourceId1));

    storage.synchronize(std::move(package));
}

TEST_F(ProjectStorage, synchronize_types_adds_extension_which_notifies_about_unresolved_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].extension = Storage::Synchronization::ImportedType{"Objec"};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Objec"), sourceId1));

    storage.synchronize(std::move(package));
}


TEST_F(ProjectStorage, synchronize_types_adds_new_types_with_missing_module)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.push_back(Storage::Synchronization::Type{
        "QObject2",
        Storage::Synchronization::ImportedType{},
        Storage::Synchronization::ImportedType{},
        TypeTraitsKind::Reference,
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
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                                     IsExportedType(qtQuickNativeModuleId,
                                                                    "QQuickItem"))))));
}

TEST_F(ProjectStorage, synchronize_types_overwrites_type_traits)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].traits = TypeTraitsKind::Value;
    package.types[1].traits = TypeTraitsKind::Value;

    storage.synchronize(SynchronizationPackage{package.imports, package.types, {sourceId1, sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Value),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Value),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                                     IsExportedType(qtQuickNativeModuleId,
                                                                    "QQuickItem"))))));
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
                    AllOf(IsStorageType(sourceId4, "QObject", TypeId{}, TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId3,
                                        "QQuickItem",
                                        fetchTypeId(sourceId4, "QObject"),
                                        TypeTraitsKind::Reference),
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
        TypeTraitsKind::Reference,
        sourceId1,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Object"},
         Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QQuickObject"}}});

    storage.synchronize(
        SynchronizationPackage{importsSourceId1, {package.types[0], package.types[2]}, {sourceId1}});

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeId{}, TypeTraitsKind::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickObject",
                                fetchTypeId(sourceId2, "QObject"),
                                TypeId{},
                                TypeTraitsKind::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Object"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                fetchTypeId(sourceId1, "QQuickObject"),
                                TypeId{},
                                TypeTraitsKind::Reference),
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
        TypeTraitsKind::Reference,
        sourceId1,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Object"},
         Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QQuickObject"}}});

    storage.synchronize(
        SynchronizationPackage{importsSourceId1, {package.types[0], package.types[2]}, {sourceId1}});

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeId{}, TypeTraitsKind::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickObject",
                                TypeId{},
                                fetchTypeId(sourceId2, "QObject"),
                                TypeTraitsKind::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Object"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                TypeId{},
                                fetchTypeId(sourceId1, "QQuickObject"),
                                TypeTraitsKind::Reference),
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
        TypeTraitsKind::Reference,
        sourceId1,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Object"},
         Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QQuickObject"}}});

    storage.synchronize(package);

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeId{}, TypeTraitsKind::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickObject",
                                fetchTypeId(sourceId2, "QObject"),
                                TypeId{},
                                TypeTraitsKind::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Object"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                fetchTypeId(sourceId1, "QQuickObject"),
                                TypeId{},
                                TypeTraitsKind::Reference),
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
        TypeTraitsKind::Reference,
        sourceId1,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Object"},
         Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QQuickObject"}}});

    storage.synchronize(package);

    ASSERT_THAT(
        storage.fetchTypes(),
        UnorderedElementsAre(
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeId{}, TypeTraitsKind::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickObject",
                                TypeId{},
                                fetchTypeId(sourceId2, "QObject"),
                                TypeTraitsKind::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Object"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                TypeId{},
                                fetchTypeId(sourceId1, "QQuickObject"),
                                TypeTraitsKind::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                             IsExportedType(qtQuickNativeModuleId, "QQuickItem"))))));
}

TEST_F(ProjectStorage, synchronize_types_notifies_cannot_resolve_for_missing_prototype)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types = {Storage::Synchronization::Type{
        "QQuickItem",
        Storage::Synchronization::ImportedType{"QObject"},
        Storage::Synchronization::ImportedType{},
        TypeTraitsKind::Reference,
        sourceId1,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item"},
         Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QQuickItem"}}}};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("QObject"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage, synchronize_types_notifies_cannot_resolve_for_missing_extension)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types = {Storage::Synchronization::Type{
        "QQuickItem",
        Storage::Synchronization::ImportedType{},
        Storage::Synchronization::ImportedType{"QObject"},
        TypeTraitsKind::Reference,
        sourceId1,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item"},
         Storage::Synchronization::ExportedType{qtQuickNativeModuleId, "QQuickItem"}}}};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("QObject"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage, synchronize_types_throws_for_invalid_module)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types = {
        Storage::Synchronization::Type{"QQuickItem",
                                       Storage::Synchronization::ImportedType{"QObject"},
                                       Storage::Synchronization::ImportedType{},
                                       TypeTraitsKind::Reference,
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
                                       TypeTraitsKind::Reference,
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
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
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
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
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
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem2",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                                     IsExportedType(qtQuickNativeModuleId,
                                                                    "QQuickItem"))))));
}

TEST_F(ProjectStorage,
       breaking_prototype_chain_by_deleting_base_component_notifies_unresolved_type_name)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Object"};
    storage.synchronize(package);

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(
        SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1, sourceId2}});
}

TEST_F(ProjectStorage, breaking_prototype_chain_by_deleting_base_component_has_unresolved_type_name)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Object"};
    storage.synchronize(package);

    storage.synchronize(
        SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1, sourceId2}});

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(IsUnresolvedTypeId()));
}

TEST_F(ProjectStorage, repairing_prototype_chain_by_fixing_base_component)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Object"};
    storage.synchronize(package);
    storage.synchronize(
        SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1, sourceId2}});

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(fetchTypeId(sourceId2, "QObject")));
}

TEST_F(ProjectStorage,
       breaking_extension_chain_by_deleting_base_component_notifies_unresolved_type_name)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].extension = Storage::Synchronization::ImportedType{"Object"};
    storage.synchronize(package);

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(
        SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1, sourceId2}});
}

TEST_F(ProjectStorage, breaking_extension_chain_by_deleting_base_component_has_unresolved_type_name)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].extension = Storage::Synchronization::ImportedType{"Object"};
    storage.synchronize(package);

    storage.synchronize(
        SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1, sourceId2}});

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasExtensionId(IsUnresolvedTypeId()));
}

TEST_F(ProjectStorage, repairing_extension_chain_by_fixing_base_component)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].extension = Storage::Synchronization::ImportedType{"Object"};
    storage.synchronize(package);
    storage.synchronize(
        SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1, sourceId2}});

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasExtensionId(fetchTypeId(sourceId2, "QObject")));
}

TEST_F(ProjectStorage, synchronize_types_add_property_declarations)
{
    auto package{createSimpleSynchronizationPackage()};

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::propertyDeclarations,
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

TEST_F(ProjectStorage,
       synchronize_adds_property_declarations_with_missing_imports_notifies_unresolved_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.imports.clear();

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Item"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage, synchronize_adds_property_declarations_with_missing_imports_has_null_type_id)
{
    auto package{createSimpleSynchronizationPackage()};
    package.imports.clear();

    storage.synchronize(package);

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"),
                PropertyDeclarations(Contains(HasPropertyTypeId(IsNullTypeId()))));
}

TEST_F(ProjectStorage, synchronize_fixes_unresolved_property_declarations_with_missing_imports)
{
    auto package{createSimpleSynchronizationPackage()};
    package.imports.clear();
    storage.synchronize(package);

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"),
                PropertyDeclarations(
                    Contains(IsPropertyDeclaration("data", fetchTypeId(sourceId2, "QObject")))));
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
                                       TypeTraitsKind::Reference,
                                       sourceId1,
                                       {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                               "Object"}}});

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::propertyDeclarations,
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

TEST_F(ProjectStorage, synchronize_types_changes_property_declaration_type)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "QQuickItem"};

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::propertyDeclarations,
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

TEST_F(ProjectStorage, synchronize_types_changes_declaration_traits)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsPointer;

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(sourceId1, "QQuickItem", fetchTypeId(sourceId2, "QObject"), TypeTraitsKind::Reference),
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
            IsStorageType(sourceId1, "QQuickItem", fetchTypeId(sourceId2, "QObject"), TypeTraitsKind::Reference),
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
                                             TypeTraitsKind::Reference),
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
            IsStorageType(sourceId1, "QQuickItem", fetchTypeId(sourceId2, "QObject"), TypeTraitsKind::Reference),
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

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::propertyDeclarations,
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

TEST_F(ProjectStorage, using_non_existing_property_type_notifies_unresolved_type_name)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations.emplace_back("data",
                                                       Storage::Synchronization::ImportedType{
                                                           "QObject2"},
                                                       Storage::PropertyDeclarationTraits::IsList);

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("QObject2"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage, using_non_existing_property_type_has_null_type_id)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations.emplace_back("data",
                                                       Storage::Synchronization::ImportedType{
                                                           "QObject2"},
                                                       Storage::PropertyDeclarationTraits::IsList);

    storage.synchronize(package);

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"),
                PropertyDeclarations(Contains(HasPropertyTypeId(IsNullTypeId()))));
}

TEST_F(ProjectStorage, resolve_type_id_after_fixing_non_existing_property_type)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations.emplace_back("data",
                                                       Storage::Synchronization::ImportedType{
                                                           "QObject2"},
                                                       Storage::PropertyDeclarationTraits::IsList);
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object"};

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"),
                PropertyDeclarations(Contains(HasPropertyTypeId(fetchTypeId(sourceId2, "QObject")))));
}

TEST_F(ProjectStorage,
       using_non_existing_qualified_exported_property_type_with_wrong_name_notifies_unresovled_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "QObject2", Storage::Import{qmlNativeModuleId, Storage::Version{}, sourceId1}};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("QObject2"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage,
       using_non_existing_qualified_exported_property_type_with_wrong_name_has_null_type_id)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "QObject2", Storage::Import{qmlNativeModuleId, Storage::Version{}, sourceId1}};

    storage.synchronize(package);

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"),
                PropertyDeclarations(Contains(HasPropertyTypeId(IsNullTypeId()))));
}

TEST_F(ProjectStorage, resolve_type_id_after_fixing_non_existing_qualified_property_type)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "QObject2", Storage::Import{qmlNativeModuleId, Storage::Version{}, sourceId1}};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "QObject", Storage::Import{qmlNativeModuleId, Storage::Version{}, sourceId1}};

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"),
                PropertyDeclarations(
                    Contains(IsPropertyDeclaration("data", fetchTypeId(sourceId2, "QObject")))));
}

TEST_F(ProjectStorage,
       using_non_existing_qualified_exported_property_type_with_wrong_module_notifies_unresovled_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = {};
    package.types[0].propertyDeclarations.pop_back();
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlNativeModuleId, Storage::Version{}, sourceId1}};
    package.types.pop_back();
    package.imports = importsSourceId1;

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage,
       using_non_existing_qualified_exported_property_type_with_wrong_module_has_null_type_id)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlNativeModuleId, Storage::Version{}, sourceId1}};
    package.types.pop_back();
    package.imports = importsSourceId1;

    storage.synchronize(package);

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"),
                PropertyDeclarations(Contains(HasPropertyTypeId(IsNullTypeId()))));
}

TEST_F(ProjectStorage, resolve_qualified_exported_property_type_with_fixed_module)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlNativeModuleId, Storage::Version{}, sourceId1}};
    package.types.pop_back();
    package.imports = importsSourceId1;
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};

    storage.synchronize(SynchronizationPackage{importsSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"),
                PropertyDeclarations(Contains(HasPropertyTypeId(IsNullTypeId()))));
}

TEST_F(ProjectStorage,
       breaking_property_declaration_type_dependency_by_deleting_type_nofifies_unresolved_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].prototype = Storage::Synchronization::ImportedType{};
    package.types.pop_back();
    package.imports = importsSourceId1;

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("QObject"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage, breaking_property_declaration_type_dependency_by_deleting_type_has_null_type_id)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].prototype = Storage::Synchronization::ImportedType{};
    package.types.pop_back();
    package.imports = importsSourceId1;

    storage.synchronize(package);

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"),
                PropertyDeclarations(Contains(IsPropertyDeclaration("data", IsNullTypeId()))));
}

TEST_F(ProjectStorage, fixing_broken_property_declaration_type_dependenc_has_valid_id)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].prototype = Storage::Synchronization::ImportedType{};
    auto objectType = package.types.back();
    package.types.pop_back();
    package.imports = importsSourceId1;
    storage.synchronize(package);

    storage.synchronize({{importsSourceId2}, {objectType}, {sourceId2}});

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"),
                PropertyDeclarations(
                    Contains(IsPropertyDeclaration("data", fetchTypeId(sourceId2, "QObject")))));
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                                  TypeTraitsKind::Reference),
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
                                  TypeTraitsKind::Reference),
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
                                  TypeTraitsKind::Reference),
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
                                  TypeTraitsKind::Reference),
                    Field(&Storage::Synchronization::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage,
       synchronize_types_changes_enumeration_declaration_change_enumerator_declaration_name)
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
                                  TypeTraitsKind::Reference),
                    Field(&Storage::Synchronization::Type::enumerationDeclarations,
                          UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                               Eq(package.types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorage,
       synchronize_types_changes_enumeration_declaration_change_enumerator_declaration_value)
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
                                  TypeTraitsKind::Reference),
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
                                  TypeTraitsKind::Reference),
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
                                  TypeTraitsKind::Reference),
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
                                             TypeTraitsKind::Reference),
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
                                  TypeTraitsKind::Reference),
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
    auto qtQuickModuleId = storage.moduleId("QtQuick", ModuleKind::QmlLibrary);

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
                                  TypeTraitsKind::Reference),
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
                                  TypeTraitsKind::Reference),
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
                                  TypeTraitsKind::Reference),
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

TEST_F(ProjectStorage,
       synchronize_types_add_alias_declarations_notifies_unresolved_type_name_for_wrong_type)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "QQuickItemWrong"};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("QQuickItemWrong"), sourceId3));

    storage.synchronize(package);
}

TEST_F(ProjectStorage, synchronize_types_add_alias_declarations_set_null_type_id_for_wrong_type)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "QQuickItemWrong"};

    storage.synchronize(package);

    ASSERT_THAT(fetchType(sourceId3, "QAliasItem"),
                PropertyDeclarations(Contains(IsPropertyDeclaration("items", IsNullTypeId()))));
}

TEST_F(ProjectStorage, synchronize_types_fixes_null_type_id_after_add_alias_declarations_for_wrong_type)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "QQuickItemWrong"};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "QQuickItem"};

    storage.synchronize(
        {moduleDependenciesSourceId3 + importsSourceId3, {package.types[2]}, {sourceId3}});

    ASSERT_THAT(fetchType(sourceId3, "QAliasItem"),
                PropertyDeclarations(
                    Contains(IsPropertyDeclaration("items", fetchTypeId(sourceId1, "QQuickItem")))));
}

TEST_F(ProjectStorage, synchronize_types_add_alias_declarations_notifies_error_for_wrong_property_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[1].aliasPropertyName = "childrenWrong";

    EXPECT_CALL(errorNotifierMock, propertyNameDoesNotExists(Eq("childrenWrong"), sourceId3))
        .Times(AtLeast(1));

    storage.synchronize(
        {package.imports, package.types, {sourceId4}, package.moduleDependencies, {sourceId4}});
}

TEST_F(ProjectStorage,
       synchronize_types_add_alias_declarations_returns_invalid_type_for_wrong_property_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[1].aliasPropertyName = "childrenWrong";

    storage.synchronize(
        {package.imports, package.types, {sourceId4}, package.moduleDependencies, {sourceId4}});

    ASSERT_THAT(fetchType(sourceId3, "QAliasItem"),
                PropertyDeclarations(Contains(IsPropertyDeclaration("items", IsNullTypeId()))));
}

TEST_F(ProjectStorage,
       synchronize_types_update_alias_declarations_returns_item_type_for_fixed_property_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[1].aliasPropertyName = "childrenWrong";
    storage.synchronize(
        {package.imports, package.types, {sourceId4}, package.moduleDependencies, {sourceId4}});
    package.types[2].propertyDeclarations[1].aliasPropertyName = "children";

    storage.synchronize(
        {importsSourceId3 + moduleDependenciesSourceId3, {package.types[2]}, {sourceId3}});

    ASSERT_THAT(fetchType(sourceId3, "QAliasItem"),
                PropertyDeclarations(
                    Contains(IsPropertyDeclaration("items", fetchTypeId(sourceId1, "QQuickItem")))));
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
                                  TypeTraitsKind::Reference),
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
        Contains(
            AllOf(IsStorageType(sourceId3,
                                "QAliasItem",
                                fetchTypeId(sourceId1, "QQuickItem"),
                                TypeTraitsKind::Reference),
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
        Contains(
            AllOf(IsStorageType(sourceId3,
                                "QAliasItem",
                                fetchTypeId(sourceId1, "QQuickItem"),
                                TypeTraitsKind::Reference),
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
                                  TypeTraitsKind::Reference),
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
        Contains(
            AllOf(IsStorageType(sourceId3,
                                "QAliasItem",
                                fetchTypeId(sourceId1, "QQuickItem"),
                                TypeTraitsKind::Reference),
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
                                  TypeTraitsKind::Reference),
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
                                  TypeTraitsKind::Reference),
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

TEST_F(ProjectStorage,
       synchronize_remove_type_with_alias_target_property_declaration_nofifies_unresolved_type_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[2].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId3);
    storage.synchronize(package);

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object2"), sourceId3));

    storage.synchronize(SynchronizationPackage{{sourceId4}});
}

TEST_F(ProjectStorage,
       synchronize_remove_type_with_alias_target_property_declaration_set_unresolved_type_id)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[2].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId3);
    storage.synchronize(package);

    storage.synchronize(SynchronizationPackage{{sourceId4}});

    ASSERT_THAT(fetchType(sourceId3, "QAliasItem"),
                PropertyDeclarations(Contains(IsPropertyDeclaration("objects", IsNullTypeId()))));
}

TEST_F(ProjectStorage, synchronize_fixes_type_with_alias_target_property_declaration)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[2].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId3);
    storage.synchronize(package);
    storage.synchronize(SynchronizationPackage{{sourceId4}});

    storage.synchronize(SynchronizationPackage{importsSourceId4 + moduleDependenciesSourceId4,
                                               {package.types[3]},
                                               {sourceId4}});

    ASSERT_THAT(fetchType(sourceId3, "QAliasItem"),
                PropertyDeclarations(
                    Contains(IsPropertyDeclaration("objects", fetchTypeId(sourceId2, "QObject")))));
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
                                  TypeTraitsKind::Reference),
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
        Contains(
            AllOf(IsStorageType(sourceId3,
                                "QAliasItem",
                                fetchTypeId(sourceId1, "QQuickItem"),
                                TypeTraitsKind::Reference),
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
        Contains(
            AllOf(IsStorageType(sourceId3,
                                "QAliasItem",
                                fetchTypeId(sourceId1, "QQuickItem"),
                                TypeTraitsKind::Reference),
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
                                  TypeTraitsKind::Reference),
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
                                  TypeTraitsKind::Reference),
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

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object2"), sourceId2));

    storage.synchronize(SynchronizationPackage{importsSourceId4 + moduleDependenciesSourceId4,
                                               {package.types[3]},
                                               {sourceId4}});
}

TEST_F(ProjectStorage, not_relinked_alias_property_for_qualified_imported_type_name_has_null_type_id)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object2", Storage::Import{pathToModuleId, Storage::Version{}, sourceId2}};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);
    package.types[3].exportedTypes[0].moduleId = qtQuickModuleId;
    importsSourceId4.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId4);

    storage.synchronize(SynchronizationPackage{importsSourceId4 + moduleDependenciesSourceId4,
                                               {package.types[3]},
                                               {sourceId4}});

    ASSERT_THAT(fetchType(sourceId3, "QAliasItem"),
                PropertyDeclarations(Contains(IsPropertyDeclaration("objects", IsNullTypeId()))));
}

TEST_F(ProjectStorage,
       relinked_alias_property_for_qualified_imported_type_name_after_alias_chain_was_fixed)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object2", Storage::Import{pathToModuleId, Storage::Version{}, sourceId2}};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);
    package.types[3].exportedTypes[0].moduleId = qtQuickModuleId;
    importsSourceId4.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId4);
    storage.synchronize(SynchronizationPackage{importsSourceId4, {package.types[3]}, {sourceId4}});
    package.types[3].exportedTypes[0].moduleId = pathToModuleId;

    storage.synchronize(SynchronizationPackage{importsSourceId4 + moduleDependenciesSourceId4,
                                               {package.types[3]},
                                               {sourceId4}});

    ASSERT_THAT(fetchType(sourceId3, "QAliasItem"),
                PropertyDeclarations(
                    Contains(IsPropertyDeclaration("objects", fetchTypeId(sourceId4, "QObject2")))));
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
        TypeTraitsKind::Reference,
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
                                  TypeTraitsKind::Reference),
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

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(sourceId3,
                                  "QAliasItem",
                                  fetchTypeId(sourceId2, "QObject"),
                                  TypeTraitsKind::Reference),
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
                                           TypeTraitsKind::Reference))));
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
                                           TypeTraitsKind::Reference))));
}

TEST_F(ProjectStorage, do_not_relink_property_type_does_not_exists_notifies_unresolved_type_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object2"), sourceId2));

    storage.synchronize(SynchronizationPackage{{sourceId4}});
}

TEST_F(ProjectStorage, not_relinked_property_type_has_null_type_id)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);

    storage.synchronize({{sourceId4}});

    ASSERT_THAT(fetchType(sourceId2, "QObject"),
                PropertyDeclarations(Contains(IsPropertyDeclaration("objects", IsNullTypeId()))));
}

TEST_F(ProjectStorage, not_relinked_property_type_fixed_after_adding_property_type_again)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);
    storage.synchronize({{sourceId4}});

    storage.synchronize(
        {importsSourceId4 + moduleDependenciesSourceId4, {package.types[3]}, {sourceId4}});

    ASSERT_THAT(fetchType(sourceId2, "QObject"),
                PropertyDeclarations(
                    Contains(IsPropertyDeclaration("objects", fetchTypeId(sourceId4, "QObject2")))));
}

TEST_F(ProjectStorage, not_relinked_property_type_fixed_after_type_is_added_again)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);
    storage.synchronize({{sourceId4}});

    storage.synchronize(
        {importsSourceId4 + moduleDependenciesSourceId4, {package.types[3]}, {sourceId4}});

    ASSERT_THAT(fetchType(sourceId2, "QObject"),
                PropertyDeclarations(
                    Contains(IsPropertyDeclaration("objects", fetchTypeId(sourceId4, "QObject2")))));
}

TEST_F(ProjectStorage, do_not_relink_alias_property_type_does_not_exists_notifies_unresolved_type_name)
{
    auto package{createSynchronizationPackageWithAliases2()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Item"), sourceId3));

    storage.synchronize(SynchronizationPackage{{sourceId1}});
}

TEST_F(ProjectStorage, not_relinked_alias_property_type_has_null_type_id)
{
    auto package{createSynchronizationPackageWithAliases2()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);

    storage.synchronize(SynchronizationPackage{{sourceId1}});

    ASSERT_THAT(fetchType(sourceId3, "QAliasItem"),
                PropertyDeclarations(Contains(IsPropertyDeclaration("objects", IsNullTypeId()))));
}

TEST_F(ProjectStorage, not_relinked_alias_property_type_fixed_after_referenced_type_is_added_again)
{
    auto package{createSynchronizationPackageWithAliases2()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2);
    storage.synchronize(package);
    storage.synchronize({{sourceId1}});

    storage.synchronize(
        {importsSourceId1 + moduleDependenciesSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(fetchType(sourceId3, "QAliasItem"),
                PropertyDeclarations(
                    Contains(IsPropertyDeclaration("objects", fetchTypeId(sourceId4, "QObject2")))));
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
                                       TypeTraitsKind::Reference)));
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
                                       TypeTraitsKind::Reference)));
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
                                       TypeTraitsKind::Reference)));
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
                                       TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage,
       change_qualified_prototype_type_module_id_notifies_that_type_name_cannot_be_resolved)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};
    storage.synchronize(package);
    package.types[1].exportedTypes[0].moduleId = qtQuickModuleId;

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});
}

TEST_F(ProjectStorage, change_qualified_extension_type_module_id_notifies_cannot_resolve)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};
    storage.synchronize(package);
    package.types[1].exportedTypes[0].moduleId = qtQuickModuleId;

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});
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
                                       TypeTraitsKind::Reference)));
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
                                       TypeTraitsKind::Reference)));
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
                                       TypeTraitsKind::Reference)));
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
                                       TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, change_prototype_type_name_throws_for_wrong_native_prototupe_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object"};
    storage.synchronize(package);
    package.types[1].exportedTypes[2].name = "QObject3";
    package.types[1].typeName = "QObject3";

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("QObject"), sourceId1));

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});
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

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("QObject"), sourceId1));

    storage.synchronize(SynchronizationPackage{importsSourceId2, {package.types[1]}, {sourceId2}});
}

TEST_F(ProjectStorage, throw_for_prototype_chain_cycles)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[1].prototype = Storage::Synchronization::ImportedType{"Object2"};
    package.types.push_back(Storage::Synchronization::Type{
        "QObject2",
        Storage::Synchronization::ImportedType{"Item"},
        Storage::Synchronization::ImportedType{},
        TypeTraitsKind::Reference,
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
        TypeTraitsKind::Reference,
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
                                             TypeTraitsKind::Reference),
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
                                             TypeTraitsKind::Reference),
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
                                             TypeTraitsKind::Reference),
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
                                                   TypeTraitsKind::Reference),
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
                                                   TypeTraitsKind::Reference),
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
                                             TypeTraitsKind::Reference),
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
                                       TypeTraitsKind::Reference,
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
                                       TypeTraitsKind::Reference)));
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
                                       TypeTraitsKind::Reference,
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
                                       TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage,
       qualified_prototype_upper_down_the_module_chain_notifies_type_name_cannot_be_resolved)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage,
       qualified_extension_upper_down_the_module_chain_notifies_type_name_cannot_be_resolved)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(package);
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
                                       TypeTraitsKind::Reference,
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
                                       TypeTraitsKind::Reference)));
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
                                       TypeTraitsKind::Reference,
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
                                       TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, qualified_prototype_with_wrong_version_notifies_type_name_cannot_be_resolved)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{4}, sourceId1}};
    package.types.push_back(
        Storage::Synchronization::Type{"QQuickObject",
                                       Storage::Synchronization::ImportedType{},
                                       Storage::Synchronization::ImportedType{},
                                       TypeTraitsKind::Reference,
                                       sourceId3,
                                       {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                               "Object"}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage, qualified_extension_with_wrong_version_notifies_type_name_cannot_be_resolved)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{4}, sourceId1}};
    package.types.push_back(
        Storage::Synchronization::Type{"QQuickObject",
                                       Storage::Synchronization::ImportedType{},
                                       Storage::Synchronization::ImportedType{},
                                       TypeTraitsKind::Reference,
                                       sourceId3,
                                       {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                               "Object"}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(package);
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
                                       TypeTraitsKind::Reference,
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
                                       TypeTraitsKind::Reference)));
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
                                       TypeTraitsKind::Reference,
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
                                       TypeTraitsKind::Reference)));
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
        TypeTraitsKind::Reference,
        sourceId3,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Object", Storage::Version{2}}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       fetchTypeId(sourceId3, "QQuickObject"),
                                       TypeId{},
                                       TypeTraitsKind::Reference)));
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
        TypeTraitsKind::Reference,
        sourceId3,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Object", Storage::Version{2}}}});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3);
    package.updatedSourceIds.push_back(sourceId3);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       TypeId{},
                                       fetchTypeId(sourceId3, "QQuickObject"),
                                       TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage,
       qualified_prototype_with_version_down_the_proto_type_chain_notifies_type_name_cannot_be_resolved)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{2}, sourceId1}};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage,
       qualified_extension_with_version_down_the_proto_type_chain_notifies_type_name_cannot_be_resolved)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qtQuickModuleId, Storage::Version{2}, sourceId1}};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(package);
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
                                       TypeTraitsKind::Reference,
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

TEST_F(ProjectStorage,
       qualified_property_declaration_type_name_down_the_module_chain_nofifies_unresolved_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Obj", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Obj"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage,
       qualified_property_declaration_type_name_down_the_module_chain_has_null_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Obj", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};

    storage.synchronize(package);

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"),
                PropertyDeclarations(Contains(HasPropertyTypeId(IsNullTypeId()))));
}

TEST_F(ProjectStorage, borken_qualified_property_declaration_type_name_down_the_module_chain_fixed)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Obj", Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1}};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Obj", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};

    storage.synchronize(
        {importsSourceId1 + moduleDependenciesSourceId1, {package.types[0]}, {sourceId1}});

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"),
                PropertyDeclarations(Not(Contains(HasPropertyTypeId(IsNullTypeId())))));
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
                                       TypeTraitsKind::Reference,
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

TEST_F(ProjectStorage, change_property_type_module_id_with_qualified_type_notifies_unresolved_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};
    storage.synchronize(package);
    package.types[1].exportedTypes[0].moduleId = qtQuickModuleId;

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(SynchronizationPackage{importsSourceId2 + moduleDependenciesSourceId2,
                                               {package.types[1]},
                                               {sourceId2}});
}

TEST_F(ProjectStorage, change_property_type_module_id_with_qualified_type_sets_type_id_to_null)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};
    storage.synchronize(package);
    package.types[1].exportedTypes[0].moduleId = qtQuickModuleId;

    storage.synchronize(SynchronizationPackage{importsSourceId2 + moduleDependenciesSourceId2,
                                               {package.types[1]},
                                               {sourceId2}});

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"),
                PropertyDeclarations(Contains(HasPropertyTypeId(IsNullTypeId()))));
}

TEST_F(ProjectStorage,
       fixed_broken_change_property_type_module_id_with_qualified_type_sets_fixes_type_idl)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", Storage::Import{qmlModuleId, Storage::Version{}, sourceId1}};
    storage.synchronize(package);
    package.types[1].exportedTypes[0].moduleId = qtQuickModuleId;
    storage.synchronize(SynchronizationPackage{importsSourceId2 + moduleDependenciesSourceId2,
                                               {package.types[1]},
                                               {sourceId2}});
    package.types[1].exportedTypes[0].moduleId = qmlModuleId;

    storage.synchronize(SynchronizationPackage{importsSourceId2 + moduleDependenciesSourceId2,
                                               {package.types[1]},
                                               {sourceId2}});

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"),
                PropertyDeclarations(Not(Contains(HasPropertyTypeId(IsNullTypeId())))));
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

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(sourceId1,
                                             "QQuickItem",
                                             fetchTypeId(sourceId2, "QObject"),
                                             TypeTraitsKind::Reference),
                               Field(&Storage::Synchronization::Type::propertyDeclarations,
                                     Contains(IsPropertyDeclaration(
                                         "data",
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
                                             TypeTraitsKind::Reference),
                               Field(&Storage::Synchronization::Type::exportedTypes,
                                     UnorderedElementsAre(IsExportedType("Object2"),
                                                          IsExportedType("Obj2"))))));
}

TEST_F(ProjectStorage, fetch_by_major_version_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::ImportedType{"Object"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2,
                                        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                                "Item",
                                                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{1}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject"),
                                       TypeTraitsKind::Reference)));
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
        TypeTraitsKind::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject"),
                                       TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, fetch_by_major_version_and_minor_version_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::ImportedType{"Obj"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2,
                                        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                                "Item",
                                                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{1, 2}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject"),
                                       TypeTraitsKind::Reference)));
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
        TypeTraitsKind::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject"),
                                       TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage,
       fetch_by_major_version_and_minor_version_for_imported_type_if_minor_version_is_not_exported_notifies_type_name_cannot_be_resolved)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::ImportedType{"Object"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2,
                                        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                                "Item",
                                                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{1, 1}, sourceId2};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId2));

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});
}

TEST_F(ProjectStorage,
       fetch_by_major_version_and_minor_version_for_qualified_imported_type_if_minor_version_is_not_exported_notifies_type_name_cannot_be_resolved)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{1, 1}, sourceId2};
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::QualifiedImportedType{"Object", import},
        Storage::Synchronization::ImportedType{},
        TypeTraitsKind::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId2));

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});
}

TEST_F(ProjectStorage, fetch_low_minor_version_for_imported_type_notifies_type_name_cannot_be_resolved)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::ImportedType{"Obj"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2,
                                        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                                "Item",
                                                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{1, 1}, sourceId2};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Obj"), sourceId2));

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});
}

TEST_F(ProjectStorage,
       fetch_low_minor_version_for_qualified_imported_type_notifies_type_name_cannot_be_resolved)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{1, 1}, sourceId2};
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::QualifiedImportedType{"Obj", import},
        Storage::Synchronization::ImportedType{},
        TypeTraitsKind::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Obj"), sourceId2));

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});
}

TEST_F(ProjectStorage, fetch_higher_minor_version_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::ImportedType{"Obj"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2,
                                        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                                "Item",
                                                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{1, 3}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject"),
                                       TypeTraitsKind::Reference)));
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
        TypeTraitsKind::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject"),
                                       TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage,
       fetch_different_major_version_for_imported_type_notifies_type_name_cannot_be_resolved)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::ImportedType{"Obj"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2,
                                        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                                "Item",
                                                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{3, 1}, sourceId2};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Obj"), sourceId2));

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});
}

TEST_F(ProjectStorage,
       fetch_different_major_version_for_qualified_imported_type_notifies_type_name_cannot_be_resolved)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{3, 1}, sourceId2};
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::QualifiedImportedType{"Obj", import},
        Storage::Synchronization::ImportedType{},
        TypeTraitsKind::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Obj"), sourceId2));

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});
}

TEST_F(ProjectStorage, fetch_other_type_by_different_version_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::ImportedType{"Obj"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2,
                                        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                                "Item",
                                                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{2, 3}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject2"),
                                       TypeTraitsKind::Reference)));
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
        TypeTraitsKind::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject2"),
                                       TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, fetch_highest_version_for_import_without_version_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::ImportedType{"Obj"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2,
                                        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                                "Item",
                                                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject4"),
                                       TypeTraitsKind::Reference)));
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
        TypeTraitsKind::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject4"),
                                       TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, fetch_highest_version_for_import_with_major_version_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::ImportedType{"Obj"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2,
                                        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                                "Item",
                                                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{2}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject3"),
                                       TypeTraitsKind::Reference)));
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
        TypeTraitsKind::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject3"),
                                       TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, fetch_exported_type_without_version_first_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::ImportedType{"BuiltInObj"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2,
                                        {Storage::Synchronization::ExportedType{qtQuickModuleId,
                                                                                "Item",
                                                                                Storage::Version{}}}};
    Storage::Import import{qmlModuleId, Storage::Version{}, sourceId2};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject"),
                                       TypeTraitsKind::Reference)));
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
        TypeTraitsKind::Reference,
        sourceId2,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item", Storage::Version{}}}};

    storage.synchronize(SynchronizationPackage{{import}, {type}, {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId2,
                                       "Item",
                                       fetchTypeId(sourceId1, "QObject"),
                                       TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, ensure_that_properties_for_removed_types_are_not_anymore_relinked)
{
    Storage::Synchronization::Type type{
        "QObject",
        Storage::Synchronization::ImportedType{""},
        Storage::Synchronization::ImportedType{},
        TypeTraitsKind::Reference,
        sourceId1,
        {Storage::Synchronization::ExportedType{qmlModuleId, "Object", Storage::Version{}}},
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
        TypeTraitsKind::Reference,
        sourceId1,
        {Storage::Synchronization::ExportedType{qtQuickModuleId, "Item", Storage::Version{2, 0}},
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
            AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                  Field(&Storage::Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                             IsExportedType(qmlModuleId, "Obj"),
                                             IsExportedType(qmlNativeModuleId, "QObject")))),
            AllOf(IsStorageType(sourceId1,
                                "QQuickItem",
                                fetchTypeId(sourceId2, "QObject"),
                                TypeTraitsKind::Reference),
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
    auto id = storage.moduleId("Qml", ModuleKind::QmlLibrary);

    ASSERT_TRUE(id);
}

TEST_F(ProjectStorage, get_same_module_id_again)
{
    auto initialId = storage.moduleId("Qml", ModuleKind::QmlLibrary);

    auto id = storage.moduleId("Qml", ModuleKind::QmlLibrary);

    ASSERT_THAT(id, Eq(initialId));
}

TEST_F(ProjectStorage, different_module_kind_returns_different_id)
{
    auto qmlId = storage.moduleId("Qml", ModuleKind::QmlLibrary);

    auto cppId = storage.moduleId("Qml", ModuleKind::CppLibrary);

    ASSERT_THAT(cppId, Ne(qmlId));
}

TEST_F(ProjectStorage, module_throws_if_id_is_invalid)
{
    ASSERT_THROW(storage.module(ModuleId{}), QmlDesigner::ModuleDoesNotExists);
}

TEST_F(ProjectStorage, module_throws_if_id_does_not_exists)
{
    ASSERT_THROW(storage.module(ModuleId::create(222)), QmlDesigner::ModuleDoesNotExists);
}

TEST_F(ProjectStorage, get_module)
{
    auto id = storage.moduleId("Qml", ModuleKind::QmlLibrary);

    auto module = storage.module(id);

    ASSERT_THAT(module, IsModule("Qml", ModuleKind::QmlLibrary));
}

TEST_F(ProjectStorage, populate_module_cache)
{
    auto id = storage.moduleId("Qml", ModuleKind::QmlLibrary);

    QmlDesigner::ProjectStorage newStorage{database, errorNotifierMock, database.isInitialized()};

    ASSERT_THAT(newStorage.module(id), IsModule("Qml", ModuleKind::QmlLibrary));
}

TEST_F(ProjectStorage, add_directory_infoes)
{
    Storage::Synchronization::DirectoryInfo directoryInfo1{qmlProjectSourceId,
                                                       sourceId1,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::DirectoryInfo directoryInfo2{qmlProjectSourceId,
                                                       sourceId2,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::DirectoryInfo directoryInfo3{qtQuickProjectSourceId,
                                                       sourceId3,
                                                       qtQuickModuleId,
                                                       Storage::Synchronization::FileType::QmlTypes};

    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {directoryInfo1, directoryInfo2, directoryInfo3}});

    ASSERT_THAT(storage.fetchDirectoryInfos({qmlProjectSourceId, qtQuickProjectSourceId}),
                UnorderedElementsAre(directoryInfo1, directoryInfo2, directoryInfo3));
}

TEST_F(ProjectStorage, remove_directory_info)
{
    Storage::Synchronization::DirectoryInfo directoryInfo1{qmlProjectSourceId,
                                                       sourceId1,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::DirectoryInfo directoryInfo2{qmlProjectSourceId,
                                                       sourceId2,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::DirectoryInfo directoryInfo3{qtQuickProjectSourceId,
                                                       sourceId3,
                                                       qtQuickModuleId,
                                                       Storage::Synchronization::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {directoryInfo1, directoryInfo2, directoryInfo3}});

    storage.synchronize(
        SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId}, {directoryInfo1}});

    ASSERT_THAT(storage.fetchDirectoryInfos({qmlProjectSourceId, qtQuickProjectSourceId}),
                UnorderedElementsAre(directoryInfo1));
}

TEST_F(ProjectStorage, update_directory_info_file_type)
{
    Storage::Synchronization::DirectoryInfo directoryInfo1{qmlProjectSourceId,
                                                       sourceId1,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::DirectoryInfo directoryInfo2{qmlProjectSourceId,
                                                       sourceId2,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::DirectoryInfo directoryInfo2b{qmlProjectSourceId,
                                                        sourceId2,
                                                        qmlModuleId,
                                                        Storage::Synchronization::FileType::QmlTypes};
    Storage::Synchronization::DirectoryInfo directoryInfo3{qtQuickProjectSourceId,
                                                       sourceId3,
                                                       qtQuickModuleId,
                                                       Storage::Synchronization::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {directoryInfo1, directoryInfo2, directoryInfo3}});

    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {directoryInfo1, directoryInfo2b}});

    ASSERT_THAT(storage.fetchDirectoryInfos({qmlProjectSourceId, qtQuickProjectSourceId}),
                UnorderedElementsAre(directoryInfo1, directoryInfo2b, directoryInfo3));
}

TEST_F(ProjectStorage, update_directory_info_module_id)
{
    Storage::Synchronization::DirectoryInfo directoryInfo1{qmlProjectSourceId,
                                                       sourceId1,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::DirectoryInfo directoryInfo2{qmlProjectSourceId,
                                                       sourceId3,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::DirectoryInfo directoryInfo2b{qmlProjectSourceId,
                                                        sourceId3,
                                                        qtQuickModuleId,
                                                        Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::DirectoryInfo directoryInfo3{qtQuickProjectSourceId,
                                                       sourceId2,
                                                       qtQuickModuleId,
                                                       Storage::Synchronization::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {directoryInfo1, directoryInfo2, directoryInfo3}});

    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {directoryInfo1, directoryInfo2b}});

    ASSERT_THAT(storage.fetchDirectoryInfos({qmlProjectSourceId, qtQuickProjectSourceId}),
                UnorderedElementsAre(directoryInfo1, directoryInfo2b, directoryInfo3));
}

TEST_F(ProjectStorage, throw_for_invalid_source_id_in_directory_info)
{
    Storage::Synchronization::DirectoryInfo directoryInfo1{qmlProjectSourceId,
                                                       SourceId{},
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {directoryInfo1}}),
                 QmlDesigner::DirectoryInfoHasInvalidSourceId);
}

TEST_F(ProjectStorage, insert_directory_info_with_invalid_module_id)
{
    Storage::Synchronization::DirectoryInfo directoryInfo1{qmlProjectSourceId,
                                                       sourceId1,
                                                       ModuleId{},
                                                       Storage::Synchronization::FileType::QmlDocument};

    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {directoryInfo1}});

    ASSERT_THAT(storage.fetchDirectoryInfos({qmlProjectSourceId, qtQuickProjectSourceId}),
                UnorderedElementsAre(directoryInfo1));
}

TEST_F(ProjectStorage, update_directory_info_with_invalid_module_id)
{
    Storage::Synchronization::DirectoryInfo directoryInfo1{qmlProjectSourceId,
                                                       sourceId1,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {directoryInfo1}});
    directoryInfo1.moduleId = ModuleId{};

    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {directoryInfo1}});

    ASSERT_THAT(storage.fetchDirectoryInfos({qmlProjectSourceId, qtQuickProjectSourceId}),
                UnorderedElementsAre(directoryInfo1));
}

TEST_F(ProjectStorage, throw_for_updating_with_invalid_project_source_id_in_directory_info)
{
    Storage::Synchronization::DirectoryInfo directoryInfo1{SourceId{},
                                                       sourceId1,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{{qmlProjectSourceId}, {directoryInfo1}}),
                 QmlDesigner::DirectoryInfoHasInvalidProjectSourceId);
}

TEST_F(ProjectStorage, fetch_directory_infos_by_directory_source_ids)
{
    Storage::Synchronization::DirectoryInfo directoryInfo1{qmlProjectSourceId,
                                                       sourceId1,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::DirectoryInfo directoryInfo2{qmlProjectSourceId,
                                                       sourceId2,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::DirectoryInfo directoryInfo3{qtQuickProjectSourceId,
                                                       sourceId3,
                                                       qtQuickModuleId,
                                                       Storage::Synchronization::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {directoryInfo1, directoryInfo2, directoryInfo3}});

    auto directoryInfos = storage.fetchDirectoryInfos({qmlProjectSourceId, qtQuickProjectSourceId});

    ASSERT_THAT(directoryInfos, UnorderedElementsAre(directoryInfo1, directoryInfo2, directoryInfo3));
}

TEST_F(ProjectStorage, fetch_directory_infos_by_directory_source_id)
{
    Storage::Synchronization::DirectoryInfo directoryInfo1{qmlProjectSourceId,
                                                       sourceId1,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::DirectoryInfo directoryInfo2{qmlProjectSourceId,
                                                       sourceId2,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::DirectoryInfo directoryInfo3{qtQuickProjectSourceId,
                                                       sourceId3,
                                                       qtQuickModuleId,
                                                       Storage::Synchronization::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {directoryInfo1, directoryInfo2, directoryInfo3}});

    auto directoryInfo = storage.fetchDirectoryInfos(qmlProjectSourceId);

    ASSERT_THAT(directoryInfo, UnorderedElementsAre(directoryInfo1, directoryInfo2));
}

TEST_F(ProjectStorage, fetch_directory_infos_by_directory_source_id_and_file_type)
{
    Storage::Synchronization::DirectoryInfo directoryInfo1{
        qmlProjectSourceId, sourceId1, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::DirectoryInfo directoryInfo2{
        qmlProjectSourceId, sourceId2, ModuleId{}, Storage::Synchronization::FileType::Directory};
    Storage::Synchronization::DirectoryInfo directoryInfo3{qtQuickProjectSourceId,
                                                           sourceId3,
                                                           qtQuickModuleId,
                                                           Storage::Synchronization::FileType::QmlTypes};
    Storage::Synchronization::DirectoryInfo directoryInfo4{
        qmlProjectSourceId, sourceId4, ModuleId{}, Storage::Synchronization::FileType::Directory};
    storage.synchronize(
        SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                               {directoryInfo1, directoryInfo2, directoryInfo3, directoryInfo4}});

    auto directoryInfo = storage.fetchDirectoryInfos(qmlProjectSourceId,
                                                     Storage::Synchronization::FileType::Directory);

    ASSERT_THAT(directoryInfo, UnorderedElementsAre(directoryInfo2, directoryInfo4));
}

TEST_F(ProjectStorage, fetch_subdirectory_source_ids)
{
    Storage::Synchronization::DirectoryInfo directoryInfo1{
        qmlProjectSourceId, sourceId1, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::DirectoryInfo directoryInfo2{
        qmlProjectSourceId, sourceId2, ModuleId{}, Storage::Synchronization::FileType::Directory};
    Storage::Synchronization::DirectoryInfo directoryInfo3{qtQuickProjectSourceId,
                                                           sourceId3,
                                                           qtQuickModuleId,
                                                           Storage::Synchronization::FileType::QmlTypes};
    Storage::Synchronization::DirectoryInfo directoryInfo4{
        qmlProjectSourceId, sourceId4, ModuleId{}, Storage::Synchronization::FileType::Directory};
    storage.synchronize(
        SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                               {directoryInfo1, directoryInfo2, directoryInfo3, directoryInfo4}});

    auto directoryInfo = storage.fetchSubdirectorySourceIds(qmlProjectSourceId);

    ASSERT_THAT(directoryInfo, UnorderedElementsAre(sourceId2, sourceId4));
}

TEST_F(ProjectStorage, fetch_directory_info_by_source_ids)
{
    Storage::Synchronization::DirectoryInfo directoryInfo1{qmlProjectSourceId,
                                                       sourceId1,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::DirectoryInfo directoryInfo2{qmlProjectSourceId,
                                                       sourceId2,
                                                       qmlModuleId,
                                                       Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::DirectoryInfo directoryInfo3{qtQuickProjectSourceId,
                                                       sourceId3,
                                                       qtQuickModuleId,
                                                       Storage::Synchronization::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{{qmlProjectSourceId, qtQuickProjectSourceId},
                                               {directoryInfo1, directoryInfo2, directoryInfo3}});

    auto directoryInfo = storage.fetchDirectoryInfo({sourceId2});

    ASSERT_THAT(directoryInfo, Eq(directoryInfo2));
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
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item")))),
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object")))),
                    AllOf(IsStorageType(sourceId3,
                                        "QQuickItem3d",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuick3DModuleId, "Item3D")))),
                    AllOf(IsStorageType(sourceId4,
                                        "MyItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item")))),
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object")))),
                    AllOf(IsStorageType(sourceId3,
                                        "QQuickItem3d",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuick3DModuleId, "Item3D")))),
                    AllOf(IsStorageType(sourceId4,
                                        "MyItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item")))),
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object")))),
                    AllOf(IsStorageType(sourceId3,
                                        "QQuickItem3d",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuick3DModuleId, "Item3D")))),
                    AllOf(IsStorageType(sourceId4,
                                        "MyItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(myModuleModuleId, "MyItem"))))));
}

TEST_F(ProjectStorage,
       module_exported_import_prevent_collision_if_module_is_indirectly_reexported_multiple_times)
{
    ModuleId qtQuick4DModuleId{storage.moduleId("QtQuick4D", ModuleKind::QmlLibrary)};
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
        TypeTraitsKind::Reference,
        sourceId5,
        {Storage::Synchronization::ExportedType{qtQuick4DModuleId, "Item4D", Storage::Version{1, 0}}}});
    package.imports.emplace_back(qtQuick4DModuleId, Storage::Version{1}, sourceId4);

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item")))),
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object")))),
                    AllOf(IsStorageType(sourceId3,
                                        "QQuickItem3d",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuick3DModuleId, "Item3D")))),
                    AllOf(IsStorageType(sourceId5,
                                        "QQuickItem4d",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuick4DModuleId, "Item4D")))),
                    AllOf(IsStorageType(sourceId4,
                                        "MyItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(myModuleModuleId, "MyItem"))))));
}

TEST_F(ProjectStorage, distinguish_between_import_kinds)
{
    ModuleId qml1ModuleId{storage.moduleId("Qml1", ModuleKind::QmlLibrary)};
    ModuleId qml11ModuleId{storage.moduleId("Qml11", ModuleKind::QmlLibrary)};
    auto package{createSimpleSynchronizationPackage()};
    package.moduleDependencies.emplace_back(qmlModuleId, Storage::Version{}, sourceId1);
    package.moduleDependencies.emplace_back(qml1ModuleId, Storage::Version{1}, sourceId1);
    package.imports.emplace_back(qml1ModuleId, Storage::Version{}, sourceId1);
    package.moduleDependencies.emplace_back(qml11ModuleId, Storage::Version{1, 1}, sourceId1);
    package.imports.emplace_back(qml11ModuleId, Storage::Version{1, 1}, sourceId1);

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object"),
                                                     IsExportedType(qmlModuleId, "Obj"),
                                                     IsExportedType(qmlNativeModuleId, "QObject")))),
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item"),
                                                     IsExportedType(qtQuickNativeModuleId,
                                                                    "QQuickItem"))))));
}

TEST_F(ProjectStorage, module_exported_import_distinguish_between_dependency_and_import_re_exports)
{
    auto package{createModuleExportedImportSynchronizationPackage()};
    package.moduleDependencies.emplace_back(qtQuick3DModuleId, Storage::Version{1}, sourceId4);

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    AllOf(IsStorageType(sourceId1,
                                        "QQuickItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item")))),
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object")))),
                    AllOf(IsStorageType(sourceId3,
                                        "QQuickItem3d",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuick3DModuleId, "Item3D")))),
                    AllOf(IsStorageType(sourceId4,
                                        "MyItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuickModuleId, "Item")))),
                    AllOf(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qmlModuleId, "Object")))),
                    AllOf(IsStorageType(sourceId3,
                                        "QQuickItem3d",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQuick3DModuleId, "Item3D")))),
                    AllOf(IsStorageType(sourceId4,
                                        "MyItem",
                                        fetchTypeId(sourceId2, "QObject"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(myModuleModuleId, "MyItem"))))));
}

TEST_F(ProjectStorage, synchronize_types_add_indirect_alias_declarations)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId3,
                                        "QAliasItem",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("items",
                                                          fetchTypeId(sourceId1, "QQuickItem"),
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "objects",
                                        fetchTypeId(sourceId2, "QObject"),
                                        Storage::PropertyDeclarationTraits::IsList
                                            | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorage, synchronize_types_add_indirect_alias_declarations_again)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId3,
                                        "QAliasItem",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("items",
                                                          fetchTypeId(sourceId1, "QQuickItem"),
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "objects",
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
                                             TypeTraitsKind::Reference),
                               Field(&Storage::Synchronization::Type::propertyDeclarations,
                                     UnorderedElementsAre(IsPropertyDeclaration(
                                         "items",
                                         fetchTypeId(sourceId1, "QQuickItem"),
                                         Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorage,
       synchronize_types_add_indirect_alias_declarations_notifies_unresolved_type_name_for_wrong_type_name)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "QQuickItemWrong"};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("QQuickItemWrong"), sourceId3));

    storage.synchronize(SynchronizationPackage{
        importsSourceId3, {package.types[2]}, {sourceId3}, moduleDependenciesSourceId3, {sourceId3}});
}

TEST_F(ProjectStorage,
       synchronize_types_add_indirect_alias_declarations_for_wrong_type_name_sets_null_type_id)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "QQuickItemWrong"};

    storage.synchronize(SynchronizationPackage{
        importsSourceId3, {package.types[2]}, {sourceId3}, moduleDependenciesSourceId3, {sourceId3}});

    auto type = storage.fetchTypeByTypeId(fetchTypeId(sourceId3, "QAliasItem"));
    ASSERT_THAT(fetchType(sourceId3, "QAliasItem"),
                PropertyDeclarations(Contains(IsPropertyDeclaration("objects", IsNullTypeId()))));
}

TEST_F(ProjectStorage,
       fixed_synchronize_types_add_indirect_alias_declarations_for_wrong_type_name_sets_type_id)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "QQuickItemWrong"};
    storage.synchronize(SynchronizationPackage{
        importsSourceId3, {package.types[2]}, {sourceId3}, moduleDependenciesSourceId3, {sourceId3}});
    package.types[2].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "Item"};

    storage.synchronize(SynchronizationPackage{
        importsSourceId3, {package.types[2]}, {sourceId3}, moduleDependenciesSourceId3, {sourceId3}});

    auto type = storage.fetchTypeByTypeId(fetchTypeId(sourceId3, "QAliasItem"));
    ASSERT_THAT(fetchType(sourceId3, "QAliasItem"),
                PropertyDeclarations(Not(Contains(IsPropertyDeclaration("objects", IsNullTypeId())))));
}

TEST_F(ProjectStorage,
       synchronize_types_add_indirect_alias_declarations_notifies_error_for_wrong_property_name)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].aliasPropertyName = "childrenWrong";

    EXPECT_CALL(errorNotifierMock, propertyNameDoesNotExists(Eq("childrenWrong.objects"), sourceId3));

    storage.synchronize(
        {importsSourceId3, {package.types[2]}, {sourceId3}, moduleDependenciesSourceId3, {sourceId3}});
}

TEST_F(ProjectStorage,
       synchronize_types_add_indirect_alias_declarations_returns_null_type_id_for_wrong_property_name)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].aliasPropertyName = "childrenWrong";

    storage.synchronize(
        {importsSourceId3, {package.types[2]}, {sourceId3}, moduleDependenciesSourceId3, {sourceId3}});

    ASSERT_THAT(fetchType(sourceId3, "QAliasItem"),
                PropertyDeclarations(Contains(IsPropertyDeclaration("objects", IsNullTypeId()))));
}

TEST_F(ProjectStorage,
       synchronize_types_updated_indirect_alias_declarations_returns_item_type_id_for_wrong_property_name)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].aliasPropertyName = "childrenWrong";
    storage.synchronize(
        {importsSourceId3, {package.types[2]}, {sourceId3}, moduleDependenciesSourceId3, {sourceId3}});
    package.types[2].propertyDeclarations[1].aliasPropertyName = "children";

    storage.synchronize(
        {importsSourceId3 + moduleDependenciesSourceId3, {package.types[2]}, {sourceId3}});

    ASSERT_THAT(fetchType(sourceId3, "QAliasItem"),
                PropertyDeclarations(
                    Contains(IsPropertyDeclaration("objects", fetchTypeId(sourceId2, "QObject")))));
}

TEST_F(ProjectStorage,
       synchronize_types_add_indirect_alias_declarations_notifies_error_for_wrong_property_name_tail)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].aliasPropertyNameTail = "objectsWrong";

    EXPECT_CALL(errorNotifierMock, propertyNameDoesNotExists(Eq("children.objectsWrong"), sourceId3));

    storage.synchronize(SynchronizationPackage{
        importsSourceId3, {package.types[2]}, {sourceId3}, moduleDependenciesSourceId3, {sourceId3}});
}

TEST_F(ProjectStorage,
       synchronize_types_add_indirect_alias_declarations_sets_property_declaration_id_to_null_for_the_wrong_property_name_tail)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].aliasPropertyNameTail = "objectsWrong";

    storage.synchronize(SynchronizationPackage{
        importsSourceId3, {package.types[2]}, {sourceId3}, moduleDependenciesSourceId3, {sourceId3}});

    ASSERT_THAT(fetchType(sourceId3, "QAliasItem"),
                PropertyDeclarations(Contains(IsPropertyDeclaration("objects", IsNullTypeId()))));
}

TEST_F(ProjectStorage,
       synchronize_types_updates_indirect_alias_declarations_fixed_property_declaration_id_for_property_name_tail)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].aliasPropertyNameTail = "objectsWrong";
    storage.synchronize(
        {importsSourceId3, {package.types[2]}, {sourceId3}, moduleDependenciesSourceId3, {sourceId3}});
    package.types[2].propertyDeclarations[1].aliasPropertyNameTail = "objects";

    storage.synchronize(
        {importsSourceId3 + moduleDependenciesSourceId3, {package.types[2]}, {sourceId3}});

    ASSERT_THAT(fetchType(sourceId3, "QAliasItem"),
                PropertyDeclarations(Contains(
                    IsPropertyDeclaration("objects", Eq(fetchTypeId(sourceId2, "QObject"))))));
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

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId3,
                                        "QAliasItem",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("items",
                                                          fetchTypeId(sourceId1, "QQuickItem"),
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "objects",
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

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId3,
                                        "QAliasItem",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("items",
                                                          fetchTypeId(sourceId1, "QQuickItem"),
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "objects",
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

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId3,
                                        "QAliasItem",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("items",
                                                          fetchTypeId(sourceId1, "QQuickItem"),
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "objects",
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
                                  TypeTraitsKind::Reference),
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

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId3,
                                        "QAliasItem",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("items",
                                                          fetchTypeId(sourceId1, "QQuickItem"),
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "objects",
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

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId3,
                                        "QAliasItem",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("items",
                                                          fetchTypeId(sourceId1, "QQuickItem"),
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "objects",
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
            IsStorageType(sourceId3,
                          "QAliasItem",
                          fetchTypeId(sourceId1, "QQuickItem"),
                          TypeTraitsKind::Reference),
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

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(sourceId3,
                                        "QAliasItem",
                                        fetchTypeId(sourceId1, "QQuickItem"),
                                        TypeTraitsKind::Reference),
                          Field(&Storage::Synchronization::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("items",
                                                          fetchTypeId(sourceId1, "QQuickItem"),
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "objects",
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

TEST_F(ProjectStorage,
       DISABLED_synchronize_types_remove_stem_property_declaration_with_an_indirect_alias_throws)
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
                                             TypeTraitsKind::Reference),
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
                                        TypeTraitsKind::Reference),
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
                Contains(AllOf(HasTypeName(Eq("QQuickItem")), HasDefaultPropertyName(Eq("children")))));
}

TEST_F(ProjectStorage, synchronize_default_property_to_a_different_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.front().defaultPropertyName = "children";
    storage.synchronize(package);
    package.types.front().defaultPropertyName = "data";

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(HasTypeName(Eq("QQuickItem")), HasDefaultPropertyName(Eq("data")))));
}

TEST_F(ProjectStorage, synchronize_to_removed_default_property)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.front().defaultPropertyName = "children";
    storage.synchronize(package);
    package.types.front().defaultPropertyName = {};

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(HasTypeName(Eq("QQuickItem")), HasDefaultPropertyName(IsEmpty()))));
}

TEST_F(ProjectStorage, synchronize_default_property_notifies_error_for_missing_default_property)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.front().defaultPropertyName = "child";

    EXPECT_CALL(errorNotifierMock, missingDefaultProperty(Eq("QQuickItem"), Eq("child"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage, gets_null_default_property_id_for_broken_default_property_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.front().defaultPropertyName = "child";

    storage.synchronize(package);

    ASSERT_THAT(defaultPropertyDeclarationId(sourceId1, "QQuickItem"), IsNullPropertyDeclarationId());
}

TEST_F(ProjectStorage, synchronize_default_fixes_default_property_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.front().defaultPropertyName = "child";
    storage.synchronize(package);
    package.types.front().defaultPropertyName = "data";

    storage.synchronize(package);

    ASSERT_THAT(defaultPropertyDeclarationId(sourceId1, "QQuickItem"),
                Eq(propertyDeclarationId(sourceId1, "QQuickItem", "data")));
}

TEST_F(ProjectStorage,
       synchronize_default_property_notifies_error_for_removing_property_without_changing_default_property)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.front().defaultPropertyName = "children";
    storage.synchronize(package);
    removeProperty(package, "QQuickItem", "children");

    EXPECT_CALL(errorNotifierMock,
                missingDefaultProperty(Eq("QQuickItem"), Eq("children"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage,
       synchronize_default_property_has_null_id_for_removing_property_without_changing_default_property)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.front().defaultPropertyName = "children";
    storage.synchronize(package);
    removeProperty(package, "QQuickItem", "children");

    storage.synchronize(package);

    ASSERT_THAT(defaultPropertyDeclarationId(sourceId1, "QQuickItem"), IsNullPropertyDeclarationId());
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
                Contains(AllOf(HasTypeName(Eq("QQuickItem")), HasDefaultPropertyName(Eq("data")))));
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
                Contains(AllOf(HasTypeName(Eq("QQuickItem")), HasDefaultPropertyName(Eq("data2")))));
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
                Contains(AllOf(HasTypeName(Eq("QQuickItem")), HasDefaultPropertyName(Eq("children")))));
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
                Contains(AllOf(HasTypeName(Eq("QQuickItem")), HasDefaultPropertyName(Eq("children")))));
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
                Contains(AllOf(HasTypeName(Eq("QQuickItem")), HasDefaultPropertyName(Eq("children")))));
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
                Contains(AllOf(HasTypeName(Eq("QQuickItem")), HasDefaultPropertyName(Eq("children")))));
}

TEST_F(ProjectStorage, get_type)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QQuickItem");

    auto type = storage.type(typeId);

    ASSERT_THAT(type, Optional(IsInfoType(sourceId1, TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, dont_get_type_for_invalid_id)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    auto type = storage.type(TypeId());

    ASSERT_THAT(type, Eq(std::nullopt));
}

TEST_F(ProjectStorage, get_default_property_declarartion_id)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QQuickItem");
    auto defaultPropertyName = storage.fetchTypeByTypeId(typeId).defaultPropertyName;
    auto defaultPropertyId = storage.propertyDeclarationId(typeId, defaultPropertyName);

    auto propertyId = storage.defaultPropertyDeclarationId(typeId);

    ASSERT_THAT(propertyId, defaultPropertyId);
}

TEST_F(ProjectStorage, get_default_property_declarartion_id_in_base_type)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    auto baseTypeId = fetchTypeId(sourceId1, "QQuickItem");
    auto defaultPropertyName = storage.fetchTypeByTypeId(baseTypeId).defaultPropertyName;
    auto defaultPropertyId = storage.propertyDeclarationId(baseTypeId, defaultPropertyName);
    auto typeId = fetchTypeId(sourceId3, "QAliasItem");

    auto propertyId = storage.defaultPropertyDeclarationId(typeId);

    ASSERT_THAT(propertyId, defaultPropertyId);
}

TEST_F(ProjectStorage, do_not_get_default_property_declarartion_id_wrong_type_in_property_chain)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].defaultPropertyName = "objects";
    storage.synchronize(package);
    auto baseTypeId = fetchTypeId(sourceId1, "QQuickItem");
    auto defaultPropertyName = storage.fetchTypeByTypeId(baseTypeId).defaultPropertyName;
    auto defaultPropertyId = storage.propertyDeclarationId(baseTypeId, defaultPropertyName);
    auto typeId = fetchTypeId(sourceId3, "QAliasItem");

    auto propertyId = storage.defaultPropertyDeclarationId(typeId);

    ASSERT_THAT(propertyId, defaultPropertyId);
}

TEST_F(ProjectStorage, get_invalid_default_property_declarartion_id_for_invalid_type)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    auto propertyId = storage.defaultPropertyDeclarationId(TypeId());

    ASSERT_FALSE(propertyId);
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
    auto importId2 = storage.importId(Storage::Import{qtQuickModuleId, Storage::Version{}, sourceId1});

    auto importedTypeNameId = storage.importedTypeNameId(importId2, "Item");

    ASSERT_THAT(importedTypeNameId, Not(expectedImportedTypeNameId));
}

TEST_F(ProjectStorage, get_imported_type_name_id_for_import_id_returns_different_id_for_different_name)
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

TEST_F(ProjectStorage,
       synchronize_document_imports_removes_import_notifies_that_type_name_cannot_be_resolved)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1);
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Object"};
    storage.synchronize(package);

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronizeDocumentImports({}, sourceId1);
}

TEST_F(ProjectStorage, synchronize_document_imports_removes_import_which_makes_prototype_unresolved)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1);
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Object"};
    storage.synchronize(package);

    storage.synchronizeDocumentImports({}, sourceId1);

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(IsUnresolvedTypeId()));
}

TEST_F(ProjectStorage, synchronize_document_imports_adds_import_which_makes_prototype_resolved)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Object"};
    storage.synchronize(package);
    Storage::Imports imports;
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1);

    storage.synchronizeDocumentImports(imports, sourceId1);

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(fetchTypeId(sourceId2, "QObject")));
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
    package.propertyEditorQmlPaths.emplace_back(qtQuickModuleId, "Item3D", sourceId3, sourceIdPath6);

    storage.synchronize(package);

    ASSERT_THAT(storage.propertyEditorPathId(fetchTypeId(sourceId2, "QObject")), sourceId1);
    ASSERT_THAT(storage.propertyEditorPathId(fetchTypeId(sourceId1, "QQuickItem")), sourceId2);
    ASSERT_THAT(storage.propertyEditorPathId(fetchTypeId(sourceId3, "QQuickItem3d")), sourceId3);
}

TEST_F(ProjectStorage, synchronize_property_editor_with_non_existing_type_name)
{
    auto package{createPropertyEditorPathsSynchronizationPackage()};
    package.propertyEditorQmlPaths.emplace_back(qtQuickModuleId, "Item4D", sourceId4, sourceIdPath6);

    storage.synchronize(package);

    ASSERT_THAT(storage.propertyEditorPathId(fetchTypeId(sourceId4, "Item4D")), IsFalse());
}

TEST_F(ProjectStorage, call_remove_type_ids_in_observer_after_synchronization)
{
    auto package{createSimpleSynchronizationPackage()};
    ProjectStorageObserverMock observerMock;
    storage.addObserver(&observerMock);
    storage.synchronize(package);
    package.types.clear();

    EXPECT_CALL(observerMock, removedTypeIds(_));

    storage.synchronize(package);
}

TEST_F(ProjectStorage, do_not_synchronize_type_annotations_without_type)
{
    SynchronizationPackage package;
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);

    storage.synchronize(package);

    ASSERT_THAT(storage.allItemLibraryEntries(), IsEmpty());
}

TEST_F(ProjectStorage, synchronize_type_annotation_type_traits)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    TypeTraits traits{TypeTraitsKind::Reference};
    traits.canBeContainer = FlagIs::True;
    traits.visibleInLibrary = FlagIs::True;

    storage.synchronize(package);

    ASSERT_THAT(storage.type(fetchTypeId(sourceId2, "QObject"))->traits, traits);
}

TEST_F(ProjectStorage, synchronize_type_annotation_type_traits_for_prototype_heirs)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.typeAnnotations.pop_back();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    TypeTraits traits{TypeTraitsKind::Reference};
    traits.canBeContainer = FlagIs::True;
    traits.visibleInLibrary = FlagIs::True;

    storage.synchronize(package);

    ASSERT_THAT(storage.type(fetchTypeId(sourceId1, "QQuickItem"))->traits, traits);
}

TEST_F(ProjectStorage, synchronize_updates_type_annotation_type_traits)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    SynchronizationPackage annotationPackage;
    annotationPackage.typeAnnotations = createTypeAnnotions();
    annotationPackage.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    TypeTraits traits{TypeTraitsKind::Reference};
    traits.canBeContainer = FlagIs::True;
    traits.visibleInLibrary = FlagIs::True;

    storage.synchronize(annotationPackage);

    ASSERT_THAT(storage.type(fetchTypeId(sourceId2, "QObject"))->traits, traits);
}

TEST_F(ProjectStorage, synchronize_updates_type_annotation_type_traits_for_prototype_heirs)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    SynchronizationPackage annotationPackage;
    annotationPackage.typeAnnotations = createTypeAnnotions();
    annotationPackage.typeAnnotations.pop_back();
    annotationPackage.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    TypeTraits traits{TypeTraitsKind::Reference};
    traits.canBeContainer = FlagIs::True;
    traits.visibleInLibrary = FlagIs::True;

    storage.synchronize(annotationPackage);

    ASSERT_THAT(storage.type(fetchTypeId(sourceId1, "QQuickItem"))->traits, traits);
}

TEST_F(ProjectStorage, synchronize_clears_annotation_type_traits_if_annotation_was_removed)
{

}

TEST_F(ProjectStorage,
       synchronize_clears_annotation_type_traits_if_annotation_was_removed_for_prototype_heirs)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    package.typeAnnotations[0].traits.isStackedContainer = FlagIs::True;
    storage.synchronize(package);
    package.typeAnnotations.pop_back();

    storage.synchronize(package);

    ASSERT_THAT(storage.type(fetchTypeId(sourceId1, "QQuickItem"))->traits,
                package.typeAnnotations[0].traits);
}

TEST_F(ProjectStorage, synchronize_updates_annotation_type_traits)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    storage.synchronize(package);
    TypeTraits traits{TypeTraitsKind::Value};
    package.types[1].traits.kind = TypeTraitsKind::Value;
    package.typeAnnotations.clear();

    storage.synchronize(package);

    ASSERT_THAT(storage.type(fetchTypeId(sourceId2, "QObject"))->traits, traits);
}

TEST_F(ProjectStorage, synchronize_type_annotation_type_icon_path)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);

    storage.synchronize(package);

    ASSERT_THAT(storage.typeIconPath(fetchTypeId(sourceId2, "QObject")), Eq("/path/to/icon.png"));
}

TEST_F(ProjectStorage, synchronize_removes_type_annotation_type_icon_path)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    storage.synchronize(package);
    package.typeAnnotations.clear();

    storage.synchronize(package);

    ASSERT_THAT(storage.typeIconPath(fetchTypeId(sourceId2, "QObject")), IsEmpty());
}

TEST_F(ProjectStorage, synchronize_updates_type_annotation_type_icon_path)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    storage.synchronize(package);
    package.typeAnnotations[0].iconPath = "/path/to/icon2.png";

    storage.synchronize(package);

    ASSERT_THAT(storage.typeIconPath(fetchTypeId(sourceId2, "QObject")), Eq("/path/to/icon2.png"));
}

TEST_F(ProjectStorage, return_empty_path_if_no_type_icon_exists)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    auto iconPath = storage.typeIconPath(fetchTypeId(sourceId2, "QObject"));

    ASSERT_THAT(iconPath, IsEmpty());
}

TEST_F(ProjectStorage, return_empty_path_if_that_type_does_not_exists)
{
    auto iconPath = storage.typeIconPath(fetchTypeId(sourceId2, "QObject"));

    ASSERT_THAT(iconPath, IsEmpty());
}

TEST_F(ProjectStorage, synchronize_type_hints)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);

    storage.synchronize(package);

    ASSERT_THAT(storage.typeHints(fetchTypeId(sourceId1, "QQuickItem")),
                UnorderedElementsAre(IsTypeHint("canBeContainer", "true"),
                                     IsTypeHint("forceClip", "false")));
}

TEST_F(ProjectStorage, synchronize_removes_type_hints)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    storage.synchronize(package);
    package.typeAnnotations.clear();

    storage.synchronize(package);

    ASSERT_THAT(storage.typeHints(fetchTypeId(sourceId2, "QObject")), IsEmpty());
}

TEST_F(ProjectStorage, return_empty_type_hints_if_no_type_hints_exists)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.typeAnnotations[0].hintsJson.clear();
    storage.synchronize(package);

    auto typeHints = storage.typeHints(fetchTypeId(sourceId2, "QObject"));

    ASSERT_THAT(typeHints, IsEmpty());
}

TEST_F(ProjectStorage, return_empty_type_hints_if_no_type_annotaion_exists)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    auto typeHints = storage.typeHints(fetchTypeId(sourceId2, "QObject"));

    ASSERT_THAT(typeHints, IsEmpty());
}

TEST_F(ProjectStorage, return_empty_type_hints_if_type_does_not_exists)
{
    auto typeHints = storage.typeHints(fetchTypeId(sourceId2, "QObject"));

    ASSERT_THAT(typeHints, IsEmpty());
}

TEST_F(ProjectStorage, synchronize_item_library_entries)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);

    storage.synchronize(package);

    ASSERT_THAT(
        storage.allItemLibraryEntries(),
        UnorderedElementsAre(
            IsItemLibraryEntry(fetchTypeId(sourceId2, "QObject"),
                               "Object",
                               "Foo",
                               "/path/icon",
                               "Basic Items",
                               "QtQuick",
                               "Foo is a Item",
                               "/path/templates/item.qml",
                               UnorderedElementsAre(IsItemLibraryProperty("x", "double", 32.1),
                                                    IsItemLibraryProperty("y", "double", 12.3)),
                               UnorderedElementsAre("/path/templates/frame.png",
                                                    "/path/templates/frame.frag")),
            IsItemLibraryEntry(fetchTypeId(sourceId2, "QObject"),
                               "Object",
                               "Bar",
                               "/path/icon2",
                               "Basic Items",
                               "QtQuick",
                               "Bar is a Item",
                               "",
                               UnorderedElementsAre(IsItemLibraryProperty("color", "color", "#blue")),
                               IsEmpty()),
            IsItemLibraryEntry(fetchTypeId(sourceId1, "QQuickItem"),
                               "Item",
                               "Item",
                               "/path/icon3",
                               "Advanced Items",
                               "QtQuick",
                               "Item is an Object",
                               "",
                               UnorderedElementsAre(IsItemLibraryProperty("x", "double", 1),
                                                    IsItemLibraryProperty("y", "double", 2)),
                               IsEmpty())));
}

TEST_F(ProjectStorage, synchronize_removes_item_library_entries)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    storage.synchronize(package);
    package.typeAnnotations.clear();

    storage.synchronize(package);

    ASSERT_THAT(storage.allItemLibraryEntries(), IsEmpty());
}

TEST_F(ProjectStorage, synchronize_updates_item_library_entries)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    storage.synchronize(package);
    package.typeAnnotations[0].itemLibraryJson
        = R"xy([{"name":"Foo","iconPath":"/path/icon","category":"Basic Items", "import":"QtQuick","toolTip":"Foo is a Item", "properties":[["x", "double", 32.1], ["y", "double", 12.3]]}])xy";

    storage.synchronize(package);

    ASSERT_THAT(storage.itemLibraryEntries(fetchTypeId(sourceId2, "QObject")),
                ElementsAre(
                    IsItemLibraryEntry(fetchTypeId(sourceId2, "QObject"),
                                       "Object",
                                       "Foo",
                                       "/path/icon",
                                       "Basic Items",
                                       "QtQuick",
                                       "Foo is a Item",
                                       "",
                                       UnorderedElementsAre(IsItemLibraryProperty("x", "double", 32.1),
                                                            IsItemLibraryProperty("y", "double", 12.3)),
                                       IsEmpty())));
}

TEST_F(ProjectStorage, synchronize_updates_item_library_entries_with_empty_entries)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    storage.synchronize(package);
    package.typeAnnotations[0].itemLibraryJson.clear();

    storage.synchronize(package);

    ASSERT_THAT(storage.itemLibraryEntries(fetchTypeId(sourceId2, "QObject")), IsEmpty());
}

TEST_F(ProjectStorage, synchronize_type_annotation_directory_source_id)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);

    storage.synchronize(package);

    ASSERT_THAT(storage.typeAnnotationSourceIds(sourceIdPath6),
                UnorderedElementsAre(sourceId4, sourceId5));
}

TEST_F(ProjectStorage, get_type_annotation_source_ids)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    storage.synchronize(package);

    auto sourceIds = storage.typeAnnotationSourceIds(sourceIdPath6);

    ASSERT_THAT(sourceIds, UnorderedElementsAre(sourceId4, sourceId5));
}

TEST_F(ProjectStorage, get_type_annotation_directory_source_ids)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createExtendedTypeAnnotations();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    storage.synchronize(package);

    auto sourceIds = storage.typeAnnotationDirectorySourceIds();

    ASSERT_THAT(sourceIds, ElementsAre(sourceIdPath1, sourceIdPath6));
}

TEST_F(ProjectStorage, get_all_item_library_entries)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    storage.synchronize(package);

    auto entries = storage.allItemLibraryEntries();

    ASSERT_THAT(
        entries,
        UnorderedElementsAre(
            IsItemLibraryEntry(fetchTypeId(sourceId2, "QObject"),
                               "Object",
                               "Foo",
                               "/path/icon",
                               "Basic Items",
                               "QtQuick",
                               "Foo is a Item",
                               "/path/templates/item.qml",
                               UnorderedElementsAre(IsItemLibraryProperty("x", "double", 32.1),
                                                    IsItemLibraryProperty("y", "double", 12.3)),
                               UnorderedElementsAre("/path/templates/frame.png",
                                                    "/path/templates/frame.frag")),
            IsItemLibraryEntry(fetchTypeId(sourceId2, "QObject"),
                               "Object",
                               "Bar",
                               "/path/icon2",
                               "Basic Items",
                               "QtQuick",
                               "Bar is a Item",
                               "",
                               UnorderedElementsAre(IsItemLibraryProperty("color", "color", "#blue")),
                               IsEmpty()),
            IsItemLibraryEntry(fetchTypeId(sourceId1, "QQuickItem"),
                               "Item",
                               "Item",
                               "/path/icon3",
                               "Advanced Items",
                               "QtQuick",
                               "Item is an Object",
                               "",
                               UnorderedElementsAre(IsItemLibraryProperty("x", "double", 1),
                                                    IsItemLibraryProperty("y", "double", 2)),
                               IsEmpty())));
}

TEST_F(ProjectStorage, get_all_item_library_entries_handles_no_entries)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.typeAnnotations[0].itemLibraryJson.clear();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    storage.synchronize(package);

    auto entries = storage.allItemLibraryEntries();

    ASSERT_THAT(entries,
                UnorderedElementsAre(
                    IsItemLibraryEntry(fetchTypeId(sourceId1, "QQuickItem"),
                                       "Item",
                                       "Item",
                                       "/path/icon3",
                                       "Advanced Items",
                                       "QtQuick",
                                       "Item is an Object",
                                       "",
                                       UnorderedElementsAre(IsItemLibraryProperty("x", "double", 1),
                                                            IsItemLibraryProperty("y", "double", 2)),
                                       IsEmpty())));
}

TEST_F(ProjectStorage, get_item_library_entries_by_type_id)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId2, "QObject");

    auto entries = storage.itemLibraryEntries(typeId);

    ASSERT_THAT(
        entries,
        UnorderedElementsAre(
            IsItemLibraryEntry(fetchTypeId(sourceId2, "QObject"),
                               "Object",
                               "Foo",
                               "/path/icon",
                               "Basic Items",
                               "QtQuick",
                               "Foo is a Item",
                               "/path/templates/item.qml",
                               UnorderedElementsAre(IsItemLibraryProperty("x", "double", 32.1),
                                                    IsItemLibraryProperty("y", "double", 12.3)),
                               UnorderedElementsAre("/path/templates/frame.png",
                                                    "/path/templates/frame.frag")),
            IsItemLibraryEntry(fetchTypeId(sourceId2, "QObject"),
                               "Object",
                               "Bar",
                               "/path/icon2",
                               "Basic Items",
                               "QtQuick",
                               "Bar is a Item",
                               "",
                               UnorderedElementsAre(IsItemLibraryProperty("color", "color", "#blue")),
                               IsEmpty())));
}

TEST_F(ProjectStorage, get_no_item_library_entries_if_type_id_is_invalid)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    storage.synchronize(package);

    auto entries = storage.itemLibraryEntries(TypeId());

    ASSERT_THAT(entries, IsEmpty());
}

TEST_F(ProjectStorage, get_no_item_library_entries_by_type_id_for_no_entries)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.typeAnnotations[0].itemLibraryJson.clear();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId2, "QObject");

    auto entries = storage.itemLibraryEntries(typeId);

    ASSERT_THAT(entries, IsEmpty());
}

TEST_F(ProjectStorage, get_item_library_entries_by_source_id)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    storage.synchronize(package);

    auto entries = storage.itemLibraryEntries(sourceId2);

    ASSERT_THAT(
        entries,
        UnorderedElementsAre(
            IsItemLibraryEntry(fetchTypeId(sourceId2, "QObject"),
                               "Object",
                               "Foo",
                               "/path/icon",
                               "Basic Items",
                               "QtQuick",
                               "Foo is a Item",
                               "/path/templates/item.qml",
                               UnorderedElementsAre(IsItemLibraryProperty("x", "double", 32.1),
                                                    IsItemLibraryProperty("y", "double", 12.3)),
                               UnorderedElementsAre("/path/templates/frame.png",
                                                    "/path/templates/frame.frag")),
            IsItemLibraryEntry(fetchTypeId(sourceId2, "QObject"),
                               "Object",
                               "Bar",
                               "/path/icon2",
                               "Basic Items",
                               "QtQuick",
                               "Bar is a Item",
                               "",
                               UnorderedElementsAre(IsItemLibraryProperty("color", "color", "#blue")),
                               IsEmpty())));
}

TEST_F(ProjectStorage, get_no_item_library_entries_by_source_id_for_no_entries)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.typeAnnotations[0].itemLibraryJson.clear();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    storage.synchronize(package);

    auto entries = storage.itemLibraryEntries(sourceId2);

    ASSERT_THAT(entries, IsEmpty());
}

TEST_F(ProjectStorage, return_type_ids_for_module_id)
{
    auto package{createBuiltinSynchronizationPackage()};
    storage.synchronize(package);

    auto typeIds = storage.typeIds(QMLModuleId);

    ASSERT_THAT(typeIds,
                UnorderedElementsAre(fetchTypeId(sourceId1, "bool"),
                                     fetchTypeId(sourceId1, "int"),
                                     fetchTypeId(sourceId1, "double"),
                                     fetchTypeId(sourceId1, "date"),
                                     fetchTypeId(sourceId1, "string"),
                                     fetchTypeId(sourceId1, "url"),
                                     fetchTypeId(sourceId1, "var")));
}

TEST_F(ProjectStorage, get_hair_ids)
{
    auto package{createHeirPackage()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject");

    auto heirIds = storage.heirIds(typeId);

    ASSERT_THAT(heirIds,
                UnorderedElementsAre(fetchTypeId(sourceId1, "QObject2"),
                                     fetchTypeId(sourceId1, "QObject3"),
                                     fetchTypeId(sourceId1, "QObject4"),
                                     fetchTypeId(sourceId1, "QObject5")));
}

TEST_F(ProjectStorage, get_no_hair_ids_for_invalid_type_id)
{
    auto package{createHeirPackage()};
    storage.synchronize(package);
    auto typeId = TypeId{};

    auto heirIds = storage.heirIds(typeId);

    ASSERT_THAT(heirIds, IsEmpty());
}

TEST_F(ProjectStorage,
       removed_document_import_notifies_for_prototypes_that_type_name_cannot_be_resolved)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.moduleDependencies.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId1);
    package.types[0].prototype = Storage::Synchronization::ImportedType{"QObject"};
    storage.synchronize(package);
    package.moduleDependencies.clear();
    package.types.clear();
    package.updatedSourceIds.clear();
    package.updatedModuleDependencySourceIds = {sourceId1};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("QObject"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage,
       removed_document_import_notifies_for_extensions_that_type_name_cannot_be_resolved)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.moduleDependencies.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId1);
    package.types[0].extension = Storage::Synchronization::ImportedType{"QObject"};
    storage.synchronize(package);
    package.moduleDependencies.clear();
    package.types.clear();
    package.updatedSourceIds.clear();
    package.updatedModuleDependencySourceIds = {sourceId1};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("QObject"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage, removed_document_import_changes_prototype_to_unresolved)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.moduleDependencies.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId1);
    package.types[0].prototype = Storage::Synchronization::ImportedType{"QObject"};
    storage.synchronize(package);
    package.moduleDependencies.clear();
    package.types.clear();
    package.updatedSourceIds.clear();
    package.updatedModuleDependencySourceIds = {sourceId1};

    storage.synchronize(package);

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(IsUnresolvedTypeId()));
}

TEST_F(ProjectStorage, removed_document_import_changes_extension_to_unresolved)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.moduleDependencies.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId1);
    package.types[0].extension = Storage::Synchronization::ImportedType{"QObject"};
    storage.synchronize(package);
    package.moduleDependencies.clear();
    package.types.clear();
    package.updatedSourceIds.clear();
    package.updatedModuleDependencySourceIds = {sourceId1};

    storage.synchronize(package);

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasExtensionId(IsUnresolvedTypeId()));
}

TEST_F(ProjectStorage, added_document_import_fixes_unresolved_prototype)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"QObject"};
    storage.synchronize(package);
    package.moduleDependencies.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId1);
    package.types.clear();
    package.updatedSourceIds.clear();
    package.updatedModuleDependencySourceIds = {sourceId1};

    storage.synchronize(package);

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(fetchTypeId(sourceId2, "QObject")));
}

TEST_F(ProjectStorage, added_document_import_fixes_unresolved_extension)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].extension = Storage::Synchronization::ImportedType{"QObject"};
    storage.synchronize(package);
    package.moduleDependencies.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId1);
    package.types.clear();
    package.updatedSourceIds.clear();
    package.updatedModuleDependencySourceIds = {sourceId1};

    storage.synchronize(package);

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasExtensionId(fetchTypeId(sourceId2, "QObject")));
}

} // namespace
