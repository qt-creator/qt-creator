// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <googletest.h>
#include <mocks/projectstoragemock.h>
#include <mocks/propertycomponentgeneratormock.h>
#include <strippedstring-matcher.h>

#include <propertyeditorcomponentgenerator.h>

namespace {

using BasicProperty = QmlDesigner::PropertyComponentGenerator::BasicProperty;
using ComplexProperty = QmlDesigner::PropertyComponentGenerator::ComplexProperty;
using QmlDesigner::PropertyMetaInfo;

class PropertyEditorComponentGenerator : public ::testing::Test
{
protected:
    QmlDesigner::NodeMetaInfo createType(Utils::SmallStringView name,
                                         QmlDesigner::TypeIds baseTypeIds = {})
    {
        auto typeId = projectStorageMock.createValue(qtQuickModuleId, name, baseTypeIds);

        return {typeId, &projectStorageMock};
    }

    QmlDesigner::PropertyMetaInfo createProperty(QmlDesigner::TypeId typeId,
                                                 Utils::SmallString name,
                                                 QmlDesigner::Storage::PropertyDeclarationTraits traits,
                                                 QmlDesigner::TypeId propertyTypeId)
    {
        auto propertyId = projectStorageMock.createProperty(typeId, name, traits, propertyTypeId);

        return {propertyId, &projectStorageMock};
    }

    void setImports(const QStringList &imports)
    {
        ON_CALL(propertyGeneratorMock, imports()).WillByDefault(Return(imports));
    }

    void addBasicProperty(const PropertyMetaInfo &property,
                          Utils::SmallStringView name,
                          const QString &componentContent)
    {
        ON_CALL(propertyGeneratorMock, create(property))
            .WillByDefault(Return(BasicProperty{name, componentContent}));
    }

    void addComplexProperty(const PropertyMetaInfo &property,
                            Utils::SmallStringView name,
                            const QString &componentContent)
    {
        ON_CALL(propertyGeneratorMock, create(property))
            .WillByDefault(Return(ComplexProperty{name, componentContent}));
    }

    QmlDesigner::PropertyMetaInfo createBasicProperty(QmlDesigner::TypeId typeId,
                                                      Utils::SmallString name,
                                                      QmlDesigner::Storage::PropertyDeclarationTraits traits,
                                                      QmlDesigner::TypeId propertyTypeId,
                                                      const QString &componentContent)
    {
        auto propertyInfo = createProperty(typeId, name, traits, propertyTypeId);
        addBasicProperty(propertyInfo, name, componentContent);

        return propertyInfo;
    }

    QmlDesigner::PropertyMetaInfo createComplexProperty(
        QmlDesigner::TypeId typeId,
        Utils::SmallString name,
        QmlDesigner::Storage::PropertyDeclarationTraits traits,
        QmlDesigner::TypeId propertyTypeId,
        const QString &componentContent)
    {
        auto propertyInfo = createProperty(typeId, name, traits, propertyTypeId);
        addComplexProperty(propertyInfo, name, componentContent);

        return propertyInfo;
    }

protected:
    QmlDesigner::SourceId sourceId = QmlDesigner::SourceId::create(10);
    NiceMock<ProjectStorageMockWithQtQtuick> projectStorageMock{sourceId};
    NiceMock<PropertyComponentGeneratorMock> propertyGeneratorMock;
    QmlDesigner::PropertyEditorComponentGenerator generator{propertyGeneratorMock};
    QmlDesigner::ModuleId qtQuickModuleId = projectStorageMock.createModule("QtQuick");
    QmlDesigner::NodeMetaInfo fooTypeInfo = createType("Foo");
    QmlDesigner::TypeId dummyTypeId = projectStorageMock.commonTypeCache().builtinTypeId<double>();
};

TEST_F(PropertyEditorComponentGenerator, no_properties_and_no_imports)
{
    QString expectedText{
        R"xy(
      Column {
        width: parent.width
        Section {
          caption: "Exposed Custom Properties"
          anchors.left: parent.left
          anchors.right: parent.right
          leftPadding: 0
          rightPadding: 0
          bottomPadding: 0
          Column {
            width: parent.width
          }
        }
      })xy"};

    auto text = generator.create(fooTypeInfo.selfAndPrototypes(), false);

    ASSERT_THAT(text, StrippedStringEq(expectedText));
}

TEST_F(PropertyEditorComponentGenerator, properties_without_component_are_not_shows)
{
    QString expectedText{
        R"xy(
      Column {
        width: parent.width
        Section {
          caption: "Exposed Custom Properties"
          anchors.left: parent.left
          anchors.right: parent.right
          leftPadding: 0
          rightPadding: 0
          bottomPadding: 0
          Column {
            width: parent.width
          }
        }
      })xy"};
    createProperty(fooTypeInfo.id(), "x", {}, dummyTypeId);

    auto text = generator.create(fooTypeInfo.selfAndPrototypes(), false);

    ASSERT_THAT(text, StrippedStringEq(expectedText));
}

TEST_F(PropertyEditorComponentGenerator, show_component_button_for_a_component_node)
{
    QString expectedText{
        R"xy(
      Column {
        width: parent.width
        ComponentButton {}
        Section {
          caption: "Exposed Custom Properties"
          anchors.left: parent.left
          anchors.right: parent.right
          leftPadding: 0
          rightPadding: 0
          bottomPadding: 0
          Column {
            width: parent.width
          }
        }
      })xy"};

    auto text = generator.create(fooTypeInfo.selfAndPrototypes(), true);

    ASSERT_THAT(text, StrippedStringEq(expectedText));
}

TEST_F(PropertyEditorComponentGenerator, imports)
{
    QString expectedText{
        R"xy(
      import QtQtuick
      import Studio 2.1
      Column {
        width: parent.width
        Section {
          caption: "Exposed Custom Properties"
          anchors.left: parent.left
          anchors.right: parent.right
          leftPadding: 0
          rightPadding: 0
          bottomPadding: 0
          Column {
            width: parent.width
          }
        }
      })xy"};
    setImports({"import QtQtuick", "import Studio 2.1"});

    auto text = generator.create(fooTypeInfo.selfAndPrototypes(), false);

    ASSERT_THAT(text, StrippedStringEq(expectedText));
}

TEST_F(PropertyEditorComponentGenerator, basic_property)
{
    QString expectedText{
        R"xy(
      Column {
        width: parent.width
        Section {
          caption: "Exposed Custom Properties"
          anchors.left: parent.left
          anchors.right: parent.right
          leftPadding: 0
          rightPadding: 0
          bottomPadding: 0
          Column {
            width: parent.width
            Column {
              width: parent.width
              leftPadding: 8
              bottomPadding: 10
              SectionLayout {
                Double{}
              }
            }
          }
        }
      })xy"};
    createBasicProperty(fooTypeInfo.id(), "value", {}, dummyTypeId, "Double{}");

    auto text = generator.create(fooTypeInfo.selfAndPrototypes(), false);

    ASSERT_THAT(text, StrippedStringEq(expectedText));
}

TEST_F(PropertyEditorComponentGenerator, basic_properties_with_base_type)
{
    QString expectedText{
        R"xy(
      Column {
        width: parent.width
        Section {
          caption: "Exposed Custom Properties"
          anchors.left: parent.left
          anchors.right: parent.right
          leftPadding: 0
          rightPadding: 0
          bottomPadding: 0
          Column {
            width: parent.width
            Column {
              width: parent.width
              leftPadding: 8
              bottomPadding: 10
              SectionLayout {
                SuperDouble{}
                Double{}
              }
            }
          }
        }
      })xy"};
    createBasicProperty(fooTypeInfo.id(), "x", {}, dummyTypeId, "Double{}");
    auto superFooInfo = createType("SuperFoo", {fooTypeInfo.id()});
    createBasicProperty(superFooInfo.id(), "value", {}, dummyTypeId, "SuperDouble{}");

    auto text = generator.create(superFooInfo.selfAndPrototypes(), false);

    ASSERT_THAT(text, StrippedStringEq(expectedText));
}

TEST_F(PropertyEditorComponentGenerator,
       only_handle_basic_properties_for_types_without_specifics_or_panes)
{
    QString expectedText{
        R"xy(
      Column {
        width: parent.width
        Section {
          caption: "Exposed Custom Properties"
          anchors.left: parent.left
          anchors.right: parent.right
          leftPadding: 0
          rightPadding: 0
          bottomPadding: 0
          Column {
            width: parent.width
            Column {
              width: parent.width
              leftPadding: 8
              bottomPadding: 10
              SectionLayout {
                SuperDouble{}
              }
            }
          }
        }
      })xy"};
    createBasicProperty(fooTypeInfo.id(), "x", {}, dummyTypeId, "Double{}");
    auto superFooInfo = createType("SuperFoo", {fooTypeInfo.id()});
    createBasicProperty(superFooInfo.id(), "value", {}, dummyTypeId, "SuperDouble{}");
    projectStorageMock.setPropertyEditorPathId(fooTypeInfo.id(), sourceId);

    auto text = generator.create(superFooInfo.selfAndPrototypes(), false);

    ASSERT_THAT(text, StrippedStringEq(expectedText));
}

TEST_F(PropertyEditorComponentGenerator, order_basic_property)
{
    QString expectedText{
        R"xy(
      Column {
        width: parent.width
        Section {
          caption: "Exposed Custom Properties"
          anchors.left: parent.left
          anchors.right: parent.right
          leftPadding: 0
          rightPadding: 0
          bottomPadding: 0
          Column {
            width: parent.width
            Column {
              width: parent.width
              leftPadding: 8
              bottomPadding: 10
              SectionLayout {
                AnotherX{}
                AndY{}
                SomeZ{}
              }
            }
          }
        }
      })xy"};
    createBasicProperty(fooTypeInfo.id(), "z", {}, dummyTypeId, "SomeZ{}");
    createBasicProperty(fooTypeInfo.id(), "x", {}, dummyTypeId, "AnotherX{}");
    createBasicProperty(fooTypeInfo.id(), "y", {}, dummyTypeId, "AndY{}");

    auto text = generator.create(fooTypeInfo.selfAndPrototypes(), false);

    ASSERT_THAT(text, StrippedStringEq(expectedText));
}

TEST_F(PropertyEditorComponentGenerator, complex_property)
{
    QString expectedText{
        R"xy(
      Column {
        width: parent.width
        Section {
          caption: "Exposed Custom Properties"
          anchors.left: parent.left
          anchors.right: parent.right
          leftPadding: 0
          rightPadding: 0
          bottomPadding: 0
          Column {
            width: parent.width
            Complex{}
          }
        }
      })xy"};
    createComplexProperty(fooTypeInfo.id(), "value", {}, dummyTypeId, "Complex{}");

    auto text = generator.create(fooTypeInfo.selfAndPrototypes(), false);

    ASSERT_THAT(text, StrippedStringEq(expectedText));
}

TEST_F(PropertyEditorComponentGenerator, complex_properties_with_base_type)
{
    QString expectedText{
        R"xy(
      Column {
        width: parent.width
        Section {
          caption: "Exposed Custom Properties"
          anchors.left: parent.left
          anchors.right: parent.right
          leftPadding: 0
          rightPadding: 0
          bottomPadding: 0
          Column {
            width: parent.width
            Complex{}
            SuperComplex{}
          }
        }
      })xy"};
    createComplexProperty(fooTypeInfo.id(), "delegate", {}, dummyTypeId, "Complex{}");
    auto superFooInfo = createType("SuperFoo", {fooTypeInfo.id()});
    createComplexProperty(superFooInfo.id(), "value", {}, dummyTypeId, "SuperComplex{}");

    auto text = generator.create(superFooInfo.selfAndPrototypes(), false);

    ASSERT_THAT(text, StrippedStringEq(expectedText));
}

TEST_F(PropertyEditorComponentGenerator,
       only_handle_complex_properties_for_types_without_specifics_or_panes)
{
    QString expectedText{
        R"xy(
      Column {
        width: parent.width
        Section {
          caption: "Exposed Custom Properties"
          anchors.left: parent.left
          anchors.right: parent.right
          leftPadding: 0
          rightPadding: 0
          bottomPadding: 0
          Column {
            width: parent.width
            SuperComplex{}
          }
        }
      })xy"};
    createComplexProperty(fooTypeInfo.id(), "delegate", {}, dummyTypeId, "Complex{}");
    auto superFooInfo = createType("SuperFoo", {fooTypeInfo.id()});
    createComplexProperty(superFooInfo.id(), "value", {}, dummyTypeId, "SuperComplex{}");
    projectStorageMock.setPropertyEditorPathId(fooTypeInfo.id(), sourceId);

    auto text = generator.create(superFooInfo.selfAndPrototypes(), false);

    ASSERT_THAT(text, StrippedStringEq(expectedText));
}

TEST_F(PropertyEditorComponentGenerator, ordered_complex_property)
{
    QString expectedText{
        R"xy(
      Column {
        width: parent.width
        Section {
          caption: "Exposed Custom Properties"
          anchors.left: parent.left
          anchors.right: parent.right
          leftPadding: 0
          rightPadding: 0
          bottomPadding: 0
          Column {
            width: parent.width
            Anchors{}
            Delegate{}
            ComplexFont{}
          }
        }
      })xy"};
    createComplexProperty(fooTypeInfo.id(), "delegate", {}, dummyTypeId, "Delegate{}");
    createComplexProperty(fooTypeInfo.id(), "font", {}, dummyTypeId, "ComplexFont{}");
    createComplexProperty(fooTypeInfo.id(), "anchors", {}, dummyTypeId, "Anchors{}");

    auto text = generator.create(fooTypeInfo.selfAndPrototypes(), false);

    ASSERT_THAT(text, StrippedStringEq(expectedText));
}

TEST_F(PropertyEditorComponentGenerator, basic_is_placed_before_complex_components)
{
    QString expectedText{
        R"xy(
      Column {
        width: parent.width
        Section {
          caption: "Exposed Custom Properties"
          anchors.left: parent.left
          anchors.right: parent.right
          leftPadding: 0
          rightPadding: 0
          bottomPadding: 0
          Column {
            width: parent.width
            Column {
              width: parent.width
              leftPadding: 8
              bottomPadding: 10
              SectionLayout {
                Double{}
              }
            }
            Font{}
          }
        }
      })xy"};
    createBasicProperty(fooTypeInfo.id(), "x", {}, dummyTypeId, "Double{}");
    createComplexProperty(fooTypeInfo.id(), "font", {}, dummyTypeId, "Font{}");

    auto text = generator.create(fooTypeInfo.selfAndPrototypes(), false);

    ASSERT_THAT(text, StrippedStringEq(expectedText));
}
} // namespace
