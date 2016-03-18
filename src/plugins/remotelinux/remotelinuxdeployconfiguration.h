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

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>

namespace RemoteLinux {
namespace Internal { class RemoteLinuxDeployConfigurationFactory; }

class REMOTELINUX_EXPORT RemoteLinuxDeployConfiguration
    : public ProjectExplorer::DeployConfiguration
{
    Q_OBJECT

public:
    RemoteLinuxDeployConfiguration(ProjectExplorer::Target *target, Core::Id id,
        const QString &defaultDisplayName);
    RemoteLinuxDeployConfiguration(ProjectExplorer::Target *target,
        RemoteLinuxDeployConfiguration *source);

    ProjectExplorer::NamedWidget *createConfigWidget() override;

    template<class T> T *earlierBuildStep(const ProjectExplorer::BuildStep *laterBuildStep) const
    {
        const QList<ProjectExplorer::BuildStep *> &buildSteps = stepList()->steps();
        for (int i = 0; i < buildSteps.count(); ++i) {
            if (buildSteps.at(i) == laterBuildStep)
                return 0;
            if (T * const step = dynamic_cast<T *>(buildSteps.at(i)))
                return step;
        }
        return 0;
    }

private:
    friend class Internal::RemoteLinuxDeployConfigurationFactory;
};

} // namespace RemoteLinux
