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
#ifndef ABSTRACTLINUXDEVICEDEPLOYSTEP_H
#define ABSTRACTLINUXDEVICEDEPLOYSTEP_H

#include <QtCore/QCoreApplication>
#include <QtCore/QSharedPointer>

namespace ProjectExplorer { class DeployConfiguration; }

namespace RemoteLinux {
class LinuxDeviceConfiguration;
class Qt4MaemoDeployConfiguration;

namespace Internal {

class AbstractLinuxDeviceDeployStep
{
    Q_DECLARE_TR_FUNCTIONS(AbstractLinuxDeviceDeployStep)
public:
    virtual ~AbstractLinuxDeviceDeployStep();

    Qt4MaemoDeployConfiguration *maemoDeployConfig() const { return m_deployConfiguration; }
    bool isDeploymentPossible(QString &whyNot) const;

protected:
    AbstractLinuxDeviceDeployStep(ProjectExplorer::DeployConfiguration *dc);

    QSharedPointer<const LinuxDeviceConfiguration> deviceConfiguration() const;
    bool initialize(QString &errorMsg);

private:
    virtual bool isDeploymentPossibleInternal(QString &whynot) const=0;

    Qt4MaemoDeployConfiguration * const m_deployConfiguration;
    mutable QSharedPointer<const LinuxDeviceConfiguration> m_deviceConfiguration;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // ABSTRACTLINUXDEVICEDEPLOYSTEP_H
