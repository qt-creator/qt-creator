/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <sqlitedatabase.h>

#include <projectstorage/projectstorage.h>
#include <projectstorage/projectstoragetypes.h>
#include <projectstorage/qmltypesparser.h>
#include <projectstorage/sourcepathcache.h>

namespace {

namespace Storage = QmlDesigner::Storage;
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
    const Storage::Type &type = arg;

    return Storage::ImportedTypeName{prototype} == type.prototype;
}

MATCHER_P4(IsType,
           typeName,
           prototype,
           accessSemantics,
           sourceId,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Type{typeName, prototype, accessSemantics, sourceId}))
{
    const Storage::Type &type = arg;

    return type.typeName == typeName && type.prototype == Storage::ImportedTypeName{prototype}
           && type.accessSemantics == accessSemantics && type.sourceId == sourceId;
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
           && propertyDeclaration.traits == traits
           && propertyDeclaration.kind == Storage::PropertyKind::Property;
}

MATCHER_P2(IsFunctionDeclaration,
           name,
           returnTypeName,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::FunctionDeclaration{name, returnTypeName}))
{
    const Storage::FunctionDeclaration &declaration = arg;

    return declaration.name == name && declaration.returnTypeName == returnTypeName;
}

MATCHER_P(IsSignalDeclaration,
          name,
          std::string(negation ? "isn't " : "is ") + PrintToString(Storage::SignalDeclaration{name}))
{
    const Storage::SignalDeclaration &declaration = arg;

    return declaration.name == name;
}

MATCHER_P2(IsParameter,
           name,
           typeName,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::ParameterDeclaration{name, typeName}))
{
    const Storage::ParameterDeclaration &declaration = arg;

    return declaration.name == name && declaration.typeName == typeName;
}

MATCHER_P(IsEnumeration,
          name,
          std::string(negation ? "isn't " : "is ")
              + PrintToString(Storage::EnumerationDeclaration{name, {}}))
{
    const Storage::EnumerationDeclaration &declaration = arg;

    return declaration.name == name;
}

MATCHER_P(IsEnumerator,
          name,
          std::string(negation ? "isn't " : "is ")
              + PrintToString(Storage::EnumeratorDeclaration{name}))
{
    const Storage::EnumeratorDeclaration &declaration = arg;

    return declaration.name == name && !declaration.hasValue;
}

MATCHER_P2(IsEnumerator,
           name,
           value,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::EnumeratorDeclaration{name, value, true}))
{
    const Storage::EnumeratorDeclaration &declaration = arg;

    return declaration.name == name && declaration.value == value && declaration.hasValue;
}

MATCHER_P3(IsExportedType,
           moduleId,
           name,
           version,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::ExportedType{moduleId, name, version}))
{
    const Storage::ExportedType &type = arg;

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
    QmlDesigner::QmlTypesParser parser{sourcePathCache, storage};
    Storage::Imports imports;
    Storage::Types types;
    SourceId qmltypesFileSourceId{sourcePathCache.sourceId("path/to/types.qmltypes")};
    QmlDesigner::Storage::ProjectData projectData{qmltypesFileSourceId,
                                                  qmltypesFileSourceId,
                                                  storage.moduleId("QtQml-cppnative"),
                                                  Storage::FileType::QmlTypes};
    SourceContextId qmltypesFileSourceContextId{sourcePathCache.sourceContextId(qmltypesFileSourceId)};
    ModuleId directoryModuleId{storage.moduleId("path/to/")};
};

TEST_F(QmlTypesParser, Imports)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        dependencies:
                          ["QtQuick 2.15", "QtQuick.Window 2.1", "QtFoo 6"]})"};

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(
        imports,
        UnorderedElementsAre(
            IsImport(storage.moduleId("QML"), Storage::Version{}, qmltypesFileSourceId),
            IsImport(storage.moduleId("QtQml-cppnative"), Storage::Version{}, qmltypesFileSourceId),
            IsImport(storage.moduleId("QtQuick-cppnative"), Storage::Version{}, qmltypesFileSourceId),
            IsImport(storage.moduleId("QtQuick.Window-cppnative"), Storage::Version{}, qmltypesFileSourceId),
            IsImport(storage.moduleId("QtFoo-cppnative"), Storage::Version{}, qmltypesFileSourceId)));
}

TEST_F(QmlTypesParser, Types)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"}
                        Component { name: "QQmlComponent"
                                    prototype: "QObject"}})"};

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types,
                UnorderedElementsAre(IsType("QObject",
                                            Storage::NativeType{},
                                            Storage::TypeAccessSemantics::Reference,
                                            qmltypesFileSourceId),
                                     IsType("QQmlComponent",
                                            Storage::NativeType{"QObject"},
                                            Storage::TypeAccessSemantics::Reference,
                                            qmltypesFileSourceId)));
}

TEST_F(QmlTypesParser, ExportedTypes)
{
    QString source{R"(import QtQuick.tooling 1.2
                      Module{
                        Component { name: "QObject"
                          exports: ["QML/QtObject 1.0", "QtQml/QtObject 2.1"]
                      }})"};
    ModuleId qmlModuleId = storage.moduleId("QML");
    ModuleId qtQmlModuleId = storage.moduleId("QtQml");
    ModuleId qtQmlNativeModuleId = storage.moduleId("QtQml-cppnative");

    parser.parse(source, imports, types, projectData);

    ASSERT_THAT(types,
                ElementsAre(Field(
                    &Storage::Type::exportedTypes,
                    ElementsAre(IsExportedType(qmlModuleId, "QtObject", Storage::Version{1, 0}),
                                IsExportedType(qtQmlModuleId, "QtObject", Storage::Version{2, 1}),
                                IsExportedType(qtQmlNativeModuleId, "QObject", Storage::Version{})))));
}

TEST_F(QmlTypesParser, Properties)
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
                    &Storage::Type::propertyDeclarations,
                    UnorderedElementsAre(
                        IsPropertyDeclaration("objectName",
                                              Storage::NativeType{"string"},
                                              Storage::PropertyDeclarationTraits::None),
                        IsPropertyDeclaration("target",
                                              Storage::NativeType{"QObject"},
                                              Storage::PropertyDeclarationTraits::IsPointer),
                        IsPropertyDeclaration("progress",
                                              Storage::NativeType{"double"},
                                              Storage::PropertyDeclarationTraits::IsReadOnly),
                        IsPropertyDeclaration("targets",
                                              Storage::NativeType{"QQuickItem"},
                                              Storage::PropertyDeclarationTraits::IsReadOnly
                                                  | Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsPointer)))));
}

TEST_F(QmlTypesParser, Functions)
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
                ElementsAre(
                    Field(&Storage::Type::functionDeclarations,
                          UnorderedElementsAre(
                              AllOf(IsFunctionDeclaration("advance", ""),
                                    Field(&Storage::FunctionDeclaration::parameters,
                                          UnorderedElementsAre(IsParameter("frames", "int"),
                                                               IsParameter("fps", "double")))),
                              AllOf(IsFunctionDeclaration("isImageLoading", "bool"),
                                    Field(&Storage::FunctionDeclaration::parameters,
                                          UnorderedElementsAre(IsParameter("url", "QUrl")))),
                              AllOf(IsFunctionDeclaration("getContext", ""),
                                    Field(&Storage::FunctionDeclaration::parameters,
                                          UnorderedElementsAre(IsParameter("args", "QQmlV4Function")))),
                              AllOf(IsFunctionDeclaration("movieUpdate", ""),
                                    Field(&Storage::FunctionDeclaration::parameters, IsEmpty()))))));
}

TEST_F(QmlTypesParser, Signals)
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
                ElementsAre(Field(&Storage::Type::signalDeclarations,
                                  UnorderedElementsAre(
                                      AllOf(IsSignalDeclaration("advance"),
                                            Field(&Storage::SignalDeclaration::parameters,
                                                  UnorderedElementsAre(IsParameter("frames", "int"),
                                                                       IsParameter("fps", "double")))),
                                      AllOf(IsSignalDeclaration("isImageLoading"),
                                            Field(&Storage::SignalDeclaration::parameters,
                                                  UnorderedElementsAre(IsParameter("url", "QUrl")))),
                                      AllOf(IsSignalDeclaration("getContext"),
                                            Field(&Storage::SignalDeclaration::parameters,
                                                  UnorderedElementsAre(
                                                      IsParameter("args", "QQmlV4Function"))))))));
}

TEST_F(QmlTypesParser, Enumerations)
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
                ElementsAre(
                    Field(&Storage::Type::enumerationDeclarations,
                          UnorderedElementsAre(
                              AllOf(IsEnumeration("NamedColorSpace"),
                                    Field(&Storage::EnumerationDeclaration::enumeratorDeclarations,
                                          UnorderedElementsAre(IsEnumerator("Unknown"),
                                                               IsEnumerator("SRgb"),
                                                               IsEnumerator("AdobeRgb"),
                                                               IsEnumerator("DisplayP3")))),
                              AllOf(IsEnumeration("VerticalLayoutDirection"),
                                    Field(&Storage::EnumerationDeclaration::enumeratorDeclarations,
                                          UnorderedElementsAre(IsEnumerator("TopToBottom"),
                                                               IsEnumerator("BottomToTop"))))))));
}

} // namespace
