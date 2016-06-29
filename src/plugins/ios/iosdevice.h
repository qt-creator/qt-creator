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

#pragma once

#include "iostoolhandler.h"

#include <projectexplorer/devicesupport/idevice.h>

#include <QVariantMap>
#include <QMap>
#include <QString>
#include <QSharedPointer>
#include <QStringList>
#include <QTimer>

namespace ProjectExplorer{
class Kit;
}
namespace Ios {
class IosConfigurations;

namespace Internal {
class IosDeviceManager;

class IosDevice : public ProjectExplorer::IDevice
{
public:
    typedef QMap<QString, QString> Dict;
    typedef QSharedPointer<const IosDevice> ConstPtr;
    typedef QSharedPointer<IosDevice> Ptr;

    ProjectExplorer::IDevice::DeviceInfo deviceInformation() const override;
    ProjectExplorer::IDeviceWidget *createWidget() override;
    QList<Core::Id> actionIds() const override;
    QString displayNameForActionId(Core::Id actionId) const override;
    void executeAction(Core::Id actionId, QWidget *parent = 0) override;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const override;
    QString displayType() const override;

    ProjectExplorer::IDevice::Ptr clone() const override;
    void fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;
    QString uniqueDeviceID() const;
    IosDevice(const QString &uid);
    QString osVersion() const;
    Utils::Port nextPort() const;
    bool canAutoDetectPorts() const override;

    static QString name();

protected:
    friend class IosDeviceFactory;
    friend class Ios::Internal::IosDeviceManager;
    IosDevice();
    IosDevice(const IosDevice &other);
    Dict m_extraInfo;
    bool m_ignoreDevice;
    mutable quint16 m_lastPort;
};

class IosDeviceManager : public QObject {
    Q_OBJECT
public:
    typedef QHash<QString, QString> TranslationMap;

    static TranslationMap translationMap();
    static IosDeviceManager *instance();

    void updateAvailableDevices(const QStringList &devices);
    void deviceConnected(const QString &uid, const QString &name = QString());
    void deviceDisconnected(const QString &uid);
    friend class IosConfigurations;
    void updateInfo(const QString &devId);
    void deviceInfo(Ios::IosToolHandler *gatherer, const QString &deviceId,
                    const Ios::IosToolHandler::Dict &info);
    void infoGathererFinished(Ios::IosToolHandler *gatherer);
    void monitorAvailableDevices();
private:
    void updateUserModeDevices();
    IosDeviceManager(QObject *parent = 0);
    QTimer m_userModeDevicesTimer;
    QStringList m_userModeDeviceIds;
};

namespace IosKitInformation {
IosDevice::ConstPtr device(ProjectExplorer::Kit *);
}

} // namespace Internal

} // namespace Ios
