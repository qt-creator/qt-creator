// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <gtest-creator-printing.h>

#include <designsystem/dsconstants.h>
#include <designsystem/dsthemegroup.h>

using QmlDesigner::DSThemeGroup;
using QmlDesigner::ThemeProperty;

namespace {
QByteArray testPropertyNameFoo = "propFoo";
QByteArray testPropertyNameBar = "propBar";

constexpr QmlDesigner::ThemeId themeId1 = 0;
constexpr QmlDesigner::ThemeId themeId2 = 1;

MATCHER_P3(HasPropertyCount,
           themeId,
           themePropCount,
           totalPropsCount,
           std::string(negation ? "hasn't " : "has ") + "total property count "
               + PrintToString(totalPropsCount) + " and theme property count "
               + PrintToString(themePropCount))
{
    const DSThemeGroup &group = arg;
    return group.count() == totalPropsCount && group.count(themeId) == themePropCount;
}

MATCHER_P2(HasThemeProperty,
           themeId,
           themeProp,
           std::string(negation ? "hasn't " : "has ") + PrintToString(themeProp))
{
    const DSThemeGroup &group = arg;
    const std::optional<ThemeProperty> prop = group.propertyValue(themeId, themeProp.name);

    return prop && themeProp.name == prop->name && themeProp.value == prop->value
           && themeProp.isBinding == prop->isBinding;
}

class DesignGroupTest : public testing::TestWithParam<QmlDesigner::GroupType>
{
protected:
    DesignGroupTest()
        : group(groupType)
    {
    }

    QmlDesigner::GroupType groupType = GetParam();
    QmlDesigner::DSThemeGroup group;
};

INSTANTIATE_TEST_SUITE_P(DesignSystem,
                         DesignGroupTest,
                         testing::Values(QmlDesigner::GroupType::Colors,
                                         QmlDesigner::GroupType::Flags,
                                         QmlDesigner::GroupType::Numbers,
                                         QmlDesigner::GroupType::Strings));

TEST_P(DesignGroupTest, add_property)
{
    // arrange
    ThemeProperty testProp{testPropertyNameFoo, "test", false};

    // act
    group.addProperty(themeId1, testProp);

    //assert
    ASSERT_THAT(group,
                AllOf(HasPropertyCount(themeId1, 1u, 1u), HasThemeProperty(themeId1, testProp)));
}

TEST_P(DesignGroupTest, add_multiple_properties)
{
    // arrange
    ThemeProperty testPropFoo{testPropertyNameFoo, "#aaccff", false};
    ThemeProperty testPropBar{testPropertyNameBar, "#bbddee", false};

    // act
    group.addProperty(themeId1, testPropFoo);
    group.addProperty(themeId1, testPropBar);

    //assert
    ASSERT_THAT(group,
                AllOf(HasPropertyCount(themeId1, 2u, 2u),
                      HasThemeProperty(themeId1, testPropFoo),
                      HasThemeProperty(themeId1, testPropBar)));
}

TEST_P(DesignGroupTest, add_property_with_empty_property_name)
{
    // arrange
    ThemeProperty testProp{"", "test", false};

    // act
    group.addProperty(themeId1, testProp);

    //assert
    ASSERT_THAT(group,
                AllOf(HasPropertyCount(themeId1, 0u, 0u), Not(HasThemeProperty(themeId1, testProp))));
}

TEST_P(DesignGroupTest, add_binding_property)
{
    // arrange
    ThemeProperty testProp{testPropertyNameFoo, "root.width", true};

    // act
    group.addProperty(themeId1, testProp);

    //assert
    ASSERT_THAT(group,
                AllOf(HasPropertyCount(themeId1, 1u, 1u), HasThemeProperty(themeId1, testProp)));
}

TEST_P(DesignGroupTest, add_property_with_duplicate_name)
{
    // arrange
    ThemeProperty testPropA{testPropertyNameFoo, "#aaccff", false};
    ThemeProperty testPropB{testPropertyNameFoo, "#bbddee", false};
    group.addProperty(themeId1, testPropA);

    // act
    group.addProperty(themeId1, testPropB);

    //assert
    ASSERT_THAT(group,
                AllOf(HasPropertyCount(themeId1, 1u, 1u),
                      HasThemeProperty(themeId1, testPropA),
                      Not(HasThemeProperty(themeId1, testPropB))));
}

TEST_P(DesignGroupTest, remove_property)
{
    // arrange
    ThemeProperty testProp{testPropertyNameFoo, "#aaccff", false};
    group.addProperty(themeId1, testProp);

    // act
    group.removeProperty(testPropertyNameFoo);

    //assert
    ASSERT_THAT(group,
                AllOf(HasPropertyCount(themeId1, 0u, 0u), Not(HasThemeProperty(themeId1, testProp))));
}

TEST_P(DesignGroupTest, remove_nonexistent_property_have_no_side_effect)
{
    // arrange
    ThemeProperty testPropFoo{testPropertyNameFoo, "#aaccff", false};
    group.addProperty(themeId1, testPropFoo);

    // act
    group.removeProperty(testPropertyNameBar);

    //assert
    ASSERT_THAT(group,
                AllOf(HasPropertyCount(themeId1, 1u, 1u), HasThemeProperty(themeId1, testPropFoo)));
}

TEST_P(DesignGroupTest, remove_theme_with_multiple_properties)
{
    // arrange
    ThemeProperty testPropFoo{testPropertyNameFoo, "#aaccff", false};
    group.addProperty(themeId1, testPropFoo);
    ThemeProperty testPropBar{testPropertyNameBar, "#bbddee", false};
    group.addProperty(themeId1, testPropBar);

    // act
    group.removeTheme(themeId1);

    //assert
    ASSERT_THAT(group,
                AllOf(HasPropertyCount(themeId1, 0u, 0u),
                      Not(HasThemeProperty(themeId1, testPropFoo)),
                      Not(HasThemeProperty(themeId1, testPropBar))));
}

TEST_P(DesignGroupTest, remove_theme_from_group_having_multiple_themes)
{
    // arrange
    ThemeProperty testPropFoo{testPropertyNameFoo, "#aaccff", false};
    group.addProperty(themeId1, testPropFoo);
    ThemeProperty testPropBar{testPropertyNameBar, "#bbddee", false};
    group.addProperty(themeId2, testPropBar);

    // act
    group.removeTheme(themeId1);

    //assert
    ASSERT_THAT(group,
                AllOf(HasPropertyCount(themeId1, 0u, 1u),
                      Not(HasThemeProperty(themeId1, testPropFoo)),
                      HasThemeProperty(themeId2, testPropBar)));
}

TEST_P(DesignGroupTest, remove_nonexistent_theme_have_no_side_effect)
{
    // arrange
    ThemeProperty testPropFoo{testPropertyNameFoo, "#aaccff", false};
    group.addProperty(themeId1, testPropFoo);

    // act
    group.removeTheme(themeId2);

    //assert
    ASSERT_THAT(group,
                AllOf(HasPropertyCount(themeId1, 1u, 1u), HasThemeProperty(themeId1, testPropFoo)));
}

TEST_P(DesignGroupTest, duplicate_theme)
{
    // arrange
    ThemeProperty testPropFoo{testPropertyNameFoo, "#aaccff", false};
    group.addProperty(themeId1, testPropFoo);
    ThemeProperty testPropBar{testPropertyNameBar, "#bbddee", false};
    group.addProperty(themeId1, testPropBar);

    // act
    group.duplicateValues(themeId1, themeId2);

    //assert
    ASSERT_THAT(group,
                AllOf(HasPropertyCount(themeId2, 2u, 2u),
                      HasThemeProperty(themeId2, testPropFoo),
                      HasThemeProperty(themeId2, testPropBar)));
}

TEST_P(DesignGroupTest, duplicate_nonexistent_have_no_side_effect)
{
    // arrange
    ThemeProperty testPropFoo{testPropertyNameFoo, "#aaccff", false};
    group.addProperty(themeId1, testPropFoo);

    // act
    group.duplicateValues(themeId2, themeId1);

    //assert
    ASSERT_THAT(group,
                AllOf(HasPropertyCount(themeId1, 1u, 1u), HasThemeProperty(themeId1, testPropFoo)));
}
} // namespace
