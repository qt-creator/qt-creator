// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once
#include "designsystem_global.h"
#include "dsconstants.h"
#include "nodeinstanceglobal.h"

#include <modelnode.h>

#include <map>
#include <optional>

namespace QmlDesigner {
class DESIGNSYSTEM_EXPORT DSThemeGroup
{
    struct PropertyData
    {
        PropertyData() = default;
        template<typename Variant>
        PropertyData(Variant &&value, bool isBinding)
            : value{std::forward<Variant>(value)}
            , isBinding{isBinding}
        {}

        QVariant value;
        bool isBinding = false;
    };

    using ThemeValues = std::map<ThemeId, PropertyData>;
    using GroupProperties = std::map<PropertyName, ThemeValues>;

public:
    DSThemeGroup(GroupType type);
    ~DSThemeGroup();

    bool addProperty(ThemeId, const ThemeProperty &prop);
    std::optional<ThemeProperty> propertyValue(ThemeId theme, const PropertyName &name) const;

    void updateProperty(ThemeId theme, PropertyName newName, const ThemeProperty &prop);
    void removeProperty(const PropertyName &name);

    size_t count(ThemeId theme) const;
    size_t count() const;
    bool isEmpty() const;

    void removeTheme(ThemeId theme);

    void duplicateValues(ThemeId from, ThemeId to);
    void decorate(ThemeId theme, ModelNode themeNode, bool wrapInGroups = true);
    void decorateComponent(ModelNode node);

private:
    void addProperty(ModelNode n, PropertyNameView propName, const PropertyData &data) const;

private:
    const GroupType m_type;
    GroupProperties m_values;
};
}
