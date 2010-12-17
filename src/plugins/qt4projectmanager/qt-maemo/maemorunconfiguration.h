/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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

class Qt4BuildConfiguration;
class Qt4Project;
class Qt4Target;

namespace Internal {

class Qt4ProFileNode;

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

    enum DebuggingType { DebugCppOnly, DebugQmlOnly, DebugCppAndQml };

    MaemoRunConfiguration(Qt4Target *parent, const QString &proFilePath);
    virtual ~MaemoRunConfiguration();

    using ProjectExplorer::RunConfiguration::isEnabled;
    bool isEnabled(ProjectExplorer::BuildConfiguration *config) const;
    QWidget *createConfigurationWidget();
    ProjectExplorer::OutputFormatter *createOutputFormatter() const;
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
    const QString arguments() const;
    void setArguments(const QString &args);
    MaemoDeviceConfig deviceConfig() const;
    MaemoPortList freePorts() const;
    bool useRemoteGdb() const;
    void setUseRemoteGdb(bool useRemoteGdb) { m_useRemoteGdb = useRemoteGdb; }
    void updateFactoryState() { emit isEnabledChanged(true); }
    DebuggingType debuggingType() const;

    const QString gdbCmd() const;
    const QString dumperLib() const;
    QString localDirToMountForRemoteGdb() const;
    QString remoteProjectSourcesMountPoint() const;

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

    int portsUsedByDebuggers() const;

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
    void proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode *pro, bool success);
    void proFileInvalidated(Qt4ProjectManager::Internal::Qt4ProFileNode *pro);
    void updateDeviceConfigurations();
    void handleDeployConfigChanged();

private:
    void init();
    void handleParseState(bool success);

    QString m_proFilePath;
    mutable QString m_gdbPath;
    MaemoRemoteMountsModel *m_remoteMounts;
    QString m_arguments;
    bool m_useRemoteGdb;

    BaseEnvironmentBase m_baseEnvironmentBase;
    Utils::Environment m_systemEnvironment;
    QList<Utils::EnvironmentItem> m_userEnvironmentChanges;
    bool m_validParse;
};

    } // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMORUNCONFIGURATION_H
