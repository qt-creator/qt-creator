// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <matchers/info_exportedtypenames-matcher.h>
#include <matchers/projectstorage-matcher.h>
#include <mocks/projectstoragemock.h>
#include <mocks/sourcepathcachemock.h>

#include <designercore/include/model.h>
#include <designercore/include/modelnode.h>
#include <designercore/include/nodemetainfo.h>

namespace {

using QmlDesigner::FlagIs;
using QmlDesigner::ModelNode;
using QmlDesigner::ModelNodes;
using QmlDesigner::Storage::ModuleKind;
using QmlDesigner::Storage::TypeTraits;
using QmlDesigner::Storage::TypeTraitsKind;

template<typename Matcher>
auto PropertyId(const Matcher &matcher)
{
    return Property(&QmlDesigner::PropertyMetaInfo::id, matcher);
}

template<typename PropertyMatcher, typename ParentPropertyMatcher, typename NameMatcher>
auto CompoundProperty(const PropertyMatcher &propertyMatcher,
                      const ParentPropertyMatcher &parentPropertyMatcher,
                      const NameMatcher &nameMatcher)
{
    return AllOf(Field(&QmlDesigner::CompoundPropertyMetaInfo::property, propertyMatcher),
                 Field(&QmlDesigner::CompoundPropertyMetaInfo::parent, parentPropertyMatcher),
                 Property(&QmlDesigner::CompoundPropertyMetaInfo::name, nameMatcher));
}

template<typename PropertyIdMatcher, typename ParentPropertyIdMatcher, typename NameMatcher>
auto CompoundPropertyIds(const PropertyIdMatcher &propertyIdMatcher,
                         const ParentPropertyIdMatcher &parentPropertyIdMatcher,
                         const NameMatcher &nameMatcher)
{
    return CompoundProperty(PropertyId(propertyIdMatcher),
                            PropertyId(parentPropertyIdMatcher),
                            nameMatcher);
}

class NodeMetaInfo : public testing::Test
{
protected:
    QmlDesigner::TypeId createDerivedDummy(QmlDesigner::TypeId baseTypeId)
    {
        return projectStorageMock.createType(qmlModuleId, "Dummy", {}, {baseTypeId});
    }

    QmlDesigner::NodeMetaInfo createDerivedDummyMetaInfo(QmlDesigner::TypeId baseTypeId)
    {
        return QmlDesigner::NodeMetaInfo{createDerivedDummy(baseTypeId), &projectStorageMock};
    }

    QmlDesigner::NodeMetaInfo createDerivedDummyMetaInfo(QmlDesigner::ModuleId moduleId,
                                                         Utils::SmallStringView typeName)
    {
        auto typeId = projectStorageMock.createType(moduleId, typeName, {});

        return createDerivedDummyMetaInfo(typeId);
    }

    QmlDesigner::NodeMetaInfo createDerivedDummyMetaInfo(Utils::SmallStringView moduleName,
                                                         ModuleKind moduleKind,
                                                         Utils::SmallStringView typeName)
    {
        auto moduleId = projectStorageMock.createModule(moduleName, moduleKind);
        auto typeId = projectStorageMock.createType(moduleId, typeName, {});

        return createDerivedDummyMetaInfo(typeId);
    }

    QmlDesigner::NodeMetaInfo createMetaInfo(QmlDesigner::ModuleId moduleId,
                                             Utils::SmallStringView typeName,
                                             QmlDesigner::Storage::TypeTraits typeTraits = {})
    {
        auto typeId = projectStorageMock.createType(moduleId, typeName, typeTraits);

        return QmlDesigner::NodeMetaInfo{typeId, &projectStorageMock};
    }

    QmlDesigner::NodeMetaInfo createMetaInfo(Utils::SmallStringView moduleName,
                                             ModuleKind moduleKind,
                                             Utils::SmallStringView typeName,
                                             QmlDesigner::Storage::TypeTraits typeTraits = {})
    {
        auto moduleId = projectStorageMock.createModule(moduleName, moduleKind);
        auto typeId = projectStorageMock.createType(moduleId, typeName, typeTraits);

        return QmlDesigner::NodeMetaInfo{typeId, &projectStorageMock};
    }

protected:
    NiceMock<SourcePathCacheMockWithPaths> pathCache{"/path/foo.qml"};
    NiceMock<ProjectStorageMockWithQtQuick> projectStorageMock{pathCache.sourceId, "/path"};
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
    QmlDesigner::ModuleId qmlModuleId = projectStorageMock.createModule("QML", ModuleKind::QmlLibrary);
    QmlDesigner::ModuleId qtQuickModuleId = projectStorageMock.createModule("QtQuick",
                                                                            ModuleKind::QmlLibrary);
    QmlDesigner::TypeId intTypeId = projectStorageMock.typeId(qmlModuleId,
                                                              "int",
                                                              QmlDesigner::Storage::Version{});
};

TEST_F(NodeMetaInfo, is_true_if_meta_info_exists)
{
    auto metaInfo = model.qtQuickItemMetaInfo();

    auto isValid = bool(metaInfo);

    ASSERT_TRUE(isValid);
}

TEST_F(NodeMetaInfo, is_valid_if_meta_info_exists)
{
    auto metaInfo = model.qtQuickItemMetaInfo();

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
    auto moduleId = projectStorageMock.createModule("/path/to/project", ModuleKind::PathLibrary);
    TypeTraits traits{TypeTraitsKind::Reference};
    traits.isFileComponent = true;
    auto typeId = projectStorageMock.createType(moduleId, "Foo", traits);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    bool isFileComponent = metaInfo.isFileComponent();

    ASSERT_TRUE(isFileComponent);
}

TEST_F(NodeMetaInfo, has_property)
{
    auto metaInfo = model.qtQuickItemMetaInfo();
    projectStorageMock.createProperty(metaInfo.id(), "x", intTypeId);

    bool hasProperty = metaInfo.hasProperty("x");

    ASSERT_TRUE(hasProperty);
}

TEST_F(NodeMetaInfo, has_not_property)
{
    auto metaInfo = model.qtQuickItemMetaInfo();

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

TEST_F(NodeMetaInfo, has_dot_property)
{
    auto metaInfo = model.qtQuickItemMetaInfo();
    projectStorageMock.createProperty(metaInfo.id(), "parent", itemMetaInfo.id());

    bool hasProperty = metaInfo.hasProperty("parent.data");

    ASSERT_TRUE(hasProperty);
}

TEST_F(NodeMetaInfo, has_no_dot_property)
{
    auto metaInfo = model.qtQuickItemMetaInfo();

    bool hasProperty = metaInfo.hasProperty("foo.data");

    ASSERT_FALSE(hasProperty);
}

TEST_F(NodeMetaInfo, has_no_dot_property_for_last_entry)
{
    auto metaInfo = model.qtQuickItemMetaInfo();
    projectStorageMock.createProperty(metaInfo.id(), "parent", metaInfo.id());

    bool hasProperty = metaInfo.hasProperty("parent.foo");

    ASSERT_FALSE(hasProperty);
}

TEST_F(NodeMetaInfo, has_dot_dot_property)
{
    auto metaInfo = model.qtQuickItemMetaInfo();
    projectStorageMock.createProperty(metaInfo.id(), "parent", metaInfo.id());

    bool hasProperty = metaInfo.hasProperty("parent.parent.data");

    ASSERT_TRUE(hasProperty);
}

TEST_F(NodeMetaInfo, has_no_dot_dot_property)
{
    auto metaInfo = model.qtQuickItemMetaInfo();

    bool hasProperty = metaInfo.hasProperty("parent.foo.data");

    ASSERT_FALSE(hasProperty);
}

TEST_F(NodeMetaInfo, has_no_dot_dot_property_for_last_entry)
{
    auto metaInfo = model.qtQuickItemMetaInfo();
    projectStorageMock.createProperty(metaInfo.id(), "parent", metaInfo.id());

    bool hasProperty = metaInfo.hasProperty("parent.parent.foo");

    ASSERT_FALSE(hasProperty);
}

TEST_F(NodeMetaInfo, has_no_dot_dot_dot_property)
{
    auto metaInfo = model.qtQuickItemMetaInfo();
    projectStorageMock.createProperty(metaInfo.id(), "parent", metaInfo.id());

    bool hasProperty = metaInfo.hasProperty("parent.parent.parent.data");

    ASSERT_FALSE(hasProperty);
}

TEST_F(NodeMetaInfo, get_property)
{
    auto metaInfo = model.qtQuickItemMetaInfo();
    auto propertyId = projectStorageMock.createProperty(metaInfo.id(), "x", intTypeId);

    auto property = metaInfo.property("x");

    ASSERT_THAT(property, PropertyId(propertyId));
}

TEST_F(NodeMetaInfo, get_invalid_property_if_not_exists)
{
    auto metaInfo = model.qtQuickItemMetaInfo();

    auto property = metaInfo.property("foo");

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

TEST_F(NodeMetaInfo, get_dot_property)
{
    auto metaInfo = model.qtQuickItemMetaInfo();
    auto propertyId = metaInfo.property("data").id();
    projectStorageMock.createProperty(metaInfo.id(), "parent", metaInfo.id());

    auto property = metaInfo.property("parent.data");

    ASSERT_THAT(property, PropertyId(propertyId));
}

TEST_F(NodeMetaInfo, get_no_dot_property)
{
    auto metaInfo = model.qtQuickItemMetaInfo();

    auto property = metaInfo.property("foo.data");

    ASSERT_THAT(property, PropertyId(IsFalse()));
}

TEST_F(NodeMetaInfo, get_no_dot_property_for_last_entry)
{
    auto metaInfo = model.qtQuickItemMetaInfo();
    projectStorageMock.createProperty(metaInfo.id(), "parent", metaInfo.id());

    auto property = metaInfo.property("parent.foo");

    ASSERT_THAT(property, PropertyId(IsFalse()));
}

TEST_F(NodeMetaInfo, get_no_dot_dot_dot_property)
{
    auto metaInfo = model.qtQuickItemMetaInfo();
    projectStorageMock.createProperty(metaInfo.id(), "parent", metaInfo.id());

    auto property = metaInfo.property("parent.parent.parent.data");

    ASSERT_THAT(property, PropertyId(IsFalse()));
}

TEST_F(NodeMetaInfo, get_properties)
{
    auto metaInfo = model.qtQuickItemMetaInfo();
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

TEST_F(NodeMetaInfo, inflate_value_properties)
{
    auto metaInfo = model.qtQuickItemMetaInfo();
    auto fontTypeId = projectStorageMock.typeId(qtQuickModuleId, "font");
    auto xPropertyId = projectStorageMock.createProperty(metaInfo.id(), "x", intTypeId);
    auto fontPropertyId = projectStorageMock.createProperty(metaInfo.id(), "font", fontTypeId);
    auto familyPropertyId = projectStorageMock.propertyDeclarationId(fontTypeId, "family");
    auto pixelSizePropertyId = projectStorageMock.propertyDeclarationId(fontTypeId, "pixelSize");

    auto properties = QmlDesigner::MetaInfoUtils::inflateValueProperties(metaInfo.properties());

    ASSERT_THAT(properties,
                AllOf(Contains(CompoundPropertyIds(xPropertyId, IsFalse(), "x")),
                      Not(Contains(CompoundPropertyIds(fontPropertyId, IsFalse(), _))),
                      Contains(CompoundPropertyIds(familyPropertyId, fontPropertyId, "font.family")),
                      Contains(
                          CompoundPropertyIds(pixelSizePropertyId, fontPropertyId, "font.pixelSize"))));
}

TEST_F(NodeMetaInfo, inflate_value_properties_handles_invalid)
{
    QmlDesigner::PropertyMetaInfos propertiesWithInvalid = {QmlDesigner::PropertyMetaInfo{}};

    auto properties = QmlDesigner::MetaInfoUtils::inflateValueProperties(propertiesWithInvalid);

    ASSERT_THAT(properties, ElementsAre(CompoundProperty(IsFalse(), IsFalse(), IsEmpty())));
}

TEST_F(NodeMetaInfo, inflate_value_and_readonly_properties)
{
    using QmlDesigner::Storage::PropertyDeclarationTraits;
    auto metaInfo = model.qtQuickItemMetaInfo();
    auto fontTypeId = projectStorageMock.typeId(qtQuickModuleId, "font");
    auto inputDeviceId = projectStorageMock.typeId(qtQuickModuleId, "InputDevice");
    auto xPropertyId = projectStorageMock.createProperty(metaInfo.id(), "x", intTypeId);
    auto fontPropertyId = projectStorageMock.createProperty(metaInfo.id(), "font", fontTypeId);
    auto familyPropertyId = projectStorageMock.propertyDeclarationId(fontTypeId, "family");
    auto pixelSizePropertyId = projectStorageMock.propertyDeclarationId(fontTypeId, "pixelSize");
    auto devicePropertyId = projectStorageMock.createProperty(metaInfo.id(),
                                                              "device",
                                                              PropertyDeclarationTraits::IsReadOnly,
                                                              inputDeviceId);
    auto seatNamePropertyId = projectStorageMock.propertyDeclarationId(inputDeviceId, "seatName");

    auto properties = QmlDesigner::MetaInfoUtils::inflateValueAndReadOnlyProperties(
        metaInfo.properties());

    ASSERT_THAT(
        properties,
        AllOf(Contains(CompoundPropertyIds(xPropertyId, IsFalse(), "x")),
              Not(Contains(CompoundPropertyIds(fontPropertyId, IsFalse(), _))),
              Contains(CompoundPropertyIds(familyPropertyId, fontPropertyId, "font.family")),
              Contains(CompoundPropertyIds(pixelSizePropertyId, fontPropertyId, "font.pixelSize")),
              Contains(CompoundPropertyIds(seatNamePropertyId, devicePropertyId, "device.seatName"))));
}

TEST_F(NodeMetaInfo, inflate_value_and_readonly_properties_handles_invalid)
{
    QmlDesigner::PropertyMetaInfos propertiesWithInvalid = {QmlDesigner::PropertyMetaInfo{}};

    auto properties = QmlDesigner::MetaInfoUtils::inflateValueAndReadOnlyProperties(
        propertiesWithInvalid);

    ASSERT_THAT(properties, ElementsAre(CompoundProperty(IsFalse(), IsFalse(), IsEmpty())));
}

TEST_F(NodeMetaInfo, get_local_properties)
{
    auto metaInfo = model.qtQuickItemMetaInfo();
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
    auto metaInfo = model.qtQuickItemMetaInfo();
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
    auto metaInfo = model.qtQuickItemMetaInfo();
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
    auto metaInfo = model.qtQuickItemMetaInfo();
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
    auto metaInfo = model.qtQuickItemMetaInfo();
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

TEST_F(NodeMetaInfo, has_default_property)
{
    auto metaInfo = model.qtQuickItemMetaInfo();

    bool property = metaInfo.hasDefaultProperty();

    ASSERT_THAT(property, IsTrue());
}

TEST_F(NodeMetaInfo, has_no_default_property)
{
    auto metaInfo = model.qtQmlModelsListElementMetaInfo();

    bool property = metaInfo.hasDefaultProperty();

    ASSERT_THAT(property, IsFalse());
}

TEST_F(NodeMetaInfo, has_no_default_property_for_default_metainfo)
{
    auto metaInfo = QmlDesigner::NodeMetaInfo{};

    bool property = metaInfo.hasDefaultProperty();

    ASSERT_THAT(property, IsFalse());
}

TEST_F(NodeMetaInfo, has_no_default_property_if_node_is_invalid)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    bool property = metaInfo.hasDefaultProperty();

    ASSERT_THAT(property, IsFalse());
}

TEST_F(NodeMetaInfo, self_and_prototypes)
{
    auto metaInfo = model.flowViewFlowActionAreaMetaInfo();

    auto prototypes = metaInfo.selfAndPrototypes();

    ASSERT_THAT(prototypes,
                ElementsAre(model.flowViewFlowActionAreaMetaInfo(),
                            model.qtQuickItemMetaInfo(),
                            model.qmlQtObjectMetaInfo()));
}

TEST_F(NodeMetaInfo, self_and_prototypes_returns_empty_container_for_default)
{
    auto metaInfo = QmlDesigner::NodeMetaInfo();

    auto prototypes = metaInfo.selfAndPrototypes();

    ASSERT_THAT(prototypes, IsEmpty());
}

TEST_F(NodeMetaInfo, prototypes)
{
    auto metaInfo = model.flowViewFlowActionAreaMetaInfo();

    auto prototypes = metaInfo.prototypes();

    ASSERT_THAT(prototypes, ElementsAre(model.qtQuickItemMetaInfo(), model.qmlQtObjectMetaInfo()));
}

TEST_F(NodeMetaInfo, prototypes_returns_empty_container_for_default)
{
    auto metaInfo = QmlDesigner::NodeMetaInfo();

    auto prototypes = metaInfo.prototypes();

    ASSERT_THAT(prototypes, IsEmpty());
}

TEST_F(NodeMetaInfo, heirs)
{
    auto metaInfo = model.qmlQtObjectMetaInfo();
    projectStorageMock.setHeirs(metaInfo.id(), {model.qtQuickItemMetaInfo().id()});

    auto heirs = metaInfo.heirs();

    ASSERT_THAT(heirs, ElementsAre(model.qtQuickItemMetaInfo()));
}

TEST_F(NodeMetaInfo, heirs_returns_empty_container_for_default)
{
    auto metaInfo = QmlDesigner::NodeMetaInfo();

    auto heirs = metaInfo.heirs();

    ASSERT_THAT(heirs, IsEmpty());
}

TEST_F(NodeMetaInfo, common_base_is_root)
{
    auto metaInfo = model.flowViewFlowActionAreaMetaInfo();

    auto commonBase = metaInfo.commonBase(model.qtQuickPropertyAnimationMetaInfo());

    ASSERT_THAT(commonBase, model.qmlQtObjectMetaInfo());
}

TEST_F(NodeMetaInfo, common_base_is_first_leaf)
{
    auto metaInfo = model.qtQuickItemMetaInfo();

    auto commonBase = metaInfo.commonBase(model.flowViewFlowActionAreaMetaInfo());

    ASSERT_THAT(commonBase, model.qtQuickItemMetaInfo());
}

TEST_F(NodeMetaInfo, common_base_is_second_leaf)
{
    auto metaInfo = model.flowViewFlowActionAreaMetaInfo();

    auto commonBase = metaInfo.commonBase(model.qtQuickItemMetaInfo());

    ASSERT_THAT(commonBase, model.qtQuickItemMetaInfo());
}

TEST_F(NodeMetaInfo, there_is_no_common_base)
{
    auto metaInfo = model.metaInfo("int");

    auto commonBase = metaInfo.commonBase(model.qtQuickItemMetaInfo());

    ASSERT_THAT(commonBase, IsFalse());
}

TEST_F(NodeMetaInfo, first_input_is_invalid_for_common_base_returns_invalid)
{
    auto metaInfo = QmlDesigner::NodeMetaInfo();

    auto commonBase = metaInfo.commonBase(model.flowViewFlowActionAreaMetaInfo());

    ASSERT_THAT(commonBase, IsFalse());
}

TEST_F(NodeMetaInfo, second_input_is_invalid_for_common_base_returns_invalid)
{
    auto metaInfo = model.flowViewFlowActionAreaMetaInfo();

    auto commonBase = metaInfo.commonBase(QmlDesigner::NodeMetaInfo());

    ASSERT_THAT(commonBase, IsFalse());
}

TEST_F(NodeMetaInfo, source_id)
{
    auto moduleId = projectStorageMock.createModule("/path/to/project", ModuleKind::PathLibrary);
    auto typeSourceId = QmlDesigner::SourceId::create(999);
    auto typeId = projectStorageMock.createType(moduleId, "Foo", {}, {}, typeSourceId);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    auto sourceId = metaInfo.sourceId();

    ASSERT_THAT(sourceId, typeSourceId);
}

TEST_F(NodeMetaInfo, invalid_returns_invalid_source_id)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    auto sourceId = metaInfo.sourceId();

    ASSERT_THAT(sourceId, IsFalse());
}

TEST_F(NodeMetaInfo, is_bool)
{
    auto metaInfo = createMetaInfo(qmlModuleId, "bool");

    bool isType = metaInfo.isBool();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_bool)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isBool();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_color)
{
    auto metaInfo = createMetaInfo(qtQuickModuleId, "color");

    bool isType = metaInfo.isColor();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_color)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isColor();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, float_is_a_floating_type)
{
    auto metaInfo = createMetaInfo("QML", ModuleKind::CppLibrary, "float");

    bool isType = metaInfo.isFloat();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, double_is_a_floating_type)
{
    auto metaInfo = createMetaInfo(qmlModuleId, "double");

    bool isType = metaInfo.isFloat();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_float)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isFloat();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_FlowView_FlowActionArea)
{
    auto metaInfo = createDerivedDummyMetaInfo("FlowView", ModuleKind::QmlLibrary, "FlowActionArea");

    bool isType = metaInfo.isFlowViewFlowActionArea();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_FlowView_FlowActionArea)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isFlowViewFlowActionArea();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_FlowView_FlowDecision)
{
    auto metaInfo = createDerivedDummyMetaInfo("FlowView", ModuleKind::QmlLibrary, "FlowDecision");

    bool isType = metaInfo.isFlowViewFlowDecision();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_FlowView_FlowDecision)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isFlowViewFlowDecision();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_FlowView_FlowItem)
{
    auto metaInfo = createDerivedDummyMetaInfo("FlowView", ModuleKind::QmlLibrary, "FlowItem");

    bool isType = metaInfo.isFlowViewFlowItem();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_FlowView_FlowItem)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isFlowViewFlowItem();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_FlowView_FlowTransition)
{
    auto metaInfo = createDerivedDummyMetaInfo("FlowView", ModuleKind::QmlLibrary, "FlowTransition");

    bool isType = metaInfo.isFlowViewFlowTransition();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_FlowView_FlowTransition)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isFlowViewFlowTransition();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_FlowView_FlowView)
{
    auto metaInfo = createDerivedDummyMetaInfo("FlowView", ModuleKind::QmlLibrary, "FlowView");

    bool isType = metaInfo.isFlowViewFlowView();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_FlowView_FlowView)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isFlowViewFlowView();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_FlowView_FlowWildcard)
{
    auto metaInfo = createDerivedDummyMetaInfo("FlowView", ModuleKind::QmlLibrary, "FlowWildcard");

    bool isType = metaInfo.isFlowViewFlowWildcard();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_FlowView_FlowWildcard)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isFlowViewFlowWildcard();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, FlowItem_is_FlowView_item)
{
    auto metaInfo = createDerivedDummyMetaInfo("FlowView", ModuleKind::QmlLibrary, "FlowItem");

    bool isType = metaInfo.isFlowViewItem();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, FlowWildcard_is_FlowView_item)
{
    auto metaInfo = createDerivedDummyMetaInfo("FlowView", ModuleKind::QmlLibrary, "FlowWildcard");

    bool isType = metaInfo.isFlowViewItem();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, FlowDecision_is_FlowView_item)
{
    auto metaInfo = createDerivedDummyMetaInfo("FlowView", ModuleKind::QmlLibrary, "FlowDecision");

    bool isType = metaInfo.isFlowViewItem();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_FlowView_Item)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isFlowViewItem();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_font)
{
    auto metaInfo = createMetaInfo(qtQuickModuleId, "font");

    bool isType = metaInfo.isFont();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_font)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isFont();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, QtQuick_Item_is_graphical_item)
{
    auto metaInfo = createDerivedDummyMetaInfo(qtQuickModuleId, "Item");

    bool isType = metaInfo.isGraphicalItem();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, QtQuickWindow_Window_is_graphical_item)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick.Window", ModuleKind::QmlLibrary, "Window");

    bool isType = metaInfo.isGraphicalItem();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, QtQuickDialogs_Dialogs_is_graphical_item)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick.Dialogs", ModuleKind::QmlLibrary, "Dialog");

    bool isType = metaInfo.isGraphicalItem();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, QtQuickControls_Popup_is_graphical_item)
{
    auto metaInfo = createMetaInfo("QtQuick.Controls", ModuleKind::QmlLibrary, "Popup");

    bool isType = metaInfo.isGraphicalItem();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_graphical_item)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isGraphicalItem();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_integer)
{
    auto metaInfo = createMetaInfo(qmlModuleId, "int");

    bool isType = metaInfo.isInteger();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_integer)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isInteger();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, QtQuick_Positioner_is_layoutable)
{
    auto metaInfo = createDerivedDummyMetaInfo(qtQuickModuleId, "Positioner");

    bool isType = metaInfo.isLayoutable();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, QtQuick_Layouts_Layout_is_layoutable)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick.Layouts", ModuleKind::QmlLibrary, "Layout");

    bool isType = metaInfo.isLayoutable();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, QtQuick_Controls_SplitView_is_layoutable)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick.Controls", ModuleKind::QmlLibrary, "SplitView");

    bool isType = metaInfo.isLayoutable();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_layoutable)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isLayoutable();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, ListView_is_ListView_or_GridView)
{
    auto metaInfo = createDerivedDummyMetaInfo(qtQuickModuleId, "ListView");

    bool isType = metaInfo.isListOrGridView();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, GridView_is_ListView_or_GridView)
{
    auto metaInfo = createDerivedDummyMetaInfo(qtQuickModuleId, "GridView");

    bool isType = metaInfo.isListOrGridView();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_ListView_or_GridView)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isListOrGridView();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_qml_component)
{
    auto metaInfo = createDerivedDummyMetaInfo(qmlModuleId, "Component");

    bool isType = metaInfo.isQmlComponent();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_qml_component)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQmlComponent();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtMultimedia_SoundEffect)
{
    auto qtMultimediaModuleId = projectStorageMock.createModule("QtMultimedia",
                                                                ModuleKind::QmlLibrary);
    auto metaInfo = createDerivedDummyMetaInfo(qtMultimediaModuleId, "SoundEffect");

    bool isType = metaInfo.isQtMultimediaSoundEffect();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtMultimedia_SoundEffect)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtMultimediaSoundEffect();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtObject)
{
    auto metaInfo = createDerivedDummyMetaInfo(qmlModuleId, "QtObject");

    bool isType = metaInfo.isQtObject();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtObject)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtObject();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_BakedLightmap)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D", ModuleKind::QmlLibrary, "BakedLightmap");

    bool isType = metaInfo.isQtQuick3DBakedLightmap();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_BakedLightmap)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DBakedLightmap();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_Camera)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D", ModuleKind::QmlLibrary, "Camera");

    bool isType = metaInfo.isQtQuick3DCamera();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_Camera)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DCamera();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_Command)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D", ModuleKind::QmlLibrary, "Command");

    bool isType = metaInfo.isQtQuick3DCommand();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_Command)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DCommand();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_DefaultMaterial)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D", ModuleKind::QmlLibrary, "DefaultMaterial");

    bool isType = metaInfo.isQtQuick3DDefaultMaterial();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_DefaultMaterial)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DDefaultMaterial();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_Effect)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D", ModuleKind::QmlLibrary, "Effect");

    bool isType = metaInfo.isQtQuick3DEffect();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_Effect)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DEffect();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_InstanceList)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D", ModuleKind::QmlLibrary, "InstanceList");

    bool isType = metaInfo.isQtQuick3DInstanceList();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_InstanceList)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DInstanceList();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_InstanceListEntry)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D", ModuleKind::QmlLibrary, "InstanceListEntry");

    bool isType = metaInfo.isQtQuick3DInstanceListEntry();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_InstanceListEntry)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DInstanceListEntry();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_Light)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D", ModuleKind::QmlLibrary, "Light");

    bool isType = metaInfo.isQtQuick3DLight();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_Light)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DLight();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_Material)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D", ModuleKind::QmlLibrary, "Material");

    bool isType = metaInfo.isQtQuick3DMaterial();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_Material)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DMaterial();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_Model)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D", ModuleKind::QmlLibrary, "Model");

    bool isType = metaInfo.isQtQuick3DModel();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_Model)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DModel();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_Node)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D", ModuleKind::QmlLibrary, "Node");

    bool isType = metaInfo.isQtQuick3DNode();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_Node)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DNode();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_Particles3D_cppnative_QQuick3DParticleAbstractShape)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D.Particles3D",
                                               ModuleKind::CppLibrary,
                                               "QQuick3DParticleAbstractShape");

    bool isType = metaInfo.isQtQuick3DParticlesAbstractShape();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_Particles3D_cppnative_QQuick3DParticleAbstractShape)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DParticlesAbstractShape();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_Particles3D_Affector3D)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D.Particles3D",
                                               ModuleKind::QmlLibrary,
                                               "Affector3D");

    bool isType = metaInfo.isQtQuick3DParticles3DAffector3D();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, QtQuick3D_Particles3D_Affector3D)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DParticles3DAffector3D();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_Particles3D_Attractor3D)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D.Particles3D",
                                               ModuleKind::QmlLibrary,
                                               "Attractor3D");

    bool isType = metaInfo.isQtQuick3DParticles3DAttractor3D();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, QtQuick3D_Particles3D_Attractor3D)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DParticles3DAttractor3D();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_Particles3D_Particle3D)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D.Particles3D",
                                               ModuleKind::QmlLibrary,
                                               "Particle3D");

    bool isType = metaInfo.isQtQuick3DParticles3DParticle3D();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, QtQuick3D_Particles3D_Particle3D)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DParticles3DParticle3D();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_Particles3D_ParticleEmitter3D)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D.Particles3D",
                                               ModuleKind::QmlLibrary,
                                               "ParticleEmitter3D");

    bool isType = metaInfo.isQtQuick3DParticles3DParticleEmitter3D();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, QtQuick3D_Particles3D_ParticleEmitter3D)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DParticles3DParticleEmitter3D();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_Particles3D_SpriteParticle3D)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D.Particles3D",
                                               ModuleKind::QmlLibrary,
                                               "SpriteParticle3D");

    bool isType = metaInfo.isQtQuick3DParticles3DSpriteParticle3D();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, QtQuick3D_Particles3D_SpriteParticle3D)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DParticles3DSpriteParticle3D();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_Pass)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D", ModuleKind::QmlLibrary, "Pass");

    bool isType = metaInfo.isQtQuick3DPass();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_Pass)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DPass();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_PrincipledMaterial)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D",
                                               ModuleKind::QmlLibrary,
                                               "PrincipledMaterial");

    bool isType = metaInfo.isQtQuick3DPrincipledMaterial();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_PrincipledMaterial)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DPrincipledMaterial();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_SpecularGlossyMaterial)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D",
                                               ModuleKind::QmlLibrary,
                                               "SpecularGlossyMaterial");

    bool isType = metaInfo.isQtQuick3DSpecularGlossyMaterial();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_SpecularGlossyMaterial)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DSpecularGlossyMaterial();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_SceneEnvironment)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D", ModuleKind::QmlLibrary, "SceneEnvironment");

    bool isType = metaInfo.isQtQuick3DSceneEnvironment();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_SceneEnvironment)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DSceneEnvironment();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_Shader)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D", ModuleKind::QmlLibrary, "Shader");

    bool isType = metaInfo.isQtQuick3DShader();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_Shader)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DShader();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_Texture)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D", ModuleKind::QmlLibrary, "Texture");

    bool isType = metaInfo.isQtQuick3DTexture();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_Texture)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DTexture();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_TextureInput)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D", ModuleKind::QmlLibrary, "TextureInput");

    bool isType = metaInfo.isQtQuick3DTextureInput();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_TextureInput)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DTextureInput();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_CubeMapTexture)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D", ModuleKind::QmlLibrary, "CubeMapTexture");

    bool isType = metaInfo.isQtQuick3DCubeMapTexture();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_CubeMapTexture)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DCubeMapTexture();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick3D_View3D)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick3D", ModuleKind::QmlLibrary, "View3D");

    bool isType = metaInfo.isQtQuick3DView3D();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick3D_View3D)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuick3DView3D();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick_BorderImage)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick", ModuleKind::QmlLibrary, "BorderImage");

    bool isType = metaInfo.isQtQuickBorderImage();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick_BorderImage)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickBorderImage();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuickControls_SwipeView)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick.Controls", ModuleKind::QmlLibrary, "SwipeView");

    bool isType = metaInfo.isQtQuickControlsSwipeView();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuickControls_SwipeView)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickControlsSwipeView();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuickControls_TabBar)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick.Controls", ModuleKind::QmlLibrary, "TabBar");

    bool isType = metaInfo.isQtQuickControlsTabBar();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuickControls_TabBar)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickControlsTabBar();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuickExtras_Picture)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick.Extras", ModuleKind::QmlLibrary, "Picture");

    bool isType = metaInfo.isQtQuickExtrasPicture();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuickExtras_Picture)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickExtrasPicture();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick_Image)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick", ModuleKind::QmlLibrary, "Image");

    bool isType = metaInfo.isQtQuickImage();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick_Image)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickImage();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick_Item)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick", ModuleKind::QmlLibrary, "Item");

    bool isType = metaInfo.isQtQuickItem();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick_Item)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickItem();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuickLayouts_BorderImage)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick.Layouts", ModuleKind::QmlLibrary, "Layout");

    bool isType = metaInfo.isQtQuickLayoutsLayout();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuickLayouts_Layout)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickLayoutsLayout();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick_Loader)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick", ModuleKind::QmlLibrary, "Loader");

    bool isType = metaInfo.isQtQuickLoader();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick_Loader)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickLoader();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick_Path)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick", ModuleKind::QmlLibrary, "Path");

    bool isType = metaInfo.isQtQuickPath();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick_Path)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickPath();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick_PauseAnimation)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick", ModuleKind::QmlLibrary, "PauseAnimation");

    bool isType = metaInfo.isQtQuickPauseAnimation();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick_PauseAnimation)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickPauseAnimation();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick_Positioner)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick", ModuleKind::QmlLibrary, "Positioner");

    bool isType = metaInfo.isQtQuickPositioner();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick_Positioner)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickPositioner();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick_PropertyAnimation)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick", ModuleKind::QmlLibrary, "PropertyAnimation");

    bool isType = metaInfo.isQtQuickPropertyAnimation();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick_PropertyAnimation)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickPropertyAnimation();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick_PropertyChanges)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick", ModuleKind::QmlLibrary, "PropertyChanges");

    bool isType = metaInfo.isQtQuickPropertyChanges();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick_PropertyChanges)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickPropertyChanges();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick_Repeater)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick", ModuleKind::QmlLibrary, "Repeater");

    bool isType = metaInfo.isQtQuickRepeater();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick_Repeater)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickRepeater();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick_State)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick", ModuleKind::QmlLibrary, "State");

    bool isType = metaInfo.isQtQuickState();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick_State)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickState();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuickNative_StateOperation)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick",
                                               ModuleKind::CppLibrary,
                                               "QQuickStateOperation");

    bool isType = metaInfo.isQtQuickStateOperation();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuickNative_StateOperation)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickStateOperation();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuickStudioComponents_GroupItem)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick.Studio.Components",
                                               ModuleKind::QmlLibrary,
                                               "GroupItem");

    bool isType = metaInfo.isQtQuickStudioComponentsGroupItem();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuickStudioComponents_GroupItem)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickStudioComponentsGroupItem();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick_Text)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick", ModuleKind::QmlLibrary, "Text");

    bool isType = metaInfo.isQtQuickText();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick_Text)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickText();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuickTimeline_Keyframe)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick.Timeline", ModuleKind::QmlLibrary, "Keyframe");

    bool isType = metaInfo.isQtQuickTimelineKeyframe();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick_Keyframe)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickTimelineKeyframe();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuickTimeline_KeyframeGroup)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick.Timeline",
                                               ModuleKind::QmlLibrary,
                                               "KeyframeGroup");

    bool isType = metaInfo.isQtQuickTimelineKeyframeGroup();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick_KeyframeGroup)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickTimelineKeyframeGroup();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuickTimeline_Timeline)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick.Timeline", ModuleKind::QmlLibrary, "Timeline");

    bool isType = metaInfo.isQtQuickTimelineTimeline();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick_Timeline)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickTimelineTimeline();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuickTimeline_TimelineAnimation)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick.Timeline",
                                               ModuleKind::QmlLibrary,
                                               "TimelineAnimation");

    bool isType = metaInfo.isQtQuickTimelineTimelineAnimation();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick_TimelineAnimation)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickTimelineTimelineAnimation();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuick_Transition)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick", ModuleKind::QmlLibrary, "Transition");

    bool isType = metaInfo.isQtQuickTransition();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuick_Transition)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickTransition();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtQuickWindow_Window)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick.Window", ModuleKind::QmlLibrary, "Window");

    bool isType = metaInfo.isQtQuickWindowWindow();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtQuickWindow_Window)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickWindowWindow();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtSafeRenderer_SafeRendererPicture)
{
    auto metaInfo = createDerivedDummyMetaInfo("Qt.SafeRenderer",
                                               ModuleKind::QmlLibrary,
                                               "SafeRendererPicture");

    bool isType = metaInfo.isQtSafeRendererSafeRendererPicture();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtSafeRenderer_SafeRendererPicture)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtSafeRendererSafeRendererPicture();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_QtSafeRenderer_SafePicture)
{
    auto metaInfo = createDerivedDummyMetaInfo("Qt.SafeRenderer", ModuleKind::QmlLibrary, "SafePicture");

    bool isType = metaInfo.isQtSafeRendererSafePicture();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_QtSafeRenderer_SafePicture)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtSafeRendererSafePicture();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_string)
{
    auto metaInfo = createMetaInfo("QML", ModuleKind::QmlLibrary, "string");

    bool isType = metaInfo.isString();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_string)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isString();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, QtQuick_Item_is_suitable_for_MouseArea_fill)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick", ModuleKind::QmlLibrary, "Item");

    bool isType = metaInfo.isSuitableForMouseAreaFill();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, QtQuick_MouseArea_is_suitable_for_MouseArea_fill)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick", ModuleKind::QmlLibrary, "MouseArea");

    bool isType = metaInfo.isSuitableForMouseAreaFill();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, QtQuickControls_Control_is_suitable_for_MouseArea_fill)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick.Controls", ModuleKind::QmlLibrary, "Control");

    bool isType = metaInfo.isSuitableForMouseAreaFill();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, QtQuickTemplates_Control_is_suitable_for_MouseArea_fill)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick.Templates", ModuleKind::QmlLibrary, "Control");

    bool isType = metaInfo.isSuitableForMouseAreaFill();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_suitable_for_MouseArea_fill)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isQtQuickTransition();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_url)
{
    auto metaInfo = createMetaInfo("QML", ModuleKind::QmlLibrary, "url");

    bool isType = metaInfo.isUrl();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_url)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isUrl();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_variant)
{
    auto metaInfo = createMetaInfo("QML", ModuleKind::QmlLibrary, "var");

    bool isType = metaInfo.isVariant();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_variant)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isVariant();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_vector2d)
{
    auto metaInfo = createMetaInfo("QtQuick", ModuleKind::QmlLibrary, "vector2d");

    bool isType = metaInfo.isVector2D();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_vector2d)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isVector2D();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_vector3d)
{
    auto metaInfo = createMetaInfo("QtQuick", ModuleKind::QmlLibrary, "vector3d");

    bool isType = metaInfo.isVector3D();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_vector3d)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isVector3D();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_vector4d)
{
    auto metaInfo = createMetaInfo("QtQuick", ModuleKind::QmlLibrary, "vector4d");

    bool isType = metaInfo.isVector4D();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_vector4d)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isVector4D();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, QtQuick_ListView_is_view)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick", ModuleKind::QmlLibrary, "ListView");

    bool isType = metaInfo.isView();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, QtQuick_GridView_is_view)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick", ModuleKind::QmlLibrary, "GridView");

    bool isType = metaInfo.isView();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, QtQuick_PathView_is_view)
{
    auto metaInfo = createDerivedDummyMetaInfo("QtQuick", ModuleKind::QmlLibrary, "PathView");

    bool isType = metaInfo.isView();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_view)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isView();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, is_enumeration)
{
    TypeTraits traits;
    traits.isEnum = true;
    auto metaInfo = createMetaInfo("QML", ModuleKind::QmlLibrary, "Foo", traits);

    bool isType = metaInfo.isEnumeration();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, is_not_enumeration)
{
    auto metaInfo = createMetaInfo("QML", ModuleKind::QmlLibrary, "Foo", {});

    bool isType = metaInfo.isEnumeration();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, default_is_not_enumeration)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isEnumeration();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, all_external_type_names)
{
    QmlDesigner::Storage::Info::ExportedTypeNames names{{qmlModuleId, "Object", 2, -1},
                                                        {qmlModuleId, "Obj", 2, 1}};
    auto metaInfo = createMetaInfo("QML", ModuleKind::QmlLibrary, "Foo");
    ON_CALL(projectStorageMock, exportedTypeNames(metaInfo.id())).WillByDefault(Return(names));

    auto exportedTypeNames = metaInfo.allExportedTypeNames();

    ASSERT_THAT(exportedTypeNames,
                UnorderedElementsAre(IsInfoExportTypeNames(qmlModuleId, "Object", 2, -1),
                                     IsInfoExportTypeNames(qmlModuleId, "Obj", 2, 1)));
}

TEST_F(NodeMetaInfo, default_has_no_external_type_names)
{
    QmlDesigner::Storage::Info::ExportedTypeNames names{{qmlModuleId, "Object", 2, -1},
                                                        {qmlModuleId, "Obj", 2, 1}};
    QmlDesigner::NodeMetaInfo metaInfo;
    ON_CALL(projectStorageMock, exportedTypeNames(_)).WillByDefault(Return(names));

    auto exportedTypeNames = metaInfo.allExportedTypeNames();

    ASSERT_THAT(exportedTypeNames, IsEmpty());
}

TEST_F(NodeMetaInfo, external_type_names_for_source_id)
{
    QmlDesigner::Storage::Info::ExportedTypeNames names{{qmlModuleId, "Object", 2, -1},
                                                        {qmlModuleId, "Obj", 2, 1}};
    auto metaInfo = createMetaInfo("QML", ModuleKind::QmlLibrary, "Foo");
    ON_CALL(projectStorageMock, exportedTypeNames(metaInfo.id(), model.fileUrlSourceId()))
        .WillByDefault(Return(names));

    auto exportedTypeNames = metaInfo.exportedTypeNamesForSourceId(model.fileUrlSourceId());

    ASSERT_THAT(exportedTypeNames,
                UnorderedElementsAre(IsInfoExportTypeNames(qmlModuleId, "Object", 2, -1),
                                     IsInfoExportTypeNames(qmlModuleId, "Obj", 2, 1)));
}

TEST_F(NodeMetaInfo, default_has_no_external_type_names_for_source_id)
{
    QmlDesigner::Storage::Info::ExportedTypeNames names{{qmlModuleId, "Object", 2, -1},
                                                        {qmlModuleId, "Obj", 2, 1}};
    QmlDesigner::NodeMetaInfo metaInfo;
    ON_CALL(projectStorageMock, exportedTypeNames(metaInfo.id(), model.fileUrlSourceId()))
        .WillByDefault(Return(names));

    auto exportedTypeNames = metaInfo.exportedTypeNamesForSourceId(model.fileUrlSourceId());

    ASSERT_THAT(exportedTypeNames, IsEmpty());
}

TEST_F(NodeMetaInfo, invalid_source_id_has_no_external_type_names_for_source_id)
{
    QmlDesigner::Storage::Info::ExportedTypeNames names{{qmlModuleId, "Object", 2, -1},
                                                        {qmlModuleId, "Obj", 2, 1}};
    auto metaInfo = createMetaInfo("QML", ModuleKind::QmlLibrary, "Foo");
    ON_CALL(projectStorageMock, exportedTypeNames(metaInfo.id(), model.fileUrlSourceId()))
        .WillByDefault(Return(names));
    QmlDesigner::SourceId sourceId;

    auto exportedTypeNames = metaInfo.exportedTypeNamesForSourceId(sourceId);

    ASSERT_THAT(exportedTypeNames, IsEmpty());
}

TEST_F(NodeMetaInfo, float_is_a_number)
{
    auto metaInfo = createMetaInfo("QML", ModuleKind::CppLibrary, "float");

    bool isType = metaInfo.isNumber();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, double_is_a_number)
{
    auto metaInfo = createMetaInfo(qmlModuleId, "double");

    bool isType = metaInfo.isNumber();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, int_is_a_number)
{
    auto metaInfo = createMetaInfo(qmlModuleId, "int");

    bool isType = metaInfo.isNumber();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, uint_is_a_number)
{
    auto metaInfo = createMetaInfo("QML", ModuleKind::CppLibrary, "uint");

    bool isType = metaInfo.isNumber();

    ASSERT_THAT(isType, IsTrue());
}

TEST_F(NodeMetaInfo, default_is_not_number)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    bool isType = metaInfo.isFloat();

    ASSERT_THAT(isType, IsFalse());
}

TEST_F(NodeMetaInfo, property_editor_specifics_path)
{
    auto metaInfo = createMetaInfo("QtQuick", ModuleKind::QmlLibrary, "Item");
    auto pathId = QmlDesigner::SourceId::create(45);
    ON_CALL(projectStorageMock, propertyEditorPathId(metaInfo.id())).WillByDefault(Return(pathId));

    auto id = metaInfo.propertyEditorPathId();

    ASSERT_THAT(id, pathId);
}

TEST_F(NodeMetaInfo, default_property_editor_specifics_path_is_empty)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    auto id = metaInfo.propertyEditorPathId();

    ASSERT_THAT(id, IsFalse());
}

TEST_F(NodeMetaInfo, is_reference)
{
    auto metaInfo = createMetaInfo("QtQuick", ModuleKind::QmlLibrary, "Item", TypeTraitsKind::Reference);

    auto type = metaInfo.type();

    ASSERT_THAT(type, QmlDesigner::MetaInfoType::Reference);
}

TEST_F(NodeMetaInfo, is_value)
{
    auto metaInfo = createMetaInfo("QML", ModuleKind::QmlLibrary, "bool", TypeTraitsKind::Value);

    auto type = metaInfo.type();

    ASSERT_THAT(type, QmlDesigner::MetaInfoType::Value);
}

TEST_F(NodeMetaInfo, is_sequence)
{
    auto metaInfo = createMetaInfo("QML", ModuleKind::QmlLibrary, "QObjectList", TypeTraitsKind::Sequence);

    auto type = metaInfo.type();

    ASSERT_THAT(type, QmlDesigner::MetaInfoType::Sequence);
}

TEST_F(NodeMetaInfo, is_none)
{
    auto metaInfo = createMetaInfo("QML", ModuleKind::QmlLibrary, "void", TypeTraitsKind::None);

    auto type = metaInfo.type();

    ASSERT_THAT(type, QmlDesigner::MetaInfoType::None);
}

TEST_F(NodeMetaInfo, default_is_none)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    auto type = metaInfo.type();

    ASSERT_THAT(type, QmlDesigner::MetaInfoType::None);
}

TEST_F(NodeMetaInfo, object_can_not_be_container)
{
    auto canBeContainer = objectMetaInfo.canBeContainer();

    ASSERT_THAT(canBeContainer, FlagIs::False);
}

TEST_F(NodeMetaInfo, default_can_not_be_container)
{
    auto canBeContainer = QmlDesigner::NodeMetaInfo{}.canBeContainer();

    ASSERT_THAT(canBeContainer, FlagIs::False);
}

TEST_F(NodeMetaInfo, invalid_can_not_be_container)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto canBeContainer = metaInfo.canBeContainer();

    ASSERT_THAT(canBeContainer, FlagIs::False);
}

TEST_F(NodeMetaInfo, component_can_be_container)
{
    auto moduleId = projectStorageMock.createModule("/path/to/project", ModuleKind::PathLibrary);
    TypeTraits traits{TypeTraitsKind::Reference};
    traits.canBeContainer = FlagIs::True;
    auto typeId = projectStorageMock.createType(moduleId, "Foo", traits);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    auto canBeContainer = metaInfo.canBeContainer();

    ASSERT_THAT(canBeContainer, FlagIs::True);
}

TEST_F(NodeMetaInfo, object_do_no_forces_clipping)
{
    auto forceClip = objectMetaInfo.forceClip();

    ASSERT_THAT(forceClip, FlagIs::False);
}

TEST_F(NodeMetaInfo, default_do_no_forces_clipping)
{
    auto forceClip = QmlDesigner::NodeMetaInfo{}.forceClip();

    ASSERT_THAT(forceClip, FlagIs::False);
}

TEST_F(NodeMetaInfo, invalid_do_no_forces_clipping)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto forceClip = metaInfo.forceClip();

    ASSERT_THAT(forceClip, FlagIs::False);
}

TEST_F(NodeMetaInfo, component_forces_clipping)
{
    auto moduleId = projectStorageMock.createModule("/path/to/project", ModuleKind::PathLibrary);
    TypeTraits traits{TypeTraitsKind::Reference};
    traits.forceClip = FlagIs::True;
    auto typeId = projectStorageMock.createType(moduleId, "Foo", traits);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    auto forceClip = metaInfo.forceClip();

    ASSERT_THAT(forceClip, FlagIs::True);
}

TEST_F(NodeMetaInfo, object_does_not_layout_children)
{
    auto doesLayoutChildren = objectMetaInfo.doesLayoutChildren();

    ASSERT_THAT(doesLayoutChildren, FlagIs::False);
}

TEST_F(NodeMetaInfo, default_does_not_layout_children)
{
    auto doesLayoutChildren = QmlDesigner::NodeMetaInfo{}.doesLayoutChildren();

    ASSERT_THAT(doesLayoutChildren, FlagIs::False);
}

TEST_F(NodeMetaInfo, invalid_does_not_layout_children)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto doesLayoutChildren = metaInfo.doesLayoutChildren();

    ASSERT_THAT(doesLayoutChildren, FlagIs::False);
}

TEST_F(NodeMetaInfo, component_layouts_children)
{
    auto moduleId = projectStorageMock.createModule("/path/to/project", ModuleKind::PathLibrary);
    TypeTraits traits{TypeTraitsKind::Reference};
    traits.doesLayoutChildren = FlagIs::True;
    auto typeId = projectStorageMock.createType(moduleId, "Foo", traits);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    auto doesLayoutChildren = metaInfo.doesLayoutChildren();

    ASSERT_THAT(doesLayoutChildren, FlagIs::True);
}

TEST_F(NodeMetaInfo, object_cannot_be_dropped_in_form_editor)
{
    auto canBeDroppedInFormEditor = objectMetaInfo.canBeDroppedInFormEditor();

    ASSERT_THAT(canBeDroppedInFormEditor, FlagIs::False);
}

TEST_F(NodeMetaInfo, default_cannot_be_dropped_in_form_editor)
{
    auto canBeDroppedInFormEditor = QmlDesigner::NodeMetaInfo{}.canBeDroppedInFormEditor();

    ASSERT_THAT(canBeDroppedInFormEditor, FlagIs::False);
}

TEST_F(NodeMetaInfo, invalid_cannot_be_dropped_in_form_editor)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto canBeDroppedInFormEditor = metaInfo.canBeDroppedInFormEditor();

    ASSERT_THAT(canBeDroppedInFormEditor, FlagIs::False);
}

TEST_F(NodeMetaInfo, component_can_be_dropped_in_form_editor)
{
    auto moduleId = projectStorageMock.createModule("/path/to/project", ModuleKind::PathLibrary);
    TypeTraits traits{TypeTraitsKind::Reference};
    traits.canBeDroppedInFormEditor = FlagIs::True;
    auto typeId = projectStorageMock.createType(moduleId, "Foo", traits);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    auto canBeDroppedInFormEditor = metaInfo.canBeDroppedInFormEditor();

    ASSERT_THAT(canBeDroppedInFormEditor, FlagIs::True);
}

TEST_F(NodeMetaInfo, object_cannot_be_dropped_in_navigator)
{
    auto canBeDroppedInNavigator = objectMetaInfo.canBeDroppedInNavigator();

    ASSERT_THAT(canBeDroppedInNavigator, FlagIs::False);
}

TEST_F(NodeMetaInfo, default_cannot_be_dropped_in_navigator)
{
    auto canBeDroppedInNavigator = QmlDesigner::NodeMetaInfo{}.canBeDroppedInNavigator();

    ASSERT_THAT(canBeDroppedInNavigator, FlagIs::False);
}

TEST_F(NodeMetaInfo, invalid_cannot_be_dropped_in_navigator)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto canBeDroppedInNavigator = metaInfo.canBeDroppedInNavigator();

    ASSERT_THAT(canBeDroppedInNavigator, FlagIs::False);
}

TEST_F(NodeMetaInfo, component_can_be_dropped_in_navigator)
{
    auto moduleId = projectStorageMock.createModule("/path/to/project", ModuleKind::PathLibrary);
    TypeTraits traits{TypeTraitsKind::Reference};
    traits.canBeDroppedInNavigator = FlagIs::True;
    auto typeId = projectStorageMock.createType(moduleId, "Foo", traits);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    auto canBeDroppedInNavigator = metaInfo.canBeDroppedInNavigator();

    ASSERT_THAT(canBeDroppedInNavigator, FlagIs::True);
}

TEST_F(NodeMetaInfo, object_cannot_be_dropped_in_3d_view)
{
    auto canBeDroppedInView3D = objectMetaInfo.canBeDroppedInView3D();

    ASSERT_THAT(canBeDroppedInView3D, FlagIs::False);
}

TEST_F(NodeMetaInfo, default_cannot_be_dropped_in_3d_view)
{
    auto canBeDroppedInView3D = QmlDesigner::NodeMetaInfo{}.canBeDroppedInView3D();

    ASSERT_THAT(canBeDroppedInView3D, FlagIs::False);
}

TEST_F(NodeMetaInfo, invalid_cannot_be_dropped_in_3d_view)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto canBeDroppedInView3D = metaInfo.canBeDroppedInView3D();

    ASSERT_THAT(canBeDroppedInView3D, FlagIs::False);
}

TEST_F(NodeMetaInfo, component_can_be_dropped_in_3d_view)
{
    auto moduleId = projectStorageMock.createModule("/path/to/project", ModuleKind::PathLibrary);
    TypeTraits traits{TypeTraitsKind::Reference};
    traits.canBeDroppedInView3D = FlagIs::True;
    auto typeId = projectStorageMock.createType(moduleId, "Foo", traits);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    auto canBeDroppedInView3D = metaInfo.canBeDroppedInView3D();

    ASSERT_THAT(canBeDroppedInView3D, FlagIs::True);
}

TEST_F(NodeMetaInfo, object_is_not_movable)
{
    auto isMovable = objectMetaInfo.isMovable();

    ASSERT_THAT(isMovable, FlagIs::False);
}

TEST_F(NodeMetaInfo, default_is_not_movable)
{
    auto isMovable = QmlDesigner::NodeMetaInfo{}.isMovable();

    ASSERT_THAT(isMovable, FlagIs::False);
}

TEST_F(NodeMetaInfo, invalid_is_not_movable)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto isMovable = metaInfo.isMovable();

    ASSERT_THAT(isMovable, FlagIs::False);
}

TEST_F(NodeMetaInfo, component_is_movable)
{
    auto moduleId = projectStorageMock.createModule("/path/to/project", ModuleKind::PathLibrary);
    TypeTraits traits{TypeTraitsKind::Reference};
    traits.isMovable = FlagIs::True;
    auto typeId = projectStorageMock.createType(moduleId, "Foo", traits);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    auto isMovable = metaInfo.isMovable();

    ASSERT_THAT(isMovable, FlagIs::True);
}

TEST_F(NodeMetaInfo, object_is_not_resizable)
{
    auto isResizable = objectMetaInfo.isResizable();

    ASSERT_THAT(isResizable, FlagIs::False);
}

TEST_F(NodeMetaInfo, default_is_not_resizable)
{
    auto isResizable = QmlDesigner::NodeMetaInfo{}.isResizable();

    ASSERT_THAT(isResizable, FlagIs::False);
}

TEST_F(NodeMetaInfo, invalid_is_not_resizable)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto isResizable = metaInfo.isResizable();

    ASSERT_THAT(isResizable, FlagIs::False);
}

TEST_F(NodeMetaInfo, component_is_resizable)
{
    auto moduleId = projectStorageMock.createModule("/path/to/project", ModuleKind::PathLibrary);
    TypeTraits traits{TypeTraitsKind::Reference};
    traits.isResizable = FlagIs::True;
    auto typeId = projectStorageMock.createType(moduleId, "Foo", traits);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    auto isResizable = metaInfo.isResizable();

    ASSERT_THAT(isResizable, FlagIs::True);
}

TEST_F(NodeMetaInfo, object_has_not_form_editor_item)
{
    auto hasFormEditorItem = objectMetaInfo.hasFormEditorItem();

    ASSERT_THAT(hasFormEditorItem, FlagIs::False);
}

TEST_F(NodeMetaInfo, default_has_not_form_editor_item)
{
    auto hasFormEditorItem = QmlDesigner::NodeMetaInfo{}.hasFormEditorItem();

    ASSERT_THAT(hasFormEditorItem, FlagIs::False);
}

TEST_F(NodeMetaInfo, invalid_has_not_form_editor_item)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto hasFormEditorItem = metaInfo.hasFormEditorItem();

    ASSERT_THAT(hasFormEditorItem, FlagIs::False);
}

TEST_F(NodeMetaInfo, component_has_form_editor_item)
{
    auto moduleId = projectStorageMock.createModule("/path/to/project", ModuleKind::PathLibrary);
    TypeTraits traits{TypeTraitsKind::Reference};
    traits.hasFormEditorItem = FlagIs::True;
    auto typeId = projectStorageMock.createType(moduleId, "Foo", traits);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    auto hasFormEditorItem = metaInfo.hasFormEditorItem();

    ASSERT_THAT(hasFormEditorItem, FlagIs::True);
}

TEST_F(NodeMetaInfo, object_is_not_stacked_container)
{
    auto isStackedContainer = objectMetaInfo.isStackedContainer();

    ASSERT_THAT(isStackedContainer, FlagIs::False);
}

TEST_F(NodeMetaInfo, default_is_not_stacked_container)
{
    auto isStackedContainer = QmlDesigner::NodeMetaInfo{}.isStackedContainer();

    ASSERT_THAT(isStackedContainer, FlagIs::False);
}

TEST_F(NodeMetaInfo, invalid_is_not_stacked_container)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto isStackedContainer = metaInfo.isStackedContainer();

    ASSERT_THAT(isStackedContainer, FlagIs::False);
}

TEST_F(NodeMetaInfo, component_is_stacked_container)
{
    auto moduleId = projectStorageMock.createModule("/path/to/project", ModuleKind::PathLibrary);
    TypeTraits traits{TypeTraitsKind::Reference};
    traits.isStackedContainer = FlagIs::True;
    auto typeId = projectStorageMock.createType(moduleId, "Foo", traits);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    auto isStackedContainer = metaInfo.isStackedContainer();

    ASSERT_THAT(isStackedContainer, FlagIs::True);
}

TEST_F(NodeMetaInfo, object_dont_takes_over_rendering_of_children)
{
    auto takesOverRenderingOfChildren = objectMetaInfo.takesOverRenderingOfChildren();

    ASSERT_THAT(takesOverRenderingOfChildren, FlagIs::False);
}

TEST_F(NodeMetaInfo, default_dont_takes_over_rendering_of_children)
{
    auto takesOverRenderingOfChildren = QmlDesigner::NodeMetaInfo{}.takesOverRenderingOfChildren();

    ASSERT_THAT(takesOverRenderingOfChildren, FlagIs::False);
}

TEST_F(NodeMetaInfo, invalid_dont_takes_over_rendering_of_children)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto takesOverRenderingOfChildren = metaInfo.takesOverRenderingOfChildren();

    ASSERT_THAT(takesOverRenderingOfChildren, FlagIs::False);
}

TEST_F(NodeMetaInfo, component_takes_over_rendering_of_children)
{
    auto moduleId = projectStorageMock.createModule("/path/to/project", ModuleKind::PathLibrary);
    TypeTraits traits{TypeTraitsKind::Reference};
    traits.takesOverRenderingOfChildren = FlagIs::True;
    auto typeId = projectStorageMock.createType(moduleId, "Foo", traits);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    auto takesOverRenderingOfChildren = metaInfo.takesOverRenderingOfChildren();

    ASSERT_THAT(takesOverRenderingOfChildren, FlagIs::True);
}

TEST_F(NodeMetaInfo, object_is_not_visible_in_navigator)
{
    auto visibleInNavigator = objectMetaInfo.visibleInNavigator();

    ASSERT_THAT(visibleInNavigator, FlagIs::False);
}

TEST_F(NodeMetaInfo, default_is_not_visible_in_navigator)
{
    auto visibleInNavigator = QmlDesigner::NodeMetaInfo{}.visibleInNavigator();

    ASSERT_THAT(visibleInNavigator, FlagIs::False);
}

TEST_F(NodeMetaInfo, invalid_is_not_visible_in_navigator)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto visibleInNavigator = metaInfo.visibleInNavigator();

    ASSERT_THAT(visibleInNavigator, FlagIs::False);
}

TEST_F(NodeMetaInfo, component_is_visible_in_navigator)
{
    auto moduleId = projectStorageMock.createModule("/path/to/project", ModuleKind::PathLibrary);
    TypeTraits traits{TypeTraitsKind::Reference};
    traits.visibleInNavigator = FlagIs::True;
    auto typeId = projectStorageMock.createType(moduleId, "Foo", traits);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    auto visibleInNavigator = metaInfo.visibleInNavigator();

    ASSERT_THAT(visibleInNavigator, FlagIs::True);
}

TEST_F(NodeMetaInfo, object_is_not_visible_in_library)
{
    auto visibleInLibrary = objectMetaInfo.visibleInLibrary();

    ASSERT_THAT(visibleInLibrary, FlagIs::False);
}

TEST_F(NodeMetaInfo, default_is_not_visible_in_library)
{
    auto visibleInLibrary = QmlDesigner::NodeMetaInfo{}.visibleInLibrary();

    ASSERT_THAT(visibleInLibrary, FlagIs::False);
}

TEST_F(NodeMetaInfo, invalid_is_not_visible_in_library)
{
    auto node = model.createModelNode("Foo");
    auto metaInfo = node.metaInfo();

    auto visibleInLibrary = metaInfo.visibleInLibrary();

    ASSERT_THAT(visibleInLibrary, FlagIs::False);
}

TEST_F(NodeMetaInfo, component_is_visible_in_library)
{
    auto moduleId = projectStorageMock.createModule("/path/to/project", ModuleKind::PathLibrary);
    TypeTraits traits{TypeTraitsKind::Reference};
    traits.visibleInLibrary = FlagIs::True;
    auto typeId = projectStorageMock.createType(moduleId, "Foo", traits);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    auto visibleInLibrary = metaInfo.visibleInLibrary();

    ASSERT_THAT(visibleInLibrary, FlagIs::True);
}

TEST_F(NodeMetaInfo, type_hints)
{
    projectStorageMock.setTypeHints(objectMetaInfo.id(), {{"inContainer", "true"}});

    auto typeHints = objectMetaInfo.typeHints();

    ASSERT_THAT(typeHints, ElementsAre(IsTypeHint("inContainer", "true")));
}

TEST_F(NodeMetaInfo, no_type_hints_for_default)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    auto typeHints = metaInfo.typeHints();

    ASSERT_THAT(typeHints, IsEmpty());
}

TEST_F(NodeMetaInfo, icon_path)
{
    projectStorageMock.setTypeIconPath(objectMetaInfo.id(), "/icon/path");

    auto path = objectMetaInfo.iconPath();

    ASSERT_THAT(path, Eq("/icon/path"));
}

TEST_F(NodeMetaInfo, no_icon_path_for_default)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    auto path = metaInfo.iconPath();

    ASSERT_THAT(path, IsEmpty());
}

TEST_F(NodeMetaInfo, item_library_entries)
{
    projectStorageMock.setItemLibraryEntries(objectMetaInfo.id(),
                                             {{objectMetaInfo.id(),
                                               "QtObject",
                                               "Object",
                                               "/icon/path",
                                               "Basic",
                                               "QtQuick",
                                               "An object",
                                               {{"x", "double", Sqlite::ValueView::create(1)}}}});

    auto entries = objectMetaInfo.itemLibrariesEntries();

    ASSERT_THAT(entries,
                ElementsAre(IsItemLibraryEntry(objectMetaInfo.id(),
                                               "QtObject",
                                               "Object",
                                               "/icon/path",
                                               "Basic",
                                               "QtQuick",
                                               "An object",
                                               "",
                                               ElementsAre(IsItemLibraryProperty("x", "double", 1)),
                                               IsEmpty())));
}

TEST_F(NodeMetaInfo, no_item_library_entries_for_default)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    auto entries = metaInfo.itemLibrariesEntries();

    ASSERT_THAT(entries, IsEmpty());
}

} // namespace
