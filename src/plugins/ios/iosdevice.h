/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef IOSDEVICE_H
#define IOSDEVICE_H

#include "iostoolhandler.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <utils/qtcoverride.h>

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

    ProjectExplorer::IDevice::DeviceInfo deviceInformation() const QTC_OVERRIDE;
    ProjectExplorer::IDeviceWidget *createWidget() QTC_OVERRIDE;
    QList<Core::Id> actionIds() const QTC_OVERRIDE;
    QString displayNameForActionId(Core::Id actionId) const QTC_OVERRIDE;
    void executeAction(Core::Id actionId, QWidget *parent = 0) QTC_OVERRIDE;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const QTC_OVERRIDE;
    QString displayType() const QTC_OVERRIDE;

    ProjectExplorer::IDevice::Ptr clone() const QTC_OVERRIDE;
    void fromMap(const QVariantMap &map) QTC_OVERRIDE;
    QVariantMap toMap() const QTC_OVERRIDE;
    QString uniqueDeviceID() const;
    IosDevice(const QString &uid);
    QString osVersion() const;
    quint16 nextPort() const;
    bool canAutoDetectPorts() const QTC_OVERRIDE;

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
