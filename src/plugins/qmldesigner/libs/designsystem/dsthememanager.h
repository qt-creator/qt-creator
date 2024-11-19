// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "designsystem_global.h"

#include "dsconstants.h"
#include "dsthemegroup.h"

#include <externaldependenciesinterface.h>
#include <modelnode.h>

#include <QCoreApplication>

namespace QmlDesigner {

using ThemeName = PropertyName;

class DSTheme;

class DESIGNSYSTEM_EXPORT DSThemeManager
{
    Q_DECLARE_TR_FUNCTIONS(DSThemeManager);

public:
    DSThemeManager();
    ~DSThemeManager();

    DSThemeManager(const DSThemeManager&) = delete;
    DSThemeManager& operator=(const DSThemeManager&) = delete;

    DSThemeManager(DSThemeManager&&) = default;
    DSThemeManager &operator=(DSThemeManager &&) = default;

    std::optional<ThemeId> addTheme(const ThemeName &themeNameHint);
    std::optional<ThemeId> themeId(const ThemeName &themeName) const;
    ThemeName themeName(ThemeId id) const;
    bool renameTheme(ThemeId id, const ThemeName &newName);
    const std::vector<ThemeId> allThemeIds() const;

    void forAllGroups(std::function<void(GroupType, DSThemeGroup *)> callback) const;

    void removeTheme(ThemeId id);
    size_t themeCount() const;
    size_t propertyCount() const;

    void duplicateTheme(ThemeId from, ThemeId to);

    bool addProperty(GroupType gType, const ThemeProperty &p);
    std::optional<ThemeProperty> property(ThemeId themeId,
                                               GroupType gType,
                                               const PropertyName &name) const;
    void removeProperty(GroupType gType, const PropertyName &p);

    bool updateProperty(ThemeId id, GroupType gType, const ThemeProperty &prop);
    bool renameProperty(GroupType gType, const PropertyName &name, const PropertyName &newName);

    void decorate(ModelNode rootNode, const QByteArray &nodeType = "QtObject", bool isMCU = false) const;
    void decorateThemeInterface(ModelNode rootNode) const;

    std::optional<QString> load(ModelNode rootModelNode);

private:
    DSThemeGroup *propertyGroup(GroupType type);
    void addGroupAliases(ModelNode rootNode) const;

    bool findPropertyType(const AbstractProperty &p, ThemeProperty *themeProp, GroupType *gt) const;

    ThemeName uniqueThemeName(const ThemeName &hint) const;
    PropertyName uniquePropertyName(const PropertyName &hint) const;

private:
    std::map<ThemeId, ThemeName> m_themes;
    std::map<GroupType, std::unique_ptr<DSThemeGroup>> m_groups;
};
}
