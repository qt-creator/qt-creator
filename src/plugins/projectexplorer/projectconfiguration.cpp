// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectconfiguration.h"

#include "kitinformation.h"
#include "target.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

const char CONFIGURATION_ID_KEY[] = "ProjectExplorer.ProjectConfiguration.Id";
const char DISPLAY_NAME_KEY[] = "ProjectExplorer.ProjectConfiguration.DisplayName";

// ProjectConfiguration

ProjectConfiguration::ProjectConfiguration(QObject *parent, Utils::Id id)
    : AspectContainer(parent)
    , m_id(id)
{
    setOwnsSubAspects(true);

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
    AspectContainer::toMap(map);
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
    AspectContainer::fromMap(map);
    return true;
}

FilePath ProjectConfiguration::mapFromBuildDeviceToGlobalPath(const FilePath &path) const
{
    IDevice::ConstPtr dev = BuildDeviceKitAspect::device(kit());
    QTC_ASSERT(dev, return path);
    return dev->filePath(path.path());
}

Id ProjectExplorer::idFromMap(const QVariantMap &map)
{
    return Id::fromSetting(map.value(QLatin1String(CONFIGURATION_ID_KEY)));
}

QString ProjectConfiguration::expandedDisplayName() const
{
    return m_target->macroExpander()->expand(m_displayName.value());
}
