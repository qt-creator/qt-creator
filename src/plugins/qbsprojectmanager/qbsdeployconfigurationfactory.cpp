/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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

bool QbsDeployConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *product) const
{
    return canCreate(parent, product->id());
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
