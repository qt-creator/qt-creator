// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <matchers/import-matcher.h>
#include <mocks/abstractviewmock.h>
#include <mocks/externaldependenciesmock.h>
#include <mocks/modelresourcemanagementmock.h>
#include <mocks/projectstoragemock.h>
#include <mocks/projectstorageobservermock.h>
#include <mocks/projectstoragetriggerupdatemock.h>

#include <bindingproperty.h>
#include <itemlibraryentry.h>
#include <model.h>
#include <modelnode.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <rewriterview.h>
#include <signalhandlerproperty.h>
#include <variantproperty.h>

namespace {
using QmlDesigner::AbstractProperty;
using QmlDesigner::AbstractView;
using QmlDesigner::ModelNode;
using QmlDesigner::ModelNodes;
using QmlDesigner::ModelResourceSet;
using QmlDesigner::SourceId;
using QmlDesigner::Storage::Info::ExportedTypeName;
using QmlDesigner::Storage::ModuleKind;

MATCHER(IsSorted, std::string(negation ? "isn't sorted" : "is sorted"))
{
    using std::begin;
    using std::end;
    return std::is_sorted(begin(arg), end(arg));
}

template<typename PropertiesMatcher, typename ExtraFilePathsMatcher>
auto IsItemLibraryEntry(QmlDesigner::TypeId typeId,
                        QByteArrayView typeName,
                        QStringView name,
                        QStringView iconPath,
                        QStringView category,
                        QStringView import,
                        QStringView toolTip,
                        QStringView templatePath,
                        PropertiesMatcher propertiesMatcher,
                        ExtraFilePathsMatcher extraFilePathsMatcher)
{
    using QmlDesigner::ItemLibraryEntry;
    return AllOf(Property("typeId", &ItemLibraryEntry::typeId, typeId),
                 Property("typeName", &ItemLibraryEntry::typeName, typeName),
                 Property("name", &ItemLibraryEntry::name, name),
                 Property("libraryEntryIconPath", &ItemLibraryEntry::libraryEntryIconPath, iconPath),
                 Property("category", &ItemLibraryEntry::category, category),
                 Property("requiredImport", &ItemLibraryEntry::requiredImport, import),
                 Property("toolTip", &ItemLibraryEntry::toolTip, toolTip),
                 Property("templatePath", &ItemLibraryEntry::templatePath, templatePath),
                 Property("properties", &ItemLibraryEntry::properties, propertiesMatcher),
                 Property("extraFilePath", &ItemLibraryEntry::extraFilePaths, extraFilePathsMatcher));
}

template<typename PropertiesMatcher, typename ExtraFilePathsMatcher>
auto IsItemLibraryEntry(QmlDesigner::TypeId typeId,
                        QByteArrayView typeName,
                        QStringView name,
                        QStringView iconPath,
                        QStringView category,
                        QStringView import,
                        QStringView toolTip,
                        QStringView templatePath,
                        SourceId sourceId,
                        PropertiesMatcher propertiesMatcher,
                        ExtraFilePathsMatcher extraFilePathsMatcher)
{
    using QmlDesigner::ItemLibraryEntry;
    return AllOf(Property("typeId", &ItemLibraryEntry::typeId, typeId),
                 Property("typeName", &ItemLibraryEntry::typeName, typeName),
                 Property("name", &ItemLibraryEntry::name, name),
                 Property("libraryEntryIconPath", &ItemLibraryEntry::libraryEntryIconPath, iconPath),
                 Property("category", &ItemLibraryEntry::category, category),
                 Property("requiredImport", &ItemLibraryEntry::requiredImport, import),
                 Property("toolTip", &ItemLibraryEntry::toolTip, toolTip),
                 Property("templatePath", &ItemLibraryEntry::templatePath, templatePath),
                 Property("sourceId", &ItemLibraryEntry::sourceId, sourceId),
                 Property("properties", &ItemLibraryEntry::properties, propertiesMatcher),
                 Property("extraFilePath", &ItemLibraryEntry::extraFilePaths, extraFilePathsMatcher));
}

MATCHER_P3(IsItemLibraryProperty,
           name,
           type,
           value,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(QmlDesigner::PropertyContainer(name, type, value)))
{
    const QmlDesigner::PropertyContainer &property = arg;

    return property.name() == name && property.type() == type && property.value() == value;
}

auto IsExportedTypeName(const auto &moduleIdMatcher,
                        const auto &nameMatcher,
                        const auto &versionMatcher,
                        const auto &typeIdMatcher)
{
    return AllOf(Field("ExportedTypeName::moduleId",
                       &QmlDesigner::Storage::Info::ExportedTypeName::moduleId,
                       moduleIdMatcher),
                 Field("ExportedTypeName::name",
                       &QmlDesigner::Storage::Info::ExportedTypeName::name,
                       nameMatcher),
                 Field("ExportedTypeName::version",
                       &QmlDesigner::Storage::Info::ExportedTypeName::version,
                       versionMatcher),
                 Field("ExportedTypeName::typeId",
                       &QmlDesigner::Storage::Info::ExportedTypeName::typeId,
                       typeIdMatcher));
}

class Model : public ::testing::Test
{
protected:
    struct StaticData
    {
        Sqlite::Database modulesDatabase{":memory:", Sqlite::JournalMode::Memory};
        QmlDesigner::ModulesStorage modulesStorage{modulesDatabase, modulesDatabase.isInitialized()};
        Sqlite::Database sourcePathDatabase{":memory:", Sqlite::JournalMode::Memory};
        QmlDesigner::SourcePathStorage sourcePathStorage{sourcePathDatabase,
                                                         sourcePathDatabase.isInitialized()};
        QmlDesigner::SourcePathCache<QmlDesigner::SourcePathStorage> sourcePathCache{sourcePathStorage};
    };

    static void SetUpTestSuite() { staticData = std::make_unique<StaticData>(); }

    static void TearDownTestSuite() { staticData.reset(); }

    Model()
    {
        model.attachView(&viewMock);
        rootNode = viewMock.rootModelNode();
        ON_CALL(resourceManagementMock, removeNodes(_, _)).WillByDefault([](auto nodes, auto) {
            return ModelResourceSet{std::move(nodes), {}, {}};
        });
        ON_CALL(resourceManagementMock, removeProperties(_, _))
            .WillByDefault([](auto properties, auto) {
                return ModelResourceSet{{}, std::move(properties), {}};
            });
    }

    ~Model() { model.detachView(&viewMock); }

    auto createNodeWithParent(const ModelNode &parentNode, const QString &id = {})
    {
        auto node = parentNode.view()->createModelNode("QtQuick.Item");
        node.setIdWithoutRefactoring(id);
        parentNode.defaultNodeAbstractProperty().reparentHere(node);

        return node;
    }

    auto createProperty(const ModelNode &parentNode, QmlDesigner::PropertyName name)
    {
        auto property = parentNode.variantProperty(name);
        property.setValue(4);
        return property;
    }

    auto createBindingProperty(const ModelNode &parentNode,
                               QmlDesigner::PropertyName name,
                               const QString &expression = "foo")
    {
        auto property = parentNode.bindingProperty(name);
        property.setExpression(expression);
        return property;
    }

protected:
    inline static std::unique_ptr<StaticData> staticData;
    QmlDesigner::ModulesStorage &modulesStorage = staticData->modulesStorage;
    NiceMock<ProjectStorageTriggerUpdateMock> projectStorageTriggerUpdateMock;
    QmlDesigner::SourcePathCache<QmlDesigner::SourcePathStorage> &pathCache = staticData->sourcePathCache;
    Utils::PathString documentFilePath = "/path/foo.qml";
    SourceId documentSourceId = pathCache.sourceId(QmlDesigner::SourcePath{documentFilePath});
    NiceMock<ProjectStorageMockWithQtQuick> projectStorageMock{documentSourceId,
                                                               "/path",
                                                               modulesStorage};
    NiceMock<ModelResourceManagementMock> resourceManagementMock;
    QmlDesigner::Imports imports = {QmlDesigner::Import::createLibraryImport("QtQuick")};
    NiceMock<AbstractViewMock> viewMock;
    QUrl fileUrl = QUrl::fromLocalFile(documentFilePath.toQString());
    QmlDesigner::ProjectStorageDependencies projectStorageDependencies = {
        projectStorageMock, pathCache, modulesStorage, projectStorageTriggerUpdateMock};
    QmlDesigner::Model model{projectStorageDependencies,
                             "Item",
                             imports,
                             QUrl::fromLocalFile(documentFilePath.toQString()),
                             std::make_unique<ModelResourceManagementMockWrapper>(
                                 resourceManagementMock)};
    QmlDesigner::ModuleId qtQuickModuleId = modulesStorage.moduleId("QtQuick", ModuleKind::QmlLibrary);
    QmlDesigner::TypeId itemTypeId = projectStorageMock.typeId(qtQuickModuleId,
                                                               "Item",
                                                               QmlDesigner::Storage::Version{});
    QmlDesigner::ImportedTypeNameId itemTypeNameId = projectStorageMock.createImportedTypeNameId(
        documentSourceId, "Item", {qtQuickModuleId, itemTypeId, "Item"});
    ModelNode rootNode;
};

class Model_Creation : public Model
{};

TEST_F(Model_Creation, root_node_has_item_type_name)
{
    auto model = QmlDesigner::Model::create(projectStorageDependencies,
                                            "Item",
                                            imports,
                                            fileUrl,
                                            std::make_unique<ModelResourceManagementMockWrapper>(
                                                resourceManagementMock));

    ASSERT_THAT(model->rootModelNode().type(), Eq("Item"));
}

TEST_F(Model_Creation, root_node_has_item_meta_info)
{
    auto model = QmlDesigner::Model::create(projectStorageDependencies,
                                            "Item",
                                            imports,
                                            fileUrl,
                                            std::make_unique<ModelResourceManagementMockWrapper>(
                                                resourceManagementMock));

    ASSERT_THAT(model->rootModelNode().metaInfo(), model->qtQuickItemMetaInfo());
}

TEST_F(Model_Creation, file_url)
{
    auto model = QmlDesigner::Model::create(projectStorageDependencies,
                                            "Item",
                                            imports,
                                            fileUrl,
                                            std::make_unique<ModelResourceManagementMockWrapper>(
                                                resourceManagementMock));

    ASSERT_THAT(model->fileUrl().toLocalFile(), Eq(documentFilePath.toQString()));
}

TEST_F(Model_Creation, file_url_source_id)
{
    auto model = QmlDesigner::Model::create(projectStorageDependencies,
                                            "Item",
                                            imports,
                                            fileUrl,
                                            std::make_unique<ModelResourceManagementMockWrapper>(
                                                resourceManagementMock));

    ASSERT_THAT(model->fileUrlSourceId(), documentSourceId);
}

TEST_F(Model_Creation, imports)
{
    auto model = QmlDesigner::Model::create(projectStorageDependencies,
                                            "Item",
                                            imports,
                                            fileUrl,
                                            std::make_unique<ModelResourceManagementMockWrapper>(
                                                resourceManagementMock));

    ASSERT_THAT(model->imports(), UnorderedElementsAreArray(imports));
}

class Model_CreationFromOtherModel : public Model
{};

TEST_F(Model_CreationFromOtherModel, root_node_has_object_type_name)
{
    auto newModel = model.createModel("QtObject");

    ASSERT_THAT(newModel->rootModelNode().type(), Eq("QtObject"));
}

TEST_F(Model_CreationFromOtherModel, root_node_has_object_meta_info)
{
    auto newModel = model.createModel("QtObject");

    ASSERT_THAT(newModel->rootModelNode().metaInfo(), newModel->qmlQtObjectMetaInfo());
}

TEST_F(Model_CreationFromOtherModel, file_url)
{
    auto newModel = model.createModel("QtObject");

    ASSERT_THAT(newModel->fileUrl().toLocalFile(), Eq(documentFilePath.toQString()));
}

TEST_F(Model_CreationFromOtherModel, file_url_source_id)
{
    auto newModel = model.createModel("QtObject");
    ASSERT_THAT(newModel->fileUrlSourceId(), documentSourceId);
}

TEST_F(Model_CreationFromOtherModel, imports)
{
    auto newModel = model.createModel("QtObject");

    ASSERT_THAT(newModel->imports(), UnorderedElementsAreArray(imports));
}

class Model_ResourceManagment : public Model
{};

TEST_F(Model_ResourceManagment, model_node_destroy_is_calling_model_resource_management_remove_node)
{
    auto node = createNodeWithParent(rootNode);

    EXPECT_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model));

    node.destroy();
}

TEST_F(Model_ResourceManagment,
       model_node_remove_propery_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.removeProperty("foo");
}

TEST_F(Model_ResourceManagment,
       node_abstract_property_reparent_here_is_calling_model_resource_management_remove_property)
{
    auto node = createNodeWithParent(rootNode);
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.nodeListProperty("foo").reparentHere(node);
}

TEST_F(Model_ResourceManagment,
       node_property_set_model_node_is_calling_model_resource_management_remove_property)
{
    auto node = createNodeWithParent(rootNode);
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.nodeProperty("foo").setModelNode(node);
}

TEST_F(Model_ResourceManagment,
       variant_property_set_value_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.variantProperty("foo").setValue(7);
}

TEST_F(Model_ResourceManagment,
       variant_property_set_dynamic_type_name_and_enumeration_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.variantProperty("foo").setDynamicTypeNameAndEnumeration("int", "Ha");
}

TEST_F(Model_ResourceManagment,
       variant_property_set_dynamic_type_name_and_value_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.variantProperty("foo").setDynamicTypeNameAndValue("int", 7);
}

TEST_F(Model_ResourceManagment,
       binding_property_set_expression_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.bindingProperty("foo").setExpression("blah");
}

TEST_F(Model_ResourceManagment,
       binding_property_set_dynamic_type_name_and_expression_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.bindingProperty("foo").setDynamicTypeNameAndExpression("int", "blah");
}

TEST_F(Model_ResourceManagment,
       signal_handler_property_set_source_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.signalHandlerProperty("foo").setSource("blah");
}

TEST_F(Model_ResourceManagment,
       signal_declaration_property_set_signature_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.signalDeclarationProperty("foo").setSignature("blah");
}

TEST_F(Model_ResourceManagment, model_node_destroy_is_calling_abstract_view_node_about_to_be_removed)
{
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node, node2}, {}, {}}));

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node)));
    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node2)));

    node.destroy();
}

TEST_F(Model_ResourceManagment, model_node_destroy_is_calling_abstract_view_node_removed)
{
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node, node2}, {}, {}}));

    EXPECT_CALL(viewMock, nodeRemoved(Eq(node), _, _));
    EXPECT_CALL(viewMock, nodeRemoved(Eq(node2), _, _));

    node.destroy();
}

TEST_F(Model_ResourceManagment,
       model_node_destroy_is_calling_abstract_view_node_removed_with_valid_nodes)
{
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node, node2, ModelNode{}}, {}, {}}));

    EXPECT_CALL(viewMock, nodeRemoved(Eq(node), _, _));
    EXPECT_CALL(viewMock, nodeRemoved(Eq(node2), _, _));

    node.destroy();
}

TEST_F(Model_ResourceManagment,
       model_node_destroy_is_calling_abstract_view_properties_about_to_be_removed)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createProperty(rootNode, "foo");
    auto property2 = createProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(UnorderedElementsAre(property, property2)));

    node.destroy();
}

TEST_F(Model_ResourceManagment, model_node_destroy_is_calling_abstract_view_properties_removed)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createProperty(rootNode, "foo");
    auto property2 = createProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(UnorderedElementsAre(property, property2)));

    node.destroy();
}

TEST_F(Model_ResourceManagment,
       model_node_destroy_is_calling_abstract_view_properties_removed_only_with_valid_properties)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createProperty(rootNode, "foo");
    auto property2 = createProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {property, property2, {}}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(UnorderedElementsAre(property, property2)));

    node.destroy();
}

TEST_F(Model_ResourceManagment,
       model_node_destroy_is_calling_abstract_view_binding_properties_about_to_be_changed)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createBindingProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {}, {{property, "yi"}, {property2, "er"}}}));

    EXPECT_CALL(viewMock,
                bindingPropertiesAboutToBeChanged(UnorderedElementsAre(property, property2)));

    node.destroy();
}

TEST_F(Model_ResourceManagment, model_node_destroy_is_calling_abstract_view_binding_properties_changed)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createBindingProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {}, {{property, "yi"}, {property2, "er"}}}));

    EXPECT_CALL(viewMock, bindingPropertiesChanged(UnorderedElementsAre(property, property2), _));

    node.destroy();
}

TEST_F(Model_ResourceManagment, model_node_destroy_is_changing_binding_property_expression)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createBindingProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {}, {{property, "yi"}, {property2, "er"}}}));

    node.destroy();

    ASSERT_THAT(property.expression(), "yi");
    ASSERT_THAT(property2.expression(), "er");
}

TEST_F(Model_ResourceManagment, model_node_destroy_is_only_changing_existing_binding_property)
{
    auto node = createNodeWithParent(rootNode);
    auto property = rootNode.bindingProperty("foo");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{}, {}, {{property, "yi"}}}));

    node.destroy();

    ASSERT_FALSE(rootNode.hasBindingProperty("foo"));
}

TEST_F(Model_ResourceManagment,
       model_node_destroy_is_calling_abstract_view_binding_properties_changed_only_with_existing_properties)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createBindingProperty(rootNode, "foo");
    auto property2 = rootNode.bindingProperty("bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(
            ModelResourceSet{{node}, {}, {{property, "yi"}, {property2, "er"}, {{}, "san"}}}));

    EXPECT_CALL(viewMock, bindingPropertiesChanged(UnorderedElementsAre(property), _));

    node.destroy();
}

TEST_F(Model_ResourceManagment,
       model_node_remove_property_is_calling_abstract_view_node_about_to_be_removed)
{
    auto property = createProperty(rootNode, "foo");
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(Return(ModelResourceSet{{node, node2}, {property}, {}}));

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node)));
    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node2)));

    rootNode.removeProperty("foo");
}

TEST_F(Model_ResourceManagment, model_node_remove_property_is_calling_abstract_view_node_removed)
{
    auto property = createProperty(rootNode, "foo");
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(Return(ModelResourceSet{{node, node2}, {property}, {}}));

    EXPECT_CALL(viewMock, nodeRemoved(Eq(node), _, _));
    EXPECT_CALL(viewMock, nodeRemoved(Eq(node2), _, _));

    rootNode.removeProperty("foo");
}

TEST_F(Model_ResourceManagment,
       model_node_remove_property_is_calling_abstract_view_node_removed_with_valid_nodes)
{
    auto property = createProperty(rootNode, "foo");
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(Return(ModelResourceSet{{node, node2, ModelNode{}}, {property}, {}}));

    EXPECT_CALL(viewMock, nodeRemoved(Eq(node), _, _));
    EXPECT_CALL(viewMock, nodeRemoved(Eq(node2), _, _));

    rootNode.removeProperty("foo");
}

TEST_F(Model_ResourceManagment,
       model_node_remove_property_is_calling_abstract_view_properties_about_to_be_removed)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(Return(ModelResourceSet{{}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(UnorderedElementsAre(property, property2)));

    rootNode.removeProperty("yi");
}

TEST_F(Model_ResourceManagment, model_node_remove_property_is_calling_abstract_view_properties_removed)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(Return(ModelResourceSet{{}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(UnorderedElementsAre(property, property2)));

    rootNode.removeProperty("yi");
}

TEST_F(Model_ResourceManagment,
       model_node_remove_property_is_calling_abstract_view_properties_removed_only_with_valid_properties)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(Return(ModelResourceSet{{}, {property, property2, {}}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(UnorderedElementsAre(property, property2)));

    rootNode.removeProperty("yi");
}

TEST_F(Model_ResourceManagment,
       model_node_remove_property_is_calling_abstract_view_binding_properties_about_to_be_changed)
{
    auto property = createProperty(rootNode, "yi");
    auto property1 = createBindingProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(
            Return(ModelResourceSet{{}, {property}, {{property1, "yi"}, {property2, "er"}}}));

    EXPECT_CALL(viewMock, bindingPropertiesAboutToBeChanged(ElementsAre(property1, property2)));

    rootNode.removeProperty("yi");
}

TEST_F(Model_ResourceManagment,
       model_node_remove_property_is_calling_abstract_view_binding_properties_changed)
{
    auto property = createProperty(rootNode, "yi");
    auto property1 = createBindingProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(
            Return(ModelResourceSet{{}, {property}, {{property1, "yi"}, {property2, "er"}}}));

    EXPECT_CALL(viewMock, bindingPropertiesChanged(ElementsAre(property1, property2), _));

    rootNode.removeProperty("yi");
}

TEST_F(Model_ResourceManagment,
       model_node_remove_property_is_calling_abstract_view_binding_properties_changed_only_with_valid_properties)
{
    auto property = createProperty(rootNode, "yi");
    auto property1 = createBindingProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(
            Return(ModelResourceSet{{}, {property}, {{property1, "yi"}, {property2, "er"}, {}}}));

    EXPECT_CALL(viewMock, bindingPropertiesChanged(ElementsAre(property1, property2), _));

    rootNode.removeProperty("yi");
}

TEST_F(Model_ResourceManagment, by_default_remove_model_node_removes_node)
{
    QmlDesigner::Model newModel{{projectStorageMock,
                                 pathCache,
                                 modulesStorage,
                                 projectStorageTriggerUpdateMock},
                                "QtQuick.Item"};
    NiceMock<AbstractViewMock> viewMock;
    newModel.attachView(&viewMock);
    auto node = createNodeWithParent(viewMock.rootModelNode());

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node)));

    node.destroy();
}

TEST_F(Model_ResourceManagment, by_default_remove_properties_removes_property)
{
    QmlDesigner::Model newModel{{projectStorageMock,
                                 pathCache,
                                 modulesStorage,
                                 projectStorageTriggerUpdateMock},
                                "QtQuick.Item"};
    NiceMock<AbstractViewMock> viewMock;
    newModel.attachView(&viewMock);
    rootNode = viewMock.rootModelNode();
    auto property = createProperty(rootNode, "yi");

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(ElementsAre(Eq(property))));

    rootNode.removeProperty("yi");
}

TEST_F(Model_ResourceManagment, by_default_remove_model_node_in_factory_method_calls_removes_node)
{
    model.detachView(&viewMock);
    auto newModel = QmlDesigner::Model::create(projectStorageDependencies,
                                               "Item",
                                               imports,
                                               documentFilePath.toQString());
    newModel->attachView(&viewMock);
    auto node = createNodeWithParent(viewMock.rootModelNode());

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node)));

    node.destroy();
}

TEST_F(Model_ResourceManagment, by_default_remove_properties_in_factory_method_calls_remove_property)
{
    model.detachView(&viewMock);
    auto newModel = QmlDesigner::Model::create(projectStorageDependencies,
                                               "Item",
                                               imports,
                                               documentFilePath.toQString());
    newModel->attachView(&viewMock);
    rootNode = viewMock.rootModelNode();
    auto property = createProperty(rootNode, "yi");

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(ElementsAre(Eq(property))));

    rootNode.removeProperty("yi");
}

TEST_F(Model_ResourceManagment, remove_model_nodes)
{
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");

    EXPECT_CALL(resourceManagementMock,
                removeNodes(AllOf(UnorderedElementsAre(node, node2), IsSorted()), &model));

    model.removeModelNodes({node, node2});
}

TEST_F(Model_ResourceManagment, remove_model_nodes_filters_invalid_model_nodes)
{
    auto node = createNodeWithParent(rootNode, "yi");

    EXPECT_CALL(resourceManagementMock, removeNodes(UnorderedElementsAre(node), &model));

    model.removeModelNodes({{}, node});
}

TEST_F(Model_ResourceManagment, remove_model_nodes_for_only_invalid_model_nodes_does_nothing)
{
    EXPECT_CALL(resourceManagementMock, removeNodes(_, _)).Times(0);

    model.removeModelNodes({{}});
}

TEST_F(Model_ResourceManagment, remove_model_nodes_reverse)
{
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");

    EXPECT_CALL(resourceManagementMock,
                removeNodes(AllOf(UnorderedElementsAre(node, node2), IsSorted()), &model));

    model.removeModelNodes({node2, node});
}

TEST_F(Model_ResourceManagment, remove_model_nodes_calls_notifier)
{
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");
    auto property = createProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    auto property3 = createProperty(rootNode, "oh");

    ON_CALL(resourceManagementMock,
            removeNodes(AllOf(UnorderedElementsAre(node, node2), IsSorted()), &model))
        .WillByDefault(
            Return(ModelResourceSet{{node, node2}, {property, property3}, {{property2, "bar"}}}));

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node));
    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node2));
    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(UnorderedElementsAre(property, property3)));
    EXPECT_CALL(viewMock, bindingPropertiesChanged(ElementsAre(property2), _));

    model.removeModelNodes({node, node2});
}

TEST_F(Model_ResourceManagment, remove_model_nodes_bypasses_model_resource_management)
{
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");
    auto property = createProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    auto property3 = createProperty(rootNode, "oh");
    ON_CALL(resourceManagementMock,
            removeNodes(AllOf(UnorderedElementsAre(node, node2), IsSorted()), &model))
        .WillByDefault(
            Return(ModelResourceSet{{node, node2}, {property, property3}, {{property2, "bar"}}}));

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node));
    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node2));
    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(_)).Times(0);
    EXPECT_CALL(viewMock, bindingPropertiesChanged(_, _)).Times(0);

    model.removeModelNodes({node, node2}, QmlDesigner::BypassModelResourceManagement::Yes);
}

TEST_F(Model_ResourceManagment, by_default_remove_model_nodes_in_factory_method_calls_removes_node)
{
    QmlDesigner::Model newModel{{projectStorageMock,
                                 pathCache,
                                 modulesStorage,
                                 projectStorageTriggerUpdateMock},
                                "QtQuick.Item"};
    NiceMock<AbstractViewMock> viewMock;
    newModel.attachView(&viewMock);
    rootNode = viewMock.rootModelNode();
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node));
    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node2));

    newModel.removeModelNodes({node, node2});
}

TEST_F(Model_ResourceManagment, remove_properties)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");

    EXPECT_CALL(resourceManagementMock,
                removeProperties(AllOf(UnorderedElementsAre(property, property2), IsSorted()), &model));

    model.removeProperties({property, property2});
}

TEST_F(Model_ResourceManagment, remove_properties_filters_invalid_properties)
{
    auto property = createProperty(rootNode, "yi");

    EXPECT_CALL(resourceManagementMock, removeProperties(UnorderedElementsAre(property), &model));

    model.removeProperties({{}, property});
}

TEST_F(Model_ResourceManagment, remove_properties_for_only_invalid_properties_does_nothing)
{
    EXPECT_CALL(resourceManagementMock, removeProperties(_, _)).Times(0);

    model.removeProperties({{}});
}

TEST_F(Model_ResourceManagment, remove_properties_reverse)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");

    EXPECT_CALL(resourceManagementMock,
                removeProperties(AllOf(UnorderedElementsAre(property, property2), IsSorted()), &model));

    model.removeProperties({property2, property});
}

TEST_F(Model_ResourceManagment, remove_properties_calls_notifier)
{
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");
    auto property = createProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    auto property3 = createProperty(rootNode, "oh");

    ON_CALL(resourceManagementMock,
            removeProperties(AllOf(UnorderedElementsAre(property, property3), IsSorted()), &model))
        .WillByDefault(
            Return(ModelResourceSet{{node, node2}, {property, property3}, {{property2, "bar"}}}));

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node));
    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node2));
    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(UnorderedElementsAre(property, property3)));
    EXPECT_CALL(viewMock, bindingPropertiesChanged(ElementsAre(property2), _));

    model.removeProperties({property, property3});
}

TEST_F(Model_ResourceManagment, remove_properties_bypasses_model_resource_management)
{
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");
    auto property = createProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    auto property3 = createProperty(rootNode, "oh");
    ON_CALL(resourceManagementMock,
            removeProperties(AllOf(UnorderedElementsAre(property, property3), IsSorted()), &model))
        .WillByDefault(
            Return(ModelResourceSet{{node, node2}, {property, property3}, {{property2, "bar"}}}));

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(_)).Times(0);
    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(UnorderedElementsAre(property, property3)));
    EXPECT_CALL(viewMock, bindingPropertiesChanged(_, _)).Times(0);

    model.removeProperties({property, property3}, QmlDesigner::BypassModelResourceManagement::Yes);
}

TEST_F(Model_ResourceManagment, by_default_remove_properties_in_factory_method_calls_removes_properties)
{
    model.detachView(&viewMock);
    QmlDesigner::Model newModel{{projectStorageMock,
                                 pathCache,
                                 modulesStorage,
                                 projectStorageTriggerUpdateMock},
                                "QtQuick.Item"};
    newModel.attachView(&viewMock);
    rootNode = viewMock.rootModelNode();
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(UnorderedElementsAre(property, property2)));

    newModel.removeProperties({property, property2});
}

class Model_Imports : public Model
{};

TEST_F(Model_Imports, change_imports_is_synchronizing_imports_with_project_storage)
{
    auto qmlModuleId = modulesStorage.moduleId("QML", ModuleKind::QmlLibrary);
    auto qtQuickModuleId = modulesStorage.moduleId("QtQuick", ModuleKind::QmlLibrary);
    auto qtQmlModelsModuleId = modulesStorage.moduleId("QtQml.Models", ModuleKind::QmlLibrary);
    auto localPathModuleId = modulesStorage.moduleId("/path", ModuleKind::PathLibrary);
    auto qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "2.1");
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models");

    EXPECT_CALL(projectStorageMock,
                synchronizeDocumentImports(
                    UnorderedElementsAre(IsImport(qmlModuleId, documentSourceId, -1, -1),
                                         IsImport(qtQuickModuleId, documentSourceId, 2, 1),
                                         IsImport(qtQmlModelsModuleId, documentSourceId, -1, -1),
                                         IsImport(localPathModuleId, documentSourceId, -1, -1)),
                    documentSourceId));

    model.changeImports({qtQuickImport, qtQmlModelsImport}, {});
}

TEST_F(Model_Imports, change_imports_with_windows_file_url)
{
    QmlDesigner::SourcePath windowsFilePath = "c:/path/foo.qml";
    QUrl windowsFilePathUrl = windowsFilePath.toQString();
    SourceId windowsSourceId = pathCache.sourceId(windowsFilePath);
    model.setFileUrl(windowsFilePathUrl);
    auto qmlModuleId = modulesStorage.moduleId("QML", ModuleKind::QmlLibrary);
    auto qtQuickModuleId = modulesStorage.moduleId("QtQuick", ModuleKind::QmlLibrary);
    auto qtQmlModelsModuleId = modulesStorage.moduleId("QtQml.Models", ModuleKind::QmlLibrary);
    auto localPathModuleId = projectStorageMock.createModule("c:/path",
                                                             QmlDesigner::Storage::ModuleKind::PathLibrary);
    auto qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "2.1");
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models");

    EXPECT_CALL(projectStorageMock,
                synchronizeDocumentImports(
                    UnorderedElementsAre(IsImport(qmlModuleId, windowsSourceId, -1, -1),
                                         IsImport(qtQuickModuleId, windowsSourceId, 2, 1),
                                         IsImport(qtQmlModelsModuleId, windowsSourceId, -1, -1),
                                         IsImport(localPathModuleId, windowsSourceId, -1, -1)),
                    windowsSourceId));

    model.changeImports({qtQuickImport, qtQmlModelsImport}, {});
}

TEST_F(Model_Imports,
       change_imports_is_not_synchronizing_imports_with_project_storage_if_no_new_imports_are_added)
{
    auto qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "2.1");
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models");
    model.changeImports({qtQuickImport, qtQmlModelsImport}, {});

    EXPECT_CALL(projectStorageMock, synchronizeDocumentImports(_, _)).Times(0);

    model.changeImports({qtQuickImport, qtQmlModelsImport}, {});
}

TEST_F(Model_Imports, change_imports_is_adding_import_in_project_storage)
{
    auto qmlModuleId = modulesStorage.moduleId("QML", ModuleKind::QmlLibrary);
    auto qtQuickModuleId = modulesStorage.moduleId("QtQuick", ModuleKind::QmlLibrary);
    auto qtQmlModelsModuleId = modulesStorage.moduleId("QtQml.Models", ModuleKind::QmlLibrary);
    auto localPathModuleId = modulesStorage.moduleId("/path", ModuleKind::PathLibrary);
    auto qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "2.1");
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models");
    model.changeImports({qtQmlModelsImport}, {});

    EXPECT_CALL(projectStorageMock,
                synchronizeDocumentImports(
                    UnorderedElementsAre(IsImport(qmlModuleId, documentSourceId, -1, -1),
                                         IsImport(qtQuickModuleId, documentSourceId, 2, 1),
                                         IsImport(qtQmlModelsModuleId, documentSourceId, -1, -1),
                                         IsImport(localPathModuleId, documentSourceId, -1, -1)),
                    documentSourceId));

    model.changeImports({qtQuickImport}, {});
}

TEST_F(Model_Imports, change_imports_is_removing_import_in_project_storage)
{
    auto qmlModuleId = modulesStorage.moduleId("QML", ModuleKind::QmlLibrary);
    auto qtQmlModelsModuleId = modulesStorage.moduleId("QtQml.Models", ModuleKind::QmlLibrary);
    auto localPathModuleId = modulesStorage.moduleId("/path", ModuleKind::PathLibrary);
    auto qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "2.1");
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models");
    model.changeImports({qtQuickImport, qtQmlModelsImport}, {});

    EXPECT_CALL(projectStorageMock,
                synchronizeDocumentImports(
                    UnorderedElementsAre(IsImport(qmlModuleId, documentSourceId, -1, -1),
                                         IsImport(qtQmlModelsModuleId, documentSourceId, -1, -1),
                                         IsImport(localPathModuleId, documentSourceId, -1, -1)),
                    documentSourceId));

    model.changeImports({}, {qtQuickImport});
}

TEST_F(Model_Imports,
       change_imports_is_not_removing_import_in_project_storage_if_import_is_not_in_model_imports)
{
    auto qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "2.1");
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models");
    model.changeImports({qtQuickImport}, {});

    EXPECT_CALL(projectStorageMock, synchronizeDocumentImports(_, _)).Times(0);

    model.changeImports({}, {qtQmlModelsImport});
}

TEST_F(Model_Imports, change_imports_is_changing_import_version_with_project_storage)
{
    auto qmlModuleId = modulesStorage.moduleId("QML", ModuleKind::QmlLibrary);
    auto qtQuickModuleId = modulesStorage.moduleId("QtQuick", ModuleKind::QmlLibrary);
    auto qtQmlModelsModuleId = modulesStorage.moduleId("QtQml.Models", ModuleKind::QmlLibrary);
    auto localPathModuleId = modulesStorage.moduleId("/path", ModuleKind::PathLibrary);
    auto qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "2.1");
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models");
    model.changeImports({qtQuickImport, qtQmlModelsImport}, {});
    qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "3.1");

    EXPECT_CALL(projectStorageMock,
                synchronizeDocumentImports(
                    UnorderedElementsAre(IsImport(qmlModuleId, documentSourceId, -1, -1),
                                         IsImport(qtQuickModuleId, documentSourceId, 3, 1),
                                         IsImport(qtQmlModelsModuleId, documentSourceId, -1, -1),
                                         IsImport(localPathModuleId, documentSourceId, -1, -1)),
                    documentSourceId));

    model.changeImports({qtQuickImport}, {});
}

TEST_F(Model_Imports, change_imports_is_normalizing_import_path_for_modules)
{
    auto directoryModuleId = modulesStorage.moduleId("/path/foo/bar", ModuleKind::PathLibrary);
    auto directoryImport = QmlDesigner::Import::createFileImport("foo/bar/../bar/");

    EXPECT_CALL(projectStorageMock,
                synchronizeDocumentImports(Contains(
                                               IsImport(directoryModuleId, documentSourceId, -1, -1)),
                                           _));

    model.changeImports({directoryImport}, {});
}

TEST_F(Model_Imports, change_imports_updates_node_to_invalid_type)
{
    auto quickImport = QmlDesigner::Import::createLibraryImport("QtQuick");
    auto node = model.createModelNode("Item");
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, {});

    model.changeImports({}, {quickImport});

    ASSERT_FALSE(node.metaInfo());
}

TEST_F(Model_Imports, change_imports_updates_root_node_to_invalid_type)
{
    auto quickImport = QmlDesigner::Import::createLibraryImport("QtQuick");
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, {});

    model.changeImports({}, {quickImport});

    ASSERT_FALSE(rootNode.metaInfo());
}

TEST_F(Model_Imports, change_imports_updates_node_to_valid_type)
{
    auto quickImport = QmlDesigner::Import::createLibraryImport("QtQuick");
    auto exportedTypeName = projectStorageMock.exportedTypeName(itemTypeNameId);
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, {});
    auto node = model.createModelNode("Item");
    model.changeImports({}, {quickImport});
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, exportedTypeName);

    model.changeImports({quickImport}, {});

    ASSERT_THAT(node.metaInfo(), model.qtQuickItemMetaInfo());
}

TEST_F(Model_Imports, change_imports_updates_root_node_to_valid_type)
{
    auto quickImport = QmlDesigner::Import::createLibraryImport("QtQuick");
    auto exportedTypeName = projectStorageMock.exportedTypeName(itemTypeNameId);
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, {});
    model.changeImports({}, {quickImport});
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, exportedTypeName);

    model.changeImports({quickImport}, {});

    ASSERT_THAT(rootNode.metaInfo(), model.qtQuickItemMetaInfo());
}

TEST_F(Model_Imports, set_imports_updates_node_to_invalid_type)
{
    auto node = model.createModelNode("Item");
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, {});

    model.setImports({});

    ASSERT_FALSE(node.metaInfo());
}

TEST_F(Model_Imports, set_imports_updates_root_node_to_invalid_type)
{
    auto quickImport = QmlDesigner::Import::createLibraryImport("QtQuick");
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, {});

    model.setImports({});

    ASSERT_FALSE(rootNode.metaInfo());
}

TEST_F(Model_Imports, set_imports_updates_node_to_valid_type)
{
    auto quickImport = QmlDesigner::Import::createLibraryImport("QtQuick");
    auto exportedTypeName = projectStorageMock.exportedTypeName(itemTypeNameId);
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, {});
    auto node = model.createModelNode("Item");
    model.setImports({});
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, exportedTypeName);

    model.setImports({quickImport});

    ASSERT_THAT(node.metaInfo(), model.qtQuickItemMetaInfo());
}

TEST_F(Model_Imports, set_imports_updates_root_node_to_valid_type)
{
    auto quickImport = QmlDesigner::Import::createLibraryImport("QtQuick");
    auto exportedTypeName = projectStorageMock.exportedTypeName(itemTypeNameId);
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, {});
    model.setImports({});
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, exportedTypeName);

    model.setImports({quickImport});

    ASSERT_THAT(rootNode.metaInfo(), model.qtQuickItemMetaInfo());
}

class Model_Node : public Model
{};

TEST_F(Model_Node, create_model_node_has_meta_info)
{
    auto node = model.createModelNode("Item");

    ASSERT_THAT(node.metaInfo(), model.qtQuickItemMetaInfo());
}

TEST_F(Model_Node, create_qualified_model_node_has_meta_info)
{
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models", "", "Foo");
    auto qtQmlModelsModulesId = modulesStorage.moduleId("QtQml.Models", ModuleKind::QmlLibrary);
    auto importId = projectStorageMock.createImportIdWithAlias(documentSourceId, "Foo");
    auto listModelTypeId = projectStorageMock.typeId(qtQmlModelsModulesId,
                                                     "ListModel",
                                                     QmlDesigner::Storage::Version{});
    projectStorageMock.createImportedTypeNameId(importId,
                                                "ListModel",
                                                {qtQmlModelsModulesId, listModelTypeId, "ListModel"});
    model.changeImports({qtQmlModelsImport}, {});

    auto node = model.createModelNode("Foo.ListModel");

    ASSERT_THAT(node.metaInfo(), model.qtQmlModelsListModelMetaInfo());
}

TEST_F(Model_Node, change_node_type_changes_meta_info)
{
    auto qtQmlModelsModulesId = modulesStorage.moduleId("QtQml.Models", ModuleKind::QmlLibrary);
    projectStorageMock.createImportedTypeNameId(documentSourceId,
                                                "QtObject",
                                                {qtQmlModelsModulesId,
                                                 model.qmlQtObjectMetaInfo().id(),
                                                 "QtObject"});
    auto node = model.createModelNode("Item");

    node.changeType("QtObject");

    ASSERT_THAT(node.metaInfo(), model.qmlQtObjectMetaInfo());
}

TEST_F(Model_Node, change_root_node_type_changes_meta_info)
{
    auto qtQmlModelsModulesId = modulesStorage.moduleId("QtQml.Models", ModuleKind::QmlLibrary);
    projectStorageMock.createImportedTypeNameId(documentSourceId,
                                                "QtObject",
                                                {qtQmlModelsModulesId,
                                                 model.qmlQtObjectMetaInfo().id(),
                                                 "QtObject"});

    model.changeRootNodeType("QtObject");

    ASSERT_THAT(rootNode.metaInfo(), model.qmlQtObjectMetaInfo());
}

class Model_MetaInfo : public Model
{};

TEST_F(Model_MetaInfo, get_meta_info_for_type_name)
{
    auto meta_info = model.metaInfo("QtObject");

    ASSERT_THAT(meta_info, model.qmlQtObjectMetaInfo());
}

TEST_F(Model_MetaInfo, meta_info_of_not_existing_type_is_invalid)
{
    auto meta_info = model.metaInfo("Foo");

    ASSERT_THAT(meta_info, IsFalse());
}

TEST_F(Model_MetaInfo, module_is_valid)
{
    auto module = model.module("QML", ModuleKind::QmlLibrary);

    ASSERT_THAT(module, IsTrue());
}

TEST_F(Model_MetaInfo, module_returns_always_the_same)
{
    auto oldModule = model.module("QML", ModuleKind::QmlLibrary);

    auto module = model.module("QML", ModuleKind::QmlLibrary);

    ASSERT_THAT(module, oldModule);
}

TEST_F(Model_MetaInfo, get_meta_info_by_module)
{
    auto module = model.module("QML", ModuleKind::QmlLibrary);

    auto metaInfo = model.metaInfo(module, "QtObject");

    ASSERT_THAT(metaInfo, model.qmlQtObjectMetaInfo());
}

TEST_F(Model_MetaInfo, get_invalid_meta_info_by_module_and_name_for_wrong_name)
{
    auto module = model.module("QML", ModuleKind::QmlLibrary);

    auto metaInfo = model.metaInfo(module, "Object");

    ASSERT_THAT(metaInfo, IsFalse());
}

TEST_F(Model_MetaInfo, get_invalid_meta_info_by_module_for_wrong_module)
{
    auto module = model.module("Qml", ModuleKind::QmlLibrary);

    auto metaInfo = model.metaInfo(module, "Object");

    ASSERT_THAT(metaInfo, IsFalse());
}

TEST_F(Model_MetaInfo, get_module_ids_that_starts_with)
{
    auto moduleIds = model.moduleIdsStartsWith("Q", ModuleKind::QmlLibrary);

    ASSERT_THAT(moduleIds, Contains(qtQuickModuleId));
}

TEST_F(Model_MetaInfo, add_project_storage_observer_to_project_storage)
{
    EXPECT_CALL(projectStorageMock, addObserver(_));

    QmlDesigner::Model model{{projectStorageMock, pathCache, modulesStorage, projectStorageTriggerUpdateMock},
                             "Item",
                             -1,
                             -1,
                             nullptr,
                             {}};
}

TEST_F(Model_MetaInfo, remove_project_storage_observer_from_project_storage)
{
    EXPECT_CALL(projectStorageMock, removeObserver(_)).Times(2); // the fixture model is calling it too

    QmlDesigner::Model model{{projectStorageMock, pathCache, modulesStorage, projectStorageTriggerUpdateMock},
                             "Item",
                             -1,
                             -1,
                             nullptr,
                             {}};
}

TEST_F(Model_MetaInfo, refresh_meta_infos_callback_is_calling_abstract_view)
{
    const QmlDesigner::TypeIds typeIds = {QmlDesigner::TypeId::create(3),
                                          QmlDesigner::TypeId::create(1)};
    ProjectStorageObserverMock observerMock;
    QmlDesigner::ProjectStorageObserver *observer = nullptr;
    ON_CALL(projectStorageMock, addObserver(_)).WillByDefault([&](auto *o) { observer = o; });
    QmlDesigner::Model model{{projectStorageMock, pathCache, modulesStorage, projectStorageTriggerUpdateMock},
                             "Item",
                             -1,
                             -1,
                             nullptr,
                             {}};
    model.attachView(&viewMock);

    EXPECT_CALL(viewMock, refreshMetaInfos(typeIds));

    observer->removedTypeIds(typeIds);
}

TEST_F(Model_MetaInfo, added_exported_type_names_are_changed_callback_is_calling_abstract_view)
{
    using QmlDesigner::Storage::Info::ExportedTypeNames;
    ExportedTypeNames added = {{qtQuickModuleId, itemTypeId, "Foo", 1, 1}};
    ProjectStorageObserverMock observerMock;
    QmlDesigner::ProjectStorageObserver *observer = nullptr;
    ON_CALL(projectStorageMock, addObserver(_)).WillByDefault([&](auto *o) { observer = o; });
    QmlDesigner::Model model{{projectStorageMock, pathCache, modulesStorage, projectStorageTriggerUpdateMock},
                             "Item",
                             -1,
                             -1,
                             nullptr,
                             {}};
    model.attachView(&viewMock);

    EXPECT_CALL(viewMock, exportedTypeNamesChanged(added, IsEmpty()));

    observer->exportedTypeNamesChanged(added, {});
}

TEST_F(Model_MetaInfo, removed_exported_type_names_are_changed_callback_is_calling_abstract_view)
{
    using QmlDesigner::Storage::Info::ExportedTypeNames;
    ExportedTypeNames removed = {{qtQuickModuleId, itemTypeId, "Foo", 1, 1}};
    ProjectStorageObserverMock observerMock;
    QmlDesigner::ProjectStorageObserver *observer = nullptr;
    ON_CALL(projectStorageMock, addObserver(_)).WillByDefault([&](auto *o) { observer = o; });
    QmlDesigner::Model model{{projectStorageMock, pathCache, modulesStorage, projectStorageTriggerUpdateMock},
                             "Item",
                             -1,
                             -1,
                             nullptr,
                             {}};
    model.attachView(&viewMock);

    EXPECT_CALL(viewMock, exportedTypeNamesChanged(IsEmpty(), removed));

    observer->exportedTypeNamesChanged({}, removed);
}

TEST_F(Model_MetaInfo, meta_infos_for_mdoule)
{
    projectStorageMock.createModule("Foo", ModuleKind::QmlLibrary);
    auto module = model.module("Foo", ModuleKind::QmlLibrary);
    auto typeId = projectStorageMock.createObject(module.id(), "Bar");
    ON_CALL(projectStorageMock, typeIds(module.id()))
        .WillByDefault(Return(QmlDesigner::SmallTypeIds<256>{typeId}));

    auto types = model.metaInfosForModule(module);

    ASSERT_THAT(types, ElementsAre(Eq(QmlDesigner::NodeMetaInfo{typeId, &projectStorageMock})));
}

TEST_F(Model_MetaInfo, singleton_meta_infos)
{
    ON_CALL(projectStorageMock, singletonTypeIds(documentSourceId))
        .WillByDefault(Return(QmlDesigner::SmallTypeIds<256>{itemTypeId}));

    auto types = model.singletonMetaInfos();

    ASSERT_THAT(types, ElementsAre(Eq(QmlDesigner::NodeMetaInfo{itemTypeId, &projectStorageMock})));
}

TEST_F(Model_MetaInfo, create_node_resolved_meta_type)
{
    auto node = model.createModelNode("Item");

    ASSERT_THAT(node.metaInfo(), model.qtQuickItemMetaInfo());
}

TEST_F(Model_MetaInfo, create_node_has_unresolved_meta_type_for_invalid_type_name)
{
    auto node = model.createModelNode("Foo");

    ASSERT_THAT(node.metaInfo(), IsFalse());
}

TEST_F(Model_MetaInfo, refresh_meta_info_if_project_storage_removed_exported_type_name)
{
    auto node = model.createModelNode("Item");
    projectStorageMock.removeType(qtQuickModuleId, "Item");
    auto observer = projectStorageMock.observers.front();
    auto itemTypeId2 = projectStorageMock.createObject(qtQuickModuleId, "Item");
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId,
                                                 {qtQuickModuleId, itemTypeId2, "Item"});
    ExportedTypeName itemExportedTypeName{qtQuickModuleId, itemTypeId, "Item"};
    ExportedTypeName item2ExportedTypeName{qtQuickModuleId, itemTypeId2, "Item"};

    observer->exportedTypeNamesChanged({item2ExportedTypeName}, {itemExportedTypeName});

    ASSERT_THAT(node.metaInfo().id(), itemTypeId2);
}

TEST_F(Model_MetaInfo, refresh_exported_type_name_if_project_storage_removed_exported_type_name)
{
    auto node = model.createModelNode("Item");
    projectStorageMock.removeType(qtQuickModuleId, "Item");
    auto observer = projectStorageMock.observers.front();
    auto itemTypeId2 = projectStorageMock.createObject(qtQuickModuleId, "Item");
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId,
                                                 {qtQuickModuleId, itemTypeId2, "Item"});
    ExportedTypeName itemExportedTypeName{qtQuickModuleId, itemTypeId, "Item"};
    ExportedTypeName item2ExportedTypeName{qtQuickModuleId, itemTypeId2, "Item"};

    observer->exportedTypeNamesChanged({item2ExportedTypeName}, {itemExportedTypeName});

    ASSERT_THAT(node.exportedTypeName(),
                IsExportedTypeName(qtQuickModuleId, "Item", HasNoVersion(), itemTypeId2));
}

TEST_F(Model_MetaInfo,
       set_null_meta_info_if_project_storage_removed_exported_type_name_cannot_be_refreshed)
{
    auto node = model.createModelNode("Item");
    projectStorageMock.removeType(qtQuickModuleId, "Item");
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, {});
    ExportedTypeName itemExportedTypeName{qtQuickModuleId, itemTypeId, "Item"};
    auto observer = projectStorageMock.observers.front();

    observer->exportedTypeNamesChanged({}, {itemExportedTypeName});

    ASSERT_THAT(node.metaInfo(), IsFalse());
}

TEST_F(Model_MetaInfo,
       reset_exported_type_name_if_project_storage_removed_exported_type_name_cannot_be_refreshed)
{
    auto node = model.createModelNode("Item");
    projectStorageMock.removeType(qtQuickModuleId, "Item");
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, {});
    ExportedTypeName itemExportedTypeName{qtQuickModuleId, itemTypeId, "Item"};
    auto observer = projectStorageMock.observers.front();

    observer->exportedTypeNamesChanged({}, {itemExportedTypeName});

    ASSERT_THAT(node.exportedTypeName(),
                IsExportedTypeName(IsFalse(), IsEmpty(), HasNoVersion(), IsFalse()));
}

TEST_F(Model_MetaInfo, notify_about_removed_exported_type_name)
{
    auto node = model.createModelNode("Item");
    projectStorageMock.removeType(qtQuickModuleId, "Item");
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, {});
    ExportedTypeName itemExportedTypeName{qtQuickModuleId, itemTypeId, "Item"};
    auto observer = projectStorageMock.observers.front();

    EXPECT_CALL(viewMock, nodeTypeChanged(node, Eq("Item"), -1, -1));

    observer->exportedTypeNamesChanged({}, {itemExportedTypeName});
}

TEST_F(Model_MetaInfo, null_meta_info_are_refreshed_if_exported_types_are_updated)
{
    auto node = model.createModelNode("Item");
    projectStorageMock.removeType(qtQuickModuleId, "Item");
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, {});
    auto observer = projectStorageMock.observers.front();
    observer->removedTypeIds({itemTypeId});
    auto itemTypeId2 = projectStorageMock.createObject(qtQuickModuleId, "Item");
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId,
                                                 {qtQuickModuleId, itemTypeId2, "Item"});
    ExportedTypeName item2ExportedTypeName{qtQuickModuleId, itemTypeId2, "Item"};

    observer->exportedTypeNamesChanged({item2ExportedTypeName}, {});

    ASSERT_THAT(node.metaInfo().id(), itemTypeId2);
}

TEST_F(Model_MetaInfo, notify_about_added_exported_type_name)
{
    auto node = model.createModelNode("Item");
    projectStorageMock.removeType(qtQuickModuleId, "Item");
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, {});
    auto observer = projectStorageMock.observers.front();
    observer->removedTypeIds({itemTypeId});
    auto itemTypeId2 = projectStorageMock.createObject(qtQuickModuleId, "Item");
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId,
                                                 {qtQuickModuleId, itemTypeId2, "Item"});
    ExportedTypeName item2ExportedTypeName{qtQuickModuleId, itemTypeId2, "Item"};

    EXPECT_CALL(viewMock, nodeTypeChanged(node, Eq("Item"), -1, -1));

    observer->exportedTypeNamesChanged({item2ExportedTypeName}, {});
}

TEST_F(Model_MetaInfo, empty_exported_type_name_are_refreshed_if_exported_types_are_updated)
{
    auto node = model.createModelNode("Item");
    projectStorageMock.removeType(qtQuickModuleId, "Item");
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, {});
    auto observer = projectStorageMock.observers.front();
    observer->removedTypeIds({itemTypeId});
    auto itemTypeId2 = projectStorageMock.createObject(qtQuickModuleId, "Item");
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId,
                                                 {qtQuickModuleId, itemTypeId2, "Item"});
    ExportedTypeName item2ExportedTypeName{qtQuickModuleId, itemTypeId2, "Item"};

    observer->exportedTypeNamesChanged({item2ExportedTypeName}, {});

    ASSERT_THAT(node.exportedTypeName(),
                IsExportedTypeName(qtQuickModuleId, "Item", HasNoVersion(), itemTypeId2));
}

class Model_TypeAnnotation : public Model
{};

TEST_F(Model_TypeAnnotation, item_library_entries)
{
    using namespace Qt::StringLiterals;
    QmlDesigner::Storage::Info::ItemLibraryEntries storageEntries{{itemTypeId,
                                                                   "Item",
                                                                   "Item",
                                                                   "/path/to/icon",
                                                                   "basic category",
                                                                   "QtQuick",
                                                                   "It's a item",
                                                                   "/path/to/template"}};
    storageEntries.front().properties.emplace_back("x", "double", Sqlite::ValueView::create(1));
    storageEntries.front().extraFilePaths.emplace_back("/extra/file/path");
    projectStorageMock.setItemLibraryEntries(documentSourceId, storageEntries);

    auto entries = model.itemLibraryEntries();

    ASSERT_THAT(entries,
                ElementsAre(
                    IsItemLibraryEntry(itemTypeId,
                                       "Item",
                                       u"Item",
                                       u"/path/to/icon",
                                       u"basic category",
                                       u"QtQuick",
                                       u"It's a item",
                                       u"/path/to/template",
                                       ElementsAre(IsItemLibraryProperty("x", "double"_L1, QVariant{1})),
                                       ElementsAre(u"/extra/file/path"))));
}

TEST_F(Model_TypeAnnotation, directory_imports_item_library_entries)
{
    using namespace Qt::StringLiterals;
    QmlDesigner::Storage::Info::ItemLibraryEntries storageEntries{{itemTypeId,
                                                                   "Item",
                                                                   "Item",
                                                                   "/path/to/icon",
                                                                   "basic category",
                                                                   "QtQuick",
                                                                   "It's a item",
                                                                   "/path/to/template"}};
    storageEntries.front().properties.emplace_back("x", "double", Sqlite::ValueView::create(1));
    storageEntries.front().extraFilePaths.emplace_back("/extra/file/path");
    storageEntries.front().componentSourceId = model.fileUrlSourceId();
    projectStorageMock.setDirectoryImportsItemLibraryEntries(documentSourceId, storageEntries);

    auto entries = model.directoryImportsItemLibraryEntries();

    ASSERT_THAT(entries,
                ElementsAre(
                    IsItemLibraryEntry(itemTypeId,
                                       "Item",
                                       u"Item",
                                       u"/path/to/icon",
                                       u"My Components",
                                       u"",
                                       u"It's a item",
                                       u"/path/to/template",
                                       documentSourceId,
                                       ElementsAre(IsItemLibraryProperty("x", "double"_L1, QVariant{1})),
                                       ElementsAre(u"/extra/file/path"))));
}

TEST_F(Model_TypeAnnotation, directory_imports_item_library_entries_from_sub_directory)
{
    using namespace Qt::StringLiterals;
    QmlDesigner::Storage::Info::ItemLibraryEntries storageEntries{{itemTypeId,
                                                                   "Item",
                                                                   "Item",
                                                                   "/path/to/icon",
                                                                   "basic category",
                                                                   "QtQuick",
                                                                   "It's a item",
                                                                   "/path/to/template"}};
    auto componentSourceId = pathCache.sourceId("/path/bar/bar.qml");
    storageEntries.front().componentSourceId = componentSourceId;
    projectStorageMock.setDirectoryImportsItemLibraryEntries(documentSourceId, storageEntries);

    auto entries = model.directoryImportsItemLibraryEntries();

    ASSERT_THAT(entries,
                ElementsAre(IsItemLibraryEntry(itemTypeId,
                                               "Item",
                                               u"Item",
                                               u"/path/to/icon",
                                               u"bar",
                                               u"",
                                               u"It's a item",
                                               u"/path/to/template",
                                               componentSourceId,
                                               _,
                                               _)));
}

TEST_F(Model_TypeAnnotation, directory_imports_item_library_entries_from_other_directory)
{
    using namespace Qt::StringLiterals;
    QmlDesigner::Storage::Info::ItemLibraryEntries storageEntries{{itemTypeId,
                                                                   "Item",
                                                                   "Item",
                                                                   "/path/to/icon",
                                                                   "basic category",
                                                                   "QtQuick",
                                                                   "It's a item",
                                                                   "/path/to/template"}};
    auto componentSourceId = pathCache.sourceId("/bar/bar.qml");
    storageEntries.front().componentSourceId = componentSourceId;
    projectStorageMock.setDirectoryImportsItemLibraryEntries(documentSourceId, storageEntries);

    auto entries = model.directoryImportsItemLibraryEntries();

    ASSERT_THAT(entries,
                ElementsAre(IsItemLibraryEntry(itemTypeId,
                                               "Item",
                                               u"Item",
                                               u"/path/to/icon",
                                               u"../bar",
                                               u"",
                                               u"It's a item",
                                               u"/path/to/template",
                                               componentSourceId,
                                               _,
                                               _)));
}

TEST_F(Model_TypeAnnotation, all_item_library_entries)
{
    using namespace Qt::StringLiterals;
    QmlDesigner::Storage::Info::ItemLibraryEntries storageEntries{{itemTypeId,
                                                                   "Item",
                                                                   "Item",
                                                                   "/path/to/icon",
                                                                   "basic category",
                                                                   "QtQuick",
                                                                   "It's a item",
                                                                   "/path/to/template"}};
    storageEntries.front().properties.emplace_back("x", "double", Sqlite::ValueView::create(1));
    storageEntries.front().extraFilePaths.emplace_back("/extra/file/path");
    ON_CALL(projectStorageMock, allItemLibraryEntries()).WillByDefault(Return(storageEntries));

    auto entries = model.allItemLibraryEntries();

    ASSERT_THAT(entries,
                ElementsAre(
                    IsItemLibraryEntry(itemTypeId,
                                       "Item",
                                       u"Item",
                                       u"/path/to/icon",
                                       u"basic category",
                                       u"QtQuick",
                                       u"It's a item",
                                       u"/path/to/template",
                                       ElementsAre(IsItemLibraryProperty("x", "double"_L1, QVariant{1})),
                                       ElementsAre(u"/extra/file/path"))));
}

class Model_ViewManagement : public Model
{
protected:
    NiceMock<AbstractViewMock> viewMock;
};

TEST_F(Model_ViewManagement, set_rewriter)
{
    NiceMock<ExternalDependenciesMock> externalDependenciesMock;
    QmlDesigner::RewriterView rewriter{externalDependenciesMock, modulesStorage};

    model.setRewriterView(&rewriter);

    ASSERT_THAT(model.rewriterView(), Eq(&rewriter));
}

TEST_F(Model_ViewManagement, attach_rewriter)
{
    NiceMock<ExternalDependenciesMock> externalDependenciesMock;
    QmlDesigner::RewriterView rewriter{externalDependenciesMock, modulesStorage};

    model.attachView(&rewriter);

    ASSERT_THAT(model.rewriterView(), Eq(&rewriter));
}

TEST_F(Model_ViewManagement, set_node_instance_view)
{
    viewMock.setKind(AbstractView::Kind::NodeInstance);

    model.setNodeInstanceView(&viewMock);

    ASSERT_THAT(model.nodeInstanceView(), Eq(&viewMock));
}

TEST_F(Model_ViewManagement, call_modelAttached_if_node_instance_view_is_set)
{
    viewMock.setKind(AbstractView::Kind::NodeInstance);

    EXPECT_CALL(viewMock, modelAttached(&model));

    model.setNodeInstanceView(&viewMock);
}

TEST_F(Model_ViewManagement, dont_call_modelAttached_if_node_instance_view_is_already_set)
{
    viewMock.setKind(AbstractView::Kind::NodeInstance);
    model.setNodeInstanceView(&viewMock);

    EXPECT_CALL(viewMock, modelAttached(&model)).Times(0);

    model.setNodeInstanceView(&viewMock);
}

TEST_F(Model_ViewManagement, detach_node_instance_view_from_other_model_before_attach_to_new_model)
{
    InSequence s;
    QmlDesigner::Model otherModel{projectStorageDependencies,
                                  "Item",
                                  imports,
                                  fileUrl,
                                  std::make_unique<ModelResourceManagementMockWrapper>(
                                      resourceManagementMock)};
    viewMock.setKind(AbstractView::Kind::NodeInstance);
    otherModel.setNodeInstanceView(&viewMock);

    EXPECT_CALL(viewMock, modelAboutToBeDetached(&otherModel));
    EXPECT_CALL(viewMock, modelAttached(&model));

    model.setNodeInstanceView(&viewMock);
}

TEST_F(Model_ViewManagement, call_modelAboutToBeDetached_for_already_set_node_instance_view)
{
    NiceMock<AbstractViewMock> otherViewMock;
    otherViewMock.setKind(AbstractView::Kind::NodeInstance);
    viewMock.setKind(AbstractView::Kind::NodeInstance);
    model.setNodeInstanceView(&otherViewMock);

    EXPECT_CALL(otherViewMock, modelAboutToBeDetached(&model));

    model.setNodeInstanceView(&viewMock);
}

TEST_F(Model_ViewManagement, attach_view_is_calling_modelAttached)
{
    EXPECT_CALL(viewMock, modelAttached(&model));

    model.attachView(&viewMock);
}

TEST_F(Model_ViewManagement, attach_view_is_not_calling_modelAttached_if_it_is_already_attached)
{
    model.attachView(&viewMock);

    EXPECT_CALL(viewMock, modelAttached(&model)).Times(0);

    model.attachView(&viewMock);
}

TEST_F(Model_ViewManagement, view_is_detached_before_it_is_attached_ot_new_model)
{
    InSequence s;
    QmlDesigner::Model otherModel{projectStorageDependencies,
                                  "Item",
                                  imports,
                                  fileUrl,
                                  std::make_unique<ModelResourceManagementMockWrapper>(
                                      resourceManagementMock)};
    otherModel.attachView(&viewMock);

    EXPECT_CALL(viewMock, modelAboutToBeDetached(&otherModel));
    EXPECT_CALL(viewMock, modelAttached(&model));

    model.attachView(&viewMock);
}

class Model_FileUrl : public Model
{
protected:
    QmlDesigner::SourcePath barFilePath = "/path/bar.qml";
    QUrl barFilePathUrl = barFilePath.toQString();
    SourceId barSourceId = pathCache.sourceId(barFilePath);
    QmlDesigner::SourcePath windowsFilePath = "c:/path/bar.qml";
    QUrl windowsFilePathUrl = windowsFilePath.toQString();
    SourceId windowsSourceId = pathCache.sourceId(windowsFilePath);
};

TEST_F(Model_FileUrl, set_file_url)
{
    model.setFileUrl(barFilePathUrl);

    ASSERT_THAT(model.fileUrl(), barFilePathUrl);
}

TEST_F(Model_FileUrl, set_windows_file_url)
{
    model.setFileUrl(windowsFilePathUrl);

    ASSERT_THAT(model.fileUrl(), windowsFilePathUrl);
}

TEST_F(Model_FileUrl, set_file_url_sets_source_id_too)
{
    model.setFileUrl(barFilePathUrl);

    ASSERT_THAT(model.fileUrlSourceId(), barSourceId);
}

TEST_F(Model_FileUrl, set_windows_file_url_sets_source_id_too)
{
    model.setFileUrl(windowsFilePathUrl);

    ASSERT_THAT(model.fileUrlSourceId(), windowsSourceId);
}

TEST_F(Model_FileUrl, notifies_change)
{
    EXPECT_CALL(viewMock, fileUrlChanged(Eq(fileUrl), Eq(barFilePathUrl)));

    model.setFileUrl(barFilePathUrl);
}

TEST_F(Model_FileUrl, do_not_notify_if_there_is_no_change)
{
    EXPECT_CALL(viewMock, fileUrlChanged(_, _)).Times(0);

    model.setFileUrl(fileUrl);
}

} // namespace
