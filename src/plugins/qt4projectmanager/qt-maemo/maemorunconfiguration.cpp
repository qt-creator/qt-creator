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

#include "maemodeviceconfigurations.h"
#include "maemomanager.h"
#include "maemosettingspage.h"
#include "maemosshthread.h"
#include "maemotoolchain.h"
#include "profilereader.h"
#include "qt4project.h"
#include "qt4buildconfiguration.h"

#include <coreplugin/progressmanager/progressmanager.h>
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
#include <QtCore/QFuture>
#include <QtCore/QPair>
#include <QtCore/QProcess>
#include <QtCore/QScopedPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QStringBuilder>

#include <QtGui/QComboBox>
#include <QtGui/QCheckBox>
#include <QtGui/QDesktopServices>
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
    MaemoRunConfigurationWidget(MaemoRunConfiguration *runConfiguration,
                                QWidget *parent = 0);

private slots:
    void configNameEdited(const QString &text);
    void argumentsEdited(const QString &args);
    void deviceConfigurationChanged(const QString &name);
    void resetDeviceConfigurations();
    void showSettingsDialog();

    void updateSimulatorPath();
    void updateTargetInformation();

private:
    void setSimInfoVisible(const MaemoDeviceConfig &devConf);

    QLineEdit *m_configNameLineEdit;
    QLineEdit *m_argsLineEdit;
    QLabel *m_executableLabel;
    QLabel *m_debuggerLabel;
    QComboBox *m_devConfBox;
    QLabel *m_simPathNameLabel;
    QLabel *m_simPathValueLabel;
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
    void deploy();
    void stopDeployment();
    bool isDeploying() const;
    const QString executableOnHost() const;
    const QString executableOnTarget() const;
    const QString executableFileName() const;
    const QString port() const;
    const QString targetCmdLinePrefix() const;
    const QString remoteDir() const;
    const QStringList options() const;
// #ifndef USE_SSH_LIB
    void deploymentFinished(bool success);
    virtual bool setProcessEnvironment(QProcess &process);
// #endif // USE_SSH_LIB
private slots:
// #ifndef USE_SSH_LIB
    void readStandardError();
    void readStandardOutput();
//  #endif // USE_SSH_LIB
    void deployProcessFinished();
#ifdef USE_SSH_LIB
    void handleFileCopied();
#endif

protected:
    ErrorDumper dumper;
    MaemoRunConfiguration *runConfig; // TODO this pointer can be invalid
    const MaemoDeviceConfig devConfig;

private:
    virtual void handleDeploymentFinished(bool success)=0;

    QFutureInterface<void> m_progress;
#ifdef USE_SSH_LIB
    QScopedPointer<MaemoSshDeployer> sshDeployer;
#else
    QProcess deployProcess;
#endif // USE_SSH_LIB
    struct Deployable
    {
        typedef void (MaemoRunConfiguration::*updateFunc)();
        Deployable(const QString &f, const QString &d, updateFunc u)
            : fileName(f), dir(d), updateTimestamp(u) {}
        QString fileName;
        QString dir;
        updateFunc updateTimestamp;
    };
    QList<Deployable> deployables;
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
    virtual void handleDeploymentFinished(bool success);
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
    virtual void handleDeploymentFinished(bool success);

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


static const QLatin1String ArgumentsKey("Arguments");
static const QLatin1String SimulatorPathKey("Simulator");
static const QLatin1String DeviceIdKey("DeviceId");
static const QLatin1String LastDeployedKey("LastDeployed");
static const QLatin1String DebuggingHelpersLastDeployedKey(
    "DebuggingHelpersLastDeployed");

MaemoRunConfiguration::MaemoRunConfiguration(Project *project,
        const QString &proFilePath)
    : RunConfiguration(project)
    , m_proFilePath(proFilePath)
    , m_cachedTargetInformationValid(false)
    , m_cachedSimulatorInformationValid(false)
    , qemu(0)
{
    if (!m_proFilePath.isEmpty()) {
        setName(tr("%1 on Maemo device").arg(QFileInfo(m_proFilePath)
            .completeBaseName()));
    } else {
        setName(tr("MaemoRunConfiguration"));
    }

    connect(&MaemoDeviceConfigurations::instance(), SIGNAL(updated()),
            this, SLOT(updateDeviceConfigurations()));

    connect(project, SIGNAL(targetInformationChanged()),
            this, SLOT(invalidateCachedTargetInformation()));

    connect(project, SIGNAL(targetInformationChanged()),
            this, SLOT(enabledStateChanged()));

    connect(project, SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode*)),
            this, SLOT(proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode*)));

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

bool MaemoRunConfiguration::isEnabled(ProjectExplorer::BuildConfiguration *config) const
{
    Qt4BuildConfiguration *qt4bc = qobject_cast<Qt4BuildConfiguration*>(config);
    QTC_ASSERT(qt4bc, return false);
    ToolChain::ToolChainType type = qt4bc->toolChainType();
    return type == ToolChain::GCC_MAEMO;
}

QWidget *MaemoRunConfiguration::configurationWidget()
{
    return new MaemoRunConfigurationWidget(this);
}

void MaemoRunConfiguration::proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode *pro)
{
    if (m_proFilePath == pro->path())
        invalidateCachedTargetInformation();
}


void MaemoRunConfiguration::save(PersistentSettingsWriter &writer) const
{
    writer.saveValue(DeviceIdKey, m_devConfig.internalId);
    writer.saveValue(ArgumentsKey, m_arguments);

    writer.saveValue(LastDeployedKey, m_lastDeployed);
    writer.saveValue(DebuggingHelpersLastDeployedKey,
        m_debuggingHelpersLastDeployed);

    writer.saveValue(SimulatorPathKey, m_simulatorPath);

    const QDir &dir = QFileInfo(project()->file()->fileName()).absoluteDir();
    writer.saveValue("ProFile", dir.relativeFilePath(m_proFilePath));

    RunConfiguration::save(writer);
}

void MaemoRunConfiguration::restore(const PersistentSettingsReader &reader)
{
    RunConfiguration::restore(reader);

    setDeviceConfig(MaemoDeviceConfigurations::instance().
        find(reader.restoreValue(DeviceIdKey).toInt()));
    m_arguments = reader.restoreValue(ArgumentsKey).toStringList();

    m_lastDeployed = reader.restoreValue(LastDeployedKey).toDateTime();
    m_debuggingHelpersLastDeployed =
        reader.restoreValue(DebuggingHelpersLastDeployedKey).toDateTime();

    m_simulatorPath = reader.restoreValue(SimulatorPathKey).toString();

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
    Qt4BuildConfiguration *qt4bc = project()->activeQt4BuildConfiguration();
    return qt4bc->qtVersion()->hasDebuggingHelper();
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

void MaemoRunConfiguration::setDeviceConfig(const MaemoDeviceConfig &devConf)
{
    m_devConfig = devConf;
}

MaemoDeviceConfig MaemoRunConfiguration::deviceConfig() const
{
    return m_devConfig;
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
    Qt4BuildConfiguration *qt4bc = qobject_cast<Qt4BuildConfiguration *>
        (project()->activeBuildConfiguration());
    QTC_ASSERT(qt4bc, return 0);
    MaemoToolChain *tc = dynamic_cast<MaemoToolChain *>(
        qt4bc->toolChain() );
    QTC_ASSERT(tc != 0, return 0);
    return tc;
}

const QString MaemoRunConfiguration::gdbCmd() const
{
    if (const MaemoToolChain *tc = toolchain())
        return tc->targetRoot() + "/bin/gdb";
    return QString();
}

QString MaemoRunConfiguration::maddeRoot() const
{
    if (const MaemoToolChain *tc = toolchain())
        return tc->maddeRoot();
    return QString();
}

const QString MaemoRunConfiguration::sysRoot() const
{
    if (const MaemoToolChain *tc = toolchain())
        return tc->sysrootRoot();
    return QString();
}

const QStringList MaemoRunConfiguration::arguments() const
{
    return m_arguments;
}

const QString MaemoRunConfiguration::dumperLib() const
{
    Qt4BuildConfiguration *qt4bc = project()->activeQt4BuildConfiguration();
    return qt4bc->qtVersion()->debuggingHelperLibrary();
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
    m_arguments = args;
}


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

    Qt4TargetInformation info = project()->targetInformation(project()->activeQt4BuildConfiguration(),
                                                             m_proFilePath);
    if (info.error != Qt4TargetInformation::NoError) {
        if (info.error == Qt4TargetInformation::ProParserError) {
            Core::ICore::instance()->messageManager()->printToOutputPane(tr(
                "Could not parse %1. The Maemo run configuration %2 "
                "can not be started.").arg(m_proFilePath).arg(name()));
        }
        emit targetInformationChanged();
        return;
    }

    QString baseDir;
    if (info.hasCustomDestDir)
        baseDir = info.workingDir;
    else
        baseDir = info.baseDestDir;
    m_executable = QDir::cleanPath(baseDir + QLatin1Char('/') + info.target);

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

    if (!m_isUserSetSimulator) {
        if (const MaemoToolChain *tc = toolchain())
            m_simulatorPath = QDir::toNativeSeparators(tc->simulatorRoot());
    }

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

void MaemoRunConfiguration::updateDeviceConfigurations()
{
    qDebug("%s: Current devid = %llu", Q_FUNC_INFO, m_devConfig.internalId);
    m_devConfig =
        MaemoDeviceConfigurations::instance().find(m_devConfig.internalId);
    qDebug("%s: new devid = %llu", Q_FUNC_INFO, m_devConfig.internalId);
    emit deviceConfigurationsUpdated();
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
    QWidget *devConfWidget = new QWidget;
    QHBoxLayout *devConfLayout = new QHBoxLayout(devConfWidget);
    m_devConfBox = new QComboBox;
    m_devConfBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    devConfLayout->addWidget(m_devConfBox);
    QLabel *addDevConfLabel
        = new QLabel(tr("<a href=\"#\">Manage device configurations</a>"));
    devConfLayout->addWidget(addDevConfLabel);
    mainLayout->addRow(new QLabel(tr("Device Configuration:")), devConfWidget);
    m_executableLabel = new QLabel(m_runConfiguration->executable());
    mainLayout->addRow(tr("Executable:"), m_executableLabel);
    m_argsLineEdit = new QLineEdit(m_runConfiguration->arguments().join(" "));
    mainLayout->addRow(tr("Arguments:"), m_argsLineEdit);
    m_debuggerLabel = new QLabel(m_runConfiguration->gdbCmd());
    mainLayout->addRow(tr("Debugger:"), m_debuggerLabel);
    mainLayout->addItem(new QSpacerItem(10, 10));
    m_simPathNameLabel = new QLabel(tr("Simulator Path:"));
    m_simPathValueLabel = new QLabel(m_runConfiguration->simulatorPath());
    mainLayout->addRow(m_simPathNameLabel, m_simPathValueLabel);
    resetDeviceConfigurations();

    connect(m_runConfiguration, SIGNAL(cachedSimulatorInformationChanged()),
        this, SLOT(updateSimulatorPath()));
    connect(m_runConfiguration, SIGNAL(deviceConfigurationsUpdated()),
            this, SLOT(resetDeviceConfigurations()));

    connect(m_configNameLineEdit, SIGNAL(textEdited(QString)), this,
        SLOT(configNameEdited(QString)));
    connect(m_argsLineEdit, SIGNAL(textEdited(QString)), this,
        SLOT(argumentsEdited(QString)));
    connect(m_devConfBox, SIGNAL(activated(QString)), this,
            SLOT(deviceConfigurationChanged(QString)));
    connect(m_runConfiguration, SIGNAL(targetInformationChanged()), this,
        SLOT(updateTargetInformation()));
    connect(addDevConfLabel, SIGNAL(linkActivated(QString)), this,
        SLOT(showSettingsDialog()));
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
    m_simPathValueLabel->setText(m_runConfiguration->simulatorPath());
}

void MaemoRunConfigurationWidget::deviceConfigurationChanged(const QString &name)
{
    const MaemoDeviceConfig &devConfig
        = MaemoDeviceConfigurations::instance().find(name);
    setSimInfoVisible(devConfig);
    m_runConfiguration->setDeviceConfig(devConfig);
}

void MaemoRunConfigurationWidget::setSimInfoVisible(const MaemoDeviceConfig &devConf)
{
    const bool isSimulator = devConf.type == MaemoDeviceConfig::Simulator;
    m_simPathNameLabel->setVisible(isSimulator);
    m_simPathValueLabel->setVisible(isSimulator);
}

void MaemoRunConfigurationWidget::resetDeviceConfigurations()
{
    m_devConfBox->clear();
    const QList<MaemoDeviceConfig> &devConfs =
        MaemoDeviceConfigurations::instance().devConfigs();
    foreach (const MaemoDeviceConfig &devConf, devConfs)
        m_devConfBox->addItem(devConf.name);
    m_devConfBox->addItem(MaemoDeviceConfig().name);
    const MaemoDeviceConfig &devConf = m_runConfiguration->deviceConfig();
    m_devConfBox->setCurrentIndex(m_devConfBox->findText(devConf.name));
    setSimInfoVisible(devConf);
}

void MaemoRunConfigurationWidget::showSettingsDialog()
{
    MaemoSettingsPage *settingsPage = MaemoManager::instance()->settingsPage();
    Core::ICore::instance()->showOptionsDialog(settingsPage->category(),
                                               settingsPage->id());
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
    , devConfig(runConfig ? runConfig->deviceConfig() : MaemoDeviceConfig())
{
#ifndef USE_SSH_LIB
    setProcessEnvironment(deployProcess);
    connect(&deployProcess, SIGNAL(readyReadStandardError()), this,
        SLOT(readStandardError()));
    connect(&deployProcess, SIGNAL(readyReadStandardOutput()), this,
        SLOT(readStandardOutput()));
    connect(&deployProcess, SIGNAL(error(QProcess::ProcessError)), &dumper,
        SLOT(printToStream(QProcess::ProcessError)));
    connect(&deployProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this,
        SLOT(deployProcessFinished()));
#endif // USE_SSH_LIB
}

void AbstractMaemoRunControl::startDeployment(bool forDebugging)
{
    QTC_ASSERT(runConfig, return);

#ifndef USE_SSH_LIB
    Core::ICore::instance()->progressManager()->addTask(m_progress.future(),
        tr("Deploying"), QLatin1String("Maemo.Deploy"));
#endif // USE_SSH_LIB
    if (devConfig.isValid()) {
        deployables.clear();
        if (runConfig->currentlyNeedsDeployment()) {
            deployables.append(Deployable(executableFileName(),
                QFileInfo(executableOnHost()).canonicalPath(),
                &MaemoRunConfiguration::wasDeployed));
        }
        if (forDebugging && runConfig->debuggingHelpersNeedDeployment()) {
            const QFileInfo &info(runConfig->dumperLib());
            deployables.append(Deployable(info.fileName(), info.canonicalPath(),
                &MaemoRunConfiguration::debuggingHelpersDeployed));
        }

#ifndef USE_SSH_LIB
        m_progress.setProgressRange(0, deployables.count());
        m_progress.setProgressValue(0);
        m_progress.reportStarted();
#endif // USE_SSH_LIB
        deploy();
    } else {
        deploymentFinished(false);
    }
}

void AbstractMaemoRunControl::deploy()
{
#ifdef USE_SSH_LIB
    if (!deployables.isEmpty()) {
        QStringList files;
        QStringList targetDirs;
        foreach (const Deployable &deployable, deployables) {
            files << deployable.dir + QDir::separator() + deployable.fileName;
            targetDirs << remoteDir();
        }
        emit addToOutputWindow(this, tr("Files to deploy: %1.").arg(files.join(" ")));
        sshDeployer.reset(new MaemoSshDeployer(devConfig, files, targetDirs));
        connect(sshDeployer.data(), SIGNAL(finished()),
                this, SLOT(deployProcessFinished()));
        connect(sshDeployer.data(), SIGNAL(fileCopied(QString)),
                this, SLOT(handleFileCopied()));
        Core::ICore::instance()->progressManager()
                ->addTask(m_progress.future(), tr("Deploying"),
                          QLatin1String("Maemo.Deploy"));
        m_progress.setProgressRange(0, deployables.count());
        m_progress.setProgressValue(0);
        m_progress.reportStarted();
        emit started();
        sshDeployer->start();
    } else {
        handleDeploymentFinished(true);
    }
#else
    if (!deployables.isEmpty()) {
        const Deployable &deployable = deployables.first();
        emit addToOutputWindow(this, tr("File to deploy: %1.").arg(deployable.fileName));

        QStringList cmdArgs;
        cmdArgs << "-P" << port() << options() << deployable.fileName
            << (devConfig.uname + "@" + devConfig.host + ":" + remoteDir());
        deployProcess.setWorkingDirectory(deployable.dir);

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
#endif // USE_SSH_LIB
}

#ifdef USE_SSH_LIB
void AbstractMaemoRunControl::handleFileCopied()
{
    Deployable deployable = deployables.takeFirst();
    (runConfig->*deployable.updateTimestamp)();
    m_progress.setProgressValue(m_progress.progressValue() + 1);
}
#endif // USE_SSH_LIB

void AbstractMaemoRunControl::stopDeployment()
{
#ifdef USE_SSH_LIB
    sshDeployer->stop();
#else
    deployProcess.kill();
#endif // USE_SSH_LIB
}

bool AbstractMaemoRunControl::isDeploying() const
{
#ifdef USE_SSH_LIB
    return !sshDeployer.isNull() && sshDeployer->isRunning();
#else
    return deployProcess.state() != QProcess::NotRunning;
#endif // USE_SSH_LIB
}

void AbstractMaemoRunControl::deployProcessFinished()
{
#if USE_SSH_LIB
    const bool success = !sshDeployer->hasError();
    if (success) {
        emit addToOutputWindow(this, tr("Deployment finished."));
    } else {
        emit error(this, tr("Deployment failed: ") % sshDeployer->error());
        m_progress.reportCanceled();
    }
    m_progress.reportFinished();
    handleDeploymentFinished(success);
#else
    bool success;
    if (deployProcess.exitCode() == 0) {
        emit addToOutputWindow(this, tr("Target deployed."));
        success = true;
        Deployable deployable = deployables.takeFirst();
        (runConfig->*deployable.updateTimestamp)();
        m_progress.setProgressValue(m_progress.progressValue() + 1);
    } else {
        emit error(this, tr("Deployment failed."));
        success = false;
    }
    if (deployables.isEmpty() || !success)
        deploymentFinished(success);
    else
        deploy();
#endif // USE_SSH_LIB
}

#ifndef USE_SSH_LIB
void AbstractMaemoRunControl::deploymentFinished(bool success)
{
    m_progress.reportFinished();
    handleDeploymentFinished(success);
}
#endif // USE_SSH_LIB

const QString AbstractMaemoRunControl::executableOnHost() const
{
    return runConfig->executable();
}

const QString AbstractMaemoRunControl::port() const
{
    return QString::number(devConfig.port);
}

const QString AbstractMaemoRunControl::executableFileName() const
{
    return QFileInfo(executableOnHost()).fileName();
}

const QString AbstractMaemoRunControl::remoteDir() const
{
    return devConfig.uname == QString::fromLocal8Bit("root")
        ? QString::fromLocal8Bit("/root")
        : QString::fromLocal8Bit("/home/") + devConfig.uname;
}

const QStringList AbstractMaemoRunControl::options() const
{
    const bool usePassword
        = devConfig.authentication == MaemoDeviceConfig::Password;
    const QLatin1String opt("-o");
    QStringList optionList;
    if (!usePassword)
        optionList << QLatin1String("-i") << devConfig.keyFile;
    return optionList << opt
        << QString::fromLatin1("PasswordAuthentication=%1").
            arg(usePassword ? "yes" : "no") << opt
        << QString::fromLatin1("PubkeyAuthentication=%1").
            arg(usePassword ? "no" : "yes") << opt
        << QString::fromLatin1("ConnectTimeout=%1").arg(devConfig.timeout)
        << opt << QLatin1String("CheckHostIP=no")
        << opt << QLatin1String("StrictHostKeyChecking=no");
}

const QString AbstractMaemoRunControl::executableOnTarget() const
{
    return QString::fromLocal8Bit("%1/%2").arg(remoteDir()).
        arg(executableFileName());
}

const QString AbstractMaemoRunControl::targetCmdLinePrefix() const
{
    return QString::fromLocal8Bit("chmod u+x %1; source /etc/profile; ").
        arg(executableOnTarget());
}

// #ifndef USE_SSH_LIB
bool AbstractMaemoRunControl::setProcessEnvironment(QProcess &process)
{
    QTC_ASSERT(runConfig, return false);
    Qt4BuildConfiguration *qt4bc = qobject_cast<Qt4BuildConfiguration*>
        (runConfig->project()->activeBuildConfiguration());
    QTC_ASSERT(qt4bc, return false);
    Environment env = Environment::systemEnvironment();
    qt4bc->toolChain()->addToEnvironment(env);
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
// #endif // USE_SSH_LIB

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

void MaemoRunControl::handleDeploymentFinished(bool success)
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
    cmdArgs << "-n" << "-p" << port() << "-l" << devConfig.uname
        << options() << devConfig.host << remoteCall;
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
    if (!isRunning())
        return;

    stoppedByUser = true;
    if (isDeploying()) {
        stopDeployment();
    } else {
        stopProcess.kill();
        QStringList cmdArgs;
        const QString remoteCall = QString::fromLocal8Bit("pkill -x %1; "
            "sleep 1; pkill -x -9 %1").arg(executableFileName());
        cmdArgs << "-n" << "-p" << port() << "-l" << devConfig.uname
            << options() << devConfig.host << remoteCall;
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
    startParams->remoteChannel = devConfig.host + ":" + gdbServerPort;
    startParams->remoteArchitecture = "arm";
    startParams->sysRoot = runConfig->sysRoot();
    startParams->toolChainType = ToolChain::GCC_MAEMO;
    startParams->debuggerCommand = runConfig->gdbCmd();
    startParams->dumperLibrary = runConfig->dumperLib();
    startParams->remoteDumperLib = QString::fromLocal8Bit("%1/%2")
        .arg(remoteDir()).arg(QFileInfo(runConfig->dumperLib()).fileName());

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

void MaemoDebugRunControl::handleDeploymentFinished(bool success)
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
    sshArgs << "-t" << "-n" << "-l" << devConfig.uname << "-p"
        << port() << options() << devConfig.host << remoteCall;
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
        sshArgs << "-n" << "-l" << devConfig.uname << "-p" << port()
            << options() << devConfig.host << remoteCall;
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
