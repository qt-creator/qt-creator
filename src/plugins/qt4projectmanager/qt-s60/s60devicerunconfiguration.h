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

#ifndef S60DEVICERUNCONFIGURATION_H
#define S60DEVICERUNCONFIGURATION_H

#include "launcher.h"

#include <debugger/debuggerrunner.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/toolchain.h>

#include <QtCore/QProcess>
#include <QtCore/QFutureInterface>
#include <QtCore/QSharedPointer>
#include <QtCore/QScopedPointer>

QT_BEGIN_NAMESPACE
class QMessageBox;
class QWidget;
QT_END_NAMESPACE

namespace Debugger {
class DebuggerStartParameters;
class DebuggerRunControl;
}

namespace SymbianUtils {
class SymbianDevice;
}

namespace Qt4ProjectManager {
class QtVersion;

namespace Internal {
class Qt4ProFileNode;
class Qt4Target;
class S60DeviceRunConfigurationFactory;

class S60DeviceRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class S60DeviceRunConfigurationFactory;

public:
    S60DeviceRunConfiguration(ProjectExplorer::Target *parent, const QString &proFilePath);
    virtual ~S60DeviceRunConfiguration();

    Qt4Target *qt4Target() const;

    bool isEnabled(ProjectExplorer::BuildConfiguration *configuration) const;
    QWidget *createConfigurationWidget();

    ProjectExplorer::OutputFormatter *createOutputFormatter() const;

    QStringList commandLineArguments() const;
    void setCommandLineArguments(const QStringList &args);

    QString projectFilePath() const;

    ProjectExplorer::ToolChain::ToolChainType toolChainType() const;

    QVariantMap toMap() const;

    void proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode *pro);

signals:
    void targetInformationChanged();

protected:
    S60DeviceRunConfiguration(ProjectExplorer::Target *parent, S60DeviceRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);

private:
    ProjectExplorer::ToolChain::ToolChainType toolChainType(ProjectExplorer::BuildConfiguration *configuration) const;
    void ctor();

    QString m_proFilePath;
    QStringList m_commandLineArguments;
};

class S60DeviceRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit S60DeviceRunConfigurationFactory(QObject *parent = 0);
    ~S60DeviceRunConfigurationFactory();

    bool canCreate(ProjectExplorer::Target *parent, const QString &id) const;
    ProjectExplorer::RunConfiguration *create(ProjectExplorer::Target *parent, const QString &id);
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::RunConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source);

    QStringList availableCreationIds(ProjectExplorer::Target *parent) const;
    // used to translate the ids to names to display to the user
    QString displayNameForId(const QString &id) const;
};

// S60DeviceRunControl configures launcher to run the application

class S60DeviceRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT
public:
    explicit S60DeviceRunControl(ProjectExplorer::RunConfiguration *runConfiguration, QString mode);
    ~S60DeviceRunControl();
    virtual void start();
    virtual StopResult stop();
    virtual bool isRunning() const;

    static QMessageBox *createTrkWaitingMessageBox(const QString &port, QWidget *parent = 0);

protected:
    virtual void initLauncher(const QString &executable, trk::Launcher *);
    virtual void handleLauncherFinished();
    virtual bool checkConfiguration(QString *errorMessage,
                                    QString *settingsCategory,
                                    QString *settingsPage) const;

protected slots:
    void printApplicationOutput(const QString &output, bool onStdErr);
    void printApplicationOutput(const QString &output);
    void printStartingNotice();
    void applicationRunNotice(uint pid);
    void applicationRunFailedNotice(const QString &errorMessage);
    void deviceRemoved(const SymbianUtils::SymbianDevice &);
    void reportDeployFinished();

private slots:
    void processStopped(uint pc, uint pid, uint tid, const QString& reason);
    void printConnectFailed(const QString &errorMessage);
    void launcherFinished();
    void slotLauncherStateChanged(int);
    void slotWaitingForTrkClosed();

protected:
    QFutureInterface<void> *m_launchProgress;

private:
    void startLaunching();
    bool setupLauncher(QString &errorMessage);

    ProjectExplorer::ToolChain::ToolChainType m_toolChain;
    QString m_serialPortName;
    QString m_serialPortFriendlyName;
    QString m_targetName;
    QStringList m_commandLineArguments;
    QString m_executableFileName;
    QString m_qtDir;
    QString m_qtBinPath;
    bool m_handleDeviceRemoval;
    trk::Launcher *m_launcher;
    char m_installationDrive;
};

// S60DeviceDebugRunControl starts debugging

class S60DeviceDebugRunControl : public Debugger::DebuggerRunControl
{
    Q_DISABLE_COPY(S60DeviceDebugRunControl)
    Q_OBJECT
public:
    explicit S60DeviceDebugRunControl(S60DeviceRunConfiguration *runConfiguration,
                                      const QString &mode);
    virtual ~S60DeviceDebugRunControl();
    virtual void start();
    virtual StopResult stop();

private slots:
    void slotFinished();

private:
    static Debugger::DebuggerStartParameters s60DebuggerStartParams(const S60DeviceRunConfiguration *rc);

    QScopedPointer<QFutureInterface<void> > m_debugProgress;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60DEVICERUNCONFIGURATION_H
