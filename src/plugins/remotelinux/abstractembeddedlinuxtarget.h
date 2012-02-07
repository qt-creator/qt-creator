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
#ifndef ABSTRACTEMBEDDEDLINUXTARGET_H
#define ABSTRACTEMBEDDEDLINUXTARGET_H

#include "remotelinux_export.h"

#include <qt4projectmanager/qt4target.h>

#include <QString>

namespace ProjectExplorer { class IBuildConfigurationFactory; }
namespace Qt4ProjectManager { class Qt4BuildConfigurationFactory; }

namespace RemoteLinux {
class DeploymentInfo;
namespace Internal { class TypeSpecificDeviceConfigurationListModel; }

class REMOTELINUX_EXPORT AbstractEmbeddedLinuxTarget : public Qt4ProjectManager::Qt4BaseTarget
{
    Q_OBJECT
public:
    AbstractEmbeddedLinuxTarget(Qt4ProjectManager::Qt4Project *parent, const QString &id,
        const QString &supportedOsType);

    ProjectExplorer::IBuildConfigurationFactory *buildConfigurationFactory() const;

    QString supportedOsType() const { return m_supportedOsType; }
    DeploymentInfo *deploymentInfo() const { return m_deploymentInfo; }
    Internal::TypeSpecificDeviceConfigurationListModel *deviceConfigModel() const {
        return m_deviceConfigModel;
    }

private:
    Qt4ProjectManager::Qt4BuildConfigurationFactory * const m_buildConfigurationFactory;
    const QString m_supportedOsType;
    DeploymentInfo * const m_deploymentInfo;
    Internal::TypeSpecificDeviceConfigurationListModel * const m_deviceConfigModel;
};

} // namespace RemoteLinux

#endif // ABSTRACTEMBEDDEDLINUXTARGET_H
