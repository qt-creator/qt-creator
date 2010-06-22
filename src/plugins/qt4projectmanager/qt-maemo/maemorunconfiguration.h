/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef MAEMORUNCONFIGURATION_H
#define MAEMORUNCONFIGURATION_H

#include "maemoconstants.h"
#include "maemodeviceconfigurations.h"
#include "maemopackagecontents.h"

#include <projectexplorer/runconfiguration.h>

#include <QtCore/QDateTime>
#include <QtCore/QStringList>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace Qt4ProjectManager {

class Qt4Project;

namespace Internal {

class MaemoManager;
class MaemoToolChain;
class Qt4BuildConfiguration;
class Qt4ProFileNode;
class Qt4Target;

class MaemoPackageCreationStep;
class MaemoRunConfigurationFactory;

class MaemoRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class MaemoRunConfigurationFactory;

public:
    MaemoRunConfiguration(Qt4Target *parent, const QString &proFilePath);
    virtual ~MaemoRunConfiguration();

    bool isEnabled(ProjectExplorer::BuildConfiguration *config) const;
    QWidget *createConfigurationWidget();
    Qt4Target *qt4Target() const;
    Qt4BuildConfiguration *activeQt4BuildConfiguration() const;

    bool currentlyNeedsDeployment(const QString &host,
        const MaemoDeployable &deployable) const;
    void setDeployed(const QString &host, const MaemoDeployable &deployable);

    const MaemoPackageCreationStep *packageStep() const;

    QString maddeRoot() const;
    QString executable() const;
    const QString sysRoot() const;
    const QString targetRoot() const;
    const QStringList arguments() const;
    void setArguments(const QStringList &args);
    void setDeviceConfig(const MaemoDeviceConfig &deviceConfig);
    MaemoDeviceConfig deviceConfig() const;
    QString runtimeGdbServerPort() const;

    const QString sshCmd() const;
    const QString scpCmd() const;
    const QString gdbCmd() const;
    const QString dumperLib() const;

    virtual QVariantMap toMap() const;

signals:
    void deviceConfigurationsUpdated(ProjectExplorer::Target *target);
    void deviceConfigurationChanged(ProjectExplorer::Target *target);
    void targetInformationChanged() const;

protected:
    MaemoRunConfiguration(Qt4Target *parent, MaemoRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);

private slots:
    void proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode *pro);
    void updateDeviceConfigurations();

private:
    void init();
    const QString cmd(const QString &cmdName) const;
    const MaemoToolChain *toolchain() const;
    void addDeployTimesToMap(QVariantMap &map) const;
    void getDeployTimesFromMap(const QVariantMap &map);

    QString m_proFilePath;
    mutable QString m_gdbPath;

    MaemoDeviceConfig m_devConfig;
    QStringList m_arguments;

    typedef QPair<MaemoDeployable, QString> DeployablePerHost;
    QHash<DeployablePerHost, QDateTime> m_lastDeployed;
};

    } // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMORUNCONFIGURATION_H
