/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef REMOTELINUXDEPLOYCONFIGURATION_H
#define REMOTELINUXDEPLOYCONFIGURATION_H

#include "linuxdeviceconfiguration.h"

#include "remotelinux_export.h"

#include <coreplugin/id.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>

namespace RemoteLinux {
class AbstractEmbeddedLinuxTarget;
class DeploymentInfo;

namespace Internal {
class RemoteLinuxDeployConfigurationFactory;
class TypeSpecificDeviceConfigurationListModel;
} // namespace Internal

class REMOTELINUX_EXPORT RemoteLinuxDeployConfiguration
    : public ProjectExplorer::DeployConfiguration
{
    Q_OBJECT

public:
    RemoteLinuxDeployConfiguration(ProjectExplorer::Target *target, const Core::Id id,
        const QString &defaultDisplayName);
    RemoteLinuxDeployConfiguration(ProjectExplorer::Target *target,
        RemoteLinuxDeployConfiguration *source);

    ProjectExplorer::DeployConfigurationWidget *configurationWidget() const;

    DeploymentInfo *deploymentInfo() const;

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

    virtual QString qmakeScope() const;
    virtual QString installPrefix() const;

signals:
    void packagingChanged();

private:
    friend class Internal::RemoteLinuxDeployConfigurationFactory;
};

} // namespace RemoteLinux

#endif // REMOTELINUXDEPLOYCONFIGURATION_H
