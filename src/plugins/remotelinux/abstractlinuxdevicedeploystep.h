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

#include "maemodeviceconfigurations.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QSharedPointer>
#include <QtCore/QVariantMap>

namespace ProjectExplorer { class DeployConfiguration; }

namespace RemoteLinux {
namespace Internal {
class Qt4MaemoDeployConfiguration;


class LinuxDeviceDeployStepHelper : public QObject
{
    Q_OBJECT
public:
    LinuxDeviceDeployStepHelper(Qt4MaemoDeployConfiguration *dc);
    ~LinuxDeviceDeployStepHelper();

    QSharedPointer<const MaemoDeviceConfig> deviceConfig() const { return m_deviceConfig; }
    QSharedPointer<const MaemoDeviceConfig> cachedDeviceConfig() const { return m_cachedDeviceConfig; }
    Qt4MaemoDeployConfiguration *deployConfiguration() const { return m_deployConfiguration; }

    void setDeviceConfig(int i);
    void prepareDeployment() { m_cachedDeviceConfig = m_deviceConfig; }

    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &map);

signals:
    void deviceConfigChanged();

private:
    void setDeviceConfig(MaemoDeviceConfig::Id internalId);
    Q_SLOT void handleDeviceConfigurationsUpdated();

    QSharedPointer<const MaemoDeviceConfig> m_deviceConfig;
    QSharedPointer<const MaemoDeviceConfig> m_cachedDeviceConfig;
    Qt4MaemoDeployConfiguration * const m_deployConfiguration;
};

class AbstractLinuxDeviceDeployStep
{
    Q_DECLARE_TR_FUNCTIONS(AbstractLinuxDeviceDeployStep)
public:
    virtual ~AbstractLinuxDeviceDeployStep() {}

    Qt4MaemoDeployConfiguration *maemoDeployConfig() const { return m_helper.deployConfiguration(); }
    bool isDeploymentPossible(QString &whyNot) const;
    LinuxDeviceDeployStepHelper &helper() { return m_helper; }
    const LinuxDeviceDeployStepHelper &helper() const { return m_helper; }

protected:
    AbstractLinuxDeviceDeployStep(ProjectExplorer::DeployConfiguration *dc);

    bool initialize(QString &errorMsg);

private:
    virtual bool isDeploymentPossibleInternal(QString &whynot) const=0;

    LinuxDeviceDeployStepHelper m_helper;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // ABSTRACTLINUXDEVICEDEPLOYSTEP_H
