// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/projectstoragemock.h"
#include "../mocks/sourcepathcachemock.h"

#include <designercore/include/model.h>
#include <designercore/include/modelnode.h>
#include <designercore/include/nodemetainfo.h>

namespace {

using QmlDesigner::ModelNode;
using QmlDesigner::ModelNodes;

template<typename Matcher>
auto PropertyId(const Matcher &matcher)
{
    return Property(&QmlDesigner::PropertyMetaInfo::id, matcher);
}

class NodeMetaInfo : public testing::Test
{
protected:
    NiceMock<SourcePathCacheMockWithPaths> pathCache{"/path/foo.qml"};
    NiceMock<ProjectStorageMockWithQtQtuick> projectStorageMock{pathCache.sourceId};
    QmlDesigner::Model model{{projectStorageMock, pathCache},
                             "Item",
                             {QmlDesigner::Import::createLibraryImport("QML"),
                              QmlDesigner::Import::createLibraryImport("QtQuick"),
                              QmlDesigner::Import::createLibraryImport("QtQml.Models")},
                             QUrl::fromLocalFile(pathCache.path.toQString())};
    ModelNode rootNode = model.rootModelNode();
    ModelNode item = model.createModelNode("Item");
    ModelNode object = model.createModelNode("QtObject");
    QmlDesigner::NodeMetaInfo itemMetaInfo = item.metaInfo();
    QmlDesigner::NodeMetaInfo objectMetaInfo = object.metaInfo();
    QmlDesigner::TypeId intTypeId = projectStorageMock.typeId(projectStorageMock.moduleId("QML"),
                                                              "int",
                                                              QmlDesigner::Storage::Version{});
};

TEST_F(NodeMetaInfo, is_true_if_meta_info_exists)
{
    auto node = model.createModelNode("Item");
    auto metaInfo = node.metaInfo();

    auto isValid = bool(metaInfo);

    ASSERT_TRUE(isValid);
}

TEST_F(NodeMetaInfo, is_valid_if_meta_info_exists)
{
    auto node = model.createModelNode("Item");
    auto metaInfo = node.metaInfo();

    auto isValid = metaInfo.isValid();

    ASSERT_TRUE(isValid);
}

TEST_F(NodeMetaInfo, is_false_if_meta_info_not_exists)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto isValid = bool(metaInfo);

    ASSERT_FALSE(isValid);
}

TEST_F(NodeMetaInfo, default_is_false)
{
    auto isValid = bool(QmlDesigner::NodeMetaInfo{});

    ASSERT_FALSE(isValid);
}

TEST_F(NodeMetaInfo, is_invalid_if_meta_info_not_exists)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto isValid = metaInfo.isValid();

    ASSERT_FALSE(isValid);
}

TEST_F(NodeMetaInfo, default_is_invalid)
{
    auto isValid = QmlDesigner::NodeMetaInfo{}.isValid();

    ASSERT_FALSE(isValid);
}

TEST_F(NodeMetaInfo, item_is_based_on_object)
{
    bool isBasedOn = itemMetaInfo.isBasedOn(objectMetaInfo);

    ASSERT_TRUE(isBasedOn);
}

TEST_F(NodeMetaInfo, item_is_based_on_item)
{
    bool isBasedOn = itemMetaInfo.isBasedOn(itemMetaInfo);

    ASSERT_TRUE(isBasedOn);
}

TEST_F(NodeMetaInfo, object_is_no_based_on_item)
{
    bool isBasedOn = objectMetaInfo.isBasedOn(itemMetaInfo);

    ASSERT_FALSE(isBasedOn);
}

TEST_F(NodeMetaInfo, object_is_not_file_component)
{
    bool isFileComponent = objectMetaInfo.isFileComponent();

    ASSERT_FALSE(isFileComponent);
}

TEST_F(NodeMetaInfo, default_is_not_file_component)
{
    bool isFileComponent = QmlDesigner::NodeMetaInfo{}.isFileComponent();

    ASSERT_FALSE(isFileComponent);
}

TEST_F(NodeMetaInfo, invalid_is_not_file_component)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    bool isFileComponent = metaInfo.isFileComponent();

    ASSERT_FALSE(isFileComponent);
}

TEST_F(NodeMetaInfo, component_is_file_component)
{
    using QmlDesigner::Storage::TypeTraits;
    auto moduleId = projectStorageMock.createModule("/path/to/project");
    auto typeId = projectStorageMock.createType(moduleId,
                                                "Foo",
                                                TypeTraits::IsFileComponent | TypeTraits::Reference);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    bool isFileComponent = metaInfo.isFileComponent();

    ASSERT_TRUE(isFileComponent);
}

TEST_F(NodeMetaInfo, is_project_component)
{
    using QmlDesigner::Storage::TypeTraits;
    auto moduleId = projectStorageMock.createModule("/path/to/project");
    auto typeId = projectStorageMock.createType(moduleId, "Foo", TypeTraits::IsProjectComponent);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    bool isProjectComponent = metaInfo.isProjectComponent();

    ASSERT_TRUE(isProjectComponent);
}

TEST_F(NodeMetaInfo, is_not_project_component)
{
    using QmlDesigner::Storage::TypeTraits;
    auto moduleId = projectStorageMock.createModule("/path/to/project");
    auto typeId = projectStorageMock.createType(moduleId, "Foo", {});
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    bool isProjectComponent = metaInfo.isProjectComponent();

    ASSERT_FALSE(isProjectComponent);
}

TEST_F(NodeMetaInfo, invalid_is_not_project_component)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isProjectComponent = metaInfo.isProjectComponent();

    ASSERT_FALSE(isProjectComponent);
}

TEST_F(NodeMetaInfo, is_in_project_module)
{
    using QmlDesigner::Storage::TypeTraits;
    auto moduleId = projectStorageMock.createModule("/path/to/project");
    auto typeId = projectStorageMock.createType(moduleId, "Foo", TypeTraits::IsInProjectModule);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    bool isInProjectModule = metaInfo.isInProjectModule();

    ASSERT_TRUE(isInProjectModule);
}

TEST_F(NodeMetaInfo, is_not_in_project_module)
{
    using QmlDesigner::Storage::TypeTraits;
    auto moduleId = projectStorageMock.createModule("/path/to/project");
    auto typeId = projectStorageMock.createType(moduleId, "Foo", {});
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    bool isInProjectModule = metaInfo.isInProjectModule();

    ASSERT_FALSE(isInProjectModule);
}

TEST_F(NodeMetaInfo, invalid_is_not_in_project_module)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isInProjectModule = metaInfo.isInProjectModule();

    ASSERT_FALSE(isInProjectModule);
}

TEST_F(NodeMetaInfo, has_property)
{
    auto node = model.createModelNode("Item");
    auto metaInfo = node.metaInfo();
    projectStorageMock.createProperty(metaInfo.id(), "x", intTypeId);

    bool hasProperty = metaInfo.hasProperty("x");

    ASSERT_TRUE(hasProperty);
}

TEST_F(NodeMetaInfo, has_not_property)
{
    auto node = model.createModelNode("Item");
    auto metaInfo = node.metaInfo();

    bool hasProperty = metaInfo.hasProperty("foo");

    ASSERT_FALSE(hasProperty);
}

TEST_F(NodeMetaInfo, default_has_not_property)
{
    auto metaInfo = QmlDesigner::NodeMetaInfo{};

    bool hasProperty = metaInfo.hasProperty("x");

    ASSERT_FALSE(hasProperty);
}

TEST_F(NodeMetaInfo, invalid_has_not_property)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    bool hasProperty = metaInfo.hasProperty("x");

    ASSERT_FALSE(hasProperty);
}

TEST_F(NodeMetaInfo, get_property)
{
    auto node = model.createModelNode("Item");
    auto metaInfo = node.metaInfo();
    auto propertyId = projectStorageMock.createProperty(metaInfo.id(), "x", intTypeId);

    auto property = metaInfo.property("x");

    ASSERT_THAT(property, PropertyId(propertyId));
}

TEST_F(NodeMetaInfo, get_invalid_property_if_not_exists)
{
    auto node = model.createModelNode("Item");
    auto metaInfo = node.metaInfo();

    auto property = metaInfo.property("x");

    ASSERT_THAT(property, PropertyId(IsFalse()));
}

TEST_F(NodeMetaInfo, get_invalid_property_for_default)
{
    auto metaInfo = QmlDesigner::NodeMetaInfo{};

    auto property = metaInfo.property("x");

    ASSERT_THAT(property, PropertyId(IsFalse()));
}

TEST_F(NodeMetaInfo, get_invalid_property_if_meta_info_is_invalid)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto property = metaInfo.property("x");

    ASSERT_THAT(property, PropertyId(IsFalse()));
}

TEST_F(NodeMetaInfo, get_properties)
{
    auto node = model.createModelNode("Item");
    auto metaInfo = node.metaInfo();
    auto xPropertyId = projectStorageMock.createProperty(metaInfo.id(), "x", intTypeId);
    auto yPropertyId = projectStorageMock.createProperty(metaInfo.id(), "y", intTypeId);

    auto properties = metaInfo.properties();

    ASSERT_THAT(properties, IsSupersetOf({PropertyId(xPropertyId), PropertyId(yPropertyId)}));
}

TEST_F(NodeMetaInfo, get_no_properties_for_default)
{
    auto metaInfo = QmlDesigner::NodeMetaInfo{};

    auto properties = metaInfo.properties();

    ASSERT_THAT(properties, IsEmpty());
}

TEST_F(NodeMetaInfo, get_no_properties_if_is_invalid)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto properties = metaInfo.properties();

    ASSERT_THAT(properties, IsEmpty());
}

TEST_F(NodeMetaInfo, get_local_properties)
{
    auto node = model.createModelNode("Item");
    auto metaInfo = node.metaInfo();
    auto xPropertyId = projectStorageMock.createProperty(metaInfo.id(), "x", intTypeId);
    auto yPropertyId = projectStorageMock.createProperty(metaInfo.id(), "y", intTypeId);

    auto properties = metaInfo.localProperties();

    ASSERT_THAT(properties, IsSupersetOf({PropertyId(xPropertyId), PropertyId(yPropertyId)}));
}

TEST_F(NodeMetaInfo, get_no_local_properties_for_default_metainfo)
{
    auto metaInfo = QmlDesigner::NodeMetaInfo{};

    auto properties = metaInfo.localProperties();

    ASSERT_THAT(properties, IsEmpty());
}

TEST_F(NodeMetaInfo, get_no_local_properties_if_node_is_invalid)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto properties = metaInfo.localProperties();

    ASSERT_THAT(properties, IsEmpty());
}

TEST_F(NodeMetaInfo, get_signal_names)
{
    auto node = model.createModelNode("Item");
    auto metaInfo = node.metaInfo();
    projectStorageMock.createSignal(metaInfo.id(), "xChanged");
    projectStorageMock.createSignal(metaInfo.id(), "yChanged");

    auto signalNames = metaInfo.signalNames();

    ASSERT_THAT(signalNames, IsSupersetOf({"xChanged", "yChanged"}));
}

TEST_F(NodeMetaInfo, get_no_signal_names_for_default_metainfo)
{
    auto metaInfo = QmlDesigner::NodeMetaInfo{};

    auto signalNames = metaInfo.signalNames();

    ASSERT_THAT(signalNames, IsEmpty());
}

TEST_F(NodeMetaInfo, get_no_signal_names_if_node_is_invalid)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto signalNames = metaInfo.signalNames();

    ASSERT_THAT(signalNames, IsEmpty());
}

TEST_F(NodeMetaInfo, get_function_names)
{
    auto node = model.createModelNode("Item");
    auto metaInfo = node.metaInfo();
    projectStorageMock.createFunction(metaInfo.id(), "setX");
    projectStorageMock.createFunction(metaInfo.id(), "setY");

    auto functionNames = metaInfo.slotNames();

    ASSERT_THAT(functionNames, IsSupersetOf({"setX", "setY"}));
}

TEST_F(NodeMetaInfo, get_no_function_names_for_default_metainfo)
{
    auto metaInfo = QmlDesigner::NodeMetaInfo{};

    auto functionNames = metaInfo.slotNames();

    ASSERT_THAT(functionNames, IsEmpty());
}

TEST_F(NodeMetaInfo, get_no_function_names_if_node_is_invalid)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto functionNames = metaInfo.slotNames();

    ASSERT_THAT(functionNames, IsEmpty());
}

TEST_F(NodeMetaInfo, get_default_property)
{
    auto node = model.createModelNode("Item");
    auto metaInfo = node.metaInfo();
    auto defaultProperty = metaInfo.property("data");

    auto property = metaInfo.defaultProperty();

    ASSERT_THAT(property, defaultProperty);
}

TEST_F(NodeMetaInfo, get_no_default_property_for_default_metainfo)
{
    auto metaInfo = QmlDesigner::NodeMetaInfo{};

    auto property = metaInfo.defaultProperty();

    ASSERT_THAT(property, IsFalse());
}

TEST_F(NodeMetaInfo, get_no_default_property_if_node_is_invalid)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto property = metaInfo.defaultProperty();

    ASSERT_THAT(property, IsFalse());
}

TEST_F(NodeMetaInfo, get_default_property_name)
{
    auto node = model.createModelNode("Item");
    auto metaInfo = node.metaInfo();
    auto defaultProperty = metaInfo.property("data");

    auto property = metaInfo.defaultPropertyName();

    ASSERT_THAT(property, "data");
}

TEST_F(NodeMetaInfo, get_no_default_property_name_for_default_metainfo)
{
    auto metaInfo = QmlDesigner::NodeMetaInfo{};

    auto property = metaInfo.defaultPropertyName();

    ASSERT_THAT(property, IsEmpty());
}

TEST_F(NodeMetaInfo, get_no_default_property_name_if_node_is_invalid)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto property = metaInfo.defaultPropertyName();

    ASSERT_THAT(property, IsEmpty());
}

} // namespace
