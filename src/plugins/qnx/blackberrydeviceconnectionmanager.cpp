/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "blackberrydeviceconnectionmanager.h"

#include "blackberrydeviceconfiguration.h"
#include "blackberrydeviceconnection.h"
#include "blackberryconfigurationmanager.h"
#include "qnxconstants.h"

#include <coreplugin/icore.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <ssh/sshconnection.h>
#include <ssh/sshkeygenerator.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QDir>

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
    connect(deviceManager, SIGNAL(deviceListReplaced()), this, SLOT(handleDeviceListChanged()));
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

    // BlackBerry Device connection needs the Qnx environments to be set
    // in order to find the Connect.jar package.
    // Let's delay the device connections at startup till the Qnx settings are loaded.
    if (BlackBerryConfigurationManager::instance()->apiLevels().isEmpty()) {
        m_pendingDeviceConnections << device;
        connect(BlackBerryConfigurationManager::instance(), SIGNAL(settingsLoaded()),
                this, SLOT(processPendingDeviceConnections()), Qt::UniqueConnection);
        return;
    }

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

/*!
 * @brief Returns default private key path in local settings.
 * @return the default private key path
 */
const QString BlackBerryDeviceConnectionManager::privateKeyPath() const
{
    return Core::ICore::userResourcePath() + QLatin1String("/qnx/id_rsa");
}

/*!
 * @brief Checks validity of default SSH keys used for connecting to a device.
 * @return true, if the default SSH keys are valid
 */
bool BlackBerryDeviceConnectionManager::hasValidSSHKeys() const
{
    const QString privateKey = privateKeyPath();
    QFileInfo privateKeyFileInfo(privateKey);
    QFileInfo publicKeyFileInfo(privateKey + QLatin1String(".pub"));

    return privateKeyFileInfo.exists() && privateKeyFileInfo.isReadable()
            && publicKeyFileInfo.exists() && publicKeyFileInfo.isReadable();
}

/*!
 * @brief Stores a new private and public SSH key in local settings.
 * @param privateKeyContent the private key content
 * @param publicKeyContent the public key content
 */
bool BlackBerryDeviceConnectionManager::setSSHKeys(const QByteArray &privateKeyContent,
        const QByteArray &publicKeyContent, QString *error)
{
    const QString privateKey = privateKeyPath();
    const QString publicKey = privateKey + QLatin1String(".pub");

    QFileInfo fileInfo(privateKey);
    QDir dir = fileInfo.dir();
    if (!dir.exists())
        dir.mkpath(QLatin1String("."));

    Utils::FileSaver privSaver(privateKey);
    privSaver.write(privateKeyContent);
    if (!privSaver.finalize(error))
        return false;
    QFile::setPermissions(privateKey, QFile::ReadOwner | QFile::WriteOwner);

    Utils::FileSaver pubSaver(publicKey);
    pubSaver.write(publicKeyContent);
    if (!pubSaver.finalize(error))
        return false;

    return true;
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

    emit deviceConnected();
}

void BlackBerryDeviceConnectionManager::handleDeviceDisconnected()
{
    BlackBerryDeviceConnection *connection = qobject_cast<BlackBerryDeviceConnection*>(sender());
    QTC_ASSERT(connection, return);

    QList<Core::Id> disconnectedDevices = m_connections.values(connection);
    foreach (Core::Id id, disconnectedDevices) {
        ProjectExplorer::DeviceManager::instance()->setDeviceState(id,
                                                    ProjectExplorer::IDevice::DeviceDisconnected);
        emit deviceDisconnected(id);
    }
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

void BlackBerryDeviceConnectionManager::processPendingDeviceConnections()
{
    if (m_pendingDeviceConnections.isEmpty()
            || BlackBerryConfigurationManager::instance()->apiLevels().isEmpty())
        return;

    foreach (ProjectExplorer::IDevice::ConstPtr device, m_pendingDeviceConnections)
        connectDevice(device);

    m_pendingDeviceConnections.clear();
    disconnect(BlackBerryConfigurationManager::instance(), SIGNAL(settingsLoaded()),
            this, SLOT(processPendingDeviceConnections()));
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
