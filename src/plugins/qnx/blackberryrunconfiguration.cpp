/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "blackberryrunconfiguration.h"
#include "qnxconstants.h"
#include "blackberrydeployconfiguration.h"
#include "blackberryrunconfigurationwidget.h"
#include "blackberrydeployinformation.h"

#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qt4projectmanager/qt4project.h>
#include <ssh/sshconnection.h>

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryRunConfiguration::BlackBerryRunConfiguration(ProjectExplorer::Target *parent, const Core::Id id, const QString &path)
    : ProjectExplorer::RunConfiguration(parent, id)
    , m_proFilePath(path)
{
    init();
}

BlackBerryRunConfiguration::BlackBerryRunConfiguration(ProjectExplorer::Target *parent,
                                         BlackBerryRunConfiguration *source)
    : ProjectExplorer::RunConfiguration(parent, source)
    , m_proFilePath(source->m_proFilePath)
{
    init();
}

void BlackBerryRunConfiguration::init()
{
    updateDisplayName();
}

void BlackBerryRunConfiguration::updateDisplayName()
{
    if (!m_proFilePath.isEmpty())
        setDefaultDisplayName(tr("%1 on BlackBerry device").arg(QFileInfo(m_proFilePath).completeBaseName()));
    else
        setDefaultDisplayName(tr("Run on BlackBerry device"));
}

QWidget *BlackBerryRunConfiguration::createConfigurationWidget()
{
    return new BlackBerryRunConfigurationWidget(this);
}

QString BlackBerryRunConfiguration::proFilePath() const
{
    return m_proFilePath;
}

QString BlackBerryRunConfiguration::deviceName() const
{
    return deployConfiguration()->deviceName();
}

QString BlackBerryRunConfiguration::barPackage() const
{
    BlackBerryDeployConfiguration *dc = deployConfiguration();
    if (!dc)
        return QString();

    QList<BarPackageDeployInformation> packages = dc->deploymentInfo()->enabledPackages();
    foreach (const BarPackageDeployInformation package, packages) {
        if (package.proFilePath == proFilePath()) {
            return package.packagePath;
        }
    }
    return QString();
}

QString BlackBerryRunConfiguration::localExecutableFilePath() const
{
    Qt4ProjectManager::Qt4Project *qt4Project = static_cast<Qt4ProjectManager::Qt4Project *>(target()->project());
    if (!qt4Project)
        return QString();

    Qt4ProjectManager::TargetInformation ti = qt4Project->rootQt4ProjectNode()->targetInformation(m_proFilePath);
    if (!ti.valid)
        return QString();

    return QDir::cleanPath(ti.buildDir + QLatin1Char('/') + ti.target);
}

bool BlackBerryRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!ProjectExplorer::RunConfiguration::fromMap(map))
        return false;

    m_proFilePath = map.value(QLatin1String(Constants::QNX_PROFILEPATH_KEY)).toString();
    if (m_proFilePath.isEmpty() || !QFileInfo(m_proFilePath).exists())
        return false;

    init();
    return true;
}

QVariantMap BlackBerryRunConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::RunConfiguration::toMap());
    map.insert(QLatin1String(Constants::QNX_PROFILEPATH_KEY), m_proFilePath);
    return map;
}

BlackBerryDeployConfiguration *BlackBerryRunConfiguration::deployConfiguration() const
{
    return qobject_cast<BlackBerryDeployConfiguration *>(target()->activeDeployConfiguration());
}

QString BlackBerryRunConfiguration::key() const
{
    return barPackage() + QLatin1Char('_') + BlackBerryDeviceConfiguration::device(target()->profile())->sshParameters().host;
}
