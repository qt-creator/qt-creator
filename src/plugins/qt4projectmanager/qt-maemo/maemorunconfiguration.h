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
#include "maemodeployable.h"

#include <utils/environment.h>

#include <projectexplorer/runconfiguration.h>

#include <QtCore/QDateTime>
#include <QtCore/QStringList>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace Qt4ProjectManager {

class Qt4Project;

namespace Internal {

class Qt4BuildConfiguration;
class Qt4ProFileNode;
class Qt4Target;

class MaemoDeviceConfigListModel;
class MaemoDeployStep;
class MaemoManager;
class MaemoRemoteMountsModel;
class MaemoRunConfigurationFactory;
class MaemoToolChain;

class MaemoRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class MaemoRunConfigurationFactory;

public:
    enum BaseEnvironmentBase {
        CleanEnvironmentBase = 0,
        SystemEnvironmentBase = 1
    };

    MaemoRunConfiguration(Qt4Target *parent, const QString &proFilePath);
    virtual ~MaemoRunConfiguration();

    bool isEnabled(ProjectExplorer::BuildConfiguration *config) const;
    QWidget *createConfigurationWidget();
    ProjectExplorer::OutputFormatter *createConfigurationWidget() const;
    Qt4Target *qt4Target() const;
    Qt4BuildConfiguration *activeQt4BuildConfiguration() const;

    MaemoDeployStep *deployStep() const;
    MaemoRemoteMountsModel *remoteMounts() const { return m_remoteMounts; }

    const MaemoToolChain *toolchain() const;
    QString maddeRoot() const;
    QString localExecutableFilePath() const;
    QString remoteExecutableFilePath() const;
    const QString sysRoot() const;
    const QString targetRoot() const;
    const QStringList arguments() const;
    void setArguments(const QStringList &args);
    MaemoDeviceConfig deviceConfig() const;
    MaemoPortList freePorts() const;
    bool useRemoteGdb() const;
    void setUseRemoteGdb(bool useRemoteGdb) { m_useRemoteGdb = useRemoteGdb; }
    void updateFactoryState() { emit isEnabledChanged(true); }

    const QString gdbCmd() const;
    const QString dumperLib() const;
    QString localDirToMountForRemoteGdb() const;

    virtual QVariantMap toMap() const;

    QString baseEnvironmentText() const;
    BaseEnvironmentBase baseEnvironmentBase() const;
    void setBaseEnvironmentBase(BaseEnvironmentBase env);

    Utils::Environment environment() const;
    Utils::Environment baseEnvironment() const;

    QList<Utils::EnvironmentItem> userEnvironmentChanges() const;
    void setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff);

    Utils::Environment systemEnvironment() const;
    void setSystemEnvironment(const Utils::Environment &environment);

    int portsUsedByDebuggers() const { return useCppDebugger() + useQmlDebugger(); }

signals:
    void deviceConfigurationChanged(ProjectExplorer::Target *target);
    void targetInformationChanged() const;

    void baseEnvironmentChanged();
    void systemEnvironmentChanged();
    void userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &diff);

protected:
    MaemoRunConfiguration(Qt4Target *parent, MaemoRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);
    QString defaultDisplayName();

private slots:
    void proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode *pro);
    void updateDeviceConfigurations();
    void handleDeployConfigChanged();

private:
    void init();

private:
    QString m_proFilePath;
    mutable QString m_gdbPath;
    MaemoRemoteMountsModel *m_remoteMounts;
    QStringList m_arguments;
    bool m_useRemoteGdb;

    BaseEnvironmentBase m_baseEnvironmentBase;
    Utils::Environment m_systemEnvironment;
    QList<Utils::EnvironmentItem> m_userEnvironmentChanges;
};

    } // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMORUNCONFIGURATION_H
