/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "qbsdeployconfigurationfactory.h"

#include "qbsinstallstep.h"
#include "qbsproject.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------

static QString genericQbsDisplayName() {
    return QCoreApplication::translate("Qbs", "Qbs Install");
}

static Core::Id genericQbsDeployConfigurationId()
{
    return "Qbs.Deploy";
}

// --------------------------------------------------------------------
// QbsDeployConfiguration:
// --------------------------------------------------------------------

QbsInstallStep *QbsDeployConfiguration::qbsInstallStep() const
{
    foreach (ProjectExplorer::BuildStep *bs, stepList()->steps()) {
        if (QbsInstallStep *install = qobject_cast<QbsInstallStep *>(bs))
            return install;
    }
    return 0;
}

QbsDeployConfiguration::QbsDeployConfiguration(ProjectExplorer::Target *target, Core::Id id) :
    ProjectExplorer::DeployConfiguration(target, id)
{ }

QbsDeployConfiguration::QbsDeployConfiguration(ProjectExplorer::Target *target,
                                               ProjectExplorer::DeployConfiguration *source) :
    ProjectExplorer::DeployConfiguration(target, source)
{
    cloneSteps(source);
}

// --------------------------------------------------------------------
// QbsDeployConfigurationFactory:
// --------------------------------------------------------------------

QbsDeployConfigurationFactory::QbsDeployConfigurationFactory(QObject *parent) :
    ProjectExplorer::DeployConfigurationFactory(parent)
{
    setObjectName(QLatin1String("QbsDeployConfiguration"));
}

QList<Core::Id> QbsDeployConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    QList<Core::Id> ids;
    const Core::Id deviceId = ProjectExplorer::DeviceKitInformation::deviceId(parent->kit());
    if (qobject_cast<QbsProject *>(parent->project())
            && deviceId == ProjectExplorer::Constants::DESKTOP_DEVICE_ID) {
        ids << genericQbsDeployConfigurationId();
    }
    return ids;
}

QString QbsDeployConfigurationFactory::displayNameForId(Core::Id id) const
{
    if (id == genericQbsDeployConfigurationId())
        return genericQbsDisplayName();
    return QString();
}

bool QbsDeployConfigurationFactory::canCreate(ProjectExplorer::Target *parent,
                                              const Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

ProjectExplorer::DeployConfiguration
*QbsDeployConfigurationFactory::create(ProjectExplorer::Target *parent, Core::Id id)
{
    Q_ASSERT(canCreate(parent, id));

    QbsDeployConfiguration *dc = new QbsDeployConfiguration(parent, id);
    dc->setDisplayName(genericQbsDisplayName());
    dc->stepList()->insertStep(0, new QbsInstallStep(dc->stepList()));
    return dc;
}

bool QbsDeployConfigurationFactory::canRestore(ProjectExplorer::Target *parent,
                                               const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::DeployConfiguration
*QbsDeployConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Core::Id id = ProjectExplorer::idFromMap(map);
    QbsDeployConfiguration *dc = new QbsDeployConfiguration(parent, id);
    if (!dc->fromMap(map)) {
        delete dc;
        return 0;
    }
    return dc;
}

ProjectExplorer::DeployConfiguration
*QbsDeployConfigurationFactory::clone(ProjectExplorer::Target *parent,
                                      ProjectExplorer::DeployConfiguration *product)
{
    if (!canClone(parent, product))
        return 0;
    return new QbsDeployConfiguration(parent, qobject_cast<QbsDeployConfiguration *>(product));
}

} // namespace Internal
} // namespace QbsProjectManager
