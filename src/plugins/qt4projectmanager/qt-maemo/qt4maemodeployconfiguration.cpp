/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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


#include "maemodeploystep.h"
#include "maemopackagecreationstep.h"
#include "qt4maemodeployconfiguration.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/target.h>

#include <qt4projectmanager/qt4projectmanagerconstants.h>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

Qt4MaemoDeployConfigurationFactory::Qt4MaemoDeployConfigurationFactory(QObject *parent) :
    ProjectExplorer::DeployConfigurationFactory(parent)
{ }

ProjectExplorer::DeployConfiguration *Qt4MaemoDeployConfigurationFactory::create(ProjectExplorer::Target *parent, const QString &id)
{
    ProjectExplorer::DeployConfiguration *dc = ProjectExplorer::DeployConfigurationFactory::create(parent, id);

    if (!dc)
        return 0;
    if (parent->id() == QLatin1String(Constants::MAEMO5_DEVICE_TARGET_ID))
        dc->setDefaultDisplayName(tr("Deploy to Maemo5 device"));
    if (parent->id() == QLatin1String(Constants::HARMATTAN_DEVICE_TARGET_ID))
        dc->setDefaultDisplayName(tr("Deploy to Harmattan device"));
    if (parent->id() == QLatin1String(Constants::MEEGO_DEVICE_TARGET_ID))
        dc->setDefaultDisplayName(tr("Deploy to Meego device"));
    dc->stepList()->insertStep(0, new MaemoPackageCreationStep(dc->stepList()));
    dc->stepList()->insertStep(1, new MaemoDeployStep(dc->stepList()));

    return dc;
}
