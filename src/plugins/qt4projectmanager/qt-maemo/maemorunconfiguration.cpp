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

#include "maemopackagecreationstep.h"
#include "maemorunconfigurationwidget.h"
#include "maemotoolchain.h"
#include "qemuruntimemanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4project.h>

#include <utils/qtcassert.h>

#include <QtCore/QStringBuilder>

namespace Qt4ProjectManager {
    namespace Internal {

using namespace ProjectExplorer;

MaemoRunConfiguration::MaemoRunConfiguration(Qt4Target *parent,
        const QString &proFilePath)
    : RunConfiguration(parent, QLatin1String(MAEMO_RC_ID))
    , m_proFilePath(proFilePath)
{
    init();
}

MaemoRunConfiguration::MaemoRunConfiguration(Qt4Target *parent,
        MaemoRunConfiguration *source)
    : RunConfiguration(parent, source)
    , m_proFilePath(source->m_proFilePath)
    , m_gdbPath(source->m_gdbPath)
    , m_devConfig(source->m_devConfig)
    , m_arguments(source->m_arguments)
    , m_lastDeployed(source->m_lastDeployed)
{
    init();
}

void MaemoRunConfiguration::init()
{
    setDisplayName(tr("New Maemo Run Configuration"));

    updateDeviceConfigurations();
    connect(&MaemoDeviceConfigurations::instance(), SIGNAL(updated()), this,
        SLOT(updateDeviceConfigurations()));

    connect(qt4Target()->qt4Project(),
        SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode*)),
        this, SLOT(proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode*)));
}

MaemoRunConfiguration::~MaemoRunConfiguration()
{
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

QWidget *MaemoRunConfiguration::createConfigurationWidget()
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
    addDeployTimesToMap(map);
    const QDir dir = QDir(target()->project()->projectDirectory());
    map.insert(ProFileKey, dir.relativeFilePath(m_proFilePath));

    return map;
}

void MaemoRunConfiguration::addDeployTimesToMap(QVariantMap &map) const
{
    QVariantList hostList;
    QVariantList fileList;
    QVariantList remotePathList;
    QVariantList timeList;
    typedef QHash<DeployablePerHost, QDateTime>::ConstIterator DepIt;
    for (DepIt it = m_lastDeployed.begin(); it != m_lastDeployed.end(); ++it) {
        hostList << it.key().first.localFilePath;
        remotePathList << it.key().first.remoteFilePath;
        fileList << it.key().second;
        timeList << it.value();
    }
    map.insert(LastDeployedHostsKey, hostList);
    map.insert(LastDeployedFilesKey, fileList);
    map.insert(LastDeployedRemotePathsKey, remotePathList);
    map.insert(LastDeployedTimesKey, timeList);
}

bool MaemoRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    setDeviceConfig(MaemoDeviceConfigurations::instance().
        find(map.value(DeviceIdKey, 0).toULongLong()));
    m_arguments = map.value(ArgumentsKey).toStringList();
    getDeployTimesFromMap(map);
    const QDir dir = QDir(target()->project()->projectDirectory());
    m_proFilePath = dir.filePath(map.value(ProFileKey).toString());

    return true;
}

void MaemoRunConfiguration::getDeployTimesFromMap(const QVariantMap &map)
{
    const QVariantList &hostList = map.value(LastDeployedHostsKey).toList();
    const QVariantList &fileList = map.value(LastDeployedFilesKey).toList();
    const QVariantList &remotePathList
        = map.value(LastDeployedRemotePathsKey).toList();
    const QVariantList &timeList = map.value(LastDeployedTimesKey).toList();
    const int elemCount
        = qMin(qMin(hostList.size(), fileList.size()),
            qMin(remotePathList.size(), timeList.size()));
    for (int i = 0; i < elemCount; ++i) {
        const MaemoDeployable d(fileList.at(i).toString(),
            remotePathList.at(i).toString());
        m_lastDeployed.insert(DeployablePerHost(d, hostList.at(i).toString()),
            timeList.at(i).toDateTime());
    }
}

bool MaemoRunConfiguration::currentlyNeedsDeployment(const QString &host,
    const MaemoDeployable &deployable) const
{
    const QDateTime &lastDeployed
        = m_lastDeployed.value(DeployablePerHost(deployable, host));
    return !lastDeployed.isValid()
        || QFileInfo(deployable.localFilePath).lastModified() > lastDeployed;
}

void MaemoRunConfiguration::setDeployed(const QString &host,
    const MaemoDeployable &deployable)
{
    m_lastDeployed.insert(DeployablePerHost(deployable, host),
        QDateTime::currentDateTime());
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
    if (const MaemoToolChain *tc = toolchain())
        return QDir::toNativeSeparators(tc->targetRoot() + QLatin1String("/bin/gdb"));
    return QString();
}

const MaemoPackageCreationStep *MaemoRunConfiguration::packageStep() const
{
    const QList<ProjectExplorer::BuildStep *> &buildSteps
        = activeQt4BuildConfiguration()->steps(ProjectExplorer::Build);
    for (int i = buildSteps.count() - 1; i >= 0; --i) {
        const MaemoPackageCreationStep * const pStep
            = qobject_cast<MaemoPackageCreationStep *>(buildSteps.at(i));
        if (pStep)
            return pStep;
    }
    Q_ASSERT(!"Impossible: Maemo run configuration without packaging step.");
    return 0;
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

QString MaemoRunConfiguration::runtimeGdbServerPort() const
{
    if (Qt4BuildConfiguration *qt4bc = activeQt4BuildConfiguration()) {
        Runtime rt;
        const int id = qt4bc->qtVersion()->uniqueId();
        if (QemuRuntimeManager::instance().runtimeForQtVersion(id, &rt))
            return rt.m_gdbServerPort;
    }
    return QLatin1String("13219");
}

void MaemoRunConfiguration::setArguments(const QStringList &args)
{
    m_arguments = args;
}

void MaemoRunConfiguration::updateDeviceConfigurations()
{
    const MaemoDeviceConfigurations &configManager
        = MaemoDeviceConfigurations::instance();
    if (!m_devConfig.isValid()) {
        const QList<MaemoDeviceConfig> &configList = configManager.devConfigs();
        if (!configList.isEmpty())
            m_devConfig = configList.first();
    } else {
        m_devConfig = configManager.find(m_devConfig.internalId);
    }
    emit deviceConfigurationsUpdated(target());
}

    } // namespace Internal
} // namespace Qt4ProjectManager
