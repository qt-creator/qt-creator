/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "maemodeviceconfigurations.h"

#include <QtCore/QDateTime>
#include <QtGui/QWidget>

#include <debugger/debuggermanager.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/applicationlauncher.h>

namespace Qt4ProjectManager {
class Qt4Project;

namespace Internal {

class MaemoManager;
class MaemoToolChain;
class Qt4ProFileNode;

class ErrorDumper : public QObject
{
    Q_OBJECT
public:
    ErrorDumper(QObject *parent = 0)
        : QObject(parent) {}

public slots:
    void printToStream(QProcess::ProcessError error);
};

class MaemoRunConfigurationFactory;

class MaemoRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class MaemoRunConfigurationFactory;

public:
    MaemoRunConfiguration(Qt4Project *project,
                          const QString &proFilePath);
    virtual ~MaemoRunConfiguration();

    bool isEnabled(ProjectExplorer::BuildConfiguration *config) const;
    using RunConfiguration::isEnabled;
    QWidget *configurationWidget();
    Qt4Project *qt4Project() const;

    bool currentlyNeedsDeployment() const;
    void wasDeployed();

    bool hasDebuggingHelpers() const;
    bool debuggingHelpersNeedDeployment() const;
    void debuggingHelpersDeployed();

    QString maddeRoot() const;
    QString executable() const;
    const QString sysRoot() const;
    const QStringList arguments() const;
    void setArguments(const QStringList &args);
    void setDeviceConfig(const MaemoDeviceConfig &deviceConfig);
    MaemoDeviceConfig deviceConfig() const;

    QString simulator() const;
    QString simulatorArgs() const;
    QString simulatorPath() const;
    QString simulatorSshPort() const { return m_simulatorSshPort; }
    QString simulatorGdbServerPort() const { return m_simulatorGdbServerPort; }
    QString visibleSimulatorParameter() const;

    const QString sshCmd() const;
    const QString scpCmd() const;
    const QString gdbCmd() const;
    const QString dumperLib() const;

    bool isQemuRunning() const;

    virtual QVariantMap toMap() const;

signals:
    void deviceConfigurationsUpdated();
    void targetInformationChanged();
    void cachedSimulatorInformationChanged();
    void qemuProcessStatus(bool running);

protected:
    MaemoRunConfiguration(Qt4Project *project,
                          MaemoRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);

private slots:
    void proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode *pro);
    void updateDeviceConfigurations();
    void invalidateCachedTargetInformation();

    void startStopQemu();
    void qemuProcessFinished();

    void enabledStateChanged();

private:
    void init();
    void updateTarget();
    void updateSimulatorInformation();
    const QString cmd(const QString &cmdName) const;
    const MaemoToolChain *toolchain() const;
    bool fileNeedsDeployment(const QString &path, const QDateTime &lastDeployed) const;

    QString m_executable;
    QString m_proFilePath;
    bool m_cachedTargetInformationValid;

    QString m_simulator;
    QString m_simulatorArgs;
    QString m_simulatorPath;
    QString m_visibleSimulatorParameter;
    QString m_simulatorLibPath;
    QString m_simulatorSshPort;
    QString m_simulatorGdbServerPort;
    bool m_cachedSimulatorInformationValid;

    QString m_gdbPath;

    MaemoDeviceConfig m_devConfig;
    QStringList m_arguments;

    QDateTime m_lastDeployed;
    QDateTime m_debuggingHelpersLastDeployed;

    QProcess *qemu;
    ErrorDumper dumper;
};

class MaemoRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit MaemoRunConfigurationFactory(QObject *parent = 0);
    ~MaemoRunConfigurationFactory();

    QStringList availableCreationIds(ProjectExplorer::Project *project) const;
    QString displayNameForId(const QString &id) const;

    bool canRestore(ProjectExplorer::Project *project, const QVariantMap &map) const;
    bool canClone(ProjectExplorer::Project *project, ProjectExplorer::RunConfiguration *source) const;
    bool canCreate(ProjectExplorer::Project *project, const QString &id) const;
    ProjectExplorer::RunConfiguration *restore(ProjectExplorer::Project *project, const QVariantMap &map);
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Project *project, ProjectExplorer::RunConfiguration *source);
    ProjectExplorer::RunConfiguration *create(ProjectExplorer::Project *project, const QString &id);

private slots:
    void addedRunConfiguration(ProjectExplorer::RunConfiguration *rc);
    void removedRunConfiguration(ProjectExplorer::RunConfiguration *rc);

    void projectAdded(ProjectExplorer::Project *project);
    void projectRemoved(ProjectExplorer::Project *project);
    void currentProjectChanged(ProjectExplorer::Project *project);

private:
    void setupRunConfiguration(MaemoRunConfiguration *rc);
};


class MaemoRunControlFactory : public ProjectExplorer::IRunControlFactory
{
    Q_OBJECT
public:
    MaemoRunControlFactory(QObject *parent = 0);
    bool canRun(ProjectExplorer::RunConfiguration *runConfiguration,
                const QString &mode) const;
    ProjectExplorer::RunControl* create(ProjectExplorer::RunConfiguration *runConfiguration,
                                        const QString &mode);
    QString displayName() const;
    QWidget *configurationWidget(ProjectExplorer::RunConfiguration *runConfiguration);
};

} // namespace Internal
} // namespace Qt4ProjectManager


#endif // MAEMORUNCONFIGURATION_H
