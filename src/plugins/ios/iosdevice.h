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
#include <projectexplorer/devicesupport/idevicefactory.h>

#include <QVariantMap>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QTimer>

namespace Ios {
class IosConfigurations;

namespace Internal {
class IosDeviceManager;

class IosDevice final : public ProjectExplorer::IDevice
{
    Q_DECLARE_TR_FUNCTIONS(Ios::Internal::IosDevice)

public:
    using Dict = QMap<QString, QString>;
    using ConstPtr = QSharedPointer<const IosDevice>;
    using Ptr = QSharedPointer<IosDevice>;

    ProjectExplorer::IDevice::DeviceInfo deviceInformation() const override;
    ProjectExplorer::IDeviceWidget *createWidget() override;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const override;

    void fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;
    QString uniqueDeviceID() const;
    QString osVersion() const;
    Utils::Port nextPort() const;
    bool canAutoDetectPorts() const override;

    static QString name();

protected:
    friend class IosDeviceFactory;
    friend class Ios::Internal::IosDeviceManager;
    IosDevice();
    IosDevice(const QString &uid);

    enum CtorHelper {};
    IosDevice(CtorHelper);

    Dict m_extraInfo;
    bool m_ignoreDevice = false;
    mutable quint16 m_lastPort;
};

class IosDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    IosDeviceFactory();

    bool canRestore(const QVariantMap &map) const override;
};

class IosDeviceManager : public QObject
{
    Q_OBJECT
public:
    using TranslationMap = QHash<QString, QString>;

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
    IosDeviceManager(QObject *parent = nullptr);
    QTimer m_userModeDevicesTimer;
    QStringList m_userModeDeviceIds;
};

} // namespace Internal
} // namespace Ios
