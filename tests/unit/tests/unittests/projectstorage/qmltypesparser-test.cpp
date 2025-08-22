// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <projectstorageerrornotifiermock.h>

#include <sqlitedatabase.h>

#include <projectstorage/projectstorage.h>
#include <projectstorage/projectstoragetypes.h>
#include <projectstorage/qmltypesparser.h>
#include <sourcepathstorage/sourcepathcache.h>

namespace {

namespace Storage = QmlDesigner::Storage;
namespace Synchronization = QmlDesigner::Storage::Synchronization;
using QmlDesigner::ModuleId;
using QmlDesigner::DirectoryPathId;
using QmlDesigner::SourceId;
using QmlDesigner::Storage::ModuleKind;

auto IsImport(const auto &moduleIdMatcher,
              const auto &versionMatcher,
              const auto &sourceIdMatcher,
              const auto &contextSourceIdMatcher)
{
    using QmlDesigner::Storage::Import;
    return AllOf(Field("Import::sourceId", &Import::sourceId, sourceIdMatcher),
                 Field("Import::moduleId", &Import::moduleId, moduleIdMatcher),
                 Field("Import::version", &Import::version, versionMatcher),
                 Field("Import::contextSourceId", &Import::contextSourceId, contextSourceIdMatcher));
}

MATCHER_P(HasPrototype, prototype, std::string(negation ? "isn't " : "is ") + PrintToString(prototype))
{
    const Synchronization::Type &type = arg;

    return Synchronization::ImportedTypeName{prototype} == type.prototype;
}

MATCHER_P5(IsType,
           typeName,
           prototype,
           extension,
           traits,
           sourceId,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Synchronization::Type{typeName, prototype, extension, traits, sourceId}))
{
    const Synchronization::Type &type = arg;

    return type.typeName == typeName && type.prototype == Synchronization::ImportedTypeName{prototype}
           && type.extension == Synchronization::ImportedTypeName{extension}
           && type.traits == traits && type.sourceId == sourceId;
}

MATCHER_P(HasFlag, flag, std::string(negation ? "hasn't " : "has ") + PrintToString(flag))
{
    return bool(arg & flag);
}

MATCHER_P(UsesCustomParser,
          value,
          std::string(negation ? "don't used custom parser " : "uses custom parser "))
{
    const Storage::TypeTraits &traits = arg;

    return traits.usesCustomParser == value;
}

MATCHER_P(IsSingleton, value, std::string(negation ? "isn't singleton " : "is singleton "))
{
    const Storage::TypeTraits &traits = arg;

    return traits.isSingleton == value;
}

MATCHER_P(IsInsideProject, value, std::string(negation ? "isn't inside project " : "is inside project "))
{
    const Storage::TypeTraits &traits = arg;

    return traits.isInsideProject == value;
}

template<typename Matcher>
auto IsTypeTrait(const Matcher &matcher)
{
    return Field("Synchronization::Type::traits", &Synchronization::Type::traits, matcher);
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
           && propertyDeclaration.traits == traits
           && propertyDeclaration.kind == Synchronization::PropertyKind::Property;
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

MATCHER_P6(IsExportedType,
           moduleId,
           name,
           version,
           contextSourceId,
           typeIdName,
           typeSourceId,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Synchronization::ExportedType{
                   contextSourceId, moduleId, name, version, contextSourceId, typeIdName}))
{
    const Synchronization::ExportedType &type = arg;

    return type.name == name && type.moduleId == moduleId && type.version == version
           && type.contextSourceId == contextSourceId && type.typeIdName == typeIdName
           && type.typeSourceId == typeSourceId;
}

class QmlTypesParser : public ::testing::Test
{
protected:
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
    QmlDesigner::QmlTypesParser parser{modulesStorage};
    Storage::Imports imports;
    Synchronization::Types types;
    Synchronization::ExportedTypes exportedTypes;
    SourceId qmltypesSourceId = sourcePathCache.sourceId("/path/to/types.qmltypes");
    ModuleId qtQmlNativeModuleId = modulesStorage.moduleId("QtQml", ModuleKind::CppLibrary);
    Synchronization::ProjectEntryInfo projectEntryInfo{SourceId::create(qmltypesSourceId.directoryPathId()),
                                                 qmltypesSourceId,
                                                 qtQmlNativeModuleId,
                                                 Synchronization::FileType::QmlTypes};
    DirectoryPathId qmltypesFileDirectoryPathId{qmltypesSourceId.directoryPathId()};
};

TEST_F(QmlTypesParser, imports)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        dependencies:
                          ["QtQuick 2.15", "QtQuick.Window 2.1", "QtFoo 6"]})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(imports,
                UnorderedElementsAre(
                    IsImport(modulesStorage.moduleId("QML", ModuleKind::CppLibrary),
                             QmlDesigner::Storage::Version{},
                             qmltypesSourceId,
                             qmltypesSourceId),
                    IsImport(modulesStorage.moduleId("QtQml", ModuleKind::CppLibrary),
                             QmlDesigner::Storage::Version{},
                             qmltypesSourceId,
                             qmltypesSourceId),
                    IsImport(modulesStorage.moduleId("QtQuick", ModuleKind::CppLibrary),
                             QmlDesigner::Storage::Version{},
                             qmltypesSourceId,
                             qmltypesSourceId),
                    IsImport(modulesStorage.moduleId("QtQuick.Window", ModuleKind::CppLibrary),
                             QmlDesigner::Storage::Version{},
                             qmltypesSourceId,
                             qmltypesSourceId),
                    IsImport(modulesStorage.moduleId("QtFoo", ModuleKind::CppLibrary),
                             QmlDesigner::Storage::Version{},
                             qmltypesSourceId,
                             qmltypesSourceId)));
}

TEST_F(QmlTypesParser, types)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"}
                        Component { name: "QQmlComponent"}})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types,
                UnorderedElementsAre(IsType("QObject",
                                            Synchronization::ImportedType{},
                                            Synchronization::ImportedType{},
                                            Storage::TypeTraitsKind::Reference,
                                            qmltypesSourceId),
                                     IsType("QQmlComponent",
                                            Synchronization::ImportedType{},
                                            Synchronization::ImportedType{},
                                            Storage::TypeTraitsKind::Reference,
                                            qmltypesSourceId)));
}

TEST_F(QmlTypesParser, prototype)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"}
                        Component { name: "QQmlComponent"
                                    prototype: "QObject"}})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types,
                UnorderedElementsAre(IsType("QObject",
                                            Synchronization::ImportedType{},
                                            Synchronization::ImportedType{},
                                            Storage::TypeTraitsKind::Reference,
                                            qmltypesSourceId),
                                     IsType("QQmlComponent",
                                            Synchronization::ImportedType{"QObject"},
                                            Synchronization::ImportedType{},
                                            Storage::TypeTraitsKind::Reference,
                                            qmltypesSourceId)));
}

TEST_F(QmlTypesParser, extension)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"}
                        Component { name: "QQmlComponent"
                                    extension: "QObject"}})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types,
                UnorderedElementsAre(IsType("QObject",
                                            Synchronization::ImportedType{},
                                            Synchronization::ImportedType{},
                                            Storage::TypeTraitsKind::Reference,
                                            qmltypesSourceId),
                                     IsType("QQmlComponent",
                                            Synchronization::ImportedType{},
                                            Synchronization::ImportedType{"QObject"},
                                            Storage::TypeTraitsKind::Reference,
                                            qmltypesSourceId)));
}

TEST_F(QmlTypesParser, ignore_java_script_extensions)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"}
                        Component { name: "QQmlComponent"
                                    extension: "QObject"
                                    extensionIsJavaScript: true
                                    }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types,
                Contains(IsType("QQmlComponent",
                                Synchronization::ImportedType{},
                                Synchronization::ImportedType{},
                                Storage::TypeTraitsKind::Reference,
                                qmltypesSourceId)));
}

TEST_F(QmlTypesParser, exported_types)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                          exports: ["QML/QtObject 1.0", "QtQml/QtObject 2.1"]
                      }})"};
    ModuleId qmlModuleId = modulesStorage.moduleId("QML", ModuleKind::QmlLibrary);
    ModuleId qtQmlModuleId = modulesStorage.moduleId("QtQml", ModuleKind::QmlLibrary);

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(exportedTypes,
                UnorderedElementsAre(IsExportedType(qmlModuleId,
                                                    "QtObject",
                                                    QmlDesigner::Storage::Version{1, 0},
                                                    qmltypesSourceId,
                                                    "QObject",
                                                    qmltypesSourceId),
                                     IsExportedType(qtQmlModuleId,
                                                    "QtObject",
                                                    QmlDesigner::Storage::Version{2, 1},
                                                    qmltypesSourceId,
                                                    "QObject",
                                                    qmltypesSourceId),
                                     IsExportedType(qtQmlNativeModuleId,
                                                    "QObject",
                                                    QmlDesigner::Storage::Version{},
                                                    qmltypesSourceId,
                                                    "QObject",
                                                    qmltypesSourceId)));
}

TEST_F(QmlTypesParser, exported_aliases_types)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                          aliases: ["QBaseObject"]
                      }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(exportedTypes,
                UnorderedElementsAre(IsExportedType(qtQmlNativeModuleId,
                                                    "QObject",
                                                    QmlDesigner::Storage::Version{},
                                                    qmltypesSourceId,
                                                    "QObject",
                                                    qmltypesSourceId),
                                     IsExportedType(qtQmlNativeModuleId,
                                                    "QBaseObject",
                                                    QmlDesigner::Storage::Version{},
                                                    qmltypesSourceId,
                                                    "QObject",
                                                    qmltypesSourceId)));
}

TEST_F(QmlTypesParser, properties)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                          Property { name: "objectName"; type: "string" }
                          Property { name: "target"; type: "QObject"; isPointer: true }
                          Property { name: "progress"; type: "double"; isReadonly: true }
                          Property { name: "targets"; type: "QQuickItem"; isList: true; isReadonly: true; isPointer: true }
                      }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types,
                ElementsAre(Field(
                    &Synchronization::Type::propertyDeclarations,
                    UnorderedElementsAre(
                        IsPropertyDeclaration("objectName",
                                              Synchronization::ImportedType{"string"},
                                              Storage::PropertyDeclarationTraits::None),
                        IsPropertyDeclaration("target",
                                              Synchronization::ImportedType{"QObject"},
                                              Storage::PropertyDeclarationTraits::IsPointer),
                        IsPropertyDeclaration("progress",
                                              Synchronization::ImportedType{"double"},
                                              Storage::PropertyDeclarationTraits::IsReadOnly),
                        IsPropertyDeclaration("targets",
                                              Synchronization::ImportedType{"QQuickItem"},
                                              Storage::PropertyDeclarationTraits::IsReadOnly
                                                  | Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsPointer)))));
}

TEST_F(QmlTypesParser, properties_with_qualified_types)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "Qt::Vector" }
                        Component { name: "Qt::List" }
                        Component { name: "QObject"
                          Property { name: "values"; type: "Vector" }
                          Property { name: "items"; type: "List" }
                          Property { name: "values2"; type: "Qt::Vector" }
                      }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types,
                Contains(
                    Field("Synchronization::Type::propertyDeclarations", &Synchronization::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("values",
                                                    Synchronization::ImportedType{"Qt::Vector"},
                                                    Storage::PropertyDeclarationTraits::None),
                              IsPropertyDeclaration("items",
                                                    Synchronization::ImportedType{"Qt::List"},
                                                    Storage::PropertyDeclarationTraits::None),
                              IsPropertyDeclaration("values2",
                                                    Synchronization::ImportedType{"Qt::Vector"},
                                                    Storage::PropertyDeclarationTraits::None)))));
}

TEST_F(QmlTypesParser, properties_without_type)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                          Property { name: "objectName"}
                          Property { name: "target"; type: "QObject"; isPointer: true }
                      }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types,
                ElementsAre(
                    Field("Synchronization::Type::propertyDeclarations", &Synchronization::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("target",
                                                    Synchronization::ImportedType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsPointer)))));
}

TEST_F(QmlTypesParser, functions)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                          Method { name: "movieUpdate" }
                          Method {
                            name: "advance"
                            Parameter { name: "frames"; type: "int" }
                            Parameter { name: "fps"; type: "double" }
                          }
                          Method {
                            name: "isImageLoading"
                            type: "bool"
                            Parameter { name: "url"; type: "QUrl" }
                          }
                          Method {
                            name: "getContext"
                            Parameter { name: "args"; type: "QQmlV4Function"; isPointer: true }
                          }
                      }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types,
                ElementsAre(
                    Field(&Synchronization::Type::functionDeclarations,
                          UnorderedElementsAre(
                              AllOf(IsFunctionDeclaration("advance", "void"),
                                    Field("Synchronization::FunctionDeclaration::parameters",
                                          &Synchronization::FunctionDeclaration::parameters,
                                          UnorderedElementsAre(IsParameter("frames", "int"),
                                                               IsParameter("fps", "double")))),
                              AllOf(IsFunctionDeclaration("isImageLoading", "bool"),
                                    Field("Synchronization::FunctionDeclaration::parameters",
                                          &Synchronization::FunctionDeclaration::parameters,
                                          UnorderedElementsAre(IsParameter("url", "QUrl")))),
                              AllOf(IsFunctionDeclaration("getContext", "void"),
                                    Field("Synchronization::FunctionDeclaration::parameters",
                                          &Synchronization::FunctionDeclaration::parameters,
                                          UnorderedElementsAre(IsParameter("args", "QQmlV4Function")))),
                              AllOf(IsFunctionDeclaration("movieUpdate", "void"),
                                    Field("Synchronization::FunctionDeclaration::parameters",
                                          &Synchronization::FunctionDeclaration::parameters,
                                          IsEmpty()))))));
}

TEST_F(QmlTypesParser, skip_java_script_functions)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                          Method {
                            name: "do"
                            isJavaScriptFunction: true
                          }
                      }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types, ElementsAre(Field("Synchronization::Type::functionDeclarations", &Synchronization::Type::functionDeclarations, IsEmpty())));
}

TEST_F(QmlTypesParser, functions_with_qualified_types)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "Qt::Vector" }
                        Component { name: "Qt::List" }
                        Component { name: "QObject"
                          Method {
                            name: "values"
                            Parameter { name: "values"; type: "Vector" }
                            Parameter { name: "items"; type: "List" }
                            Parameter { name: "values2"; type: "Qt::Vector" }
                          }
                      }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types,
                Contains(
                    Field("Synchronization::Type::functionDeclarations",
                          &Synchronization::Type::functionDeclarations,
                          UnorderedElementsAre(AllOf(
                              IsFunctionDeclaration("values", "void"),
                              Field("Synchronization::FunctionDeclaration::parameters",
                                    &Synchronization::FunctionDeclaration::parameters,
                                    UnorderedElementsAre(IsParameter("values", "Qt::Vector"),
                                                         IsParameter("items", "Qt::List"),
                                                         IsParameter("values2", "Qt::Vector"))))))));
}

#undef signals
TEST_F(QmlTypesParser, signals)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                          Method { name: "movieUpdate" }
                          Signal {
                            name: "advance"
                            Parameter { name: "frames"; type: "int" }
                            Parameter { name: "fps"; type: "double" }
                          }
                          Signal {
                            name: "isImageLoading"
                            Parameter { name: "url"; type: "QUrl" }
                          }
                          Signal {
                            name: "getContext"
                            Parameter { name: "args"; type: "QQmlV4Function"; isPointer: true }
                          }
                      }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types,
                ElementsAre(Field("Synchronization::Type::signalDeclarations", &Synchronization::Type::signalDeclarations,
                                  UnorderedElementsAre(
                                      AllOf(IsSignalDeclaration("advance"),
                                            Field("Synchronization::SignalDeclaration::parameters", &Synchronization::SignalDeclaration::parameters,
                                                  UnorderedElementsAre(IsParameter("frames", "int"),
                                                                       IsParameter("fps", "double")))),
                                      AllOf(IsSignalDeclaration("isImageLoading"),
                                            Field("Synchronization::SignalDeclaration::parameters", &Synchronization::SignalDeclaration::parameters,
                                                  UnorderedElementsAre(IsParameter("url", "QUrl")))),
                                      AllOf(IsSignalDeclaration("getContext"),
                                            Field("Synchronization::SignalDeclaration::parameters", &Synchronization::SignalDeclaration::parameters,
                                                  UnorderedElementsAre(
                                                      IsParameter("args", "QQmlV4Function"))))))));
}

TEST_F(QmlTypesParser, signals_with_qualified_types)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "Qt::Vector" }
                        Component { name: "Qt::List" }
                        Component { name: "QObject"
                          Signal {
                            name: "values"
                            Parameter { name: "values"; type: "Vector" }
                            Parameter { name: "items"; type: "List" }
                            Parameter { name: "values2"; type: "Qt::Vector" }
                          }
                      }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types,
                Contains(
                    Field("Synchronization::Type::signalDeclarations", &Synchronization::Type::signalDeclarations,
                          UnorderedElementsAre(AllOf(
                              IsSignalDeclaration("values"),
                              Field("Synchronization::SignalDeclaration::parameters", &Synchronization::SignalDeclaration::parameters,
                                    UnorderedElementsAre(IsParameter("values", "Qt::Vector"),
                                                         IsParameter("items", "Qt::List"),
                                                         IsParameter("values2", "Qt::Vector"))))))));
}

TEST_F(QmlTypesParser, enumerations)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                          Enum {
                              name: "NamedColorSpace"
                              values: [
                                  "Unknown",
                                  "SRgb",
                                  "AdobeRgb",
                                  "DisplayP3",
                              ]
                          }
                          Enum {
                              name: "VerticalLayoutDirection"
                              values: ["TopToBottom", "BottomToTop"]
                          }
                      }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types,
                Contains(Field(
                    &Synchronization::Type::enumerationDeclarations,
                    UnorderedElementsAre(
                        AllOf(IsEnumeration("NamedColorSpace"),
                              Field("Synchronization::EnumerationDeclaration::enumeratorDeclarations", &Synchronization::EnumerationDeclaration::enumeratorDeclarations,
                                    UnorderedElementsAre(IsEnumerator("Unknown"),
                                                         IsEnumerator("SRgb"),
                                                         IsEnumerator("AdobeRgb"),
                                                         IsEnumerator("DisplayP3")))),
                        AllOf(IsEnumeration("VerticalLayoutDirection"),
                              Field("Synchronization::EnumerationDeclaration::enumeratorDeclarations", &Synchronization::EnumerationDeclaration::enumeratorDeclarations,
                                    UnorderedElementsAre(IsEnumerator("TopToBottom"),
                                                         IsEnumerator("BottomToTop"))))))));
}

TEST_F(QmlTypesParser, enumeration_is_type_too)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                          Enum {
                              name: "NamedColorSpace"
                              values: [
                                  "Unknown",
                                  "SRgb",
                                  "AdobeRgb",
                                  "DisplayP3",
                              ]
                          }
                          Enum {
                              name: "VerticalLayoutDirection"
                              values: ["TopToBottom", "BottomToTop"]
                          }
                          exports: ["QML/QtObject 1.0", "QtQml/QtObject 2.1"]
                      }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);
    QmlDesigner::Storage::TypeTraits traits{QmlDesigner::Storage::TypeTraitsKind::Value};
    traits.isEnum = true;

    ASSERT_THAT(types,
                UnorderedElementsAre(IsType("QObject::NamedColorSpace",
                                            Synchronization::ImportedType{},
                                            Synchronization::ImportedType{},
                                            traits,
                                            qmltypesSourceId),
                                     IsType("QObject::VerticalLayoutDirection",
                                            Synchronization::ImportedType{},
                                            Synchronization::ImportedType{},
                                            traits,
                                            qmltypesSourceId),
                                     _));
}

TEST_F(QmlTypesParser, exported_types_for_enumerations)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                          Enum {
                              name: "NamedColorSpace"
                              values: [
                                  "Unknown",
                                  "SRgb",
                                  "AdobeRgb",
                                  "DisplayP3",
                              ]
                          }
                          Enum {
                              name: "VerticalLayoutDirection"
                              values: ["TopToBottom", "BottomToTop"]
                          }
                          exports: ["QML/QtObject 1.0", "QtQml/QtObject 2.1"]
                      }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);
    QmlDesigner::Storage::TypeTraits traits{QmlDesigner::Storage::TypeTraitsKind::Value};
    traits.isEnum = true;

    ASSERT_THAT(exportedTypes,
                IsSupersetOf({IsExportedType(qtQmlNativeModuleId,
                                             "QObject::NamedColorSpace",
                                             QmlDesigner::Storage::Version{},
                                             qmltypesSourceId,
                                             "QObject::NamedColorSpace",
                                             qmltypesSourceId),
                              IsExportedType(qtQmlNativeModuleId,
                                             "QObject::VerticalLayoutDirection",
                                             QmlDesigner::Storage::Version{},
                                             qmltypesSourceId,
                                             "QObject::VerticalLayoutDirection",
                                             qmltypesSourceId)}));
}

TEST_F(QmlTypesParser, exported_types_for_enumerations_with_alias)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                          Enum {
                              name: "NamedColorSpaces"
                              alias: "NamedColorSpace"
                              values: [
                                  "Unknown",
                                  "SRgb",
                                  "AdobeRgb",
                                  "DisplayP3",
                              ]
                          }
                          exports: ["QML/QtObject 1.0", "QtQml/QtObject 2.1"]
                      }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);
    QmlDesigner::Storage::TypeTraits traits{QmlDesigner::Storage::TypeTraitsKind::Value};
    traits.isEnum = true;

    ASSERT_THAT(exportedTypes,
                IsSupersetOf({IsExportedType(qtQmlNativeModuleId,
                                             "QObject::NamedColorSpace",
                                             QmlDesigner::Storage::Version{},
                                             qmltypesSourceId,
                                             "QObject::NamedColorSpaces",
                                             qmltypesSourceId),
                              IsExportedType(qtQmlNativeModuleId,
                                             "QObject::NamedColorSpaces",
                                             QmlDesigner::Storage::Version{},
                                             qmltypesSourceId,
                                             "QObject::NamedColorSpaces",
                                             qmltypesSourceId)}));
}

TEST_F(QmlTypesParser, exported_types_for_enumerations_with_alias_too)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                          Enum {
                              name: "NamedColorSpaces"
                              alias: "NamedColorSpace"
                              values: [
                                  "Unknown",
                                  "SRgb",
                                  "AdobeRgb",
                                  "DisplayP3",
                              ]
                          }
                          Enum {
                              name: "NamedColorSpace"
                              values: [
                                  "Unknown",
                                  "SRgb",
                                  "AdobeRgb",
                                  "DisplayP3",
                              ]
                          }
                          exports: ["QML/QtObject 1.0", "QtQml/QtObject 2.1"]
                      }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);
    QmlDesigner::Storage::TypeTraits traits{QmlDesigner::Storage::TypeTraitsKind::Value};
    traits.isEnum = true;

    ASSERT_THAT(exportedTypes,
                IsSupersetOf({IsExportedType(qtQmlNativeModuleId,
                                             "QObject::NamedColorSpace",
                                             QmlDesigner::Storage::Version{},
                                             qmltypesSourceId,
                                             "QObject::NamedColorSpaces",
                                             qmltypesSourceId),
                              IsExportedType(qtQmlNativeModuleId,
                                             "QObject::NamedColorSpaces",
                                             QmlDesigner::Storage::Version{},
                                             qmltypesSourceId,
                                             "QObject::NamedColorSpaces",
                                             qmltypesSourceId)}));
}

TEST_F(QmlTypesParser, enumeration_is_referenced_by_qualified_name)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                          Property { name: "colorSpace"; type: "NamedColorSpace" }
                          Enum {
                              name: "NamedColorSpace"
                              values: [
                                  "Unknown",
                                  "SRgb",
                                  "AdobeRgb",
                                  "DisplayP3",
                              ]
                          }
                      }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types,
                Contains(Field("Synchronization::Type::propertyDeclarations", &Synchronization::Type::propertyDeclarations,
                               ElementsAre(IsPropertyDeclaration(
                                   "colorSpace",
                                   Synchronization::ImportedType{"QObject::NamedColorSpace"},
                                   QmlDesigner::Storage::PropertyDeclarationTraits::None)))));
}

TEST_F(QmlTypesParser, alias_enumeration_is_referenced_by_qualified_name)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                          Property { name: "colorSpace"; type: "NamedColorSpaces" }
                          Enum {
                              name: "NamedColorSpace"
                              alias: "NamedColorSpaces"
                              values: [
                                  "Unknown",
                                  "SRgb",
                                  "AdobeRgb",
                                  "DisplayP3",
                              ]
                          }
                      }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types,
                Contains(Field("Synchronization::Type::propertyDeclarations", &Synchronization::Type::propertyDeclarations,
                               ElementsAre(IsPropertyDeclaration(
                                   "colorSpace",
                                   Synchronization::ImportedType{"QObject::NamedColorSpaces"},
                                   QmlDesigner::Storage::PropertyDeclarationTraits::None)))));
}

TEST_F(QmlTypesParser, access_type_is_reference)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                                    accessSemantics: "reference"}})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types, ElementsAre(IsTypeTrait(Storage::TypeTraitsKind::Reference)));
}

TEST_F(QmlTypesParser, access_type_is_value)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                                    accessSemantics: "value"}})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types, Contains(IsTypeTrait(Storage::TypeTraitsKind::Value)));
}

TEST_F(QmlTypesParser, access_type_is_sequence)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                                    accessSemantics: "sequence"}})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types, ElementsAre(IsTypeTrait(Storage::TypeTraitsKind::Sequence)));
}

TEST_F(QmlTypesParser, access_type_is_none)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                                    accessSemantics: "none"}})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types, ElementsAre(IsTypeTrait(Storage::TypeTraitsKind::None)));
}

TEST_F(QmlTypesParser, uses_custom_parser)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                                    hasCustomParser: true }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types, ElementsAre(IsTypeTrait(UsesCustomParser(true))));
}

TEST_F(QmlTypesParser, uses_no_custom_parser)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                                    hasCustomParser: false }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types, ElementsAre(IsTypeTrait(UsesCustomParser(false))));
}

TEST_F(QmlTypesParser, default_property)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                                    defaultProperty: "children" }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types,
                ElementsAre(Field("Synchronization::Type::defaultPropertyName", &Synchronization::Type::defaultPropertyName, Eq("children"))));
}

TEST_F(QmlTypesParser, is_singleton)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                                    isSingleton: true}})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types, ElementsAre(IsTypeTrait(IsSingleton(true))));
}

TEST_F(QmlTypesParser, is_not_singleton)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                                    isSingleton: false}})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types, ElementsAre(IsTypeTrait(IsSingleton(false))));
}

TEST_F(QmlTypesParser, is_by_default_not_singleton)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"}})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types, ElementsAre(IsTypeTrait(IsSingleton(false))));
}

TEST_F(QmlTypesParser, is_inside_project)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"}})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::Yes);

    ASSERT_THAT(types, ElementsAre(IsTypeTrait(IsInsideProject(true))));
}

TEST_F(QmlTypesParser, is_not_inside_project)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"}})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types, ElementsAre(IsTypeTrait(IsInsideProject(false))));
}

TEST_F(QmlTypesParser, value_type_adds_list)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QUrl"
                                    accessSemantics: "value"}})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types,
                Contains(IsType("QList<QUrl>",
                                Synchronization::ImportedType{},
                                Synchronization::ImportedType{},
                                Storage::TypeTraitsKind::Sequence,
                                qmltypesSourceId)));
}

TEST_F(QmlTypesParser, value_list_is_inside_project)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QUrl"
                                    accessSemantics: "value" }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::Yes);

    ASSERT_THAT(types, Each(IsTypeTrait(IsInsideProject(true))));
}

TEST_F(QmlTypesParser, value_list_is_not_inside_project)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QUrl"
                                    accessSemantics: "value" }})"};

    parser.parse(source, imports, types, exportedTypes, projectEntryInfo, Storage::IsInsideProject::No);

    ASSERT_THAT(types, Each(IsTypeTrait(IsInsideProject(false))));
}

} // namespace
