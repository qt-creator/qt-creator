// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <sqlitedatabase.h>

#include <projectstorage/projectstorage.h>
#include <projectstorage/projectstoragetypes.h>
#include <projectstorage/qmltypesparser.h>
#include <projectstorage/sourcepathcache.h>

namespace {

namespace Storage = QmlDesigner::Storage;
namespace Synchronization = QmlDesigner::Storage::Synchronization;
using QmlDesigner::ModuleId;
using QmlDesigner::SourceContextId;
using QmlDesigner::SourceId;

MATCHER_P3(IsImport,
           moduleId,
           version,
           sourceId,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Import{moduleId, version, sourceId}))
{
    const Storage::Import &import = arg;

    return import.moduleId == moduleId && import.version == version && import.sourceId == sourceId;
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

template<typename Matcher>
auto IsTypeTrait(const Matcher &matcher)
{
    return Field(&Synchronization::Type::traits, matcher);
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

MATCHER_P3(IsExportedType,
           moduleId,
           name,
           version,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Synchronization::ExportedType{moduleId, name, version}))
{
    const Synchronization::ExportedType &type = arg;

    return type.name == name && type.moduleId == moduleId && type.version == version;
}

class QmlTypesParser : public ::testing::Test
{
public:
protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    QmlDesigner::ProjectStorage<Sqlite::Database> storage{database, database.isInitialized()};
    QmlDesigner::SourcePathCache<QmlDesigner::ProjectStorage<Sqlite::Database>> sourcePathCache{
        storage};
    QmlDesigner::QmlTypesParser parser{storage};
    Storage::Imports imports;
    Synchronization::Types types;
    SourceId qmltypesFileSourceId{sourcePathCache.sourceId("path/to/types.qmltypes")};
    ModuleId qtQmlNativeModuleId = storage.moduleId("QtQml-cppnative");
    Synchronization::ProjectData projectData{qmltypesFileSourceId,
                                             qmltypesFileSourceId,
                                             qtQmlNativeModuleId,
                                             Synchronization::FileType::QmlTypes};
    SourceContextId qmltypesFileSourceContextId{sourcePathCache.sourceContextId(qmltypesFileSourceId)};
    ModuleId directoryModuleId{storage.moduleId("path/to/")};
};

TEST_F(QmlTypesParser, imports)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        dependencies:
                          ["QtQuick 2.15", "QtQuick.Window 2.1", "QtFoo 6"]})"};

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(imports,
                UnorderedElementsAre(IsImport(storage.moduleId("QML-cppnative"),
                                              QmlDesigner::Storage::Version{},
                                              qmltypesFileSourceId),
                                     IsImport(storage.moduleId("QtQml-cppnative"),
                                              QmlDesigner::Storage::Version{},
                                              qmltypesFileSourceId),
                                     IsImport(storage.moduleId("QtQuick-cppnative"),
                                              QmlDesigner::Storage::Version{},
                                              qmltypesFileSourceId),
                                     IsImport(storage.moduleId("QtQuick.Window-cppnative"),
                                              QmlDesigner::Storage::Version{},
                                              qmltypesFileSourceId),
                                     IsImport(storage.moduleId("QtFoo-cppnative"),
                                              QmlDesigner::Storage::Version{},
                                              qmltypesFileSourceId)));
}

TEST_F(QmlTypesParser, types)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"}
                        Component { name: "QQmlComponent"}})"};

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types,
                UnorderedElementsAre(IsType("QObject",
                                            Synchronization::ImportedType{},
                                            Synchronization::ImportedType{},
                                            Storage::TypeTraits::Reference,
                                            qmltypesFileSourceId),
                                     IsType("QQmlComponent",
                                            Synchronization::ImportedType{},
                                            Synchronization::ImportedType{},
                                            Storage::TypeTraits::Reference,
                                            qmltypesFileSourceId)));
}

TEST_F(QmlTypesParser, prototype)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"}
                        Component { name: "QQmlComponent"
                                    prototype: "QObject"}})"};

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types,
                UnorderedElementsAre(IsType("QObject",
                                            Synchronization::ImportedType{},
                                            Synchronization::ImportedType{},
                                            Storage::TypeTraits::Reference,
                                            qmltypesFileSourceId),
                                     IsType("QQmlComponent",
                                            Synchronization::ImportedType{"QObject"},
                                            Synchronization::ImportedType{},
                                            Storage::TypeTraits::Reference,
                                            qmltypesFileSourceId)));
}

TEST_F(QmlTypesParser, extension)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"}
                        Component { name: "QQmlComponent"
                                    extension: "QObject"}})"};

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types,
                UnorderedElementsAre(IsType("QObject",
                                            Synchronization::ImportedType{},
                                            Synchronization::ImportedType{},
                                            Storage::TypeTraits::Reference,
                                            qmltypesFileSourceId),
                                     IsType("QQmlComponent",
                                            Synchronization::ImportedType{},
                                            Synchronization::ImportedType{"QObject"},
                                            Storage::TypeTraits::Reference,
                                            qmltypesFileSourceId)));
}

TEST_F(QmlTypesParser, exported_types)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                          exports: ["QML/QtObject 1.0", "QtQml/QtObject 2.1"]
                      }})"};
    ModuleId qmlModuleId = storage.moduleId("QML");
    ModuleId qtQmlModuleId = storage.moduleId("QtQml");

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(
        types,
        ElementsAre(Field(
            &Synchronization::Type::exportedTypes,
            UnorderedElementsAre(
                IsExportedType(qmlModuleId, "QtObject", QmlDesigner::Storage::Version{1, 0}),
                IsExportedType(qtQmlModuleId, "QtObject", QmlDesigner::Storage::Version{2, 1}),
                IsExportedType(qtQmlNativeModuleId, "QObject", QmlDesigner::Storage::Version{})))));
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

    parser.parse(source, imports, types, projectData);

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

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types,
                Contains(
                    Field(&Synchronization::Type::propertyDeclarations,
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

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types,
                ElementsAre(
                    Field(&Synchronization::Type::propertyDeclarations,
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

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types,
                ElementsAre(Field(
                    &Synchronization::Type::functionDeclarations,
                    UnorderedElementsAre(
                        AllOf(IsFunctionDeclaration("advance", ""),
                              Field(&Synchronization::FunctionDeclaration::parameters,
                                    UnorderedElementsAre(IsParameter("frames", "int"),
                                                         IsParameter("fps", "double")))),
                        AllOf(IsFunctionDeclaration("isImageLoading", "bool"),
                              Field(&Synchronization::FunctionDeclaration::parameters,
                                    UnorderedElementsAre(IsParameter("url", "QUrl")))),
                        AllOf(IsFunctionDeclaration("getContext", ""),
                              Field(&Synchronization::FunctionDeclaration::parameters,
                                    UnorderedElementsAre(IsParameter("args", "QQmlV4Function")))),
                        AllOf(IsFunctionDeclaration("movieUpdate", ""),
                              Field(&Synchronization::FunctionDeclaration::parameters, IsEmpty()))))));
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

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types, ElementsAre(Field(&Synchronization::Type::functionDeclarations, IsEmpty())));
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

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types,
                Contains(
                    Field(&Synchronization::Type::functionDeclarations,
                          UnorderedElementsAre(AllOf(
                              IsFunctionDeclaration("values", ""),
                              Field(&Synchronization::FunctionDeclaration::parameters,
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

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types,
                ElementsAre(Field(&Synchronization::Type::signalDeclarations,
                                  UnorderedElementsAre(
                                      AllOf(IsSignalDeclaration("advance"),
                                            Field(&Synchronization::SignalDeclaration::parameters,
                                                  UnorderedElementsAre(IsParameter("frames", "int"),
                                                                       IsParameter("fps", "double")))),
                                      AllOf(IsSignalDeclaration("isImageLoading"),
                                            Field(&Synchronization::SignalDeclaration::parameters,
                                                  UnorderedElementsAre(IsParameter("url", "QUrl")))),
                                      AllOf(IsSignalDeclaration("getContext"),
                                            Field(&Synchronization::SignalDeclaration::parameters,
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

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types,
                Contains(
                    Field(&Synchronization::Type::signalDeclarations,
                          UnorderedElementsAre(AllOf(
                              IsSignalDeclaration("values"),
                              Field(&Synchronization::SignalDeclaration::parameters,
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

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types,
                Contains(Field(
                    &Synchronization::Type::enumerationDeclarations,
                    UnorderedElementsAre(
                        AllOf(IsEnumeration("NamedColorSpace"),
                              Field(&Synchronization::EnumerationDeclaration::enumeratorDeclarations,
                                    UnorderedElementsAre(IsEnumerator("Unknown"),
                                                         IsEnumerator("SRgb"),
                                                         IsEnumerator("AdobeRgb"),
                                                         IsEnumerator("DisplayP3")))),
                        AllOf(IsEnumeration("VerticalLayoutDirection"),
                              Field(&Synchronization::EnumerationDeclaration::enumeratorDeclarations,
                                    UnorderedElementsAre(IsEnumerator("TopToBottom"),
                                                         IsEnumerator("BottomToTop"))))))));
}

TEST_F(QmlTypesParser, enumeration_is_exported_as_type)
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

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(
        types,
        UnorderedElementsAre(
            AllOf(IsType("QObject::NamedColorSpace",
                         Synchronization::ImportedType{},
                         Synchronization::ImportedType{},
                         Storage::TypeTraits::Value | Storage::TypeTraits::IsEnum,
                         qmltypesFileSourceId),
                  Field(&Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQmlNativeModuleId,
                                                            "QObject::NamedColorSpace",
                                                            QmlDesigner::Storage::Version{})))),
            AllOf(IsType("QObject::VerticalLayoutDirection",
                         Synchronization::ImportedType{},
                         Synchronization::ImportedType{},
                         Storage::TypeTraits::Value | Storage::TypeTraits::IsEnum,
                         qmltypesFileSourceId),
                  Field(&Synchronization::Type::exportedTypes,
                        UnorderedElementsAre(IsExportedType(qtQmlNativeModuleId,
                                                            "QObject::VerticalLayoutDirection",
                                                            QmlDesigner::Storage::Version{})))),
            _));
}

TEST_F(QmlTypesParser, enumeration_is_exported_as_type_with_alias)
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

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types,
                UnorderedElementsAre(
                    AllOf(IsType("QObject::NamedColorSpaces",
                                 Synchronization::ImportedType{},
                                 Synchronization::ImportedType{},
                                 Storage::TypeTraits::Value | Storage::TypeTraits::IsEnum,
                                 qmltypesFileSourceId),
                          Field(&Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQmlNativeModuleId,
                                                                    "QObject::NamedColorSpace",
                                                                    QmlDesigner::Storage::Version{}),
                                                     IsExportedType(qtQmlNativeModuleId,
                                                                    "QObject::NamedColorSpaces",
                                                                    QmlDesigner::Storage::Version{})))),
                    _));
}

TEST_F(QmlTypesParser, enumeration_is_exported_as_type_with_alias_too)
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

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types,
                UnorderedElementsAre(
                    AllOf(IsType("QObject::NamedColorSpaces",
                                 Synchronization::ImportedType{},
                                 Synchronization::ImportedType{},
                                 Storage::TypeTraits::Value | Storage::TypeTraits::IsEnum,
                                 qmltypesFileSourceId),
                          Field(&Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(qtQmlNativeModuleId,
                                                                    "QObject::NamedColorSpace",
                                                                    QmlDesigner::Storage::Version{}),
                                                     IsExportedType(qtQmlNativeModuleId,
                                                                    "QObject::NamedColorSpaces",
                                                                    QmlDesigner::Storage::Version{})))),
                    _));
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

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types,
                Contains(Field(&Synchronization::Type::propertyDeclarations,
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

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types,
                Contains(Field(&Synchronization::Type::propertyDeclarations,
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

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types, ElementsAre(IsTypeTrait(Storage::TypeTraits::Reference)));
}

TEST_F(QmlTypesParser, access_type_is_value)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                                    accessSemantics: "value"}})"};

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types, ElementsAre(IsTypeTrait(Storage::TypeTraits::Value)));
}

TEST_F(QmlTypesParser, access_type_is_sequence)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                                    accessSemantics: "sequence"}})"};

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types, ElementsAre(IsTypeTrait(Storage::TypeTraits::Sequence)));
}

TEST_F(QmlTypesParser, access_type_is_none)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                                    accessSemantics: "none"}})"};

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types, ElementsAre(IsTypeTrait(Storage::TypeTraits::None)));
}

TEST_F(QmlTypesParser, uses_custom_parser)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                                    hasCustomParser: true }})"};

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types, ElementsAre(IsTypeTrait(HasFlag(Storage::TypeTraits::UsesCustomParser))));
}

TEST_F(QmlTypesParser, uses_no_custom_parser)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                                    hasCustomParser: false }})"};

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types, ElementsAre(IsTypeTrait(Not(HasFlag(Storage::TypeTraits::UsesCustomParser)))));
}

} // namespace
