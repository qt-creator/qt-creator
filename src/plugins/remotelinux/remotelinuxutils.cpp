/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#include "remotelinuxutils.h"

#include "linuxdeviceconfiguration.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/abi.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4target.h>
#include <qtsupport/baseqtversion.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QList>
#include <QtCore/QString>

using namespace ExtensionSystem;
using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace QtSupport;

namespace RemoteLinux {
namespace Internal {
namespace {

bool isUnixQt(const BaseQtVersion *qtVersion)
{
    if (!qtVersion)
        return false;
    const QList<Abi> &abis = qtVersion->qtAbis();
    foreach (const Abi &abi, abis) {
        switch (abi.os()) {
        case Abi::UnixOS: case Abi::BsdOS: case Abi::LinuxOS: case Abi::MacOS: return true;
        default: continue;
        }
    }
    return false;
}

} // anonymous namespace
} // namespace Internal

bool RemoteLinuxUtils::hasUnixQt(const Target *target)
{
    const Qt4BaseTarget * const qtTarget = qobject_cast<const Qt4BaseTarget *>(target);
    if (!qtTarget)
        return false;
    const Qt4BuildConfiguration * const bc = qtTarget->activeQt4BuildConfiguration();
    return bc && Internal::isUnixQt(bc->qtVersion());
}

QString RemoteLinuxUtils::osTypeToString(const QString &osType)
{
    const QList<ILinuxDeviceConfigurationFactory *> &factories
        = PluginManager::instance()->getObjects<ILinuxDeviceConfigurationFactory>();
    foreach (const ILinuxDeviceConfigurationFactory * const factory, factories) {
        if (factory->supportsOsType(osType))
            return factory->displayNameForOsType(osType);
    }
    return QCoreApplication::translate("RemoteLinux", "Unknown OS");
}

QString RemoteLinuxUtils::deviceConfigurationName(const LinuxDeviceConfiguration::ConstPtr &devConf)
{
    return devConf ? devConf->name() : QCoreApplication::translate("RemoteLinux", "(No device)");
}

} // namespace RemoteLinux
