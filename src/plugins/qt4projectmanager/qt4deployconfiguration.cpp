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

#include "qt4deployconfiguration.h"

#include "qt4projectmanagerconstants.h"
#include "qt4target.h"
#include "qt-maemo/maemodeploystep.h"
#include "qt-maemo/maemopackagecreationstep.h"
#include "qt-s60/s60createpackagestep.h"
#include "qt-s60/s60deploystep.h"
#include "qt-s60/s60deployconfiguration.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/target.h>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

Qt4DeployConfigurationFactory::Qt4DeployConfigurationFactory(QObject *parent) :
    ProjectExplorer::DeployConfigurationFactory(parent),
    m_s60DeployConfigurationFactory(new S60DeployConfigurationFactory(this))
{ }

ProjectExplorer::DeployConfiguration *Qt4DeployConfigurationFactory::create(ProjectExplorer::Target *parent, const QString &id)
{
    ProjectExplorer::DeployConfiguration *dc = 0;
    if (parent->id() == QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        dc = m_s60DeployConfigurationFactory->create(parent, id);
    else
        dc = ProjectExplorer::DeployConfigurationFactory::create(parent, id);

    if (!dc)
        return 0;

    if (parent->id() == Constants::S60_DEVICE_TARGET_ID) {
        dc->setDefaultDisplayName(tr("Deploy to Symbian device"));
        dc->stepList()->insertStep(0, new S60CreatePackageStep(dc->stepList()));
        dc->stepList()->insertStep(1, new S60DeployStep(dc->stepList()));
    } else if (parent->id() == Constants::MAEMO_DEVICE_TARGET_ID) {
        dc->setDefaultDisplayName(tr("Deploy to Maemo device"));
        dc->stepList()->insertStep(0, new MaemoPackageCreationStep(dc->stepList()));
        dc->stepList()->insertStep(1, new MaemoDeployStep(dc->stepList()));
    }

    return dc;
}

bool Qt4DeployConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    if (parent->id() == Constants::S60_DEVICE_TARGET_ID)
        return m_s60DeployConfigurationFactory->canRestore(parent, map);
    else
        return ProjectExplorer::DeployConfigurationFactory::canRestore(parent, map);
}

ProjectExplorer::DeployConfiguration *Qt4DeployConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (parent->id() == Constants::S60_DEVICE_TARGET_ID)
        return m_s60DeployConfigurationFactory->restore(parent, map);
    else
        return ProjectExplorer::DeployConfigurationFactory::restore(parent, map);
}

bool Qt4DeployConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *product) const
{
    if (parent->id() == Constants::S60_DEVICE_TARGET_ID)
        return m_s60DeployConfigurationFactory->canClone(parent, product);
    else
        return ProjectExplorer::DeployConfigurationFactory::canClone(parent, product);
}

ProjectExplorer::DeployConfiguration *Qt4DeployConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *product)
{
    if (parent->id() == Constants::S60_DEVICE_TARGET_ID)
        return m_s60DeployConfigurationFactory->clone(parent, product);
    else
        return ProjectExplorer::DeployConfigurationFactory::clone(parent, product);
}
