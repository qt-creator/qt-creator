/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "projectconfiguration.h"

using namespace ProjectExplorer;

const char CONFIGURATION_ID_KEY[] = "ProjectExplorer.ProjectConfiguration.Id";
const char DISPLAY_NAME_KEY[] = "ProjectExplorer.ProjectConfiguration.DisplayName";
const char DEFAULT_DISPLAY_NAME_KEY[] = "ProjectExplorer.ProjectConfiguration.DefaultDisplayName";

ProjectConfiguration::ProjectConfiguration(QObject *parent, Core::Id id) :
    QObject(parent),
    m_id(id)
{ setObjectName(id.toString()); }

ProjectConfiguration::ProjectConfiguration(QObject *parent, const ProjectConfiguration *source) :
    QObject(parent),
    m_id(source->m_id),
    m_defaultDisplayName(source->m_defaultDisplayName)
{
    Q_ASSERT(source);
    m_displayName = tr("Clone of %1").arg(source->displayName());
}

ProjectConfiguration::~ProjectConfiguration()
{ }

Core::Id ProjectConfiguration::id() const
{
    return m_id;
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

bool ProjectConfiguration::usesDefaultDisplayName() const
{
    return m_displayName.isEmpty();
}

QVariantMap ProjectConfiguration::toMap() const
{
    QVariantMap map;
    map.insert(QLatin1String(CONFIGURATION_ID_KEY), m_id.toSetting());
    map.insert(QLatin1String(DISPLAY_NAME_KEY), m_displayName);
    map.insert(QLatin1String(DEFAULT_DISPLAY_NAME_KEY), m_defaultDisplayName);
    return map;
}

bool ProjectConfiguration::fromMap(const QVariantMap &map)
{
    m_id = Core::Id::fromSetting(map.value(QLatin1String(CONFIGURATION_ID_KEY)));
    m_displayName = map.value(QLatin1String(DISPLAY_NAME_KEY), QString()).toString();
    m_defaultDisplayName = map.value(QLatin1String(DEFAULT_DISPLAY_NAME_KEY),
                                     m_defaultDisplayName.isEmpty() ?
                                         m_displayName : m_defaultDisplayName).toString();
    return m_id.isValid();
}

Core::Id ProjectExplorer::idFromMap(const QVariantMap &map)
{
    return Core::Id::fromSetting(map.value(QLatin1String(CONFIGURATION_ID_KEY)));
}

QString ProjectExplorer::displayNameFromMap(const QVariantMap &map)
{
    return map.value(QLatin1String(DISPLAY_NAME_KEY), QString()).toString();
}
