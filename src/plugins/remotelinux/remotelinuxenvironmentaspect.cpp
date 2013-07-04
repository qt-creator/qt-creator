/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "remotelinuxenvironmentaspect.h"

#include "remotelinuxenvironmentaspectwidget.h"

namespace RemoteLinux {

// --------------------------------------------------------------------
// RemoteLinuxEnvironmentAspect:
// --------------------------------------------------------------------

RemoteLinuxEnvironmentAspect::RemoteLinuxEnvironmentAspect(ProjectExplorer::RunConfiguration *rc) :
    ProjectExplorer::EnvironmentAspect(rc)
{ }

RemoteLinuxEnvironmentAspect *RemoteLinuxEnvironmentAspect::clone(ProjectExplorer::RunConfiguration *parent) const
{
    return new RemoteLinuxEnvironmentAspect(this, parent);
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
    if (baseEnvironmentBase() == static_cast<int>(RemoteBaseEnvironment))
        return m_remoteEnvironment;
    return Utils::Environment();
}

RemoteLinuxRunConfiguration *RemoteLinuxEnvironmentAspect::runConfiguration() const
{
    return qobject_cast<RemoteLinuxRunConfiguration *>(EnvironmentAspect::runConfiguration());
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

RemoteLinuxEnvironmentAspect::RemoteLinuxEnvironmentAspect(const RemoteLinuxEnvironmentAspect *other,
                                                           ProjectExplorer::RunConfiguration *parent) :
    ProjectExplorer::EnvironmentAspect(other, parent)
{ }

} // namespace RemoteLinux

