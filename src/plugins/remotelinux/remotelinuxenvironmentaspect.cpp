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

#include "remotelinuxenvironmentaspect.h"

#include "remotelinuxenvironmentaspectwidget.h"
#include "utils/algorithm.h"

static const char DISPLAY_KEY[] = "DISPLAY";
static const char VERSION_KEY[] = "RemoteLinux.EnvironmentAspect.Version";
static const int ENVIRONMENTASPECT_VERSION = 1; // Version was introduced in 4.3 with the value 1

namespace {

bool displayAlreadySet(const QList<Utils::EnvironmentItem> &changes)
{
    return Utils::contains(changes, [](const Utils::EnvironmentItem &item) {
        return item.name == DISPLAY_KEY;
    });
}

} // anonymous namespace

namespace RemoteLinux {

// --------------------------------------------------------------------
// RemoteLinuxEnvironmentAspect:
// --------------------------------------------------------------------

RemoteLinuxEnvironmentAspect::RemoteLinuxEnvironmentAspect(ProjectExplorer::RunConfiguration *rc) :
    ProjectExplorer::EnvironmentAspect(rc)
{
    setRunConfigWidgetCreator([this] { return new RemoteLinuxEnvironmentAspectWidget(this); });
}

RemoteLinuxEnvironmentAspect *RemoteLinuxEnvironmentAspect::create(ProjectExplorer::RunConfiguration *parent) const
{
    return new RemoteLinuxEnvironmentAspect(parent);
}

QList<int> RemoteLinuxEnvironmentAspect::possibleBaseEnvironments() const
{
    return QList<int>() << static_cast<int>(RemoteBaseEnvironment)
                        << static_cast<int>(CleanBaseEnvironment);
}

QString RemoteLinuxEnvironmentAspect::baseEnvironmentDisplayName(int base) const
{
    if (base == static_cast<int>(CleanBaseEnvironment))
        return tr("Clean Environment");
    else  if (base == static_cast<int>(RemoteBaseEnvironment))
        return tr("System Environment");
    return QString();
}

Utils::Environment RemoteLinuxEnvironmentAspect::baseEnvironment() const
{
    Utils::Environment env;
    if (baseEnvironmentBase() == static_cast<int>(RemoteBaseEnvironment))
        env = m_remoteEnvironment;
    return env;
}

Utils::Environment RemoteLinuxEnvironmentAspect::remoteEnvironment() const
{
    return m_remoteEnvironment;
}

void RemoteLinuxEnvironmentAspect::setRemoteEnvironment(const Utils::Environment &env)
{
    if (env != m_remoteEnvironment) {
        m_remoteEnvironment = env;
        if (baseEnvironmentBase() == static_cast<int>(RemoteBaseEnvironment))
            emit environmentChanged();
    }
}

QString RemoteLinuxEnvironmentAspect::userEnvironmentChangesAsString() const
{
    QString env;
    QString placeHolder = QLatin1String("%1=%2 ");
    foreach (const Utils::EnvironmentItem &item, userEnvironmentChanges())
        env.append(placeHolder.arg(item.name, item.value));
    return env.mid(0, env.size() - 1);
}

void RemoteLinuxEnvironmentAspect::fromMap(const QVariantMap &map)
{
    ProjectExplorer::EnvironmentAspect::fromMap(map);

    const auto version = map.value(QLatin1String(VERSION_KEY), 0).toInt();
    if (version == 0) {
        // In Qt Creator versions prior to 4.3 RemoteLinux included DISPLAY=:0.0 in the base
        // environment, if DISPLAY was not set. In order to keep existing projects expecting
        // that working, add the DISPLAY setting to user changes in them. New projects will
        // have version 1 and will not get DISPLAY set.
        auto changes = userEnvironmentChanges();
        if (!displayAlreadySet(changes)) {
            changes.append(Utils::EnvironmentItem(QLatin1String(DISPLAY_KEY), QLatin1String(":0.0")));
            setUserEnvironmentChanges(changes);
        }
    }
}

void RemoteLinuxEnvironmentAspect::toMap(QVariantMap &map) const
{
    ProjectExplorer::EnvironmentAspect::toMap(map);
    map.insert(QLatin1String(VERSION_KEY), ENVIRONMENTASPECT_VERSION);
}

} // namespace RemoteLinux

