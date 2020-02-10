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
#include "target.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QFormLayout>
#include <QWidget>

using namespace ProjectExplorer;

const char CONFIGURATION_ID_KEY[] = "ProjectExplorer.ProjectConfiguration.Id";
const char DISPLAY_NAME_KEY[] = "ProjectExplorer.ProjectConfiguration.DisplayName";

// ProjectConfiguration

ProjectConfiguration::ProjectConfiguration(QObject *parent, Utils::Id id)
    : QObject(parent)
    , m_id(id)
{
    QTC_CHECK(parent);
    QTC_CHECK(id.isValid());
    setObjectName(id.toString());

    for (QObject *obj = this; obj; obj = obj->parent()) {
        m_target = qobject_cast<Target *>(obj);
        if (m_target)
            break;
    }
    QTC_CHECK(m_target);
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

Utils::Id ProjectConfiguration::id() const
{
    return m_id;
}

QString ProjectConfiguration::settingsIdKey()
{
    return QString(CONFIGURATION_ID_KEY);
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

QVariantMap ProjectConfiguration::toMap() const
{
    QTC_CHECK(m_id.isValid());
    QVariantMap map;
    map.insert(QLatin1String(CONFIGURATION_ID_KEY), m_id.toSetting());
    m_displayName.toMap(map, DISPLAY_NAME_KEY);
    m_aspects.toMap(map);
    return map;
}

Target *ProjectConfiguration::target() const
{
    return m_target;
}

bool ProjectConfiguration::fromMap(const QVariantMap &map)
{
    Utils::Id id = Utils::Id::fromSetting(map.value(QLatin1String(CONFIGURATION_ID_KEY)));
    // Note: This is only "startsWith", not ==, as RunConfigurations currently still
    // mangle in their build keys.
    QTC_ASSERT(id.toString().startsWith(m_id.toString()), return false);

    m_displayName.fromMap(map, DISPLAY_NAME_KEY);
    m_aspects.fromMap(map);
    return true;
}

Utils::BaseAspect *ProjectConfiguration::aspect(Utils::Id id) const
{
    return m_aspects.aspect(id);
}

void ProjectConfiguration::acquaintAspects()
{
    for (Utils::BaseAspect *aspect : m_aspects)
        aspect->acquaintSiblings(m_aspects);
}

Utils::Id ProjectExplorer::idFromMap(const QVariantMap &map)
{
    return Utils::Id::fromSetting(map.value(QLatin1String(CONFIGURATION_ID_KEY)));
}

QString ProjectConfiguration::expandedDisplayName() const
{
    return m_target->macroExpander()->expand(m_displayName.value());
}
