/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberrydeviceconnectionmanager.h"

#include "blackberrydeviceconfiguration.h"
#include "blackberrydeviceconnection.h"
#include "qnxconstants.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <ssh/sshconnection.h>
#include <utils/qtcassert.h>

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryDeviceConnectionManager *BlackBerryDeviceConnectionManager::m_instance = 0;

BlackBerryDeviceConnectionManager::BlackBerryDeviceConnectionManager() :
    QObject()
{
}

BlackBerryDeviceConnectionManager::~BlackBerryDeviceConnectionManager()
{
    killAllConnections();
}

void BlackBerryDeviceConnectionManager::initialize()
{
    ProjectExplorer::DeviceManager *deviceManager = ProjectExplorer::DeviceManager::instance();
    connect(deviceManager, SIGNAL(deviceAdded(Core::Id)), this, SLOT(connectDevice(Core::Id)));
    connect(deviceManager, SIGNAL(deviceRemoved(Core::Id)), this, SLOT(disconnectDevice(Core::Id)));
    connect(deviceManager, SIGNAL(deviceListChanged()), this, SLOT(handleDeviceListChanged()));
}

void BlackBerryDeviceConnectionManager::killAllConnections()
{
    QList<BlackBerryDeviceConnection*> connections = m_connections.uniqueKeys();
    foreach (BlackBerryDeviceConnection *connection, connections) {
        connection->disconnect();
        connection->disconnectDevice();
        delete connection;
    }
}

BlackBerryDeviceConnectionManager *BlackBerryDeviceConnectionManager::instance()
{
    if (m_instance == 0)
        m_instance = new BlackBerryDeviceConnectionManager();
    return m_instance;
}

bool BlackBerryDeviceConnectionManager::isConnected(Core::Id deviceId)
{
    BlackBerryDeviceConnection *connection = m_connections.key(deviceId);
    if (!connection)
        return false;

    return connection->connectionState() == BlackBerryDeviceConnection::Connected;
}

QString BlackBerryDeviceConnectionManager::connectionLog(Core::Id deviceId) const
{
    BlackBerryDeviceConnection *connection = m_connections.key(deviceId);
    if (!connection)
        return QString();

    return connection->messageLog();
}

void BlackBerryDeviceConnectionManager::connectDevice(Core::Id deviceId)
{
    ProjectExplorer::IDevice::ConstPtr device =
            ProjectExplorer::DeviceManager::instance()->find(deviceId);
    if (device.isNull())
        return;

    connectDevice(device);
}

void BlackBerryDeviceConnectionManager::connectDevice(const ProjectExplorer::IDevice::ConstPtr &device)
{
    if (device->type() != Core::Id(Constants::QNX_BB_OS_TYPE))
        return;

    ProjectExplorer::DeviceManager::instance()->setDeviceState(device->id(),
                                                    ProjectExplorer::IDevice::DeviceStateUnknown);

    // Disconnect existing connection if it only belongs to this device,
    // and if the host has changed
    BlackBerryDeviceConnection *connection = m_connections.key(device->id());
    if (connection && connection->host() != device->sshParameters().host) {
        if (connectionUsageCount(device->id()) == 1)
            disconnectDevice(device);

        m_connections.remove(connection, device->id());
        connection = 0;
    }

    if (!connection)
        connection = connectionForHost(device->sshParameters().host);

    if (!connection) {
        connection = new BlackBerryDeviceConnection();
        m_connections.insertMulti(connection, device->id());

        connect(connection, SIGNAL(deviceConnected()), this, SLOT(handleDeviceConnected()));
        connect(connection, SIGNAL(deviceDisconnected()), this, SLOT(handleDeviceDisconnected()));
        connect(connection, SIGNAL(processOutput(QString)), this, SLOT(handleProcessOutput(QString)));
        connect(connection, SIGNAL(deviceAboutToConnect()), this, SLOT(handleDeviceAboutToConnect()));

        connection->connectDevice(device);
    } else {
        if (!m_connections.values(connection).contains(device->id()))
            m_connections.insertMulti(connection, device->id());

        switch (connection->connectionState()) {
        case BlackBerryDeviceConnection::Connected:
            ProjectExplorer::DeviceManager::instance()->setDeviceState(device->id(),
                                                    ProjectExplorer::IDevice::DeviceReadyToUse);
            break;
        case BlackBerryDeviceConnection::Connecting:
            ProjectExplorer::DeviceManager::instance()->setDeviceState(device->id(),
                                                    ProjectExplorer::IDevice::DeviceStateUnknown);
            break;
        case BlackBerryDeviceConnection::Disconnected:
            connection->connectDevice(device);
            break;
        }
    }
}

void BlackBerryDeviceConnectionManager::disconnectDevice(const ProjectExplorer::IDevice::ConstPtr &device)
{
    disconnectDevice(device->id());
}

void BlackBerryDeviceConnectionManager::disconnectDevice(Core::Id deviceId)
{
    BlackBerryDeviceConnection *connection = m_connections.key(deviceId);
    if (!connection)
        return;

    connection->disconnectDevice();
}

void BlackBerryDeviceConnectionManager::handleDeviceListChanged()
{
    disconnectRemovedDevices();
    reconnectChangedDevices();
    connectAddedDevices();
}

void BlackBerryDeviceConnectionManager::handleDeviceConnected()
{
    BlackBerryDeviceConnection *connection = qobject_cast<BlackBerryDeviceConnection*>(sender());
    QTC_ASSERT(connection, return);

    QList<Core::Id> knownDevices = m_connections.values(connection);
    foreach (Core::Id id, knownDevices)
        ProjectExplorer::DeviceManager::instance()->setDeviceState(id,
                                                    ProjectExplorer::IDevice::DeviceReadyToUse);

    QList<Core::Id> sameHostDevices = devicesForHost(connection->host());
    foreach (Core::Id id, sameHostDevices) {
        if (!knownDevices.contains(id)) {
            m_connections.insertMulti(connection, id);
            ProjectExplorer::DeviceManager::instance()->setDeviceState(id,
                                                    ProjectExplorer::IDevice::DeviceReadyToUse);
        }
    }
}

void BlackBerryDeviceConnectionManager::handleDeviceDisconnected()
{
    BlackBerryDeviceConnection *connection = qobject_cast<BlackBerryDeviceConnection*>(sender());
    QTC_ASSERT(connection, return);

    QList<Core::Id> disconnectedDevices = m_connections.values(connection);
    foreach (Core::Id id, disconnectedDevices)
        ProjectExplorer::DeviceManager::instance()->setDeviceState(id,
                                                    ProjectExplorer::IDevice::DeviceDisconnected);
}

void BlackBerryDeviceConnectionManager::handleDeviceAboutToConnect()
{
    BlackBerryDeviceConnection *connection = qobject_cast<BlackBerryDeviceConnection*>(sender());
    QTC_ASSERT(connection, return);

    QList<Core::Id> deviceIds = m_connections.values(connection);
    foreach (Core::Id deviceId, deviceIds)
        emit deviceAboutToConnect(deviceId);
}

void BlackBerryDeviceConnectionManager::handleProcessOutput(const QString &output)
{
    BlackBerryDeviceConnection *connection = qobject_cast<BlackBerryDeviceConnection*>(sender());
    QTC_ASSERT(connection, return);

    QList<Core::Id> deviceIds = m_connections.values(connection);
    foreach (Core::Id deviceId, deviceIds)
        emit connectionOutput(deviceId, output);
}

BlackBerryDeviceConnection *BlackBerryDeviceConnectionManager::connectionForHost(const QString &host) const
{
    QList<BlackBerryDeviceConnection*> connections = m_connections.uniqueKeys();

    foreach (BlackBerryDeviceConnection *connection, connections) {
        if (connection->host() == host)
            return connection;
    }

    return 0;
}

QList<Core::Id> BlackBerryDeviceConnectionManager::devicesForHost(const QString &host) const
{
    QList<Core::Id> result;
    ProjectExplorer::DeviceManager *deviceManager = ProjectExplorer::DeviceManager::instance();

    for (int i = 0; i < deviceManager->deviceCount(); ++i) {
        ProjectExplorer::IDevice::ConstPtr device = deviceManager->deviceAt(i);
        if (device->type() == Core::Id(Constants::QNX_BB_OS_TYPE)
                && device->sshParameters().host == host)
            result << device->id();
    }

    return result;
}

int BlackBerryDeviceConnectionManager::connectionUsageCount(Core::Id deviceId)
{
    BlackBerryDeviceConnection *connection = m_connections.key(deviceId);
    return m_connections.count(connection);
}

void BlackBerryDeviceConnectionManager::disconnectRemovedDevices()
{
    ProjectExplorer::DeviceManager *deviceManager = ProjectExplorer::DeviceManager::instance();

    QList<Core::Id> knownDevices = m_connections.values();
    foreach (Core::Id id, knownDevices) {
        ProjectExplorer::IDevice::ConstPtr device = deviceManager->find(id);
        if (device.isNull() && connectionUsageCount(id) <= 1)
            disconnectDevice(id);
    }
}

void BlackBerryDeviceConnectionManager::reconnectChangedDevices()
{
    ProjectExplorer::DeviceManager *deviceManager = ProjectExplorer::DeviceManager::instance();
    QList<Core::Id> connectedDevices = m_connections.values();

    for (int i = 0; i < deviceManager->deviceCount(); ++i) {
        ProjectExplorer::IDevice::ConstPtr device = deviceManager->deviceAt(i);
        if (!connectedDevices.contains(device->id()))
            continue;

        BlackBerryDeviceConnection *connection = m_connections.key(device->id());
        QTC_ASSERT(connection, continue);

        if (connection->host() == device->sshParameters().host)
            continue;

        if (connectionUsageCount(device->id()) <= 1)
            disconnectDevice(device->id());

        m_connections.remove(connection, device->id());
        connectDevice(device->id());
    }
}

void BlackBerryDeviceConnectionManager::connectAddedDevices()
{
    ProjectExplorer::DeviceManager *deviceManager = ProjectExplorer::DeviceManager::instance();

    QList<Core::Id> knownDevices = m_connections.values();
    for (int i = 0; i < deviceManager->deviceCount(); ++i) {
        Core::Id deviceId = deviceManager->deviceAt(i)->id();
        if (!knownDevices.contains(deviceId))
            connectDevice(deviceId);
    }
}
