/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "blackberrydeployconfiguration.h"

#include "qnxconstants.h"
#include "bardescriptorfilenode.h"
#include "blackberrydeployconfigurationwidget.h"
#include "blackberrydeployinformation.h"

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const char DEPLOYMENT_INFO_KEY[]     = "Qnx.BlackBerry.DeployInformation";
}

BlackBerryDeployConfiguration::BlackBerryDeployConfiguration(ProjectExplorer::Target *parent)
    : ProjectExplorer::DeployConfiguration(parent, Core::Id(Constants::QNX_BB_DEPLOYCONFIGURATION_ID))
{
    ctor();
}

BlackBerryDeployConfiguration::BlackBerryDeployConfiguration(ProjectExplorer::Target *parent,
                                               BlackBerryDeployConfiguration *source)
    : ProjectExplorer::DeployConfiguration(parent, source)
{
    ctor();
    cloneSteps(source);
}

void BlackBerryDeployConfiguration::ctor()
{
    m_deployInformation = new BlackBerryDeployInformation(target());

    setDefaultDisplayName(tr("Deploy to BlackBerry Device"));
}

BlackBerryDeployConfiguration::~BlackBerryDeployConfiguration()
{
}

BlackBerryDeployInformation *BlackBerryDeployConfiguration::deploymentInfo() const
{
    return m_deployInformation;
}

ProjectExplorer::NamedWidget *BlackBerryDeployConfiguration::createConfigWidget()
{
    return new BlackBerryDeployConfigurationWidget(this);
}

QVariantMap BlackBerryDeployConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::DeployConfiguration::toMap());
    map.insert(QLatin1String(DEPLOYMENT_INFO_KEY), deploymentInfo()->toMap());
    return map;
}

bool BlackBerryDeployConfiguration::fromMap(const QVariantMap &map)
{
    if (!ProjectExplorer::DeployConfiguration::fromMap(map))
        return false;

    QVariantMap deployInfoMap = map.value(QLatin1String(DEPLOYMENT_INFO_KEY)).toMap();
    deploymentInfo()->fromMap(deployInfoMap);
    return true;
}
