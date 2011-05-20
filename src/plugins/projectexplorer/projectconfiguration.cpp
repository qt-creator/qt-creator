/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "projectconfiguration.h"

using namespace ProjectExplorer;

namespace {
const char * const CONFIGURATION_ID_KEY("ProjectExplorer.ProjectConfiguration.Id");
const char * const DISPLAY_NAME_KEY("ProjectExplorer.ProjectConfiguration.DisplayName");
const char * const DEFAULT_DISPLAY_NAME_KEY("ProjectExplorer.ProjectConfiguration.DefaultDisplayName");
}

ProjectConfiguration::ProjectConfiguration(QObject *parent, const QString &id) :
    QObject(parent),
    m_id(id)
{
    Q_ASSERT(!m_id.isEmpty());
}

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

QString ProjectConfiguration::id() const
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
    if (name == m_defaultDisplayName) {
        m_displayName.clear();
    } else {
        m_displayName = name;
    }
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
    map.insert(QLatin1String(CONFIGURATION_ID_KEY), m_id);
    map.insert(QLatin1String(DISPLAY_NAME_KEY), m_displayName);
    map.insert(QLatin1String(DEFAULT_DISPLAY_NAME_KEY), m_defaultDisplayName);
    return map;
}

bool ProjectConfiguration::fromMap(const QVariantMap &map)
{
    m_id = map.value(QLatin1String(CONFIGURATION_ID_KEY), QString()).toString();
    m_displayName = map.value(QLatin1String(DISPLAY_NAME_KEY), QString()).toString();
    m_defaultDisplayName = map.value(QLatin1String(DEFAULT_DISPLAY_NAME_KEY),
                                     m_defaultDisplayName.isEmpty() ?
                                         m_displayName : m_defaultDisplayName).toString();
    return !m_id.isEmpty();
}

QString ProjectExplorer::idFromMap(const QVariantMap &map)
{
    return map.value(QLatin1String(CONFIGURATION_ID_KEY), QString()).toString();
}

QString ProjectExplorer::displayNameFromMap(const QVariantMap &map)
{
    return map.value(QLatin1String(DISPLAY_NAME_KEY), QString()).toString();
}
