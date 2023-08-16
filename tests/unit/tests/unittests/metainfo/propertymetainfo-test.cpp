// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <matchers/info_exportedtypenames-matcher.h>
#include <matchers/qvariant-matcher.h>
#include <mocks/projectstoragemock.h>
#include <mocks/sourcepathcachemock.h>

#include <designercore/include/model.h>
#include <designercore/include/modelnode.h>
#include <designercore/include/nodemetainfo.h>
#include <enumeration.h>

#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

namespace {

using QmlDesigner::Enumeration;
using QmlDesigner::ModelNode;
using QmlDesigner::ModelNodes;
using QmlDesigner::Storage::PropertyDeclarationTraits;
using QmlDesigner::Storage::TypeTraits;

class PropertyMetaInfo : public ::testing::Test
{
protected:
    QmlDesigner::NodeMetaInfo createNodeMetaInfo(Utils::SmallStringView moduleName,
                                                 Utils::SmallStringView typeName,
                                                 QmlDesigner::Storage::TypeTraits typeTraits = {})
    {
        auto moduleId = projectStorageMock.createModule(moduleName);
        auto typeId = projectStorageMock.createType(moduleId, typeName, typeTraits);

        return QmlDesigner::NodeMetaInfo{typeId, &projectStorageMock};
    }

protected:
    NiceMock<SourcePathCacheMockWithPaths> pathCache{"/path/foo.qml"};
    NiceMock<ProjectStorageMockWithQtQtuick> projectStorageMock{pathCache.sourceId};
    QmlDesigner::Model model{{projectStorageMock, pathCache},
                             "Item",
                             {QmlDesigner::Import::createLibraryImport("QML"),
                              QmlDesigner::Import::createLibraryImport("QtQuick"),
                              QmlDesigner::Import::createLibraryImport("QtQml.Models")},
                             QUrl::fromLocalFile(pathCache.path.toQString())};
    QmlDesigner::NodeMetaInfo nodeInfo = createNodeMetaInfo("QtQuick", "Foo");
};

TEST_F(PropertyMetaInfo, name)
{
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, nodeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");

    auto name = propertyInfo.name();

    ASSERT_THAT(name, "bar");
}

TEST_F(PropertyMetaInfo, default_has_no_name)
{
    QmlDesigner::PropertyMetaInfo propertyInfo;

    auto name = propertyInfo.name();

    ASSERT_THAT(name, IsEmpty());
}

TEST_F(PropertyMetaInfo, property_type)
{
    auto barInfo = createNodeMetaInfo("QtQuick", "Bar");
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, barInfo.id());
    auto propertyInfo = nodeInfo.property("bar");

    auto type = propertyInfo.propertyType();

    ASSERT_THAT(type, barInfo);
}

TEST_F(PropertyMetaInfo, default_hads_invalid_property_type)
{
    QmlDesigner::PropertyMetaInfo propertyInfo;

    auto type = propertyInfo.propertyType();

    ASSERT_THAT(type, IsFalse());
}

TEST_F(PropertyMetaInfo, type)
{
    auto barInfo = createNodeMetaInfo("QtQuick", "Bar");
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, barInfo.id());
    auto propertyInfo = nodeInfo.property("bar");

    auto type = propertyInfo.type();

    ASSERT_THAT(type, nodeInfo);
}

TEST_F(PropertyMetaInfo, default_has_invalid_type)
{
    QmlDesigner::PropertyMetaInfo propertyInfo;

    auto type = propertyInfo.type();

    ASSERT_THAT(type, IsFalse());
}

TEST_F(PropertyMetaInfo, is_writable)
{
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, nodeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");

    bool isList = propertyInfo.isWritable();

    ASSERT_THAT(isList, IsTrue());
}

TEST_F(PropertyMetaInfo, is_not_writable)
{
    projectStorageMock.createProperty(nodeInfo.id(),
                                      "bar",
                                      PropertyDeclarationTraits::IsReadOnly,
                                      nodeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");

    bool isList = propertyInfo.isWritable();

    ASSERT_THAT(isList, IsFalse());
}

TEST_F(PropertyMetaInfo, default_is_not_writable)
{
    projectStorageMock.createProperty(nodeInfo.id(),
                                      "bar",
                                      PropertyDeclarationTraits::IsReadOnly,
                                      nodeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");

    bool isList = propertyInfo.isWritable();

    ASSERT_THAT(isList, IsFalse());
}

TEST_F(PropertyMetaInfo, is_list)
{
    projectStorageMock.createProperty(nodeInfo.id(),
                                      "bar",
                                      PropertyDeclarationTraits::IsList,
                                      nodeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");

    bool isList = propertyInfo.isListProperty();

    ASSERT_THAT(isList, IsTrue());
}

TEST_F(PropertyMetaInfo, is_not_list)
{
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, nodeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");

    bool isList = propertyInfo.isListProperty();

    ASSERT_THAT(isList, IsFalse());
}

TEST_F(PropertyMetaInfo, default_is_not_list)
{
    QmlDesigner::PropertyMetaInfo propertyInfo;

    bool isList = propertyInfo.isListProperty();

    ASSERT_THAT(isList, IsFalse());
}

TEST_F(PropertyMetaInfo, is_enumeration)
{
    auto enumInfo = createNodeMetaInfo("QtQuick", "MyEnum", TypeTraits::IsEnum);
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, enumInfo.id());
    auto propertyInfo = nodeInfo.property("bar");

    bool isEnum = propertyInfo.isEnumType();

    ASSERT_THAT(isEnum, IsTrue());
}

TEST_F(PropertyMetaInfo, is_not_enumeration)
{
    auto notEnumInfo = createNodeMetaInfo("QtQuick", "NoEnum", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, notEnumInfo.id());
    auto propertyInfo = nodeInfo.property("bar");

    bool isEnum = propertyInfo.isEnumType();

    ASSERT_THAT(isEnum, IsFalse());
}

TEST_F(PropertyMetaInfo, default_is_not_enumeration)
{
    QmlDesigner::PropertyMetaInfo propertyInfo;

    bool isEnum = propertyInfo.isEnumType();

    ASSERT_THAT(isEnum, IsFalse());
}

TEST_F(PropertyMetaInfo, is_private)
{
    projectStorageMock.createProperty(nodeInfo.id(), "__bar", {}, nodeInfo.id());
    auto propertyInfo = nodeInfo.property("__bar");

    bool isPrivate = propertyInfo.isPrivate();

    ASSERT_THAT(isPrivate, IsTrue());
}

TEST_F(PropertyMetaInfo, is_not_private)
{
    projectStorageMock.createProperty(nodeInfo.id(), "_bar", {}, nodeInfo.id());
    auto propertyInfo = nodeInfo.property("_bar");

    bool isPrivate = propertyInfo.isPrivate();

    ASSERT_THAT(isPrivate, IsFalse());
}

TEST_F(PropertyMetaInfo, default_is_not_private)
{
    QmlDesigner::PropertyMetaInfo propertyInfo;

    bool isPrivate = propertyInfo.isPrivate();

    ASSERT_THAT(isPrivate, IsFalse());
}

TEST_F(PropertyMetaInfo, is_pointer)
{
    projectStorageMock.createProperty(nodeInfo.id(),
                                      "bar",
                                      PropertyDeclarationTraits::IsPointer,
                                      nodeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");

    bool isPointer = propertyInfo.isPointer();

    ASSERT_THAT(isPointer, IsTrue());
}

TEST_F(PropertyMetaInfo, is_not_pointer)
{
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, nodeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");

    bool isPointer = propertyInfo.isPointer();

    ASSERT_THAT(isPointer, IsFalse());
}

TEST_F(PropertyMetaInfo, default_is_not_pointer)
{
    QmlDesigner::PropertyMetaInfo propertyInfo;

    bool isPointer = propertyInfo.isPointer();

    ASSERT_THAT(isPointer, IsFalse());
}

TEST_F(PropertyMetaInfo, cast_to_enumeration)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "MyEnum", TypeTraits::IsEnum);
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    Enumeration enumeration{"MyEnum.Foo"};
    auto value = QVariant::fromValue(enumeration);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<Enumeration>(enumeration));
}

TEST_F(PropertyMetaInfo, dont_to_cast_enumeration_if_property_type_is_not_enumeration)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "MyEnum", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    Enumeration enumeration{"MyEnum.Foo"};
    auto value = QVariant::fromValue(enumeration);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, QVariantIsValid(IsFalse()));
}

TEST_F(PropertyMetaInfo, dont_to_cast_enumeration_if_value_is_not_Enumeration)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "MyEnum", TypeTraits::IsEnum);
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(QString{"enumeration"});

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, QVariantIsValid(IsFalse()));
}

TEST_F(PropertyMetaInfo, cast_to_model_node)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "var", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(model.rootModelNode());

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, value);
}

TEST_F(PropertyMetaInfo, cast_to_qvariant_always_returns_the_save_variant_if_the_property_type_is_var)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "var", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(QString{"foo"});

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QString>("foo"));
}

TEST_F(PropertyMetaInfo, cast_double_to_double)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "double", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(14.2);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<double>(14.2));
}

TEST_F(PropertyMetaInfo, cast_int_to_double_returns_number_variant)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "double", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(14);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<double>(14.0));
}

TEST_F(PropertyMetaInfo, cast_default_to_double_returns_zero_variant)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "double", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant();

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<double>(0));
}

TEST_F(PropertyMetaInfo, cast_qstring_to_double_returns_zero_variant)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "double", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(QString{"foo"});

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<double>(0));
}

TEST_F(PropertyMetaInfo, cast_float_to_float)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML-cppnative", "float", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(14.2f);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<float>(14.2f));
}

TEST_F(PropertyMetaInfo, cast_int_to_float_returns_number_variant)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML-cppnative", "float", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(14);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<float>(14.0));
}

TEST_F(PropertyMetaInfo, cast_default_to_float_returns_zero_variant)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML-cppnative", "float", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant();

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<float>(0));
}

TEST_F(PropertyMetaInfo, cast_qstring_to_float_returns_zero_variant)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML-cppnative", "float", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(QString{"foo"});

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<float>(0));
}

TEST_F(PropertyMetaInfo, cast_int_to_int)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "int", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(14);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<int>(14));
}

TEST_F(PropertyMetaInfo, cast_double_to_int_returns_number_variant)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "int", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(14.2);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<int>(14));
}

TEST_F(PropertyMetaInfo, cast_default_to_int_returns_zero_variant)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "int", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant();

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<int>(0));
}

TEST_F(PropertyMetaInfo, cast_qstring_to_int_returns_zero_variant)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "int", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(QString{"foo"});

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<int>(0));
}

TEST_F(PropertyMetaInfo, cast_bool_to_bool)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "bool", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(true);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<bool>(true));
}

TEST_F(PropertyMetaInfo, cast_float_to_bool_returns_boolean_variant)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "bool", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(14.2f);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<bool>(true));
}

TEST_F(PropertyMetaInfo, cast_double_to_bool_returns_boolean_variant)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "bool", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(14.2);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<bool>(true));
}

TEST_F(PropertyMetaInfo, cast_int_to_bool_returns_boolean_variant)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "bool", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(14);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<bool>(true));
}

TEST_F(PropertyMetaInfo, cast_long_to_bool_returns_boolean_variant)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "bool", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(14L);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<bool>(true));
}

TEST_F(PropertyMetaInfo, cast_long_long_to_bool_returns_boolean_variant)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "bool", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(14LL);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<bool>(true));
}

TEST_F(PropertyMetaInfo, cast_default_to_bool_returns_zero_variant)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "bool", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant();

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<bool>(false));
}

TEST_F(PropertyMetaInfo, cast_qstring_to_bool_returns_zero_variant)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "bool", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(QString{"foo"});

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<bool>(false));
}

TEST_F(PropertyMetaInfo, cast_string_to_string)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "string", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(QString{"foo"});

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QString>("foo"));
}

TEST_F(PropertyMetaInfo, cast_QByteArray_to_empty_string)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "string", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(QByteArray{"foo"});

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QString>(IsEmpty()));
}

TEST_F(PropertyMetaInfo, cast_int_to_empty_string)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "string", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(14);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QString>(IsEmpty()));
}

TEST_F(PropertyMetaInfo, cast_default_to_empty_string)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "string", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant();

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QString>(IsEmpty()));
}

TEST_F(PropertyMetaInfo, cast_datatime_to_datetime)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "date", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto dataTime = QDateTime::currentDateTime();
    auto value = QVariant::fromValue(dataTime);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QDateTime>(dataTime));
}

TEST_F(PropertyMetaInfo, cast_int_to_datetime_returns_default_datetime)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "date", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(14);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QDateTime>(IsNull()));
}

TEST_F(PropertyMetaInfo, cast_string_to_datetime_returns_default_datetime)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "date", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(QString{"Monday"});

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QDateTime>(IsNull()));
}

TEST_F(PropertyMetaInfo, cast_default_to_datetime_returns_default_datetime)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "date", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant();

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QDateTime>(IsNull()));
}

TEST_F(PropertyMetaInfo, cast_url_to_url)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "url", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto url = QUrl("http://www.qt.io/future");
    auto value = QVariant::fromValue(url);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QUrl>(url));
}

TEST_F(PropertyMetaInfo, cast_string_to_empty_url)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "url", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant::fromValue(QString{"http://www.qt.io/future"});

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QUrl>(IsEmpty()));
}

TEST_F(PropertyMetaInfo, cast_default_to_empty_url)
{
    auto propertyTypeInfo = createNodeMetaInfo("QML", "url", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant();

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QUrl>(IsEmpty()));
}

TEST_F(PropertyMetaInfo, cast_color_to_color)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "color", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto color = QColor(Qt::red);
    auto value = QVariant(color);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QColor>(color));
}

TEST_F(PropertyMetaInfo, cast_string_to_null_color)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "color", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant("red");

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QColor>(Not(IsValid())));
}

TEST_F(PropertyMetaInfo, cast_int_to_null_color)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "color", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant(14);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QColor>(Not(IsValid())));
}

TEST_F(PropertyMetaInfo, cast_default_to_null_color)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "color", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant();

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QColor>(Not(IsValid())));
}

TEST_F(PropertyMetaInfo, cast_vector2d_to_vector2d)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "vector2d", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto vector2d = QVector2D{32.2f, 2.2f};
    auto value = QVariant(vector2d);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QVector2D>(vector2d));
}

TEST_F(PropertyMetaInfo, cast_string_to_vector2d_returns_an_empty_vector2d)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "vector2d", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant(QString{"foo"});

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QVector2D>(IsNull()));
}

TEST_F(PropertyMetaInfo, cast_int_to_vector2d_returns_an_empty_vector2d)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "vector2d", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant(12);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QVector2D>(IsNull()));
}

TEST_F(PropertyMetaInfo, cast_vector3d_to_vector2d_returns_an_empty_vector2d)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "vector2d", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant(QVector3D{32.2f, 2.2f, 784.f});

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QVector2D>(IsNull()));
}

TEST_F(PropertyMetaInfo, cast_default_to_vector2d_returns_an_empty_vector2d)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "vector2d", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant();

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QVector2D>(IsNull()));
}

TEST_F(PropertyMetaInfo, cast_vector3d_to_vector3d)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "vector3d", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto vector3d = QVector3D{32.2f, 2.2f, 44.4f};
    auto value = QVariant(vector3d);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QVector3D>(vector3d));
}

TEST_F(PropertyMetaInfo, cast_string_to_vector3d_returns_an_empty_vector3d)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "vector3d", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant(QString{"foo"});

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QVector3D>(IsNull()));
}

TEST_F(PropertyMetaInfo, cast_int_to_vector3d_returns_an_empty_vector3d)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "vector3d", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant(12);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QVector3D>(IsNull()));
}

TEST_F(PropertyMetaInfo, cast_vector4d_to_vector3d_returns_an_empty_vector3d)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "vector3d", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant(QVector4D{32.2f, 2.2f, 784.f, 99.f});

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QVector3D>(IsNull()));
}

TEST_F(PropertyMetaInfo, cast_default_to_vector3d_returns_an_empty_vector3d)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "vector3d", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant();

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QVector3D>(IsNull()));
}

TEST_F(PropertyMetaInfo, cast_vector4d_to_vector4d)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "vector4d", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto vector4d = QVector4D{32.2f, 2.2f, 44.4f, 23.f};
    auto value = QVariant(vector4d);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QVector4D>(vector4d));
}

TEST_F(PropertyMetaInfo, cast_string_to_vector4d_returns_an_empty_vector4d)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "vector4d", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant(QString{"foo"});

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QVector4D>(IsNull()));
}

TEST_F(PropertyMetaInfo, cast_int_to_vector4d_returns_an_empty_vector4d)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "vector4d", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant(12);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QVector4D>(IsNull()));
}

TEST_F(PropertyMetaInfo, cast_vector2d_to_vector4d_returns_an_empty_vector4d)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "vector4d", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant(QVector2D{32.2f, 2.2f});

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QVector4D>(IsNull()));
}

TEST_F(PropertyMetaInfo, cast_default_to_vector4d_returns_an_empty_vector4d)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "vector4d", {});
    projectStorageMock.createProperty(nodeInfo.id(), "bar", {}, propertyTypeInfo.id());
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant();

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, IsQVariant<QVector4D>(IsNull()));
}

TEST_F(PropertyMetaInfo, default_cast_to_invalid_variant)
{
    auto propertyInfo = QmlDesigner::PropertyMetaInfo{};
    auto value = QVariant(43);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, Not(IsValid()));
}

TEST_F(PropertyMetaInfo, not_existing_property_cast_returns_invalid_value)
{
    auto propertyTypeInfo = createNodeMetaInfo("QtQuick", "vector4d", {});
    auto propertyInfo = nodeInfo.property("bar");
    auto value = QVariant(43);

    auto castedValue = propertyInfo.castedValue(value);

    ASSERT_THAT(castedValue, Not(IsValid()));
}
} // namespace
