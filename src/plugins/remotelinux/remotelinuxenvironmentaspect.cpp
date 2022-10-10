// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "remotelinuxenvironmentaspect.h"

#include "remotelinuxenvironmentaspectwidget.h"
#include "remotelinuxtr.h"

#include <utils/algorithm.h>

namespace RemoteLinux {

const char DISPLAY_KEY[] = "DISPLAY";
const char VERSION_KEY[] = "RemoteLinux.EnvironmentAspect.Version";
const int ENVIRONMENTASPECT_VERSION = 1; // Version was introduced in 4.3 with the value 1

static bool displayAlreadySet(const Utils::EnvironmentItems &changes)
{
    return Utils::contains(changes, [](const Utils::EnvironmentItem &item) {
        return item.name == DISPLAY_KEY;
    });
}

RemoteLinuxEnvironmentAspect::RemoteLinuxEnvironmentAspect(ProjectExplorer::Target *target)
{
    addSupportedBaseEnvironment(Tr::tr("Clean Environment"), {});
    addPreferredBaseEnvironment(Tr::tr("System Environment"), [this] { return m_remoteEnvironment; });

    setConfigWidgetCreator([this, target] {
        return new RemoteLinuxEnvironmentAspectWidget(this, target);
    });
}

void RemoteLinuxEnvironmentAspect::setRemoteEnvironment(const Utils::Environment &env)
{
    if (env != m_remoteEnvironment) {
        m_remoteEnvironment = env;
        emit environmentChanged();
    }
}

QString RemoteLinuxEnvironmentAspect::userEnvironmentChangesAsString() const
{
    QString env;
    QString placeHolder = QLatin1String("%1=%2 ");
    const Utils::EnvironmentItems items = userEnvironmentChanges();
    for (const Utils::EnvironmentItem &item : items)
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

