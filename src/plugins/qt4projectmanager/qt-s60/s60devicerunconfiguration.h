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

#ifndef S60DEVICERUNCONFIGURATION_H
#define S60DEVICERUNCONFIGURATION_H

#include "launcher.h"

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/toolchain.h>

#include <QtCore/QProcess>
#include <QtCore/QFutureInterface>

QT_BEGIN_NAMESPACE
class QMessageBox;
class QWidget;
QT_END_NAMESPACE

namespace Debugger {
class DebuggerStartParameters;
}

namespace Qt4ProjectManager {

namespace Internal {
class Qt4ProFileNode;
class Qt4Target;
class S60DeviceRunConfigurationFactory;

class S60DeviceRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class S60DeviceRunConfigurationFactory;

public:
    enum SigningMode {
        SignSelf,
        SignCustom
    };

    S60DeviceRunConfiguration(ProjectExplorer::Target *parent, const QString &proFilePath);
    virtual ~S60DeviceRunConfiguration();

    Qt4Target *qt4Target() const;

    bool isEnabled(ProjectExplorer::BuildConfiguration *configuration) const;
    QWidget *configurationWidget();

    QString serialPortName() const;
    void setSerialPortName(const QString &name);
    // See SerialDeviceListener
    int communicationType() const;
    void setCommunicationType(int t);

    QString targetName() const;
    QString basePackageFilePath() const;
    QString symbianPlatform() const;
    QString symbianTarget() const;
    QString packageTemplateFileName() const;
    SigningMode signingMode() const;
    void setSigningMode(SigningMode mode);
    QString customSignaturePath() const;
    void setCustomSignaturePath(const QString &path);
    QString customKeyPath() const;
    void setCustomKeyPath(const QString &path);

    QString packageFileName() const;
    QString localExecutableFileName() const;

    QStringList commandLineArguments() const;
    void setCommandLineArguments(const QStringList &args);

    ProjectExplorer::ToolChain::ToolChainType toolChainType() const;

    QVariantMap toMap() const;

signals:
    void targetInformationChanged();

private slots:
    void invalidateCachedTargetInformation();
    void proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode *pro);

protected:
    S60DeviceRunConfiguration(ProjectExplorer::Target *parent, S60DeviceRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);

private:
    ProjectExplorer::ToolChain::ToolChainType toolChainType(ProjectExplorer::BuildConfiguration *configuration) const;
    void updateTarget();
    void ctor();

    QString m_proFilePath;
    QString m_targetName;
    QString m_platform;
    QString m_target;
    QString m_baseFileName;
    QString m_packageTemplateFileName;
    bool m_cachedTargetInformationValid;
    QString m_serialPortName;
    int m_communicationType;
    SigningMode m_signingMode;
    QString m_customSignaturePath;
    QString m_customKeyPath;
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

/* S60DeviceRunControlBase: Builds the package and starts launcher
 * to deploy. Subclasses can configure the launcher to run or start a debugger.
 * Building the  package comprises for:
 * GnuPoc: run 'make sis'
 * Other:  run the makesis.exe tool, run signsis */

class S60DeviceRunControlBase : public ProjectExplorer::RunControl
{
    Q_OBJECT
public:
    explicit S60DeviceRunControlBase(ProjectExplorer::RunConfiguration *runConfiguration);
    ~S60DeviceRunControlBase();
    virtual void start();
    virtual void stop();
    virtual bool isRunning() const;

    static QMessageBox *createTrkWaitingMessageBox(const QString &port, QWidget *parent = 0);

protected:
    virtual void initLauncher(const QString &executable, trk::Launcher *) = 0;
    virtual void handleLauncherFinished() = 0;
    void processFailed(const QString &program, QProcess::ProcessError errorCode);

    virtual bool checkConfiguration(QString *errorMessage,
                                    QString *settingsCategory,
                                    QString *settingsPage) const;

protected slots:
    void printApplicationOutput(const QString &output);

private slots:
    void processStopped(uint pc, uint pid, uint tid, const QString& reason);
    void readStandardError();
    void readStandardOutput();
    void makesisProcessFailed();
    void makesisProcessFinished();
    void signsisProcessFailed();
    void signsisProcessFinished();
    void printConnectFailed(const QString &errorMessage);
    void printCopyingNotice();
    void printCreateFileFailed(const QString &filename, const QString &errorMessage);
    void printWriteFileFailed(const QString &filename, const QString &errorMessage);
    void printCloseFileFailed(const QString &filename, const QString &errorMessage);
    void printCopyProgress(int progress);
    void printInstallingNotice();
    void printInstallFailed(const QString &filename, const QString &errorMessage);
    void printInstallingFinished();
    void launcherFinished();
    void slotLauncherStateChanged(int);
    void slotWaitingForTrkClosed();
    void reportDeployFinished();

private:
    bool createPackageFileFromTemplate(QString *errorMessage);
    void startSigning();
    void startDeployment();

    ProjectExplorer::ToolChain::ToolChainType m_toolChain;
    QString m_serialPortName;
    QString m_serialPortFriendlyName;
    int     m_communicationType;
    QString m_targetName;
    QString m_baseFileName;
    QStringList m_commandLineArguments;
    QString m_symbianPlatform;
    QString m_symbianTarget;
    QString m_packageTemplateFile;
    QString m_packageFilePath;
    QString m_workingDirectory;
    QString m_toolsDirectory;
    QString m_executableFileName;
    QString m_qtDir;
    bool m_useCustomSignature;
    QString m_customSignaturePath;
    QString m_customKeyPath;
    QProcess *m_makesis;
    QProcess *m_signsis;
    QString m_makesisTool;
    QString m_packageFile;

    QFutureInterface<void> *m_deployProgress;
    trk::Launcher *m_launcher;
};

// Configure launcher to run the application
class S60DeviceRunControl : public S60DeviceRunControlBase
{
    Q_OBJECT
public:
    explicit S60DeviceRunControl(ProjectExplorer::RunConfiguration *runConfiguration);

protected:
    virtual void initLauncher(const QString &executable, trk::Launcher *);
    virtual void handleLauncherFinished();

private slots:
    void printStartingNotice();
    void printRunNotice(uint pid);
    void printRunFailNotice(const QString &errorMessage);

private:
};

class S60DeviceDebugRunControl : public S60DeviceRunControlBase
{
    Q_DISABLE_COPY(S60DeviceDebugRunControl)
    Q_OBJECT
public:
    explicit S60DeviceDebugRunControl(S60DeviceRunConfiguration *runConfiguration);
    virtual ~S60DeviceDebugRunControl();

    virtual void stop();

protected:
    virtual void initLauncher(const QString &executable, trk::Launcher *);
    virtual void handleLauncherFinished();
    virtual bool checkConfiguration(QString *errorMessage,
                                    QString *settingsCategory,
                                    QString *settingsPage) const;

private slots:
    void debuggingFinished();
private:
    QSharedPointer<Debugger::DebuggerStartParameters> m_startParams;
    QString m_localExecutableFileName;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60DEVICERUNCONFIGURATION_H
