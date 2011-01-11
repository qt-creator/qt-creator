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

#include "maemorunconfiguration.h"

#include "maemodeployables.h"
#include "maemodeploystep.h"
#include "maemodeviceconfiglistmodel.h"
#include "maemoglobal.h"
#include "maemoqemumanager.h"
#include "maemoremotemountsmodel.h"
#include "maemorunconfigurationwidget.h"
#include "maemotoolchain.h"
#include "qtoutputformatter.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4project.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QtCore/QStringBuilder>

namespace Qt4ProjectManager {
namespace Internal {

namespace {
const bool DefaultUseRemoteGdbValue = false;
} // anonymous namespace

using namespace ProjectExplorer;

MaemoRunConfiguration::MaemoRunConfiguration(Qt4Target *parent,
        const QString &proFilePath)
    : RunConfiguration(parent, QLatin1String(MAEMO_RC_ID))
    , m_proFilePath(proFilePath)
    , m_useRemoteGdb(DefaultUseRemoteGdbValue)
    , m_baseEnvironmentBase(SystemEnvironmentBase)
    , m_validParse(parent->qt4Project()->validParse(m_proFilePath))
{
    init();
}

MaemoRunConfiguration::MaemoRunConfiguration(Qt4Target *parent,
        MaemoRunConfiguration *source)
    : RunConfiguration(parent, source)
    , m_proFilePath(source->m_proFilePath)
    , m_gdbPath(source->m_gdbPath)
    , m_arguments(source->m_arguments)
    , m_useRemoteGdb(source->useRemoteGdb())
    , m_baseEnvironmentBase(source->m_baseEnvironmentBase)
    , m_systemEnvironment(source->m_systemEnvironment)
    , m_userEnvironmentChanges(source->m_userEnvironmentChanges)
    , m_validParse(source->m_validParse)
{
    init();
}

void MaemoRunConfiguration::init()
{
    setDefaultDisplayName(defaultDisplayName());
    setUseCppDebugger(true);
    setUseQmlDebugger(false);
    m_remoteMounts = new MaemoRemoteMountsModel(this);

    connect(target(),
        SIGNAL(activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration*)),
        this, SLOT(handleDeployConfigChanged()));
    handleDeployConfigChanged();

    Qt4Project *pro = qt4Target()->qt4Project();
    connect(pro, SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode*,bool)),
            this, SLOT(proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode*,bool)));
    connect(pro, SIGNAL(proFileInvalidated(Qt4ProjectManager::Internal::Qt4ProFileNode *)),
            this, SLOT(proFileInvalidated(Qt4ProjectManager::Internal::Qt4ProFileNode*)));
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

bool MaemoRunConfiguration::isEnabled(ProjectExplorer::BuildConfiguration * /* config */) const
{
    if (!m_validParse)
        return false;
    return true;
}

QWidget *MaemoRunConfiguration::createConfigurationWidget()
{
    return new MaemoRunConfigurationWidget(this);
}

ProjectExplorer::OutputFormatter *MaemoRunConfiguration::createOutputFormatter() const
{
    return new QtOutputFormatter(qt4Target()->qt4Project());
}

void MaemoRunConfiguration::handleParseState(bool success)
{
    bool enabled = isEnabled();
    m_validParse = success;
    if (enabled != isEnabled()) {
        emit isEnabledChanged(!enabled);
    }
}

void MaemoRunConfiguration::proFileInvalidated(Qt4ProjectManager::Internal::Qt4ProFileNode *pro)
{
    if (m_proFilePath != pro->path())
        return;
    handleParseState(false);
}

void MaemoRunConfiguration::proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode *pro, bool success)
{
    if (m_proFilePath == pro->path()) {
        handleParseState(success);
        emit targetInformationChanged();
    }
}

QVariantMap MaemoRunConfiguration::toMap() const
{
    QVariantMap map(RunConfiguration::toMap());
    map.insert(ArgumentsKey, m_arguments);
    const QDir dir = QDir(target()->project()->projectDirectory());
    map.insert(ProFileKey, dir.relativeFilePath(m_proFilePath));
    map.insert(UseRemoteGdbKey, useRemoteGdb());
    map.insert(BaseEnvironmentBaseKey, m_baseEnvironmentBase);
    map.insert(UserEnvironmentChangesKey,
        Utils::EnvironmentItem::toStringList(m_userEnvironmentChanges));
    map.unite(m_remoteMounts->toMap());
    return map;
}

bool MaemoRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    m_arguments = map.value(ArgumentsKey).toString();
    const QDir dir = QDir(target()->project()->projectDirectory());
    m_proFilePath = dir.filePath(map.value(ProFileKey).toString());
    m_useRemoteGdb = map.value(UseRemoteGdbKey, DefaultUseRemoteGdbValue).toBool();
    m_userEnvironmentChanges =
        Utils::EnvironmentItem::fromStringList(map.value(UserEnvironmentChangesKey)
        .toStringList());
    m_baseEnvironmentBase = static_cast<BaseEnvironmentBase> (map.value(BaseEnvironmentBaseKey,
        SystemEnvironmentBase).toInt());
    m_remoteMounts->fromMap(map);

    m_validParse = qt4Target()->qt4Project()->validParse(m_proFilePath);

    setDefaultDisplayName(defaultDisplayName());

    return true;
}

QString MaemoRunConfiguration::defaultDisplayName()
{
    if (!m_proFilePath.isEmpty())
        return (QFileInfo(m_proFilePath).completeBaseName());
    //: Maemo run configuration default display name
    return tr("Run on Maemo device");
}

MaemoDeviceConfig MaemoRunConfiguration::deviceConfig() const
{
    const MaemoDeployStep * const step = deployStep();
    return step ? step->deviceConfigModel()->current() : MaemoDeviceConfig();
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
    return QDir::toNativeSeparators(targetRoot() + QLatin1String("/bin/gdb"));
}

MaemoDeployStep *MaemoRunConfiguration::deployStep() const
{
    return MaemoGlobal::buildStep<MaemoDeployStep>(target()->activeDeployConfiguration());
}

const QString MaemoRunConfiguration::sysRoot() const
{
    if (const MaemoToolChain *tc = toolchain())
        return tc->sysroot();
    return QString();
}

const QString MaemoRunConfiguration::targetRoot() const
{
    QTC_ASSERT(activeQt4BuildConfiguration(), return QString());
    return MaemoGlobal::targetRoot(activeQt4BuildConfiguration()->qtVersion());
}

const QString MaemoRunConfiguration::arguments() const
{
    return m_arguments;
}

const QString MaemoRunConfiguration::dumperLib() const
{
    Qt4BuildConfiguration *qt4bc(activeQt4BuildConfiguration());
    return qt4bc->qtVersion()->debuggingHelperLibrary();
}


QString MaemoRunConfiguration::localDirToMountForRemoteGdb() const
{
    const QString projectDir
        = QDir::fromNativeSeparators(QDir::cleanPath(activeBuildConfiguration()
            ->target()->project()->projectDirectory()));
    const QString execDir
        = QDir::fromNativeSeparators(QFileInfo(localExecutableFilePath()).path());
    const int length = qMin(projectDir.length(), execDir.length());
    int lastSeparatorPos = 0;
    for (int i = 0; i < length; ++i) {
        if (projectDir.at(i) != execDir.at(i))
            return projectDir.left(lastSeparatorPos);
        if (projectDir.at(i) == QLatin1Char('/'))
            lastSeparatorPos = i;
    }
    return projectDir.length() == execDir.length()
        ? projectDir : projectDir.left(lastSeparatorPos);
}

QString MaemoRunConfiguration::remoteProjectSourcesMountPoint() const
{
    return MaemoGlobal::homeDirOnDevice(deviceConfig().server.uname)
        + QLatin1String("/gdbSourcesDir_")
        + QFileInfo(localExecutableFilePath()).fileName();
}

QString MaemoRunConfiguration::localExecutableFilePath() const
{
    TargetInformation ti = qt4Target()->qt4Project()->rootProjectNode()
        ->targetInformation(m_proFilePath);
    if (!ti.valid)
        return QString();

    return QDir::cleanPath(ti.workingDir + QLatin1Char('/') + ti.target);
}

QString MaemoRunConfiguration::remoteExecutableFilePath() const
{
    const MaemoDeployStep * const step = deployStep();
    return step
        ? step->deployables()->remoteExecutableFilePath(localExecutableFilePath())
        : QString();
}

MaemoPortList MaemoRunConfiguration::freePorts() const
{
    const MaemoDeviceConfig &devConfig = deviceConfig();
    const Qt4BuildConfiguration * const qt4bc = activeQt4BuildConfiguration();
    if (devConfig.type == MaemoDeviceConfig::Simulator && qt4bc) {
        MaemoQemuRuntime rt;
        const int id = qt4bc->qtVersion()->uniqueId();
        if (MaemoQemuManager::instance().runtimeForQtVersion(id, &rt))
            return rt.m_freePorts;
    }
    return devConfig.freePorts();
}

bool MaemoRunConfiguration::useRemoteGdb() const
{
    return m_useRemoteGdb
        && MaemoGlobal::allowsRemoteMounts(activeQt4BuildConfiguration()->qtVersion());
}

void MaemoRunConfiguration::setArguments(const QString &args)
{
    m_arguments = args;
}

MaemoRunConfiguration::DebuggingType MaemoRunConfiguration::debuggingType() const
{
    if (!MaemoGlobal::allowsQmlDebugging(activeQt4BuildConfiguration()->qtVersion()))
        return DebugCppOnly;
    if (useCppDebugger()) {
        if (useQmlDebugger())
            return DebugCppAndQml;
        return DebugCppOnly;
    }
    return DebugQmlOnly;
}

int MaemoRunConfiguration::portsUsedByDebuggers() const
{
    switch (debuggingType()) {
    case DebugCppOnly:
    case DebugQmlOnly:
        return 1;
    case DebugCppAndQml:
    default:
        return 2;
    }
}

void MaemoRunConfiguration::updateDeviceConfigurations()
{
    emit deviceConfigurationChanged(target());
}

void MaemoRunConfiguration::handleDeployConfigChanged()
{
    const QList<DeployConfiguration *> &deployConfigs
        = target()->deployConfigurations();
    DeployConfiguration * const activeDeployConf
        = target()->activeDeployConfiguration();
    for (int i = 0; i < deployConfigs.count(); ++i) {
        MaemoDeployStep * const step
            = MaemoGlobal::buildStep<MaemoDeployStep>(deployConfigs.at(i));
        MaemoDeviceConfigListModel * const devConfigModel
            = step->deviceConfigModel();
        if (deployConfigs.at(i) == activeDeployConf) {
            connect(devConfigModel, SIGNAL(currentChanged()), this,
                SLOT(updateDeviceConfigurations()));
            connect(devConfigModel, SIGNAL(modelReset()), this,
                SLOT(updateDeviceConfigurations()));
        } else {
            disconnect(devConfigModel, 0, this,
                SLOT(updateDeviceConfigurations()));
        }
    }
    updateDeviceConfigurations();
    updateFactoryState();
}

QString MaemoRunConfiguration::baseEnvironmentText() const
{
    if (m_baseEnvironmentBase == CleanEnvironmentBase)
        return tr("Clean Environment");
    else  if (m_baseEnvironmentBase == SystemEnvironmentBase)
        return tr("System Environment");
    return QString();
}

MaemoRunConfiguration::BaseEnvironmentBase MaemoRunConfiguration::baseEnvironmentBase() const
{
    return m_baseEnvironmentBase;
}

void MaemoRunConfiguration::setBaseEnvironmentBase(BaseEnvironmentBase env)
{
    if (m_baseEnvironmentBase != env) {
        m_baseEnvironmentBase = env;
        emit baseEnvironmentChanged();
    }
}

Utils::Environment MaemoRunConfiguration::environment() const
{
    Utils::Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

Utils::Environment MaemoRunConfiguration::baseEnvironment() const
{
    return (m_baseEnvironmentBase == SystemEnvironmentBase ? systemEnvironment()
        : Utils::Environment());
}

QList<Utils::EnvironmentItem> MaemoRunConfiguration::userEnvironmentChanges() const
{
    return m_userEnvironmentChanges;
}

void MaemoRunConfiguration::setUserEnvironmentChanges(
    const QList<Utils::EnvironmentItem> &diff)
{
    if (m_userEnvironmentChanges != diff) {
        m_userEnvironmentChanges = diff;
        emit userEnvironmentChangesChanged(diff);
    }
}

Utils::Environment MaemoRunConfiguration::systemEnvironment() const
{
    return m_systemEnvironment;
}

void MaemoRunConfiguration::setSystemEnvironment(const Utils::Environment &environment)
{
    if (m_systemEnvironment.size() == 0 || m_systemEnvironment != environment) {
        m_systemEnvironment = environment;
        emit systemEnvironmentChanged();
    }
}

} // namespace Internal
} // namespace Qt4ProjectManager
