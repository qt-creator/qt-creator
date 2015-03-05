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

#include "remotelinuxenvironmentaspect.h"

#include "abstractremotelinuxrunconfiguration.h"
#include "remotelinuxenvironmentaspectwidget.h"

namespace RemoteLinux {

// --------------------------------------------------------------------
// RemoteLinuxEnvironmentAspect:
// --------------------------------------------------------------------

RemoteLinuxEnvironmentAspect::RemoteLinuxEnvironmentAspect(ProjectExplorer::RunConfiguration *rc) :
    ProjectExplorer::EnvironmentAspect(rc)
{ }

RemoteLinuxEnvironmentAspect *RemoteLinuxEnvironmentAspect::create(ProjectExplorer::RunConfiguration *parent) const
{
    return new RemoteLinuxEnvironmentAspect(parent);
}

ProjectExplorer::RunConfigWidget *RemoteLinuxEnvironmentAspect::createConfigurationWidget()
{
    return new RemoteLinuxEnvironmentAspectWidget(this);
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
    const QString displayKey = QLatin1String("DISPLAY");
    if (!env.hasKey(displayKey))
        env.appendOrSet(displayKey, QLatin1String(":0.0"));
    return env;
}

AbstractRemoteLinuxRunConfiguration *RemoteLinuxEnvironmentAspect::runConfiguration() const
{
    return qobject_cast<AbstractRemoteLinuxRunConfiguration *>(EnvironmentAspect::runConfiguration());
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

} // namespace RemoteLinux

