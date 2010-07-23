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

#include "maemodeployables.h"
#include "maemodeploystep.h"
#include "maemodeviceconfiglistmodel.h"
#include "maemoglobal.h"
#include "maemoremotemountsmodel.h"
#include "maemorunconfigurationwidget.h"
#include "maemotoolchain.h"
#include "qemuruntimemanager.h"
#include "qtoutputformatter.h"

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
    , m_arguments(source->m_arguments)
{
    init();
}

void MaemoRunConfiguration::init()
{
    m_devConfigModel = new MaemoDeviceConfigListModel(this);
    m_remoteMounts = new MaemoRemoteMountsModel(this);
    setDisplayName(QFileInfo(m_proFilePath).completeBaseName());

    updateDeviceConfigurations();
    connect(m_devConfigModel, SIGNAL(currentChanged()), this,
        SLOT(updateDeviceConfigurations()));
    connect(m_devConfigModel, SIGNAL(modelReset()), this,
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

ProjectExplorer::OutputFormatter *MaemoRunConfiguration::createConfigurationWidget() const
{
    return new QtOutputFormatter(qt4Target()->qt4Project());
}

void MaemoRunConfiguration::proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode *pro)
{
    if (m_proFilePath == pro->path())
        emit targetInformationChanged();
}

QVariantMap MaemoRunConfiguration::toMap() const
{
    QVariantMap map(RunConfiguration::toMap());
    map.insert(ArgumentsKey, m_arguments);
    const QDir dir = QDir(target()->project()->projectDirectory());
    map.insert(ProFileKey, dir.relativeFilePath(m_proFilePath));
    map.unite(m_devConfigModel->toMap());
    map.unite(m_remoteMounts->toMap());
    return map;
}

bool MaemoRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    m_arguments = map.value(ArgumentsKey).toStringList();
    const QDir dir = QDir(target()->project()->projectDirectory());
    m_proFilePath = dir.filePath(map.value(ProFileKey).toString());
    m_devConfigModel->fromMap(map);
    m_remoteMounts->fromMap(map);

    return true;
}

MaemoDeviceConfig MaemoRunConfiguration::deviceConfig() const
{
    return m_devConfigModel->current();
}

MaemoDeviceConfigListModel *MaemoRunConfiguration::deviceConfigModel() const
{
    return m_devConfigModel;
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

MaemoDeployStep *MaemoRunConfiguration::deployStep() const
{
    MaemoDeployStep * const step
        = MaemoGlobal::buildStep<MaemoDeployStep>(activeQt4BuildConfiguration());
    Q_ASSERT_X(step, Q_FUNC_INFO,
        "Impossible: Maemo build configuration without deploy step.");
    return step;
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
    return deployStep()->deployables()
        ->remoteExecutableFilePath(localExecutableFilePath());
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
    emit deviceConfigurationChanged(target());
}

} // namespace Internal
} // namespace Qt4ProjectManager
