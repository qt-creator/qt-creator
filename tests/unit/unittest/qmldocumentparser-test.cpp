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
#include <projectstorage/qmldocumentparser.h>
#include <projectstorage/sourcepathcache.h>

namespace {

namespace Storage = QmlDesigner::Storage;
using QmlDesigner::ModuleId;
using QmlDesigner::SourceContextId;
using QmlDesigner::SourceId;

MATCHER_P(HasPrototype, prototype, std::string(negation ? "isn't " : "is ") + PrintToString(prototype))
{
    const Storage::Type &type = arg;

    return Storage::ImportedTypeName{prototype} == type.prototype;
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

class QmlDocumentParser : public ::testing::Test
{
public:
protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    QmlDesigner::ProjectStorage<Sqlite::Database> storage{database, database.isInitialized()};
    QmlDesigner::SourcePathCache<QmlDesigner::ProjectStorage<Sqlite::Database>> sourcePathCache{
        storage};
    QmlDesigner::QmlDocumentParser parser{storage, sourcePathCache};
    Storage::Imports imports;
    SourceId qmlFileSourceId{sourcePathCache.sourceId("/path/to/qmlfile.qml")};
    SourceContextId qmlFileSourceContextId{sourcePathCache.sourceContextId(qmlFileSourceId)};
    ModuleId directoryModuleId{storage.moduleId("/path/to")};
};

TEST_F(QmlDocumentParser, Prototype)
{
    auto type = parser.parse("Example{}", imports, qmlFileSourceId);

    ASSERT_THAT(type, HasPrototype(Storage::ImportedType("Example")));
}

TEST_F(QmlDocumentParser, QualifiedPrototype)
{
    auto exampleModuleId = storage.moduleId("Example");
    QString text = R"(import Example 2.1 as Example
                      Example.Item{})";

    auto type = parser.parse(text, imports, qmlFileSourceId);

    ASSERT_THAT(type,
                HasPrototype(Storage::QualifiedImportedType("Item",
                                                            Storage::Import{exampleModuleId,
                                                                            Storage::Version{2, 1},
                                                                            qmlFileSourceId})));
}

TEST_F(QmlDocumentParser, Properties)
{
    auto type = parser.parse(R"(Example{ property int foo })", imports, qmlFileSourceId);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(IsPropertyDeclaration("foo",
                                                           Storage::ImportedType{"int"},
                                                           Storage::PropertyDeclarationTraits::None)));
}

TEST_F(QmlDocumentParser, QualifiedProperties)
{
    auto exampleModuleId = storage.moduleId("Example");

    auto type = parser.parse(R"(import Example 2.1 as Example
                                Item{ property Example.Foo foo})",
                             imports,
                             qmlFileSourceId);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(IsPropertyDeclaration(
                    "foo",
                    Storage::QualifiedImportedType("Foo",
                                                   Storage::Import{exampleModuleId,
                                                                   Storage::Version{2, 1},
                                                                   qmlFileSourceId}),
                    Storage::PropertyDeclarationTraits::None)));
}

TEST_F(QmlDocumentParser, EnumerationInProperties)
{
    auto type = parser.parse(R"(import Example 2.1 as Example
                                Item{ property Enumeration.Foo foo})",
                             imports,
                             qmlFileSourceId);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(IsPropertyDeclaration("foo",
                                                           Storage::ImportedType("Enumeration.Foo"),
                                                           Storage::PropertyDeclarationTraits::None)));
}

TEST_F(QmlDocumentParser, QualifiedEnumerationInProperties)
{
    auto exampleModuleId = storage.moduleId("Example");

    auto type = parser.parse(R"(import Example 2.1 as Example
                                Item{ property Example.Enumeration.Foo foo})",
                             imports,
                             qmlFileSourceId);

    ASSERT_THAT(type.propertyDeclarations,
                UnorderedElementsAre(IsPropertyDeclaration(
                    "foo",
                    Storage::QualifiedImportedType("Enumeration.Foo",
                                                   Storage::Import{exampleModuleId,
                                                                   Storage::Version{2, 1},
                                                                   qmlFileSourceId}),
                    Storage::PropertyDeclarationTraits::None)));
}

TEST_F(QmlDocumentParser, Imports)
{
    ModuleId fooDirectoryModuleId = storage.moduleId("/path/foo");
    ModuleId qmlModuleId = storage.moduleId("QML");
    ModuleId qtQmlModuleId = storage.moduleId("QtQml");
    ModuleId qtQuickModuleId = storage.moduleId("QtQuick");

    auto type = parser.parse(R"(import QtQuick
                                import "../foo"
                                Example{})",
                             imports,
                             qmlFileSourceId);

    ASSERT_THAT(imports,
                UnorderedElementsAre(
                    Storage::Import{directoryModuleId, Storage::Version{}, qmlFileSourceId},
                    Storage::Import{fooDirectoryModuleId, Storage::Version{}, qmlFileSourceId},
                    Storage::Import{qmlModuleId, Storage::Version{1, 0}, qmlFileSourceId},
                    Storage::Import{qtQmlModuleId, Storage::Version{6, 0}, qmlFileSourceId},
                    Storage::Import{qtQuickModuleId, Storage::Version{}, qmlFileSourceId}));
}

TEST_F(QmlDocumentParser, Functions)
{
    auto type = parser.parse(
        "Example{\n function someScript(x, y) {}\n function otherFunction() {}\n}",
        imports,
        qmlFileSourceId);

    ASSERT_THAT(type.functionDeclarations,
                UnorderedElementsAre(AllOf(IsFunctionDeclaration("otherFunction", ""),
                                           Field(&Storage::FunctionDeclaration::parameters, IsEmpty())),
                                     AllOf(IsFunctionDeclaration("someScript", ""),
                                           Field(&Storage::FunctionDeclaration::parameters,
                                                 ElementsAre(IsParameter("x", ""),
                                                             IsParameter("y", ""))))));
}

TEST_F(QmlDocumentParser, Signals)
{
    auto type = parser.parse("Example{\n signal someSignal(int x, real y)\n signal signal2()\n}",
                             imports,
                             qmlFileSourceId);

    ASSERT_THAT(type.signalDeclarations,
                UnorderedElementsAre(AllOf(IsSignalDeclaration("someSignal"),
                                           Field(&Storage::SignalDeclaration::parameters,
                                                 ElementsAre(IsParameter("x", "int"),
                                                             IsParameter("y", "real")))),
                                     AllOf(IsSignalDeclaration("signal2"),
                                           Field(&Storage::SignalDeclaration::parameters, IsEmpty()))));
}

TEST_F(QmlDocumentParser, Enumeration)
{
    auto type = parser.parse("Example{\n enum Color{red, green, blue=10, white}\n enum "
                             "State{On,Off}\n}",
                             imports,
                             qmlFileSourceId);

    ASSERT_THAT(type.enumerationDeclarations,
                UnorderedElementsAre(
                    AllOf(IsEnumeration("Color"),
                          Field(&Storage::EnumerationDeclaration::enumeratorDeclarations,
                                ElementsAre(IsEnumerator("red", 0),
                                            IsEnumerator("green", 1),
                                            IsEnumerator("blue", 10),
                                            IsEnumerator("white", 11)))),
                    AllOf(IsEnumeration("State"),
                          Field(&Storage::EnumerationDeclaration::enumeratorDeclarations,
                                ElementsAre(IsEnumerator("On", 0), IsEnumerator("Off", 1))))));
}

TEST_F(QmlDocumentParser, DISABLED_DuplicateImportsAreRemoved)
{
    ModuleId fooDirectoryModuleId = storage.moduleId("/path/foo");
    ModuleId qmlModuleId = storage.moduleId("QML");
    ModuleId qtQmlModuleId = storage.moduleId("QtQml");
    ModuleId qtQuickModuleId = storage.moduleId("QtQuick");

    auto type = parser.parse(R"(import QtQuick
                                import "../foo"
                                import QtQuick
                                import "../foo"
                                import "/path/foo"
                                import "."
 
                                Example{})",
                             imports,
                             qmlFileSourceId);

    ASSERT_THAT(imports,
                UnorderedElementsAre(
                    Storage::Import{directoryModuleId, Storage::Version{}, qmlFileSourceId},
                    Storage::Import{fooDirectoryModuleId, Storage::Version{}, qmlFileSourceId},
                    Storage::Import{qmlModuleId, Storage::Version{1, 0}, qmlFileSourceId},
                    Storage::Import{qtQmlModuleId, Storage::Version{6, 0}, qmlFileSourceId},
                    Storage::Import{qtQuickModuleId, Storage::Version{}, qmlFileSourceId}));
}

} // namespace
