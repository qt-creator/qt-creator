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

#include "idevice.h"

#include "devicemanager.h"
#include "deviceprocesslist.h"

#include "../kit.h"
#include "../kitinformation.h"
#include "../runconfiguration.h"
#include "../runnables.h"

#include <ssh/sshconnection.h>
#include <utils/icon.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <utils/url.h>

#include <QCoreApplication>
#include <QStandardPaths>

#include <QString>
#include <QUuid>

/*!
 * \class ProjectExplorer::IDevice
 * \brief The IDevice class is the base class for all devices.
 *
 * The term \e device refers to some host to which files can be deployed or on
 * which an application can run, for example.
 * In the typical case, this would be some sort of embedded computer connected in some way to
 * the PC on which \QC runs. This class itself does not specify a connection
 * protocol; that
 * kind of detail is to be added by subclasses.
 * Devices are managed by a \c DeviceManager.
 * \sa ProjectExplorer::DeviceManager
 */

/*!
 * \fn Core::Id ProjectExplorer::IDevice::invalidId()
 * A value that no device can ever have as its internal id.
 */

/*!
 * \fn QString ProjectExplorer::IDevice::displayType() const
 * Prints a representation of the device's type suitable for displaying to a
 * user.
 */

/*!
 * \fn ProjectExplorer::IDeviceWidget *ProjectExplorer::IDevice::createWidget()
 * Creates a widget that displays device information not part of the IDevice base class.
 *        The widget can also be used to let the user change these attributes.
 */

/*!
 * \fn QStringList ProjectExplorer::IDevice::actionIds() const
 * Returns a list of ids representing actions that can be run on this device.
 * These actions will be available in the \gui Devices options page.
 */

/*!
 * \fn QString ProjectExplorer::IDevice::displayNameForActionId(Core::Id actionId) const
 * A human-readable string for \a actionId. Will be displayed on a button which,
 *        when clicked, starts the respective action.
 */

/*!
 * \fn void ProjectExplorer::IDevice::executeAction(Core::Id actionId, QWidget *parent) const
 * Executes the action specified by \a actionId. This is typically done via some
 * sort of dialog or wizard, so \a parent widget is provided.
 */

/*!
 * \fn ProjectExplorer::IDevice::Ptr ProjectExplorer::IDevice::clone() const
 * Creates an identical copy of a device object.
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
const char VersionKey[] = "Version";

// Connection
const char HostKey[] = "Host";
const char SshPortKey[] = "SshPort";
const char PortsSpecKey[] = "FreePortsSpec";
const char UserNameKey[] = "Uname";
const char AuthKey[] = "Authentication";
const char KeyFileKey[] = "KeyFile";
const char PasswordKey[] = "Password";
const char TimeoutKey[] = "Timeout";
const char HostKeyCheckingKey[] = "HostKeyChecking";
const char SshOptionsKey[] = "SshOptions";

const char DebugServerKey[] = "DebugServerKey";

typedef QSsh::SshConnectionParameters::AuthenticationType AuthType;
const AuthType DefaultAuthType = QSsh::SshConnectionParameters::AuthenticationTypePublicKey;
const IDevice::MachineType DefaultMachineType = IDevice::Hardware;

const int DefaultTimeout = 10;

namespace Internal {
class IDevicePrivate
{
public:
    IDevicePrivate() :
        origin(IDevice::AutoDetected),
        deviceState(IDevice::DeviceStateUnknown),
        machineType(IDevice::Hardware),
        version(0)
    { }

    QString displayName;
    Core::Id type;
    IDevice::Origin origin;
    Core::Id id;
    IDevice::DeviceState deviceState;
    IDevice::MachineType machineType;
    int version; // This is used by devices that have been added by the SDK.

    QSsh::SshConnectionParameters sshParameters;
    Utils::PortList freePorts;
    QString debugServerPath;

    QList<Utils::Icon> deviceIcons;
};
} // namespace Internal

DeviceTester::DeviceTester(QObject *parent) : QObject(parent) { }

IDevice::IDevice() : d(new Internal::IDevicePrivate)
{
    d->sshParameters.hostKeyDatabase = DeviceManager::instance()->hostKeyDatabase();
}

IDevice::IDevice(Core::Id type, Origin origin, MachineType machineType, Core::Id id)
    : d(new Internal::IDevicePrivate)
{
    d->type = type;
    d->origin = origin;
    d->machineType = machineType;
    QTC_CHECK(origin == ManuallyAdded || id.isValid());
    d->id = id.isValid() ? id : newId();
    d->sshParameters.hostKeyDatabase = DeviceManager::instance()->hostKeyDatabase();
}

IDevice::IDevice(const IDevice &other)
    : QEnableSharedFromThis<IDevice>(other)
    , d(new Internal::IDevicePrivate)
{
    *d = *other.d;
}

IDevice::~IDevice()
{
    delete d;
}

/*!
    Specifies a free-text name for the device to be displayed in GUI elements.
*/

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

/*!
    Identifies the type of the device. Devices with the same type share certain
    abilities. This attribute is immutable.

    \sa ProjectExplorer::IDeviceFactory
 */

Core::Id IDevice::type() const
{
    return d->type;
}

/*!
    Returns \c true if the device has been added via some sort of auto-detection
    mechanism. Devices that are not auto-detected can only ever be created
    interactively from the \gui Options page. This attribute is immutable.

    \sa DeviceSettingsWidget
*/

bool IDevice::isAutoDetected() const
{
    return d->origin == AutoDetected;
}

/*!
    Identifies the device. If an id is given when constructing a device then
    this id is used. Otherwise, a UUID is generated and used to identity the
    device.

    \sa ProjectExplorer::DeviceManager::findInactiveAutoDetectedDevice()
*/

Core::Id IDevice::id() const
{
    return d->id;
}

/*!
    Tests whether a device can be compatible with the given kit. The default
    implementation will match the device type specified in the kit against
    the device's own type.
*/
bool IDevice::isCompatibleWith(const Kit *k) const
{
    return DeviceTypeKitInformation::deviceTypeId(k) == type();
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

DeviceTester *IDevice::createDeviceTester() const
{
    QTC_ASSERT(false, qDebug("This should not have been called..."));
    return 0;
}

Utils::OsType IDevice::osType() const
{
    return Utils::OsTypeOther;
}

DeviceProcess *IDevice::createProcess(QObject * /* parent */) const
{
    QTC_CHECK(false);
    return 0;
}

DeviceEnvironmentFetcher::Ptr IDevice::environmentFetcher() const
{
    return DeviceEnvironmentFetcher::Ptr();
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

/*!
    Restores a device object from a serialized state as written by toMap().
    If subclasses override this to restore additional state, they must call the
    base class implementation.
*/

void IDevice::fromMap(const QVariantMap &map)
{
    d->type = typeFromMap(map);
    d->displayName = map.value(QLatin1String(DisplayNameKey)).toString();
    d->id = Core::Id::fromSetting(map.value(QLatin1String(IdKey)));
    if (!d->id.isValid())
        d->id = newId();
    d->origin = static_cast<Origin>(map.value(QLatin1String(OriginKey), ManuallyAdded).toInt());

    d->sshParameters.host = map.value(QLatin1String(HostKey)).toString();
    d->sshParameters.port = map.value(QLatin1String(SshPortKey), 22).toInt();
    d->sshParameters.userName = map.value(QLatin1String(UserNameKey)).toString();
    d->sshParameters.authenticationType
        = static_cast<AuthType>(map.value(QLatin1String(AuthKey), DefaultAuthType).toInt());
    d->sshParameters.password = map.value(QLatin1String(PasswordKey)).toString();
    d->sshParameters.privateKeyFile = map.value(QLatin1String(KeyFileKey), defaultPrivateKeyFilePath()).toString();
    d->sshParameters.timeout = map.value(QLatin1String(TimeoutKey), DefaultTimeout).toInt();
    d->sshParameters.hostKeyCheckingMode = static_cast<QSsh::SshHostKeyCheckingMode>
            (map.value(QLatin1String(HostKeyCheckingKey), QSsh::SshHostKeyCheckingNone).toInt());
    const QVariant optionsVariant = map.value(QLatin1String(SshOptionsKey));
    if (optionsVariant.isValid())  // false for QtC < 3.4
        d->sshParameters.options = QSsh::SshConnectionOptions(optionsVariant.toInt());

    QString portsSpec = map.value(PortsSpecKey).toString();
    if (portsSpec.isEmpty())
        portsSpec = "10000-10100";
    d->freePorts = Utils::PortList::fromString(portsSpec);
    d->machineType = static_cast<MachineType>(map.value(QLatin1String(MachineTypeKey), DefaultMachineType).toInt());
    d->version = map.value(QLatin1String(VersionKey), 0).toInt();

    d->debugServerPath = map.value(QLatin1String(DebugServerKey)).toString();
}

/*!
    Serializes a device object, for example to save it to a file.
    If subclasses override this function to save additional state, they must
    call the base class implementation.
*/

QVariantMap IDevice::toMap() const
{
    QVariantMap map;
    map.insert(QLatin1String(DisplayNameKey), d->displayName);
    map.insert(QLatin1String(TypeKey), d->type.toString());
    map.insert(QLatin1String(IdKey), d->id.toSetting());
    map.insert(QLatin1String(OriginKey), d->origin);

    map.insert(QLatin1String(MachineTypeKey), d->machineType);
    map.insert(QLatin1String(HostKey), d->sshParameters.host);
    map.insert(QLatin1String(SshPortKey), d->sshParameters.port);
    map.insert(QLatin1String(UserNameKey), d->sshParameters.userName);
    map.insert(QLatin1String(AuthKey), d->sshParameters.authenticationType);
    map.insert(QLatin1String(PasswordKey), d->sshParameters.password);
    map.insert(QLatin1String(KeyFileKey), d->sshParameters.privateKeyFile);
    map.insert(QLatin1String(TimeoutKey), d->sshParameters.timeout);
    map.insert(QLatin1String(HostKeyCheckingKey), d->sshParameters.hostKeyCheckingMode);
    map.insert(QLatin1String(SshOptionsKey), static_cast<int>(d->sshParameters.options));

    map.insert(QLatin1String(PortsSpecKey), d->freePorts.toString());
    map.insert(QLatin1String(VersionKey), d->version);

    map.insert(QLatin1String(DebugServerKey), d->debugServerPath);

    return map;
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
    d->sshParameters.hostKeyDatabase = DeviceManager::instance()->hostKeyDatabase();
}

QUrl IDevice::toolControlChannel(const ControlChannelHint &) const
{
    QUrl url;
    url.setScheme(Utils::urlTcpScheme());
    url.setHost(d->sshParameters.host);
    return url;
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

QString IDevice::debugServerPath() const
{
    return d->debugServerPath;
}

void IDevice::setDebugServerPath(const QString &path)
{
    d->debugServerPath = path;
}

int IDevice::version() const
{
    return d->version;
}

QString IDevice::defaultPrivateKeyFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
        + QLatin1String("/.ssh/id_rsa");
}

QString IDevice::defaultPublicKeyFilePath()
{
    return defaultPrivateKeyFilePath() + QLatin1String(".pub");
}

void DeviceProcessSignalOperation::setDebuggerCommand(const QString &cmd)
{
    m_debuggerCommand = cmd;
}

DeviceProcessSignalOperation::DeviceProcessSignalOperation()
{
}

DeviceEnvironmentFetcher::DeviceEnvironmentFetcher()
{
}

} // namespace ProjectExplorer
