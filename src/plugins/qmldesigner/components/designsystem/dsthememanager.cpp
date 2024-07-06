// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dsthememanager.h"

#include "dsconstants.h"
#include "dsthemegroup.h"

#include <nodeproperty.h>
#include <model.h>
#include <utils/qtcassert.h>

#include <QLoggingCategory>
#include <QVariant>

namespace {
Q_LOGGING_CATEGORY(dsLog, "qtc.designer.designSystem", QtInfoMsg)
}

namespace QmlDesigner {

DSThemeManager::DSThemeManager() {}

DSThemeManager::~DSThemeManager() {}

std::optional<ThemeId> DSThemeManager::addTheme(const ThemeName &themeName)
{
    if (themeName.trimmed().isEmpty() || themeId(themeName)) {
        qCDebug(dsLog) << "Can not add new Theme. Duplicate theme name";
        return {};
    }

    const ThemeId newThemeId = m_themes.empty() ? 1 : m_themes.rbegin()->first + 1;
    m_themes.insert({newThemeId, themeName});

    // Copy the new theme properties from an old theme(first one).
    if (m_themes.size() > 1)
        duplicateTheme(m_themes.begin()->first, newThemeId);

    return newThemeId;
}

std::optional<ThemeId> DSThemeManager::themeId(const ThemeName &themeName) const
{
    for (auto &[id, name] : m_themes) {
        if (themeName == name)
            return id;
    }
    return {};
}

size_t DSThemeManager::themeCount() const
{
    return m_themes.size();
}

void DSThemeManager::removeTheme(ThemeId id)
{
    if (!m_themes.contains(id))
        return;

    for (auto groupItr = m_groups.begin(); groupItr != m_groups.end(); ++groupItr)
        groupItr->second->removeTheme(id);

    m_themes.erase(id);
}

void DSThemeManager::duplicateTheme(ThemeId from, ThemeId to)
{
    for (auto groupItr = m_groups.begin(); groupItr != m_groups.end(); ++groupItr)
        groupItr->second->duplicateValues(from, to);
}

std::optional<ThemeProperty> DSThemeManager::property(ThemeId themeId,
                                                      GroupType gType,
                                                      const PropertyName &name) const
{
    if (m_themes.contains(themeId)) {
        auto groupItr = m_groups.find(gType);
        if (groupItr != m_groups.end())
            return groupItr->second->propertyValue(themeId, name);
    }

    qCDebug(dsLog) << "Error fetching property: {" << themeId << GroupId(gType) << name << "}";
    return {};
}

bool DSThemeManager::addProperty(GroupType gType, const ThemeProperty &p)
{
    if (!m_themes.size()) {
        qCDebug(dsLog) << "Can not add proprty. Themes empty";
        return false;
    }

    // A property is added to all themes.
    DSThemeGroup *dsGroup = propertyGroup(gType);
    QTC_ASSERT(dsGroup, return false);

    bool success = true;
    for (auto itr = m_themes.begin(); itr != m_themes.end(); ++itr)
        success &= dsGroup->addProperty(itr->first, p);

    return success;
}

void DSThemeManager::removeProperty(GroupType gType, const PropertyName &name)
{
    // A property is removed from all themes.
    DSThemeGroup *dsGroup = propertyGroup(gType);
    QTC_ASSERT(dsGroup, return);
    dsGroup->removeProperty(name);
}

void DSThemeManager::updateProperty(ThemeId id, GroupType gType, const ThemeProperty &p)
{
    updateProperty(id, gType, p, p.name);
}

void DSThemeManager::updateProperty(ThemeId id,
                                    GroupType gType,
                                    const ThemeProperty &p,
                                    const PropertyName &newName)
{
    if (!m_themes.contains(id))
        return;

    DSThemeGroup *dsGroup = propertyGroup(gType);
    QTC_ASSERT(dsGroup, return);

    dsGroup->updateProperty(id, newName, p);
}

void DSThemeManager::decorate(ModelNode rootNode) const
{
    if (!m_themes.size())
        return;

    auto p = rootNode.bindingProperty("currentTheme");
    p.setDynamicTypeNameAndExpression("QtObject", QString::fromLatin1(m_themes.begin()->second));
    addGroupAliases(rootNode);

    auto model = rootNode.model();
    for (auto itr = m_themes.begin(); itr != m_themes.end(); ++itr) {
        auto themeNode  = model->createModelNode("QtObject");
        auto themeProperty = model->rootModelNode().nodeProperty(itr->second);
        themeProperty.setDynamicTypeNameAndsetModelNode("QtObject", themeNode);

        // Add property groups
        for (auto groupItr = m_groups.begin(); groupItr != m_groups.end(); ++groupItr)
            groupItr->second->decorate(itr->first, themeNode);
    }
}

DSThemeGroup *DSThemeManager::propertyGroup(GroupType type)
{
    auto itr = m_groups.find(type);
    if (itr == m_groups.end())
        itr = m_groups.insert({type, std::make_unique<DSThemeGroup>(type)}).first;

    return itr->second.get();
}

void DSThemeManager::addGroupAliases(ModelNode rootNode) const
{
    QSet<PropertyName> groupNames;
    for (auto groupItr = m_groups.begin(); groupItr != m_groups.end(); ++groupItr) {
        DSThemeGroup *group = groupItr->second.get();
        const PropertyName groupName = GroupId(group->type());
        if (group->count())
            groupNames.insert(groupName);
    }

    for (const auto &name : groupNames) {
        auto p = rootNode.bindingProperty(name);
        auto binding = QString("currentTheme.%1").arg(QString::fromLatin1(name));
        p.setDynamicTypeNameAndExpression("QtObject", binding);
    }
}
}
