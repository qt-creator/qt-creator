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
#include "maemorunconfigurationwidget.h"
#include "maemoruncontrol.h"
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
#include <QtCore/QProcess>

namespace {
const char * const MAEMO_RC_ID("Qt4ProjectManager.MaemoRunConfiguration");
const char * const MAEMO_RC_ID_PREFIX("Qt4ProjectManager.MaemoRunConfiguration.");

bool hasMaemoRunConfig(ProjectExplorer::Project* project)
{
    Qt4ProjectManager::Qt4Project *qt4Project(
        qobject_cast<Qt4ProjectManager::Qt4Project *>(project));
    if (qt4Project) {
        QList<ProjectExplorer::RunConfiguration *> list =
                qt4Project->runConfigurations();
        foreach (ProjectExplorer::RunConfiguration *rc, list) {
            if (qobject_cast<Qt4ProjectManager::Internal::MaemoRunConfiguration *>(rc))
                return true;
        }
    }
    return false;
}

QString targetFromId(const QString &id)
{
    QString prefix(MAEMO_RC_ID_PREFIX);
    if (!id.startsWith(prefix))
        return QString();
    return id.mid(prefix.size());
}

QString targetToId(const QString &target)
{
    return QString::fromLatin1(MAEMO_RC_ID_PREFIX) + target;
}

} // namespace

static const QLatin1String ArgumentsKey("Arguments");
static const QLatin1String SimulatorPathKey("Simulator");
static const QLatin1String DeviceIdKey("DeviceId");
static const QLatin1String LastDeployedKey("LastDeployed");
static const QLatin1String DebuggingHelpersLastDeployedKey(
    "DebuggingHelpersLastDeployed");
static const QLatin1String ProFileKey("ProFile");

namespace Qt4ProjectManager {
namespace Internal {

using namespace ProjectExplorer;

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

MaemoRunConfiguration::MaemoRunConfiguration(Qt4Project *project,
        const QString &proFilePath)
    : RunConfiguration(project, QLatin1String(MAEMO_RC_ID))
    , m_proFilePath(proFilePath)
    , m_cachedTargetInformationValid(false)
    , m_cachedSimulatorInformationValid(false)
    , qemu(0)
{
    ctor();
}

MaemoRunConfiguration::MaemoRunConfiguration(Qt4Project *project,
                                             MaemoRunConfiguration *source)
    : RunConfiguration(project, source)
    , m_executable(source->m_executable)
    , m_proFilePath(source->m_proFilePath)
    , m_cachedTargetInformationValid(false)
    , m_simulator(source->m_simulator)
    , m_simulatorArgs(source->m_simulatorArgs)
    , m_simulatorPath(source->m_simulatorPath)
    , m_visibleSimulatorParameter(source->m_visibleSimulatorParameter)
    , m_cachedSimulatorInformationValid(false)
    , m_isUserSetSimulator(source->m_isUserSetSimulator)
    , m_userSimulatorPath(source->m_userSimulatorPath)
    , m_gdbPath(source->m_gdbPath)
    , m_devConfig(source->m_devConfig)
    , m_arguments(source->m_arguments)
    , m_lastDeployed(source->m_lastDeployed)
    , m_debuggingHelpersLastDeployed(source->m_debuggingHelpersLastDeployed)
    , qemu(0)
#if USE_SSL_PASSWORD
    , m_remoteUserPassword(source->m_remoteUserPassword)
    , m_remoteHostRequiresPassword(source->m_remoteHostRequiresPassword)
#endif
{
    ctor();
}

void MaemoRunConfiguration::ctor()
{
    if (!m_proFilePath.isEmpty()) {
        setDisplayName(tr("%1 on Maemo device").arg(QFileInfo(m_proFilePath)
            .completeBaseName()));
    } else {
        setDisplayName(tr("MaemoRunConfiguration"));
    }

    connect(&MaemoDeviceConfigurations::instance(), SIGNAL(updated()),
            this, SLOT(updateDeviceConfigurations()));

    connect(qt4Project(), SIGNAL(targetInformationChanged()),
            this, SLOT(invalidateCachedTargetInformation()));

    connect(qt4Project(), SIGNAL(targetInformationChanged()),
            this, SLOT(enabledStateChanged()));

    connect(qt4Project(), SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode*)),
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

Qt4Project *MaemoRunConfiguration::qt4Project() const
{
    return static_cast<Qt4Project *>(project());
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

QVariantMap MaemoRunConfiguration::toMap() const
{
    QVariantMap map(RunConfiguration::toMap());
    map.insert(DeviceIdKey, m_devConfig.internalId);
    map.insert(ArgumentsKey, m_arguments);

    map.insert(LastDeployedKey, m_lastDeployed);
    map.insert(DebuggingHelpersLastDeployedKey,
        m_debuggingHelpersLastDeployed);

    map.insert(SimulatorPathKey, m_simulatorPath);

    const QDir &dir = QFileInfo(qt4Project()->file()->fileName()).absoluteDir();
    map.insert(ProFileKey, dir.relativeFilePath(m_proFilePath));

    return map;
}

bool MaemoRunConfiguration::fromMap(const QVariantMap &map)
{
    setDeviceConfig(MaemoDeviceConfigurations::instance().
        find(map.value(DeviceIdKey, 0).toInt()));
    m_arguments = map.value(ArgumentsKey).toStringList();

    m_lastDeployed = map.value(LastDeployedKey).toDateTime();
    m_debuggingHelpersLastDeployed =
        map.value(DebuggingHelpersLastDeployedKey).toDateTime();

    m_simulatorPath = map.value(SimulatorPathKey).toString();

    const QDir &dir = QFileInfo(qt4Project()->file()->fileName()).absoluteDir();
    m_proFilePath = dir.filePath(map.value(ProFileKey).toString());

    return RunConfiguration::fromMap(map);
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
    Qt4BuildConfiguration *qt4bc = qt4Project()->activeQt4BuildConfiguration();
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
        (qt4Project()->activeBuildConfiguration());
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
    Qt4BuildConfiguration *qt4bc = qt4Project()->activeQt4BuildConfiguration();
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

    Qt4TargetInformation info = qt4Project()->targetInformation(qt4Project()->activeQt4BuildConfiguration(),
                                                             m_proFilePath);
    if (info.error != Qt4TargetInformation::NoError) {
        if (info.error == Qt4TargetInformation::ProParserError) {
            Core::ICore::instance()->messageManager()->printToOutputPane(tr(
                "Could not parse %1. The Maemo run configuration %2 "
                "can not be started.").arg(m_proFilePath).arg(displayName()));
        }
        emit targetInformationChanged();
        return;
    }

    m_executable = QDir::cleanPath(info.workingDir + QLatin1Char('/') + info.target);

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

// #pragma mark -- MaemoRunConfigurationFactory

MaemoRunConfigurationFactory::MaemoRunConfigurationFactory(QObject *parent) :
    IRunConfigurationFactory(parent)
{
    ProjectExplorerPlugin *explorer = ProjectExplorerPlugin::instance();
    connect(explorer->session(), SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(projectAdded(ProjectExplorer::Project*)));
    connect(explorer->session(), SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            this, SLOT(projectRemoved(ProjectExplorer::Project*)));
    connect(explorer, SIGNAL(currentProjectChanged(ProjectExplorer::Project*)),
            this, SLOT(currentProjectChanged(ProjectExplorer::Project*)));
}

MaemoRunConfigurationFactory::~MaemoRunConfigurationFactory()
{
}

bool MaemoRunConfigurationFactory::canCreate(Project *parent,
                                             const QString &id) const
{
    if (!qobject_cast<Qt4Project *>(parent))
        return false;
    return id.startsWith(QLatin1String(MAEMO_RC_ID));
}

bool MaemoRunConfigurationFactory::canRestore(Project *parent,
                                              const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, id);
}

bool MaemoRunConfigurationFactory::canClone(Project *parent,
                                            RunConfiguration *source) const
{
    QString id(source->id());
    return canCreate(parent, id);
}

QStringList MaemoRunConfigurationFactory::availableCreationIds(Project *pro) const
{
    Qt4Project *qt4Project(qobject_cast<Qt4Project *>(pro));
    if (!qt4Project)
        return QStringList();

    return qt4Project->applicationProFilePathes(QLatin1String(MAEMO_RC_ID_PREFIX));
}

QString MaemoRunConfigurationFactory::displayNameForId(const QString &id) const
{
    QString target(targetFromId(id));
    if (target.isEmpty())
        return QString();
    return tr("%1 on Maemo Device").arg(QFileInfo(target).completeBaseName());
}

RunConfiguration *MaemoRunConfigurationFactory::create(Project *parent,
                                                       const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    Qt4Project *pqt4parent(static_cast<Qt4Project *>(parent));
    MaemoRunConfiguration *rc(new MaemoRunConfiguration(pqt4parent, targetFromId(id)));
    setupRunConfiguration(rc);
    return rc;

}

RunConfiguration *MaemoRunConfigurationFactory::restore(Project *parent,
                                                        const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Qt4Project *qt4Project(qobject_cast<Qt4Project *>(parent));
    Q_ASSERT(qt4Project);

    MaemoRunConfiguration *rc(new MaemoRunConfiguration(qt4Project, QString()));
    if (!rc->fromMap(map)) {
        delete rc;
        return 0;
    }

    setupRunConfiguration(rc);
    return rc;
}

RunConfiguration *MaemoRunConfigurationFactory::clone(Project *parent,
                                                      RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    Qt4Project *qt4Project(static_cast<Qt4Project *>(parent));
    MaemoRunConfiguration *old(static_cast<MaemoRunConfiguration *>(source));
    MaemoRunConfiguration *rc(new MaemoRunConfiguration(qt4Project, old));

    setupRunConfiguration(rc);

    return rc;
}

void MaemoRunConfigurationFactory::setupRunConfiguration(MaemoRunConfiguration *rc)
{
    if (!rc)
        return;

    connect(rc->project(), SIGNAL(runConfigurationsEnabledStateChanged()),
            rc, SLOT(enabledStateChanged()));
    connect(MaemoManager::instance(), SIGNAL(startStopQemu()),
            rc, SLOT(startStopQemu()));
    connect(rc, SIGNAL(qemuProcessStatus(bool)),
            MaemoManager::instance(), SLOT(updateQemuSimulatorStarter(bool)));
}

void MaemoRunConfigurationFactory::addedRunConfiguration(ProjectExplorer::RunConfiguration *rc)
{
    if (hasMaemoRunConfig(rc->project()))
        MaemoManager::instance()->addQemuSimulatorStarter(rc->project());
}

void MaemoRunConfigurationFactory::removedRunConfiguration(ProjectExplorer::RunConfiguration *rc)
{
    if (!hasMaemoRunConfig(rc->project()))
        MaemoManager::instance()->removeQemuSimulatorStarter(rc->project());
}

void MaemoRunConfigurationFactory::projectAdded(
    ProjectExplorer::Project *project)
{
    connect(project, SIGNAL(addedRunConfiguration(ProjectExplorer::RunConfiguration*)),
            this, SLOT(addedRunConfiguration(ProjectExplorer::RunConfiguration*)));
    connect(project, SIGNAL(removedRunConfiguration(ProjectExplorer::RunConfiguration*)),
            this, SLOT(removedRunConfiguration(ProjectExplorer::RunConfiguration*)));

    if (hasMaemoRunConfig(project))
        MaemoManager::instance()->addQemuSimulatorStarter(project);
}

void MaemoRunConfigurationFactory::projectRemoved(
    ProjectExplorer::Project *project)
{
    disconnect(project, SIGNAL(addedRunConfiguration(ProjectExplorer::RunConfiguration*)),
               this, SLOT(addedRunConfiguration(ProjectExplorer::RunConfiguration*)));
    disconnect(project, SIGNAL(removedRunConfiguration(ProjectExplorer::RunConfiguration*)),
               this, SLOT(removedRunConfiguration(ProjectExplorer::RunConfiguration*)));

    if (hasMaemoRunConfig(project))
        MaemoManager::instance()->removeQemuSimulatorStarter(project);
}

void MaemoRunConfigurationFactory::currentProjectChanged(
    ProjectExplorer::Project *project)
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

} // namespace Internal
} // namespace Qt4ProjectManager
