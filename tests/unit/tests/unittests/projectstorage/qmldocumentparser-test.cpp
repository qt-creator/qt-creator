// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <projectstorageerrornotifiermock.h>

#include <sqlitedatabase.h>

#include <projectstorage/projectstorage.h>
#include <projectstorage/qmldocumentparser.h>
#include <projectstorage/sourcepathcache.h>

namespace {

#undef signals

namespace Storage = QmlDesigner::Storage;
namespace Synchronization = Storage::Synchronization;
using QmlDesigner::ModuleId;
using QmlDesigner::SourceContextId;
using QmlDesigner::SourceId;
using QmlDesigner::Storage::ModuleKind;
using Storage::TypeTraits;

MATCHER_P(HasPrototype, prototype, std::string(negation ? "isn't " : "is ") + PrintToString(prototype))
{
    const Synchronization::Type &type = arg;

    return Synchronization::ImportedTypeName{prototype} == type.prototype;
}

MATCHER_P3(IsPropertyDeclaration,
           name,
           typeName,
           traits,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Synchronization::PropertyDeclaration{name, typeName, traits}))
{
    const Synchronization::PropertyDeclaration &propertyDeclaration = arg;

    return propertyDeclaration.name == name
           && Synchronization::ImportedTypeName{typeName} == propertyDeclaration.typeName
           && propertyDeclaration.traits == traits;
}

MATCHER_P4(IsAliasPropertyDeclaration,
           name,
           typeName,
           traits,
           aliasPropertyName,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(
                   Synchronization::PropertyDeclaration{name, typeName, traits, aliasPropertyName}))
{
    const Synchronization::PropertyDeclaration &propertyDeclaration = arg;

    return propertyDeclaration.name == name
           && Synchronization::ImportedTypeName{typeName} == propertyDeclaration.typeName
           && propertyDeclaration.traits == traits
           && propertyDeclaration.aliasPropertyName == aliasPropertyName
           && propertyDeclaration.aliasPropertyNameTail.empty();
}

MATCHER_P5(IsAliasPropertyDeclaration,
           name,
           typeName,
           traits,
           aliasPropertyName,
           aliasPropertyNameTail,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(
                   Synchronization::PropertyDeclaration{name, typeName, traits, aliasPropertyName}))
{
    const Synchronization::PropertyDeclaration &propertyDeclaration = arg;

    return propertyDeclaration.name == name
           && Synchronization::ImportedTypeName{typeName} == propertyDeclaration.typeName
           && propertyDeclaration.traits == traits
           && propertyDeclaration.aliasPropertyName == aliasPropertyName
           && propertyDeclaration.aliasPropertyNameTail == aliasPropertyNameTail;
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

class QmlDocumentParser : public ::testing::Test
{
public:
protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ProjectStorageErrorNotifierMock errorNotifierMock;
    QmlDesigner::ProjectStorage storage{database, errorNotifierMock, database.isInitialized()};
    QmlDesigner::SourcePathCache<QmlDesigner::ProjectStorage> sourcePathCache{
        storage};
    QmlDesigner::QmlDocumentParser parser{storage, sourcePathCache};
    Storage::Imports imports;
    SourceId qmlFileSourceId{sourcePathCache.sourceId("/path/to/qmlfile.qml")};
    SourceContextId qmlFileSourceContextId{qmlFileSourceId.contextId()};
    Utils::PathString directoryPath{sourcePathCache.sourceContextPath(qmlFileSourceContextId)};
    ModuleId directoryModuleId{storage.moduleId(directoryPath, ModuleKind::PathLibrary)};
};

TEST_F(QmlDocumentParser, prototype)
{
    QString component = "Example{}";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(type, HasPrototype(Synchronization::ImportedType("Example")));
}

TEST_F(QmlDocumentParser, qualified_prototype)
{
    auto exampleModuleId = storage.moduleId("Example", ModuleKind::QmlLibrary);
    QString component = R"(import Example 2.1 as Example
                      Example.Item{})";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(type,
                HasPrototype(Synchronization::QualifiedImportedType(
                    "Item",
                    Storage::Import{exampleModuleId, Storage::Version{2, 1}, qmlFileSourceId})));
}

TEST_F(QmlDocumentParser, properties)
{
    QString component = R"(Example{ property int foo })";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(IsPropertyDeclaration("foo",
                                                           Synchronization::ImportedType{"int"},
                                                           Storage::PropertyDeclarationTraits::None)));
}

TEST_F(QmlDocumentParser, qualified_properties)
{
    auto exampleModuleId = storage.moduleId("Example", ModuleKind::QmlLibrary);
    QString component = R"(import Example 2.1 as Example
                       Item{ property Example.Foo foo})";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(IsPropertyDeclaration(
                    "foo",
                    Synchronization::QualifiedImportedType("Foo",
                                                           Storage::Import{exampleModuleId,
                                                                           Storage::Version{2, 1},
                                                                           qmlFileSourceId}),
                    Storage::PropertyDeclarationTraits::None)));
}

TEST_F(QmlDocumentParser, enumeration_in_properties)
{
    QString component = R"(import Example 2.1 as Example
                           Item{ property Enumeration.Foo foo})";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(
                    IsPropertyDeclaration("foo",
                                          Synchronization::ImportedType("Enumeration.Foo"),
                                          Storage::PropertyDeclarationTraits::None)));
}

TEST_F(QmlDocumentParser, qualified_enumeration_in_properties)
{
    auto exampleModuleId = storage.moduleId("Example", ModuleKind::QmlLibrary);
    QString component = R"(import Example 2.1 as Example
                           Item{ property Example.Enumeration.Foo foo})";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(IsPropertyDeclaration(
                    "foo",
                    Synchronization::QualifiedImportedType("Enumeration.Foo",
                                                           Storage::Import{exampleModuleId,
                                                                           Storage::Version{2, 1},
                                                                           qmlFileSourceId}),
                    Storage::PropertyDeclarationTraits::None)));
}

TEST_F(QmlDocumentParser, imports)
{
    ModuleId fooDirectoryModuleId = storage.moduleId("/path/foo", ModuleKind::PathLibrary);
    ModuleId qmlModuleId = storage.moduleId("QML", ModuleKind::QmlLibrary);
    ModuleId qtQuickModuleId = storage.moduleId("QtQuick", ModuleKind::QmlLibrary);
    QString component = R"(import QtQuick
                           import "../foo"
                           Example{})";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(imports,
                UnorderedElementsAre(
                    Storage::Import{directoryModuleId, Storage::Version{}, qmlFileSourceId},
                    Storage::Import{fooDirectoryModuleId, Storage::Version{}, qmlFileSourceId},
                    Storage::Import{qmlModuleId, Storage::Version{}, qmlFileSourceId},
                    Storage::Import{qtQuickModuleId, Storage::Version{}, qmlFileSourceId}));
}

TEST_F(QmlDocumentParser, imports_with_version)
{
    ModuleId fooDirectoryModuleId = storage.moduleId("/path/foo", ModuleKind::PathLibrary);
    ModuleId qmlModuleId = storage.moduleId("QML", ModuleKind::QmlLibrary);
    ModuleId qtQuickModuleId = storage.moduleId("QtQuick", ModuleKind::QmlLibrary);
    QString component = R"(import QtQuick 2.1
                           import "../foo"
                           Example{})";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(imports,
                UnorderedElementsAre(
                    Storage::Import{directoryModuleId, Storage::Version{}, qmlFileSourceId},
                    Storage::Import{fooDirectoryModuleId, Storage::Version{}, qmlFileSourceId},
                    Storage::Import{qmlModuleId, Storage::Version{}, qmlFileSourceId},
                    Storage::Import{qtQuickModuleId, Storage::Version{2, 1}, qmlFileSourceId}));
}

TEST_F(QmlDocumentParser, imports_with_explict_directory)
{
    ModuleId qmlModuleId = storage.moduleId("QML", ModuleKind::QmlLibrary);
    ModuleId qtQuickModuleId = storage.moduleId("QtQuick", ModuleKind::QmlLibrary);
    QString component = R"(import QtQuick
                           import "../to"
                           import "."
                           Example{})";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(
        imports,
        UnorderedElementsAre(Storage::Import{directoryModuleId, Storage::Version{}, qmlFileSourceId},
                             Storage::Import{qmlModuleId, Storage::Version{}, qmlFileSourceId},
                             Storage::Import{qtQuickModuleId, Storage::Version{}, qmlFileSourceId}));
}

TEST_F(QmlDocumentParser, functions)
{
    QString component = "Example{\n function someScript(x, y) {}\n function otherFunction() {}\n}";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(type.functionDeclarations,
                UnorderedElementsAre(
                    AllOf(IsFunctionDeclaration("otherFunction", ""),
                          Field(&Synchronization::FunctionDeclaration::parameters, IsEmpty())),
                    AllOf(IsFunctionDeclaration("someScript", ""),
                          Field(&Synchronization::FunctionDeclaration::parameters,
                                ElementsAre(IsParameter("x", ""), IsParameter("y", ""))))));
}

TEST_F(QmlDocumentParser, signals)
{
    QString component = "Example{\n signal someSignal(int x, real y)\n signal signal2()\n}";

    auto type = parser.parse("Example{\n signal someSignal(int x, real y)\n signal signal2()\n}",
                             imports,
                             qmlFileSourceId,
                             directoryPath);

    ASSERT_THAT(type.signalDeclarations,
                UnorderedElementsAre(
                    AllOf(IsSignalDeclaration("someSignal"),
                          Field(&Synchronization::SignalDeclaration::parameters,
                                ElementsAre(IsParameter("x", "int"), IsParameter("y", "real")))),
                    AllOf(IsSignalDeclaration("signal2"),
                          Field(&Synchronization::SignalDeclaration::parameters, IsEmpty()))));
}

TEST_F(QmlDocumentParser, enumeration)
{
    QString component = R"(Example{
                           enum Color{red, green, blue=10, white}
                           enum State{On,Off}})";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(type.enumerationDeclarations,
                UnorderedElementsAre(
                    AllOf(IsEnumeration("Color"),
                          Field(&Synchronization::EnumerationDeclaration::enumeratorDeclarations,
                                ElementsAre(IsEnumerator("red", 0),
                                            IsEnumerator("green", 1),
                                            IsEnumerator("blue", 10),
                                            IsEnumerator("white", 11)))),
                    AllOf(IsEnumeration("State"),
                          Field(&Synchronization::EnumerationDeclaration::enumeratorDeclarations,
                                ElementsAre(IsEnumerator("On", 0), IsEnumerator("Off", 1))))));
}

TEST_F(QmlDocumentParser, DISABLED_duplicate_imports_are_removed)
{
    ModuleId fooDirectoryModuleId = storage.moduleId("/path/foo", ModuleKind::PathLibrary);
    ModuleId qmlModuleId = storage.moduleId("QML", ModuleKind::QmlLibrary);
    ModuleId qtQmlModuleId = storage.moduleId("QtQml", ModuleKind::QmlLibrary);
    ModuleId qtQuickModuleId = storage.moduleId("QtQuick", ModuleKind::QmlLibrary);
    QString component = R"(import QtQuick
                           import "../foo"
                           import QtQuick
                           import "../foo"
                           import "/path/foo"
                           import "."
 
                           Example{})";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(imports,
                UnorderedElementsAre(
                    Storage::Import{directoryModuleId, Storage::Version{}, qmlFileSourceId},
                    Storage::Import{fooDirectoryModuleId, Storage::Version{}, qmlFileSourceId},
                    Storage::Import{qmlModuleId, Storage::Version{1, 0}, qmlFileSourceId},
                    Storage::Import{qtQmlModuleId, Storage::Version{6, 0}, qmlFileSourceId},
                    Storage::Import{qtQuickModuleId, Storage::Version{}, qmlFileSourceId}));
}

TEST_F(QmlDocumentParser, alias_item_properties)
{
    QString component = R"(Example{
                             property alias delegate: foo
                               Item {
                                 id: foo
                               }
                           })";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(IsPropertyDeclaration("delegate",
                                                           Synchronization::ImportedType{"Item"},
                                                           Storage::PropertyDeclarationTraits::None)));
}

TEST_F(QmlDocumentParser, alias_properties)
{
    QString component = R"(Example{
                             property alias text: foo.text2
                             Item {
                               id: foo
                             }
                           })";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(
                    IsAliasPropertyDeclaration("text",
                                               Synchronization::ImportedType{"Item"},
                                               Storage::PropertyDeclarationTraits::None,
                                               "text2")));
}

TEST_F(QmlDocumentParser, indirect_alias_properties)
{
    QString component = R"(Example{
                             property alias textSize: foo.text.size
                             Item {
                               id: foo
                             }
                           })";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(
                    IsAliasPropertyDeclaration("textSize",
                                               Synchronization::ImportedType{"Item"},
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

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(type.propertyDeclarations, IsEmpty());
}

TEST_F(QmlDocumentParser, list_property)
{
    QString component = R"(Item{
                             property list<Foo> foos
                           })";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(
                    IsPropertyDeclaration("foos",
                                          Synchronization::ImportedType{"Foo"},
                                          Storage::PropertyDeclarationTraits::IsList)));
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

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(
                    IsPropertyDeclaration("foos",
                                          Synchronization::ImportedType{"Foo"},
                                          Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(QmlDocumentParser, qualified_list_property)
{
    auto exampleModuleId = storage.moduleId("Example", ModuleKind::QmlLibrary);
    QString component = R"(import Example 2.1 as Example
                           Item{
                             property list<Example.Foo> foos
                           })";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(IsPropertyDeclaration(
                    "foos",
                    Synchronization::QualifiedImportedType{"Foo",
                                                           Storage::Import{exampleModuleId,
                                                                           Storage::Version{2, 1},
                                                                           qmlFileSourceId}},
                    Storage::PropertyDeclarationTraits::IsList)));
}

TEST_F(QmlDocumentParser, default_property)
{
    QString component = R"(import Example 2.1 as Example
                           Item{
                             default property list<Example.Foo> foos
                           })";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(type.defaultPropertyName, Eq("foos"));
}

TEST_F(QmlDocumentParser, has_file_component_trait)
{
    QString component = R"(import Example
                           Item{
                           })";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_TRUE(type.traits.isFileComponent);
}

TEST_F(QmlDocumentParser, has_is_reference_trait)
{
    QString component = R"(import Example
                           Item{
                           })";

    auto type = parser.parse(component, imports, qmlFileSourceId, directoryPath);

    ASSERT_THAT(type.traits.kind, QmlDesigner::Storage::TypeTraitsKind::Reference);
}

} // namespace
