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
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qnxrunconfigurationfactory.h"

#include "qnxconstants.h"
#include "qnxrunconfiguration.h"
#include "qnxdeviceconfigurationfactory.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>

using namespace Qnx;
using namespace Qnx::Internal;

static Utils::FileName pathFromId(Core::Id id)
{
    return Utils::FileName::fromString(id.suffixAfter(Constants::QNX_QNX_RUNCONFIGURATION_PREFIX));
}

QnxRunConfigurationFactory::QnxRunConfigurationFactory(QObject *parent) :
    ProjectExplorer::IRunConfigurationFactory(parent)
{
}

QList<Core::Id> QnxRunConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent, CreationMode mode) const
{
    using QmakeProjectManager::QmakeProject;
    if (!canHandle(parent))
        return QList<Core::Id>();

    QmakeProject *qt4Project = qobject_cast<QmakeProject *>(parent->project());
    if (!qt4Project)
        return QList<Core::Id>();
    QList<QmakeProjectManager::QmakeProFileNode *> nodes = qt4Project->applicationProFiles();

    if (mode == AutoCreate)
        nodes = QmakeProject::nodesWithQtcRunnable(nodes);

    return QmakeProject::idsForNodes(Core::Id(Constants::QNX_QNX_RUNCONFIGURATION_PREFIX),
                                     nodes);
}

QString QnxRunConfigurationFactory::displayNameForId(Core::Id id) const
{
    const Utils::FileName path = pathFromId(id);
    if (path.isEmpty())
        return QString();

    if (id.name().startsWith(Constants::QNX_QNX_RUNCONFIGURATION_PREFIX))
        return tr("%1 on QNX Device").arg(path.toFileInfo().completeBaseName());

    return QString();
}

bool QnxRunConfigurationFactory::canCreate(ProjectExplorer::Target *parent, Core::Id id) const
{
    if (!canHandle(parent) || !id.name().startsWith(Constants::QNX_QNX_RUNCONFIGURATION_PREFIX))
        return false;

    QmakeProjectManager::QmakeProject *qt4Project = qobject_cast<QmakeProjectManager::QmakeProject *>(parent->project());
    if (!qt4Project)
        return false;

    return qt4Project->hasApplicationProFile(pathFromId(id));
}

ProjectExplorer::RunConfiguration *QnxRunConfigurationFactory::doCreate(ProjectExplorer::Target *parent, Core::Id id)
{
    const Utils::FileName projectFilePath = pathFromId(id);
    const QmakeProjectManager::QmakeProject * const qt4Project
            = qobject_cast<QmakeProjectManager::QmakeProject *>(parent->project());
    QTC_ASSERT(qt4Project, return 0);
    foreach (const QmakeProjectManager::QmakeProFileNode * const node,
             qt4Project->applicationProFiles()) {
        if (node->path() == projectFilePath)
            return new QnxRunConfiguration(parent, id, node->targetInformation().target);
    }
    QTC_CHECK(false);
    return 0;
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
