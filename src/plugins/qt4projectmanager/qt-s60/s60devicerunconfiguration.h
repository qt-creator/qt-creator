/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef S60DEVICERUNCONFIGURATION_H
#define S60DEVICERUNCONFIGURATION_H

#include <debugger/debuggerrunner.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/toolchaintype.h>

#include <QtCore/QFutureInterface>
#include <QtCore/QScopedPointer>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QMessageBox;
class QWidget;
QT_END_NAMESPACE

namespace SymbianUtils {
class SymbianDevice;
}

namespace trk {
class Launcher;
}


namespace tcftrk {
struct TcfTrkCommandResult;
class TcfTrkDevice;
class TcfTrkEvent;
}

namespace Qt4ProjectManager {
class QtVersion;
class Qt4BaseTarget;

namespace Internal {
class Qt4SymbianTarget;
class Qt4ProFileNode;
class S60DeviceRunConfigurationFactory;

class S60DeviceRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class S60DeviceRunConfigurationFactory;

public:
    S60DeviceRunConfiguration(Qt4ProjectManager::Qt4BaseTarget *parent, const QString &proFilePath);
    virtual ~S60DeviceRunConfiguration();

    Qt4SymbianTarget *qt4Target() const;
    const QtVersion *qtVersion() const;

    using ProjectExplorer::RunConfiguration::isEnabled;
    bool isEnabled(ProjectExplorer::BuildConfiguration *configuration) const;
    QWidget *createConfigurationWidget();

    ProjectExplorer::OutputFormatter *createOutputFormatter() const;

    QString commandLineArguments() const;
    void setCommandLineArguments(const QString &args);

    QString projectFilePath() const;

    ProjectExplorer::ToolChainType toolChainType() const;

    QString targetName() const;
    QString localExecutableFileName() const;
    quint32 executableUid() const;

    bool isDebug() const;
    QString symbianTarget() const;

    QVariantMap toMap() const;

    QString proFilePath() const;
signals:
    void targetInformationChanged();

protected:
    S60DeviceRunConfiguration(Qt4ProjectManager::Qt4BaseTarget *parent, S60DeviceRunConfiguration *source);
    QString defaultDisplayName() const;
    virtual bool fromMap(const QVariantMap &map);
private slots:
    void proFileInvalidated(Qt4ProjectManager::Internal::Qt4ProFileNode *pro);
    void proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode *pro, bool success);

private:
    ProjectExplorer::ToolChainType toolChainType(ProjectExplorer::BuildConfiguration *configuration) const;
    void ctor();
    void handleParserState(bool sucess);

    QString m_proFilePath;
    QString m_commandLineArguments;
    bool m_validParse;
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
    virtual bool promptToStop(bool *optionalPrompt = 0) const;

    static QMessageBox *createTrkWaitingMessageBox(const QString &port, QWidget *parent = 0);

protected:
    virtual void initLauncher(const QString &executable, trk::Launcher *);
    virtual void handleRunFinished();
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
    void reportLaunchFinished();

    void finishRunControl();

private slots:
    void processStopped(uint pc, uint pid, uint tid, const QString& reason);
    void printConnectFailed(const QString &errorMessage);
    void launcherFinished();
    void slotLauncherStateChanged(int);
    void slotWaitingForTrkClosed();

private slots:
    void slotError(const QString &error);
    void slotTrkLogMessage(const QString &log);
    void slotTcftrkEvent(const tcftrk::TcfTrkEvent &event);
    void slotSerialPong(const QString &message);

protected:
    QFutureInterface<void> *m_launchProgress;

private:
    void initCommunication();
    void startLaunching();
    bool setupLauncher(QString &errorMessage);
    void doStop();

    void handleModuleLoadSuspended(const tcftrk::TcfTrkEvent &event);
    void handleContextSuspended(const tcftrk::TcfTrkEvent &event);
    void handleContextAdded(const tcftrk::TcfTrkEvent &event);
    void handleContextRemoved(const tcftrk::TcfTrkEvent &event);
    void handleLogging(const tcftrk::TcfTrkEvent &event);

private:
    void handleCreateProcess(const tcftrk::TcfTrkCommandResult &result);
    void handleAddListener(const tcftrk::TcfTrkCommandResult &result);
    void handleGetThreads(const tcftrk::TcfTrkCommandResult &result);

    enum State {
        StateUninit,
        StateConnecting,
        StateConnected,
        StateProcessRunning
    };

    ProjectExplorer::ToolChainType m_toolChain;
    QString m_serialPortName;
    QString m_serialPortFriendlyName;
    QString m_address;
    unsigned short m_port;
    quint32 m_executableUid;
    QString m_targetName;
    QString m_commandLineArguments;
    QString m_executableFileName;
    QString m_qtDir;
    QString m_qtBinPath;
    bool m_runSmartInstaller;

    tcftrk::TcfTrkDevice *m_tcfTrkDevice;
    trk::Launcher *m_launcher;
    char m_installationDrive;

    QString m_runningProcessId;
    State m_state;

    bool m_useOldTrk; //FIXME: remove old TRK
};

// S60DeviceDebugRunControl starts debugging

class S60DeviceDebugRunControl : public Debugger::DebuggerRunControl
{
    Q_DISABLE_COPY(S60DeviceDebugRunControl)
    Q_OBJECT
public:
    explicit S60DeviceDebugRunControl(S60DeviceRunConfiguration *runConfiguration,
                                      const QString &mode);
    virtual void start();
    virtual bool promptToStop(bool *optionalPrompt = 0) const;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60DEVICERUNCONFIGURATION_H
