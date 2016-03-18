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

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/environmentaspect.h>

namespace RemoteLinux {
class RemoteLinuxEnvironmentAspectWidget;

class REMOTELINUX_EXPORT RemoteLinuxEnvironmentAspect : public ProjectExplorer::EnvironmentAspect
{
    Q_OBJECT

public:
    RemoteLinuxEnvironmentAspect(ProjectExplorer::RunConfiguration *rc);
    RemoteLinuxEnvironmentAspect *create(ProjectExplorer::RunConfiguration *parent) const;
    ProjectExplorer::RunConfigWidget *createConfigurationWidget();

    QList<int> possibleBaseEnvironments() const;
    QString baseEnvironmentDisplayName(int base) const;
    Utils::Environment baseEnvironment() const;

    Utils::Environment remoteEnvironment() const;
    void setRemoteEnvironment(const Utils::Environment &env);

    QString userEnvironmentChangesAsString() const;

private:
    enum BaseEnvironmentBase {
        CleanBaseEnvironment = 0,
        RemoteBaseEnvironment = 1
    };

    Utils::Environment m_remoteEnvironment;
};

} // namespace RemoteLinux
