// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <googletest.h>

#include <mocks/abstractviewmock.h>
#include <mocks/projectstoragemock.h>
#include <mocks/sourcepathcachemock.h>
#include <modelutils.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>

namespace {
using QmlDesigner::ModelNode;
using QmlDesigner::ModelNodes;
using QmlDesigner::Storage::ModuleKind;

class ModelUtilsWithModel : public ::testing::Test
{
protected:
    NiceMock<SourcePathCacheMockWithPaths> pathCacheMock{"/path/model.qml"};
    QmlDesigner::SourceId sourceId = pathCacheMock.createSourceId("/path/foo.qml");
    NiceMock<ProjectStorageMockWithQtQuick> projectStorageMock{pathCacheMock.sourceId, "/path"};
    QmlDesigner::ModuleId moduleId = projectStorageMock.moduleId("QtQuick", ModuleKind::QmlLibrary);
    QmlDesigner::Model model{{projectStorageMock, pathCacheMock},
                             "Item",
                             {QmlDesigner::Import::createLibraryImport("QML"),
                              QmlDesigner::Import::createLibraryImport("QtQuick"),
                              QmlDesigner::Import::createLibraryImport("QtQml.Models")},
                             QUrl::fromLocalFile(pathCacheMock.path.toQString())};
};

TEST_F(ModelUtilsWithModel, component_file_path)
{
    QmlDesigner::Storage::TypeTraits traits{QmlDesigner::Storage::TypeTraitsKind::Reference};
    traits.isFileComponent = true;
    auto typeId = projectStorageMock.createType(moduleId, "Foo", traits, {}, sourceId);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    auto path = QmlDesigner::ModelUtils::componentFilePath(pathCacheMock, metaInfo);

    ASSERT_THAT(path, "/path/foo.qml");
}

TEST_F(ModelUtilsWithModel, empty_component_file_path_for_non_file_component)
{
    auto typeId = projectStorageMock.createType(moduleId, "Foo", {}, {}, sourceId);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    auto path = QmlDesigner::ModelUtils::componentFilePath(pathCacheMock, metaInfo);

    ASSERT_THAT(path, IsEmpty());
}

TEST_F(ModelUtilsWithModel, empty_component_file_path_for_invalid_meta_info)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    auto path = QmlDesigner::ModelUtils::componentFilePath(pathCacheMock, metaInfo);

    ASSERT_THAT(path, IsEmpty());
}

TEST_F(ModelUtilsWithModel, component_file_path_for_node)
{
    QmlDesigner::Storage::TypeTraits traits{QmlDesigner::Storage::TypeTraitsKind::Reference};
    traits.isFileComponent = true;
    auto typeId = projectStorageMock.createType(moduleId, "Foo", traits, {}, sourceId);
    projectStorageMock.createImportedTypeNameId(pathCacheMock.sourceId, "Foo", typeId);
    auto node = model.createModelNode("Foo");

    auto path = QmlDesigner::ModelUtils::componentFilePath(node);

    ASSERT_THAT(path, "/path/foo.qml");
}

TEST_F(ModelUtilsWithModel, component_file_path_for_invalid_node_is_empty)
{
    auto path = QmlDesigner::ModelUtils::componentFilePath(QmlDesigner::ModelNode{});

    ASSERT_THAT(path, IsEmpty());
}

TEST_F(ModelUtilsWithModel, component_file_path_for_node_without_metainfo_is_empty)
{
    QmlDesigner::Storage::TypeTraits traits{QmlDesigner::Storage::TypeTraitsKind::Reference};
    traits.isFileComponent = true;
    projectStorageMock.createType(moduleId, "Foo", traits, {}, sourceId);
    auto node = model.createModelNode("Foo");

    auto path = QmlDesigner::ModelUtils::componentFilePath(node);

    ASSERT_THAT(path, IsEmpty());
}

TEST_F(ModelUtilsWithModel, component_file_path_for_non_file_component_node_is_empty)
{
    auto typeId = projectStorageMock.createType(moduleId, "Foo", {}, {}, sourceId);
    projectStorageMock.createImportedTypeNameId(pathCacheMock.sourceId, "Foo", typeId);
    auto node = model.createModelNode("Foo");

    auto path = QmlDesigner::ModelUtils::componentFilePath(node);

    ASSERT_THAT(path, IsEmpty());
}

TEST_F(ModelUtilsWithModel, find_lowest_common_ancestor)
{
    auto child1 = model.createModelNode("Item");
    auto child2 = model.createModelNode("Item");
    model.rootModelNode().defaultNodeAbstractProperty().reparentHere(child1);
    model.rootModelNode().defaultNodeAbstractProperty().reparentHere(child2);
    ModelNodes nodes{child1, child2};

    auto commonAncestor = QmlDesigner::ModelUtils::lowestCommonAncestor(nodes);

    ASSERT_THAT(commonAncestor, model.rootModelNode());
}

TEST_F(ModelUtilsWithModel, lowest_common_ancestor_return_invalid_node_if_argument_is_invalid)
{
    auto child1 = model.createModelNode("Item");
    auto child2 = ModelNode{};
    model.rootModelNode().defaultNodeAbstractProperty().reparentHere(child1);
    ModelNodes nodes{child1, child2};

    auto commonAncestor = QmlDesigner::ModelUtils::lowestCommonAncestor(nodes);

    ASSERT_THAT(commonAncestor, Not(IsValid()));
}

TEST_F(ModelUtilsWithModel, find_lowest_common_ancestor_when_one_of_the_nodes_is_parent)
{
    auto parentNode = model.createModelNode("Item");
    auto childNode = model.createModelNode("Item");
    parentNode.defaultNodeAbstractProperty().reparentHere(childNode);
    model.rootModelNode().defaultNodeAbstractProperty().reparentHere(parentNode);
    ModelNodes nodes{childNode, parentNode};

    auto commonAncestor = QmlDesigner::ModelUtils::lowestCommonAncestor(nodes);

    ASSERT_THAT(commonAncestor, parentNode);
}

TEST_F(ModelUtilsWithModel, lowest_common_ancestor_for_uncle_and_nephew_should_return_the_grandfather)
{
    auto grandFatherNode = model.createModelNode("Item");
    auto fatherNode = model.createModelNode("Item");
    auto uncleNode = model.createModelNode("Item");
    auto nephewNode = model.createModelNode("Item");
    fatherNode.defaultNodeAbstractProperty().reparentHere(nephewNode);
    grandFatherNode.defaultNodeAbstractProperty().reparentHere(fatherNode);
    grandFatherNode.defaultNodeAbstractProperty().reparentHere(uncleNode);
    model.rootModelNode().defaultNodeAbstractProperty().reparentHere(grandFatherNode);
    ModelNodes nodes{uncleNode, nephewNode};

    auto commonAncestor = QmlDesigner::ModelUtils::lowestCommonAncestor(nodes);

    ASSERT_THAT(commonAncestor, grandFatherNode);
}

TEST(ModelUtils, isValidQmlIdentifier_fails_on_empty_values)
{
    QStringView emptyValue = u"";

    bool isValidQmlIdentifier = QmlDesigner::ModelUtils::isValidQmlIdentifier(emptyValue);

    ASSERT_FALSE(isValidQmlIdentifier);
}

TEST(ModelUtils, isValidQmlIdentifier_fails_on_upper_case_first_letter)
{
    QStringView id = u"Lmn";

    bool isValidQmlIdentifier = QmlDesigner::ModelUtils::isValidQmlIdentifier(id);

    ASSERT_FALSE(isValidQmlIdentifier);
}

TEST(ModelUtils, isValidQmlIdentifier_fails_on_digital_first_letter)
{
    QStringView id = u"6mn";

    bool isValidQmlIdentifier = QmlDesigner::ModelUtils::isValidQmlIdentifier(id);

    ASSERT_FALSE(isValidQmlIdentifier);
}

TEST(ModelUtils, isValidQmlIdentifier_fails_on_unicodes)
{
    QStringView id = u"sähköverkko";

    bool isValidQmlIdentifier = QmlDesigner::ModelUtils::isValidQmlIdentifier(id);

    ASSERT_FALSE(isValidQmlIdentifier);
}

TEST(ModelUtils, isValidQmlIdentifier_passes_on_lower_case_first_letter)
{
    QStringView id = u"mn";

    bool isValidQmlIdentifier = QmlDesigner::ModelUtils::isValidQmlIdentifier(id);

    ASSERT_TRUE(isValidQmlIdentifier);
}

TEST(ModelUtils, isValidQmlIdentifier_passes_on_underscored_first_letter)
{
    QStringView id = u"_m";

    bool isValidQmlIdentifier = QmlDesigner::ModelUtils::isValidQmlIdentifier(id);

    ASSERT_TRUE(isValidQmlIdentifier);
}

TEST(ModelUtils, isValidQmlIdentifier_passes_on_digital_non_first_letter)
{
    QStringView id = u"_6";

    bool isValidQmlIdentifier = QmlDesigner::ModelUtils::isValidQmlIdentifier(id);

    ASSERT_TRUE(isValidQmlIdentifier);
}

TEST(ModelUtils, isValidQmlIdentifier_passes_on_upper_case_non_first_letter)
{
    QStringView id = u"mN";

    bool isValidQmlIdentifier = QmlDesigner::ModelUtils::isValidQmlIdentifier(id);

    ASSERT_TRUE(isValidQmlIdentifier);
}

TEST(ModelUtils, isValidQmlIdentifier_passes_on_underscore_only)
{
    QStringView id = u"_";

    bool isValidQmlIdentifier = QmlDesigner::ModelUtils::isValidQmlIdentifier(id);

    ASSERT_TRUE(isValidQmlIdentifier);
}

class IsQmlKeywords : public testing::TestWithParam<QStringView>
{};

INSTANTIATE_TEST_SUITE_P(ModelUtils,
                         IsQmlKeywords,
                         testing::Values(u"alias",
                                         u"as",
                                         u"break",
                                         u"case",
                                         u"catch",
                                         u"continue",
                                         u"debugger",
                                         u"default",
                                         u"delete",
                                         u"do",
                                         u"else",
                                         u"finally",
                                         u"for",
                                         u"function",
                                         u"if",
                                         u"import",
                                         u"in",
                                         u"instanceof",
                                         u"new",
                                         u"print",
                                         u"return",
                                         u"switch",
                                         u"this",
                                         u"throw",
                                         u"try",
                                         u"typeof",
                                         u"var",
                                         u"void",
                                         u"while",
                                         u"with"));

TEST_P(IsQmlKeywords, is_qml_keyword)
{
    QStringView id = GetParam();

    bool isQmlKeyword = QmlDesigner::ModelUtils::isQmlKeyword(id);

    ASSERT_THAT(isQmlKeyword, IsTrue());
}

TEST(ModelUtils, is_not_qml_keyword)
{
    QStringView id = u"foo";

    bool isQmlKeyword = QmlDesigner::ModelUtils::isQmlKeyword(id);

    ASSERT_THAT(isQmlKeyword, IsFalse());
}

TEST(ModelUtils, empty_is_not_qml_keyword)
{
    QStringView id = u"";

    bool isQmlKeyword = QmlDesigner::ModelUtils::isQmlKeyword(id);

    ASSERT_THAT(isQmlKeyword, IsFalse());
}

class IsDiscouragedQmlId : public testing::TestWithParam<QStringView>
{};

INSTANTIATE_TEST_SUITE_P(ModelUtils,
                         IsDiscouragedQmlId,
                         testing::Values(u"action",
                                         u"anchors",
                                         u"baseState",
                                         u"border",
                                         u"bottom",
                                         u"clip",
                                         u"data",
                                         u"enabled",
                                         u"flow",
                                         u"focus",
                                         u"font",
                                         u"height",
                                         u"id",
                                         u"item",
                                         u"layer",
                                         u"left",
                                         u"margin",
                                         u"opacity",
                                         u"padding",
                                         u"parent",
                                         u"right",
                                         u"scale",
                                         u"shaderInfo",
                                         u"source",
                                         u"sprite",
                                         u"spriteSequence",
                                         u"state",
                                         u"text",
                                         u"texture",
                                         u"time",
                                         u"top",
                                         u"visible",
                                         u"width",
                                         u"x",
                                         u"y",
                                         u"z"));

TEST_P(IsDiscouragedQmlId, is_discouraged_Qml_id)
{
    QStringView id = GetParam();

    bool isDiscouragedQmlId = QmlDesigner::ModelUtils::isDiscouragedQmlId(id);

    ASSERT_THAT(isDiscouragedQmlId, IsTrue());
}

TEST(ModelUtils, is_not_discouraged_Qml_id)
{
    QStringView id = u"foo";

    bool isDiscouragedQmlId = QmlDesigner::ModelUtils::isDiscouragedQmlId(id);

    ASSERT_THAT(isDiscouragedQmlId, IsFalse());
}

TEST(ModelUtils, empty_is_not_discouraged_Qml_id)
{
    QStringView id = u"";

    bool isDiscouragedQmlId = QmlDesigner::ModelUtils::isDiscouragedQmlId(id);

    ASSERT_THAT(isDiscouragedQmlId, IsFalse());
}

class IsQmlBuiltinType : public testing::TestWithParam<QStringView>
{};

INSTANTIATE_TEST_SUITE_P(ModelUtils,
                         IsQmlBuiltinType,
                         testing::Values(u"bool",
                                         u"color",
                                         u"date",
                                         u"double",
                                         u"enumeration",
                                         u"font",
                                         u"int",
                                         u"list",
                                         u"matrix4x4",
                                         u"point",
                                         u"quaternion",
                                         u"real",
                                         u"rect",
                                         u"size",
                                         u"string",
                                         u"url",
                                         u"var",
                                         u"variant",
                                         u"vector",
                                         u"vector2d",
                                         u"vector3d",
                                         u"vector4d"));

TEST_P(IsQmlBuiltinType, is_Qml_builtin_type)
{
    QStringView id = GetParam();

    bool isQmlBuiltinType = QmlDesigner::ModelUtils::isQmlBuiltinType(id);

    ASSERT_THAT(isQmlBuiltinType, IsTrue());
}

TEST(ModelUtils, is_not_Qml_builtin_type)
{
    QStringView id = u"foo";

    bool isQmlBuiltinType = QmlDesigner::ModelUtils::isQmlBuiltinType(id);

    ASSERT_THAT(isQmlBuiltinType, IsFalse());
}

TEST(ModelUtils, empty_is_not_Qml_builtin_type)
{
    QStringView id = u"";

    bool isQmlBuiltinType = QmlDesigner::ModelUtils::isQmlBuiltinType(id);

    ASSERT_THAT(isQmlBuiltinType, IsFalse());
}

class IsBannedQmlId : public testing::TestWithParam<QStringView>
{};

INSTANTIATE_TEST_SUITE_P(ModelUtils,
                         IsBannedQmlId,
                         testing::Values(u"alias",
                                         u"as",
                                         u"break",
                                         u"case",
                                         u"catch",
                                         u"continue",
                                         u"debugger",
                                         u"default",
                                         u"delete",
                                         u"do",
                                         u"else",
                                         u"finally",
                                         u"for",
                                         u"function",
                                         u"if",
                                         u"import",
                                         u"in",
                                         u"instanceof",
                                         u"new",
                                         u"print",
                                         u"return",
                                         u"switch",
                                         u"this",
                                         u"throw",
                                         u"try",
                                         u"typeof",
                                         u"var",
                                         u"void",
                                         u"while",
                                         u"with",
                                         u"action",
                                         u"anchors",
                                         u"baseState",
                                         u"border",
                                         u"bottom",
                                         u"clip",
                                         u"data",
                                         u"enabled",
                                         u"flow",
                                         u"focus",
                                         u"font",
                                         u"height",
                                         u"id",
                                         u"item",
                                         u"layer",
                                         u"left",
                                         u"margin",
                                         u"opacity",
                                         u"padding",
                                         u"parent",
                                         u"right",
                                         u"scale",
                                         u"shaderInfo",
                                         u"source",
                                         u"sprite",
                                         u"spriteSequence",
                                         u"state",
                                         u"text",
                                         u"texture",
                                         u"time",
                                         u"top",
                                         u"visible",
                                         u"width",
                                         u"x",
                                         u"y",
                                         u"z",
                                         u"bool",
                                         u"color",
                                         u"date",
                                         u"double",
                                         u"enumeration",
                                         u"font",
                                         u"int",
                                         u"list",
                                         u"matrix4x4",
                                         u"point",
                                         u"quaternion",
                                         u"real",
                                         u"rect",
                                         u"size",
                                         u"string",
                                         u"url",
                                         u"var",
                                         u"variant",
                                         u"vector",
                                         u"vector2d",
                                         u"vector3d",
                                         u"vector4d"));

TEST_P(IsBannedQmlId, is_banned_Qml_id)
{
    QStringView id = GetParam();

    bool isBannedQmlId = QmlDesigner::ModelUtils::isBannedQmlId(id);

    ASSERT_THAT(isBannedQmlId, IsTrue());
}

TEST(ModelUtils, is_not_banned_Qml_id)
{
    QStringView id = u"foo";

    bool isBannedQmlId = QmlDesigner::ModelUtils::isBannedQmlId(id);

    ASSERT_THAT(isBannedQmlId, IsFalse());
}

TEST(ModelUtils, empty_is_not_banned_Qml_id)
{
    QStringView id = u"";

    bool isBannedQmlId = QmlDesigner::ModelUtils::isBannedQmlId(id);

    ASSERT_THAT(isBannedQmlId, IsFalse());
}

TEST(ModelUtils, expressionToList_empty_expression_returns_empty_list)
{
    QString expression = "";

    QStringList list = QmlDesigner::ModelUtils::expressionToList(expression);

    ASSERT_THAT(list, IsEmpty());
}

TEST(ModelUtils, expressionToList_empty_array_returns_empty_list)
{
    QString expression = "[]";

    QStringList list = QmlDesigner::ModelUtils::expressionToList(expression);

    ASSERT_THAT(list, IsEmpty());
}

TEST(ModelUtils, expressionToList_comma_only_array_returns_empty_list)
{
    QString expression = "[,,,]";

    QStringList list = QmlDesigner::ModelUtils::expressionToList(expression);

    ASSERT_THAT(list, IsEmpty());
}

TEST(ModelUtils, expressionToList_space_only_array_returns_empty_list)
{
    QString expression = "[ , , , ]";

    QStringList list = QmlDesigner::ModelUtils::expressionToList(expression);

    ASSERT_THAT(list, IsEmpty());
}

TEST(ModelUtils, expressionToList_single_expression_returns_single_item_list)
{
    QString expression = "aaa";

    QStringList list = QmlDesigner::ModelUtils::expressionToList(expression);

    ASSERT_THAT(list, UnorderedElementsAre("aaa"));
}

TEST(ModelUtils, expressionToList_single_expression_keeps_middle_spaces)
{
    QString expression = "aa a  b";

    QStringList list = QmlDesigner::ModelUtils::expressionToList(expression);

    ASSERT_THAT(list, UnorderedElementsAre("aa a  b"));
}

TEST(ModelUtils, expressionToList_single_expression_omites_side_spaces)
{
    QString expression = "  aa a  b ";

    QStringList list = QmlDesigner::ModelUtils::expressionToList(expression);

    ASSERT_THAT(list, UnorderedElementsAre("aa a  b"));
}

TEST(ModelUtils, expressionToList_single_item_array_returns_single_item_list)
{
    QString expression = "[aaa]";

    QStringList list = QmlDesigner::ModelUtils::expressionToList(expression);

    ASSERT_THAT(list, UnorderedElementsAre("aaa"));
}

TEST(ModelUtils, expressionToList_array_with_multiple_items_returns_all)
{
    QString expression = "[bbb,aaa,ccc]";

    QStringList list = QmlDesigner::ModelUtils::expressionToList(expression);

    ASSERT_THAT(list, UnorderedElementsAre("bbb", "aaa", "ccc"));
}

TEST(ModelUtils, expressionToList_array_with_empty_items_returns_clean_list)
{
    QString expression = "[,aaa,,bbb,ccc,,]";

    QStringList list = QmlDesigner::ModelUtils::expressionToList(expression);

    ASSERT_THAT(list, UnorderedElementsAre("aaa", "bbb", "ccc"));
}

TEST(ModelUtils, expressionToList_keeps_middle_spaces_in_tokens)
{
    QString expression = "[aaa,,a bb  b,ccc]";

    QStringList list = QmlDesigner::ModelUtils::expressionToList(expression);

    ASSERT_THAT(list, UnorderedElementsAre("aaa", "a bb  b", "ccc"));
}

TEST(ModelUtils, expressionToList_omits_side_spaces_in_tokens)
{
    QString expression = "[aaa,,  a bb  b ,ccc]";

    QStringList list = QmlDesigner::ModelUtils::expressionToList(expression);

    ASSERT_THAT(list, UnorderedElementsAre("aaa", "a bb  b", "ccc"));
}

TEST(ModelUtils, expressionToList_unicodes_supported)
{
    QString expression = "[你好, foo]";

    QStringList list = QmlDesigner::ModelUtils::expressionToList(expression);

    ASSERT_THAT(list, UnorderedElementsAre("你好", "foo"));
}

TEST(ModelUtils, listToExpression_returns_non_array_expression_for_single_items)
{
    QStringList list = {"aaa"};

    QString expression = QmlDesigner::ModelUtils::listToExpression(list);

    ASSERT_THAT(expression, Eq("aaa"));
}

TEST(ModelUtils, listToExpression_returns_expression)
{
    QStringList list = {"aaa", "bbb", "ccc"};

    QString expression = QmlDesigner::ModelUtils::listToExpression(list);

    ASSERT_THAT(expression, Eq("[aaa,bbb,ccc]"));
}

TEST(ModelUtils, listToExpression_returns_expression_with_empty_items)
{
    QStringList list = {"aaa", "bbb", "", "ccc"};

    QString expression = QmlDesigner::ModelUtils::listToExpression(list);

    ASSERT_THAT(expression, Eq("[aaa,bbb,,ccc]"));
}

TEST(ModelUtils, listToExpression_returns_empty_for_empty_expressions)
{
    QStringList list = {};

    QString expression = QmlDesigner::ModelUtils::listToExpression(list);

    ASSERT_THAT(expression, IsEmpty());
}

} // namespace
