/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include "idevice.h"

#include "devicemanager.h"
#include "deviceprocesslist.h"

#include <coreplugin/id.h>
#include <ssh/sshconnection.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDesktopServices>

#include <QString>
#include <QUuid>

/*!
 * \class ProjectExplorer::IDevice
 * \brief This is the base class for all devices.
 *
 * The term "device" refers
 * here to some host to which e.g. files can be deployed or on which an application can run.
 * In the typical case, this would be some sort of embedded computer connected in some way to
 * the PC on which QtCreator runs. This class itself does not specify a connection protocol; that
 * kind of detail is to be added by subclasses.
 * Devices are managed by a \c DeviceManager.
 * \sa ProjectExplorer::DeviceManager
 */

/*!
 * \fn QString ProjectExplorer::IDevice::type() const
 * \brief Identifies the type of the device.
 * Devices with the same type share certain abilities.
 * This attribute is immutable.
 * \sa ProjectExplorer::IDeviceFactory
 */


/*!
 * \fn QString ProjectExplorer::IDevice::displayName() const
 * \brief A free-text name for the device to be displayed in GUI elements.
 */

/*!
 * \fn bool ProjectExplorer::IDevice::isAutoDetected() const
 * \brief True iff the device has been added via some sort of auto-detection mechanism.
 * Devices that are not auto-detected can only ever be created interactively from the
 * settings page.
 * This attribute is immutable.
 * \sa DeviceSettingsWidget
 */

/*!
 * \fn Core::Id ProjectExplorer::IDevice::id() const
 * \brief Identify the device.
 * If an id is given when constructing a device then this id is used. Otherwise a UUID is
 * generated and used to identity the device.
 * \sa ProjectExplorer::DeviceManager::findInactiveAutoDetectedDevice()
 */

/*!
 * \fn Core::Id ProjectExplorer::IDevice::invalidId()
 * \brief A value that no device can ever have as its internal id.
 */

/*!
 * \fn QString ProjectExplorer::IDevice::displayType() const
 * \brief Prints a representation of the device's type suitable for displaying to a user.
 */

/*!
 * \fn ProjectExplorer::IDeviceWidget *ProjectExplorer::IDevice::createWidget() const
 * \brief Creates a widget that displays device information not part of the IDevice base class.
 *        The widget can also be used to let the user change these attributes.
 */

/*!
 * \fn QStringList ProjectExplorer::IDevice::actionIds() const
 * \brief Returns a list of ids representing actions that can be run on this device.
 *        These actions will be available in the "Devices" options page.
 */

/*!
 * \fn QString ProjectExplorer::IDevice::displayNameForActionId(const QString &actionId) const
 * \brief A human-readable string for the given id. Will be displayed on a button which,
 *        when clicked, starts the respective action.
 */

/*!
 * \fn void ProjectExplorer::IDevice::executeAction(Core::Id actionId, QWidget *parent)
 * \brief Executes the respective action. This is typically done via some sort of dialog or
 *        wizard, so a parent widget argument is provided.
 */

/*!
 * \fn ProjectExplorer::IDevice::Ptr ProjectExplorer::IDevice::clone() const
 * \brief Creates an identical copy of a device object.
 */

/*!
 * \fn void fromMap(const QVariantMap &map)
 * \brief Restores a device object from a serialized state as written by \c toMap().
 * If subclasses override this to restore additional state, they must call the base class
 * implementation.
 */

/*!
 * \fn QVariantMap toMap() const
 * \brief Serializes a device object, e.g. to save it to a file.
 * If subclasses override this to save additional state, they must call the base class
 * implementation.
 */

static Core::Id newId()
{
    return Core::Id::fromString(QUuid::createUuid().toString());
}

namespace ProjectExplorer {

const char DisplayNameKey[] = "Name";
const char TypeKey[] = "OsType";
const char IdKey[] = "InternalId";
const char OriginKey[] = "Origin";
const char MachineTypeKey[] = "Type";

// Connection
const char HostKey[] = "Host";
const char SshPortKey[] = "SshPort";
const char PortsSpecKey[] = "FreePortsSpec";
const char UserNameKey[] = "Uname";
const char AuthKey[] = "Authentication";
const char KeyFileKey[] = "KeyFile";
const char PasswordKey[] = "Password";
const char TimeoutKey[] = "Timeout";

typedef QSsh::SshConnectionParameters::AuthenticationType AuthType;
const AuthType DefaultAuthType = QSsh::SshConnectionParameters::AuthenticationByKey;
const IDevice::MachineType DefaultMachineType = IDevice::Hardware;

const int DefaultTimeout = 10;

namespace Internal {
class IDevicePrivate
{
public:
    IDevicePrivate() :
        origin(IDevice::AutoDetected),
        deviceState(IDevice::DeviceStateUnknown),
        machineType(IDevice::Hardware)
    { }

    QString displayName;
    Core::Id type;
    IDevice::Origin origin;
    Core::Id id;
    IDevice::DeviceState deviceState;
    IDevice::MachineType machineType;

    QSsh::SshConnectionParameters sshParameters;
    Utils::PortList freePorts;
};
} // namespace Internal

PortsGatheringMethod::~PortsGatheringMethod() { }
DeviceProcessSupport::~DeviceProcessSupport() { }

IDevice::IDevice() : d(new Internal::IDevicePrivate)
{ }

IDevice::IDevice(Core::Id type, Origin origin, MachineType machineType, Core::Id id)
    : d(new Internal::IDevicePrivate)
{
    d->type = type;
    d->origin = origin;
    d->machineType = machineType;
    QTC_CHECK(origin == ManuallyAdded || id.isValid());
    d->id = id.isValid() ? id : newId();
}

IDevice::IDevice(const IDevice &other) : d(new Internal::IDevicePrivate)
{
    *d = *other.d;
}

IDevice::~IDevice()
{
    delete d;
}

QString IDevice::displayName() const
{
    return d->displayName;
}

void IDevice::setDisplayName(const QString &name)
{
    if (d->displayName == name)
        return;
    d->displayName = name;
}

IDevice::DeviceInfo IDevice::deviceInformation() const
{
    const QString key = QCoreApplication::translate("ProjectExplorer::IDevice", "Device");
    return DeviceInfo() << IDevice::DeviceInfoItem(key, deviceStateToString());
}

Core::Id IDevice::type() const
{
    return d->type;
}

bool IDevice::isAutoDetected() const
{
    return d->origin == AutoDetected;
}

Core::Id IDevice::id() const
{
    return d->id;
}

DeviceProcessSupport::Ptr IDevice::processSupport() const
{
    return DeviceProcessSupport::Ptr();
}

PortsGatheringMethod::Ptr IDevice::portsGatheringMethod() const
{
    return PortsGatheringMethod::Ptr();
}

DeviceProcessList *IDevice::createProcessListModel(QObject *parent) const
{
    Q_UNUSED(parent);
    QTC_ASSERT(false, qDebug("This should not have been called..."); return 0);
    return 0;
}

IDevice::DeviceState IDevice::deviceState() const
{
    return d->deviceState;
}

void IDevice::setDeviceState(const IDevice::DeviceState state)
{
    if (d->deviceState == state)
        return;
    d->deviceState = state;
}

Core::Id IDevice::typeFromMap(const QVariantMap &map)
{
    return Core::Id::fromSetting(map.value(QLatin1String(TypeKey)));
}

Core::Id IDevice::idFromMap(const QVariantMap &map)
{
    return Core::Id::fromSetting(map.value(QLatin1String(IdKey)));
}

void IDevice::fromMap(const QVariantMap &map)
{
    d->type = typeFromMap(map);
    d->displayName = map.value(QLatin1String(DisplayNameKey)).toString();
    d->id = Core::Id(map.value(QLatin1String(IdKey), newId().name()).toByteArray());
    d->origin = static_cast<Origin>(map.value(QLatin1String(OriginKey), ManuallyAdded).toInt());

    d->sshParameters.host = map.value(QLatin1String(HostKey)).toString();
    d->sshParameters.port = map.value(QLatin1String(SshPortKey), 22).toInt();
    d->sshParameters.userName = map.value(QLatin1String(UserNameKey)).toString();
    d->sshParameters.authenticationType
        = static_cast<AuthType>(map.value(QLatin1String(AuthKey), DefaultAuthType).toInt());
    d->sshParameters.password = map.value(QLatin1String(PasswordKey)).toString();
    d->sshParameters.privateKeyFile = map.value(QLatin1String(KeyFileKey), defaultPrivateKeyFilePath()).toString();
    d->sshParameters.timeout = map.value(QLatin1String(TimeoutKey), DefaultTimeout).toInt();

    d->freePorts = Utils::PortList::fromString(map.value(QLatin1String(PortsSpecKey),
        QLatin1String("10000-10100")).toString());
    d->machineType = static_cast<MachineType>(map.value(QLatin1String(MachineTypeKey), DefaultMachineType).toInt());
}

QVariantMap IDevice::toMap() const
{
    QVariantMap map;
    map.insert(QLatin1String(DisplayNameKey), d->displayName);
    map.insert(QLatin1String(TypeKey), d->type.toString());
    map.insert(QLatin1String(IdKey), d->id.name());
    map.insert(QLatin1String(OriginKey), d->origin);

    map.insert(QLatin1String(MachineTypeKey), d->machineType);
    map.insert(QLatin1String(HostKey), d->sshParameters.host);
    map.insert(QLatin1String(SshPortKey), d->sshParameters.port);
    map.insert(QLatin1String(UserNameKey), d->sshParameters.userName);
    map.insert(QLatin1String(AuthKey), d->sshParameters.authenticationType);
    map.insert(QLatin1String(PasswordKey), d->sshParameters.password);
    map.insert(QLatin1String(KeyFileKey), d->sshParameters.privateKeyFile);
    map.insert(QLatin1String(TimeoutKey), d->sshParameters.timeout);

    map.insert(QLatin1String(PortsSpecKey), d->freePorts.toString());

    return map;
}

IDevice::Ptr IDevice::sharedFromThis()
{
    return DeviceManager::instance()->fromRawPointer(this);
}

IDevice::ConstPtr IDevice::sharedFromThis() const
{
    return DeviceManager::instance()->fromRawPointer(this);
}

QString IDevice::deviceStateToString() const
{
    const char context[] = "ProjectExplorer::IDevice";
    switch (d->deviceState) {
    case IDevice::DeviceReadyToUse: return QCoreApplication::translate(context, "Ready to use");
    case IDevice::DeviceConnected: return QCoreApplication::translate(context, "Connected");
    case IDevice::DeviceDisconnected: return QCoreApplication::translate(context, "Disconnected");
    case IDevice::DeviceStateUnknown: return QCoreApplication::translate(context, "Unknown");
    default: return QCoreApplication::translate(context, "Invalid");
    }
}

QSsh::SshConnectionParameters IDevice::sshParameters() const
{
    return d->sshParameters;
}

void IDevice::setSshParameters(const QSsh::SshConnectionParameters &sshParameters)
{
    d->sshParameters = sshParameters;
}

void IDevice::setFreePorts(const Utils::PortList &freePorts)
{
    d->freePorts = freePorts;
}

Utils::PortList IDevice::freePorts() const
{
    return d->freePorts;
}

IDevice::MachineType IDevice::machineType() const
{
    return d->machineType;
}

QString IDevice::defaultPrivateKeyFilePath()
{
    return QDesktopServices::storageLocation(QDesktopServices::HomeLocation)
        + QLatin1String("/.ssh/id_rsa");
}

QString IDevice::defaultPublicKeyFilePath()
{
    return defaultPrivateKeyFilePath() + QLatin1String(".pub");
}

} // namespace ProjectExplorer
