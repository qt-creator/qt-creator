// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dsthemegroup.h"

#include <model.h>
#include <nodeproperty.h>
#include <variantproperty.h>
#include <utils/qtcassert.h>

#include <QLoggingCategory>
#include <QVariant>

namespace QmlDesigner {
using namespace std;

namespace {
Q_LOGGING_CATEGORY(dsLog, "qtc.designer.designSystem", QtInfoMsg)

std::optional<TypeName> groupTypeName(GroupType type)
{
    switch (type) {
    case QmlDesigner::GroupType::Colors: return "color"; break;
    case QmlDesigner::GroupType::Flags: return "bool"; break;
    case QmlDesigner::GroupType::Numbers: return "real"; break;
    case QmlDesigner::GroupType::Strings: return "string"; break;
    }
    return {};
}

QDebug &operator<<(QDebug &s, const ThemeProperty &p)
{
    s << "{Name:" << p.name << ", Value:" << p.value << ", isBinding:" << p.isBinding << "}";
    return s;
}
}

DSThemeGroup::DSThemeGroup(GroupType type):
    m_type(type)
{}

DSThemeGroup::~DSThemeGroup() {}

bool DSThemeGroup::addProperty(ThemeId theme, const ThemeProperty &prop)
{
    if (!prop.isValid()) {
        qCDebug(dsLog) << "Add property failed. Invalid property." << prop;
        return false;
    }

    if (!m_values.contains(prop.name))
        m_values[prop.name] = {};

    auto &tValues = m_values.at(prop.name);
    if (tValues.contains(theme)) {
        qCDebug(dsLog) << "Add property failed. Duplicate property name." << prop;
        return false;
    }

    tValues.emplace(std::piecewise_construct,
                    std::forward_as_tuple(theme),
                    std::forward_as_tuple(prop.value, prop.isBinding));

    return true;
}

std::optional<ThemeProperty> DSThemeGroup::propertyValue(ThemeId theme, const PropertyName &name) const
{
    if (!m_values.contains(name))
        return {};

    const auto &tValues = m_values.at(name);
    const auto itr = tValues.find(theme);
    if (itr != tValues.end()) {
        auto &[value, isBindind] = itr->second;
        return ThemeProperty{name, value, isBindind};
    }
    return {};
}

void DSThemeGroup::updateProperty(ThemeId theme, PropertyName newName, const ThemeProperty &prop)
{
    if (!m_values.contains(prop.name)) {
        qCDebug(dsLog) << "Property update failure. Can't find property" << prop;
        return;
    }

    if (!ThemeProperty{newName, prop.value, prop.isBinding}.isValid()) {
        qCDebug(dsLog) << "Property update failure. Invalid property" << prop << newName;
        return;
    }

    if (newName != prop.name && m_values.contains(newName)) {
        qCDebug(dsLog) << "Property update failure. Property name update already exists" << newName
                       << prop;
        return;
    }

    auto &tValues = m_values.at(prop.name);
    const auto itr = tValues.find(theme);
    if (itr == tValues.end()) {
        qCDebug(dsLog) << "Property update failure. No property for the theme" << theme << prop;
        return;
    }

    auto &entry = tValues.at(theme);
    entry.value = prop.value;
    entry.isBinding = prop.isBinding;
    if (newName != prop.name) {
        m_values[newName] = std::move(tValues);
        m_values.erase(prop.name);
    }
}

void DSThemeGroup::removeProperty(const PropertyName &name)
{
    m_values.erase(name);
}

size_t DSThemeGroup::count(ThemeId theme) const
{
    return std::accumulate(m_values.cbegin(),
                           m_values.cend(),
                           0ull,
                           [theme](size_t c, const GroupProperties::value_type &p) {
                               return c + (p.second.contains(theme) ? 1 : 0);
                           });
}

size_t DSThemeGroup::count() const
{
    return m_values.size();
}

void DSThemeGroup::removeTheme(ThemeId theme)
{
    for (auto itr = m_values.begin(); itr != m_values.end();) {
        itr->second.erase(theme);
        itr = itr->second.size() == 0 ? m_values.erase(itr) : std::next(itr);
    }
}

void DSThemeGroup::duplicateValues(ThemeId from, ThemeId to)
{
    for (auto itr = m_values.begin(); itr != m_values.end(); ++itr) {
        auto &[propName, values] = *itr;
        auto fromValueItr = values.find(from);
        if (fromValueItr != values.end())
            values[to] = fromValueItr->second;
    }
}

void DSThemeGroup::decorate(ThemeId theme, ModelNode themeNode)
{
    if (!count(theme))
        return; // No props for this theme in this group.

    const auto groupName = GroupId(m_type);
    const auto typeName = groupTypeName(m_type);
    auto groupNode = themeNode.model()->createModelNode("QtObject");
    auto groupProperty = themeNode.nodeProperty(groupName);

    if (!groupProperty || !typeName || !groupNode) {
        qCDebug(dsLog) << "Adding group node failed." << groupName << theme;
        return;
    }

    for (auto itr = m_values.begin(); itr != m_values.end(); ++itr) {
        auto &[propName, values] = *itr;
        auto themeValue = values.find(theme);
        if (themeValue != values.end()) {
            auto &propData = themeValue->second;
            if (propData.isBinding) {
                auto bindingProp = groupNode.bindingProperty(propName);
                if (bindingProp)
                    bindingProp.setDynamicTypeNameAndExpression(*typeName, propData.value.toString());
            } else {
                auto nodeProp = groupNode.variantProperty(propName);
                if (nodeProp)
                    nodeProp.setDynamicTypeNameAndValue(*typeName, propData.value);
            }
        }
    }

    groupProperty.setDynamicTypeNameAndsetModelNode("QtObject", groupNode);
}
}
