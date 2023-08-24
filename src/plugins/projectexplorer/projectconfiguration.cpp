// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectconfiguration.h"

#include "target.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

const char CONFIGURATION_ID_KEY[] = "ProjectExplorer.ProjectConfiguration.Id";
const char DISPLAY_NAME_KEY[] = "ProjectExplorer.ProjectConfiguration.DisplayName";

// ProjectConfiguration

ProjectConfiguration::ProjectConfiguration(Target *target, Id id)
    : m_target(target)
    , m_id(id)
{
    QTC_CHECK(target);
    QTC_CHECK(id.isValid());
    setObjectName(id.toString());
}

ProjectConfiguration::~ProjectConfiguration() = default;

Project *ProjectConfiguration::project() const
{
    return m_target->project();
}

Kit *ProjectConfiguration::kit() const
{
    return m_target->kit();
}

Id ProjectConfiguration::id() const
{
    return m_id;
}

Key ProjectConfiguration::settingsIdKey()
{
    return CONFIGURATION_ID_KEY;
}

void ProjectConfiguration::setDisplayName(const QString &name)
{
    if (m_displayName.setValue(name))
        emit displayNameChanged();
}

void ProjectConfiguration::setDefaultDisplayName(const QString &name)
{
    if (m_displayName.setDefaultValue(name))
        emit displayNameChanged();
}

void ProjectConfiguration::setToolTip(const QString &text)
{
    if (text == m_toolTip)
        return;
    m_toolTip = text;
    emit toolTipChanged();
}

QString ProjectConfiguration::toolTip() const
{
    return m_toolTip;
}

void ProjectConfiguration::toMap(Store &map) const
{
    QTC_CHECK(m_id.isValid());
    map.insert(CONFIGURATION_ID_KEY, m_id.toSetting());
    m_displayName.toMap(map, DISPLAY_NAME_KEY);
    AspectContainer::toMap(map);
}

Target *ProjectConfiguration::target() const
{
    return m_target;
}

void ProjectConfiguration::fromMap(const Store &map)
{
    Id id = Id::fromSetting(map.value(CONFIGURATION_ID_KEY));
    // Note: This is only "startsWith", not ==, as RunConfigurations currently still
    // mangle in their build keys.
    QTC_ASSERT(id.name().startsWith(m_id.name()), reportError(); return);

    m_displayName.fromMap(map, DISPLAY_NAME_KEY);
    AspectContainer::fromMap(map);
}

Id ProjectExplorer::idFromMap(const Store &map)
{
    return Id::fromSetting(map.value(CONFIGURATION_ID_KEY));
}

QString ProjectConfiguration::expandedDisplayName() const
{
    return m_target->macroExpander()->expand(m_displayName.value());
}
