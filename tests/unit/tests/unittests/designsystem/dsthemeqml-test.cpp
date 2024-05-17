// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <bindingproperty.h>
#include <model.h>
#include <nodeproperty.h>
#include <projectstoragemock.h>
#include <sourcepathcachemock.h>
#include <variantproperty.h>

#include <designsystem/dsthemegroup.h>
#include <designsystem/dsthememanager.h>

#include <format>
#include <functional>

using QmlDesigner::DSThemeManager;
using QmlDesigner::GroupType;
using QmlDesigner::Import;
using QmlDesigner::ModelNode;
using QmlDesigner::ThemeProperty;

template<>
struct std::formatter<QByteArray> : std::formatter<std::string>
{
    template<typename FormatContext>
    auto format(const QByteArray &ba, FormatContext &ctx) const
    {
        return std::formatter<std::string>::format(ba.toStdString(), ctx);
    }
};

namespace {

static std::string formatedPropStr(const char tag[], const char name[], const QVariant &v)
{
    return std::format("{}Property({}, {})", tag, name, v.toString().toStdString());
}

static auto bindingPropStr = std::bind(&formatedPropStr,
                                       "Binding",
                                       std::placeholders::_1,
                                       std::placeholders::_2);
static auto variantPropStr = std::bind(&formatedPropStr,
                                       "Variant",
                                       std::placeholders::_1,
                                       std::placeholders::_2);

constexpr const char testPropertyName1[] = "prop1";
constexpr const char darkThemeName[] = "dark";
constexpr QmlDesigner::ThemeId testThemeId = 1;

MATCHER_P2(HasNodeProperty,
           name,
           typeName,
           std::format("{} have node {} with type {})", (negation ? "Does't " : "Does "), name, typeName))
{
    ModelNode n = arg;
    return n.hasNodeProperty(name) && n.nodeProperty(name).modelNode().isValid()
           && n.nodeProperty(name).modelNode().type() == typeName;
}

MATCHER_P2(HasBindingProperty,
           name,
           value,
           std::format("{} have {})", (negation ? "Does't " : "Does "), bindingPropStr(name, value)))
{
    ModelNode n = arg;
    return n.hasBindingProperty(name) && n.bindingProperty(name).expression() == value;
}

MATCHER_P2(HasVariantProperty,
           name,
           value,
           std::format("{} have {})", (negation ? "Does't " : "Does "), variantPropStr(name, value)))
{
    ModelNode n = arg;
    return n.hasVariantProperty(name) && n.variantProperty(name).value() == value;
}

MATCHER_P2(HasGroupVariantProperty,
           groupName,
           themeProp,
           std::format("{} have node {} with {})",
                       (negation ? "Does't " : "Does "),
                       groupName.constData(),
                       PrintToString(themeProp)))
{
    ModelNode n = arg;

    ModelNode groupNode = n.nodeProperty(groupName).modelNode();

    return groupNode.isValid() && groupNode.hasVariantProperty(themeProp.name)
           && groupNode.variantProperty(themeProp.name).value() == themeProp.value;
}

MATCHER_P2(HasGroupBindingProperty,
           groupName,
           themeProp,
           std::format("{} have node {} with {})",
                       (negation ? "Does't " : "Does "),
                       groupName.constData(),
                       PrintToString(themeProp)))
{
    ModelNode n = arg;

    ModelNode groupNode = n.nodeProperty(groupName).modelNode();

    return groupNode.isValid() && groupNode.hasBindingProperty(themeProp.name)
           && groupNode.bindingProperty(themeProp.name).expression() == themeProp.value.toString();
}

class DesignSystemQmlTest : public testing::TestWithParam<QmlDesigner::GroupType>
{
protected:
    DesignSystemQmlTest()
        : group(groupType)
    {}

    const QmlDesigner::GroupType groupType = GetParam();
    const QmlDesigner::PropertyName groupName = GroupId(groupType);
    QmlDesigner::DSThemeGroup group;
    NiceMock<SourcePathCacheMockWithPaths> pathCacheMock{"/path/model.qm"};
    NiceMock<ProjectStorageMockWithQtQuick> projectStorageMock{pathCacheMock.sourceId, "/path"};
    QmlDesigner::Model model{{projectStorageMock, pathCacheMock},
                             "QtObject",
                             {Import::createLibraryImport("QM"),
                              Import::createLibraryImport("QtQuick")},
                             QUrl::fromLocalFile(pathCacheMock.path.toQString())};
};

INSTANTIATE_TEST_SUITE_P(DesignSystem,
                         DesignSystemQmlTest,
                         testing::Values(QmlDesigner::GroupType::Colors,
                                         QmlDesigner::GroupType::Flags,
                                         QmlDesigner::GroupType::Numbers,
                                         QmlDesigner::GroupType::Strings));

TEST_P(DesignSystemQmlTest, group_aliase_properties_are_generated)
{
    // arrange
    ThemeProperty testProp{testPropertyName1, "test", false};
    DSThemeManager mgr;
    mgr.addTheme(darkThemeName);
    mgr.addProperty(groupType, testProp);
    ModelNode rootNode = model.rootModelNode();
    QString binding = QString("currentTheme.%1").arg(QString::fromLatin1(groupName));

    // act
    mgr.decorate(rootNode);

    // assert
    ASSERT_THAT(rootNode,
                AllOf(Property(&ModelNode::type, Eq("QtObject")),
                      HasBindingProperty(groupName, binding),
                      HasBindingProperty("currentTheme", darkThemeName),
                      HasNodeProperty(darkThemeName, "QtObject")));
}

TEST_P(DesignSystemQmlTest, empty_groups_generate_no_group_aliase_properties)
{
    // arrange
    DSThemeManager mgr;
    ModelNode rootNode = model.rootModelNode();
    QString binding = QString("currentTheme.%1").arg(QString::fromLatin1(groupName));

    // act
    mgr.decorate(rootNode);

    // assert
    ASSERT_THAT(rootNode,
                AllOf(Property(&ModelNode::type, Eq("QtObject")),
                      Not(HasBindingProperty(groupName, binding)),
                      Not(HasBindingProperty("currentTheme", darkThemeName)),
                      Not(HasNodeProperty(darkThemeName, "QtObject"))));
}

TEST_P(DesignSystemQmlTest, decorate_appends_binding_property_to_group_node)
{
    // arrange
    ThemeProperty testProp{testPropertyName1, "width", true};
    group.addProperty(testThemeId, testProp);
    ModelNode rootNode = model.rootModelNode();

    // act
    group.decorate(testThemeId, rootNode);

    // assert
    ASSERT_THAT(rootNode,
                AllOf(HasNodeProperty(groupName, "QtObject"),
                      HasGroupBindingProperty(groupName, testProp)));
}

TEST_P(DesignSystemQmlTest, mcu_flag_decorate_appends_binding_property_to_root_node)
{
    // arrange
    ThemeProperty testProp{testPropertyName1, "width", true};
    group.addProperty(testThemeId, testProp);
    ModelNode rootNode = model.rootModelNode();

    // act
    group.decorate(testThemeId, rootNode, false);

    // assert
    ASSERT_THAT(rootNode,
                AllOf(Not(HasNodeProperty(groupName, "QtObject")),
                      HasBindingProperty(testProp.name, testProp.value)));
}

TEST_P(DesignSystemQmlTest, decorate_appends_variant_property_to_group_node)
{
    // arrange
    ThemeProperty testProp{testPropertyName1, 5, false};
    group.addProperty(testThemeId, testProp);
    ModelNode rootNode = model.rootModelNode();

    // act
    group.decorate(testThemeId, rootNode);

    // assert
    ASSERT_THAT(rootNode,
                AllOf(HasNodeProperty(groupName, "QtObject"),
                      HasGroupVariantProperty(groupName, testProp)));
}

TEST_P(DesignSystemQmlTest, mcu_flag_decorate_appends_variant_property_to_root_node)
{
    // arrange
    ThemeProperty testProp{testPropertyName1, 5, false};
    group.addProperty(testThemeId, testProp);
    ModelNode rootNode = model.rootModelNode();

    // act
    group.decorate(testThemeId, rootNode, false);

    // assert
    ASSERT_THAT(rootNode,
                AllOf(Not(HasNodeProperty(groupName, "QtObject")),
                      HasVariantProperty(testProp.name, testProp.value)));
}

TEST_P(DesignSystemQmlTest, empty_group_decorate_adds_no_property)
{
    // arrange
    ModelNode rootNode = model.rootModelNode();

    // act
    group.decorate(testThemeId, rootNode);

    // assert
    ASSERT_THAT(rootNode, Not(HasNodeProperty(groupName, "QtObject")));
}
} // namespace
