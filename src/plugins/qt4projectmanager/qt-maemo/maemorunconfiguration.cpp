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
#include "maemoruncontrol.h"
#include "maemosettingspage.h"
#include "maemosshthread.h"
#include "maemotoolchain.h"
#include "profilereader.h"
#include "qt4project.h"
#include "qt4buildconfiguration.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <projectexplorer/persistentsettings.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>

#include <QtCore/QDebug>
#include <QtCore/QPair>
#include <QtCore/QProcess>
#include <QtCore/QSharedPointer>

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

#include "maemorunconfiguration.moc"
