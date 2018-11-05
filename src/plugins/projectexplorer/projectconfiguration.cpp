/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "projectconfiguration.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;

const char CONFIGURATION_ID_KEY[] = "ProjectExplorer.ProjectConfiguration.Id";
const char DISPLAY_NAME_KEY[] = "ProjectExplorer.ProjectConfiguration.DisplayName";
const char DEFAULT_DISPLAY_NAME_KEY[] = "ProjectExplorer.ProjectConfiguration.DefaultDisplayName";

// ProjectConfigurationAspect

ProjectConfigurationAspect::ProjectConfigurationAspect() = default;

ProjectConfigurationAspect::~ProjectConfigurationAspect() = default;

void ProjectConfigurationAspect::setConfigWidgetCreator
    (const ConfigWidgetCreator &configWidgetCreator)
{
    m_configWidgetCreator = configWidgetCreator;
}

QWidget *ProjectConfigurationAspect::createConfigWidget() const
{
    return m_configWidgetCreator ? m_configWidgetCreator() : nullptr;
}


// ProjectConfigurationAspects

ProjectConfigurationAspects::ProjectConfigurationAspects() = default;

ProjectConfigurationAspects::~ProjectConfigurationAspects()
{
    qDeleteAll(base());
}

ProjectConfigurationAspect *ProjectConfigurationAspects::aspect(Core::Id id) const
{
    return Utils::findOrDefault(base(), Utils::equal(&ProjectConfigurationAspect::id, id));
}

void ProjectConfigurationAspects::fromMap(const QVariantMap &map) const
{
    for (ProjectConfigurationAspect *aspect : *this)
        aspect->fromMap(map);
}

void ProjectConfigurationAspects::toMap(QVariantMap &map) const
{
    for (ProjectConfigurationAspect *aspect : *this)
        aspect->toMap(map);
}


// ProjectConfiguration

ProjectConfiguration::ProjectConfiguration(QObject *parent, Core::Id id)
    : QObject(parent), m_id(id)
{
    QTC_CHECK(id.isValid());
    setObjectName(id.toString());
}

ProjectConfiguration::~ProjectConfiguration() = default;

Core::Id ProjectConfiguration::id() const
{
    return m_id;
}

QString ProjectConfiguration::settingsIdKey()
{
    return QString(CONFIGURATION_ID_KEY);
}

QString ProjectConfiguration::displayName() const
{
    if (!m_displayName.isEmpty())
        return m_displayName;
    return m_defaultDisplayName;
}

void ProjectConfiguration::setDisplayName(const QString &name)
{
    if (displayName() == name)
        return;
    if (name == m_defaultDisplayName)
        m_displayName.clear();
    else
        m_displayName = name;
    emit displayNameChanged();
}

void ProjectConfiguration::setDefaultDisplayName(const QString &name)
{
    if (m_defaultDisplayName == name)
        return;
    const QString originalName = displayName();
    m_defaultDisplayName = name;
    if (originalName != displayName())
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

bool ProjectConfiguration::usesDefaultDisplayName() const
{
    return m_displayName.isEmpty();
}

QVariantMap ProjectConfiguration::toMap() const
{
    QTC_CHECK(m_id.isValid());
    QVariantMap map;
    map.insert(QLatin1String(CONFIGURATION_ID_KEY), m_id.toSetting());
    map.insert(QLatin1String(DISPLAY_NAME_KEY), m_displayName);
    map.insert(QLatin1String(DEFAULT_DISPLAY_NAME_KEY), m_defaultDisplayName);

    m_aspects.toMap(map);

    return map;
}

bool ProjectConfiguration::fromMap(const QVariantMap &map)
{
    Core::Id id = Core::Id::fromSetting(map.value(QLatin1String(CONFIGURATION_ID_KEY)));
    // Note: This is only "startsWith", not ==, as RunConfigurations currently still
    // mangle in their build keys.
    QTC_ASSERT(id.toString().startsWith(m_id.toString()), return false);

    m_displayName = map.value(QLatin1String(DISPLAY_NAME_KEY), QString()).toString();
    m_defaultDisplayName = map.value(QLatin1String(DEFAULT_DISPLAY_NAME_KEY),
                                     m_defaultDisplayName.isEmpty() ?
                                         m_displayName : m_defaultDisplayName).toString();

    m_aspects.fromMap(map);

    return true;
}

ProjectConfigurationAspect *ProjectConfiguration::aspect(Core::Id id) const
{
    return m_aspects.aspect(id);
}

Core::Id ProjectExplorer::idFromMap(const QVariantMap &map)
{
    return Core::Id::fromSetting(map.value(QLatin1String(CONFIGURATION_ID_KEY)));
}

// StatefulProjectConfiguration

bool StatefulProjectConfiguration::isEnabled() const
{
    return m_isEnabled;
}

StatefulProjectConfiguration::StatefulProjectConfiguration(QObject *parent, Core::Id id) :
    ProjectConfiguration(parent, id)
{ }

void StatefulProjectConfiguration::setEnabled(bool enabled)
{
    if (enabled == m_isEnabled)
        return;
    m_isEnabled = enabled;
    emit enabledChanged();
}
