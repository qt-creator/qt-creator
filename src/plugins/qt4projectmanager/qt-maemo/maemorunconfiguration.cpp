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
#include "maemorunconfiguration.h"

#include "maemoconstants.h"
#include "maemomanager.h"
#include "maemorunconfigurationwidget.h"
#include "maemotoolchain.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <qt4projectmanager/qt4project.h>

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtCore/QStringBuilder>

namespace Qt4ProjectManager {
    namespace Internal {

using namespace ProjectExplorer;

MaemoRunConfiguration::MaemoRunConfiguration(Qt4Target *parent,
        const QString &proFilePath)
    : RunConfiguration(parent, QLatin1String(MAEMO_RC_ID))
    , m_proFilePath(proFilePath)
    , m_cachedSimulatorInformationValid(false)
    , qemu(0)
{
    init();
}

MaemoRunConfiguration::MaemoRunConfiguration(Qt4Target *parent,
        MaemoRunConfiguration *source)
    : RunConfiguration(parent, source)
    , m_proFilePath(source->m_proFilePath)
    , m_simulator(source->m_simulator)
    , m_simulatorArgs(source->m_simulatorArgs)
    , m_simulatorPath(source->m_simulatorPath)
    , m_visibleSimulatorParameter(source->m_visibleSimulatorParameter)
    , m_simulatorLibPath(source->m_simulatorLibPath)
    , m_simulatorSshPort(source->m_simulatorSshPort)
    , m_simulatorGdbServerPort(source->m_simulatorGdbServerPort)
    , m_cachedSimulatorInformationValid(false)
    , m_gdbPath(source->m_gdbPath)
    , m_devConfig(source->m_devConfig)
    , m_arguments(source->m_arguments)
    , m_lastDeployed(source->m_lastDeployed)
    , m_debuggingHelpersLastDeployed(source->m_debuggingHelpersLastDeployed)
    , qemu(0)
{
    init();
}

void MaemoRunConfiguration::init()
{
    if (!m_proFilePath.isEmpty()) {
        setDisplayName(tr("%1 on Maemo device").arg(QFileInfo(m_proFilePath)
            .completeBaseName()));
    } else {
        setDisplayName(tr("MaemoRunConfiguration"));
    }

    updateDeviceConfigurations();
    connect(&MaemoDeviceConfigurations::instance(), SIGNAL(updated()), this,
        SLOT(updateDeviceConfigurations()));

    connect(qt4Target()->qt4Project(), SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode*)),
            this, SLOT(proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode*)));

    connect(qt4Target()->qt4Project(),
        SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode*)),
        this, SLOT(proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode*)));

    qemu = new QProcess(this);
    connect(qemu, SIGNAL(finished(int, QProcess::ExitStatus)), this,
        SLOT(qemuProcessFinished()));

    connect(&MaemoManager::instance(), SIGNAL(startStopQemu()), this,
        SLOT(startStopQemu()));
    connect(this, SIGNAL(qemuProcessStatus(bool)), &MaemoManager::instance(),
        SLOT(updateQemuSimulatorStarter(bool)));
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

Qt4Target *MaemoRunConfiguration::qt4Target() const
{
    return static_cast<Qt4Target *>(target());
}

Qt4BuildConfiguration *MaemoRunConfiguration::activeQt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(activeBuildConfiguration());
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
        emit targetInformationChanged();
}

QVariantMap MaemoRunConfiguration::toMap() const
{
    QVariantMap map(RunConfiguration::toMap());
    map.insert(DeviceIdKey, m_devConfig.internalId);
    map.insert(ArgumentsKey, m_arguments);

    addDeployTimesToMap(LastDeployedKey, m_lastDeployed, map);
    addDeployTimesToMap(DebuggingHelpersLastDeployedKey,
                        m_debuggingHelpersLastDeployed, map);

    map.insert(SimulatorPathKey, m_simulatorPath);

    const QDir dir = QDir(target()->project()->projectDirectory());
    map.insert(ProFileKey, dir.relativeFilePath(m_proFilePath));

    return map;
}

void MaemoRunConfiguration::addDeployTimesToMap(const QString &key,
    const QMap<QString, QDateTime> &deployTimes, QVariantMap &map) const
{
    QMap<QString, QVariant> variantMap;
    QMap<QString, QDateTime>::ConstIterator it = deployTimes.begin();
    for (; it != deployTimes.end(); ++it)
        variantMap.insert(it.key(), it.value());
    map.insert(key, variantMap);
}

bool MaemoRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    setDeviceConfig(MaemoDeviceConfigurations::instance().
        find(map.value(DeviceIdKey, 0).toInt()));
    m_arguments = map.value(ArgumentsKey).toStringList();

    getDeployTimesFromMap(LastDeployedKey, m_lastDeployed, map);
    getDeployTimesFromMap(DebuggingHelpersLastDeployedKey,
                          m_debuggingHelpersLastDeployed, map);

    m_simulatorPath = map.value(SimulatorPathKey).toString();

    const QDir dir = QDir(target()->project()->projectDirectory());
    m_proFilePath = dir.filePath(map.value(ProFileKey).toString());

    return true;
}

void MaemoRunConfiguration::getDeployTimesFromMap(const QString &key,
    QMap<QString, QDateTime> &deployTimes, const QVariantMap &map)
{
    const QVariantMap &variantMap = map.value(key).toMap();
    for (QVariantMap::ConstIterator it = variantMap.begin();
         it != variantMap.end(); ++it)
        deployTimes.insert(it.key(), it.value().toDateTime());
}

bool MaemoRunConfiguration::currentlyNeedsDeployment(const QString &host) const
{
    return fileNeedsDeployment(executable(), m_lastDeployed.value(host));
}

void MaemoRunConfiguration::wasDeployed(const QString &host)
{
    m_lastDeployed.insert(host, QDateTime::currentDateTime());
}

bool MaemoRunConfiguration::hasDebuggingHelpers() const
{
    Qt4BuildConfiguration *qt4bc = activeQt4BuildConfiguration();
    return qt4bc->qtVersion()->hasDebuggingHelper();
}

bool MaemoRunConfiguration::debuggingHelpersNeedDeployment(const QString &host) const
{
    if (hasDebuggingHelpers()) {
        return fileNeedsDeployment(dumperLib(),
                   m_debuggingHelpersLastDeployed.value(host));
    }
    return false;
}

void MaemoRunConfiguration::debuggingHelpersDeployed(const QString &host)
{
    m_debuggingHelpersLastDeployed.insert(host, QDateTime::currentDateTime());
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
    emit deviceConfigurationChanged(target());
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
    Qt4BuildConfiguration *qt4bc(activeQt4BuildConfiguration());
    QTC_ASSERT(qt4bc, return 0);
    MaemoToolChain *tc = dynamic_cast<MaemoToolChain *>(qt4bc->toolChain());
    QTC_ASSERT(tc != 0, return 0);
    return tc;
}

const QString MaemoRunConfiguration::gdbCmd() const
{
    QSettings *settings = Core::ICore::instance()->settings();
    bool useMadde = settings->value(QLatin1String("MaemoDebugger/useMaddeGdb"),
        true).toBool();

    QString gdbPath;
    if (const MaemoToolChain *tc = toolchain())
        gdbPath = tc->targetRoot() + QLatin1String("/bin/gdb");

    if (!useMadde) {
        gdbPath = settings->value(QLatin1String("MaemoDebugger/gdb"),
            gdbPath).toString();
    }
    return QDir::toNativeSeparators(gdbPath);
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

const QString MaemoRunConfiguration::targetRoot() const
{
    if (const MaemoToolChain *tc = toolchain())
        return tc->targetRoot();
    return QString();
}

const QStringList MaemoRunConfiguration::arguments() const
{
    return m_arguments;
}

const QString MaemoRunConfiguration::dumperLib() const
{
    Qt4BuildConfiguration *qt4bc(activeQt4BuildConfiguration());
    return qt4bc->qtVersion()->debuggingHelperLibrary();
}

QString MaemoRunConfiguration::executable() const
{
    TargetInformation ti = qt4Target()->qt4Project()->rootProjectNode()
        ->targetInformation(m_proFilePath);
    if (!ti.valid)
        return QString();

    return QDir::toNativeSeparators(QDir::cleanPath(ti.workingDir
        + QLatin1Char('/') + ti.target));
}

QString MaemoRunConfiguration::simulatorSshPort() const
{
    updateSimulatorInformation();
    return m_simulatorSshPort;
}

QString MaemoRunConfiguration::simulatorGdbServerPort() const
{
    updateSimulatorInformation();;
    return m_simulatorGdbServerPort;
}

QString MaemoRunConfiguration::simulatorPath() const
{
    updateSimulatorInformation();
    return m_simulatorPath;
}

QString MaemoRunConfiguration::visibleSimulatorParameter() const
{
    updateSimulatorInformation();
    return m_visibleSimulatorParameter;
}

QString MaemoRunConfiguration::simulator() const
{
    updateSimulatorInformation();
    return m_simulator;
}

QString MaemoRunConfiguration::simulatorArgs() const
{
    updateSimulatorInformation();
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

void MaemoRunConfiguration::updateSimulatorInformation() const
{
    if (m_cachedSimulatorInformationValid)
        return;

    m_simulator.clear();
    m_simulatorPath.clear();
    m_simulatorArgs.clear();
    m_visibleSimulatorParameter.clear();
    m_simulatorLibPath.clear();
    m_simulatorSshPort.clear();
    m_simulatorGdbServerPort.clear();
    m_cachedSimulatorInformationValid = true;

    if (const MaemoToolChain *tc = toolchain())
        m_simulatorPath = QDir::toNativeSeparators(tc->simulatorRoot());

    if (!m_simulatorPath.isEmpty()) {
        m_visibleSimulatorParameter = tr("'%1' does not contain a valid Maemo "
            "simulator image.").arg(m_simulatorPath);
    }

    QDir dir = QDir(m_simulatorPath);
    if (!m_simulatorPath.isEmpty() && dir.exists(m_simulatorPath)) {
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

                    m_simulator = map.value(QLatin1String("qemu"));
                    m_simulatorArgs = map.value(QLatin1String("qemu_args"));
                    const QString &libPathSpec
                        = map.value(QLatin1String("libpath"));
                    m_simulatorLibPath
                        = libPathSpec.mid(libPathSpec.indexOf(QLatin1Char('=')) + 1);
                    m_simulatorSshPort = map.value(QLatin1String("sshport"));
                    m_simulatorGdbServerPort
                        = map.value(QLatin1String("redirport2"));

                    m_visibleSimulatorParameter = m_simulator
#ifdef Q_OS_WIN
                        + QLatin1String(".exe")
#endif
                        + QLatin1Char(' ') + m_simulatorArgs;
                }
            }
        }
    } else {
        m_visibleSimulatorParameter = tr("Simulator could not be found. Please "
            "check the Qt Version you are using and that a simulator image is "
            "already installed.");
    }

    emit cachedSimulatorInformationChanged();
}

void MaemoRunConfiguration::startStopQemu()
{
    ProjectExplorerPlugin *explorer = ProjectExplorerPlugin::instance();
    if (explorer->session()->startupProject() != target()->project())
            return;

    const MaemoDeviceConfig &config = deviceConfig();
    if (!config.isValid()|| config.type != MaemoDeviceConfig::Simulator)
        return;

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

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
#ifdef Q_OS_WIN
    const QLatin1Char colon(';');
    const QString path = QDir::toNativeSeparators(root + QLatin1Char('/'));
    const QLatin1String key("PATH");
    env.insert(key, env.value(key) % colon % path % QLatin1String("bin"));
    env.insert(key, env.value(key) % colon % path % QLatin1String("madlib"));
#elif defined(Q_OS_UNIX)
    const QLatin1String key("LD_LIBRARY_PATH");
    env.insert(key, env.value(key) % QLatin1Char(':') % m_simulatorLibPath);
#endif
    qemu->setProcessEnvironment(env);
    qemu->setWorkingDirectory(simulatorPath());

    const QString app = root % QLatin1String("/madlib/") % simulator()
#ifdef Q_OS_WIN
        % QLatin1String(".exe")
#endif
    ;   // keep

    qemu->start(app % QLatin1Char(' ') % simulatorArgs(), QIODevice::ReadWrite);
    emit qemuProcessStatus(qemu->waitForStarted());
}

void MaemoRunConfiguration::qemuProcessFinished()
{
    emit qemuProcessStatus(false);
}

void MaemoRunConfiguration::updateDeviceConfigurations()
{
    qDebug("%s: Current devid = %llu", Q_FUNC_INFO, m_devConfig.internalId);
    const MaemoDeviceConfigurations &configManager
        = MaemoDeviceConfigurations::instance();
    if (!m_devConfig.isValid()) {
        const QList<MaemoDeviceConfig> &configList = configManager.devConfigs();
        if (!configList.isEmpty())
            m_devConfig = configList.first();
    } else {
        m_devConfig = configManager.find(m_devConfig.internalId);
    }
    qDebug("%s: new devid = %llu", Q_FUNC_INFO, m_devConfig.internalId);
    emit deviceConfigurationsUpdated();
}

    } // namespace Internal
} // namespace Qt4ProjectManager
