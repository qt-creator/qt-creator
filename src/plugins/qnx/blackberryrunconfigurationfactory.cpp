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

#include "blackberryrunconfigurationfactory.h"
#include "qnxconstants.h"
#include "blackberryrunconfiguration.h"
#include "blackberrydeviceconfigurationfactory.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4project.h>

using namespace Qnx;
using namespace Qnx::Internal;

static QString pathFromId(const Core::Id id)
{
    return id.suffixAfter(Constants::QNX_BB_RUNCONFIGURATION_PREFIX);
}

BlackBerryRunConfigurationFactory::BlackBerryRunConfigurationFactory(QObject *parent) :
    ProjectExplorer::IRunConfigurationFactory(parent)
{
}

QList<Core::Id> BlackBerryRunConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    QList<Core::Id> ids;
    if (!canHandle(parent))
        return ids;

    Qt4ProjectManager::Qt4Project *qt4Project = qobject_cast<Qt4ProjectManager::Qt4Project *>(parent->project());
    if (!qt4Project)
        return ids;

    QStringList proFiles = qt4Project->applicationProFilePathes(QLatin1String(Constants::QNX_BB_RUNCONFIGURATION_PREFIX));
    foreach (const QString &pf, proFiles)
        ids << Core::Id::fromString(pf);

    return ids;
}

QString BlackBerryRunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    const QString path = pathFromId(id);
    if (path.isEmpty())
        return QString();

    if (id.name().startsWith(Constants::QNX_BB_RUNCONFIGURATION_PREFIX))
        return QFileInfo(path).completeBaseName();

    return QString();
}

bool BlackBerryRunConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const Core::Id id) const
{
    if (!canHandle(parent))
        return false;

    Qt4ProjectManager::Qt4Project *qt4Project = qobject_cast<Qt4ProjectManager::Qt4Project *>(parent->project());
    if (!qt4Project)
        return false;

    if (!id.name().startsWith(Constants::QNX_BB_RUNCONFIGURATION_PREFIX))
        return false;

    return qt4Project->hasApplicationProFile(pathFromId(id));
}

ProjectExplorer::RunConfiguration *BlackBerryRunConfigurationFactory::doCreate(ProjectExplorer::Target *parent,
                                                                      const Core::Id id)
{
    return new BlackBerryRunConfiguration(parent, id, pathFromId(id));
}

bool BlackBerryRunConfigurationFactory::canRestore(ProjectExplorer::Target *parent,
                                            const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;

    return ProjectExplorer::idFromMap(map).name().startsWith(Constants::QNX_BB_RUNCONFIGURATION_PREFIX);
}

ProjectExplorer::RunConfiguration *BlackBerryRunConfigurationFactory::doRestore(
        ProjectExplorer::Target *parent,
        const QVariantMap &map)
{
    Q_UNUSED(map);
    return new BlackBerryRunConfiguration(parent, Core::Id(Constants::QNX_BB_RUNCONFIGURATION_PREFIX), QString());
}

bool BlackBerryRunConfigurationFactory::canClone(ProjectExplorer::Target *parent,
                                          ProjectExplorer::RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::RunConfiguration *BlackBerryRunConfigurationFactory::clone(
        ProjectExplorer::Target *parent,
        ProjectExplorer::RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    BlackBerryRunConfiguration *old = static_cast<BlackBerryRunConfiguration *>(source);
    return new BlackBerryRunConfiguration(parent, old);

}

bool BlackBerryRunConfigurationFactory::canHandle(ProjectExplorer::Target *t) const
{
    if (!t->project()->supportsKit(t->kit()))
        return false;
    if (!qobject_cast<Qt4ProjectManager::Qt4Project *>(t->project()))
        return false;

    Core::Id deviceType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(t->kit());
    if (deviceType != BlackBerryDeviceConfigurationFactory::deviceType())
        return false;

    return true;
}
