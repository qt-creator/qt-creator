// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dsthemegroup.h"

#include <abstractproperty.h>
#include <model.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <utils/qtcassert.h>
#include <variantproperty.h>

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

QVariant getDefaultValueForGroupType(GroupType type)
{
    switch (type) {
    case QmlDesigner::GroupType::Colors: return QVariant("#000000");
    case QmlDesigner::GroupType::Flags: return QVariant(false);
    case QmlDesigner::GroupType::Numbers: return QVariant(0);
    case QmlDesigner::GroupType::Strings: return QVariant("");
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

    auto [valuesIterator, i] = m_values.try_emplace(prop.name);

    auto &tValues = valuesIterator->second;
    auto [iter, inserted] = tValues.try_emplace(theme, prop.value, prop.isBinding);
    if (!inserted)
        qCDebug(dsLog) << "Add property failed. Duplicate property name." << prop;

    return inserted;
}

std::optional<ThemeProperty> DSThemeGroup::propertyValue(ThemeId theme, const PropertyName &name) const
{
    const auto valuesIter = m_values.find(name);
    if (valuesIter == m_values.end())
        return {};

    const auto &tValues = valuesIter->second;

    if (const auto itr = tValues.find(theme); itr != tValues.end()) {
        auto &[value, isBindind] = itr->second;
        return ThemeProperty{name, value, isBindind};
    }
    return {};
}

bool DSThemeGroup::hasProperty(const PropertyName &name) const
{
    return m_values.contains(name);
}

bool DSThemeGroup::updateProperty(ThemeId theme, const ThemeProperty &prop)
{
    if (!prop.isValid()) {
        qCDebug(dsLog) << "Property update failure. Invalid property" << prop;
        return false;
    }

    const auto valuesIter = m_values.find(prop.name);
    if (valuesIter == m_values.end()) {
        qCDebug(dsLog) << "Property update failure. Can't find property" << prop;
        return false;
    }

    auto &tValues = valuesIter->second;
    const auto itr = tValues.find(theme);
    if (itr == tValues.end()) {
        qCDebug(dsLog) << "Property update failure. No property for the theme" << theme << prop;
        return false;
    }

    auto &entry = itr->second;
    entry.value = prop.value;
    entry.isBinding = prop.isBinding;
    return true;
}

void DSThemeGroup::removeProperty(const PropertyName &name)
{
    m_values.erase(name);
}

bool DSThemeGroup::renameProperty(const PropertyName &name, const PropertyName &newName)
{
    auto itr = m_values.find(name);
    if (itr == m_values.end()) {
        qCDebug(dsLog) << "Renaming non-existing property" << name;
        return false;
    }

    if (m_values.contains(newName) || newName.trimmed().isEmpty()) {
        qCDebug(dsLog) << "Renaming failed. Invalid new name" << name;
        return false;
    }

    auto node = m_values.extract(itr);
    node.key() = newName;
    m_values.insert(std::move(node));
    return true;
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
    for (auto &[propName, values] : m_values) {
        ThemeValues::iterator fromValueItr = values.find(from);
        if (fromValueItr != values.end())
            values[to] = fromValueItr->second;
    }
}

void DSThemeGroup::decorate(ThemeId theme, ModelNode themeNode, bool wrapInGroups)
{
    if (!count(theme))
        return; // No props for this theme in this group.

    ModelNode targetNode = themeNode;
    const auto typeName = groupTypeName(m_type);

    if (wrapInGroups) {
        // Create a group node
        const auto groupName = GroupId(m_type);
        auto groupNode = themeNode.model()->createModelNode("QtObject");
        NodeProperty groupProperty = themeNode.nodeProperty(groupName);

        if (!groupProperty || !typeName || !groupNode) {
            qCDebug(dsLog) << "Adding group node failed." << groupName << theme;
            return;
        }
        groupProperty.setDynamicTypeNameAndsetModelNode("QtObject", groupNode);
        targetNode = groupNode;
    }

    // Add properties
    for (auto &[propName, values] : m_values) {
        auto themeValue = values.find(theme);
        if (themeValue != values.end())
            addProperty(targetNode, propName, themeValue->second);
    }
}

void DSThemeGroup::decorateComponent(ModelNode node)
{
    const auto typeName = groupTypeName(m_type);
    // Add properties with type to the node
    for (auto &[propName, values] : m_values) {
        auto nodeProp = node.variantProperty(propName);
        nodeProp.setDynamicTypeNameAndValue(*typeName, getDefaultValueForGroupType(m_type));
    }
}

std::vector<PropertyName> DSThemeGroup::propertyNames() const
{
    std::vector<PropertyName> names;
    names.reserve(m_values.size());
    std::transform(m_values.begin(),
                   m_values.end(),
                   std::back_inserter(names),
                   [](const GroupProperties::value_type &p) { return p.first; });
    return names;
}

void DSThemeGroup::addProperty(ModelNode n, PropertyNameView propName, const PropertyData &data) const
{
    auto metaInfo = n.model()->metaInfo(n.type());
    const bool propDefined = metaInfo.property(propName).isValid();

    const auto typeName = groupTypeName(m_type);
    if (data.isBinding) {
        if (propDefined)
            n.bindingProperty(propName).setExpression(data.value.toString());
        else if (auto bindingProp = n.bindingProperty(propName))
            bindingProp.setDynamicTypeNameAndExpression(*typeName, data.value.toString());
        else
            qCDebug(dsLog) << "Assigning invalid binding" << propName << n.id();
    } else {
        if (propDefined)
            n.variantProperty(propName).setValue(data.value);
        else if (auto nodeProp = n.variantProperty(propName))
            nodeProp.setDynamicTypeNameAndValue(*typeName, data.value);
        else
            qCDebug(dsLog) << "Assigning invalid variant property" << propName << n.id();
    }
}
}
