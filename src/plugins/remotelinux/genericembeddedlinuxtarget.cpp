/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "genericembeddedlinuxtarget.h"

#include "remotelinux_constants.h"
#include "remotelinuxrunconfiguration.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qt4projectmanager/qt4project.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/customexecutablerunconfiguration.h>

#include <QCoreApplication>

namespace RemoteLinux {
namespace Internal {

GenericEmbeddedLinuxTarget::GenericEmbeddedLinuxTarget(Qt4ProjectManager::Qt4Project *parent,
                                                       const Core::Id id) :
    AbstractEmbeddedLinuxTarget(parent, id)
{
    setDisplayName(tr("Embedded Linux"));
}

QList<ProjectExplorer::RunConfiguration *> GenericEmbeddedLinuxTarget::runConfigurationsForNode(ProjectExplorer::Node *n)
{
    QList<ProjectExplorer::RunConfiguration *> result;
    foreach (ProjectExplorer::RunConfiguration *rc, runConfigurations())
        if (RemoteLinuxRunConfiguration *qt4c = qobject_cast<RemoteLinuxRunConfiguration *>(rc))
            if (qt4c->proFilePath() == n->path())
                result << rc;
    return result;
}

Utils::FileName GenericEmbeddedLinuxTarget::mkspec(const Qt4ProjectManager::Qt4BuildConfiguration *bc) const
{
   QtSupport::BaseQtVersion *version = bc->qtVersion();
   if (!version)
       return Utils::FileName();
   return version->mkspec();
}

bool GenericEmbeddedLinuxTarget::supportsDevice(const ProjectExplorer::IDevice::ConstPtr &device) const
{
    return device->type() == Core::Id(Constants::GenericLinuxOsType);
}

void GenericEmbeddedLinuxTarget::createApplicationProFiles(bool reparse)
{
    if (!reparse)
        removeUnconfiguredCustomExectutableRunConfigurations();

    // We use the list twice
    QList<Qt4ProjectManager::Qt4ProFileNode *> profiles = qt4Project()->applicationProFiles();
    QStringList pathes;
    foreach (Qt4ProjectManager::Qt4ProFileNode *pro, profiles)
        pathes.append(pro->path());

    foreach (ProjectExplorer::RunConfiguration *rc, runConfigurations()) {
        if (RemoteLinuxRunConfiguration *qt4rc = qobject_cast<RemoteLinuxRunConfiguration *>(rc))
            pathes.removeAll(qt4rc->proFilePath());
    }

    // Only add new runconfigurations if there are none.
    foreach (const QString &path, pathes) {
        RemoteLinuxRunConfiguration *qt4rc =
            new RemoteLinuxRunConfiguration(this, Core::Id(RemoteLinuxRunConfiguration::Id), path);
        addRunConfiguration(qt4rc);
    }

    // Oh still none? Add a custom executable runconfiguration
    if (runConfigurations().isEmpty())
        addRunConfiguration(new QtSupport::CustomExecutableRunConfiguration(this));
}

} // namespace Internal
} // namespace RemoteLinux
