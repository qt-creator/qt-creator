/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "blackberryrunconfiguration.h"
#include "qnxconstants.h"
#include "blackberrydeployconfiguration.h"
#include "blackberryrunconfigurationwidget.h"
#include "blackberrydeployinformation.h"

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/target.h>
#include <ssh/sshconnection.h>

#include <QFileInfo>

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

    connect(deployConfiguration()->deploymentInfo(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SIGNAL(targetInformationChanged()));
    connect(deployConfiguration()->deploymentInfo(), SIGNAL(modelReset()), this, SIGNAL(targetInformationChanged()));
}

void BlackBerryRunConfiguration::updateDisplayName()
{
    if (!m_proFilePath.isEmpty())
        setDefaultDisplayName(QFileInfo(m_proFilePath).completeBaseName());
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
    BlackBerryDeviceConfiguration::ConstPtr device
            = BlackBerryDeviceConfiguration::device(target()->kit());
    if (!device)
        return QString();

    return device->displayName();
}

QString BlackBerryRunConfiguration::barPackage() const
{
    BlackBerryDeployConfiguration *dc = deployConfiguration();
    if (!dc)
        return QString();

    QList<BarPackageDeployInformation> packages = dc->deploymentInfo()->enabledPackages();
    foreach (const BarPackageDeployInformation package, packages) {
        if (package.proFilePath == proFilePath())
            return package.packagePath();
    }
    return QString();
}

QString BlackBerryRunConfiguration::localExecutableFilePath() const
{
    return target()->applicationTargets()
            .targetForProject(Utils::FileName::fromString(m_proFilePath)).toString();
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
    return barPackage() + QLatin1Char('_') + BlackBerryDeviceConfiguration::device(target()->kit())->sshParameters().host;
}
