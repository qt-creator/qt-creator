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

#include "qnxrunconfigurationfactory.h"

#include "qnxconstants.h"
#include "qnxrunconfiguration.h"
#include "qnxdeviceconfigurationfactory.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4project.h>

using namespace Qnx;
using namespace Qnx::Internal;

static QString pathFromId(const Core::Id id)
{
    return id.suffixAfter(Constants::QNX_QNX_RUNCONFIGURATION_PREFIX);
}

QnxRunConfigurationFactory::QnxRunConfigurationFactory(QObject *parent) :
    ProjectExplorer::IRunConfigurationFactory(parent)
{
}

QList<Core::Id> QnxRunConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    QList<Core::Id> ids;
    if (!canHandle(parent))
        return ids;

    Qt4ProjectManager::Qt4Project *qt4Project = qobject_cast<Qt4ProjectManager::Qt4Project *>(parent->project());
    if (!qt4Project)
        return ids;

    QStringList proFiles = qt4Project->applicationProFilePathes(QLatin1String(Constants::QNX_QNX_RUNCONFIGURATION_PREFIX));
    foreach (const QString &pf, proFiles)
        ids << Core::Id::fromString(pf);
    return ids;
}

QString QnxRunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    const QString path = pathFromId(id);
    if (path.isEmpty())
        return QString();

    if (id.name().startsWith(Constants::QNX_QNX_RUNCONFIGURATION_PREFIX))
        return tr("%1 on QNX Device").arg(QFileInfo(path).completeBaseName());

    return QString();
}

bool QnxRunConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const Core::Id id) const
{
    if (!canHandle(parent) || !id.name().startsWith(Constants::QNX_QNX_RUNCONFIGURATION_PREFIX))
        return false;

    Qt4ProjectManager::Qt4Project *qt4Project = qobject_cast<Qt4ProjectManager::Qt4Project *>(parent->project());
    if (!qt4Project)
        return false;

    return qt4Project->hasApplicationProFile(pathFromId(id));
}

ProjectExplorer::RunConfiguration *QnxRunConfigurationFactory::doCreate(ProjectExplorer::Target *parent, const Core::Id id)
{
    return new QnxRunConfiguration(parent, id, pathFromId(id));
}

bool QnxRunConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    return canHandle(parent)
            && ProjectExplorer::idFromMap(map).name().startsWith(Constants::QNX_QNX_RUNCONFIGURATION_PREFIX);
}

ProjectExplorer::RunConfiguration *QnxRunConfigurationFactory::doRestore(ProjectExplorer::Target *parent,
                                                                         const QVariantMap &map)
{
    Q_UNUSED(map);
    return new QnxRunConfiguration(parent, Core::Id(Constants::QNX_QNX_RUNCONFIGURATION_PREFIX), QString());
}

bool QnxRunConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::RunConfiguration *QnxRunConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    QnxRunConfiguration *old = static_cast<QnxRunConfiguration *>(source);
    return new QnxRunConfiguration(parent, old);
}

bool QnxRunConfigurationFactory::canHandle(ProjectExplorer::Target *t) const
{
    Core::Id deviceType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(t->kit());
    if (deviceType != QnxDeviceConfigurationFactory::deviceType())
        return false;

    return true;
}
