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
using namespace ProjectExplorer;
typedef QSharedPointer<RunConfiguration> RunConfig;

#define USE_SSL_PASSWORD 0

class ErrorDumper : public QObject
{
    Q_OBJECT
public:
    ErrorDumper(QObject *parent = 0)
        : QObject(parent) {}

public slots:
    void printToStream(QProcess::ProcessError error);
};


class MaemoRunConfiguration : public RunConfiguration
{
    Q_OBJECT
public:
    MaemoRunConfiguration(Project *project, const QString &proFilePath);
    ~MaemoRunConfiguration();

    QString type() const;
    bool isEnabled() const;
    QWidget *configurationWidget();
    Qt4Project *project() const;

    void save(ProjectExplorer::PersistentSettingsWriter &writer) const;
    void restore(const ProjectExplorer::PersistentSettingsReader &reader);

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

    QString simulator() const;
    QString simulatorArgs() const;
    QString simulatorPath() const;
    QString visibleSimulatorParameter() const;

    bool remoteHostIsSimulator() const { return m_remoteHostIsSimulator; }
    const QString remoteHostName() const;
    const QString remoteUserName() const;
    int remotePort() const;

    const QString remoteDir() const;
    const QString sshCmd() const;
    const QString scpCmd() const;
    const QString gdbCmd() const;
    const QString dumperLib() const;

    void setRemoteHostIsSimulator(bool isSimulator);
    void setRemoteHostName(const QString &hostName);
    void setRemoteUserName(const QString &userName);
    void setRemotePort(int port);

    bool isQemuRunning() const;

#if USE_SSL_PASSWORD
    // Only valid if remoteHostRequiresPassword() == true.
    void setRemotePassword(const QString &password);
    const QString remoteUserPassword() const { return m_remoteUserPassword; }

        void setRemoteHostRequiresPassword(bool requiresPassword);
    bool remoteHostRequiresPassword() const { return m_remoteHostRequiresPassword; }
#endif

signals:
    void targetInformationChanged();
    void cachedSimulatorInformationChanged();
    void qemuProcessStatus(bool running);

private slots:
    void invalidateCachedTargetInformation();

    void setUserSimulatorPath(const QString &path);
    void invalidateCachedSimulatorInformation();
    void resetCachedSimulatorInformation();

    void startStopQemu();
    void qemuProcessFinished();

    void enabledStateChanged();

private:
    void updateTarget();
    void updateSimulatorInformation();
    const QString cmd(const QString &cmdName) const;
    const MaemoToolChain *toolchain() const;
    bool fileNeedsDeployment(const QString &path, const QDateTime &lastDeployed) const;

private:
    // Keys for saving/loading attributes.
    QString m_executable;
    QString m_proFilePath;
    bool m_cachedTargetInformationValid;

    QString m_simulator;
    QString m_simulatorArgs;
    QString m_simulatorPath;
    QString m_visibleSimulatorParameter;
    bool m_cachedSimulatorInformationValid;

    bool m_isUserSetSimulator;
    QString m_userSimulatorPath;

    QString m_gdbPath;

    // Information about the remote host.
    bool m_remoteHostIsSimulator;

    QStringList m_argumentsSim;
    QString m_remoteHostNameSim;
    QString m_remoteUserNameSim;
    int m_remotePortSim;

    QStringList m_argumentsDevice;
    QString m_remoteHostNameDevice;
    QString m_remoteUserNameDevice;
    int m_remotePortDevice;

    QDateTime m_lastDeployed;
    QDateTime m_debuggingHelpersLastDeployed;

    QProcess *qemu;
    ErrorDumper dumper;

#if USE_SSL_PASSWORD
    QString m_remoteUserPassword;
    bool m_remoteHostRequiresPassword;
#endif
};


class MaemoRunConfigurationFactory : public IRunConfigurationFactory
{
    Q_OBJECT
public:
    MaemoRunConfigurationFactory(QObject *parent);
    ~MaemoRunConfigurationFactory();

    bool canRestore(const QString &type) const;
    QStringList availableCreationTypes(Project *project) const;
    QString displayNameForType(const QString &type) const;
    RunConfig create(Project *project, const QString &type);

private slots:
    void addedRunConfiguration(ProjectExplorer::Project* project);
    void removedRunConfiguration(ProjectExplorer::Project* project);

    void projectAdded(ProjectExplorer::Project* project);
    void projectRemoved(ProjectExplorer::Project* project);
    void currentProjectChanged(ProjectExplorer::Project* project);
};


class MaemoRunControlFactory : public IRunControlFactory
{
    Q_OBJECT
public:
    MaemoRunControlFactory(QObject *parent = 0);
    bool canRun(const RunConfig &runConfiguration, const QString &mode) const;
    RunControl* create(const RunConfig &runConfiguration, const QString &mode);
    QString displayName() const;
    QWidget *configurationWidget(const RunConfig &runConfiguration);
};

} // namespace Internal
} // namespace Qt4ProjectManager


#endif // MAEMORUNCONFIGURATION_H
