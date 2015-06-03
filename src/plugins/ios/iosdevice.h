/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
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
#ifndef IOSDEVICE_H
#define IOSDEVICE_H

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
    quint16 nextPort() const;
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
public slots:
    void updateInfo(const QString &devId);
//private slots:
    void deviceInfo(Ios::IosToolHandler *gatherer, const QString &deviceId,
                    const Ios::IosToolHandler::Dict &info);
    void infoGathererFinished(Ios::IosToolHandler *gatherer);
    void monitorAvailableDevices();
private slots:
    void updateUserModeDevices();
private:
    IosDeviceManager(QObject *parent = 0);
    QTimer m_userModeDevicesTimer;
    QStringList m_userModeDeviceIds;
};

namespace IosKitInformation {
IosDevice::ConstPtr device(ProjectExplorer::Kit *);
}

} // namespace Internal

} // namespace Ios

#endif // IOSDEVICE_H
