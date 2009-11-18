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

#include "maemorunconfiguration.h"

#include "maemomanager.h"
#include "maemotoolchain.h"
#include "profilereader.h"
#include "qt4project.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <debugger/debuggermanager.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <projectexplorer/persistentsettings.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>

#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QSharedPointer>

#include <QtGui/QCheckBox>
#include <QtGui/QFormLayout>
#include <QtGui/QFrame>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QRadioButton>
#include <QtGui/QToolButton>

using namespace ProjectExplorer;

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

class MaemoRunConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    MaemoRunConfigurationWidget(
        MaemoRunConfiguration *runConfiguration, QWidget *parent = 0);

private slots:
    void configNameEdited(const QString &text);
    void argumentsEdited(const QString &args);
    void hostNameEdited(const QString &name);
    void userNameEdited(const QString &name);
    void portEdited(const QString &port);
    void hostTypeChanged();

#if USE_SSL_PASSWORD
    void passwordUseChanged();
    void passwordEdited(const QString &password);
#endif

    void updateTargetInformation();
    
    void updateSimulatorPath();
    void updateVisibleSimulatorParameter();

private:
    QLineEdit *m_configNameLineEdit;
    QLineEdit *m_argsLineEdit;
    QLineEdit *m_hostNameLineEdit;
    QLineEdit *m_userLineEdit;
    QLineEdit *m_passwordLineEdit;
    QLineEdit *m_portLineEdit;
    QLabel *m_executableLabel;
    QLabel *m_debuggerLabel;
    QLabel *m_simParamsValueLabel;
    QLabel *m_chooseSimPathLabel;
    QLabel *m_simParamsNameLabel;
    QCheckBox *m_passwordCheckBox;
    QRadioButton *m_hwButton;
    QRadioButton *m_simButton;
    QToolButton *m_resetButton;
    Utils::PathChooser *m_simPathChooser;
    MaemoRunConfiguration *m_runConfiguration;
};

class AbstractMaemoRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT

public:
    AbstractMaemoRunControl(RunConfiguration *runConfig);
    virtual ~AbstractMaemoRunControl() {}

protected:
    void startDeployment(bool forDebugging);
    void stopDeployment();
    bool isDeploying() const;
    const QString executableOnHost() const;
    const QString executableOnTarget() const;
    const QString executableFileName() const;
    const QString port() const;
    const QString targetCmdLinePrefix() const;
    virtual void deploymentFinished(bool success)=0;
    virtual bool setProcessEnvironment(QProcess &process);

private slots:
    void readStandardError();
    void readStandardOutput();
    void deployProcessFinished();

protected:
    ErrorDumper dumper;
    MaemoRunConfiguration *runConfig; // TODO this pointer can be invalid

private:
    QProcess deployProcess;
    bool deployingExecutable;
    bool deployingDumperLib;
};

class MaemoRunControl : public AbstractMaemoRunControl
{
    Q_OBJECT
public:
    MaemoRunControl(RunConfiguration *runConfiguration);
    ~MaemoRunControl();
    void start();
    void stop();
    bool isRunning() const;

private slots:
    void executionFinished();

private:
    void deploymentFinished(bool success);
    void startExecution();

    QProcess sshProcess;
    QProcess stopProcess;
    bool stoppedByUser;
};

class MaemoDebugRunControl : public AbstractMaemoRunControl
{
    Q_OBJECT
public:
    MaemoDebugRunControl(RunConfiguration *runConfiguration);
    ~MaemoDebugRunControl();
    void start();
    void stop();
    bool isRunning() const;
    Q_SLOT void debuggingFinished();

signals:
    void stopRequested();

private slots:
    void gdbServerStarted();
    void debuggerOutput(const QString &output);

private:
    void deploymentFinished(bool success);

    void startGdbServer();
    void gdbServerStartFailed(const QString &reason);
    void startDebugging();

    QProcess gdbServer;
    QProcess stopProcess;
    const QString gdbServerPort;
    Debugger::DebuggerManager *debuggerManager;
    QSharedPointer<Debugger::DebuggerStartParameters> startParams;
    int inferiorPid;
};

void ErrorDumper::printToStream(QProcess::ProcessError error)
{
    QString reason;
    switch (error) {
        case QProcess::FailedToStart:
            reason = "The process failed to start. Either the invoked program is"
                " missing, or you may have insufficient permissions to invoke "
                "the program.";
            break;

        case QProcess::Crashed:
            reason = "The process crashed some time after starting successfully.";
            break;

        case QProcess::Timedout:
            reason = "The last waitFor...() function timed out. The state of "
                "QProcess is unchanged, and you can try calling waitFor...() "
                "again.";
            break;

        case QProcess::WriteError:
            reason = "An error occurred when attempting to write to the process."
                " For example, the process may not be running, or it may have "
                "closed its input channel.";
            break;

        case QProcess::ReadError:
            reason = "An error occurred when attempting to read from the process."
                " For example, the process may not be running.";
            break;

        default:
            reason = "QProcess::UnknownError";
            break;
    }
    qWarning() << "Failed to run emulator. Reason:" << reason;
}


// #pragma mark -- MaemoRunConfiguration


static const QLatin1String RemoteHostIsSimulatorKey("RemoteHostIsSimulator");
static const QLatin1String ArgumentsKeySim("ArgumentsSim");
static const QLatin1String RemoteHostNameKeySim("RemoteHostNameSim");
static const QLatin1String RemoteUserNameKeySim("RemoteUserNameSim");
static const QLatin1String RemotePortKeySim("RemotePortSim");

static const QLatin1String ArgumentsKeyDevice("ArgumentsDevice");
static const QLatin1String RemoteHostNameKeyDevice("RemoteHostNameDevice");
static const QLatin1String RemoteUserNameKeyDevice("RemoteUserNameDevice");
static const QLatin1String RemotePortKeyDevice("RemotePortDevice");

static const QLatin1String LastDeployedKey("LastDeployed");
static const QLatin1String DebuggingHelpersLastDeployedKey(
    "DebuggingHelpersLastDeployed");

static const QLatin1String SimulatorPath("SimulatorPath");
static const QLatin1String IsUserSetSimulator("IsUserSetSimulator");

#if USE_SSL_PASSWORD
static const QLatinString RemoteUserPasswordKey("RemoteUserPassword");
static const QLatinString RemoteHostRequiresPasswordKey(
    "RemoteHostRequiresPassword");
#endif

MaemoRunConfiguration::MaemoRunConfiguration(Project *project,
        const QString &proFilePath)
    : RunConfiguration(project)
    , m_proFilePath(proFilePath)
    , m_cachedTargetInformationValid(false)
    , m_cachedSimulatorInformationValid(false)
    , m_isUserSetSimulator(false)
    , m_remotePortSim(22)
    , m_remotePortDevice(22)
    , qemu(0)
{
    if (!m_proFilePath.isEmpty()) {
        setName(tr("%1 on Maemo device").arg(QFileInfo(m_proFilePath)
            .completeBaseName()));
    } else {
        setName(tr("MaemoRunConfiguration"));
    }

    connect(project, SIGNAL(targetInformationChanged()), this,
        SLOT(invalidateCachedTargetInformation()));
    connect(project, SIGNAL(activeBuildConfigurationChanged()), this,
        SLOT(invalidateCachedTargetInformation()));

    connect(project, SIGNAL(targetInformationChanged()), this,
        SLOT(invalidateCachedSimulatorInformation()));
    connect(project, SIGNAL(activeBuildConfigurationChanged()), this,
        SLOT(invalidateCachedSimulatorInformation()));

    qemu = new QProcess(this);
    connect(qemu, SIGNAL(error(QProcess::ProcessError)), &dumper,
        SLOT(printToStream(QProcess::ProcessError)));
    connect(qemu, SIGNAL(finished(int, QProcess::ExitStatus)), this,
        SLOT(qemuProcessFinished()));
}

MaemoRunConfiguration::~MaemoRunConfiguration()
{
    if (qemu && qemu->state() != QProcess::NotRunning) {
        qemu->terminate();
        qemu->kill();
    }
    delete qemu;
    qemu = NULL;
}

QString MaemoRunConfiguration::type() const
{
    return QLatin1String("Qt4ProjectManager.MaemoRunConfiguration");
}

Qt4Project *MaemoRunConfiguration::project() const
{
    Qt4Project *pro = qobject_cast<Qt4Project *>(RunConfiguration::project());
    Q_ASSERT(pro != 0);
    return pro;
}

bool MaemoRunConfiguration::isEnabled() const
{
    Qt4Project *qt4Project = qobject_cast<Qt4Project*>(project());
    QTC_ASSERT(qt4Project, return false);
    ToolChain::ToolChainType type =
        qt4Project->toolChainType(qt4Project->activeBuildConfiguration());
    return type == ToolChain::GCC_MAEMO;
}

QWidget *MaemoRunConfiguration::configurationWidget()
{
    return new MaemoRunConfigurationWidget(this);
}

void MaemoRunConfiguration::save(PersistentSettingsWriter &writer) const
{
    writer.saveValue(RemoteHostIsSimulatorKey, m_remoteHostIsSimulator);
    writer.saveValue(ArgumentsKeySim, m_argumentsSim);
    writer.saveValue(RemoteHostNameKeySim, m_remoteHostNameSim);
    writer.saveValue(RemoteUserNameKeySim, m_remoteUserNameSim);
    writer.saveValue(RemotePortKeySim, m_remotePortSim);

    writer.saveValue(ArgumentsKeyDevice, m_argumentsDevice);
    writer.saveValue(RemoteHostNameKeyDevice, m_remoteHostNameDevice);
    writer.saveValue(RemoteUserNameKeyDevice, m_remoteUserNameDevice);
    writer.saveValue(RemotePortKeyDevice, m_remotePortDevice);

#if USE_SSL_PASSWORD
    writer.saveValue(RemoteUserPasswordKey, m_remoteUserPassword);
    writer.saveValue(RemoteHostRequiresPasswordKey, m_remoteHostRequiresPassword);
#endif

    writer.saveValue(LastDeployedKey, m_lastDeployed);
    writer.saveValue(DebuggingHelpersLastDeployedKey,
        m_debuggingHelpersLastDeployed);

    writer.saveValue(SimulatorPath, m_simulatorPath);
    writer.saveValue(IsUserSetSimulator, m_isUserSetSimulator);

    const QDir &dir = QFileInfo(project()->file()->fileName()).absoluteDir();
    writer.saveValue("ProFile", dir.relativeFilePath(m_proFilePath));

    RunConfiguration::save(writer);
}

void MaemoRunConfiguration::restore(const PersistentSettingsReader &reader)
{
    RunConfiguration::restore(reader);

    m_remoteHostIsSimulator = reader.restoreValue(RemoteHostIsSimulatorKey)
        .toBool();
    m_argumentsSim = reader.restoreValue(ArgumentsKeySim).toStringList();
    m_remoteHostNameSim = reader.restoreValue(RemoteHostNameKeySim).toString();
    m_remoteUserNameSim = reader.restoreValue(RemoteUserNameKeySim).toString();
    m_remotePortSim = reader.restoreValue(RemotePortKeySim).toInt();

    m_argumentsDevice = reader.restoreValue(ArgumentsKeyDevice).toStringList();
    m_remoteHostNameDevice = reader.restoreValue(RemoteHostNameKeyDevice)
        .toString();
    m_remoteUserNameDevice = reader.restoreValue(RemoteUserNameKeyDevice)
        .toString();
    m_remotePortDevice = reader.restoreValue(RemotePortKeyDevice).toInt();

#if USE_SSL_PASSWORD
    m_remoteUserPassword = reader.restoreValue(RemoteUserPasswordKey).toString();
    m_remoteHostRequiresPassword =
        reader.restoreValue(RemoteHostRequiresPasswordKey).toBool();
#endif

    m_lastDeployed = reader.restoreValue(LastDeployedKey).toDateTime();
    m_debuggingHelpersLastDeployed =
        reader.restoreValue(DebuggingHelpersLastDeployedKey).toDateTime();

    m_simulatorPath = reader.restoreValue(SimulatorPath).toString();
    m_isUserSetSimulator = reader.restoreValue(IsUserSetSimulator).toBool();

    const QDir &dir = QFileInfo(project()->file()->fileName()).absoluteDir();
    m_proFilePath = dir.filePath(reader.restoreValue("ProFile").toString());
}

bool MaemoRunConfiguration::currentlyNeedsDeployment() const
{
    return fileNeedsDeployment(executable(), m_lastDeployed);
}

void MaemoRunConfiguration::wasDeployed()
{
    m_lastDeployed = QDateTime::currentDateTime();
}

bool MaemoRunConfiguration::hasDebuggingHelpers() const
{
    return project()->qtVersion(project()->activeBuildConfiguration())
        ->hasDebuggingHelper();
}

bool MaemoRunConfiguration::debuggingHelpersNeedDeployment() const
{
    if (hasDebuggingHelpers())
        return fileNeedsDeployment(dumperLib(), m_debuggingHelpersLastDeployed);
    return false;
}

void MaemoRunConfiguration::debuggingHelpersDeployed()
{
    m_debuggingHelpersLastDeployed = QDateTime::currentDateTime();
}

bool MaemoRunConfiguration::fileNeedsDeployment(const QString &path,
    const QDateTime &lastDeployed) const
{
    return !lastDeployed.isValid()
        || QFileInfo(path).lastModified() > lastDeployed;
}

const QString MaemoRunConfiguration::remoteHostName() const
{
    return m_remoteHostIsSimulator ? m_remoteHostNameSim
        : m_remoteHostNameDevice;
}

const QString MaemoRunConfiguration::remoteUserName() const
{
    return m_remoteHostIsSimulator ? m_remoteUserNameSim
        : m_remoteUserNameDevice;
}

int MaemoRunConfiguration::remotePort() const
{
    int port = m_remoteHostIsSimulator ? m_remotePortSim : m_remotePortDevice;
    return port > 0 ? port : 22;
}

const QString MaemoRunConfiguration::remoteDir() const
{
    return remoteUserName() == QString::fromLocal8Bit("root")
        ? QString::fromLocal8Bit("/root")
        : QString::fromLocal8Bit("/home/") + remoteUserName();
}

const QString MaemoRunConfiguration::sshCmd() const
{
    return cmd(QString::fromLocal8Bit("ssh"));
}

const QString MaemoRunConfiguration::scpCmd() const
{
    return cmd(QString::fromLocal8Bit("scp"));
}

const QString MaemoRunConfiguration::cmd(const QString &cmdName) const
{
    QString command(cmdName);
#ifdef Q_OS_WIN
    command = maddeRoot() + QLatin1String("/bin/") + command
          + QLatin1String(".exe");
#endif
    return command;
}

const MaemoToolChain *MaemoRunConfiguration::toolchain() const
{
    Qt4Project *qt4Project = qobject_cast<Qt4Project *>(project());
    QTC_ASSERT(qt4Project != 0, return 0);
    MaemoToolChain *tc = dynamic_cast<MaemoToolChain *>(
        qt4Project->toolChain(qt4Project->activeBuildConfiguration()) );
    QTC_ASSERT(tc != 0, return 0);
    return tc;
}

const QString MaemoRunConfiguration::gdbCmd() const
{
    return toolchain() != 0
            ? toolchain()->targetRoot() + "/bin/gdb"
            : QString();
}

QString MaemoRunConfiguration::maddeRoot() const
{
    return toolchain() != 0 ? toolchain()->maddeRoot() : QString();
}

const QString MaemoRunConfiguration::sysRoot() const
{
    return toolchain() != 0 ? toolchain()->sysrootRoot() : QString();
}

const QStringList MaemoRunConfiguration::arguments() const
{
    return m_remoteHostIsSimulator ? m_argumentsSim : m_argumentsDevice;
}

const QString MaemoRunConfiguration::dumperLib() const
{
    return project()->qtVersion(project()->activeBuildConfiguration())->
        debuggingHelperLibrary();
}

QString MaemoRunConfiguration::executable() const
{
    const_cast<MaemoRunConfiguration*> (this)->updateTarget();
    return m_executable;
}

QString MaemoRunConfiguration::simulatorPath() const
{
    qDebug("MaemoRunConfiguration::simulatorPath() called, %s",
        qPrintable(m_simulatorPath));

    const_cast<MaemoRunConfiguration*> (this)->updateSimulatorInformation();
    return m_simulatorPath;
}

QString MaemoRunConfiguration::visibleSimulatorParameter() const
{
    qDebug("MaemoRunConfiguration::visibleSimulatorParameter() called");

    const_cast<MaemoRunConfiguration*> (this)->updateSimulatorInformation();
    return m_visibleSimulatorParameter;
}

QString MaemoRunConfiguration::simulator() const
{
    const_cast<MaemoRunConfiguration*> (this)->updateSimulatorInformation();
    return m_simulator;
}

QString MaemoRunConfiguration::simulatorArgs() const
{
    const_cast<MaemoRunConfiguration*> (this)->updateSimulatorInformation();
    return m_simulatorArgs;
}

void MaemoRunConfiguration::setArguments(const QStringList &args)
{
    if (m_remoteHostIsSimulator)
        m_argumentsSim = args;
    else
        m_argumentsDevice = args;
}

void MaemoRunConfiguration::setRemoteHostIsSimulator(bool isSimulator)
{
    m_remoteHostIsSimulator = isSimulator;
}

void MaemoRunConfiguration::setRemoteHostName(const QString &hostName)
{
    m_lastDeployed = QDateTime();
    m_debuggingHelpersLastDeployed = QDateTime();

    if (m_remoteHostIsSimulator)
        m_remoteHostNameSim = hostName;
    else
        m_remoteHostNameDevice = hostName;
}

void MaemoRunConfiguration::setRemoteUserName(const QString &userName)
{
    m_lastDeployed = QDateTime();
    m_debuggingHelpersLastDeployed = QDateTime();

    if (m_remoteHostIsSimulator)
        m_remoteUserNameSim = userName;
    else
        m_remoteUserNameDevice = userName;
}

void MaemoRunConfiguration::setRemotePort(int port)
{
    m_lastDeployed = QDateTime();
    m_debuggingHelpersLastDeployed = QDateTime();

    if (m_remoteHostIsSimulator)
        m_remotePortSim = port;
    else
        m_remotePortDevice = port;
}

#if USE_SSL_PASSWORD
void MaemoRunConfiguration::setRemotePassword(const QString &password)
{
    Q_ASSERT(remoteHostRequiresPassword());
    m_remoteUserPassword = password;
}

void MaemoRunConfiguration::setRemoteHostRequiresPassword(bool requiresPassword)
{
    m_remoteHostRequiresPassword = requiresPassword;
}
#endif

bool MaemoRunConfiguration::isQemuRunning() const
{
    return (qemu && qemu->state() != QProcess::NotRunning);
}

void MaemoRunConfiguration::invalidateCachedTargetInformation()
{
    m_cachedTargetInformationValid = false;
    emit targetInformationChanged();
}

void MaemoRunConfiguration::setUserSimulatorPath(const QString &path)
{
    qDebug("MaemoRunConfiguration::setUserSimulatorPath() called, "
        "m_simulatorPath: %s, new path: %s", qPrintable(m_simulatorPath),
        qPrintable(path));

    m_isUserSetSimulator = true;
    if (m_userSimulatorPath != path)
        m_cachedSimulatorInformationValid = false;

    m_userSimulatorPath = path;
    emit cachedSimulatorInformationChanged();
}

void MaemoRunConfiguration::invalidateCachedSimulatorInformation()
{
    qDebug("MaemoRunConfiguration::invalidateCachedSimulatorInformation() "
        "called");

    m_cachedSimulatorInformationValid = false;
    emit cachedSimulatorInformationChanged();
}

void MaemoRunConfiguration::resetCachedSimulatorInformation()
{
    m_userSimulatorPath.clear();
    m_isUserSetSimulator = false;

    m_cachedSimulatorInformationValid = false;
    emit cachedSimulatorInformationChanged();
}

void MaemoRunConfiguration::updateTarget()
{
    if (m_cachedTargetInformationValid)
        return;

    m_executable = QString::null;
    m_cachedTargetInformationValid = true;

    if (Qt4Project *qt4Project = static_cast<Qt4Project *>(project())) {
        Qt4PriFileNode * priFileNode = qt4Project->rootProjectNode()
            ->findProFileFor(m_proFilePath);
        if (!priFileNode) {
            emit targetInformationChanged();
            return;
        }

        QtVersion *qtVersion =
            qt4Project->qtVersion(qt4Project->activeBuildConfiguration());
        ProFileReader *reader = priFileNode->createProFileReader();
        reader->setCumulative(false);
        reader->setQtVersion(qtVersion);

        // Find out what flags we pass on to qmake, this code is duplicated in
        // the qmake step
        QtVersion::QmakeBuildConfigs defaultBuildConfiguration =
            qtVersion->defaultBuildConfig();
        QtVersion::QmakeBuildConfig projectBuildConfiguration =
            QtVersion::QmakeBuildConfig(qt4Project->activeBuildConfiguration()
                ->value("buildConfiguration").toInt());

        QStringList addedUserConfigArguments;
        QStringList removedUserConfigArguments;
        if ((defaultBuildConfiguration & QtVersion::BuildAll)
            && !(projectBuildConfiguration & QtVersion::BuildAll))
            removedUserConfigArguments << "debug_and_release";

        if (!(defaultBuildConfiguration & QtVersion::BuildAll)
            && (projectBuildConfiguration & QtVersion::BuildAll))
            addedUserConfigArguments << "debug_and_release";

        if ((defaultBuildConfiguration & QtVersion::DebugBuild)
            && !(projectBuildConfiguration & QtVersion::DebugBuild))
            addedUserConfigArguments << "release";

        if (!(defaultBuildConfiguration & QtVersion::DebugBuild)
            && (projectBuildConfiguration & QtVersion::DebugBuild))
            addedUserConfigArguments << "debug";

        reader->setUserConfigCmdArgs(addedUserConfigArguments,
            removedUserConfigArguments);

        if (!reader->readProFile(m_proFilePath)) {
            delete reader;
            Core::ICore::instance()->messageManager()->printToOutputPane(tr(
                "Could not parse %1. The Maemo run configuration %2 "
                "can not be started.").arg(m_proFilePath).arg(name()));
            emit targetInformationChanged();
            return;
        }

        // Extract data
        QDir baseProjectDirectory =
            QFileInfo(project()->file()->fileName()).absoluteDir();
        QString relSubDir =
            baseProjectDirectory.relativeFilePath(QFileInfo(m_proFilePath).path());
        QDir baseBuildDirectory =
            project()->buildDirectory(project()->activeBuildConfiguration());
        QString baseDir = baseBuildDirectory.absoluteFilePath(relSubDir);

        if (!reader->contains("DESTDIR")) {
#if 0   // TODO: fix this, seems to be wrong on windows
            if (reader->values("CONFIG").contains("debug_and_release_target")) {
                QString qmakeBuildConfig = "release";
                if (projectBuildConfiguration & QtVersion::DebugBuild)
                    qmakeBuildConfig = "debug";
                baseDir += QLatin1Char('/') + qmakeBuildConfig;
            }
#endif
        } else {
            const QString &destDir = reader->value("DESTDIR");
            if (QDir::isRelativePath(destDir))
                baseDir += QLatin1Char('/') + destDir;
            else
                baseDir = destDir;
        }

        QString target = reader->value("TARGET");
        if (target.isEmpty())
            target = QFileInfo(m_proFilePath).baseName();

        m_executable = QDir::cleanPath(baseDir + QLatin1Char('/') + target);
        delete reader;
    }

    emit targetInformationChanged();
}

void MaemoRunConfiguration::updateSimulatorInformation()
{
    if (m_cachedSimulatorInformationValid)
        return;

    m_simulator = QString::null;
    m_simulatorArgs == QString::null;
    m_cachedSimulatorInformationValid = true;
    m_simulatorPath = QDir::toNativeSeparators(m_userSimulatorPath);
    m_visibleSimulatorParameter = tr("Could not autodetect target simulator, "
        "please choose one on your own.");

    if (!m_isUserSetSimulator)
        m_simulatorPath = QDir::toNativeSeparators(toolchain()->simulatorRoot());

    if (!m_simulatorPath.isEmpty()) {
        m_visibleSimulatorParameter = tr("'%1' is not a valid Maemo simulator.")
            .arg(m_simulatorPath);
    }

    QDir dir(m_simulatorPath);
    if (dir.exists(m_simulatorPath)) {
        const QStringList &files = dir.entryList(QDir::Files | QDir::NoSymLinks
            | QDir::NoDotAndDotDot);
        if (files.count() >= 2) {
            const QLatin1String info("information");
            if (files.contains(info)) {
                QFile file(m_simulatorPath + QLatin1Char('/') + info);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QMap<QString, QString> map;
                    QTextStream stream(&file);
                    while (!stream.atEnd()) {
                        const QString &line = stream.readLine().trimmed();
                        const int index = line.indexOf(QLatin1Char('='));
                        map.insert(line.mid(0, index).remove(QLatin1Char('\'')),
                            line.mid(index + 1).remove(QLatin1Char('\'')));
                    }

                    m_simulator = map.value(QLatin1String("runcommand"));
                    m_simulatorArgs = map.value(QLatin1String("runcommand_args"));

                    m_visibleSimulatorParameter = m_simulator
#ifdef Q_OS_WIN
                        + QLatin1String(".exe")
#endif
                        + QLatin1Char(' ') + m_simulatorArgs;
                }
            }
        }
    } else {
        m_visibleSimulatorParameter = tr("'%1' could not be found. Please "
            "choose a simulator on your own.").arg(m_simulatorPath);
    }

    emit cachedSimulatorInformationChanged();
}

void MaemoRunConfiguration::startStopQemu()
{
    if (qemu->state() != QProcess::NotRunning) {
        if (qemu->state() == QProcess::Running) {
            qemu->terminate();
            qemu->kill();
            emit qemuProcessStatus(false);
        }
        return;
    }

    QString root = maddeRoot();
    if (root.isEmpty() || simulator().isEmpty())
        return;

    const QLatin1Char colon(';');
    const QString path = QDir::toNativeSeparators(root + QLatin1Char('/'));

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PATH", env.value("Path") + colon + path + QLatin1String("bin"));
    env.insert("PATH", env.value("Path") + colon + path + QLatin1String("madlib"));

    qemu->setProcessEnvironment(env);
    qemu->setWorkingDirectory(simulatorPath());

    QString app = root + QLatin1String("/madlib/") + simulator()
#ifdef Q_OS_WIN
        + QLatin1String(".exe")
#endif
    ;   // keep

    qemu->start(app + QLatin1Char(' ') + simulatorArgs(), QIODevice::ReadWrite);
    emit qemuProcessStatus(qemu->waitForStarted());
}

void MaemoRunConfiguration::qemuProcessFinished()
{
    emit qemuProcessStatus(false);
}

void MaemoRunConfiguration::enabledStateChanged()
{
    MaemoManager::instance()->setQemuSimulatorStarterEnabled(isEnabled());
}


// #pragma mark -- MaemoRunConfigurationWidget

MaemoRunConfigurationWidget::MaemoRunConfigurationWidget(
        MaemoRunConfiguration *runConfiguration, QWidget *parent)
    : QWidget(parent)
    , m_runConfiguration(runConfiguration)
{
    QFormLayout *mainLayout = new QFormLayout;
    setLayout(mainLayout);

    mainLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_configNameLineEdit = new QLineEdit(m_runConfiguration->name());
    mainLayout->addRow(tr("Run configuration name:"), m_configNameLineEdit);
    m_executableLabel = new QLabel(m_runConfiguration->executable());
    mainLayout->addRow(tr("Executable:"), m_executableLabel);
    m_argsLineEdit = new QLineEdit(m_runConfiguration->arguments().join(" "));
    mainLayout->addRow(tr("Arguments:"), m_argsLineEdit);
    m_debuggerLabel = new QLabel(m_runConfiguration->gdbCmd());
    mainLayout->addRow(tr("Debugger:"), m_debuggerLabel);
    mainLayout->addItem(new QSpacerItem(10, 10));

    QHBoxLayout *hostTypeLayout = new QHBoxLayout;
    m_hwButton = new QRadioButton(tr("Physical device"));
    hostTypeLayout->addWidget(m_hwButton);
    m_simButton = new QRadioButton(tr("Simulator"));
    hostTypeLayout->addWidget(m_simButton);
    hostTypeLayout->addStretch(1);
    mainLayout->addRow(tr("Remote host type:"), hostTypeLayout);

    m_chooseSimPathLabel = new QLabel(tr("Choose simulator:"));
    m_simPathChooser = new Utils::PathChooser;
    m_simPathChooser->setPath(m_runConfiguration->simulatorPath());

    QHBoxLayout *pathLayout = new QHBoxLayout;
    pathLayout->addWidget(m_simPathChooser);

    m_resetButton = new QToolButton();
    m_resetButton->setToolTip(tr("Reset to default"));
    m_resetButton->setIcon(QIcon(":/core/images/reset.png"));
    pathLayout->addWidget(m_resetButton);

    mainLayout->addRow(m_chooseSimPathLabel, pathLayout);
    m_simParamsNameLabel = new QLabel(tr("Simulator command line:"));
    m_simParamsValueLabel= new QLabel(m_runConfiguration->visibleSimulatorParameter());
    mainLayout->addRow(m_simParamsNameLabel, m_simParamsValueLabel);

    connect(m_simPathChooser, SIGNAL(changed(QString)), m_runConfiguration,
        SLOT(setUserSimulatorPath(QString)));
    connect(m_runConfiguration, SIGNAL(cachedSimulatorInformationChanged()),
        this, SLOT(updateSimulatorPath()));
    connect(m_runConfiguration, SIGNAL(cachedSimulatorInformationChanged()),
        this, SLOT(updateVisibleSimulatorParameter()));
    connect(m_resetButton, SIGNAL(clicked()), m_runConfiguration,
        SLOT(resetCachedSimulatorInformation()));

    m_hostNameLineEdit = new QLineEdit(m_runConfiguration->remoteHostName());
    mainLayout->addRow(tr("Remote host name:"), m_hostNameLineEdit);
    m_userLineEdit = new QLineEdit(m_runConfiguration->remoteUserName());
    mainLayout->addRow(tr("Remote user name:"), m_userLineEdit);
    m_portLineEdit = new QLineEdit(QString::number(m_runConfiguration->remotePort()));
    mainLayout->addRow(tr("Remote SSH port:"), m_portLineEdit);

    // Unlikely to ever work: ssh uses /dev/tty directly instead of stdin/out
#if USE_SSL_PASSWORD
    m_passwordCheckBox = new QCheckBox(tr("Remote password:"));
    m_passwordCheckBox->setToolTip(tr("Uncheck for passwordless login"));
    m_passwordCheckBox->setChecked(m_runConfiguration
        ->remoteHostRequiresPassword());
    m_passwordLineEdit = new QLineEdit(m_runConfiguration->remoteUserPassword());
    m_passwordLineEdit->setEchoMode(QLineEdit::Password);
    m_passwordLineEdit->setEnabled(m_passwordCheckBox->isChecked());
    mainLayout->addRow(m_passwordCheckBox, m_passwordLineEdit);

    connect(m_passwordCheckBox, SIGNAL(stateChanged(int)), this,
        SLOT(passwordUseChanged()));
    connect(m_passwordLineEdit, SIGNAL(textEdited(QString)), this,
        SLOT(passwordEdited(QString)));
#endif

    connect(m_configNameLineEdit, SIGNAL(textEdited(QString)), this,
        SLOT(configNameEdited(QString)));
    connect(m_argsLineEdit, SIGNAL(textEdited(QString)), this,
        SLOT(argumentsEdited(QString)));
    connect(m_runConfiguration, SIGNAL(targetInformationChanged()), this,
        SLOT(updateTargetInformation()));
    connect(m_hwButton, SIGNAL(toggled(bool)), this, SLOT(hostTypeChanged()));
    connect(m_simButton, SIGNAL(toggled(bool)), this, SLOT(hostTypeChanged()));
    connect(m_hostNameLineEdit, SIGNAL(textEdited(QString)), this,
        SLOT(hostNameEdited(QString)));
    connect(m_userLineEdit, SIGNAL(textEdited(QString)), this,
        SLOT(userNameEdited(QString)));
    connect(m_portLineEdit, SIGNAL(textEdited(QString)), this,
        SLOT(portEdited(QString)));
    if (m_runConfiguration->remoteHostIsSimulator())
        m_simButton->setChecked(true);
    else
        m_hwButton->setChecked(true);
}

void MaemoRunConfigurationWidget::configNameEdited(const QString &text)
{
    m_runConfiguration->setName(text);
}

void MaemoRunConfigurationWidget::argumentsEdited(const QString &text)
{
    m_runConfiguration->setArguments(text.split(' ', QString::SkipEmptyParts));
}

void MaemoRunConfigurationWidget::updateTargetInformation()
{
    m_executableLabel->setText(m_runConfiguration->executable());
}

void MaemoRunConfigurationWidget::updateSimulatorPath()
{
    m_simPathChooser->setPath(m_runConfiguration->simulatorPath());
}

void MaemoRunConfigurationWidget::updateVisibleSimulatorParameter()
{
    m_simParamsValueLabel->setText(m_runConfiguration->visibleSimulatorParameter());
}

void MaemoRunConfigurationWidget::hostTypeChanged()
{
    const bool isSimulator = m_simButton->isChecked();

    m_chooseSimPathLabel->setVisible(isSimulator);
    m_simPathChooser->setVisible(isSimulator);
    m_simParamsNameLabel->setVisible(isSimulator);
    m_simParamsValueLabel->setVisible(isSimulator);
    m_resetButton->setVisible(isSimulator);

    m_runConfiguration->setRemoteHostIsSimulator(isSimulator);
    m_argsLineEdit->setText(m_runConfiguration->arguments().join(" "));
    m_hostNameLineEdit->setText(m_runConfiguration->remoteHostName());
    m_userLineEdit->setText(m_runConfiguration->remoteUserName());
    m_portLineEdit->setText(QString::number(m_runConfiguration->remotePort()));
}

void MaemoRunConfigurationWidget::hostNameEdited(const QString &hostName)
{
    m_runConfiguration->setRemoteHostName(hostName);
}

void MaemoRunConfigurationWidget::userNameEdited(const QString &userName)
{
    m_runConfiguration->setRemoteUserName(userName);
}

#if USE_SSL_PASSWORD
void MaemoRunConfigurationWidget::passwordUseChanged()
{
    const bool usePassword = m_passwordCheckBox->checkState() == Qt::Checked;
    m_passwordLineEdit->setEnabled(usePassword);
    m_runConfiguration->setRemoteHostRequiresPassword(usePassword);
}

void MaemoRunConfigurationWidget::passwordEdited(const QString &password)
{
    m_runConfiguration->setRemotePassword(password);
}
#endif

void MaemoRunConfigurationWidget::portEdited(const QString &portString)
{
    bool isValidString;
    int port = portString.toInt(&isValidString);
    if (isValidString)
        m_runConfiguration->setRemotePort(port);
    else
        m_portLineEdit->setText(QString::number(m_runConfiguration->remotePort()));
}


// #pragma mark -- MaemoRunConfigurationFactory


MaemoRunConfigurationFactory::MaemoRunConfigurationFactory(QObject* parent)
    : IRunConfigurationFactory(parent)
{
}

MaemoRunConfigurationFactory::~MaemoRunConfigurationFactory()
{
}

bool MaemoRunConfigurationFactory::canRestore(const QString &type) const
{
    return type == "Qt4ProjectManager.MaemoRunConfiguration";
}

QStringList MaemoRunConfigurationFactory::availableCreationTypes(
    Project *pro) const
{
    Qt4Project *qt4project = qobject_cast<Qt4Project *>(pro);
    if (qt4project) {
        QStringList applicationProFiles;
        QList<Qt4ProFileNode *> list = qt4project->applicationProFiles();
        foreach (Qt4ProFileNode * node, list) {
            applicationProFiles.append("MaemoRunConfiguration." + node->path());
        }
        return applicationProFiles;
    }
    return QStringList();
}

QString MaemoRunConfigurationFactory::displayNameForType(
    const QString &type) const
{
    const int size = QString::fromLocal8Bit("MaemoRunConfiguration.").size();
    return tr("%1 on Maemo Device").arg(QFileInfo(type.mid(size))
        .completeBaseName());
}

RunConfiguration *MaemoRunConfigurationFactory::create(Project *project,
    const QString &type)
{
    Qt4Project *qt4project = qobject_cast<Qt4Project *>(project);
    Q_ASSERT(qt4project);

    connect(project, SIGNAL(addedRunConfiguration(ProjectExplorer::Project*,
        QString)), this, SLOT(addedRunConfiguration(ProjectExplorer::Project*)));
    connect(project, SIGNAL(removedRunConfiguration(ProjectExplorer::Project*,
        QString)), this, SLOT(removedRunConfiguration(ProjectExplorer::Project*)));

    RunConfiguration *rc = 0;
    const QLatin1String prefix("MaemoRunConfiguration.");
    if (type.startsWith(prefix)) {
        rc = new MaemoRunConfiguration(qt4project, type.mid(QString(prefix).size()));
    } else {
        Q_ASSERT(type == "Qt4ProjectManager.MaemoRunConfiguration");
        rc = new MaemoRunConfiguration(qt4project, QString::null);
    }

    if (rc) {
        connect(project, SIGNAL(runConfigurationsEnabledStateChanged()),
            rc, SLOT(enabledStateChanged()));
        connect(MaemoManager::instance(), SIGNAL(startStopQemu()), rc,
            SLOT(startStopQemu()));
        connect(rc, SIGNAL(qemuProcessStatus(bool)),
            MaemoManager::instance(), SLOT(updateQemuSimulatorStarter(bool)));
    }

    ProjectExplorerPlugin *explorer = ProjectExplorerPlugin::instance();
    connect(explorer->session(), SIGNAL(projectAdded(ProjectExplorer::Project*)),
        this, SLOT(projectAdded(ProjectExplorer::Project*)));
    connect(explorer->session(), SIGNAL(projectRemoved(ProjectExplorer::Project*)),
        this, SLOT(projectRemoved(ProjectExplorer::Project*)));
    connect(explorer, SIGNAL(currentProjectChanged(ProjectExplorer::Project*)),
        this, SLOT(currentProjectChanged(ProjectExplorer::Project*)));

    return rc;
}

bool hasMaemoRunConfig(ProjectExplorer::Project* project)
{
    if (Qt4Project *qt4Project = qobject_cast<Qt4Project *>(project)) {
        QList<RunConfiguration *> list = qt4Project->runConfigurations();
        foreach (RunConfiguration *rc, list) {
            if (qobject_cast<MaemoRunConfiguration *>(rc))
                return true;
        }
    }
    return false;
}

void MaemoRunConfigurationFactory::addedRunConfiguration(
    ProjectExplorer::Project* project)
{
    if (hasMaemoRunConfig(project))
        MaemoManager::instance()->addQemuSimulatorStarter(project);
}

void MaemoRunConfigurationFactory::removedRunConfiguration(
    ProjectExplorer::Project* project)
{
    if (!hasMaemoRunConfig(project))
        MaemoManager::instance()->removeQemuSimulatorStarter(project);
}

void MaemoRunConfigurationFactory::projectAdded(
    ProjectExplorer::Project* project)
{
    if (hasMaemoRunConfig(project))
        MaemoManager::instance()->addQemuSimulatorStarter(project);
}

void MaemoRunConfigurationFactory::projectRemoved(
    ProjectExplorer::Project* project)
{
    if (hasMaemoRunConfig(project))
        MaemoManager::instance()->removeQemuSimulatorStarter(project);
}

void MaemoRunConfigurationFactory::currentProjectChanged(
    ProjectExplorer::Project* project)
{
    bool hasRunConfig = hasMaemoRunConfig(project);
    MaemoManager::instance()->setQemuSimulatorStarterEnabled(hasRunConfig);

    bool isRunning = false;
    if (Qt4Project *qt4Project = qobject_cast<Qt4Project *>(project)) {
        RunConfiguration *rc = qt4Project->activeRunConfiguration();
        if (MaemoRunConfiguration *mrc = qobject_cast<MaemoRunConfiguration *>(rc))
            isRunning = mrc->isQemuRunning();
    }
    MaemoManager::instance()->updateQemuSimulatorStarter(isRunning);
}


// #pragma mark -- MaemoRunControlFactory


MaemoRunControlFactory::MaemoRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

bool MaemoRunControlFactory::canRun(RunConfiguration *runConfiguration,
    const QString &mode) const
{
    return qobject_cast<MaemoRunConfiguration *>(runConfiguration)
        && (mode == ProjectExplorer::Constants::RUNMODE
        || mode == ProjectExplorer::Constants::DEBUGMODE);
}

RunControl* MaemoRunControlFactory::create(RunConfiguration *runConfig,
    const QString &mode)
{
    MaemoRunConfiguration *rc = qobject_cast<MaemoRunConfiguration *>(runConfig);
    Q_ASSERT(rc);
    Q_ASSERT(mode == ProjectExplorer::Constants::RUNMODE
        || mode == ProjectExplorer::Constants::DEBUGMODE);
    if (mode == ProjectExplorer::Constants::RUNMODE)
        return new MaemoRunControl(rc);
    return new MaemoDebugRunControl(rc);
}

QString MaemoRunControlFactory::displayName() const
{
    return tr("Run on device");
}

QWidget* MaemoRunControlFactory::configurationWidget(RunConfiguration *config)
{
    Q_UNUSED(config)
    return 0;
}


// #pragma mark -- AbstractMaemoRunControl


AbstractMaemoRunControl::AbstractMaemoRunControl(RunConfiguration *rc)
    : RunControl(rc)
    , runConfig(qobject_cast<MaemoRunConfiguration *>(rc))
{
    setProcessEnvironment(deployProcess);

    connect(&deployProcess, SIGNAL(readyReadStandardError()), this,
        SLOT(readStandardError()));
    connect(&deployProcess, SIGNAL(readyReadStandardOutput()), this,
        SLOT(readStandardOutput()));
    connect(&deployProcess, SIGNAL(error(QProcess::ProcessError)), &dumper,
        SLOT(printToStream(QProcess::ProcessError)));
    connect(&deployProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this,
        SLOT(deployProcessFinished()));
}

void AbstractMaemoRunControl::startDeployment(bool forDebugging)
{
    QTC_ASSERT(runConfig, return);
    QStringList deployables;
    if (runConfig->currentlyNeedsDeployment()) {
        deployingExecutable = true;
        deployables << executableFileName();
    } else {
        deployingExecutable = false;
    }
    if (forDebugging && runConfig->debuggingHelpersNeedDeployment()) {
        deployables << runConfig->dumperLib();
        deployingDumperLib = true;
    } else {
        deployingDumperLib = false;
    }
    if (!deployables.isEmpty()) {
        emit addToOutputWindow(this, tr("Files to deploy: %1.")
            .arg(deployables.join(" ")));
        QStringList cmdArgs;
        cmdArgs << "-P" << port() << deployables << (runConfig->remoteUserName()
            + "@" + runConfig->remoteHostName() + ":" + runConfig->remoteDir());
        deployProcess.setWorkingDirectory(QFileInfo(executableOnHost()).absolutePath());
        deployProcess.start(runConfig->scpCmd(), cmdArgs);
        if (!deployProcess.waitForStarted()) {
            emit error(this, tr("Could not start scp. Deployment failed."));
            deployProcess.kill();
        } else {
            emit started();
        }
    } else {
        deploymentFinished(true);
    }
}

void AbstractMaemoRunControl::stopDeployment()
{
    deployProcess.kill();
}

bool AbstractMaemoRunControl::isDeploying() const
{
    return deployProcess.state() != QProcess::NotRunning;
}

void AbstractMaemoRunControl::deployProcessFinished()
{
    bool success;
    if (deployProcess.exitCode() == 0) {
        emit addToOutputWindow(this, tr("Target deployed."));
        success = true;
        if (deployingExecutable)
            runConfig->wasDeployed();
        if (deployingDumperLib)
            runConfig->debuggingHelpersDeployed();
    } else {
        emit error(this, tr("Deployment failed."));
        success = false;
    }
    deploymentFinished(success);
}

const QString AbstractMaemoRunControl::executableOnHost() const
{
    return runConfig->executable();
}

const QString AbstractMaemoRunControl::port() const
{
    return QString::number(runConfig->remotePort());
}

const QString AbstractMaemoRunControl::executableFileName() const
{
    return QFileInfo(executableOnHost()).fileName();
}

const QString AbstractMaemoRunControl::executableOnTarget() const
{
    return QString::fromLocal8Bit("%1/%2").arg(runConfig->remoteDir()).
        arg(executableFileName());
}

const QString AbstractMaemoRunControl::targetCmdLinePrefix() const
{
    return QString::fromLocal8Bit("chmod u+x %1; source /etc/profile; ").
        arg(executableOnTarget());
}

bool AbstractMaemoRunControl::setProcessEnvironment(QProcess &process)
{
    QTC_ASSERT(runConfig, return false);
    Qt4Project *qt4Project = qobject_cast<Qt4Project *>(runConfig->project());
    QTC_ASSERT(qt4Project, return false);
    Environment env = Environment::systemEnvironment();
    qt4Project->toolChain(qt4Project->activeBuildConfiguration())
        ->addToEnvironment(env);
    process.setEnvironment(env.toStringList());

    return true;
}

void AbstractMaemoRunControl::readStandardError()
{
    QProcess *process = static_cast<QProcess *>(sender());
    const QByteArray &data = process->readAllStandardError();
    emit addToOutputWindow(this, QString::fromLocal8Bit(data.constData(),
        data.length()));
}

void AbstractMaemoRunControl::readStandardOutput()
{
    QProcess *process = static_cast<QProcess *>(sender());
    const QByteArray &data = process->readAllStandardOutput();
    emit addToOutputWindow(this, QString::fromLocal8Bit(data.constData(),
        data.length()));
}


// #pragma mark -- MaemoRunControl


MaemoRunControl::MaemoRunControl(RunConfiguration *runConfiguration)
    : AbstractMaemoRunControl(runConfiguration)
{
    setProcessEnvironment(sshProcess);
    setProcessEnvironment(stopProcess);

    connect(&sshProcess, SIGNAL(readyReadStandardError()), this,
        SLOT(readStandardError()));
    connect(&sshProcess, SIGNAL(readyReadStandardOutput()), this,
        SLOT(readStandardOutput()));
    connect(&sshProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this,
        SLOT(executionFinished()));
    connect(&sshProcess, SIGNAL(error(QProcess::ProcessError)), &dumper,
        SLOT(printToStream(QProcess::ProcessError)));
}

MaemoRunControl::~MaemoRunControl()
{
    stop();
    stopProcess.waitForFinished(5000);
}

void MaemoRunControl::start()
{
    stoppedByUser = false;
    startDeployment(false);
}

void MaemoRunControl::deploymentFinished(bool success)
{
    if (success)
        startExecution();
    else
        emit finished();
}

void MaemoRunControl::startExecution()
{
    const QString remoteCall = QString::fromLocal8Bit("%1 %2 %3")
        .arg(targetCmdLinePrefix()).arg(executableOnTarget())
        .arg(runConfig->arguments().join(" "));

    QStringList cmdArgs;
    cmdArgs << "-n" << "-p" << port() << "-l" << runConfig->remoteUserName()
        << runConfig->remoteHostName() << remoteCall;
    sshProcess.start(runConfig->sshCmd(), cmdArgs);

    sshProcess.start(runConfig->sshCmd(), cmdArgs);
    emit addToOutputWindow(this, tr("Starting remote application."));
    if (sshProcess.waitForStarted()) {
        emit started();
    } else {
        emit error(this, tr("Could not start ssh!"));
        sshProcess.kill();
    }
}

void MaemoRunControl::executionFinished()
{
    if (stoppedByUser)
        emit addToOutputWindow(this, tr("Remote process stopped by user."));
    else if (sshProcess.exitCode() != 0)
        emit addToOutputWindow(this, tr("Remote process exited with error."));
    else
        emit addToOutputWindow(this, tr("Remote process finished successfully."));
    emit finished();
}

void MaemoRunControl::stop()
{
    stoppedByUser = true;
    if (isDeploying()) {
        stopDeployment();
    } else {
        stopProcess.kill();
        QStringList cmdArgs;
        const QString remoteCall = QString::fromLocal8Bit("pkill -x %1; "
            "sleep 1; pkill -x -9 %1").arg(executableFileName());
        cmdArgs << "-n" << "-p" << port() << "-l" << runConfig->remoteUserName()
            << runConfig->remoteHostName() << remoteCall;
        stopProcess.start(runConfig->sshCmd(), cmdArgs);
    }
}

bool MaemoRunControl::isRunning() const
{
    return isDeploying() || sshProcess.state() != QProcess::NotRunning;
}


// #pragma mark -- MaemoDebugRunControl


MaemoDebugRunControl::MaemoDebugRunControl(RunConfiguration *runConfiguration)
    : AbstractMaemoRunControl(runConfiguration)
    , gdbServerPort("10000"), debuggerManager(0)
    , startParams(new Debugger::DebuggerStartParameters)
{
    setProcessEnvironment(gdbServer);
    setProcessEnvironment(stopProcess);

    qDebug("Maemo Debug run controls started");
    debuggerManager = ExtensionSystem::PluginManager::instance()
        ->getObject<Debugger::DebuggerManager>();

    QTC_ASSERT(debuggerManager != 0, return);
    startParams->startMode = Debugger::StartRemote;
    startParams->executable = executableOnHost();
    startParams->remoteChannel = runConfig->remoteHostName() + ":"
        + gdbServerPort;
    startParams->remoteArchitecture = "arm";
    startParams->sysRoot = runConfig->sysRoot();
    startParams->toolChainType = ToolChain::GCC_MAEMO;
    startParams->debuggerCommand = runConfig->gdbCmd();
    startParams->dumperLibrary = runConfig->dumperLib();
    startParams->remoteDumperLib = QString::fromLocal8Bit("%1/%2")
        .arg(runConfig->remoteDir()).arg(QFileInfo(runConfig->dumperLib())
            .fileName());

    connect(this, SIGNAL(stopRequested()), debuggerManager, SLOT(exitDebugger()));
    connect(debuggerManager, SIGNAL(debuggingFinished()), this,
        SLOT(debuggingFinished()), Qt::QueuedConnection);
    connect(debuggerManager, SIGNAL(applicationOutputAvailable(QString)),
        this, SLOT(debuggerOutput(QString)), Qt::QueuedConnection);
}

MaemoDebugRunControl::~MaemoDebugRunControl()
{
    disconnect(SIGNAL(addToOutputWindow(RunControl*,QString)));
    disconnect(SIGNAL(addToOutputWindowInline(RunControl*,QString)));
    stop();
    debuggingFinished();
}

void MaemoDebugRunControl::start()
{
    startDeployment(true);
}

void MaemoDebugRunControl::deploymentFinished(bool success)
{
    if (success) {
        startGdbServer();
    } else {
        emit finished();
    }
}

void MaemoDebugRunControl::startGdbServer()
{
    const QString remoteCall(QString::fromLocal8Bit("%1 gdbserver :%2 %3 %4").
        arg(targetCmdLinePrefix()).arg(gdbServerPort). arg(executableOnTarget())
        .arg(runConfig->arguments().join(" ")));
    QStringList sshArgs;
    sshArgs << "-t" << "-n" << "-l" << runConfig->remoteUserName() << "-p"
        << port() << runConfig->remoteHostName() << remoteCall;
    inferiorPid = -1;
    disconnect(&gdbServer, SIGNAL(readyReadStandardError()), 0, 0);
    connect(&gdbServer, SIGNAL(readyReadStandardError()), this,
        SLOT(gdbServerStarted()));
    gdbServer.start(runConfig->sshCmd(), sshArgs);
    qDebug("Maemo: started gdb server, ssh arguments were %s", qPrintable(sshArgs.join(" ")));
}

void MaemoDebugRunControl::gdbServerStartFailed(const QString &reason)
{
    emit addToOutputWindow(this, tr("Debugging failed: %1").arg(reason));
    emit stopRequested();
    emit finished();
}

void MaemoDebugRunControl::gdbServerStarted()
{
    const QByteArray output = gdbServer.readAllStandardError();
    qDebug("gdbserver's stderr output: %s", output.data());

    const QByteArray searchString("pid = ");
    const int searchStringLength = searchString.length();

    int pidStartPos = output.indexOf(searchString);
    const int pidEndPos = output.indexOf("\n", pidStartPos + searchStringLength);
    if (pidStartPos == -1 || pidEndPos == -1) {
        gdbServerStartFailed(output.data());
        return;
    }

    pidStartPos += searchStringLength;
    QByteArray pidString = output.mid(pidStartPos, pidEndPos - pidStartPos);
    qDebug("pidString = %s", pidString.data());

    bool ok;
    const int pid = pidString.toInt(&ok);
    if (!ok) {
        gdbServerStartFailed(tr("Debugging failed, could not parse gdb "
            "server pid!"));
        return;
    }

    inferiorPid = pid;
    qDebug("inferiorPid = %d", inferiorPid);

    disconnect(&gdbServer, SIGNAL(readyReadStandardError()), 0, 0);
    connect(&gdbServer, SIGNAL(readyReadStandardError()), this,
        SLOT(readStandardError()));
    startDebugging();
}

void MaemoDebugRunControl::startDebugging()
{
    debuggerManager->startNewDebugger(startParams);
}

void MaemoDebugRunControl::stop()
{
    if (!isRunning())
        return;
    emit addToOutputWindow(this, tr("Stopping debugging operation ..."));
    if (isDeploying()) {
        stopDeployment();
    } else {
        emit stopRequested();
    }
}

bool MaemoDebugRunControl::isRunning() const
{
    return isDeploying() || gdbServer.state() != QProcess::NotRunning
        || debuggerManager->state() != Debugger::DebuggerNotReady;
}

void MaemoDebugRunControl::debuggingFinished()
{
    if (gdbServer.state() != QProcess::NotRunning) {
        stopProcess.kill();
        const QString remoteCall = QString::fromLocal8Bit("kill %1; sleep 1; "
            "kill -9 %1; pkill -x -9 gdbserver").arg(inferiorPid);
        QStringList sshArgs;
        sshArgs << "-n" << "-l" << runConfig->remoteUserName() << "-p" << port()
            << runConfig->remoteHostName() << remoteCall;
        stopProcess.start(runConfig->sshCmd(), sshArgs);
    }
    qDebug("ssh return code is %d", gdbServer.exitCode());
    emit addToOutputWindow(this, tr("Debugging finished."));
    emit finished();
}

void MaemoDebugRunControl::debuggerOutput(const QString &output)
{
    emit addToOutputWindowInline(this, output);
}

#include "maemorunconfiguration.moc"
