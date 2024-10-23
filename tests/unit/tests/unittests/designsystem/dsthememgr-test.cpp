// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <bindingproperty.h>
#include <model.h>
#include <nodeproperty.h>
#include <projectstoragemock.h>
#include <sourcepathcachemock.h>

#include <designsystem/dsthememanager.h>

using QmlDesigner::DSThemeManager;
using QmlDesigner::GroupType;
using QmlDesigner::Import;
using QmlDesigner::ModelNode;
using QmlDesigner::ThemeProperty;

namespace {
constexpr const char testPropNameFoo[] = "propFoo";
constexpr const char testPropNameBar[] = "propBar";

constexpr const char darkThemeName[] = "dark";
constexpr const char lightThemeName[] = "light";

MATCHER_P3(HasProperty,
           themeId,
           group,
           themeProp,
           std::string(negation ? "hasn't " : "has ") + PrintToString(themeProp))
{
    const DSThemeManager &mgr = arg;
    const std::optional<ThemeProperty> prop = mgr.property(themeId, group, themeProp.name);

    return prop && themeProp.name == prop->name && themeProp.value == prop->value
           && themeProp.isBinding == prop->isBinding;
}

class DesignSystemManagerTest : public testing::TestWithParam<QmlDesigner::GroupType>
{
protected:
    QmlDesigner::GroupType groupType = GetParam();
    DSThemeManager mgr;
};

INSTANTIATE_TEST_SUITE_P(DesignSystem,
                         DesignSystemManagerTest,
                         testing::Values(QmlDesigner::GroupType::Colors,
                                         QmlDesigner::GroupType::Flags,
                                         QmlDesigner::GroupType::Numbers,
                                         QmlDesigner::GroupType::Strings));

TEST(DesignSystemManagerTest, add_theme)
{
    // arrange
    DSThemeManager mgr;

    // act
    const auto themeId = mgr.addTheme(darkThemeName);

    // assert
    ASSERT_THAT(themeId, Optional(A<QmlDesigner::ThemeId>()));
}

TEST(DesignSystemManagerTest, add_theme_with_empty_name_fails)
{
    // arrange
    DSThemeManager mgr;

    // act
    const auto themeId = mgr.addTheme("");

    // assert
    ASSERT_THAT(themeId, Eq(std::nullopt));
}

TEST(DesignSystemManagerTest, add_theme_generates_valid_theme_id)
{
    // arrange
    DSThemeManager mgr;

    // act
    const auto themeId = mgr.addTheme(darkThemeName);

    // assert
    ASSERT_THAT(mgr.themeId(darkThemeName), Optional(themeId));
}

TEST(DesignSystemManagerTest, remove_theme)
{
    // arrange
    DSThemeManager mgr;
    const auto themeId = mgr.addTheme(darkThemeName);

    // act
    mgr.removeTheme(*themeId);

    // assert
    ASSERT_THAT(mgr, Property(&DSThemeManager::themeCount, 0));
}

TEST(DesignSystemManagerTest, remove_theme_with_properties)
{
    // arrange
    DSThemeManager mgr;
    const auto themeId = mgr.addTheme(darkThemeName);
    ThemeProperty testProp{testPropNameFoo, "#aaccbb", false};
    mgr.addProperty(GroupType::Colors, testProp);

    // act
    mgr.removeTheme(*themeId);

    // assert
    ASSERT_THAT(mgr,
                AllOf(Property(&DSThemeManager::themeCount, 0),
                      Not(HasProperty(*themeId, GroupType::Colors, testProp))));
}

TEST_P(DesignSystemManagerTest, add_property_without_theme)
{
    // arrange
    ThemeProperty testProp{testPropNameFoo, "test", false};

    // act
    mgr.addProperty(groupType, testProp);

    //assert
    ASSERT_THAT(mgr, Property(&DSThemeManager::themeCount, 0));
}

TEST_P(DesignSystemManagerTest, add_property)
{
    // arrange
    ThemeProperty testProp{testPropNameFoo, "test", false};
    const auto themeId = mgr.addTheme(darkThemeName);

    // act
    mgr.addProperty(groupType, testProp);

    // assert
    ASSERT_THAT(mgr,
                AllOf(Property(&DSThemeManager::themeCount, 1),
                      HasProperty(*themeId, groupType, testProp)));
}

TEST_P(DesignSystemManagerTest, adding_invalid_property_fails)
{
    // arrange
    ThemeProperty testProp{testPropNameFoo, {}, false};
    const auto themeId = mgr.addTheme(darkThemeName);

    // act
    const bool result = mgr.addProperty(groupType, testProp);

    // assert
    ASSERT_FALSE(result);
    ASSERT_THAT(mgr,
                AllOf(Property(&DSThemeManager::themeCount, 1),
                      Not(HasProperty(*themeId, groupType, testProp))));
}

TEST_P(DesignSystemManagerTest, adding_property_adds_property_to_all_themes)
{
    // arrange
    ThemeProperty testProp{testPropNameFoo, "test", false};
    const auto themeIdDark = mgr.addTheme(darkThemeName);
    const auto themeIdLight = mgr.addTheme(lightThemeName);

    // act
    mgr.addProperty(groupType, testProp);

    // assert
    ASSERT_THAT(mgr,
                AllOf(Property(&DSThemeManager::themeCount, 2),
                      HasProperty(*themeIdDark, groupType, testProp),
                      HasProperty(*themeIdLight, groupType, testProp)));
}

TEST_P(DesignSystemManagerTest, update_property_value)
{
    // arrange
    ThemeProperty testProp{testPropNameFoo, "test", false};
    ThemeProperty testPropUpdated{testPropNameFoo, "foo", false};
    const auto themeId = mgr.addTheme(darkThemeName);
    mgr.addProperty(groupType, testProp);

    // act
    mgr.updateProperty(*themeId, groupType, testPropUpdated);

    // assert
    ASSERT_THAT(mgr,
                AllOf(Property(&DSThemeManager::themeCount, 1),
                      HasProperty(*themeId, groupType, testPropUpdated)));
}

TEST_P(DesignSystemManagerTest, update_property_name)
{
    // arrange
    ThemeProperty testProp{testPropNameFoo, "test", false};
    ThemeProperty testPropUpdated{testPropNameBar, "test", false};
    const auto themeId = mgr.addTheme(darkThemeName);
    mgr.addProperty(groupType, testProp);

    // act
    mgr.updateProperty(*themeId, groupType, testProp, testPropUpdated.name);

    // assert
    ASSERT_THAT(mgr,
                AllOf(Property(&DSThemeManager::themeCount, 1),
                      HasProperty(*themeId, groupType, testPropUpdated)));
}

TEST_P(DesignSystemManagerTest, updating_invalid_property_fails)
{
    // arrange
    ThemeProperty testProp{testPropNameFoo, "test", false};
    ThemeProperty testPropUpdated{testPropNameFoo, {}, false};
    const auto themeId = mgr.addTheme(darkThemeName);
    mgr.addProperty(groupType, testProp);

    // act
    mgr.updateProperty(*themeId, groupType, testProp, testPropUpdated.name);

    // assert
    ASSERT_THAT(mgr,
                AllOf(Property(&DSThemeManager::themeCount, 1),
                      HasProperty(*themeId, groupType, testProp)));
}

TEST_P(DesignSystemManagerTest, remove_property)
{
    // arrange
    ThemeProperty testProp{testPropNameFoo, "test", false};
    const auto themeId = mgr.addTheme(darkThemeName);
    mgr.addProperty(groupType, testProp);

    // act
    mgr.removeProperty(groupType, testPropNameFoo);

    // assert
    ASSERT_THAT(mgr,
                AllOf(Property(&DSThemeManager::themeCount, 1),
                      Not(HasProperty(*themeId, groupType, testProp))));
}

TEST_P(DesignSystemManagerTest, remove_absent_property_fails)
{
    // arrange
    ThemeProperty testProp{testPropNameFoo, "test", false};
    const auto themeId = mgr.addTheme(darkThemeName);
    mgr.addProperty(groupType, testProp);

    // act
    mgr.removeProperty(groupType, testPropNameBar);

    // assert
    ASSERT_THAT(mgr,
                AllOf(Property(&DSThemeManager::themeCount, 1),
                      HasProperty(*themeId, groupType, testProp)));
}
} // namespace
