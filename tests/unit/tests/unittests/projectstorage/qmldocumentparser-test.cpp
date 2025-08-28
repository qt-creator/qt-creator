// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <projectstorageerrornotifiermock.h>

#include <sqlitedatabase.h>

#include <projectstorage/projectstorage.h>
#include <projectstorage/qmldocumentparser.h>
#include <sourcepathstorage/sourcepathcache.h>

namespace {

#undef signals

namespace Storage = QmlDesigner::Storage;
namespace Synchronization = Storage::Synchronization;
using QmlDesigner::DirectoryPathId;
using QmlDesigner::ModuleId;
using QmlDesigner::SourceId;
using QmlDesigner::Storage::ModuleKind;
using Storage::TypeTraits;
using Synchronization::PropertyKind;

auto HasPrototype(const auto &prototypeMatcher)
{
    return Field("Type::prototype", &Synchronization::Type::prototype, prototypeMatcher);
}

auto IsPropertyDeclaration(const auto &nameMatcher, const auto &typeNameMatcher, const auto &traitsMatcher)
{
    return AllOf(Field("PropertyDeclaration::name",
                       &Synchronization::PropertyDeclaration::name,
                       nameMatcher),
                 Field("PropertyDeclaration::typeName",
                       &Synchronization::PropertyDeclaration::typeName,
                       typeNameMatcher),
                 Field("PropertyDeclaration::traits",
                       &Synchronization::PropertyDeclaration::traits,
                       traitsMatcher));
}

auto IsPropertyDeclarationKind(PropertyKind kind)
{
    return Field("PropertyDeclaration::kind", &Synchronization::PropertyDeclaration::kind, kind);
}

auto IsAliasPropertyDeclaration(const auto &nameMatcher,
                                const auto &typeNameMatcher,
                                const auto &traitsMatcher,
                                const auto &aliasPropertyNameMatcher,
                                const auto &aliasPropertyNameTailMatcher)
{
    return AllOf(IsPropertyDeclaration(nameMatcher, typeNameMatcher, traitsMatcher),
                 Field("PropertyDeclaration::aliasPropertyName",
                       &Synchronization::PropertyDeclaration::aliasPropertyName,
                       aliasPropertyNameMatcher),
                 Field("PropertyDeclaration::aliasPropertyNameTail",
                       &Synchronization::PropertyDeclaration::aliasPropertyNameTail,
                       aliasPropertyNameTailMatcher));
}

MATCHER_P2(IsFunctionDeclaration,
           name,
           returnTypeName,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Synchronization::FunctionDeclaration{name, returnTypeName}))
{
    const Synchronization::FunctionDeclaration &declaration = arg;

    return declaration.name == name && declaration.returnTypeName == returnTypeName;
}

MATCHER_P(IsSignalDeclaration,
          name,
          std::string(negation ? "isn't " : "is ")
              + PrintToString(Synchronization::SignalDeclaration{name}))
{
    const Synchronization::SignalDeclaration &declaration = arg;

    return declaration.name == name;
}

MATCHER_P2(IsParameter,
           name,
           typeName,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Synchronization::ParameterDeclaration{name, typeName}))
{
    const Synchronization::ParameterDeclaration &declaration = arg;

    return declaration.name == name && declaration.typeName == typeName;
}

MATCHER_P(IsEnumeration,
          name,
          std::string(negation ? "isn't " : "is ")
              + PrintToString(Synchronization::EnumerationDeclaration{name, {}}))
{
    const Synchronization::EnumerationDeclaration &declaration = arg;

    return declaration.name == name;
}

MATCHER_P(IsEnumerator,
          name,
          std::string(negation ? "isn't " : "is ")
              + PrintToString(Synchronization::EnumeratorDeclaration{name}))
{
    const Synchronization::EnumeratorDeclaration &declaration = arg;

    return declaration.name == name && !declaration.hasValue;
}

MATCHER_P2(IsEnumerator,
           name,
           value,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Synchronization::EnumeratorDeclaration{name, value, true}))
{
    const Synchronization::EnumeratorDeclaration &declaration = arg;

    return declaration.name == name && declaration.value == value && declaration.hasValue;
}

auto IsImport(const auto &moduleIdMatcher,
              const auto &versionMatcher,
              const auto &sourceIdMatcher,
              const auto &contextSourceIdMatcher,
              const auto &aliasMatcher)
{
    using QmlDesigner::Storage::Import;
    return AllOf(Field("Import::sourceId", &Import::sourceId, sourceIdMatcher),
                 Field("Import::moduleId", &Import::moduleId, moduleIdMatcher),
                 Field("Import::version", &Import::version, versionMatcher),
                 Field("Import::contextSourceId", &Import::contextSourceId, contextSourceIdMatcher),
                 Field("Import::alias", &Import::alias, aliasMatcher));
}

auto IsQualifiedImportedType(const auto &nameMatcher, const auto &aliasMatcher)
{
    using Synchronization::QualifiedImportedType;
    return VariantWith<QualifiedImportedType>(
        AllOf(Field("QualifiedImportedType::name", &QualifiedImportedType::name, nameMatcher),
              Field("QualifiedImportedType::alias", &QualifiedImportedType::alias, aliasMatcher)));
};

auto IsImportedType(const auto &nameMatcher)
{
    using Synchronization::ImportedType;
    return VariantWith<ImportedType>(Field("ImportedType::name", &ImportedType::name, nameMatcher));
};

class QmlDocumentParser : public ::testing::Test
{
public:
    struct StaticData
    {
        Sqlite::Database modulesDatabase{":memory:", Sqlite::JournalMode::Memory};
        QmlDesigner::ModulesStorage modulesStorage{modulesDatabase, modulesDatabase.isInitialized()};
    };

    static void SetUpTestSuite() { staticData = std::make_unique<StaticData>(); }

    static void TearDownTestSuite() { staticData.reset(); }

protected:
    inline static std::unique_ptr<StaticData> staticData;
    QmlDesigner::ModulesStorage &modulesStorage = staticData->modulesStorage;
    Sqlite::Database sourcePathDatabase{":memory:", Sqlite::JournalMode::Memory};
    QmlDesigner::SourcePathStorage sourcePathStorage{sourcePathDatabase,
                                                     sourcePathDatabase.isInitialized()};
    QmlDesigner::SourcePathCache<QmlDesigner::SourcePathStorage> sourcePathCache{sourcePathStorage};
    QmlDesigner::QmlDocumentParser parser{modulesStorage, sourcePathCache};
    Storage::Imports imports;
    SourceId qmlFileSourceId{sourcePathCache.sourceId("/path/to/qmlfile.qml")};
    DirectoryPathId qmlFileDirectoryPathId{qmlFileSourceId.directoryPathId()};
    SourceId qmlFileDirectorySourceId = SourceId::create(qmlFileDirectoryPathId);
    Utils::PathString directoryPath{sourcePathCache.directoryPath(qmlFileDirectoryPathId)};
    ModuleId directoryModuleId{modulesStorage.moduleId(directoryPath, ModuleKind::PathLibrary)};
};

TEST_F(QmlDocumentParser, prototype)
{
    QString component = "Example{}";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(type, HasPrototype(IsImportedType("Example")));
}

TEST_F(QmlDocumentParser, qualified_prototype)
{
    QString component = R"(import Example 2.1 as Example
                           Example.Item{})";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(type, HasPrototype(IsQualifiedImportedType("Item", "Example")));
}

TEST_F(QmlDocumentParser, properties)
{
    QString component = R"(Example{ property int foo })";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(IsPropertyDeclaration("foo",
                                                           IsImportedType("int"),
                                                           Storage::PropertyDeclarationTraits::None)));
}

TEST_F(QmlDocumentParser, qualified_properties)
{
    QString component = R"(import Example 2.1 as Example
                           Item{ property Example.Foo foo})";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(IsPropertyDeclaration("foo",
                                                           IsQualifiedImportedType("Foo", "Example"),
                                                           Storage::PropertyDeclarationTraits::None)));
}

TEST_F(QmlDocumentParser, enumeration_in_properties)
{
    QString component = R"(import Example 2.1 as Example
                           Item{ property Enumeration.Foo foo})";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(IsPropertyDeclaration("foo",
                                                           IsImportedType("Enumeration.Foo"),
                                                           Storage::PropertyDeclarationTraits::None)));
}

TEST_F(QmlDocumentParser, qualified_enumeration_in_properties)
{
    QString component = R"(import Example 2.1 as Example
                           Item{ property Example.Enumeration.Foo foo})";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(
                    IsPropertyDeclaration("foo",
                                          IsQualifiedImportedType("Enumeration.Foo", "Example"),
                                          Storage::PropertyDeclarationTraits::None)));
}

TEST_F(QmlDocumentParser, imports)
{
    ModuleId fooDirectoryModuleId = modulesStorage.moduleId("/path/foo", ModuleKind::PathLibrary);
    ModuleId qmlModuleId = modulesStorage.moduleId("QML", ModuleKind::QmlLibrary);
    ModuleId qtQuickModuleId = modulesStorage.moduleId("QtQuick", ModuleKind::QmlLibrary);
    QString component = R"(import QtQuick
                           import "../foo"
                           Example{})";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(
        imports,
        UnorderedElementsAre(
            IsImport(directoryModuleId, Storage::Version{}, qmlFileSourceId, qmlFileSourceId, IsEmpty()),
            IsImport(fooDirectoryModuleId, Storage::Version{}, qmlFileSourceId, qmlFileSourceId, IsEmpty()),
            IsImport(qmlModuleId, Storage::Version{}, qmlFileSourceId, qmlFileSourceId, IsEmpty()),
            IsImport(qtQuickModuleId, Storage::Version{}, qmlFileSourceId, qmlFileSourceId, IsEmpty())));
}

TEST_F(QmlDocumentParser, imports_with_version)
{
    ModuleId fooDirectoryModuleId = modulesStorage.moduleId("/path/foo", ModuleKind::PathLibrary);
    ModuleId qmlModuleId = modulesStorage.moduleId("QML", ModuleKind::QmlLibrary);
    ModuleId qtQuickModuleId = modulesStorage.moduleId("QtQuick", ModuleKind::QmlLibrary);
    QString component = R"(import QtQuick 2.1
                           import "../foo"
                           Example{})";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(
        imports,
        UnorderedElementsAre(
            IsImport(directoryModuleId, Storage::Version{}, qmlFileSourceId, qmlFileSourceId, IsEmpty()),
            IsImport(fooDirectoryModuleId, Storage::Version{}, qmlFileSourceId, qmlFileSourceId, IsEmpty()),
            IsImport(qmlModuleId, Storage::Version{}, qmlFileSourceId, qmlFileSourceId, IsEmpty()),
            IsImport(qtQuickModuleId, Storage::Version{2, 1}, qmlFileSourceId, qmlFileSourceId, IsEmpty())));
}

TEST_F(QmlDocumentParser, imports_with_explict_directory)
{
    ModuleId qmlModuleId = modulesStorage.moduleId("QML", ModuleKind::QmlLibrary);
    ModuleId qtQuickModuleId = modulesStorage.moduleId("QtQuick", ModuleKind::QmlLibrary);
    QString component = R"(import QtQuick
                           import "../to"
                           import "."
                           Example{})";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(
        imports,
        UnorderedElementsAre(
            IsImport(directoryModuleId, Storage::Version{}, qmlFileSourceId, qmlFileSourceId, IsEmpty()),
            IsImport(qmlModuleId, Storage::Version{}, qmlFileSourceId, qmlFileSourceId, IsEmpty()),
            IsImport(qtQuickModuleId, Storage::Version{}, qmlFileSourceId, qmlFileSourceId, IsEmpty())));
}

TEST_F(QmlDocumentParser, imports_with_alias)
{
    ModuleId fooDirectoryModuleId = modulesStorage.moduleId("/path/foo", ModuleKind::PathLibrary);
    ModuleId qmlModuleId = modulesStorage.moduleId("QML", ModuleKind::QmlLibrary);
    ModuleId qtQuickModuleId = modulesStorage.moduleId("QtQuick", ModuleKind::QmlLibrary);
    QString component = R"(import QtQuick as Quick
                           import "../foo" as Foo
                           Example{})";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(
        imports,
        UnorderedElementsAre(
            IsImport(directoryModuleId, Storage::Version{}, qmlFileSourceId, qmlFileSourceId, IsEmpty()),
            IsImport(fooDirectoryModuleId, Storage::Version{}, qmlFileSourceId, qmlFileSourceId, Eq("Foo")),
            IsImport(qmlModuleId, Storage::Version{}, qmlFileSourceId, qmlFileSourceId, IsEmpty()),
            IsImport(qtQuickModuleId, Storage::Version{}, qmlFileSourceId, qmlFileSourceId, Eq("Quick"))));
}

TEST_F(QmlDocumentParser, functions)
{
    QString component = "Example{\n function someScript(x, y) {}\n function otherFunction() {}\n}";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(type.functionDeclarations,
                UnorderedElementsAre(AllOf(IsFunctionDeclaration("otherFunction", ""),
                                           Field("Synchronization::FunctionDeclaration::parameters",
                                                 &Synchronization::FunctionDeclaration::parameters,
                                                 IsEmpty())),
                                     AllOf(IsFunctionDeclaration("someScript", ""),
                                           Field("Synchronization::FunctionDeclaration::parameters",
                                                 &Synchronization::FunctionDeclaration::parameters,
                                                 ElementsAre(IsParameter("x", ""),
                                                             IsParameter("y", ""))))));
}

TEST_F(QmlDocumentParser, signals)
{
    QString component = "Example{\n signal someSignal(int x, real y)\n signal signal2()\n}";

    auto type = parser.parse("Example{\n signal someSignal(int x, real y)\n signal signal2()\n}",
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(type.signalDeclarations,
                UnorderedElementsAre(AllOf(IsSignalDeclaration("someSignal"),
                                           Field("Synchronization::SignalDeclaration::parameters",
                                                 &Synchronization::SignalDeclaration::parameters,
                                                 ElementsAre(IsParameter("x", "int"),
                                                             IsParameter("y", "real")))),
                                     AllOf(IsSignalDeclaration("signal2"),
                                           Field("Synchronization::SignalDeclaration::parameters",
                                                 &Synchronization::SignalDeclaration::parameters,
                                                 IsEmpty()))));
}

TEST_F(QmlDocumentParser, enumeration)
{
    QString component = R"(Example{
                           enum Color{red, green, blue=10, white}
                           enum State{On,Off}})";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(type.enumerationDeclarations,
                UnorderedElementsAre(
                    AllOf(IsEnumeration("Color"),
                          Field("Synchronization::EnumerationDeclaration::enumeratorDeclarations",
                                &Synchronization::EnumerationDeclaration::enumeratorDeclarations,
                                ElementsAre(IsEnumerator("red", 0),
                                            IsEnumerator("green", 1),
                                            IsEnumerator("blue", 10),
                                            IsEnumerator("white", 11)))),
                    AllOf(IsEnumeration("State"),
                          Field("Synchronization::EnumerationDeclaration::enumeratorDeclarations",
                                &Synchronization::EnumerationDeclaration::enumeratorDeclarations,
                                ElementsAre(IsEnumerator("On", 0), IsEnumerator("Off", 1))))));
}

TEST_F(QmlDocumentParser, DISABLED_duplicate_imports_are_removed)
{
    ModuleId fooDirectoryModuleId = modulesStorage.moduleId("/path/foo", ModuleKind::PathLibrary);
    ModuleId qmlModuleId = modulesStorage.moduleId("QML", ModuleKind::QmlLibrary);
    ModuleId qtQmlModuleId = modulesStorage.moduleId("QtQml", ModuleKind::QmlLibrary);
    ModuleId qtQuickModuleId = modulesStorage.moduleId("QtQuick", ModuleKind::QmlLibrary);
    QString component = R"(import QtQuick
                           import "../foo"
                           import QtQuick
                           import "../foo"
                           import "/path/foo"
                           import QtQuick as Quick
                           import "."
 
                           Example{})";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(
        imports,
        UnorderedElementsAre(
            IsImport(directoryModuleId, Storage::Version{}, qmlFileSourceId, qmlFileSourceId, _),
            IsImport(fooDirectoryModuleId, Storage::Version{}, qmlFileSourceId, qmlFileSourceId, _),
            IsImport(qmlModuleId, Storage::Version{1, 0}, qmlFileSourceId, qmlFileSourceId, _),
            IsImport(qtQmlModuleId, Storage::Version{6, 0}, qmlFileSourceId, qmlFileSourceId, _),
            IsImport(qtQuickModuleId, Storage::Version{}, qmlFileSourceId, qmlFileSourceId, IsEmpty()),
            IsImport(qtQuickModuleId, Storage::Version{}, qmlFileSourceId, qmlFileSourceId, Eq("QtQuick"))));
}

TEST_F(QmlDocumentParser, alias_item_properties)
{
    QString component = R"(Example{
                             property alias delegate: foo
                               Item {
                                 id: foo
                               }
                           })";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(
                    AllOf(IsPropertyDeclaration("delegate",
                                                IsImportedType("Item"),
                                                Storage::PropertyDeclarationTraits::None),
                          IsPropertyDeclarationKind(PropertyKind::Property))));
}

TEST_F(QmlDocumentParser, alias_properties)
{
    QString component = R"(Example{
                             property alias text: foo.text2
                             Item {
                               id: foo
                             }
                           })";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(IsAliasPropertyDeclaration("text",
                                                                IsImportedType("Item"),
                                                                Storage::PropertyDeclarationTraits::None,
                                                                "text2",
                                                                IsEmpty())));
}

TEST_F(QmlDocumentParser, indirect_alias_properties)
{
    QString component = R"(Example{
                             property alias textSize: foo.text.size
                             Item {
                               id: foo
                             }
                           })";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(IsAliasPropertyDeclaration("textSize",
                                                                IsImportedType("Item"),
                                                                Storage::PropertyDeclarationTraits::None,
                                                                "text",
                                                                "size")));
}

TEST_F(QmlDocumentParser, invalid_alias_properties_are_skipped)
{
    QString component = R"(Example{
                             property alias textSize: foo2.text.size
                             Item {
                               id: foo
                             }
                           })";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(type.propertyDeclarations, IsEmpty());
}

TEST_F(QmlDocumentParser, list_property)
{
    QString component = R"(Item{
                             property list<Foo> foos
                           })";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(IsPropertyDeclaration(
                    "foos", IsImportedType("Foo"), Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(QmlDocumentParser, alias_on_list_property)
{
    QString component = R"(Item{
                             property alias foos: foo.foos

                             Item {
                               id: foo
                               property list<Foo> foos
                             }
                            })";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(
                    AllOf(IsPropertyDeclaration("foos",
                                                Synchronization::ImportedType{"Foo"},
                                                Storage::PropertyDeclarationTraits::IsList),
                          IsPropertyDeclarationKind(PropertyKind::Property))));
}

TEST_F(QmlDocumentParser, qualified_list_property)
{
    QString component = R"(import Example 2.1 as Example
                           Item{
                             property list<Example.Foo> foos
                           })";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(
                    IsPropertyDeclaration("foos",
                                          IsQualifiedImportedType("Foo", "Example"),
                                          Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(QmlDocumentParser, default_property)
{
    QString component = R"(import Example 2.1 as Example
                           Item{
                             default property list<Example.Foo> foos
                           })";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(type.defaultPropertyName, Eq("foos"));
}

TEST_F(QmlDocumentParser, has_file_component_trait)
{
    QString component = R"(import Example
                           Item{
                           })";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_TRUE(type.traits.isFileComponent);
}

TEST_F(QmlDocumentParser, has_is_reference_trait)
{
    QString component = R"(import Example
                           Item{
                           })";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_THAT(type.traits.kind, QmlDesigner::Storage::TypeTraitsKind::Reference);
}

TEST_F(QmlDocumentParser, is_singleton)
{
    QString component = R"(pragma Singleton
                           Item{
                           })";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_TRUE(type.traits.isSingleton);
}

TEST_F(QmlDocumentParser, is_not_singleton)
{
    QString component = R"(Item{
                           })";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_FALSE(type.traits.isSingleton);
}

TEST_F(QmlDocumentParser, is_inside_project)
{
    QString component = R"(Item{
                           })";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::Yes);

    ASSERT_TRUE(type.traits.isInsideProject);
}

TEST_F(QmlDocumentParser, is_not_inside_project)
{
    QString component = R"(Item{
                           })";

    auto type = parser.parse(component,
                             imports,
                             qmlFileSourceId,
                             directoryPath,
                             Storage::IsInsideProject::No);

    ASSERT_FALSE(type.traits.isInsideProject);
}

} // namespace
