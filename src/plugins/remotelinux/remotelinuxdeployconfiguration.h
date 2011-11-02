/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#ifndef REMOTELINUXDEPLOYCONFIGURATION_H
#define REMOTELINUXDEPLOYCONFIGURATION_H

#include "linuxdeviceconfiguration.h"
#include "remotelinux_export.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>

#include <QtCore/QSharedPointer>

namespace RemoteLinux {
class DeploymentInfo;

namespace Internal {
class RemoteLinuxDeployConfigurationPrivate;
class TypeSpecificDeviceConfigurationListModel;
} // namespace Internal

class REMOTELINUX_EXPORT RemoteLinuxDeployConfiguration
    : public ProjectExplorer::DeployConfiguration
{
    Q_OBJECT
    Q_DISABLE_COPY(RemoteLinuxDeployConfiguration)
public:
    RemoteLinuxDeployConfiguration(ProjectExplorer::Target *target, const QString &id,
        const QString &defaultDisplayName, const QString &supportedOsType);
    RemoteLinuxDeployConfiguration(ProjectExplorer::Target *target,
        RemoteLinuxDeployConfiguration *source);

    ~RemoteLinuxDeployConfiguration();

    bool fromMap(const QVariantMap &map);
    ProjectExplorer::DeployConfigurationWidget *configurationWidget() const;

    void setDeviceConfiguration(int index);
    QSharedPointer<DeploymentInfo> deploymentInfo() const;
    QSharedPointer<Internal::TypeSpecificDeviceConfigurationListModel> deviceConfigModel() const;
    QSharedPointer<const LinuxDeviceConfiguration> deviceConfiguration() const;
    QString supportedOsType() const;

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

signals:
    void deviceConfigurationListChanged();
    void currentDeviceConfigurationChanged();

private:
    QVariantMap toMap() const;

    void initialize();
    void setDeviceConfig(LinuxDeviceConfiguration::Id internalId);
    Q_SLOT void handleDeviceConfigurationListUpdated();

    Internal::RemoteLinuxDeployConfigurationPrivate * const d;
};

} // namespace RemoteLinux

#endif // REMOTELINUXDEPLOYCONFIGURATION_H
