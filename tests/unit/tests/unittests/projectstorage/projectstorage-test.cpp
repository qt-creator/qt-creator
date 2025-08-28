// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <matchers/info_exportedtypenames-matcher.h>
#include <matchers/projectstorage-matcher.h>
#include <matchers/version-matcher.h>
#include <projectstorageerrornotifiermock.h>
#include <projectstorageobservermock.h>

#include <modelnode.h>
#include <projectstorage/projectstorage.h>
#include <sourcepathstorage/sourcepathcache.h>
#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitewritestatement.h>

#include <random>

namespace {

using QmlDesigner::DirectoryPathId;
using QmlDesigner::DirectoryPathIds;
using QmlDesigner::FileNameId;
using QmlDesigner::FileStatus;
using QmlDesigner::FileStatuses;
using QmlDesigner::FlagIs;
using QmlDesigner::ModuleId;
using QmlDesigner::PropertyDeclarationId;
using QmlDesigner::SourceId;
using QmlDesigner::SourceIds;
using QmlDesigner::Storage::ModuleKind;
using QmlDesigner::Storage::Synchronization::SynchronizationPackage;
using QmlDesigner::Storage::Synchronization::TypeAnnotations;
using QmlDesigner::Storage::TypeTraits;
using QmlDesigner::Storage::TypeTraitsKind;
using QmlDesigner::Storage::VersionNumber;
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

auto IsStorageType(const auto &sourceIdMatcher, const auto &typeName, const auto &traitsMatcher)
{
    return AllOf(Field("Type::sourceId", &Storage::Synchronization::Type::sourceId, sourceIdMatcher),
                 Field("Type::typeName", &Storage::Synchronization::Type::typeName, typeName),
                 Field("Type::typeName", &Storage::Synchronization::Type::traits, traitsMatcher));
}

auto IsExportedTypeName(const auto &moduleIdMatcher, const auto &nameMatcher, const auto &versionMatcher)
{
    return AllOf(Field("ExportedTypeName::moduleId",
                       &QmlDesigner::Storage::Info::ExportedTypeName::moduleId,
                       moduleIdMatcher),
                 Field("ExportedTypeName::name",
                       &QmlDesigner::Storage::Info::ExportedTypeName::name,
                       nameMatcher),
                 Field("ExportedTypeName::version",
                       &QmlDesigner::Storage::Info::ExportedTypeName::version,
                       versionMatcher));
}

auto IsExportedTypeName(const auto &moduleIdMatcher,
                        const auto &nameMatcher,
                        const auto &versionMatcher,
                        const auto &typeIdMatcher)
{
    return AllOf(Field("ExportedTypeName::moduleId",
                       &QmlDesigner::Storage::Info::ExportedTypeName::moduleId,
                       moduleIdMatcher),
                 Field("ExportedTypeName::name",
                       &QmlDesigner::Storage::Info::ExportedTypeName::name,
                       nameMatcher),
                 Field("ExportedTypeName::version",
                       &QmlDesigner::Storage::Info::ExportedTypeName::version,
                       versionMatcher),
                 Field("ExportedTypeName::typeId",
                       &QmlDesigner::Storage::Info::ExportedTypeName::typeId,
                       typeIdMatcher));
}

auto IsInfoPropertyDeclaration(const auto &nameMatcher,
                               const auto &propertyTypeIdMatcher,
                               const auto &traitsMatcher)
{
    return AllOf(Field("PropertyDeclaration::name",
                       &Storage::Info::PropertyDeclaration::name,
                       nameMatcher),
                 Field("PropertyDeclaration::propertyTypeId",
                       &Storage::Info::PropertyDeclaration::propertyTypeId,
                       propertyTypeIdMatcher),
                 Field("PropertyDeclaration::traits",
                       &Storage::Info::PropertyDeclaration::traits,
                       traitsMatcher));
}

template<typename PropertyTypeIdMatcher, typename TraitsMatcher>
auto IsInfoPropertyDeclaration(TypeId typeId,
                               Utils::SmallStringView name,
                               TraitsMatcher traitsMatcher,
                               const PropertyTypeIdMatcher &propertyTypeIdMatcher)
{
    return AllOf(Field("PropertyDeclaration::typeId", &Storage::Info::PropertyDeclaration::typeId, typeId),
                 Field("PropertyDeclaration::name", &Storage::Info::PropertyDeclaration::name, name),
                 Field("PropertyDeclaration::propertyTypeId",
                       &Storage::Info::PropertyDeclaration::propertyTypeId,
                       propertyTypeIdMatcher),
                 Field("PropertyDeclaration::traits",
                       &Storage::Info::PropertyDeclaration::traits,
                       traitsMatcher));
}

auto IsUnresolvedTypeId()
{
    return Property("TypeId::internalId", &QmlDesigner::TypeId::internalId, -1);
}

auto IsNullTypeId()
{
    return Property("TypeId::isNull", &QmlDesigner::TypeId::isNull, true);
}

auto IsNullModuleId()
{
    return Property("TypeId::isNull", &QmlDesigner::ModuleId::isNull, true);
}

auto IsNullPropertyDeclarationId()
{
    return Property("PropertyDeclarationId::isNull", &QmlDesigner::PropertyDeclarationId::isNull, true);
}

template<typename Matcher>
auto HasPrototypeId(const Matcher &matcher)
{
    return Field("Type::prototypeId", &Storage::Synchronization::Type::prototypeId, matcher);
}

template<typename Matcher>
auto HasExtensionId(const Matcher &matcher)
{
    return Field("Type::extensionId", &Storage::Synchronization::Type::extensionId, matcher);
}

auto HasInfoPropertyTypeId(const auto &matcher)
{
    return Field("PropertyDeclaration::propertyTypeId",
                 &Storage::Info::PropertyDeclaration::propertyTypeId,
                 matcher);
}

template<typename Matcher>
auto HasTypeName(const Matcher &matcher)
{
    return Field("Type::typeName", &Storage::Synchronization::Type::typeName, matcher);
}

template<typename Matcher>
auto HasDefaultPropertyName(const Matcher &matcher)
{
    return Field("Type::defaultPropertyName",
                 &Storage::Synchronization::Type::defaultPropertyName,
                 matcher);
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
        Sqlite::Database sourcePathDatabase{":memory:", Sqlite::JournalMode::Memory};
        Sqlite::Database modulesDatabase{":memory:", Sqlite::JournalMode::Memory};
        QmlDesigner::ModulesStorage modulesStorage{modulesDatabase, modulesDatabase.isInitialized()};
        NiceMock<ProjectStorageErrorNotifierMock> errorNotifierMock;
        QmlDesigner::ProjectStorage storage{database,
                                            errorNotifierMock,
                                            modulesStorage,
                                            database.isInitialized()};
        QmlDesigner::SourcePathStorage sourcePathStorage{sourcePathDatabase,
                                                         sourcePathDatabase.isInitialized()};
    };

    static void SetUpTestSuite() { staticData = std::make_unique<StaticData>(); }

    static void TearDownTestSuite() { staticData.reset(); }

    ProjectStorage() { storage.setErrorNotifier(errorNotifierMock); }

    ~ProjectStorage() { storage.resetForTestsOnly(); }

    auto createVerySimpleSynchronizationPackage()
    {
        SynchronizationPackage package;

        importsSourceId1.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1);
        importsSourceId1.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1, sourceId1);
        package.imports = importsSourceId1;
        package.updatedImportSourceIds.push_back(sourceId1);

        package.types.emplace_back("QQuickItem",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Reference,
                                   sourceId1);
        package.exportedTypes.emplace_back(
            qmldir1SourceId, qtQuickModuleId, "Item", Storage::Version{}, sourceId1, "QQuickItem");
        package.exportedTypes.emplace_back(qmldir1DirectorySourceId,
                                           qtQuickNativeModuleId,
                                           "Item",
                                           Storage::Version{},
                                           sourceId1,
                                           "QQuickItem");

        package.types.emplace_back("QObject",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Reference,
                                   sourceId2);
        package.exportedTypes.emplace_back(
            qmldir2SourceId, qmlModuleId, "Object", Storage::Version{}, sourceId2, "QObject");
        package.exportedTypes.emplace_back(qmldir2DirectorySourceId,
                                           qmlNativeModuleId,
                                           "QObject",
                                           Storage::Version{},
                                           sourceId2,
                                           "QObject");

        package.updatedTypeSourceIds = {sourceId1, sourceId2};
        package.updatedExportedTypeSourceIds = {qmldir1SourceId,
                                                qmldir1DirectorySourceId,
                                                qmldir2SourceId,
                                                qmldir2DirectorySourceId};

        return package;
    }

    auto createSimpleSynchronizationPackage()
    {
        SynchronizationPackage package;

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1);
        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1, "Qml");
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1, sourceId1);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1, sourceId1, "Quick");
        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId2, sourceId2);
        package.updatedImportSourceIds.emplace_back(sourceId1);
        package.updatedImportSourceIds.emplace_back(sourceId2);

        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId1,
                                                qmldir1SourceId);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId,
                                                Storage::Version{},
                                                sourceId1,
                                                qmldir1SourceId);
        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId2,
                                                qmldir2SourceId);
        package.updatedModuleDependencySourceIds.push_back(qmldir1SourceId);
        package.updatedModuleDependencySourceIds.push_back(qmldir2SourceId);

        importsSourceId1.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1);
        importsSourceId1.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1, "Qml");
        importsSourceId1.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1, sourceId1);
        importsSourceId1.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1, sourceId1, "Quick");

        moduleDependenciesSourceId1.emplace_back(qmlNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId1,
                                                 qmldir1SourceId);
        moduleDependenciesSourceId1.emplace_back(qtQuickNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId1,
                                                 qmldir1SourceId);

        importsSourceId2.emplace_back(qmlModuleId, Storage::Version{}, sourceId2, sourceId2);
        moduleDependenciesSourceId2.emplace_back(qmlNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId2,
                                                 qmldir2SourceId);

        package.types.push_back(Storage::Synchronization::Type{
            "QQuickItem",
            Storage::Synchronization::ImportedType{"QObject"},
            Storage::Synchronization::ImportedType{},
            TypeTraitsKind::Reference,
            sourceId1,
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
            "data"});
        package.exportedTypes.emplace_back(
            qmldir1SourceId, qtQuickModuleId, "Item", Storage::Version{}, sourceId1, "QQuickItem");
        package.exportedTypes.emplace_back(qmldir1DirectorySourceId,
                                           qtQuickNativeModuleId,
                                           "QQuickItem",
                                           Storage::Version{},
                                           sourceId1,
                                           "QQuickItem");
        package.types.push_back(
            Storage::Synchronization::Type{"QObject",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraitsKind::Reference,
                                           sourceId2});
        package.exportedTypes.emplace_back(
            qmldir2SourceId, qmlModuleId, "Object", Storage::Version{2}, sourceId2, "QObject");
        package.exportedTypes.emplace_back(
            qmldir2SourceId, qmlModuleId, "Obj", Storage::Version{2}, sourceId2, "QObject");
        package.exportedTypes.emplace_back(qmldir2DirectorySourceId,
                                           qmlNativeModuleId,
                                           "QObject",
                                           Storage::Version{},
                                           sourceId2,
                                           "QObject");

        package.updatedTypeSourceIds = {sourceId1, sourceId2};
        package.updatedExportedTypeSourceIds = {qmldir1SourceId,
                                                qmldir1DirectorySourceId,
                                                qmldir2SourceId,
                                                qmldir2DirectorySourceId};

        return package;
    }

    auto createBuiltinSynchronizationPackage()
    {
        SynchronizationPackage package;

        package.imports.emplace_back(QMLModuleId, Storage::Version{}, sourceId1, sourceId1);
        package.updatedImportSourceIds.push_back(sourceId1);
        package.moduleDependencies.emplace_back(QMLModuleId,
                                                Storage::Version{},
                                                sourceId1,
                                                qmldir1SourceId);
        package.updatedModuleDependencySourceIds.push_back(sourceId1);

        importsSourceId1.emplace_back(QMLModuleId, Storage::Version{}, sourceId1, sourceId1);
        moduleDependenciesSourceId1.emplace_back(QMLModuleId,
                                                 Storage::Version{},
                                                 sourceId1,
                                                 qmldir1SourceId);

        package.types.emplace_back("bool",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Value,
                                   sourceId1);
        package.exportedTypes.emplace_back(
            qmldir1SourceId, QMLModuleId, "bool", Storage::Version{}, sourceId1, "bool");
        package.types.emplace_back("int",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Value,
                                   sourceId1);
        package.exportedTypes
            .emplace_back(qmldir1SourceId, QMLModuleId, "int", Storage::Version{}, sourceId1, "int");
        package.types.emplace_back("uint",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Value,
                                   sourceId1);
        package.exportedTypes.emplace_back(
            qmldir1SourceId, QMLNativeModuleId, "uint", Storage::Version{}, sourceId1, "uint");
        package.types.emplace_back("double",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Value,
                                   sourceId1);
        package.exportedTypes.emplace_back(
            qmldir1SourceId, QMLModuleId, "double", Storage::Version{}, sourceId1, "double");
        package.exportedTypes.emplace_back(
            qmldir1SourceId, QMLModuleId, "real", Storage::Version{}, sourceId1, "double");
        package.types.emplace_back("float",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Value,
                                   sourceId1);
        package.exportedTypes.emplace_back(
            qmldir1SourceId, QMLNativeModuleId, "float", Storage::Version{}, sourceId1, "float");
        package.types.emplace_back("date",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Value,
                                   sourceId1);
        package.exportedTypes.emplace_back(
            qmldir1SourceId, QMLModuleId, "date", Storage::Version{}, sourceId1, "date");

        package.types.emplace_back("string",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Value,
                                   sourceId1);
        package.exportedTypes.emplace_back(
            qmldir1SourceId, QMLModuleId, "string", Storage::Version{}, sourceId1, "string");
        package.types.emplace_back("url",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Value,
                                   sourceId1);
        package.exportedTypes
            .emplace_back(qmldir1SourceId, QMLModuleId, "url", Storage::Version{}, sourceId1, "url");
        package.types.emplace_back("var",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Value,
                                   sourceId1);
        package.exportedTypes
            .emplace_back(qmldir1SourceId, QMLModuleId, "var", Storage::Version{}, sourceId1, "var");

        package.updatedTypeSourceIds = {sourceId1};
        package.updatedExportedTypeSourceIds = {qmldir1SourceId};

        return package;
    }

    auto createSynchronizationPackageWithAliases()
    {
        auto package{createSimpleSynchronizationPackage()};

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId3, sourceId3);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
        package.updatedImportSourceIds.push_back(sourceId3);

        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId3,
                                                qmldir3SourceId);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId,
                                                Storage::Version{},
                                                sourceId3,
                                                qmldir3SourceId);
        package.updatedModuleDependencySourceIds.push_back(qmldir3SourceId);

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId4, sourceId4);
        package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId4, sourceId4);
        package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId4, sourceId4, "Directory");
        package.updatedImportSourceIds.push_back(sourceId4);

        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId4,
                                                qmldir4SourceId);
        package.updatedModuleDependencySourceIds.push_back(qmldir4SourceId);

        importsSourceId3.emplace_back(qmlModuleId, Storage::Version{}, sourceId3, sourceId3);
        importsSourceId3.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
        moduleDependenciesSourceId3.emplace_back(qmlNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId3,
                                                 qmldir3SourceId);
        moduleDependenciesSourceId3.emplace_back(qtQuickNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId3,
                                                 qmldir3SourceId);

        importsSourceId4.emplace_back(qmlModuleId, Storage::Version{}, sourceId4, sourceId4);
        importsSourceId4.emplace_back(pathToModuleId, Storage::Version{}, sourceId4, sourceId4);
        importsSourceId4.emplace_back(pathToModuleId, Storage::Version{}, sourceId4, sourceId4, "Directory");
        moduleDependenciesSourceId4.emplace_back(qmlNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId4,
                                                 qmldir4SourceId);

        package.types[1].propertyDeclarations.push_back(
            Storage::Synchronization::PropertyDeclaration{"objects",
                                                          Storage::Synchronization::ImportedType{
                                                              "QObject"},
                                                          Storage::PropertyDeclarationTraits::IsList});
        auto &aliasItem = package.types.emplace_back("QAliasItem",
                                                     Storage::Synchronization::ImportedType{"Item"},
                                                     Storage::Synchronization::ImportedType{},
                                                     TypeTraitsKind::Reference,
                                                     sourceId3);
        package.exportedTypes.emplace_back(qmldir3SourceId,
                                           qtQuickModuleId,
                                           "AliasItem",
                                           Storage::Version{},
                                           sourceId3,
                                           "QAliasItem");
        package.exportedTypes.emplace_back(qmldir3DirectorySourceId,
                                           qtQuickNativeModuleId,
                                           "QAliasItem",
                                           Storage::Version{},
                                           sourceId3,
                                           "QAliasItem");

        aliasItem.propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
            "data",
            Storage::Synchronization::ImportedType{"QObject"},
            Storage::PropertyDeclarationTraits::IsList});
        aliasItem.propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
            "items", Storage::Synchronization::ImportedType{"Item"}, "children"});
        aliasItem.propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
            "objects", Storage::Synchronization::ImportedType{"Item"}, "objects"});

        auto &object2 = package.types.emplace_back("QObject2",
                                                   Storage::Synchronization::ImportedType{},
                                                   Storage::Synchronization::ImportedType{},
                                                   TypeTraitsKind::Reference,
                                                   sourceId4);
        package.exportedTypes.emplace_back(qmldir4DirectorySourceId,
                                           pathToModuleId,
                                           "Object2",
                                           Storage::Version{},
                                           sourceId4,
                                           "QObject2");
        package.exportedTypes.emplace_back(qmldir4DirectorySourceId,
                                           pathToModuleId,
                                           "Obj2",
                                           Storage::Version{},
                                           sourceId4,
                                           "QObject2");
        object2.propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
            "objects",
            Storage::Synchronization::ImportedType{"QObject"},
            Storage::PropertyDeclarationTraits::IsList});

        package.updatedExportedTypeSourceIds.push_back(qmldir3SourceId);
        package.updatedExportedTypeSourceIds.push_back(qmldir3DirectorySourceId);
        package.updatedExportedTypeSourceIds.push_back(qmldir4DirectorySourceId);

        package.updatedTypeSourceIds.push_back(sourceId3);
        package.updatedTypeSourceIds.push_back(sourceId4);

        return package;
    }

    auto createSynchronizationPackageWithIndirectAliases()
    {
        SynchronizationPackage package;

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1, sourceId1);
        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId2, sourceId2);
        package.updatedImportSourceIds.push_back(sourceId1);
        package.updatedImportSourceIds.push_back(sourceId2);

        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId1,
                                                qmldir1SourceId);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId,
                                                Storage::Version{},
                                                sourceId1,
                                                qmldir1SourceId);
        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId2,
                                                qmldir2SourceId);

        package.updatedModuleDependencySourceIds.push_back(qmldir1SourceId);
        package.updatedModuleDependencySourceIds.push_back(qmldir2SourceId);

        importsSourceId1.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1);
        importsSourceId1.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1, sourceId1);
        moduleDependenciesSourceId1.emplace_back(qmlNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId1,
                                                 qmldir1SourceId);
        moduleDependenciesSourceId1.emplace_back(qtQuickNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId1,
                                                 qmldir1SourceId);

        importsSourceId2.emplace_back(qmlModuleId, Storage::Version{}, sourceId2, sourceId2);
        moduleDependenciesSourceId2.emplace_back(qmlNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId2,
                                                 qmldir2SourceId);

        package.types.push_back(Storage::Synchronization::Type{
            "QQuickItem",
            Storage::Synchronization::ImportedType{"QObject"},
            Storage::Synchronization::ImportedType{},
            TypeTraitsKind::Reference,
            sourceId1,
            {Storage::Synchronization::PropertyDeclaration{
                 "children",
                 Storage::Synchronization::ImportedType{"QChildren"},
                 Storage::PropertyDeclarationTraits::IsReadOnly},
             Storage::Synchronization::PropertyDeclaration{
                 "kids",
                 Storage::Synchronization::ImportedType{"QChildren2"},
                 Storage::PropertyDeclarationTraits::IsReadOnly}}});
        package.exportedTypes.emplace_back(
            qmldir1SourceId, qtQuickModuleId, "Item", Storage::Version{}, sourceId1, "QQuickItem");
        package.exportedTypes.emplace_back(qmldir1DirectorySourceId,
                                           qtQuickNativeModuleId,
                                           "QQuickItem",
                                           Storage::Version{},
                                           sourceId1,
                                           "QQuickItem");
        package.types.push_back(
            Storage::Synchronization::Type{"QObject",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{},
                                           TypeTraitsKind::Reference,
                                           sourceId2});
        package.exportedTypes.emplace_back(
            qmldir2SourceId, qmlModuleId, "Object", Storage::Version{2}, sourceId2, "QObject");
        package.exportedTypes.emplace_back(
            qmldir2SourceId, qmlModuleId, "Obj", Storage::Version{2}, sourceId2, "QObject");
        package.exportedTypes.emplace_back(qmldir2DirectorySourceId,
                                           qmlNativeModuleId,
                                           "QObject",
                                           Storage::Version{2},
                                           sourceId2,
                                           "QObject");

        package.updatedTypeSourceIds = {sourceId1, sourceId2};
        package.updatedExportedTypeSourceIds = {qmldir1SourceId,
                                                qmldir1DirectorySourceId,
                                                qmldir2SourceId,
                                                qmldir2DirectorySourceId};

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId3, sourceId3);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
        package.updatedImportSourceIds.push_back(sourceId3);

        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId3,
                                                qmldir3SourceId);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId,
                                                Storage::Version{},
                                                sourceId3,
                                                qmldir3SourceId);
        package.updatedModuleDependencySourceIds.push_back(qmldir3SourceId);

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId4, sourceId4);
        package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId4, sourceId4);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId4, sourceId4);
        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId4,
                                                qmldir4SourceId);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId,
                                                Storage::Version{},
                                                sourceId4,
                                                qmldir4SourceId);
        package.updatedImportSourceIds.push_back(sourceId4);
        package.updatedModuleDependencySourceIds.push_back(qmldir4SourceId);

        importsSourceId3.emplace_back(qmlModuleId, Storage::Version{}, sourceId3, sourceId3);
        importsSourceId3.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
        moduleDependenciesSourceId3.emplace_back(qmlNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId3,
                                                 qmldir3SourceId);
        moduleDependenciesSourceId3.emplace_back(qtQuickNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId3,
                                                 qmldir3SourceId);

        importsSourceId4.emplace_back(qmlModuleId, Storage::Version{}, sourceId4, sourceId4);
        importsSourceId4.emplace_back(pathToModuleId, Storage::Version{}, sourceId4, sourceId4);
        importsSourceId4.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId4, sourceId4);
        moduleDependenciesSourceId4.emplace_back(qmlNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId4,
                                                 qmldir4SourceId);
        moduleDependenciesSourceId4.emplace_back(qtQuickNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId4,
                                                 qmldir4SourceId);

        package.types[1].propertyDeclarations.push_back(
            Storage::Synchronization::PropertyDeclaration{"objects",
                                                          Storage::Synchronization::ImportedType{
                                                              "QObject"},
                                                          Storage::PropertyDeclarationTraits::IsList});
        auto &aliasItem = package.types.emplace_back("QAliasItem",
                                                     Storage::Synchronization::ImportedType{"Item"},
                                                     Storage::Synchronization::ImportedType{},
                                                     TypeTraitsKind::Reference,
                                                     sourceId3);
        package.exportedTypes.emplace_back(qmldir3SourceId,
                                           qtQuickModuleId,
                                           "AliasItem",
                                           Storage::Version{},
                                           sourceId3,
                                           "QAliasItem");
        package.exportedTypes.emplace_back(qmldir3SourceId,
                                           qtQuickNativeModuleId,
                                           "QAliasItem",
                                           Storage::Version{},
                                           sourceId3,
                                           "QAliasItem");
        aliasItem.propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
            "items", Storage::Synchronization::ImportedType{"Item"}, "children", "items"});
        aliasItem.propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
            "objects", Storage::Synchronization::ImportedType{"Item"}, "children", "objects"});

        package.types.emplace_back("QObject2",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Reference,
                                   sourceId4);
        package.exportedTypes.emplace_back(
            qmldir4SourceId, pathToModuleId, "Object2", Storage::Version{}, sourceId4, "QObject2");
        package.exportedTypes.emplace_back(
            qmldir4SourceId, pathToModuleId, "Obj2", Storage::Version{}, sourceId4, "QObject2");
        package.types[3].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
            "children",
            Storage::Synchronization::ImportedType{"QChildren2"},
            Storage::PropertyDeclarationTraits::IsReadOnly});

        package.updatedTypeSourceIds.push_back(sourceId3);
        package.updatedTypeSourceIds.push_back(sourceId4);
        package.updatedExportedTypeSourceIds.push_back(qmldir3SourceId);
        package.updatedExportedTypeSourceIds.push_back(qmldir4SourceId);

        package.types.push_back(Storage::Synchronization::Type{
            "QChildren",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraitsKind::Reference,
            sourceId5,
            {Storage::Synchronization::PropertyDeclaration{"items",
                                                           Storage::Synchronization::ImportedType{
                                                               "QQuickItem"},
                                                           Storage::PropertyDeclarationTraits::IsList},
             Storage::Synchronization::PropertyDeclaration{
                 "objects",
                 Storage::Synchronization::ImportedType{"QObject"},
                 Storage::PropertyDeclarationTraits::IsList
                     | Storage::PropertyDeclarationTraits::IsReadOnly}}});
        package.exportedTypes.emplace_back(qmldir5SourceId,
                                           qtQuickModuleId,
                                           "Children",
                                           Storage::Version{2},
                                           sourceId5,
                                           "QChildren");
        package.exportedTypes.emplace_back(qmldir5SourceId,
                                           qtQuickNativeModuleId,
                                           "QChildren",
                                           Storage::Version{},
                                           sourceId5,
                                           "QChildren");

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId5, sourceId5);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId5, sourceId5);
        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId5,
                                                qmldir5SourceId);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId,
                                                Storage::Version{},
                                                sourceId5,
                                                qmldir5SourceId);
        importsSourceId5.emplace_back(qmlModuleId, Storage::Version{}, sourceId5, sourceId5);
        importsSourceId5.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId5, sourceId5);
        moduleDependenciesSourceId5.emplace_back(qmlNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId5,
                                                 qmldir5SourceId);
        moduleDependenciesSourceId5.emplace_back(qtQuickNativeModuleId,
                                                 Storage::Version{},
                                                 sourceId5,
                                                 qmldir5SourceId);
        package.updatedImportSourceIds.push_back(sourceId5);
        package.updatedModuleDependencySourceIds.push_back(qmldir5SourceId);
        package.updatedTypeSourceIds.push_back(sourceId5);
        package.updatedExportedTypeSourceIds.push_back(qmldir5SourceId);

        package.types.push_back(Storage::Synchronization::Type{
            "QChildren2",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraitsKind::Reference,
            sourceId6,
            {Storage::Synchronization::PropertyDeclaration{"items",
                                                           Storage::Synchronization::ImportedType{
                                                               "QQuickItem"},
                                                           Storage::PropertyDeclarationTraits::IsList},
             Storage::Synchronization::PropertyDeclaration{
                 "objects",
                 Storage::Synchronization::ImportedType{"Object2"},
                 Storage::PropertyDeclarationTraits::IsList
                     | Storage::PropertyDeclarationTraits::IsReadOnly}}});
        package.exportedTypes.emplace_back(qmldir6SourceId,
                                           qtQuickModuleId,
                                           "Children2",
                                           Storage::Version{2},
                                           sourceId6,
                                           "QChildren2");
        package.exportedTypes.emplace_back(qmldir6SourceId,
                                           qtQuickNativeModuleId,
                                           "QChildren2",
                                           Storage::Version{},
                                           sourceId6,
                                           "QChildren2");

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId6, sourceId6);
        package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId6, sourceId6);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId6, sourceId6);
        package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                                Storage::Version{},
                                                sourceId6,
                                                qmldir6SourceId);
        package.moduleDependencies.emplace_back(qtQuickNativeModuleId,
                                                Storage::Version{},
                                                sourceId6,
                                                qmldir6SourceId);
        package.updatedImportSourceIds.push_back(sourceId6);
        package.updatedModuleDependencySourceIds.push_back(sourceId6);
        package.updatedTypeSourceIds.push_back(sourceId6);
        package.updatedExportedTypeSourceIds.push_back(qmldir6SourceId);

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

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId5, sourceId5);
        package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId5, sourceId5);
        package.updatedImportSourceIds.push_back(sourceId5);

        auto &aliasItem = package.types.emplace_back("QAliasItem2",
                                                     Storage::Synchronization::ImportedType{
                                                         "Object"},
                                                     Storage::Synchronization::ImportedType{},
                                                     TypeTraitsKind::Reference,
                                                     sourceId5);

        aliasItem.propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
            "objects", Storage::Synchronization::ImportedType{"AliasItem"}, "objects"});

        package.exportedTypes.emplace_back(qmldir5SourceId,
                                           qtQuickModuleId,
                                           "AliasItem2",
                                           Storage::Version{},
                                           sourceId5,
                                           "QAliasItem2");

        package.updatedTypeSourceIds.push_back(sourceId5);

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

        package.types.emplace_back("QObject",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Reference,
                                   sourceId1);
        package.exportedTypes.emplace_back(
            qmldir1SourceId, qmlModuleId, "Object", Storage::Version{1}, sourceId1, "QObject");
        package.exportedTypes.emplace_back(
            qmldir1SourceId, qmlModuleId, "Obj", Storage::Version{1, 2}, sourceId1, "QObject");
        package.exportedTypes.emplace_back(
            qmldir1SourceId, qmlModuleId, "BuiltInObj", Storage::Version{}, sourceId1, "QObject");
        package.exportedTypes.emplace_back(
            qmldir1SourceId, qmlNativeModuleId, "QObject", Storage::Version{}, sourceId1, "QObject");
        package.types.emplace_back("QObject2",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Reference,
                                   sourceId1);
        package.exportedTypes.emplace_back(
            qmldir1SourceId, qmlModuleId, "Object", Storage::Version{2, 0}, sourceId1, "QObject2");
        package.exportedTypes.emplace_back(
            qmldir1SourceId, qmlModuleId, "Obj", Storage::Version{2, 3}, sourceId1, "QObject2");
        package.exportedTypes.emplace_back(qmldir1SourceId,
                                           qmlNativeModuleId,
                                           "QObject2",
                                           Storage::Version{},
                                           sourceId1,
                                           "QObject2");
        package.types.emplace_back("QObject3",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Reference,
                                   sourceId1);
        package.exportedTypes.emplace_back(
            qmldir1SourceId, qmlModuleId, "Object", Storage::Version{2, 11}, sourceId1, "QObject3");
        package.exportedTypes.emplace_back(
            qmldir1SourceId, qmlModuleId, "Obj", Storage::Version{2, 11}, sourceId1, "QObject3");
        package.exportedTypes.emplace_back(qmldir1SourceId,
                                           qmlNativeModuleId,
                                           "QObject3",
                                           Storage::Version{},
                                           sourceId1,
                                           "QObject3");
        package.types.emplace_back("QObject4",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Reference,
                                   sourceId1);
        package.exportedTypes.emplace_back(
            qmldir1SourceId, qmlModuleId, "Object", Storage::Version{3, 4}, sourceId1, "QObject4");
        package.exportedTypes.emplace_back(
            qmldir1SourceId, qmlModuleId, "Obj", Storage::Version{3, 4}, sourceId1, "QObject4");
        package.exportedTypes.emplace_back(qmldir1SourceId,
                                           qmlModuleId,
                                           "BuiltInObj",
                                           Storage::Version{3, 4},
                                           sourceId1,
                                           "QObject4");
        package.exportedTypes.emplace_back(qmldir1SourceId,
                                           qmlNativeModuleId,
                                           "QObject4",
                                           Storage::Version{},
                                           sourceId1,
                                           "QObject4");
        package.updatedTypeSourceIds.push_back(sourceId1);
        package.updatedExportedTypeSourceIds.push_back(qmldir1SourceId);

        shuffle(package.types);
        shuffle(package.exportedTypes);

        return package;
    }

    auto createSynchronizationPackageWithVersionsAndImport()
    {
        auto package = createSynchronizationPackageWithVersions();

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, qmldir1SourceId);
        package.updatedImportSourceIds.push_back(qmldir1SourceId);

        return package;
    }
    enum class ExchangePrototypeAndExtension { No, Yes };

    auto createPackageWithProperties(
        ExchangePrototypeAndExtension exchange = ExchangePrototypeAndExtension::No)
    {
        SynchronizationPackage package;

        package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1);
        package.updatedImportSourceIds.push_back(sourceId1);

        package.types.push_back(Storage::Synchronization::Type{
            "QObject",
            Storage::Synchronization::ImportedType{},
            Storage::Synchronization::ImportedType{},
            TypeTraitsKind::Reference,
            sourceId1,
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
        package.exportedTypes.emplace_back(
            qmldir1SourceId, qmlModuleId, "Object", Storage::Version{}, sourceId1, "QObject");

        package.types.push_back(Storage::Synchronization::Type{
            "QObject2",
            exchange == ExchangePrototypeAndExtension::No
                ? Storage::Synchronization::ImportedType{"Object"}
                : Storage::Synchronization::ImportedType{},
            exchange == ExchangePrototypeAndExtension::Yes
                ? Storage::Synchronization::ImportedType{"Object"}
                : Storage::Synchronization::ImportedType{},
            TypeTraitsKind::Reference,
            sourceId1,
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
        package.exportedTypes.emplace_back(
            qmldir1SourceId, qmlModuleId, "Object2", Storage::Version{}, sourceId1, "QObject2");

        package.types.push_back(Storage::Synchronization::Type{
            "QObject3",
            Storage::Synchronization::ImportedType{"Object2"},
            Storage::Synchronization::ImportedType{},
            TypeTraitsKind::Reference,
            sourceId1,
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
        package.exportedTypes.emplace_back(
            qmldir1SourceId, qmlModuleId, "Object3", Storage::Version{}, sourceId1, "QObject3");

        package.updatedTypeSourceIds.push_back(sourceId1);
        package.updatedExportedTypeSourceIds.push_back(qmldir1SourceId);

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
                                           sourceId1});
        package.types.push_back(
            Storage::Synchronization::Type{"QObject5",
                                           Storage::Synchronization::ImportedType{},
                                           Storage::Synchronization::ImportedType{"Object2"},
                                           TypeTraitsKind::Reference,
                                           sourceId1});

        return package;
    }

    auto createModuleExportedImportSynchronizationPackage()
    {
        SynchronizationPackage package;

        package.imports.emplace_back(qmlModuleId, Storage::Version{1}, sourceId1, sourceId1);
        package.updatedImportSourceIds.push_back(sourceId1);

        package.updatedModuleIds.push_back(qtQuickModuleId);

        package.types.emplace_back("QQuickItem",
                                   Storage::Synchronization::ImportedType{"Object"},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Reference,
                                   sourceId1);

        package.exportedTypes.emplace_back(qmldir1SourceId,
                                           qtQuickModuleId,
                                           "Item",
                                           Storage::Version{1, 0},
                                           sourceId1,
                                           "QQuickItem");

        package.updatedModuleIds.push_back(qmlModuleId);
        package.moduleExportedImports.emplace_back(qtQuickModuleId,
                                                   qmlModuleId,
                                                   Storage::Version{},
                                                   Storage::Synchronization::IsAutoVersion::Yes);
        package.types.emplace_back("QObject",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Reference,
                                   sourceId2);

        package.exportedTypes.emplace_back(
            qmldir2SourceId, qmlModuleId, "Object", Storage::Version{1, 0}, sourceId2, "QObject");

        package.imports.emplace_back(qtQuickModuleId, Storage::Version{1}, sourceId3, sourceId3);
        package.updatedImportSourceIds.push_back(sourceId3);

        package.moduleExportedImports.emplace_back(qtQuick3DModuleId,
                                                   qtQuickModuleId,
                                                   Storage::Version{},
                                                   Storage::Synchronization::IsAutoVersion::Yes);
        package.updatedModuleIds.push_back(qtQuick3DModuleId);
        package.types.emplace_back("QQuickItem3d",
                                   Storage::Synchronization::ImportedType{"Item"},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Reference,
                                   sourceId3);

        package.exportedTypes.emplace_back(qmldir3SourceId,
                                           qtQuick3DModuleId,
                                           "Item3D",
                                           Storage::Version{1, 0},
                                           sourceId3,
                                           "QQuickItem3d");

        package.imports.emplace_back(qtQuick3DModuleId, Storage::Version{1}, sourceId4, sourceId4);
        package.updatedImportSourceIds.push_back(sourceId4);

        package.types.emplace_back("MyItem",
                                   Storage::Synchronization::ImportedType{"Object"},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Reference,
                                   sourceId4);

        package.exportedTypes.emplace_back(qmldir4SourceId,
                                           myModuleModuleId,
                                           "MyItem",
                                           Storage::Version{1, 0},
                                           sourceId4,
                                           "MyItem");

        package.updatedTypeSourceIds = {sourceId1, sourceId2, sourceId3, sourceId4};
        package.updatedExportedTypeSourceIds = {qmldir1SourceId,
                                                qmldir2SourceId,
                                                qmldir3SourceId,
                                                qmldir4SourceId};

        return package;
    }

    auto createPropertyEditorPathsSynchronizationPackage()
    {
        SynchronizationPackage package;

        package.updatedModuleIds.push_back(qtQuickModuleId);
        package.types.emplace_back("QQuickItem",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Reference,
                                   sourceId1);
        package.exportedTypes.emplace_back(qmldir1SourceId,
                                           qtQuickModuleId,
                                           "Item",
                                           Storage::Version{1, 0},
                                           sourceId1,
                                           "QQuickItem");

        package.types.emplace_back("QObject",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Reference,
                                   sourceId2);
        package.exportedTypes.emplace_back(qmldir2SourceId,
                                           qtQuickModuleId,
                                           "QtObject",
                                           Storage::Version{1, 0},
                                           sourceId2,
                                           "QObject");

        package.updatedModuleIds.push_back(qtQuick3DModuleId);
        package.types.emplace_back("QQuickItem3d",
                                   Storage::Synchronization::ImportedType{},
                                   Storage::Synchronization::ImportedType{},
                                   TypeTraitsKind::Reference,
                                   sourceId3);
        package.exportedTypes.emplace_back(qmldir3SourceId,
                                           qtQuickModuleId,
                                           "Item3D",
                                           Storage::Version{1, 0},
                                           sourceId3,
                                           "QQuickItem3d");

        package.imports.emplace_back(qtQuick3DModuleId, Storage::Version{1}, sourceId4, sourceId4);
        package.updatedImportSourceIds.push_back(sourceId4);

        package.updatedTypeSourceIds = {sourceId1, sourceId2, sourceId3};
        package.updatedExportedTypeSourceIds = {qmldir1SourceId, qmldir2SourceId, qmldir3SourceId};

        package.propertyEditorQmlPaths.emplace_back(qtQuickModuleId,
                                                    "QtObject",
                                                    sourceId1,
                                                    directoryPathIdPath6);
        package.propertyEditorQmlPaths.emplace_back(qtQuickModuleId,
                                                    "Item",
                                                    sourceId2,
                                                    directoryPathIdPath6);
        package.propertyEditorQmlPaths.emplace_back(qtQuickModuleId,
                                                    "Item3D",
                                                    sourceId3,
                                                    directoryPathIdPath6);
        package.updatedPropertyEditorQmlPathDirectoryIds.emplace_back(directoryPathIdPath6);

        return package;
    }

    auto createTypeAnnotions() const
    {
        TypeAnnotations annotations;

        TypeTraits traits{TypeTraitsKind::Reference};
        traits.canBeContainer = FlagIs::True;
        traits.visibleInLibrary = FlagIs::True;

        annotations.emplace_back(sourceId4,
                                 directoryPathIdPath6,
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
                                 directoryPathIdPath6,
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
                                 directoryPathIdPath1,
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

    TypeId fetchTypeId(SourceId sourceId, Utils::SmallStringView name) const
    {
        return storage.fetchTypeIdByName(sourceId, name);
    }

    auto fetchType(SourceId sourceId, Utils::SmallStringView name)
    {
        return storage.fetchTypeByTypeId(storage.fetchTypeIdByName(sourceId, name));
    }

    Storage::Info::ExportedTypeNames fetchExportedTypeNames(SourceId sourceId,
                                                            Utils::SmallStringView name)
    {
        auto typeId = storage.fetchTypeIdByName(sourceId, name);

        return storage.exportedTypeNames(typeId);
    }

    Storage::Info::ExportedTypeNames fetchExportedTypeNames(
        const std::vector<std::pair<SourceId, Utils::SmallStringView>> &types)
    {
        Storage::Info::ExportedTypeNames exportedTypesNames;

        for (const auto &[sourceId, name] : types) {
            auto names = fetchExportedTypeNames(sourceId, name);
            exportedTypesNames.insert(exportedTypesNames.end(), names.begin(), names.end());
        }

        return exportedTypesNames;
    }

    Storage::Info::PropertyDeclarations fetchPropertyDeclarations(SourceId sourceId,
                                                                  Utils::SmallStringView name) const
    {
        auto ids = storage.propertyDeclarationIds(fetchTypeId(sourceId, name));

        return Utils::transform<Storage::Info::PropertyDeclarations>(ids, [&](auto id) {
            return *storage.propertyDeclaration(id);
        });
    }

    Storage::Info::PropertyDeclarations fetchLocalPropertyDeclarations(SourceId sourceId,
                                                                       Utils::SmallStringView name) const
    {
        auto ids = storage.localPropertyDeclarationIds(fetchTypeId(sourceId, name));

        return Utils::transform<Storage::Info::PropertyDeclarations>(ids, [&](auto id) {
            return *storage.propertyDeclaration(id);
        });
    }

    auto fetchTypes()
    {
        Storage::Info::Types types;
        for (TypeId id : storage.fetchTypeIds())
            types.push_back(*storage.type(id));

        return types;
    }

    auto fetchPrototype(SourceId sourceId, Utils::SmallStringView name) const
    {
        auto prototypes = storage.prototypeIds(fetchTypeId(sourceId, name));

        if (prototypes.size())
            return prototypes.front();

        return TypeId{};
    }

    auto fetchFunctionNames(SourceId sourceId, Utils::SmallStringView name) const
    {
        return storage.functionDeclarationNames(fetchTypeId(sourceId, name));
    }

    auto fetchFunctionDeclarations(SourceId sourceId, Utils::SmallStringView name) const
    {
        return storage.fetchFunctionDeclarationsForTestOnly(fetchTypeId(sourceId, name));
    }

    auto fetchSignalDeclarations(SourceId sourceId, Utils::SmallStringView name) const
    {
        return storage.fetchSignalDeclarationsForTestOnly(fetchTypeId(sourceId, name));
    }

    auto fetchEnumerationDeclarations(SourceId sourceId, Utils::SmallStringView name) const
    {
        return storage.fetchEnumerationDeclarationsForTestOnly(fetchTypeId(sourceId, name));
    }

    auto defaultPropertyDeclarationId(SourceId sourceId, Utils::SmallStringView typeName) const
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

    static void renameExportedTypeTypeIdNames(auto &exportedTypes,
                                              SourceId sourceId,
                                              Utils::SmallStringView name,
                                              Utils::SmallStringView newName)
    {
        using QmlDesigner::Storage::Synchronization::ExportedType;
        auto isName = [&](const ExportedType &exportedType) {
            return exportedType.typeSourceId == sourceId and exportedType.typeIdName == name;
        };

        for (ExportedType &exportedType : exportedTypes | std::views::filter(isName))
            exportedType.typeIdName = newName;
    }

    static void renameExportedTypeNames(auto &exportedTypes,
                                        Utils::SmallStringView name,
                                        Utils::SmallStringView newName)
    {
        using QmlDesigner::Storage::Synchronization::ExportedType;
        auto isName = [&](const ExportedType &exportedType) { return exportedType.name == name; };

        for (ExportedType &exportedType : exportedTypes | std::views::filter(isName))
            exportedType.name = newName;
    }

    Storage::Synchronization::ProjectEntryInfos fetchProjectEntryInfos(const SourceIds &sourceContextIds)
    {
        Storage::Synchronization::ProjectEntryInfos projectEntryInfos;

        for (SourceId sourceContextId : sourceContextIds) {
            auto infos = storage.fetchProjectEntryInfos(sourceContextId);
            projectEntryInfos.insert(projectEntryInfos.end(), infos.begin(), infos.end());
        }

        return projectEntryInfos;
    }

    static QmlDesigner::FileStatus createFileStatus(SourceId sourceId,
                                                    long long size,
                                                    long long modifiedTime)
    {
        using file_time_type = std::filesystem::file_time_type;

        return QmlDesigner::FileStatus{sourceId,
                                       size,
                                       file_time_type{file_time_type::duration{modifiedTime}}};
    }

    Storage::Synchronization::ProjectEntryInfos fetchProjectEntryInfos(
        const DirectoryPathIds &directoryPathIds)
    {
        Storage::Synchronization::ProjectEntryInfos projectEntryInfos;

        for (DirectoryPathId directoryPathId : directoryPathIds) {
            SourceId sourceContextId = SourceId::create(directoryPathId, FileNameId{});
            auto infos = storage.fetchProjectEntryInfos(sourceContextId);
            projectEntryInfos.insert(projectEntryInfos.end(), infos.begin(), infos.end());
        }

        return projectEntryInfos;
    }

protected:
    inline static std::unique_ptr<StaticData> staticData;
    Sqlite::Database &database = staticData->database;
    Sqlite::Database &modulesDatabase = staticData->modulesDatabase;
    QmlDesigner::ProjectStorage &storage = staticData->storage;
    QmlDesigner::ModulesStorage &modulesStorage = staticData->modulesStorage;
    NiceMock<ProjectStorageErrorNotifierMock> errorNotifierMock;
    QmlDesigner::SourcePathCache<QmlDesigner::SourcePathStorage> sourcePathCache{
        staticData->sourcePathStorage};
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
    DirectoryPathId directoryPathIdPath1{sourceIdPath1.directoryPathId()};
    SourceId sourceIdPath6{sourcePathCache.sourceId(pathPath6)};
    DirectoryPathId directoryPathIdPath6{sourceIdPath6.directoryPathId()};
    SourceId qmldir1SourceId{sourcePathCache.sourceId("/path1/qmldir")};
    DirectoryPathId qmldir1DirectoryPathId = qmldir1SourceId.directoryPathId();
    SourceId qmldir1DirectorySourceId = SourceId::create(qmldir1DirectoryPathId);
    SourceId qmldir2SourceId{sourcePathCache.sourceId("/path2/qmldir")};
    DirectoryPathId qmldir2DirectoryPathId = qmldir2SourceId.directoryPathId();
    SourceId qmldir2DirectorySourceId = SourceId::create(qmldir2DirectoryPathId);
    SourceId qmldir3SourceId{sourcePathCache.sourceId("/path3/qmldir")};
    DirectoryPathId qmldir3DirectoryPathId = qmldir3SourceId.directoryPathId();
    SourceId qmldir3DirectorySourceId = SourceId::create(qmldir3DirectoryPathId);
    SourceId qmldir4SourceId{sourcePathCache.sourceId("/path4/qmldir")};
    DirectoryPathId qmldir4DirectoryPathId = qmldir4SourceId.directoryPathId();
    SourceId qmldir4DirectorySourceId = SourceId::create(qmldir4DirectoryPathId);
    SourceId qmldir5SourceId{sourcePathCache.sourceId("/path5/qmldir")};
    DirectoryPathId qmldir5DirectoryPathId = qmldir5SourceId.directoryPathId();
    SourceId qmldir5DirectorySourceId = SourceId::create(qmldir5DirectoryPathId);
    SourceId qmldir6SourceId{sourcePathCache.sourceId("/path6/qmldir")};
    DirectoryPathId qmldir6DirectoryPathId = qmldir6SourceId.directoryPathId();
    SourceId qmldir6DirectorySourceId = SourceId::create(qmldir6DirectoryPathId);
    ModuleId qmlModuleId{modulesStorage.moduleId("Qml", ModuleKind::QmlLibrary)};
    ModuleId qmlNativeModuleId{modulesStorage.moduleId("Qml", ModuleKind::CppLibrary)};
    ModuleId qtQuickModuleId{modulesStorage.moduleId("QtQuick", ModuleKind::QmlLibrary)};
    ModuleId qtQuickNativeModuleId{modulesStorage.moduleId("QtQuick", ModuleKind::CppLibrary)};
    ModuleId pathToModuleId{modulesStorage.moduleId("/path/to", ModuleKind::PathLibrary)};
    ModuleId qtQuick3DModuleId{modulesStorage.moduleId("QtQuick3D", ModuleKind::QmlLibrary)};
    ModuleId myModuleModuleId{modulesStorage.moduleId("MyModule", ModuleKind::QmlLibrary)};
    ModuleId QMLModuleId{modulesStorage.moduleId("QML", ModuleKind::QmlLibrary)};
    ModuleId QMLNativeModuleId{modulesStorage.moduleId("QML", ModuleKind::CppLibrary)};
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

TEST_F(ProjectStorage, synchronize_types_adds_new_types)
{
    auto package{createSimpleSynchronizationPackage()};

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                                     IsStorageType(sourceId1,
                                                   "QQuickItem",
                                                   fetchTypeId(sourceId2, "QObject"),
                                                   TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, synchronize_exported_types)
{
    auto package{createSimpleSynchronizationPackage()};

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchExportedTypeNames(sourceId2, "QObject"),
                UnorderedElementsAre(IsExportedTypeName(qmlModuleId, "Object", IsVersion(2U)),
                                     IsExportedTypeName(qmlModuleId, "Obj", IsVersion(2U)),
                                     IsExportedTypeName(qmlNativeModuleId, "QObject", HasNoVersion())));
    ASSERT_THAT(fetchExportedTypeNames(sourceId1, "QQuickItem"),
                UnorderedElementsAre(IsExportedTypeName(qtQuickModuleId, "Item", HasNoVersion()),
                                     IsExportedTypeName(qtQuickNativeModuleId,
                                                        "QQuickItem",
                                                        HasNoVersion())));
}

TEST_F(ProjectStorage, synchronize_throws_for_duplicate_exported_type_names)
{
    auto package{createSimpleSynchronizationPackage()};
    package.exportedTypes.push_back(package.exportedTypes.front());

    ASSERT_THROW(storage.synchronize(std::move(package)), QmlDesigner::ExportedTypeCannotBeInserted);
}

TEST_F(ProjectStorage, synchronize_notifies_error_for_duplicate_exported_type_names)
{
    auto package{createSimpleSynchronizationPackage()};
    package.exportedTypes.push_back(package.exportedTypes.front());

    EXPECT_CALL(errorNotifierMock, exportedTypeNameIsDuplicate(qtQuickModuleId, Eq("Item")));

    EXPECT_ANY_THROW(storage.synchronize(std::move(package)));
}

TEST_F(ProjectStorage, synchronize_types_adds_new_types_with_extension_chain)
{
    auto package{createSimpleSynchronizationPackage()};
    swap(package.types.front().extension, package.types.front().prototype);

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                                     IsStorageType(sourceId1,
                                                   "QQuickItem",
                                                   TypeId{},
                                                   fetchTypeId(sourceId2, "QObject"),
                                                   TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, synchronize_types_adds_new_types_with_exported_prototype_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Object"};

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    IsStorageType(sourceId2, "QObject", TypeId{}, TypeId{}, TypeTraitsKind::Reference),
                    IsStorageType(sourceId1,
                                  "QQuickItem",
                                  fetchTypeId(sourceId2, "QObject"),
                                  TypeId{},
                                  TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, synchronize_types_adds_new_types_with_exported_extension_name)
{
    auto package{createSimpleSynchronizationPackage()};
    swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].extension = Storage::Synchronization::ImportedType{"Object"};

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                                     IsStorageType(sourceId1,
                                                   "QQuickItem",
                                                   TypeId{},
                                                   fetchTypeId(sourceId2, "QObject"),
                                                   TypeTraitsKind::Reference)));
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
    package.exportedTypes.emplace_back(qmldir2DirectorySourceId,
                                       qmlNativeModuleId,
                                       "Objec",
                                       Storage::Version{2},
                                       sourceId2,
                                       "QObject");

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(fetchTypeId(sourceId2, "QObject")));
}

TEST_F(ProjectStorage,
       synchronize_types_updates_prototype_to_unresolved_after_exported_type_name_is_removed)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Objec"};
    package.exportedTypes.emplace_back(qmldir2DirectorySourceId,
                                       qmlNativeModuleId,
                                       "Objec",
                                       Storage::Version{2},
                                       sourceId2,
                                       "QObject");
    storage.synchronize(package);
    package.exportedTypes.pop_back();

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(IsUnresolvedTypeId()));
}

TEST_F(ProjectStorage,
       synchronize_types_updates_unresolved_prototype_indirectly_after_exported_type_name_is_added)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Objec"};
    storage.synchronize(package);
    package.exportedTypes.emplace_back(qmldir2DirectorySourceId,
                                       qmlNativeModuleId,
                                       "Objec",
                                       Storage::Version{2},
                                       sourceId2,
                                       "QObject");
    package.types.clear();
    package.updatedTypeSourceIds.clear();

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(fetchTypeId(sourceId2, "QObject")));
}

TEST_F(ProjectStorage,
       synchronize_types_updates_prototype_indirectly_to_unresolved_after_exported_type_name_is_removed)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Objec"};
    package.exportedTypes.emplace_back(qmldir2DirectorySourceId,
                                       qmlNativeModuleId,
                                       "Objec",
                                       Storage::Version{2},
                                       sourceId2,
                                       "QObject");
    storage.synchronize(package);
    package.exportedTypes.pop_back();
    package.types.clear();
    package.updatedTypeSourceIds.clear();

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(IsUnresolvedTypeId()));
}

TEST_F(ProjectStorage,
       synchronize_types_updates_prototype_indirectly_to_unresolved_after_exported_type_name_is_removed_notifies_type_name_cannot_be_resolved)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Objec"};
    package.exportedTypes.emplace_back(qmldir2DirectorySourceId,
                                       qmlNativeModuleId,
                                       "Objec",
                                       Storage::Version{2},
                                       sourceId2,
                                       "QObject");
    storage.synchronize(package);
    package.exportedTypes.pop_back();
    package.types.clear();
    package.updatedTypeSourceIds.clear();

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Objec"), sourceId1));

    storage.synchronize(std::move(package));
}

TEST_F(ProjectStorage, synchronize_types_updates_unresolved_extension_after_exported_type_name_is_added)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = {};
    package.types[0].extension = Storage::Synchronization::ImportedType{"Objec"};
    storage.synchronize(package);
    package.exportedTypes.emplace_back(qmldir2DirectorySourceId,
                                       qmlNativeModuleId,
                                       "Objec",
                                       Storage::Version{2},
                                       sourceId2,
                                       "QObject");
    package.types.clear();
    package.updatedTypeSourceIds.clear();

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasExtensionId(fetchTypeId(sourceId2, "QObject")));
}

TEST_F(ProjectStorage,
       synchronize_types_updates_extension_to_unresolved_after_exported_type_name_is_removed)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].extension = Storage::Synchronization::ImportedType{"Objec"};
    package.exportedTypes.emplace_back(qmldir2DirectorySourceId,
                                       qmlNativeModuleId,
                                       "Objec",
                                       Storage::Version{2},
                                       sourceId2,
                                       "QObject");
    storage.synchronize(package);
    package.exportedTypes.pop_back();
    package.types.clear();
    package.updatedTypeSourceIds.clear();

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasExtensionId(IsUnresolvedTypeId()));
}

TEST_F(ProjectStorage,
       synchronize_types_updates_unresolved_extension_indirectly_after_exported_type_name_is_added)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = {};
    package.types[0].extension = Storage::Synchronization::ImportedType{"Objec"};
    storage.synchronize(package);
    package.exportedTypes.emplace_back(qmldir2DirectorySourceId,
                                       qmlNativeModuleId,
                                       "Objec",
                                       Storage::Version{2},
                                       sourceId2,
                                       "QObject");
    package.types.clear();
    package.updatedTypeSourceIds.clear();

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasExtensionId(fetchTypeId(sourceId2, "QObject")));
}

TEST_F(ProjectStorage,
       synchronize_types_updates_invalid_extension_indirectly_after_exported_type_name_is_removed)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].extension = Storage::Synchronization::ImportedType{"Objec"};
    package.exportedTypes.emplace_back(qmldir2DirectorySourceId,
                                       qmlNativeModuleId,
                                       "Objec",
                                       Storage::Version{2},
                                       sourceId2,
                                       "QObject");
    storage.synchronize(package);
    package.exportedTypes.pop_back();
    package.types.clear();
    package.updatedTypeSourceIds.clear();

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasExtensionId(IsUnresolvedTypeId()));
}

TEST_F(ProjectStorage,
       synchronize_types_updates_extension_indirectly_to_unresolved_after_exported_type_name_is_removed_notifies_type_name_cannot_be_resolved)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].extension = Storage::Synchronization::ImportedType{"Objec"};
    package.exportedTypes.emplace_back(qmldir2DirectorySourceId,
                                       qmlNativeModuleId,
                                       "Objec",
                                       Storage::Version{2},
                                       sourceId2,
                                       "QObject");
    storage.synchronize(package);
    package.exportedTypes.pop_back();
    package.types.clear();
    package.updatedTypeSourceIds.clear();

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

TEST_F(ProjectStorage,
       synchronize_types_updates_unresolved_extension_and_protype_after_exported_type_name_is_added)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Objec"};
    package.types[0].extension = Storage::Synchronization::ImportedType{"Objec"};
    storage.synchronize(package);
    package.exportedTypes.emplace_back(qmldir2DirectorySourceId,
                                       qmlNativeModuleId,
                                       "Objec",
                                       Storage::Version{2},
                                       sourceId2,
                                       "QObject");
    storage.synchronize(package);

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(fetchTypeId(sourceId2, "QObject")));
}

TEST_F(ProjectStorage, synchronize_types_adds_new_types_reverse_order)
{
    auto package{createSimpleSynchronizationPackage()};
    std::reverse(package.types.begin(), package.types.end());

    storage.synchronize(std::move(package));

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                                     IsStorageType(sourceId1,
                                                   "QQuickItem",
                                                   fetchTypeId(sourceId2, "QObject"),
                                                   TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, synchronize_types_overwrites_type_traits)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].traits = TypeTraitsKind::Value;
    package.types[1].traits = TypeTraitsKind::Value;

    storage.synchronize({.imports = package.imports,
                         .updatedImportSourceIds = {sourceId1, sourceId2},
                         .types = package.types,
                         .updatedTypeSourceIds = {sourceId1, sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Value),
                                     IsStorageType(sourceId1,
                                                   "QQuickItem",
                                                   fetchTypeId(sourceId2, "QObject"),
                                                   TypeTraitsKind::Value)));
}

TEST_F(ProjectStorage, synchronize_types_overwrites_sources)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].sourceId = sourceId3;
    Storage::Synchronization::ExportedTypes exportedTypes;
    exportedTypes.emplace_back(
        qmldir1SourceId, qtQuickModuleId, "Item", Storage::Version{}, sourceId3, "QQuickItem");
    package.exportedTypes.emplace_back(qmldir1DirectorySourceId,
                                       qtQuickNativeModuleId,
                                       "QQuickItem",
                                       Storage::Version{},
                                       sourceId3,
                                       "QQuickItem");
    Storage::Imports newImports;
    newImports.emplace_back(qmlModuleId, Storage::Version{}, sourceId3, sourceId3);
    newImports.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId3, sourceId3);
    newImports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
    newImports.emplace_back(qtQuickNativeModuleId, Storage::Version{}, sourceId3, sourceId3);

    storage.synchronize({.imports = newImports,
                         .updatedImportSourceIds = {sourceId1, sourceId3},
                         .exportedTypes = exportedTypes,
                         .updatedExportedTypeSourceIds = {qmldir1SourceId, qmldir1DirectorySourceId},
                         .types = package.types,
                         .updatedTypeSourceIds = {sourceId1, sourceId3}});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(IsStorageType(sourceId2, "QObject", TypeId{}, TypeTraitsKind::Reference),
                                     IsStorageType(sourceId3,
                                                   "QQuickItem",
                                                   fetchTypeId(sourceId2, "QObject"),
                                                   TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, synchronize_types_insert_type_into_prototype_chain)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].prototype = Storage::Synchronization::ImportedType{"QQuickObject"};
    auto quickObjectType = Storage::Synchronization::Type{"QQuickObject",
                                                          Storage::Synchronization::ImportedType{
                                                              "QObject"},
                                                          Storage::Synchronization::ImportedType{},
                                                          TypeTraitsKind::Reference,
                                                          sourceId1};
    package.exportedTypes.emplace_back(
        qmldir1SourceId, qtQuickModuleId, "Object", Storage::Version{}, sourceId1, "QQuickObject");
    package.exportedTypes.emplace_back(qmldir1DirectorySourceId,
                                       qtQuickNativeModuleId,
                                       "QQuickObject",
                                       Storage::Version{},
                                       sourceId1,
                                       "QQuickObject");

    storage.synchronize({.imports = importsSourceId1,
                         .updatedImportSourceIds = {sourceId1},
                         .exportedTypes = package.exportedTypes,
                         .updatedExportedTypeSourceIds = package.updatedExportedTypeSourceIds,
                         .types = {package.types[0], quickObjectType},
                         .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    IsStorageType(sourceId2, "QObject", TypeId{}, TypeId{}, TypeTraitsKind::Reference),
                    IsStorageType(sourceId1,
                                  "QQuickObject",
                                  fetchTypeId(sourceId2, "QObject"),
                                  TypeId{},
                                  TypeTraitsKind::Reference),
                    IsStorageType(sourceId1,
                                  "QQuickItem",
                                  fetchTypeId(sourceId1, "QQuickObject"),
                                  TypeId{},
                                  TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, synchronize_types_insert_type_into_extension_chain)
{
    auto package{createSimpleSynchronizationPackage()};
    swap(package.types.front().extension, package.types.front().prototype);
    storage.synchronize(package);
    package.types[0].extension = Storage::Synchronization::ImportedType{"QQuickObject"};
    package.types.push_back(
        Storage::Synchronization::Type{"QQuickObject",
                                       Storage::Synchronization::ImportedType{},
                                       Storage::Synchronization::ImportedType{"QObject"},
                                       TypeTraitsKind::Reference,
                                       sourceId1});
    package.exportedTypes.emplace_back(
        qmldir1SourceId, qtQuickModuleId, "Object", Storage::Version{}, sourceId1, "QQuickObject");
    package.exportedTypes.emplace_back(qmldir1DirectorySourceId,
                                       qtQuickNativeModuleId,
                                       "QQuickObject",
                                       Storage::Version{},
                                       sourceId1,
                                       "QQuickObject");

    storage.synchronize({.imports = importsSourceId1,
                         .updatedImportSourceIds = {sourceId1},
                         .exportedTypes = package.exportedTypes,
                         .updatedExportedTypeSourceIds = package.updatedExportedTypeSourceIds,
                         .types = {package.types[0], package.types[2]},
                         .updatedTypeSourceIds{sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    IsStorageType(sourceId2, "QObject", TypeId{}, TypeId{}, TypeTraitsKind::Reference),
                    IsStorageType(sourceId1,
                                  "QQuickObject",
                                  TypeId{},
                                  fetchTypeId(sourceId2, "QObject"),
                                  TypeTraitsKind::Reference),
                    IsStorageType(sourceId1,
                                  "QQuickItem",
                                  TypeId{},
                                  fetchTypeId(sourceId1, "QQuickObject"),
                                  TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, synchronize_types_add_qualified_prototype)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{"Object", "Quick"};
    package.types.push_back(
        Storage::Synchronization::Type{"QQuickObject",
                                       Storage::Synchronization::ImportedType{"QObject"},
                                       Storage::Synchronization::ImportedType{},
                                       TypeTraitsKind::Reference,
                                       sourceId1});
    package.exportedTypes.emplace_back(
        qmldir1SourceId, qtQuickModuleId, "Object", Storage::Version{}, sourceId1, "QQuickObject");
    package.exportedTypes.emplace_back(qmldir1DirectorySourceId,
                                       qtQuickNativeModuleId,
                                       "QQuickObject",
                                       Storage::Version{},
                                       sourceId1,
                                       "QQuickObject");

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    IsStorageType(sourceId2, "QObject", TypeId{}, TypeId{}, TypeTraitsKind::Reference),
                    IsStorageType(sourceId1,
                                  "QQuickObject",
                                  fetchTypeId(sourceId2, "QObject"),
                                  TypeId{},
                                  TypeTraitsKind::Reference),
                    IsStorageType(sourceId1,
                                  "QQuickItem",
                                  fetchTypeId(sourceId1, "QQuickObject"),
                                  TypeId{},
                                  TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, synchronize_types_add_qualified_extension)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{};
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{"Object", "Quick"};
    package.types.push_back(
        Storage::Synchronization::Type{"QQuickObject",
                                       Storage::Synchronization::ImportedType{"QObject"},
                                       Storage::Synchronization::ImportedType{},
                                       TypeTraitsKind::Reference,
                                       sourceId1});
    package.exportedTypes.emplace_back(
        qmldir1SourceId, qtQuickModuleId, "Object", Storage::Version{}, sourceId1, "QQuickObject");
    package.exportedTypes.emplace_back(qmldir1DirectorySourceId,
                                       qtQuickNativeModuleId,
                                       "QQuickObject",
                                       Storage::Version{},
                                       sourceId1,
                                       "QQuickObject");

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(
                    IsStorageType(sourceId2, "QObject", TypeId{}, TypeId{}, TypeTraitsKind::Reference),
                    IsStorageType(sourceId1,
                                  "QQuickObject",
                                  fetchTypeId(sourceId2, "QObject"),
                                  TypeId{},
                                  TypeTraitsKind::Reference),
                    IsStorageType(sourceId1,
                                  "QQuickItem",
                                  TypeId{},
                                  fetchTypeId(sourceId1, "QQuickObject"),
                                  TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, synchronize_types_notifies_cannot_resolve_for_missing_prototype)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types = {
        Storage::Synchronization::Type{"QQuickItem",
                                       Storage::Synchronization::ImportedType{"QObject"},
                                       Storage::Synchronization::ImportedType{},
                                       TypeTraitsKind::Reference,
                                       sourceId1}};
    package.exportedTypes.clear();

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("QObject"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage, synchronize_types_notifies_cannot_resolve_for_missing_extension)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types = {
        Storage::Synchronization::Type{"QQuickItem",
                                       Storage::Synchronization::ImportedType{},
                                       Storage::Synchronization::ImportedType{"QObject"},
                                       TypeTraitsKind::Reference,
                                       sourceId1}};
    package.exportedTypes.clear();

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("QObject"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage, synchronize_types_throws_for_invalid_module)
{
    auto package{createSimpleSynchronizationPackage()};
    package.exportedTypes.emplace_back(
        qmldir1SourceId, ModuleId{}, "Item", Storage::Version{}, sourceId1, "QQuickItem");

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::ModuleDoesNotExists);
}

TEST_F(ProjectStorage, type_with_invalid_source_id_throws)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types = {Storage::Synchronization::Type{"QQuickItem",
                                                    Storage::Synchronization::ImportedType{""},
                                                    Storage::Synchronization::ImportedType{},
                                                    TypeTraitsKind::Reference,
                                                    SourceId{}}};

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::TypeHasInvalidSourceId);
}

TEST_F(ProjectStorage, reset_type_if_source_id_is_synchronized)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    storage.synchronize({.updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(storage.fetchTypes(),
                Not(Contains(IsStorageType(sourceId1, "QQuickItem", TypeTraitsKind::Reference))));
}

TEST_F(ProjectStorage, delete_exported_type_name_if_source_id_is_synchronized)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    storage.synchronize({.updatedExportedTypeSourceIds = {qmldir1SourceId}});

    ASSERT_THAT(fetchExportedTypeNames(sourceId1, "QQuickItem"),
                Not(Contains(IsExportedTypeName(qtQuickModuleId, "Item", _))));
}

TEST_F(ProjectStorage, dont_reset_type_if_source_id_is_not_synchronized)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    storage.synchronize({.updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1, "QQuickItem", TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, dont_delete_exported_type_name_if_source_id_is_not_synchronized)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    storage.synchronize({.updatedExportedTypeSourceIds = {qmldir2SourceId}});

    ASSERT_THAT(fetchExportedTypeNames(sourceId1, "QQuickItem"),
                Contains(IsExportedTypeName(qtQuickModuleId, "Item", _)));
}

TEST_F(ProjectStorage, update_exported_types_type_id_name)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    QmlDesigner::Storage::Synchronization::ExportedType exportedType{
        qmldir1SourceId, qtQuickModuleId, "Item", Storage::Version{}, sourceId1, "QQuickItem2"};

    storage.synchronize(
        {.exportedTypes = {exportedType}, .updatedExportedTypeSourceIds = {qmldir1SourceId}});

    ASSERT_THAT(fetchExportedTypeNames(sourceId1, "QQuickItem2"),
                ElementsAre(IsExportedTypeName(qtQuickModuleId, "Item", _)));
}

TEST_F(ProjectStorage, update_exported_types_source_id)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    QmlDesigner::Storage::Synchronization::ExportedType exportedType{
        qmldir1SourceId, qtQuickModuleId, "Item", Storage::Version{}, sourceId2, "QQuickItem"};

    storage.synchronize(
        {.exportedTypes = {exportedType}, .updatedExportedTypeSourceIds = {qmldir1SourceId}});

    ASSERT_THAT(fetchExportedTypeNames(sourceId2, "QQuickItem"),
                ElementsAre(IsExportedTypeName(qtQuickModuleId, "Item", _)));
}

TEST_F(ProjectStorage, update_exported_context_source_id)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    QmlDesigner::Storage::Synchronization::ExportedType exportedType{
        qmldir3SourceId, qtQuickModuleId, "Item", Storage::Version{}, sourceId1, "QQuickItem"};

    storage.synchronize({.exportedTypes = {exportedType},
                         .updatedExportedTypeSourceIds = {qmldir1SourceId, qmldir3SourceId}});

    storage.synchronize({.updatedExportedTypeSourceIds = {qmldir3SourceId}});
    ASSERT_THAT(fetchExportedTypeNames(sourceId1, "QQuickItem"),
                Not(Contains(IsExportedTypeName(qtQuickModuleId, "Item", _))));
}

TEST_F(ProjectStorage,
       breaking_prototype_chain_by_deleting_base_component_notifies_unresolved_type_name)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Object"};
    storage.synchronize({.imports = importsSourceId1,
                         .updatedImportSourceIds = {sourceId1, sourceId2},
                         .exportedTypes = package.exportedTypes,
                         .updatedExportedTypeSourceIds = package.updatedExportedTypeSourceIds,
                         .types = package.types,
                         .updatedTypeSourceIds = {sourceId1, sourceId2}});

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize({.updatedExportedTypeSourceIds = {qmldir2SourceId}});
}

TEST_F(ProjectStorage, breaking_prototype_chain_by_deleting_base_component_has_unresolved_type_name)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Object"};
    storage.synchronize(package);

    storage.synchronize({.updatedExportedTypeSourceIds = {qmldir2SourceId}});

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(IsUnresolvedTypeId()));
}

TEST_F(ProjectStorage, repairing_prototype_chain_by_fixing_base_component)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Object"};
    auto exportedType = package.exportedTypes[2];
    package.exportedTypes.erase(std::next(package.exportedTypes.begin(), 2));
    storage.synchronize(package);

    storage.synchronize(
        {.exportedTypes = {exportedType}, .updatedExportedTypeSourceIds = {qmldir2SourceId}});

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(fetchTypeId(sourceId2, "QObject")));
}

TEST_F(ProjectStorage,
       breaking_extension_chain_by_deleting_base_component_notifies_unresolved_type_name)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].extension = Storage::Synchronization::ImportedType{"Object"};
    storage.synchronize(package);

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize({.updatedExportedTypeSourceIds = {qmldir2SourceId}});
}

TEST_F(ProjectStorage, breaking_extension_chain_by_deleting_base_component_has_unresolved_type_name)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].extension = Storage::Synchronization::ImportedType{"Object"};
    storage.synchronize(package);

    storage.synchronize({.updatedExportedTypeSourceIds = {qmldir2SourceId}});

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasExtensionId(IsUnresolvedTypeId()));
}

TEST_F(ProjectStorage, repairing_extension_chain_by_fixing_base_component)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].extension = Storage::Synchronization::ImportedType{"Object"};
    auto exportedType = package.exportedTypes[2];
    package.exportedTypes.erase(std::next(package.exportedTypes.begin(), 2));
    storage.synchronize(package);

    storage.synchronize(
        {.exportedTypes = {exportedType}, .updatedExportedTypeSourceIds = {qmldir2SourceId}});

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasExtensionId(fetchTypeId(sourceId2, "QObject")));
}

TEST_F(ProjectStorage, synchronize_types_add_property_declarations)
{
    auto package{createSimpleSynchronizationPackage()};

    storage.synchronize(package);

    ASSERT_THAT(fetchPropertyDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("children",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
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

    ASSERT_THAT(fetchPropertyDeclarations(sourceId1, "QQuickItem"),
                Contains(HasInfoPropertyTypeId(IsNullTypeId())));
}

TEST_F(ProjectStorage, synchronize_fixes_unresolved_property_declarations_with_missing_imports)
{
    auto package{createSimpleSynchronizationPackage()};
    package.imports.clear();
    storage.synchronize(package);

    storage.synchronize({.imports = importsSourceId1, .updatedImportSourceIds = {sourceId1}});

    ASSERT_THAT(fetchPropertyDeclarations(sourceId1, "QQuickItem"),
                Contains(IsInfoPropertyDeclaration("data", fetchTypeId(sourceId2, "QObject"), _)));
}

TEST_F(ProjectStorage, synchronize_types_add_property_declaration_qualified_type)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", "Quick"};
    package.types.emplace_back("QQuickObject",
                               Storage::Synchronization::ImportedType{"QObject"},
                               Storage::Synchronization::ImportedType{},
                               TypeTraitsKind::Reference,
                               sourceId1);
    package.exportedTypes.emplace_back(
        qmldir1SourceId, qtQuickModuleId, "Object", Storage::Version{}, sourceId1, "QQuickObject");
    package.updatedExportedTypeSourceIds.push_back(qmldir1SourceId);

    storage.synchronize(package);

    ASSERT_THAT(fetchPropertyDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId1, "QQuickObject"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("children",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
}

TEST_F(ProjectStorage, synchronize_types_changes_property_declaration_type)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "QQuickItem"};

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchPropertyDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("children",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
}

TEST_F(ProjectStorage, synchronize_types_changes_declaration_traits)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsPointer;

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchPropertyDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsPointer),
                    IsInfoPropertyDeclaration("children",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
}

TEST_F(ProjectStorage, synchronize_types_changes_declaration_traits_and_type)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsPointer;
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "QQuickItem"};

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchPropertyDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsPointer),
                    IsInfoPropertyDeclaration("children",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
}

TEST_F(ProjectStorage, synchronize_types_removes_a_property_declaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations.pop_back();

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchPropertyDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList)));
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

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchPropertyDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("object",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsPointer),
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("children",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
}

TEST_F(ProjectStorage, synchronize_types_rename_a_property_declaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[1].name = "objects";

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchPropertyDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
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

    ASSERT_THAT(fetchPropertyDeclarations(sourceId1, "QQuickItem"),
                Contains(HasInfoPropertyTypeId(IsNullTypeId())));
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

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchPropertyDeclarations(sourceId1, "QQuickItem"),
                Contains(HasInfoPropertyTypeId(fetchTypeId(sourceId2, "QObject"))));
}

TEST_F(ProjectStorage,
       using_non_existing_qualified_exported_property_type_with_wrong_name_notifies_unresovled_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object2", "Qml"};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object2"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage,
       using_non_existing_qualified_exported_property_type_with_wrong_name_has_null_type_id)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object2", "Qml"};

    storage.synchronize(package);

    ASSERT_THAT(fetchPropertyDeclarations(sourceId1, "QQuickItem"),
                Contains(HasInfoPropertyTypeId(IsNullTypeId())));
}

TEST_F(ProjectStorage, resolve_type_id_after_fixing_non_existing_qualified_property_type)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object2", "Qml"};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", "Qml"};

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchPropertyDeclarations(sourceId1, "QQuickItem"),
                Contains(IsInfoPropertyDeclaration("data", fetchTypeId(sourceId2, "QObject"), _)));
}

TEST_F(ProjectStorage,
       using_non_existing_qualified_exported_property_type_with_wrong_alias_notifies_unresovled_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = {};
    package.types[0].propertyDeclarations.pop_back();
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", "Qml2"};
    package.types.pop_back();
    package.imports = importsSourceId1;

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage,
       using_non_existing_qualified_exported_property_type_with_wrong_alias_has_null_type_id)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", "Qml2"};
    package.types.pop_back();
    package.imports = importsSourceId1;

    storage.synchronize(package);

    ASSERT_THAT(fetchPropertyDeclarations(sourceId1, "QQuickItem"),
                Contains(HasInfoPropertyTypeId(IsNullTypeId())));
}

TEST_F(ProjectStorage, resolve_qualified_exported_property_type_with_fixed_module)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", "Qml"};
    package.types.pop_back();
    package.imports = importsSourceId1;
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", "Qml"};

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchPropertyDeclarations(sourceId1, "QQuickItem"),
                Contains(IsInfoPropertyDeclaration("data", fetchTypeId(sourceId2, "QObject"), _)));
}

TEST_F(ProjectStorage,
       breaking_property_declaration_type_dependency_by_deleting_exported_type_notifies_unresolved_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].prototype = Storage::Synchronization::ImportedType{};
    std::erase_if(package.exportedTypes, [](auto &&e) { return e.name == "QObject"; });

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("QObject"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage, breaking_property_declaration_type_dependency_by_deleting_type_has_null_type_id)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].prototype = Storage::Synchronization::ImportedType{};
    std::erase_if(package.exportedTypes, [](auto &&e) { return e.name == "QObject"; });

    storage.synchronize(package);

    ASSERT_THAT(fetchPropertyDeclarations(sourceId1, "QQuickItem"),
                Contains(IsInfoPropertyDeclaration("data", IsNullTypeId(), _)));
}

TEST_F(ProjectStorage, fixing_broken_property_declaration_type_dependenc_has_valid_id)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].prototype = Storage::Synchronization::ImportedType{};
    auto exportedTypes = package.exportedTypes;
    std::erase_if(exportedTypes, [](auto &&e) { return e.name != "QObject"; });
    std::erase_if(package.exportedTypes, [](auto &&e) { return e.name == "QObject"; });
    storage.synchronize(package);

    storage.synchronize({.exportedTypes = exportedTypes,
                         .updatedTypeSourceIds = package.updatedExportedTypeSourceIds});

    ASSERT_THAT(fetchPropertyDeclarations(sourceId1, "QQuickItem"),
                Contains(IsInfoPropertyDeclaration("data", fetchTypeId(sourceId2, "QObject"), _)));
}

TEST_F(ProjectStorage, synchronize_types_add_function_declarations)
{
    auto package{createSimpleSynchronizationPackage()};

    storage.synchronize(package);

    ASSERT_THAT(fetchFunctionNames(sourceId1, "QQuickItem"),
                UnorderedElementsAre("execute", "values"));
}

TEST_F(ProjectStorage, synchronize_types_changes_function_declaration_return_type)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations[1].returnTypeName = "item";

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchFunctionDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                     Eq(package.types[0].functionDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_changes_function_declaration_name)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations[1].name = "name";

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchFunctionDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                     Eq(package.types[0].functionDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_changes_function_declaration_pop_parameters)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations[1].parameters.pop_back();

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchFunctionDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                     Eq(package.types[0].functionDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_changes_function_declaration_append_parameters)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations[1].parameters.push_back(
        Storage::Synchronization::ParameterDeclaration{"arg4", "int"});

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchFunctionDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                     Eq(package.types[0].functionDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_changes_function_declaration_change_parameter_name)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations[1].parameters[0].name = "other";

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchFunctionDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                     Eq(package.types[0].functionDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_changes_function_declaration_change_parameter_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations[1].parameters[0].name = "long long";

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchFunctionDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                     Eq(package.types[0].functionDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_changes_function_declaration_change_parameter_traits)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations[1].parameters[0].traits = QmlDesigner::Storage::
        PropertyDeclarationTraits::IsList;

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchFunctionDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                     Eq(package.types[0].functionDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_removes_function_declaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations.pop_back();

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchFunctionDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0])));
}

TEST_F(ProjectStorage, synchronize_types_add_function_declaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations.push_back(Storage::Synchronization::FunctionDeclaration{
        "name", "string", {Storage::Synchronization::ParameterDeclaration{"arg", "int"}}});

    storage.synchronize({.imports = importsSourceId1,
                         .updatedImportSourceIds = {sourceId1},
                         .types = {package.types[0]},
                         .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchFunctionDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                     Eq(package.types[0].functionDeclarations[1]),
                                     Eq(package.types[0].functionDeclarations[2])));
}

TEST_F(ProjectStorage, synchronize_types_add_function_declarations_with_overloads)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].functionDeclarations.push_back(
        Storage::Synchronization::FunctionDeclaration{"execute", "", {}});

    storage.synchronize(package);

    ASSERT_THAT(fetchFunctionDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                     Eq(package.types[0].functionDeclarations[1]),
                                     Eq(package.types[0].functionDeclarations[2])));
}

TEST_F(ProjectStorage, synchronize_types_function_declarations_adding_overload)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].functionDeclarations.push_back(
        Storage::Synchronization::FunctionDeclaration{"execute", "", {}});

    storage.synchronize(package);

    ASSERT_THAT(fetchFunctionDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                     Eq(package.types[0].functionDeclarations[1]),
                                     Eq(package.types[0].functionDeclarations[2])));
}

TEST_F(ProjectStorage, synchronize_types_function_declarations_removing_overload)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].functionDeclarations.push_back(
        Storage::Synchronization::FunctionDeclaration{"execute", "", {}});
    storage.synchronize(package);
    package.types[0].functionDeclarations.pop_back();

    storage.synchronize(package);

    ASSERT_THAT(fetchFunctionDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].functionDeclarations[0]),
                                     Eq(package.types[0].functionDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_add_signal_declarations)
{
    auto package{createSimpleSynchronizationPackage()};

    storage.synchronize(package);

    ASSERT_THAT(fetchSignalDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                     Eq(package.types[0].signalDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_changes_signal_declaration_name)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations[1].name = "name";

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchSignalDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                     Eq(package.types[0].signalDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_changes_signal_declaration_pop_parameters)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations[1].parameters.pop_back();

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchSignalDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                     Eq(package.types[0].signalDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_changes_signal_declaration_append_parameters)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations[1].parameters.push_back(
        Storage::Synchronization::ParameterDeclaration{"arg4", "int"});

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchSignalDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                     Eq(package.types[0].signalDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_changes_signal_declaration_change_parameter_name)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations[1].parameters[0].name = "other";

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchSignalDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                     Eq(package.types[0].signalDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_changes_signal_declaration_change_parameter_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations[1].parameters[0].typeName = "long long";

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchSignalDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                     Eq(package.types[0].signalDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_changes_signal_declaration_change_parameter_traits)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations[1].parameters[0].traits = QmlDesigner::Storage::PropertyDeclarationTraits::IsList;

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchSignalDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                     Eq(package.types[0].signalDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_removes_signal_declaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations.pop_back();

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchSignalDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0])));
}

TEST_F(ProjectStorage, synchronize_types_add_signal_declaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations.push_back(Storage::Synchronization::SignalDeclaration{
        "name", {Storage::Synchronization::ParameterDeclaration{"arg", "int"}}});

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchSignalDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                     Eq(package.types[0].signalDeclarations[1]),
                                     Eq(package.types[0].signalDeclarations[2])));
}

TEST_F(ProjectStorage, synchronize_types_add_signal_declarations_with_overloads)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].signalDeclarations.push_back(
        Storage::Synchronization::SignalDeclaration{"execute", {}});

    storage.synchronize(package);

    ASSERT_THAT(fetchSignalDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                     Eq(package.types[0].signalDeclarations[1]),
                                     Eq(package.types[0].signalDeclarations[2])));
}

TEST_F(ProjectStorage, synchronize_types_signal_declarations_adding_overload)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].signalDeclarations.push_back(
        Storage::Synchronization::SignalDeclaration{"execute", {}});

    storage.synchronize(package);

    ASSERT_THAT(fetchSignalDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                     Eq(package.types[0].signalDeclarations[1]),
                                     Eq(package.types[0].signalDeclarations[2])));
}

TEST_F(ProjectStorage, synchronize_types_signal_declarations_removing_overload)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].signalDeclarations.push_back(
        Storage::Synchronization::SignalDeclaration{"execute", {}});
    storage.synchronize(package);
    package.types[0].signalDeclarations.pop_back();

    storage.synchronize(package);

    ASSERT_THAT(fetchSignalDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].signalDeclarations[0]),
                                     Eq(package.types[0].signalDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_add_enumeration_declarations)
{
    auto package{createSimpleSynchronizationPackage()};

    storage.synchronize(package);

    ASSERT_THAT(fetchEnumerationDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                     Eq(package.types[0].enumerationDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_changes_enumeration_declaration_name)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations[1].name = "Name";

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchEnumerationDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                     Eq(package.types[0].enumerationDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_changes_enumeration_declaration_pop_enumerator_declaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations[1].enumeratorDeclarations.pop_back();

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchEnumerationDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                     Eq(package.types[0].enumerationDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_changes_enumeration_declaration_append_enumerator_declaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations[1].enumeratorDeclarations.push_back(
        Storage::Synchronization::EnumeratorDeclaration{"Haa", 54});

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchEnumerationDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                     Eq(package.types[0].enumerationDeclarations[1])));
}

TEST_F(ProjectStorage,
       synchronize_types_changes_enumeration_declaration_change_enumerator_declaration_name)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations[1].enumeratorDeclarations[0].name = "Hoo";

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchEnumerationDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                     Eq(package.types[0].enumerationDeclarations[1])));
}

TEST_F(ProjectStorage,
       synchronize_types_changes_enumeration_declaration_change_enumerator_declaration_value)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations[1].enumeratorDeclarations[1].value = 11;

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchEnumerationDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                     Eq(package.types[0].enumerationDeclarations[1])));
}

TEST_F(ProjectStorage,
       synchronize_types_changes_enumeration_declaration_add_that_enumerator_declaration_has_value)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations[1].enumeratorDeclarations[0].value = 11;
    package.types[0].enumerationDeclarations[1].enumeratorDeclarations[0].hasValue = true;

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchEnumerationDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                     Eq(package.types[0].enumerationDeclarations[1])));
}

TEST_F(ProjectStorage,
       synchronize_types_changes_enumeration_declaration_remove_that_enumerator_declaration_has_value)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations[1].enumeratorDeclarations[0].hasValue = false;

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchEnumerationDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                     Eq(package.types[0].enumerationDeclarations[1])));
}

TEST_F(ProjectStorage, synchronize_types_removes_enumeration_declaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations.pop_back();

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchEnumerationDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0])));
}

TEST_F(ProjectStorage, synchronize_types_add_enumeration_declaration)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].enumerationDeclarations.push_back(Storage::Synchronization::EnumerationDeclaration{
        "name", {Storage::Synchronization::EnumeratorDeclaration{"Foo", 98, true}}});

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchEnumerationDeclarations(sourceId1, "QQuickItem"),
                UnorderedElementsAre(Eq(package.types[0].enumerationDeclarations[0]),
                                     Eq(package.types[0].enumerationDeclarations[1]),
                                     Eq(package.types[0].enumerationDeclarations[2])));
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
    auto qtQuickModuleId = modulesStorage.moduleId("QtQuick", ModuleKind::QmlLibrary);

    auto typeId = storage.fetchTypeIdByModuleIdsAndExportedName({qtQuickModuleId}, "Object");

    ASSERT_FALSE(typeId.isValid());
}

TEST_F(ProjectStorage, synchronize_types_add_alias_declarations)
{
    auto package{createSynchronizationPackageWithAliases()};

    storage.synchronize(package);

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage, synchronize_types_add_alias_declarations_again)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);

    storage.synchronize(package);

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage, synchronize_types_remove_alias_declarations)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations.pop_back();

    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList)));
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

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                Contains(IsInfoPropertyDeclaration("items", IsNullTypeId(), _)));
}

TEST_F(ProjectStorage, synchronize_types_fixes_null_type_id_after_add_alias_declarations_for_wrong_type)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "QQuickItemWrong"};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "QQuickItem"};

    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                Contains(IsInfoPropertyDeclaration("items", fetchTypeId(sourceId1, "QQuickItem"), _)));
}

TEST_F(ProjectStorage, synchronize_types_add_alias_declarations_notifies_error_for_wrong_property_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[1].aliasPropertyName = "childrenWrong";

    EXPECT_CALL(errorNotifierMock, propertyNameDoesNotExists(Eq("childrenWrong"), sourceId3))
        .Times(AtLeast(1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage,
       synchronize_types_add_alias_declarations_returns_invalid_type_for_wrong_property_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[1].aliasPropertyName = "childrenWrong";

    storage.synchronize(package);

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                Contains(IsInfoPropertyDeclaration("items", IsNullTypeId(), _)));
}

TEST_F(ProjectStorage,
       synchronize_types_update_alias_declarations_returns_item_type_for_fixed_property_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[1].aliasPropertyName = "childrenWrong";
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].aliasPropertyName = "children";

    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                Contains(IsInfoPropertyDeclaration("items", fetchTypeId(sourceId1, "QQuickItem"), _)));
}

TEST_F(ProjectStorage, synchronize_types_change_alias_declarations_type_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[2].typeName = Storage::Synchronization::ImportedType{
        "Obj2"};
    importsSourceId3.emplace_back(pathToModuleId, Storage::Version{}, sourceId3, sourceId3);

    storage.synchronize({.imports = importsSourceId3,
                         .updatedImportSourceIds = {sourceId3},
                         .types = {package.types[2]},
                         .updatedTypeSourceIds = {sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage, synchronize_types_change_alias_declarations_property_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[2].aliasPropertyName = "children";

    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList)));
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

    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList)));
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

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage, synchronize_types_change_alias_target_property_declaration_traits)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[1].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly;

    storage.synchronize({.types = {package.types[1]}, .updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage, synchronize_types_change_alias_target_property_declaration_type_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Item"};
    importsSourceId2.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2, sourceId2);

    storage.synchronize({.imports = importsSourceId2,
                         .updatedImportSourceIds = {sourceId2},
                         .types = {package.types[1]},
                         .updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage, synchronize_types_remove_property_declaration_with_an_alias_throws)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[1].propertyDeclarations.pop_back();

    ASSERT_THROW(storage.synchronize(
                     {.types = {package.types[1]}, .updatedTypeSourceIds = {sourceId2}}),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorage, synchronize_types_remove_property_declaration_and_alias)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[1].propertyDeclarations.pop_back();
    package.types[2].propertyDeclarations.pop_back();

    storage.synchronize({.types = {package.types[1], package.types[2]},
                         .updatedTypeSourceIds = {sourceId2, sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage,
       synchronize_remove_type_with_alias_target_property_declaration_nofifies_unresolved_type_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[2].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId3, sourceId3);
    storage.synchronize(package);

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object2"), sourceId3));

    storage.synchronize({.updatedImportSourceIds = {sourceId4}, .updatedTypeSourceIds = {sourceId4}});
}

TEST_F(ProjectStorage,
       synchronize_remove_type_with_alias_target_property_declaration_set_unresolved_type_id)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[2].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId3, sourceId3);
    storage.synchronize(package);

    storage.synchronize({.updatedImportSourceIds = {sourceId4}, .updatedTypeSourceIds = {sourceId4}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                Contains(IsInfoPropertyDeclaration("objects", IsNullTypeId(), _)));
}

TEST_F(ProjectStorage, synchronize_fixes_type_with_alias_target_property_declaration)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[2].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId3, sourceId3);
    storage.synchronize(package);
    storage.synchronize({.updatedImportSourceIds = {sourceId4}, .updatedTypeSourceIds = {sourceId4}});

    storage.synchronize({.imports = importsSourceId4 + moduleDependenciesSourceId4,
                         .updatedImportSourceIds = {sourceId4},
                         .types = {package.types[3]},
                         .updatedTypeSourceIds = {sourceId4}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                Contains(IsInfoPropertyDeclaration("objects", fetchTypeId(sourceId2, "QObject"), _)));
}

TEST_F(ProjectStorage, synchronize_types_remove_type_and_alias_property_declaration)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[2].propertyDeclarations[2].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId3, sourceId3);
    storage.synchronize(package);
    package.types[2].propertyDeclarations.pop_back();

    storage.synchronize({.types = {package.types[0], package.types[2]},
                         .updatedTypeSourceIds = {sourceId1, sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage, update_alias_property_if_property_is_overloaded)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects",
        Storage::Synchronization::ImportedType{"QQuickItem"},
        Storage::PropertyDeclarationTraits::IsList | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage, alias_property_is_overloaded)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[0].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects",
        Storage::Synchronization::ImportedType{"QQuickItem"},
        Storage::PropertyDeclarationTraits::IsList | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize(package);

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList)));
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

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage, relink_alias_property)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2, sourceId2);
    storage.synchronize(package);
    Storage::Synchronization::ExportedType exportedType{qmldir4DirectorySourceId,
                                                        qtQuickModuleId,
                                                        "Object2",
                                                        Storage::Version{},
                                                        sourceId4,
                                                        "QObject2"};

    storage.synchronize({.exportedTypes = {exportedType},
                         .updatedExportedTypeSourceIds = {qmldir4DirectorySourceId}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId4, "QObject2"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage, do_not_relink_alias_property_for_qualified_imported_type_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object2", "Local"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2, "Local");
    storage.synchronize(package);
    Storage::Synchronization::ExportedType exportedType{qmldir4DirectorySourceId,
                                                        qtQuickModuleId,
                                                        "Object2",
                                                        Storage::Version{},
                                                        sourceId4,
                                                        "QObject2"};
    importsSourceId4.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId4, sourceId4);

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object2"), sourceId2));

    storage.synchronize({.imports = importsSourceId4 + moduleDependenciesSourceId4,
                         .updatedImportSourceIds = {sourceId4},
                         .exportedTypes = {exportedType},
                         .updatedExportedTypeSourceIds = {qmldir4DirectorySourceId}});
}

TEST_F(ProjectStorage, not_relinked_alias_property_for_qualified_imported_type_name_has_null_type_id)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object2", "Local"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2, "Local");
    storage.synchronize(package);
    Storage::Synchronization::ExportedType exportedType{qmldir4DirectorySourceId,
                                                        qtQuickModuleId,
                                                        "Object2",
                                                        Storage::Version{},
                                                        sourceId4,
                                                        "QObject2"};
    importsSourceId4.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId4, sourceId4);

    storage.synchronize({.imports = importsSourceId4 + moduleDependenciesSourceId4,
                         .updatedImportSourceIds = {sourceId4},
                         .exportedTypes = {exportedType},
                         .updatedExportedTypeSourceIds = {qmldir4DirectorySourceId}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                Contains(IsInfoPropertyDeclaration("objects", IsNullTypeId(), _)));
}

TEST_F(ProjectStorage,
       relinked_alias_property_for_qualified_imported_type_name_after_alias_chain_was_fixed)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object2", "Local"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2, "Local");
    storage.synchronize(package);
    Storage::Synchronization::ExportedType exportedType{qmldir4DirectorySourceId,
                                                        qtQuickModuleId,
                                                        "Object2",
                                                        Storage::Version{},
                                                        sourceId4,
                                                        "QObject2"};
    importsSourceId4.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId4, sourceId4);
    storage.synchronize({.imports = importsSourceId4,
                         .updatedImportSourceIds = {sourceId4},
                         .exportedTypes = {exportedType},
                         .updatedExportedTypeSourceIds = {qmldir4DirectorySourceId}});
    exportedType.moduleId = pathToModuleId;

    storage.synchronize({.imports = importsSourceId4 + moduleDependenciesSourceId4,
                         .updatedImportSourceIds = {sourceId4},
                         .exportedTypes = {exportedType},
                         .updatedExportedTypeSourceIds = {qmldir4DirectorySourceId}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                Contains(IsInfoPropertyDeclaration("objects", fetchTypeId(sourceId4, "QObject2"), _)));
}

TEST_F(ProjectStorage,
       do_relink_alias_property_for_qualified_imported_type_name_even_if_an_other_similar_time_name_exists)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object2", "Local"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2, "Local");
    package.types.push_back(Storage::Synchronization::Type{"QObject2",
                                                           Storage::Synchronization::ImportedType{},
                                                           Storage::Synchronization::ImportedType{},
                                                           TypeTraitsKind::Reference,
                                                           sourceId5});
    package.exportedTypes.emplace_back(
        qmldir5SourceId, qtQuickModuleId, "Object2", Storage::Version{}, sourceId5, "QObject2");
    package.updatedTypeSourceIds.push_back(sourceId5);
    package.updatedExportedTypeSourceIds.push_back(qmldir5SourceId);

    storage.synchronize(package);

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId4, "QObject2"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage, relink_alias_property_react_to_type_name_change)
{
    auto package{createSynchronizationPackageWithAliases2()};
    package.types[2].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "items", Storage::Synchronization::ImportedType{"Item"}, "children"});
    storage.synchronize(package);
    package.types[0].typeName = "QQuickItem2";
    Storage::Synchronization::ExportedTypes exportedTypes;
    exportedTypes.emplace_back(
        qmldir1SourceId, qtQuickModuleId, "Item", Storage::Version{}, sourceId1, "QQuickItem2");
    exportedTypes.emplace_back(qmldir1DirectorySourceId,
                               qtQuickNativeModuleId,
                               "QQuickItem",
                               Storage::Version{},
                               sourceId1,
                               "QQuickItem2");

    storage.synchronize({.exportedTypes = exportedTypes,
                         .updatedExportedTypeSourceIds = {qmldir1SourceId, qmldir1DirectorySourceId},
                         .types = {package.types[0]},
                         .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem2"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("data",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage, do_not_relink_alias_property_for_deleted_type)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2, sourceId2);
    storage.synchronize(package);
    QmlDesigner::Storage::Synchronization::ExportedType exportedType{qmldir4DirectorySourceId,
                                                                     qtQuickModuleId,
                                                                     "Object2",
                                                                     Storage::Version{},
                                                                     sourceId4,
                                                                     "QObject2"};

    storage.synchronize({.exportedTypes = {exportedType},
                         .updatedExportedTypeSourceIds = {qmldir3SourceId,
                                                          qmldir3DirectorySourceId,
                                                          qmldir4DirectorySourceId},
                         .updatedTypeSourceIds = {sourceId3}});

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
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2, sourceId2);
    storage.synchronize(package);
    package.types[0].prototype = Storage::Synchronization::ImportedType{};
    importsSourceId1.emplace_back(pathToModuleId, Storage::Version{}, sourceId1, sourceId1);
    importsSourceId4.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId4, sourceId4);
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.types[3].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Item"};

    storage.synchronize({.imports = importsSourceId1 + importsSourceId4,
                         .updatedImportSourceIds = {sourceId1, sourceId2, sourceId3, sourceId4},
                         .types = {package.types[0], package.types[3]},
                         .updatedTypeSourceIds = {sourceId1, sourceId2, sourceId3, sourceId4}});

    ASSERT_THAT(storage.fetchTypes(), SizeIs(2));
}

TEST_F(ProjectStorage, do_not_relink_alias_property_for_deleted_type_and_property_type_name_change)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2);
    storage.synchronize(package);
    QmlDesigner::Storage::Synchronization::ExportedType exportedType{qmldir4DirectorySourceId,
                                                                     qtQuickModuleId,
                                                                     "Object2",
                                                                     Storage::Version{},
                                                                     sourceId4,
                                                                     "QObject2"};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "QObject"};
    importsSourceId4.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId4, sourceId4);

    storage.synchronize({.imports = importsSourceId2 + importsSourceId4,
                         .updatedImportSourceIds = {sourceId2, sourceId3, sourceId4},
                         .exportedTypes = {exportedType},
                         .updatedExportedTypeSourceIds = {qmldir4DirectorySourceId},
                         .updatedTypeSourceIds = {sourceId3}});

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
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2, sourceId2);
    storage.synchronize(package);

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object2"), sourceId2));

    storage.synchronize({.updatedExportedTypeSourceIds = {qmldir4DirectorySourceId}});
}

TEST_F(ProjectStorage, not_relinked_property_type_has_null_type_id)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2, sourceId2);
    storage.synchronize(package);

    storage.synchronize({.updatedExportedTypeSourceIds = {qmldir4DirectorySourceId}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId2, "QObject"),
                Contains(IsInfoPropertyDeclaration("objects", IsNullTypeId(), _)));
}

TEST_F(ProjectStorage, not_relinked_property_type_fixed_after_adding_property_type_again)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2, sourceId2);
    storage.synchronize(package);
    storage.synchronize({.updatedImportSourceIds = {sourceId4}, .updatedTypeSourceIds = {sourceId4}});

    storage.synchronize({.imports = importsSourceId4 + moduleDependenciesSourceId4,
                         .updatedImportSourceIds = {sourceId4},
                         .types = {package.types[3]},
                         .updatedTypeSourceIds = {sourceId4}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId2, "QObject"),
                Contains(IsInfoPropertyDeclaration("objects", fetchTypeId(sourceId4, "QObject2"), _)));
}

TEST_F(ProjectStorage, not_relinked_property_type_fixed_after_type_is_added_again)
{
    auto package{createSynchronizationPackageWithAliases()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2, sourceId2);
    storage.synchronize(package);
    storage.synchronize({.updatedImportSourceIds = {sourceId4}, .updatedTypeSourceIds = {sourceId4}});

    storage.synchronize({.imports = importsSourceId4 + moduleDependenciesSourceId4,
                         .updatedImportSourceIds = {sourceId4},
                         .types = {package.types[3]},
                         .updatedTypeSourceIds = {sourceId4}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId2, "QObject"),
                Contains(IsInfoPropertyDeclaration("objects", fetchTypeId(sourceId4, "QObject2"), _)));
}

TEST_F(ProjectStorage, do_not_relink_alias_property_type_does_not_exists_notifies_unresolved_type_name)
{
    auto package{createSynchronizationPackageWithAliases2()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2);
    storage.synchronize(package);
    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Item"), sourceId1));

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Item"), sourceId3));

    storage.synchronize({.updatedExportedTypeSourceIds = {qmldir1SourceId}});
}

TEST_F(ProjectStorage, not_relinked_alias_property_type_has_null_type_id)
{
    auto package{createSynchronizationPackageWithAliases2()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2);
    storage.synchronize(package);

    storage.synchronize({.updatedExportedTypeSourceIds = {qmldir1SourceId}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                Contains(IsInfoPropertyDeclaration("objects", IsNullTypeId(), _)));
}

TEST_F(ProjectStorage, not_relinked_alias_property_type_fixed_after_referenced_type_is_added_again)
{
    auto package{createSynchronizationPackageWithAliases2()};
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2);
    storage.synchronize(package);
    storage.synchronize({.updatedExportedTypeSourceIds = {qmldir1SourceId}});
    QmlDesigner::Storage::Synchronization::ExportedType exportedType{
        qmldir1SourceId, qtQuickModuleId, "Item", Storage::Version{}, sourceId1, "QQuickItem"};

    storage.synchronize(
        {.exportedTypes = {exportedType}, .updatedExportedTypeSourceIds = {qmldir1SourceId}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                Contains(IsInfoPropertyDeclaration("objects", fetchTypeId(sourceId4, "QObject2"), _)));
}

TEST_F(ProjectStorage, change_prototype_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[1].typeName = "QObject3";
    Storage::Synchronization::ExportedTypes exportedTypes;
    exportedTypes.emplace_back(
        qmldir2SourceId, qmlModuleId, "Object", Storage::Version{2}, sourceId2, "QObject3");
    exportedTypes.emplace_back(
        qmldir2SourceId, qmlModuleId, "Obj", Storage::Version{2}, sourceId2, "QObject3");
    exportedTypes.emplace_back(qmldir2DirectorySourceId,
                               qmlNativeModuleId,
                               "QObject",
                               Storage::Version{2},
                               sourceId2,
                               "QObject3");

    storage.synchronize({.exportedTypes = exportedTypes,
                         .updatedExportedTypeSourceIds = {qmldir2SourceId, qmldir2DirectorySourceId},
                         .types = {package.types[1]},
                         .updatedTypeSourceIds = {sourceId2}});

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
    Storage::Synchronization::ExportedTypes exportedTypes;
    exportedTypes.emplace_back(
        qmldir2SourceId, qmlModuleId, "Object", Storage::Version{2}, sourceId2, "QObject3");
    exportedTypes.emplace_back(
        qmldir2SourceId, qmlModuleId, "Obj", Storage::Version{2}, sourceId2, "QObject3");
    exportedTypes.emplace_back(qmldir2DirectorySourceId,
                               qmlNativeModuleId,
                               "QObject",
                               Storage::Version{2},
                               sourceId2,
                               "QObject3");

    storage.synchronize({.exportedTypes = exportedTypes,
                         .updatedExportedTypeSourceIds = {qmldir2SourceId, qmldir2DirectorySourceId},
                         .types = {package.types[1]},
                         .updatedTypeSourceIds = {sourceId2}});

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
    Storage::Synchronization::ExportedType exportedType{qmldir2DirectorySourceId,
                                                        qtQuickModuleId,
                                                        "QObject",
                                                        Storage::Version{2},
                                                        sourceId2,
                                                        "QObject"};

    storage.synchronize({.exportedTypes = {exportedType},
                         .updatedExportedTypeSourceIds = {qmldir2SourceId, qmldir2DirectorySourceId}});

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
    Storage::Synchronization::ExportedType exportedType{qmldir2DirectorySourceId,
                                                        qtQuickModuleId,
                                                        "QObject",
                                                        Storage::Version{2},
                                                        sourceId2,
                                                        "QObject"};

    storage.synchronize({.exportedTypes = {exportedType},
                         .updatedExportedTypeSourceIds = {qmldir2SourceId, qmldir2DirectorySourceId}});

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
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{"Object", "Qml"};
    storage.synchronize(package);
    Storage::Synchronization::ExportedType exportedType{
        qmldir2SourceId, qtQuickModuleId, "Object", Storage::Version{2}, sourceId2, "QObject"};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(
        {.exportedTypes = {exportedType}, .updatedExportedTypeSourceIds = {qmldir2SourceId}});
}

TEST_F(ProjectStorage, change_qualified_extension_type_module_id_notifies_cannot_resolve)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{"Object", "Qml"};
    storage.synchronize(package);
    Storage::Synchronization::ExportedType exportedType{
        qmldir2SourceId, qtQuickModuleId, "Object", Storage::Version{2}, sourceId2, "QObject"};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(
        {.exportedTypes = {exportedType}, .updatedExportedTypeSourceIds = {qmldir2SourceId}});
}

TEST_F(ProjectStorage, change_qualified_prototype_type_module_id)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{"Object", "Qml"};
    storage.synchronize(package);
    Storage::Synchronization::ExportedType exportedType{
        qmldir2SourceId, qtQuickModuleId, "Object", Storage::Version{2}, sourceId2, "QObject"};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{"Object", "Quick"};

    storage.synchronize({.exportedTypes = {exportedType},
                         .updatedExportedTypeSourceIds = {qmldir2SourceId},
                         .types = {package.types[0]},
                         .updatedTypeSourceIds = {sourceId1}});

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
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{"Object", "Qml"};
    storage.synchronize(package);
    Storage::Synchronization::ExportedType exportedType{
        qmldir2SourceId, qtQuickModuleId, "Object", Storage::Version{2}, sourceId2, "QObject"};
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{"Object", "Quick"};

    storage.synchronize({.exportedTypes = {exportedType},
                         .updatedExportedTypeSourceIds = {qmldir2SourceId},
                         .types = {package.types[0]},
                         .updatedTypeSourceIds = {sourceId1}});

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
    Storage::Synchronization::ExportedTypes exportedTypes;
    exportedTypes.emplace_back(
        qmldir2SourceId, qtQuickModuleId, "Object", Storage::Version{2}, sourceId2, "QObject3");
    exportedTypes.emplace_back(qmldir2DirectorySourceId,
                               qtQuickModuleId,
                               "QObject3",
                               Storage::Version{2},
                               sourceId2,
                               "QObject3");
    package.types[1].typeName = "QObject3";

    storage.synchronize({.exportedTypes = exportedTypes,
                         .updatedExportedTypeSourceIds = {qmldir2SourceId, qmldir2DirectorySourceId},
                         .types = {package.types[1]},
                         .updatedTypeSourceIds = {sourceId2}});

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
    Storage::Synchronization::ExportedTypes exportedTypes;
    exportedTypes.emplace_back(
        qmldir2SourceId, qtQuickModuleId, "Object", Storage::Version{2}, sourceId2, "QObject3");
    exportedTypes.emplace_back(qmldir2DirectorySourceId,
                               qtQuickModuleId,
                               "QObject3",
                               Storage::Version{2},
                               sourceId2,
                               "QObject3");
    package.types[1].typeName = "QObject3";

    storage.synchronize({.exportedTypes = exportedTypes,
                         .updatedExportedTypeSourceIds = {qmldir2SourceId, qmldir2DirectorySourceId},
                         .types = {package.types[1]},
                         .updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       TypeId{},
                                       fetchTypeId(sourceId2, "QObject3"),
                                       TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, change_prototype_type_name_notifies_error_for_wrong_native_prototupe_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object"};
    storage.synchronize(package);
    Storage::Synchronization::ExportedTypes exportedTypes;
    exportedTypes.emplace_back(
        qmldir2SourceId, qmlModuleId, "Object", Storage::Version{2}, sourceId2, "QObject3");
    exportedTypes.emplace_back(qmldir2DirectorySourceId,
                               qmlNativeModuleId,
                               "QObject3",
                               Storage::Version{2},
                               sourceId2,
                               "QObject3");
    package.types[1].typeName = "QObject3";

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("QObject"), sourceId1));

    storage.synchronize({.exportedTypes = exportedTypes,
                         .updatedExportedTypeSourceIds = {qmldir2SourceId, qmldir2DirectorySourceId},
                         .types = {package.types[1]},
                         .updatedTypeSourceIds = {sourceId2}});
}

TEST_F(ProjectStorage, change_extension_type_name_notifies_error_for_wrong_native_extension_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object"};
    storage.synchronize(package);
    Storage::Synchronization::ExportedTypes exportedTypes;
    exportedTypes.emplace_back(
        qmldir2SourceId, qmlModuleId, "Object", Storage::Version{2}, sourceId2, "QObject3");
    exportedTypes.emplace_back(qmldir2DirectorySourceId,
                               qmlNativeModuleId,
                               "QObject3",
                               Storage::Version{2},
                               sourceId2,
                               "QObject3");
    package.types[1].typeName = "QObject3";

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("QObject"), sourceId1));

    storage.synchronize({.exportedTypes = exportedTypes,
                         .updatedExportedTypeSourceIds = {qmldir2SourceId, qmldir2DirectorySourceId},
                         .types = {package.types[1]},
                         .updatedTypeSourceIds = {sourceId2}});
}

TEST_F(ProjectStorage, throw_for_prototype_chain_cycles)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[1].prototype = Storage::Synchronization::ImportedType{"Object2"};
    package.types.emplace_back("QObject2",
                               Storage::Synchronization::ImportedType{"Item"},
                               Storage::Synchronization::ImportedType{},
                               TypeTraitsKind::Reference,
                               sourceId3);
    package.exportedTypes.emplace_back(
        qmldir3SourceId, pathToModuleId, "Object2", Storage::Version{}, sourceId3, "QObject2");
    package.exportedTypes.emplace_back(
        qmldir3SourceId, pathToModuleId, "Obj2", Storage::Version{}, sourceId3, "QObject2");
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId3, sourceId3);

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorage, notifies_error_for_prototype_chain_cycles)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[1].prototype = Storage::Synchronization::ImportedType{"Object2"};
    package.types.emplace_back("QObject2",
                               Storage::Synchronization::ImportedType{"Item"},
                               Storage::Synchronization::ImportedType{},
                               TypeTraitsKind::Reference,
                               sourceId3);
    package.exportedTypes.emplace_back(
        qmldir3SourceId, pathToModuleId, "Object2", Storage::Version{}, sourceId3, "QObject2");
    package.exportedTypes.emplace_back(
        qmldir3SourceId, pathToModuleId, "Obj2", Storage::Version{}, sourceId3, "QObject2");
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId3, sourceId3);

    EXPECT_CALL(errorNotifierMock, prototypeCycle(Eq("QObject2"), sourceId3));

    EXPECT_ANY_THROW(storage.synchronize(package));
}

TEST_F(ProjectStorage, throw_for_extension_chain_cycles)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[1].extension = Storage::Synchronization::ImportedType{"Object2"};
    package.types.emplace_back("QObject2",
                               Storage::Synchronization::ImportedType{"Item"},
                               Storage::Synchronization::ImportedType{},
                               TypeTraitsKind::Reference,
                               sourceId3);
    package.exportedTypes.emplace_back(
        qmldir3SourceId, pathToModuleId, "Object2", Storage::Version{}, sourceId3, "QObject2");
    package.exportedTypes.emplace_back(
        qmldir3SourceId, pathToModuleId, "Obj2", Storage::Version{}, sourceId3, "QObject2");
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2);
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId3, sourceId3);

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::PrototypeChainCycle);
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
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Item"};

    ASSERT_THROW(storage.synchronize(
                     {.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}}),
                 QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorage, throw_for_type_id_and_extenssion_id_are_the_same_for_relinking)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.types[0].extension = Storage::Synchronization::ImportedType{"Item"};

    ASSERT_THROW(storage.synchronize(
                     {.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}}),
                 QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorage, recursive_aliases)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};

    storage.synchronize(package);

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId5, "QAliasItem2"),
                ElementsAre(IsInfoPropertyDeclaration("objects",
                                                      fetchTypeId(sourceId2, "QObject"),
                                                      Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage, recursive_aliases_change_property_type)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};
    storage.synchronize(package);
    package.types[1].propertyDeclarations[0].typeName = Storage::Synchronization::ImportedType{
        "Object2"};
    importsSourceId2.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2);

    storage.synchronize({.imports = importsSourceId2,
                         .updatedImportSourceIds = {sourceId2},
                         .types = {package.types[1]},
                         .updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId5, "QAliasItem2"),
                ElementsAre(IsInfoPropertyDeclaration("objects",
                                                      fetchTypeId(sourceId4, "QObject2"),
                                                      Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage, update_aliases_after_injecting_property)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects",
        Storage::Synchronization::ImportedType{"Item"},
        Storage::PropertyDeclarationTraits::IsList | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId5, "QAliasItem2"),
                ElementsAre(
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
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

    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId5, "QAliasItem2"),
                ElementsAre(
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                ElementsAre(
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
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
    importsSourceId2.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2);

    storage.synchronize({.imports = importsSourceId2,
                         .updatedImportSourceIds = {sourceId2},
                         .types = {package.types[1]},
                         .updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId5, "QAliasItem2"),
                ElementsAre(
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
}

TEST_F(ProjectStorage, check_for_alias_type_cycle_throws)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};
    package.types[1].propertyDeclarations.clear();
    package.types[1].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects", Storage::Synchronization::ImportedType{"AliasItem2"}, "objects"});
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2, sourceId2);

    ASSERT_THROW(storage.synchronize(package), QmlDesigner::AliasChainCycle);
}

TEST_F(ProjectStorage, check_for_alias_type_cycle_after_update_throws)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};
    storage.synchronize(package);
    package.types[1].propertyDeclarations.clear();
    package.types[1].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects", Storage::Synchronization::ImportedType{"AliasItem2"}, "objects"});
    importsSourceId2.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2, sourceId2);

    ASSERT_THROW(storage.synchronize({.imports = importsSourceId2,
                                      .updatedImportSourceIds = {sourceId2},
                                      .types = {package.types[1]},
                                      .updatedTypeSourceIds = {sourceId2}}),
                 QmlDesigner::AliasChainCycle);
}

TEST_F(ProjectStorage, check_for_alias_type_cycle_after_update_notifies_about_error)
{
    auto package{createSynchronizationPackageWithRecursiveAliases()};
    storage.synchronize(package);
    package.types[1].propertyDeclarations.clear();
    package.types[1].propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects", Storage::Synchronization::ImportedType{"AliasItem2"}, "objects"});
    importsSourceId2.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2, sourceId2);

    EXPECT_CALL(errorNotifierMock, aliasCycle(Eq("QObject"), Eq("objects"), sourceId2));

    EXPECT_ANY_THROW(storage.synchronize({.imports = importsSourceId2,
                                          .updatedImportSourceIds = {sourceId2},
                                          .types = {package.types[1]},
                                          .updatedTypeSourceIds = {sourceId2}}));
}

TEST_F(ProjectStorage, qualified_prototype)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{"Object", "Qml"};
    package.types.emplace_back("QQuickObject",
                               Storage::Synchronization::ImportedType{},
                               Storage::Synchronization::ImportedType{},
                               TypeTraitsKind::Reference,
                               sourceId3);
    package.exportedTypes.emplace_back(
        qmldir3SourceId, qtQuickModuleId, "Object", Storage::Version{}, sourceId3, "QQuickObject");
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
    package.updatedTypeSourceIds.push_back(sourceId3);
    package.updatedImportSourceIds.push_back(sourceId3);
    package.updatedExportedTypeSourceIds.push_back(qmldir3SourceId);

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
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{"Object", "Qml"};
    package.types.emplace_back("QQuickObject",
                               Storage::Synchronization::ImportedType{},
                               Storage::Synchronization::ImportedType{},
                               TypeTraitsKind::Reference,
                               sourceId3);
    package.exportedTypes.emplace_back(
        qmldir3SourceId, qtQuickModuleId, "Object", Storage::Version{}, sourceId3, "QQuickObject");
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
    package.updatedTypeSourceIds.push_back(sourceId3);
    package.updatedImportSourceIds.push_back(sourceId3);
    package.updatedExportedTypeSourceIds.push_back(qmldir3SourceId);

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
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{"Object", "Quick"};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage,
       qualified_extension_upper_down_the_module_chain_notifies_type_name_cannot_be_resolved)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{"Object", "Quick"};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage, qualified_prototype_upper_in_the_module_chain)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{"Object", "Quick"};
    package.types.emplace_back("QQuickObject",
                               Storage::Synchronization::ImportedType{},
                               Storage::Synchronization::ImportedType{},
                               TypeTraitsKind::Reference,
                               sourceId3);
    package.exportedTypes.emplace_back(
        qmldir3SourceId, qtQuickModuleId, "Object", Storage::Version{}, sourceId3, "QQuickObject");
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
    package.updatedTypeSourceIds.push_back(sourceId3);
    package.updatedImportSourceIds.push_back(sourceId3);
    package.updatedExportedTypeSourceIds.push_back(qmldir3SourceId);

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
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{"Object", "Quick"};
    package.types.emplace_back("QQuickObject",
                               Storage::Synchronization::ImportedType{},
                               Storage::Synchronization::ImportedType{},
                               TypeTraitsKind::Reference,
                               sourceId3);
    package.exportedTypes.emplace_back(
        qmldir3SourceId, qtQuickModuleId, "Object", Storage::Version{}, sourceId3, "QQuickObject");
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
    package.updatedTypeSourceIds.push_back(sourceId3);
    package.updatedImportSourceIds.push_back(sourceId3);
    package.updatedExportedTypeSourceIds.push_back(qmldir3SourceId);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       TypeId{},
                                       fetchTypeId(sourceId3, "QQuickObject"),
                                       TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, qualified_prototype_with_wrong_alias_notifies_type_name_cannot_be_resolved)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{"Object", "Qml2"};
    package.types.emplace_back("QQuickObject",
                               Storage::Synchronization::ImportedType{},
                               Storage::Synchronization::ImportedType{},
                               TypeTraitsKind::Reference,
                               sourceId3);
    package.exportedTypes.emplace_back(
        qmldir3SourceId, qtQuickModuleId, "Object", Storage::Version{}, sourceId3, "QQuickObject");
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
    package.updatedTypeSourceIds.push_back(sourceId3);
    package.updatedImportSourceIds.push_back(sourceId3);
    package.updatedExportedTypeSourceIds.push_back(qmldir3SourceId);

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage, qualified_extension_with_wrong_version_notifies_type_name_cannot_be_resolved)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{"Object", "Qml2"};
    package.types.emplace_back("QQuickObject",
                               Storage::Synchronization::ImportedType{},
                               Storage::Synchronization::ImportedType{},
                               TypeTraitsKind::Reference,
                               sourceId3);
    package.exportedTypes.emplace_back(
        qmldir3SourceId, qtQuickModuleId, "Object", Storage::Version{}, sourceId3, "QQuickObject");
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
    package.updatedTypeSourceIds.push_back(sourceId3);
    package.updatedImportSourceIds.push_back(sourceId3);
    package.updatedExportedTypeSourceIds.push_back(qmldir3SourceId);

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage, qualified_prototype_with_version)
{
    auto package{createSimpleSynchronizationPackage()};
    package.imports[1].version = Storage::Version{2};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{"Object", "Qml"};
    package.types.emplace_back("QQuickObject",
                               Storage::Synchronization::ImportedType{},
                               Storage::Synchronization::ImportedType{},
                               TypeTraitsKind::Reference,
                               sourceId3);
    package.exportedTypes.emplace_back(
        qmldir3SourceId, qtQuickModuleId, "Object", Storage::Version{}, sourceId3, "QQuickObject");
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
    package.updatedTypeSourceIds.push_back(sourceId3);
    package.updatedImportSourceIds.push_back(sourceId3);
    package.updatedExportedTypeSourceIds.push_back(qmldir3SourceId);

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
    package.imports[1].version = Storage::Version{2};
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{"Object", "Qml"};
    package.types.emplace_back("QQuickObject",
                               Storage::Synchronization::ImportedType{},
                               Storage::Synchronization::ImportedType{},
                               TypeTraitsKind::Reference,
                               sourceId3);
    package.exportedTypes.emplace_back(
        qmldir3SourceId, qtQuickModuleId, "Object", Storage::Version{}, sourceId3, "QQuickObject");
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
    package.updatedTypeSourceIds.push_back(sourceId3);
    package.updatedImportSourceIds.push_back(sourceId3);
    package.updatedExportedTypeSourceIds.push_back(qmldir3SourceId);

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
    package.imports[3].version = Storage::Version{2};
    package.types[0].prototype = Storage::Synchronization::QualifiedImportedType{"Object", "Quick"};
    package.exportedTypes[0].version = Storage::Version{2};
    package.types.emplace_back("QQuickObject",
                               Storage::Synchronization::ImportedType{},
                               Storage::Synchronization::ImportedType{},
                               TypeTraitsKind::Reference,
                               sourceId3);
    package.exportedTypes.emplace_back(
        qmldir3SourceId, qtQuickModuleId, "Object", Storage::Version{2}, sourceId3, "QQuickObject");
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
    package.updatedTypeSourceIds.push_back(sourceId3);
    package.updatedImportSourceIds.push_back(sourceId3);
    package.updatedExportedTypeSourceIds.push_back(qmldir3SourceId);

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
    package.imports[3].version = Storage::Version{2};
    package.types[0].extension = Storage::Synchronization::QualifiedImportedType{"Object", "Quick"};
    package.exportedTypes[0].version = Storage::Version{2};
    package.types.emplace_back("QQuickObject",
                               Storage::Synchronization::ImportedType{},
                               Storage::Synchronization::ImportedType{},
                               TypeTraitsKind::Reference,
                               sourceId3);
    package.exportedTypes.emplace_back(
        qmldir3SourceId, qtQuickModuleId, "Object", Storage::Version{2}, sourceId3, "QQuickObject");
    package.updatedTypeSourceIds.push_back(sourceId3);
    package.updatedImportSourceIds.push_back(sourceId3);
    package.updatedExportedTypeSourceIds.push_back(qmldir3SourceId);

    storage.synchronize(package);

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(sourceId1,
                                       "QQuickItem",
                                       TypeId{},
                                       fetchTypeId(sourceId3, "QQuickObject"),
                                       TypeTraitsKind::Reference)));
}

TEST_F(ProjectStorage, qualified_property_declaration_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", "Qml"};
    package.types.emplace_back("QQuickObject",
                               Storage::Synchronization::ImportedType{},
                               Storage::Synchronization::ImportedType{},
                               TypeTraitsKind::Reference,
                               sourceId3);
    package.exportedTypes.emplace_back(
        qmldir3SourceId, qtQuickModuleId, "Object", Storage::Version{}, sourceId3, "QQuickObject");
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
    package.updatedTypeSourceIds.push_back(sourceId3);
    package.updatedImportSourceIds.push_back(sourceId3);
    package.updatedExportedTypeSourceIds.push_back(qmldir3SourceId);

    storage.synchronize(package);

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId1, "QQuickItem"),
                Contains(IsInfoPropertyDeclaration("data",
                                                   fetchTypeId(sourceId2, "QObject"),
                                                   Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage,
       qualified_property_declaration_type_name_down_the_module_chain_nofifies_unresolved_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Obj", "Quick"};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Obj"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage,
       qualified_property_declaration_type_name_down_the_module_chain_has_null_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Obj", "Quick"};

    storage.synchronize(package);

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId1, "QQuickItem"),
                Contains(HasInfoPropertyTypeId(IsNullTypeId())));
}

TEST_F(ProjectStorage, borken_qualified_property_declaration_type_name_down_the_module_chain_fixed)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Obj", "Quick"};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Obj", "Qml"};

    storage.synchronize({.types = {package.types[0]}, .updatedTypeSourceIds = {sourceId1}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId1, "QQuickItem"),
                Not(Contains(HasInfoPropertyTypeId(IsNullTypeId()))));
}

TEST_F(ProjectStorage, qualified_property_declaration_type_name_in_the_module_chain)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", "Quick"};
    package.types.emplace_back("QQuickObject",
                               Storage::Synchronization::ImportedType{},
                               Storage::Synchronization::ImportedType{},
                               TypeTraitsKind::Reference,
                               sourceId3);
    package.exportedTypes.emplace_back(
        qmldir3SourceId, qtQuickModuleId, "Object", Storage::Version{}, sourceId3, "QQuickObject");
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
    package.updatedTypeSourceIds.push_back(sourceId3);
    package.updatedImportSourceIds.push_back(sourceId3);
    package.updatedExportedTypeSourceIds.push_back(qmldir3SourceId);

    storage.synchronize(package);

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId1, "QQuickItem"),
                Contains(IsInfoPropertyDeclaration("data",
                                                   fetchTypeId(sourceId3, "QQuickObject"),
                                                   Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage, qualified_property_declaration_type_name_with_version)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", "Qml2"};
    package.imports.emplace_back(qmlModuleId, Storage::Version{2}, sourceId1, sourceId1, "Qml2");

    storage.synchronize(package);

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId1, "QQuickItem"),
                Contains(IsInfoPropertyDeclaration("data",
                                                   fetchTypeId(sourceId2, "QObject"),
                                                   Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage, change_property_type_module_id_with_qualified_type_notifies_unresolved_type_name)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", "Qml"};
    storage.synchronize(package);
    Storage::Synchronization::ExportedType exportedType{
        qmldir2SourceId, qtQuickModuleId, "Object", Storage::Version{2}, sourceId2, "QObject"};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronize(
        {.exportedTypes = {exportedType}, .updatedExportedTypeSourceIds = {qmldir2SourceId}});
}

TEST_F(ProjectStorage, change_property_type_module_id_with_qualified_type_sets_type_id_to_null)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", "Qml"};
    storage.synchronize(package);
    Storage::Synchronization::ExportedType exportedType{
        qmldir2SourceId, qtQuickModuleId, "Object", Storage::Version{2}, sourceId2, "QObject"};

    storage.synchronize(
        {.exportedTypes = {exportedType}, .updatedExportedTypeSourceIds = {qmldir2SourceId}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId1, "QQuickItem"),
                Contains(HasInfoPropertyTypeId(IsNullTypeId())));
}

TEST_F(ProjectStorage,
       fixed_broken_change_property_type_module_id_with_qualified_type_sets_fixes_type_idl)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", "Qml"};
    storage.synchronize(package);
    Storage::Synchronization::ExportedType exportedType{
        qmldir2SourceId, qtQuickModuleId, "Object", Storage::Version{2}, sourceId2, "QObject"};
    storage.synchronize(
        {.exportedTypes = {exportedType}, .updatedExportedTypeSourceIds = {qmldir2SourceId}});
    exportedType.moduleId = qmlModuleId;

    storage.synchronize(
        {.exportedTypes = {exportedType}, .updatedExportedTypeSourceIds = {qmldir2SourceId}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId1, "QQuickItem"),
                Not(Contains(HasInfoPropertyTypeId(IsNullTypeId()))));
}

TEST_F(ProjectStorage, change_property_type_module_id_with_qualified_type)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", "Qml"};
    storage.synchronize(package);
    package.types[0].propertyDeclarations[0].typeName = Storage::Synchronization::QualifiedImportedType{
        "Object", "Quick"};
    package.exportedTypes[2].moduleId = qtQuickModuleId;
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId2, sourceId2);

    storage.synchronize(package);

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId1, "QQuickItem"),
                Contains(IsInfoPropertyDeclaration("data",
                                                   fetchTypeId(sourceId2, "QObject"),
                                                   Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage, add_file_statuses)
{
    FileStatus fileStatus1 = createFileStatus(sourceId1, 100, 100);
    FileStatus fileStatus2 = createFileStatus(sourceId2, 101, 101);

    storage.synchronize({.fileStatuses = {fileStatus1, fileStatus2},
                         .updatedFileStatusSourceIds = {sourceId1, sourceId2}});

    ASSERT_THAT(convert(storage.fetchAllFileStatuses()),
                UnorderedElementsAre(fileStatus1, fileStatus2));
}

TEST_F(ProjectStorage, remove_file_status)
{
    FileStatus fileStatus1 = createFileStatus(sourceId1, 100, 100);
    FileStatus fileStatus2 = createFileStatus(sourceId2, 101, 101);
    storage.synchronize({.fileStatuses = {fileStatus1, fileStatus2},
                         .updatedFileStatusSourceIds = {sourceId1, sourceId2}});

    storage.synchronize(
        {.fileStatuses = {fileStatus1}, .updatedFileStatusSourceIds = {sourceId1, sourceId2}});

    ASSERT_THAT(convert(storage.fetchAllFileStatuses()), UnorderedElementsAre(fileStatus1));
}

TEST_F(ProjectStorage, update_file_status)
{
    FileStatus fileStatus1 = createFileStatus(sourceId1, 100, 100);
    FileStatus fileStatus2 = createFileStatus(sourceId2, 101, 101);
    FileStatus fileStatus2b = createFileStatus(sourceId2, 102, 102);
    storage.synchronize({.fileStatuses = {fileStatus1, fileStatus2},
                         .updatedFileStatusSourceIds = {sourceId1, sourceId2}});

    storage.synchronize({.fileStatuses = {fileStatus1, fileStatus2b},
                         .updatedFileStatusSourceIds = {sourceId1, sourceId2}});

    ASSERT_THAT(convert(storage.fetchAllFileStatuses()),
                UnorderedElementsAre(fileStatus1, fileStatus2b));
}

TEST_F(ProjectStorage, throw_for_invalid_source_id_in_file_status)
{
    FileStatus fileStatus1 = createFileStatus(SourceId{}, 100, 100);

    ASSERT_THROW(storage.synchronize(
                     {.fileStatuses = {fileStatus1}, .updatedFileStatusSourceIds = {sourceId1}}),
                 QmlDesigner::FileStatusHasInvalidSourceId);
}

TEST_F(ProjectStorage, fetch_all_file_statuses)
{
    FileStatus fileStatus1 = createFileStatus(sourceId1, 100, 100);
    FileStatus fileStatus2 = createFileStatus(sourceId2, 101, 101);
    storage.synchronize({.fileStatuses = {fileStatus1, fileStatus2},
                         .updatedFileStatusSourceIds = {sourceId1, sourceId2}});

    auto fileStatuses = convert(storage.fetchAllFileStatuses());

    ASSERT_THAT(fileStatuses, ElementsAre(fileStatus1, fileStatus2));
}

TEST_F(ProjectStorage, fetch_all_file_statuses_reverse)
{
    FileStatus fileStatus1 = createFileStatus(sourceId1, 100, 100);
    FileStatus fileStatus2 = createFileStatus(sourceId2, 101, 101);
    storage.synchronize({.fileStatuses = {fileStatus2, fileStatus1},
                         .updatedFileStatusSourceIds = {sourceId1, sourceId2}});

    auto fileStatuses = convert(storage.fetchAllFileStatuses());

    ASSERT_THAT(fileStatuses, ElementsAre(fileStatus1, fileStatus2));
}

TEST_F(ProjectStorage, fetch_file_status)
{
    FileStatus fileStatus1 = createFileStatus(sourceId1, 100, 100);
    FileStatus fileStatus2 = createFileStatus(sourceId2, 101, 101);
    storage.synchronize({.fileStatuses = {fileStatus1, fileStatus2},
                         .updatedFileStatusSourceIds = {sourceId1, sourceId2}});

    auto fileStatus = storage.fetchFileStatus(sourceId1);

    ASSERT_THAT(fileStatus, Eq(fileStatus1));
}

TEST_F(ProjectStorage, synchronize_types_without_type_name)
{
    auto package{createSynchronizationPackageWithAliases()};
    storage.synchronize(package);
    package.types[3].typeName.clear();
    package.types[3].prototype = Storage::Synchronization::ImportedType{"Object"};

    storage.synchronize({.types = {package.types[3]}, .updatedTypeSourceIds = {sourceId4}});

    ASSERT_THAT(fetchExportedTypeNames(sourceId4, "QObject2"),
                UnorderedElementsAre(IsExportedTypeName(_, "Object2", _),
                                     IsExportedTypeName(_, "Obj2", _)));
}

TEST_F(ProjectStorage, fetch_by_major_version_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::ImportedType{"Object"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2};
    Storage::Import import{qmlModuleId, Storage::Version{1}, sourceId2, sourceId2};

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(storage.prototypeIds(fetchTypeId(sourceId2, "Item")),
                Contains(fetchTypeId(sourceId1, "QObject")));
}

TEST_F(ProjectStorage, fetch_by_major_version_for_qualified_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{1}, sourceId2, sourceId2, "Qml"};
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::QualifiedImportedType{"Object",
                                                                                        "Qml"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2};

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(storage.prototypeIds(fetchTypeId(sourceId2, "Item")),
                Contains(fetchTypeId(sourceId1, "QObject")));
}

TEST_F(ProjectStorage, fetch_by_major_version_and_minor_version_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::ImportedType{"Obj"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2};
    Storage::Import import{qmlModuleId, Storage::Version{1, 2}, sourceId2, sourceId2};

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(storage.prototypeIds(fetchTypeId(sourceId2, "Item")),
                Contains(fetchTypeId(sourceId1, "QObject")));
}

TEST_F(ProjectStorage, fetch_by_major_version_and_minor_version_for_qualified_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{1, 2}, sourceId2, sourceId2, "Qml"};
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::QualifiedImportedType{"Obj", "Qml"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2};

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(storage.prototypeIds(fetchTypeId(sourceId2, "Item")),
                Contains(fetchTypeId(sourceId1, "QObject")));
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
                                        sourceId2};
    Storage::Import import{qmlModuleId, Storage::Version{1, 1}, sourceId2, sourceId2};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId2));

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});
}

TEST_F(ProjectStorage,
       fetch_by_major_version_and_minor_version_for_qualified_imported_type_if_minor_version_is_not_exported_notifies_type_name_cannot_be_resolved)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{1, 1}, sourceId2, sourceId2, "Qml"};
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::QualifiedImportedType{"Object",
                                                                                        "Qml"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId2));

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});
}

TEST_F(ProjectStorage, fetch_low_minor_version_for_imported_type_notifies_type_name_cannot_be_resolved)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::ImportedType{"Obj"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2};
    Storage::Import import{qmlModuleId, Storage::Version{1, 1}, sourceId2, sourceId2};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Obj"), sourceId2));

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});
}

TEST_F(ProjectStorage,
       fetch_low_minor_version_for_qualified_imported_type_notifies_type_name_cannot_be_resolved)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{1, 1}, sourceId2, sourceId2, "Qml"};
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::QualifiedImportedType{"Obj", "Qml"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Obj"), sourceId2));

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});
}

TEST_F(ProjectStorage, fetch_higher_minor_version_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::ImportedType{"Obj"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2};
    Storage::Import import{qmlModuleId, Storage::Version{1, 3}, sourceId2, sourceId2};

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(storage.prototypeIds(fetchTypeId(sourceId2, "Item")),
                Contains(fetchTypeId(sourceId1, "QObject")));
}

TEST_F(ProjectStorage, fetch_higher_minor_version_for_qualified_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{1, 3}, sourceId2, sourceId2, "Qml"};
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::QualifiedImportedType{"Obj", "Qml"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2};

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(storage.prototypeIds(fetchTypeId(sourceId2, "Item")),
                Contains(fetchTypeId(sourceId1, "QObject")));
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
                                        sourceId2};
    Storage::Import import{qmlModuleId, Storage::Version{3, 1}, sourceId2, sourceId2};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Obj"), sourceId2));

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});
}

TEST_F(ProjectStorage,
       fetch_different_major_version_for_qualified_imported_type_notifies_type_name_cannot_be_resolved)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{3, 1}, sourceId2, sourceId2, "Qml"};
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::QualifiedImportedType{"Obj", "Qml"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Obj"), sourceId2));

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});
}

TEST_F(ProjectStorage, fetch_other_type_by_different_version_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::ImportedType{"Obj"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2};
    Storage::Import import{qmlModuleId, Storage::Version{2, 3}, sourceId2, sourceId2};

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(storage.prototypeIds(fetchTypeId(sourceId2, "Item")),
                Contains(fetchTypeId(sourceId1, "QObject2")));
}

TEST_F(ProjectStorage, fetch_other_type_by_different_version_for_qualified_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{2, 3}, sourceId2, sourceId2, "Qml"};
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::QualifiedImportedType{"Obj", "Qml"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2};

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(storage.prototypeIds(fetchTypeId(sourceId2, "Item")),
                Contains(fetchTypeId(sourceId1, "QObject2")));
}

TEST_F(ProjectStorage, fetch_highest_version_for_import_without_version_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::ImportedType{"Obj"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2};
    Storage::Import import{qmlModuleId, Storage::Version{}, sourceId2, sourceId2};

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(storage.prototypeIds(fetchTypeId(sourceId2, "Item")),
                Contains(fetchTypeId(sourceId1, "QObject4")));
}

TEST_F(ProjectStorage, fetch_highest_version_for_import_without_version_for_qualified_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{}, sourceId2, sourceId2, "Qml"};
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::QualifiedImportedType{"Obj", "Qml"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2};

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(storage.prototypeIds(fetchTypeId(sourceId2, "Item")),
                Contains(fetchTypeId(sourceId1, "QObject4")));
}

TEST_F(ProjectStorage, fetch_highest_version_for_import_with_major_version_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::ImportedType{"Obj"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2};
    Storage::Import import{qmlModuleId, Storage::Version{2}, sourceId2, sourceId2};

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(storage.prototypeIds(fetchTypeId(sourceId2, "Item")),
                Contains(fetchTypeId(sourceId1, "QObject3")));
}

TEST_F(ProjectStorage, fetch_highest_version_for_import_with_major_version_for_qualified_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{2}, sourceId2, sourceId2, "Qml"};
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::QualifiedImportedType{"Obj", "Qml"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2};

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(storage.prototypeIds(fetchTypeId(sourceId2, "Item")),
                Contains(fetchTypeId(sourceId1, "QObject3")));
}

TEST_F(ProjectStorage, fetch_exported_type_without_version_first_for_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Synchronization::Type type{"Item",
                                        Storage::Synchronization::ImportedType{"BuiltInObj"},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId2};
    Storage::Import import{qmlModuleId, Storage::Version{}, sourceId2, sourceId2};

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(storage.prototypeIds(fetchTypeId(sourceId2, "Item")),
                Contains(fetchTypeId(sourceId1, "QObject")));
}

TEST_F(ProjectStorage, fetch_exported_type_without_version_first_for_qualified_imported_type)
{
    auto package{createSynchronizationPackageWithVersions()};
    storage.synchronize(package);
    Storage::Import import{qmlModuleId, Storage::Version{}, sourceId2, sourceId2, "Qml"};
    Storage::Synchronization::Type type{
        "Item",
        Storage::Synchronization::QualifiedImportedType{"BuiltInObj", "Qml"},
        Storage::Synchronization::ImportedType{},
        TypeTraitsKind::Reference,
        sourceId2};

    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId2},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId2}});

    ASSERT_THAT(storage.prototypeIds(fetchTypeId(sourceId2, "Item")),
                Contains(fetchTypeId(sourceId1, "QObject")));
}

TEST_F(ProjectStorage, ensure_that_properties_for_removed_types_are_not_anymore_relinked)
{
    Storage::Synchronization::Type type{"QObject",
                                        Storage::Synchronization::ImportedType{""},
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraitsKind::Reference,
                                        sourceId1};
    Storage::Import import{qmlModuleId, Storage::Version{}, sourceId1, sourceId2};
    storage.synchronize({.imports = {import},
                         .updatedImportSourceIds = {sourceId1},
                         .types = {type},
                         .updatedTypeSourceIds = {sourceId1}});

    ASSERT_NO_THROW(storage.synchronize(
        {.updatedImportSourceIds = {sourceId1}, .updatedTypeSourceIds = {sourceId1}}));
}

TEST_F(ProjectStorage, ensure_that_prototypes_for_removed_types_are_not_anymore_relinked)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);

    ASSERT_NO_THROW(storage.synchronize({.updatedImportSourceIds = {sourceId1, sourceId2},
                                         .updatedTypeSourceIds = {sourceId1, sourceId2}}));
}

TEST_F(ProjectStorage, ensure_that_extensions_for_removed_types_are_not_anymore_relinked)
{
    auto package{createSimpleSynchronizationPackage()};
    std::swap(package.types.front().extension, package.types.front().prototype);
    storage.synchronize(package);

    ASSERT_NO_THROW(storage.synchronize({.updatedImportSourceIds = {sourceId1, sourceId2},
                                         .updatedTypeSourceIds = {sourceId1, sourceId2}}));
}

TEST_F(ProjectStorage, add_project_entry_infoes)
{
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo1{
        qmldir1DirectorySourceId, sourceId1, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo2{
        qmldir1DirectorySourceId, sourceId2, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo3{
        qmldir2DirectorySourceId,
        sourceId3,
        qtQuickModuleId,
        Storage::Synchronization::FileType::QmlTypes};

    storage.synchronize(SynchronizationPackage{
        .projectEntryInfos = {projectEntryInfo1, projectEntryInfo2, projectEntryInfo3},
        .updatedProjectEntryInfoSourceIds = {qmldir1DirectorySourceId, qmldir2DirectorySourceId}});

    ASSERT_THAT(fetchProjectEntryInfos({qmldir1DirectorySourceId, qmldir2DirectorySourceId}),
                UnorderedElementsAre(projectEntryInfo1, projectEntryInfo2, projectEntryInfo3));
}

TEST_F(ProjectStorage, remove_project_entry_info)
{
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo1{
        qmldir1DirectorySourceId, sourceId1, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo2{
        qmldir1DirectorySourceId, sourceId2, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo3{
        qmldir2DirectorySourceId,
        sourceId3,
        qtQuickModuleId,
        Storage::Synchronization::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{
        .projectEntryInfos = {projectEntryInfo1, projectEntryInfo2, projectEntryInfo3},
        .updatedProjectEntryInfoSourceIds = {qmldir1DirectorySourceId, qmldir2DirectorySourceId}});

    storage.synchronize(
        SynchronizationPackage{.projectEntryInfos = {projectEntryInfo1},
                               .updatedProjectEntryInfoSourceIds = {qmldir1DirectorySourceId,
                                                                    qmldir2DirectorySourceId}});

    ASSERT_THAT(fetchProjectEntryInfos({qmldir1DirectorySourceId, qmldir2DirectorySourceId}),
                UnorderedElementsAre(projectEntryInfo1));
}

TEST_F(ProjectStorage, update_project_entry_info_file_type)
{
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo1{
        qmldir1DirectorySourceId, sourceId1, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo2{
        qmldir1DirectorySourceId, sourceId2, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo2b{
        qmldir1DirectorySourceId, sourceId2, qmlModuleId, Storage::Synchronization::FileType::QmlTypes};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo3{
        qmldir2DirectorySourceId,
        sourceId3,
        qtQuickModuleId,
        Storage::Synchronization::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{
        .projectEntryInfos = {projectEntryInfo1, projectEntryInfo2, projectEntryInfo3},
        .updatedProjectEntryInfoSourceIds = {qmldir1DirectorySourceId, qmldir2DirectorySourceId}});

    storage.synchronize(
        SynchronizationPackage{.projectEntryInfos = {projectEntryInfo1, projectEntryInfo2b},
                               .updatedProjectEntryInfoSourceIds = {qmldir1DirectorySourceId}});

    ASSERT_THAT(fetchProjectEntryInfos({qmldir1DirectorySourceId, qmldir2DirectorySourceId}),
                UnorderedElementsAre(projectEntryInfo1, projectEntryInfo2b, projectEntryInfo3));
}

TEST_F(ProjectStorage, update_project_entry_info_module_id)
{
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo1{
        qmldir1DirectorySourceId, sourceId1, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo2{
        qmldir1DirectorySourceId, sourceId3, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo2b{
        qmldir1DirectorySourceId,
        sourceId3,
        qtQuickModuleId,
        Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo3{
        qmldir2DirectorySourceId,
        sourceId2,
        qtQuickModuleId,
        Storage::Synchronization::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{
        .projectEntryInfos = {projectEntryInfo1, projectEntryInfo2, projectEntryInfo3},
        .updatedProjectEntryInfoSourceIds = {qmldir1DirectorySourceId, qmldir2DirectorySourceId}});

    storage.synchronize({
        .projectEntryInfos = {projectEntryInfo1, projectEntryInfo2b},
        .updatedProjectEntryInfoSourceIds = {qmldir1DirectorySourceId},
    });

    ASSERT_THAT(fetchProjectEntryInfos({qmldir1DirectorySourceId, qmldir2DirectorySourceId}),
                UnorderedElementsAre(projectEntryInfo1, projectEntryInfo2b, projectEntryInfo3));
}

TEST_F(ProjectStorage, throw_for_invalid_source_id_in_project_entry_info)
{
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo1{
        qmldir1DirectorySourceId,
        SourceId{},
        qmlModuleId,
        Storage::Synchronization::FileType::QmlDocument};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{
                     .projectEntryInfos = {projectEntryInfo1},
                     .updatedProjectEntryInfoSourceIds = {qmldir1DirectorySourceId}}),
                 QmlDesigner::ProjectEntryInfoHasInvalidSourceId);
}

TEST_F(ProjectStorage, insert_project_entry_info_with_invalid_module_id)
{
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo1{
        qmldir1DirectorySourceId, sourceId1, ModuleId{}, Storage::Synchronization::FileType::QmlDocument};

    storage.synchronize(
        SynchronizationPackage{.projectEntryInfos = {projectEntryInfo1},
                               .updatedProjectEntryInfoSourceIds = {qmldir1DirectorySourceId}});

    ASSERT_THAT(fetchProjectEntryInfos({qmldir1DirectorySourceId, qmldir2DirectorySourceId}),
                UnorderedElementsAre(projectEntryInfo1));
}

TEST_F(ProjectStorage, update_project_entry_info_with_invalid_module_id)
{
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo1{
        qmldir1DirectorySourceId, sourceId1, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};
    storage.synchronize(
        SynchronizationPackage{.projectEntryInfos = {projectEntryInfo1},
                               .updatedProjectEntryInfoSourceIds = {qmldir1DirectorySourceId}});
    projectEntryInfo1.moduleId = ModuleId{};

    storage.synchronize(
        SynchronizationPackage{.projectEntryInfos = {projectEntryInfo1},
                               .updatedProjectEntryInfoSourceIds = {qmldir1DirectorySourceId}});

    ASSERT_THAT(fetchProjectEntryInfos({qmldir1DirectorySourceId, qmldir2DirectorySourceId}),
                UnorderedElementsAre(projectEntryInfo1));
}

TEST_F(ProjectStorage, throw_for_updating_with_invalid_project_directory_path_id_in_project_entry_info)
{
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo1{
        SourceId{}, sourceId1, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{
                     .projectEntryInfos = {projectEntryInfo1},
                     .updatedProjectEntryInfoSourceIds = {qmldir1DirectorySourceId}}),
                 QmlDesigner::ProjectEntryInfoHasInvalidProjectSourceId);
}

TEST_F(ProjectStorage, fetch_project_entry_infos_by_directory_directory_path_ids)
{
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo1{
        qmldir1DirectorySourceId, sourceId1, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo2{
        qmldir1DirectorySourceId, sourceId2, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo3{
        qmldir2DirectorySourceId,
        sourceId3,
        qtQuickModuleId,
        Storage::Synchronization::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{
        .projectEntryInfos = {projectEntryInfo1, projectEntryInfo2, projectEntryInfo3},
        .updatedProjectEntryInfoSourceIds = {qmldir1DirectorySourceId, qmldir2DirectorySourceId}});

    auto projectEntryInfos = storage.fetchProjectEntryInfos(
        {qmldir1DirectorySourceId, qmldir2DirectorySourceId});

    ASSERT_THAT(projectEntryInfos,
                UnorderedElementsAre(projectEntryInfo1, projectEntryInfo2, projectEntryInfo3));
}

TEST_F(ProjectStorage, fetch_project_entry_infos_by_directory_directory_path_id)
{
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo1{
        qmldir1DirectorySourceId, sourceId1, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo2{
        qmldir1DirectorySourceId, sourceId2, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo3{
        qmldir2DirectorySourceId,
        sourceId3,
        qtQuickModuleId,
        Storage::Synchronization::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{
        .projectEntryInfos = {projectEntryInfo1, projectEntryInfo2, projectEntryInfo3},
        .updatedProjectEntryInfoSourceIds = {qmldir1DirectorySourceId, qmldir2DirectorySourceId}});

    auto projectEntryInfo = storage.fetchProjectEntryInfos(qmldir1DirectorySourceId);

    ASSERT_THAT(projectEntryInfo, UnorderedElementsAre(projectEntryInfo1, projectEntryInfo2));
}

TEST_F(ProjectStorage, fetch_project_entry_infos_by_directory_directory_path_id_and_file_type)
{
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo1{
        qmldir1DirectorySourceId, sourceId1, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo2{
        qmldir1DirectorySourceId, sourceId2, ModuleId{}, Storage::Synchronization::FileType::Directory};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo3{
        qmldir2DirectorySourceId,
        sourceId3,
        qtQuickModuleId,
        Storage::Synchronization::FileType::QmlTypes};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo4{
        qmldir1DirectorySourceId, sourceId4, ModuleId{}, Storage::Synchronization::FileType::Directory};
    storage.synchronize(
        {.projectEntryInfos = {projectEntryInfo1, projectEntryInfo2, projectEntryInfo3, projectEntryInfo4},
         .updatedProjectEntryInfoSourceIds = {qmldir1DirectorySourceId, qmldir2DirectorySourceId}});

    auto projectEntryInfo = storage.fetchProjectEntryInfos(qmldir1DirectorySourceId,
                                                           Storage::Synchronization::FileType::Directory);

    ASSERT_THAT(projectEntryInfo, UnorderedElementsAre(projectEntryInfo2, projectEntryInfo4));
}

TEST_F(ProjectStorage, fetch_subdirectory_directory_path_ids)
{
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo1{
        qmldir1DirectorySourceId, sourceId1, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};
    auto directory1Id = SourceId::create(sourceId2.directoryPathId());
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo2{
        qmldir1DirectorySourceId, directory1Id, ModuleId{}, Storage::Synchronization::FileType::Directory};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo3{
        qmldir2DirectorySourceId,
        sourceId3,
        qtQuickModuleId,
        Storage::Synchronization::FileType::QmlTypes};
    auto directory2Id = SourceId::create(sourceId4.directoryPathId());
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo4{
        qmldir1DirectorySourceId, directory2Id, ModuleId{}, Storage::Synchronization::FileType::Directory};
    storage.synchronize(
        {.projectEntryInfos = {projectEntryInfo1, projectEntryInfo2, projectEntryInfo3, projectEntryInfo4},
         .updatedProjectEntryInfoSourceIds = {qmldir1DirectorySourceId, qmldir2DirectorySourceId}});

    auto projectEntryInfo = storage.fetchSubdirectoryIds(qmldir1DirectoryPathId);

    ASSERT_THAT(projectEntryInfo,
                UnorderedElementsAre(directory1Id.directoryPathId(), directory2Id.directoryPathId()));
}

TEST_F(ProjectStorage, fetch_project_entry_info_by_source_id)
{
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo1{
        qmldir1DirectorySourceId, sourceId1, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo2{
        qmldir1DirectorySourceId, sourceId2, qmlModuleId, Storage::Synchronization::FileType::QmlDocument};
    Storage::Synchronization::ProjectEntryInfo projectEntryInfo3{
        qmldir2DirectorySourceId,
        sourceId3,
        qtQuickModuleId,
        Storage::Synchronization::FileType::QmlTypes};
    storage.synchronize(SynchronizationPackage{
        .projectEntryInfos = {projectEntryInfo1, projectEntryInfo2, projectEntryInfo3},
        .updatedProjectEntryInfoSourceIds = {qmldir1DirectorySourceId, qmldir2DirectorySourceId}});

    auto projectEntryInfo = storage.fetchProjectEntryInfo(sourceId2);

    ASSERT_THAT(projectEntryInfo, Eq(projectEntryInfo2));
}

TEST_F(ProjectStorage, module_exported_import)
{
    auto package{createModuleExportedImportSynchronizationPackage()};

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchPrototype(sourceId1, "QQuickItem"), fetchTypeId(sourceId2, "QObject"));
    ASSERT_THAT(fetchPrototype(sourceId2, "QObject"), IsNullTypeId());
    ASSERT_THAT(fetchPrototype(sourceId3, "QQuickItem3d"), fetchTypeId(sourceId1, "QQuickItem"));
    ASSERT_THAT(fetchPrototype(sourceId4, "MyItem"), fetchTypeId(sourceId2, "QObject"));
}

TEST_F(ProjectStorage, module_exported_import_deletes_indirect_imports_too)
{
    auto package{createModuleExportedImportSynchronizationPackage()};
    storage.synchronize(package);
    storage.synchronize({.updatedImportSourceIds = package.updatedImportSourceIds});

    auto imports = storage.fetchDocumentImports();

    ASSERT_THAT(imports, IsEmpty());
}

TEST_F(ProjectStorage, module_exported_import_with_different_versions)
{
    auto package{createModuleExportedImportSynchronizationPackage()};
    package.imports.back().version.major.value = 2;
    package.exportedTypes[2].version.major.value = 2;
    package.moduleExportedImports.back().isAutoVersion = Storage::Synchronization::IsAutoVersion::No;
    package.moduleExportedImports.back().version = Storage::Version{1};

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchPrototype(sourceId1, "QQuickItem"), fetchTypeId(sourceId2, "QObject"));
    ASSERT_THAT(fetchPrototype(sourceId2, "QObject"), IsNullTypeId());
    ASSERT_THAT(fetchPrototype(sourceId3, "QQuickItem3d"), fetchTypeId(sourceId1, "QQuickItem"));
    ASSERT_THAT(fetchPrototype(sourceId4, "MyItem"), fetchTypeId(sourceId2, "QObject"));
}

TEST_F(ProjectStorage, module_exported_import_with_indirect_different_versions)
{
    auto package{createModuleExportedImportSynchronizationPackage()};
    package.imports[1].version.major.value = 2;
    package.imports.back().version.major.value = 2;
    package.exportedTypes[0].version.major.value = 2;
    package.exportedTypes[2].version.major.value = 2;
    package.moduleExportedImports[0].isAutoVersion = Storage::Synchronization::IsAutoVersion::No;
    package.moduleExportedImports[0].version = Storage::Version{1};

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchPrototype(sourceId1, "QQuickItem"), fetchTypeId(sourceId2, "QObject"));
    ASSERT_THAT(fetchPrototype(sourceId2, "QObject"), IsNullTypeId());
    ASSERT_THAT(fetchPrototype(sourceId3, "QQuickItem3d"), fetchTypeId(sourceId1, "QQuickItem"));
    ASSERT_THAT(fetchPrototype(sourceId4, "MyItem"), fetchTypeId(sourceId2, "QObject"));
}

TEST_F(ProjectStorage,
       module_exported_import_prevent_collision_if_module_is_indirectly_reexported_multiple_times)
{
    ModuleId qtQuick4DModuleId{modulesStorage.moduleId("QtQuick4D", ModuleKind::QmlLibrary)};
    auto package{createModuleExportedImportSynchronizationPackage()};
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{1}, sourceId5, sourceId5);
    package.moduleExportedImports.emplace_back(qtQuick4DModuleId,
                                               qtQuickModuleId,
                                               Storage::Version{},
                                               Storage::Synchronization::IsAutoVersion::Yes);
    package.moduleExportedImports.emplace_back(qtQuick4DModuleId,
                                               qmlModuleId,
                                               Storage::Version{},
                                               Storage::Synchronization::IsAutoVersion::Yes);
    package.updatedModuleIds.push_back(qtQuick4DModuleId);
    package.types.emplace_back("QQuickItem4d",
                               Storage::Synchronization::ImportedType{"Item"},
                               Storage::Synchronization::ImportedType{},
                               TypeTraitsKind::Reference,
                               sourceId5);
    package.imports.emplace_back(qtQuick4DModuleId, Storage::Version{1}, sourceId4, sourceId4);

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchPrototype(sourceId1, "QQuickItem"), fetchTypeId(sourceId2, "QObject"));
    ASSERT_THAT(fetchPrototype(sourceId2, "QObject"), IsNullTypeId());
    ASSERT_THAT(fetchPrototype(sourceId3, "QQuickItem3d"), fetchTypeId(sourceId1, "QQuickItem"));
    ASSERT_THAT(fetchPrototype(sourceId4, "MyItem"), fetchTypeId(sourceId2, "QObject"));
    ASSERT_THAT(fetchPrototype(sourceId5, "QQuickItem4d"), fetchTypeId(sourceId1, "QQuickItem"));
}

TEST_F(ProjectStorage, distinguish_between_import_kinds)
{
    ModuleId qml1ModuleId{modulesStorage.moduleId("Qml1", ModuleKind::QmlLibrary)};
    ModuleId qml11ModuleId{modulesStorage.moduleId("Qml11", ModuleKind::QmlLibrary)};
    auto package{createSimpleSynchronizationPackage()};
    package.moduleDependencies.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, qmldir1SourceId);
    package.moduleDependencies.emplace_back(qml1ModuleId, Storage::Version{1}, sourceId1, qmldir1SourceId);
    package.imports.emplace_back(qml1ModuleId, Storage::Version{}, sourceId1, qmldir1SourceId);
    package.moduleDependencies.emplace_back(qml11ModuleId,
                                            Storage::Version{1, 1},
                                            sourceId1,
                                            qmldir1SourceId);
    package.imports.emplace_back(qml11ModuleId, Storage::Version{1, 1}, sourceId1, qmldir1SourceId);

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchPrototype(sourceId1, "QQuickItem"), fetchTypeId(sourceId2, "QObject"));
    ASSERT_THAT(fetchPrototype(sourceId2, "QObject"), IsNullTypeId());
}

TEST_F(ProjectStorage, module_exported_import_distinguish_between_dependency_and_import_re_exports)
{
    auto package{createModuleExportedImportSynchronizationPackage()};
    package.moduleDependencies.emplace_back(qtQuick3DModuleId,
                                            Storage::Version{1},
                                            sourceId4,
                                            qmldir4SourceId);

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchPrototype(sourceId1, "QQuickItem"), fetchTypeId(sourceId2, "QObject"));
    ASSERT_THAT(fetchPrototype(sourceId2, "QObject"), IsNullTypeId());
    ASSERT_THAT(fetchPrototype(sourceId3, "QQuickItem3d"), fetchTypeId(sourceId1, "QQuickItem"));
    ASSERT_THAT(fetchPrototype(sourceId4, "MyItem"), fetchTypeId(sourceId2, "QObject"));
}

TEST_F(ProjectStorage, module_exported_import_with_qualified_imported_type)
{
    auto package{createModuleExportedImportSynchronizationPackage()};
    package.imports.emplace_back(qtQuick3DModuleId, Storage::Version{1}, sourceId4, sourceId4, "Quick3D");
    package.types.back().prototype = Storage::Synchronization::QualifiedImportedType{"Object",
                                                                                     "Quick3D"};

    storage.synchronize(std::move(package));

    ASSERT_THAT(fetchPrototype(sourceId1, "QQuickItem"), fetchTypeId(sourceId2, "QObject"));
    ASSERT_THAT(fetchPrototype(sourceId2, "QObject"), IsNullTypeId());
    ASSERT_THAT(fetchPrototype(sourceId3, "QQuickItem3d"), fetchTypeId(sourceId1, "QQuickItem"));
    ASSERT_THAT(fetchPrototype(sourceId4, "MyItem"), fetchTypeId(sourceId2, "QObject"));
}

TEST_F(ProjectStorage, synchronize_types_add_indirect_alias_declarations)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};

    storage.synchronize(package);

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
}

TEST_F(ProjectStorage, synchronize_types_add_indirect_alias_declarations_again)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);

    storage.synchronize(package);

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
}

TEST_F(ProjectStorage, synchronize_types_remove_indirect_alias_declaration)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations.pop_back();

    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage,
       synchronize_types_add_indirect_alias_declarations_notifies_unresolved_type_name_for_wrong_type_name)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "QQuickItemWrong"};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("QQuickItemWrong"), sourceId3));

    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});
}

TEST_F(ProjectStorage,
       synchronize_types_add_indirect_alias_declarations_for_wrong_type_name_sets_null_type_id)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "QQuickItemWrong"};

    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                Contains(IsInfoPropertyDeclaration("objects", IsNullTypeId(), _)));
}

TEST_F(ProjectStorage,
       fixed_synchronize_types_add_indirect_alias_declarations_for_wrong_type_name_sets_type_id)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "QQuickItemWrong"};
    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});
    package.types[2].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "Item"};

    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                Not(Contains(IsInfoPropertyDeclaration("objects", IsNullTypeId(), _))));
}

TEST_F(ProjectStorage,
       synchronize_types_add_indirect_alias_declarations_notifies_error_for_wrong_property_name)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].aliasPropertyName = "childrenWrong";

    EXPECT_CALL(errorNotifierMock, propertyNameDoesNotExists(Eq("childrenWrong.objects"), sourceId3));

    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});
}

TEST_F(ProjectStorage,
       synchronize_types_add_indirect_alias_declarations_returns_null_type_id_for_wrong_property_name)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].aliasPropertyName = "childrenWrong";

    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                Contains(IsInfoPropertyDeclaration("objects", IsNullTypeId(), _)));
}

TEST_F(ProjectStorage,
       synchronize_types_updated_indirect_alias_declarations_returns_item_type_id_for_wrong_property_name)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].aliasPropertyName = "childrenWrong";
    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});
    package.types[2].propertyDeclarations[1].aliasPropertyName = "children";

    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                Contains(IsInfoPropertyDeclaration("objects", fetchTypeId(sourceId2, "QObject"), _)));
}

TEST_F(ProjectStorage,
       synchronize_types_add_indirect_alias_declarations_notifies_error_for_wrong_property_name_tail)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].aliasPropertyNameTail = "objectsWrong";

    EXPECT_CALL(errorNotifierMock, propertyNameDoesNotExists(Eq("children.objectsWrong"), sourceId3));

    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});
}

TEST_F(ProjectStorage,
       synchronize_types_add_indirect_alias_declarations_sets_property_declaration_id_to_null_for_the_wrong_property_name_tail)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].aliasPropertyNameTail = "objectsWrong";

    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                Contains(IsInfoPropertyDeclaration("objects", IsNullTypeId(), _)));
}

TEST_F(ProjectStorage,
       synchronize_types_updates_indirect_alias_declarations_fixed_property_declaration_id_for_property_name_tail)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].aliasPropertyNameTail = "objectsWrong";
    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});
    package.types[2].propertyDeclarations[1].aliasPropertyNameTail = "objects";

    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                Contains(
                    IsInfoPropertyDeclaration("objects", Eq(fetchTypeId(sourceId2, "QObject")), _)));
}

TEST_F(ProjectStorage, synchronize_types_change_indirect_alias_declaration_type_name)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "Obj2"};
    importsSourceId3.emplace_back(pathToModuleId, Storage::Version{}, sourceId3, sourceId3);

    storage.synchronize({.imports = importsSourceId3,
                         .updatedImportSourceIds = {sourceId3},
                         .types = {package.types[2]},
                         .updatedTypeSourceIds = {sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId4, "QObject2"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
}

TEST_F(ProjectStorage, synchronize_types_change_indirect_alias_declaration_tails_type_name)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[4].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "Obj2"};
    importsSourceId5.emplace_back(pathToModuleId, Storage::Version{}, sourceId5, sourceId5);

    storage.synchronize({.imports = importsSourceId5,
                         .updatedImportSourceIds = {sourceId5},
                         .types = {package.types[4]},
                         .updatedTypeSourceIds = {sourceId5}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId4, "QObject2"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
}

TEST_F(ProjectStorage, synchronize_types_change_indirect_alias_declarations_property_name)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].aliasPropertyName = "kids";

    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId4, "QObject2"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
}

TEST_F(ProjectStorage, synchronize_types_change_indirect_alias_declarations_property_name_tail)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations[1].aliasPropertyNameTail = "items";

    storage.synchronize({.types = {package.types[2]}, .updatedTypeSourceIds = {sourceId3}});

    ASSERT_THAT(
        fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
        UnorderedElementsAre(IsInfoPropertyDeclaration("items",
                                                       fetchTypeId(sourceId1, "QQuickItem"),
                                                       Storage::PropertyDeclarationTraits::IsList),
                             IsInfoPropertyDeclaration("objects",
                                                       fetchTypeId(sourceId1, "QQuickItem"),
                                                       Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage, synchronize_types_change_indirect_alias_declarations_to_property_declaration)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    auto type = package.types[2];
    type.propertyDeclarations.pop_back();
    type.propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects",
        Storage::Synchronization::ImportedType{"QQuickItem"},
        Storage::PropertyDeclarationTraits::IsList | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize({.types = {type}, .updatedTypeSourceIds = {sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
}

TEST_F(ProjectStorage, synchronize_types_change_property_declarations_to_indirect_alias_declaration)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    auto &originalType = package.types[2];
    auto changedType = originalType;
    changedType.propertyDeclarations.pop_back();
    changedType.propertyDeclarations.push_back(Storage::Synchronization::PropertyDeclaration{
        "objects",
        Storage::Synchronization::ImportedType{"QQuickItem"},
        Storage::PropertyDeclarationTraits::IsList | Storage::PropertyDeclarationTraits::IsReadOnly});
    storage.synchronize({.types = {changedType}, .updatedTypeSourceIds = {sourceId3}});

    storage.synchronize({.types = {originalType}, .updatedTypeSourceIds = {sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
}

TEST_F(ProjectStorage, synchronize_types_change_indirect_alias_target_property_declaration_traits)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[4].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly;

    storage.synchronize({.types = {package.types[4]}, .updatedTypeSourceIds = {sourceId5}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId2, "QObject"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
}

TEST_F(ProjectStorage, synchronize_types_change_indirect_alias_target_property_declaration_type_name)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[4].propertyDeclarations[1].typeName = Storage::Synchronization::ImportedType{
        "Item"};

    storage.synchronize({.types = {package.types[4]}, .updatedTypeSourceIds = {sourceId5}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList),
                    IsInfoPropertyDeclaration("objects",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly)));
}

TEST_F(ProjectStorage, synchronize_types_remove_property_declaration_with_an_indirect_alias_throws)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[4].propertyDeclarations.pop_back();

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{.types = {package.types[4]},
                                                            .updatedTypeSourceIds = {sourceId5}}),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorage,
       DISABLED_synchronize_types_remove_stem_property_declaration_with_an_indirect_alias_throws)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations.erase(package.types[0].propertyDeclarations.begin());

    ASSERT_THROW(storage.synchronize(SynchronizationPackage{.types = {package.types[0]},
                                                            .updatedTypeSourceIds = {sourceId1}}),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorage, synchronize_types_remove_property_declaration_and_indirect_alias)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[2].propertyDeclarations.pop_back();
    package.types[4].propertyDeclarations.pop_back();

    storage.synchronize(SynchronizationPackage{.types = {package.types[2], package.types[4]},
                                               .updatedTypeSourceIds = {sourceId3, sourceId5}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"),
                UnorderedElementsAre(
                    IsInfoPropertyDeclaration("items",
                                              fetchTypeId(sourceId1, "QQuickItem"),
                                              Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(ProjectStorage, synchronize_types_remove_property_declaration_and_indirect_alias_steam)
{
    auto package{createSynchronizationPackageWithIndirectAliases()};
    storage.synchronize(package);
    package.types[0].propertyDeclarations.clear();
    package.types[2].propertyDeclarations.clear();

    storage.synchronize(SynchronizationPackage{.types = {package.types[0], package.types[2]},
                                               .updatedTypeSourceIds = {sourceId1, sourceId3}});

    ASSERT_THAT(fetchLocalPropertyDeclarations(sourceId3, "QAliasItem"), IsEmpty());
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

TEST_F(ProjectStorage, get_exported_name)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    storage.synchronize(package);
    auto objectimportedTypeNameId = storage.importedTypeNameId(sourceId1, "Object");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(qmlModuleId,
                                   "Object",
                                   IsVersion(3, 4),
                                   fetchTypeId(sourceId1, "QObject4")));
}

TEST_F(ProjectStorage, get_no_exported_name_for_missing_import)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    package.imports.clear();
    storage.synchronize(package);
    auto objectimportedTypeNameId = storage.importedTypeNameId(sourceId1, "Object");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(IsNullModuleId(), IsEmpty(), HasNoVersion(), IsNullTypeId()));
}

TEST_F(ProjectStorage, get_no_exported_name_for_invalid_imported_type_name_id)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    storage.synchronize(package);
    QmlDesigner::ImportedTypeNameId objectimportedTypeNameId;

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(IsNullModuleId(), IsEmpty(), HasNoVersion(), IsNullTypeId()));
}

TEST_F(ProjectStorage, get_no_exported_name_for_missing_export)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    storage.synchronize(package);
    auto objectimportedTypeNameId = storage.importedTypeNameId(sourceId1, "Foo");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(IsNullModuleId(), IsEmpty(), HasNoVersion(), IsNullTypeId()));
}

TEST_F(ProjectStorage, get_exported_name_with_version)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    package.imports.front().version = Storage::Version{3, 4};
    storage.synchronize(package);
    auto objectimportedTypeNameId = storage.importedTypeNameId(sourceId1, "Object");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(qmlModuleId,
                                   "Object",
                                   IsVersion(3, 4),
                                   fetchTypeId(sourceId1, "QObject4")));
}

TEST_F(ProjectStorage, get_exported_name_with_major_version_only)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    package.imports.front().version = Storage::Version{2};
    storage.synchronize(package);
    auto objectimportedTypeNameId = storage.importedTypeNameId(sourceId1, "Object");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(qmlModuleId,
                                   "Object",
                                   IsVersion(2, 11),
                                   fetchTypeId(sourceId1, "QObject3")));
}

TEST_F(ProjectStorage, get_same_exported_name_with_higher_minor_version)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    package.imports.front().version = Storage::Version{3, 5};
    storage.synchronize(package);
    auto objectimportedTypeNameId = storage.importedTypeNameId(sourceId1, "Object");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(qmlModuleId,
                                   "Object",
                                   IsVersion(3, 4),
                                   fetchTypeId(sourceId1, "QObject4")));
}

TEST_F(ProjectStorage, get_no_exported_name_with_lower_minor_version)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    package.imports.front().version = Storage::Version{3, 3};
    storage.synchronize(package);
    auto objectimportedTypeNameId = storage.importedTypeNameId(sourceId1, "Object");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(IsNullModuleId(), IsEmpty(), HasNoVersion(), IsNullTypeId()));
}

TEST_F(ProjectStorage, get_no_exported_name_with_higher_major_version)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    package.imports.front().version = Storage::Version{4, 0};
    storage.synchronize(package);
    auto objectimportedTypeNameId = storage.importedTypeNameId(sourceId1, "Object");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(IsNullModuleId(), IsEmpty(), HasNoVersion(), IsNullTypeId()));
}

TEST_F(ProjectStorage, get_different_exported_name_with_lower_major_version)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    package.imports.front().version = Storage::Version{2, 0};
    storage.synchronize(package);
    auto objectimportedTypeNameId = storage.importedTypeNameId(sourceId1, "Object");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(qmlModuleId,
                                   "Object",
                                   IsVersion(2, 0),
                                   fetchTypeId(sourceId1, "QObject2")));
}

TEST_F(ProjectStorage, get_different_exported_name_with_higher_minor_version)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    package.imports.front().version = Storage::Version{2, 12};
    storage.synchronize(package);
    auto objectimportedTypeNameId = storage.importedTypeNameId(sourceId1, "Object");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(qmlModuleId,
                                   "Object",
                                   IsVersion(2, 11),
                                   fetchTypeId(sourceId1, "QObject3")));
}

TEST_F(ProjectStorage, get_exported_name_for_exported_name_without_version)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    package.imports.clear();
    package.imports.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId1, qmldir1SourceId);
    storage.synchronize(package);
    auto objectimportedTypeNameId = storage.importedTypeNameId(sourceId1, "QObject");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(qmlNativeModuleId,
                                   "QObject",
                                   HasNoVersion(),
                                   fetchTypeId(sourceId1, "QObject")));
}

TEST_F(ProjectStorage, get_exported_name_with_qualified_import)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    storage.synchronize(package);
    auto importId = storage.importId({sourceId1, qmlModuleId, Storage::Version{}});
    auto objectimportedTypeNameId = storage.importedTypeNameId(importId, "Object");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(qmlModuleId,
                                   "Object",
                                   IsVersion(3, 4),
                                   fetchTypeId(sourceId1, "QObject4")));
}

TEST_F(ProjectStorage, get_no_exported_name_with_invalid_import_id)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    storage.synchronize(package);
    QmlDesigner::ImportId importId;
    auto objectimportedTypeNameId = storage.importedTypeNameId(importId, "Object");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(IsNullModuleId(), IsEmpty(), HasNoVersion(), IsNullTypeId()));
}

TEST_F(ProjectStorage, get_exported_name_with_qualified_import_version)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    package.imports.front().version = Storage::Version{2, 0};
    storage.synchronize(package);
    auto importId = storage.importId({sourceId1, qmlModuleId, Storage::Version{2, 0}});
    auto objectimportedTypeNameId = storage.importedTypeNameId(importId, "Object");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(qmlModuleId,
                                   "Object",
                                   IsVersion(2, 0),
                                   fetchTypeId(sourceId1, "QObject2")));
}

TEST_F(ProjectStorage, get_exported_name_with_qualified_import_major_version_only_version)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    package.imports.front().version = Storage::Version{2};
    storage.synchronize(package);
    auto importId = storage.importId({sourceId1, qmlModuleId, Storage::Version{2}});
    auto objectimportedTypeNameId = storage.importedTypeNameId(importId, "Object");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(qmlModuleId,
                                   "Object",
                                   IsVersion(2, 11),
                                   fetchTypeId(sourceId1, "QObject3")));
}

TEST_F(ProjectStorage, get_same_exported_name_with_higher_qualified_import_minor_version)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    package.imports.front().version = Storage::Version{2, 10};
    storage.synchronize(package);
    auto importId = storage.importId({sourceId1, qmlModuleId, Storage::Version{2, 10}});
    auto objectimportedTypeNameId = storage.importedTypeNameId(importId, "Object");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(qmlModuleId,
                                   "Object",
                                   IsVersion(2, 0),
                                   fetchTypeId(sourceId1, "QObject2")));
}

TEST_F(ProjectStorage, get_no_exported_name_with_lower_qualified_import_minor_version)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    package.imports.front().version = Storage::Version{3, 0};
    storage.synchronize(package);
    auto importId = storage.importId({sourceId1, qmlModuleId, Storage::Version{3, 0}});
    auto objectimportedTypeNameId = storage.importedTypeNameId(importId, "Object");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(IsNullModuleId(), IsEmpty(), HasNoVersion(), IsNullTypeId()));
}

TEST_F(ProjectStorage, get_different_exported_name_with_lower_qualified_import_major_version)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    package.imports.front().version = Storage::Version{1};
    storage.synchronize(package);
    auto importId = storage.importId({sourceId1, qmlModuleId, Storage::Version{1}});
    auto objectimportedTypeNameId = storage.importedTypeNameId(importId, "Object");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(qmlModuleId,
                                   "Object",
                                   IsVersion(1),
                                   fetchTypeId(sourceId1, "QObject")));
}

TEST_F(ProjectStorage, get_different_exported_name_with_higher_qualified_import_minor_version)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    package.imports.front().version = Storage::Version{2, 12};
    storage.synchronize(package);
    auto importId = storage.importId({sourceId1, qmlModuleId, Storage::Version{2, 12}});
    auto objectimportedTypeNameId = storage.importedTypeNameId(importId, "Object");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(qmlModuleId,
                                   "Object",
                                   IsVersion(2, 11),
                                   fetchTypeId(sourceId1, "QObject3")));
}

TEST_F(ProjectStorage,
       get_different_exported_name_with_qualified_import_for_exported_name_without_version)
{
    auto package{createSynchronizationPackageWithVersionsAndImport()};
    package.imports.clear();
    package.imports.emplace_back(qmlNativeModuleId, Storage::Version{}, sourceId1, qmldir1SourceId);
    storage.synchronize(package);
    auto importId = storage.importId({sourceId1, qmlNativeModuleId, Storage::Version{}});
    auto objectimportedTypeNameId = storage.importedTypeNameId(importId, "QObject");

    auto exportedTypeName = storage.exportedTypeName(objectimportedTypeNameId);

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(qmlNativeModuleId,
                                   "QObject",
                                   HasNoVersion(),
                                   fetchTypeId(sourceId1, "QObject")));
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
    auto package{createPackageWithProperties(ExchangePrototypeAndExtension::Yes)};
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
    auto package{createPackageWithProperties(ExchangePrototypeAndExtension::Yes)};
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
    auto package{createPackageWithProperties(ExchangePrototypeAndExtension::Yes)};
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
    auto package{createPackageWithProperties(ExchangePrototypeAndExtension::Yes)};
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
    renameExportedTypeTypeIdNames(package.exportedTypes, sourceId1, "QQuickItem", "QQuickItem2");
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
    renameExportedTypeTypeIdNames(package.exportedTypes, sourceId1, "bool", "bool2");
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
    renameExportedTypeTypeIdNames(package.exportedTypes, sourceId1, "var", "variant");
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

TEST_F(ProjectStorage, get_no_prototype_ids_with_extension)
{
    auto package{createPackageWithProperties(ExchangePrototypeAndExtension::Yes)};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");

    auto prototypeIds = storage.prototypeIds(typeId);

    ASSERT_THAT(prototypeIds, ElementsAre(fetchTypeId(sourceId1, "QObject2")));
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

TEST_F(ProjectStorage, get_no_prototype_and_self_ids_with_extension)
{
    auto package{createPackageWithProperties(ExchangePrototypeAndExtension::Yes)};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");

    auto prototypeAndSelfIds = storage.prototypeAndSelfIds(typeId);

    ASSERT_THAT(prototypeAndSelfIds,
                ElementsAre(fetchTypeId(sourceId1, "QObject3"), fetchTypeId(sourceId1, "QObject2")));
}

TEST_F(ProjectStorage, is_based_on_for_direct_prototype)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject2");
    auto baseTypeId = fetchTypeId(sourceId1, "QObject");
    auto baseTypeId2 = fetchTypeId(sourceId1, "QObject3");

    TypeId base = storage.basedOn(typeId, baseTypeId, baseTypeId2, TypeId{});

    ASSERT_THAT(base, baseTypeId);
}

TEST_F(ProjectStorage, is_based_on_for_indirect_prototype)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");
    auto baseTypeId = fetchTypeId(sourceId1, "QObject");

    TypeId base = storage.basedOn(typeId, baseTypeId);

    ASSERT_THAT(base, baseTypeId);
}

TEST_F(ProjectStorage, is_based_on_for_direct_extension)
{
    auto package{createPackageWithProperties(ExchangePrototypeAndExtension::Yes)};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject2");
    auto baseTypeId = fetchTypeId(sourceId1, "QObject");

    TypeId basedOn = storage.basedOn(typeId, baseTypeId);

    ASSERT_THAT(basedOn, baseTypeId);
}

TEST_F(ProjectStorage, is_based_on_for_indirect_extension)
{
    auto package{createPackageWithProperties(ExchangePrototypeAndExtension::Yes)};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject3");
    auto baseTypeId = fetchTypeId(sourceId1, "QObject");

    TypeId base = storage.basedOn(typeId, baseTypeId);

    ASSERT_THAT(base, baseTypeId);
}

TEST_F(ProjectStorage, is_based_on_for_self)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject2");
    auto baseTypeId = fetchTypeId(sourceId1, "QObject2");

    TypeId base = storage.basedOn(typeId, baseTypeId);

    ASSERT_THAT(base, baseTypeId);
}

TEST_F(ProjectStorage, is_not_based_on)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject");
    auto baseTypeId = fetchTypeId(sourceId1, "QObject2");
    auto baseTypeId2 = fetchTypeId(sourceId1, "QObject3");

    TypeId base = storage.basedOn(typeId, baseTypeId, baseTypeId2, TypeId{});

    ASSERT_THAT(base, IsNull());
}

TEST_F(ProjectStorage, is_not_based_on_if_no_base_type_is_given)
{
    auto package{createPackageWithProperties()};
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId1, "QObject");

    TypeId base = storage.basedOn(typeId);

    ASSERT_THAT(base, IsNull());
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

    auto importId = storage.importId(Storage::Import{sourceId1, qmlModuleId, Storage::Version{}});

    ASSERT_TRUE(importId);
}

TEST_F(ProjectStorage, get_invalid_import_id_if_not_exists)
{
    auto importId = storage.importId(Storage::Import{sourceId1, qmlModuleId, Storage::Version{}});

    ASSERT_FALSE(importId);
}

TEST_F(ProjectStorage, get_imported_type_name_id_for_import_id)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    auto importId = storage.importId(Storage::Import{sourceId1, qmlModuleId, Storage::Version{}});

    auto importedTypeNameId = storage.importedTypeNameId(importId, "Item");

    ASSERT_TRUE(importedTypeNameId);
}

TEST_F(ProjectStorage,
       get_imported_type_name_id_for_import_id_returns_different_id_for_different_importId)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    auto importId = storage.importId(Storage::Import{sourceId1, qmlModuleId, Storage::Version{}});
    auto expectedImportedTypeNameId = storage.importedTypeNameId(importId, "Item");
    auto importId2 = storage.importId(Storage::Import{sourceId1, qtQuickModuleId, Storage::Version{}});

    auto importedTypeNameId = storage.importedTypeNameId(importId2, "Item");

    ASSERT_THAT(importedTypeNameId, Not(expectedImportedTypeNameId));
}

TEST_F(ProjectStorage, get_imported_type_name_id_for_import_id_returns_different_id_for_different_name)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    auto importId = storage.importId(Storage::Import{sourceId1, qmlModuleId, Storage::Version{}});
    auto expectedImportedTypeNameId = storage.importedTypeNameId(importId, "Item");

    auto importedTypeNameId = storage.importedTypeNameId(importId, "Item2");

    ASSERT_THAT(importedTypeNameId, Not(expectedImportedTypeNameId));
}

TEST_F(ProjectStorage, synchronize_document_imports)
{
    Storage::Imports imports;
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1);
    imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1, sourceId1);

    storage.synchronizeDocumentImports(imports, sourceId1);

    ASSERT_TRUE(storage.importId(imports.back()));
}

TEST_F(ProjectStorage, synchronize_document_alias_imports)
{
    Storage::Imports imports;
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1);
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1, "Qml");

    storage.synchronizeDocumentImports(imports, sourceId1);

    ASSERT_TRUE(storage.importId(sourceId1, "Qml"));
}

TEST_F(ProjectStorage, synchronize_document_imports_removes_import)
{
    Storage::Imports imports;
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1);
    imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1, sourceId1);
    storage.synchronizeDocumentImports(imports, sourceId1);
    auto removedImport = imports.back();
    imports.pop_back();

    storage.synchronizeDocumentImports(imports, sourceId1);

    ASSERT_FALSE(storage.importId(removedImport));
}

TEST_F(ProjectStorage, synchronize_document_imports_removes_alias_import)
{
    Storage::Imports imports;
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1);
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1, "Qml");
    storage.synchronizeDocumentImports(imports, sourceId1);
    auto removedImport = imports.back();
    imports.pop_back();

    storage.synchronizeDocumentImports(imports, sourceId1);

    ASSERT_FALSE(storage.importId(sourceId1, "Qml"));
}

TEST_F(ProjectStorage, synchronize_document_imports_removes_non_alias_import)
{
    Storage::Imports imports;
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1, "Qml");
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1);
    storage.synchronizeDocumentImports(imports, sourceId1);
    auto removedImport = imports.back();
    imports.pop_back();

    storage.synchronizeDocumentImports(imports, sourceId1);

    ASSERT_FALSE(storage.importId(removedImport));
}

TEST_F(ProjectStorage, synchronize_document_imports_adds_import)
{
    Storage::Imports imports;
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1);
    storage.synchronizeDocumentImports(imports, sourceId1);
    imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId1, sourceId1);

    storage.synchronizeDocumentImports(imports, sourceId1);

    ASSERT_TRUE(storage.importId(imports.back()));
}

TEST_F(ProjectStorage, synchronize_document_imports_adds_alias_import)
{
    Storage::Imports imports;
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1);
    storage.synchronizeDocumentImports(imports, sourceId1);
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1, "Qml");

    storage.synchronizeDocumentImports(imports, sourceId1);

    ASSERT_TRUE(storage.importId(sourceId1, "Qml"));
}

TEST_F(ProjectStorage,
       synchronize_document_imports_removes_import_notifies_that_type_name_cannot_be_resolved)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1);
    package.types[0].prototype = Storage::Synchronization::ImportedType{"Object"};
    storage.synchronize(package);

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("Object"), sourceId1));

    storage.synchronizeDocumentImports({}, sourceId1);
}

TEST_F(ProjectStorage, synchronize_document_imports_removes_import_which_makes_prototype_unresolved)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1);
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
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId1, sourceId1);

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
                UnorderedElementsAre(
                    IsInfoExportTypeName(qmlModuleId, typeId, "Object", 2U, VersionNumber::noVersion),
                    IsInfoExportTypeName(qmlModuleId, typeId, "Obj", 2U, VersionNumber::noVersion),
                    IsInfoExportTypeName(qmlNativeModuleId,
                                         typeId,
                                         "QObject",
                                         VersionNumber::noVersion,
                                         VersionNumber::noVersion)));
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
    package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId3, sourceId1);
    storage.synchronize(package);
    auto typeId = fetchTypeId(sourceId2, "QObject");

    auto exportedTypeNames = storage.exportedTypeNames(typeId, sourceId3);

    ASSERT_THAT(exportedTypeNames,
                UnorderedElementsAre(
                    IsInfoExportTypeName(qmlModuleId, typeId, "Object", 2U, VersionNumber::noVersion),
                    IsInfoExportTypeName(qmlModuleId, typeId, "Obj", 2U, VersionNumber::noVersion)));
}

TEST_F(ProjectStorage, get_no_exported_type_names_for_source_id_for_invalid_type_id)
{
    auto package{createSimpleSynchronizationPackage()};
    package.imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId3, sourceId3);
    storage.synchronize(package);
    TypeId typeId;

    auto exportedTypeNames = storage.exportedTypeNames(typeId, sourceId3);

    ASSERT_THAT(exportedTypeNames, IsEmpty());
}

TEST_F(ProjectStorage, get_no_exported_type_names_for_source_id_for_non_matching_import)
{
    auto package{createSimpleSynchronizationPackage()};
    package.imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId3, sourceId3);
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
    package.propertyEditorQmlPaths.emplace_back(qtQuickModuleId,
                                                "Item3D",
                                                sourceId3,
                                                directoryPathIdPath6);

    storage.synchronize(package);

    ASSERT_THAT(storage.propertyEditorPathId(fetchTypeId(sourceId2, "QObject")), sourceId1);
    ASSERT_THAT(storage.propertyEditorPathId(fetchTypeId(sourceId1, "QQuickItem")), sourceId2);
    ASSERT_THAT(storage.propertyEditorPathId(fetchTypeId(sourceId3, "QQuickItem3d")), sourceId3);
}

TEST_F(ProjectStorage, synchronize_property_editor_with_non_existing_type_name)
{
    auto package{createPropertyEditorPathsSynchronizationPackage()};
    package.propertyEditorQmlPaths.emplace_back(qtQuickModuleId,
                                                "Item4D",
                                                sourceId4,
                                                directoryPathIdPath6);

    storage.synchronize(package);

    ASSERT_THAT(storage.propertyEditorPathId(fetchTypeId(sourceId4, "Item4D")), IsFalse());
}

TEST_F(ProjectStorage, call_remove_type_ids_in_observer_after_synchronization)
{
    auto package{createSimpleSynchronizationPackage()};
    NiceMock<ProjectStorageObserverMock> observerMock;
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

TEST_F(ProjectStorage, synchronize_type_annotation_type_traits_from_prototypes)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.typeAnnotations.pop_back();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    TypeTraits traits{TypeTraitsKind::Reference};
    traits.canBeContainer = FlagIs::True;
    traits.visibleInLibrary = FlagIs::True;
    auto type = package.types.front();
    package.types.erase(package.types.begin());
    storage.synchronize(package);
    package.typeAnnotations.clear();
    package.updatedTypeAnnotationSourceIds.clear();
    package.types.push_back(type);

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
                               ModuleKind::QmlLibrary,
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
                               ModuleKind::QmlLibrary,
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
                               ModuleKind::QmlLibrary,
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
                                       ModuleKind::QmlLibrary,
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

    ASSERT_THAT(storage.typeAnnotationSourceIds(directoryPathIdPath6),
                UnorderedElementsAre(sourceId4, sourceId5));
}

TEST_F(ProjectStorage, get_type_annotation_source_ids)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createTypeAnnotions();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    storage.synchronize(package);

    auto sourceIds = storage.typeAnnotationSourceIds(directoryPathIdPath6);

    ASSERT_THAT(sourceIds, UnorderedElementsAre(sourceId4, sourceId5));
}

TEST_F(ProjectStorage, get_type_annotation_directory_source_ids)
{
    auto package{createSimpleSynchronizationPackage()};
    package.typeAnnotations = createExtendedTypeAnnotations();
    package.updatedTypeAnnotationSourceIds = createUpdatedTypeAnnotionSourceIds(
        package.typeAnnotations);
    storage.synchronize(package);

    auto sourceIds = storage.typeAnnotationDirectoryIds();

    ASSERT_THAT(sourceIds, ElementsAre(directoryPathIdPath1, directoryPathIdPath6));
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
                               ModuleKind::QmlLibrary,
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
                               ModuleKind::QmlLibrary,
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
                               ModuleKind::QmlLibrary,
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
                                       ModuleKind::QmlLibrary,
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
                               ModuleKind::QmlLibrary,
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
                               ModuleKind::QmlLibrary,
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
                               ModuleKind::QmlLibrary,
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
                               ModuleKind::QmlLibrary,
                               "Bar is a Item",
                               "",
                               UnorderedElementsAre(IsItemLibraryProperty("color", "color", "#blue")),
                               IsEmpty())));
}

TEST_F(ProjectStorage, get_directory_imports_item_library_entries_by_source_id)
{
    auto package{createSimpleSynchronizationPackage()};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2);
    package.exportedTypes.emplace_back(
        qmldir2SourceId, pathToModuleId, "Object", Storage::Version{}, sourceId2, "QObject");
    storage.synchronize(package);

    auto entries = storage.directoryImportsItemLibraryEntries(sourceId2);

    ASSERT_THAT(entries,
                UnorderedElementsAre(IsItemLibraryEntry(fetchTypeId(sourceId2, "QObject"),
                                                        "Object",
                                                        "Object",
                                                        "",
                                                        "/path/to",
                                                        ModuleKind::PathLibrary,
                                                        sourceId2)));
}

TEST_F(ProjectStorage,
       dont_get_local_file_item_library_entries_by_source_id_if_type_name_is_not_capital)
{
    auto package{createSimpleSynchronizationPackage()};
    package.imports.emplace_back(pathToModuleId, Storage::Version{}, sourceId2, sourceId2);
    package.exportedTypes.emplace_back(
        qmldir2SourceId, pathToModuleId, "object", Storage::Version{}, sourceId2, "QObject");
    storage.synchronize(package);

    auto entries = storage.itemLibraryEntries(sourceId2);

    ASSERT_THAT(entries, IsEmpty());
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
    package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                            Storage::Version{},
                                            sourceId1,
                                            qmldir1SourceId);
    package.types[0].prototype = Storage::Synchronization::ImportedType{"QObject"};
    storage.synchronize(package);
    package.moduleDependencies.clear();
    package.types.clear();
    package.updatedTypeSourceIds.clear();
    package.updatedModuleDependencySourceIds = {qmldir1SourceId};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("QObject"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage,
       removed_document_import_notifies_for_extensions_that_type_name_cannot_be_resolved)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                            Storage::Version{},
                                            sourceId1,
                                            qmldir1SourceId);
    package.types[0].extension = Storage::Synchronization::ImportedType{"QObject"};
    storage.synchronize(package);
    package.moduleDependencies.clear();
    package.types.clear();
    package.exportedTypes.clear();
    package.updatedTypeSourceIds.clear();
    package.updatedModuleDependencySourceIds = {qmldir1SourceId};

    EXPECT_CALL(errorNotifierMock, typeNameCannotBeResolved(Eq("QObject"), sourceId1));

    storage.synchronize(package);
}

TEST_F(ProjectStorage, removed_document_import_changes_prototype_to_unresolved)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                            Storage::Version{},
                                            sourceId1,
                                            qmldir1SourceId);
    package.types[0].prototype = Storage::Synchronization::ImportedType{"QObject"};
    storage.synchronize(package);
    package.moduleDependencies.clear();
    package.types.clear();
    package.updatedTypeSourceIds.clear();
    package.updatedModuleDependencySourceIds = {qmldir1SourceId};

    storage.synchronize(package);

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(IsUnresolvedTypeId()));
}

TEST_F(ProjectStorage, removed_document_import_changes_extension_to_unresolved)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                            Storage::Version{},
                                            sourceId1,
                                            qmldir1SourceId);
    package.types[0].extension = Storage::Synchronization::ImportedType{"QObject"};
    storage.synchronize(package);
    package.moduleDependencies.clear();
    package.types.clear();
    package.updatedTypeSourceIds.clear();
    package.updatedModuleDependencySourceIds = {qmldir1SourceId};

    storage.synchronize(package);

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasExtensionId(IsUnresolvedTypeId()));
}

TEST_F(ProjectStorage, added_document_import_fixes_unresolved_prototype)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].prototype = Storage::Synchronization::ImportedType{"QObject"};
    storage.synchronize(package);
    package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                            Storage::Version{},
                                            sourceId1,
                                            qmldir1SourceId);
    package.types.clear();
    package.updatedTypeSourceIds.clear();
    package.updatedModuleDependencySourceIds = {qmldir1SourceId};

    storage.synchronize(package);

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasPrototypeId(fetchTypeId(sourceId2, "QObject")));
}

TEST_F(ProjectStorage, added_document_import_fixes_unresolved_extension)
{
    auto package{createVerySimpleSynchronizationPackage()};
    package.types[0].extension = Storage::Synchronization::ImportedType{"QObject"};
    storage.synchronize(package);
    package.moduleDependencies.emplace_back(qmlNativeModuleId,
                                            Storage::Version{},
                                            sourceId1,
                                            qmldir1SourceId);
    package.types.clear();
    package.exportedTypes.clear();
    package.updatedTypeSourceIds.clear();
    package.updatedExportedTypeSourceIds.clear();
    package.updatedModuleDependencySourceIds = {qmldir1SourceId};

    storage.synchronize(package);

    ASSERT_THAT(fetchType(sourceId1, "QQuickItem"), HasExtensionId(fetchTypeId(sourceId2, "QObject")));
}

TEST_F(ProjectStorage, added_export_is_notifying_changed_exported_type_names)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.exportedTypes.emplace_back(
        qmldir2SourceId, qmlNativeModuleId, "Objec", Storage::Version{}, sourceId2, "QObject");
    NiceMock<ProjectStorageObserverMock> observerMock;
    storage.addObserver(&observerMock);

    EXPECT_CALL(observerMock,
                exportedTypeNamesChanged(ElementsAre(
                                             IsInfoExportTypeName(qmlNativeModuleId,
                                                                  fetchTypeId(sourceId2, "QObject"),
                                                                  Eq("Objec"),
                                                                  A<QmlDesigner::Storage::Version>())),
                                         IsEmpty()));

    storage.synchronize(std::move(package));
}

TEST_F(ProjectStorage, removed_export_is_notifying_changed_exported_type_names)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    package.exportedTypes.pop_back();
    NiceMock<ProjectStorageObserverMock> observerMock;
    storage.addObserver(&observerMock);

    EXPECT_CALL(observerMock,
                exportedTypeNamesChanged(IsEmpty(),
                                         ElementsAre(
                                             IsInfoExportTypeName(qmlNativeModuleId,
                                                                  fetchTypeId(sourceId2, "QObject"),
                                                                  Eq("QObject"),
                                                                  A<QmlDesigner::Storage::Version>()))));

    storage.synchronize(std::move(package));
}

TEST_F(ProjectStorage, changed_export_is_notifying_changed_exported_type_names)
{
    auto package{createSimpleSynchronizationPackage()};
    storage.synchronize(package);
    renameExportedTypeNames(package.exportedTypes, "Obj", "Obj2");
    NiceMock<ProjectStorageObserverMock> observerMock;
    storage.addObserver(&observerMock);

    EXPECT_CALL(observerMock,
                exportedTypeNamesChanged(
                    ElementsAre(IsInfoExportTypeName(qmlModuleId,
                                                     fetchTypeId(sourceId2, "QObject"),
                                                     Eq("Obj2"),
                                                     A<QmlDesigner::Storage::Version>())),
                    ElementsAre(IsInfoExportTypeName(qmlModuleId,
                                                     fetchTypeId(sourceId2, "QObject"),
                                                     Eq("Obj"),
                                                     A<QmlDesigner::Storage::Version>()))));

    storage.synchronize(std::move(package));
}

TEST_F(ProjectStorage, get_unqiue_singleton_type_ids)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.back().traits.isSingleton = true;
    Storage::Imports imports;
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId5, sourceId5);
    imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId5, sourceId5);
    storage.synchronizeDocumentImports(imports, sourceId5);
    storage.synchronize(package);

    auto singletonTypeIds = storage.singletonTypeIds(sourceId5);

    ASSERT_THAT(singletonTypeIds, ElementsAre(fetchTypeId(sourceId2, "QObject")));
}

TEST_F(ProjectStorage, get_only_singleton_type_ids_for_document_imports)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.back().traits.isSingleton = true;
    package.types.front().traits.isSingleton = true;
    Storage::Imports imports;
    imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId5, sourceId5);
    storage.synchronizeDocumentImports(imports, sourceId5);
    storage.synchronize(package);

    auto singletonTypeIds = storage.singletonTypeIds(sourceId5);

    ASSERT_THAT(singletonTypeIds, ElementsAre(fetchTypeId(sourceId1, "QQuickItem")));
}

TEST_F(ProjectStorage, get_only_singleton_type_ids_exported_types)
{
    auto package{createSimpleSynchronizationPackage()};
    package.types.back().traits.isSingleton = true;
    std::erase_if(package.exportedTypes, [](const auto &e) { return e.typeIdName == "QObject"; });
    package.types.front().traits.isSingleton = true;
    Storage::Imports imports;
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId5, sourceId5);
    imports.emplace_back(qtQuickModuleId, Storage::Version{}, sourceId5, sourceId5);
    storage.synchronizeDocumentImports(imports, sourceId5);
    storage.synchronize(package);

    auto singletonTypeIds = storage.singletonTypeIds(sourceId5);

    ASSERT_THAT(singletonTypeIds, ElementsAre(fetchTypeId(sourceId1, "QQuickItem")));
}

} // namespace
