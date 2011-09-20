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

#ifndef QT4PROJECTMANAGER_QT4DEPLOYCONFIGURATION_H
#define QT4PROJECTMANAGER_QT4DEPLOYCONFIGURATION_H

#include <remotelinux/remotelinuxdeployconfiguration.h>

namespace RemoteLinux {
class DeployableFilesPerProFile;
class DeploymentSettingsAssistant;
}

namespace Madde {
namespace Internal {

class Qt4MaemoDeployConfigurationFactory : public ProjectExplorer::DeployConfigurationFactory
{
    Q_OBJECT

public:
    explicit Qt4MaemoDeployConfigurationFactory(QObject *parent = 0);

    virtual QStringList availableCreationIds(ProjectExplorer::Target *parent) const;
    virtual QString displayNameForId(const QString &id) const;
    virtual bool canCreate(ProjectExplorer::Target *parent, const QString &id) const;
    virtual ProjectExplorer::DeployConfiguration *create(ProjectExplorer::Target *parent, const QString &id);
    virtual bool canRestore(ProjectExplorer::Target *parent,
        const QVariantMap &map) const;
    virtual ProjectExplorer::DeployConfiguration *restore(ProjectExplorer::Target *parent,
        const QVariantMap &map);
    virtual ProjectExplorer::DeployConfiguration *clone(ProjectExplorer::Target *parent,
        ProjectExplorer::DeployConfiguration *product);
};

class Qt4MaemoDeployConfiguration : public RemoteLinux::RemoteLinuxDeployConfiguration
{
    Q_OBJECT

public:
    ~Qt4MaemoDeployConfiguration();

    ProjectExplorer::DeployConfigurationWidget *configurationWidget() const;

    QSharedPointer<RemoteLinux::DeploymentSettingsAssistant> deploymentSettingsAssistant() const;
    QString localDesktopFilePath(const RemoteLinux::DeployableFilesPerProFile *proFileInfo) const;

    static QString fremantleWithPackagingId();
    static QString fremantleWithoutPackagingId();
    static QString harmattanId();
    static QString meegoId();

private:
    friend class Internal::Qt4MaemoDeployConfigurationFactory;

    Qt4MaemoDeployConfiguration(ProjectExplorer::Target *target, const QString &id,
        const QString &displayName, const QString &supportedOsType);
    Qt4MaemoDeployConfiguration(ProjectExplorer::Target *target,
        Qt4MaemoDeployConfiguration *source);

    QSharedPointer<RemoteLinux::DeploymentSettingsAssistant> m_deploymentSettingsAssistant;
};

} // namespace Internal
} // namespace Madde

#endif // QT4PROJECTMANAGER_QT4DEPLOYCONFIGURATION_H
