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

#include "qt4symbiantarget.h"
#include "qt4projectmanagerconstants.h"
#include "qt4project.h"
#include "qt4nodes.h"
#include "qt4buildconfiguration.h"
#include "symbianidevice.h"

#include "qt-s60/s60deployconfiguration.h"
#include "qt-s60/s60devicerunconfiguration.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/project.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/customexecutablerunconfiguration.h>
#include <extensionsystem/pluginmanager.h>
#include <QApplication>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

Qt4SymbianTarget::Qt4SymbianTarget(Qt4Project *parent, const Core::Id id) :
    Qt4BaseTarget(parent, id),
    m_buildConfigurationFactory(new Qt4BuildConfigurationFactory(this))
{
    setDisplayName(defaultDisplayName(id));
    setIcon(iconForId(id));
}

Qt4SymbianTarget::~Qt4SymbianTarget()
{ }

QString Qt4SymbianTarget::defaultDisplayName(const Core::Id id)
{
    if (id == Core::Id(Constants::S60_DEVICE_TARGET_ID))
        return QApplication::translate("Qt4ProjectManager::Qt4Target", "Symbian Device", "Qt4 Symbian Device target display name");
    return QString();
}

QIcon Qt4SymbianTarget::iconForId(Core::Id id)
{
    if (id == Core::Id(Constants::S60_DEVICE_TARGET_ID))
        return QIcon(QLatin1String(":/projectexplorer/images/SymbianDevice.png"));
    return QIcon();
}

ProjectExplorer::IBuildConfigurationFactory *Qt4SymbianTarget::buildConfigurationFactory() const
{
    return m_buildConfigurationFactory;
}

void Qt4SymbianTarget::createApplicationProFiles(bool reparse)
{
    if (!reparse)
        removeUnconfiguredCustomExectutableRunConfigurations();

    // We use the list twice
    QList<Qt4ProFileNode *> profiles = qt4Project()->applicationProFiles();
    QSet<QString> paths;
    foreach (Qt4ProFileNode *pro, profiles)
        paths << pro->path();

    if (id() == Core::Id(Constants::S60_DEVICE_TARGET_ID)) {
        foreach (ProjectExplorer::RunConfiguration *rc, runConfigurations())
            if (S60DeviceRunConfiguration *qt4rc = qobject_cast<S60DeviceRunConfiguration *>(rc))
                paths.remove(qt4rc->proFilePath());

        // Only add new runconfigurations if there are none.
        foreach (const QString &path, paths)
            addRunConfiguration(new S60DeviceRunConfiguration(this, path));
    }

    // Oh still none? Add a custom executable runconfiguration
    if (runConfigurations().isEmpty()) {
        addRunConfiguration(new QtSupport::CustomExecutableRunConfiguration(this));
    }
}

QList<ProjectExplorer::RunConfiguration *> Qt4SymbianTarget::runConfigurationsForNode(ProjectExplorer::Node *n)
{
    QList<ProjectExplorer::RunConfiguration *> result;
    foreach (ProjectExplorer::RunConfiguration *rc, runConfigurations()) {
        if (id() == Core::Id(Constants::S60_DEVICE_TARGET_ID)) {
            if (S60DeviceRunConfiguration *s60rc = qobject_cast<S60DeviceRunConfiguration *>(rc))
                if (s60rc->proFilePath() == n->path())
                    result << rc;
        }
    }
    return result;
}

ProjectExplorer::IDevice::ConstPtr Qt4SymbianTarget::currentDevice() const
{
    S60DeployConfiguration *dc = dynamic_cast<S60DeployConfiguration *>(activeDeployConfiguration());
    if (dc)
        return dc->device();
    return ProjectExplorer::IDevice::ConstPtr();
}
