// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "idevice.h"

#include "devicemanager.h"
#include "deviceprocesslist.h"
#include "idevicefactory.h"
#include "sshparameters.h"

#include "../kit.h"
#include "../kitinformation.h"
#include "../projectexplorertr.h"
#include "../target.h"

#include <coreplugin/icore.h>

#include <utils/devicefileaccess.h>
#include <utils/displayname.h>
#include <utils/icon.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <utils/url.h>

#include <QCoreApplication>
#include <QStandardPaths>

#include <QDateTime>
#include <QReadWriteLock>
#include <QString>
#include <QUuid>

/*!
 * \class ProjectExplorer::IDevice::DeviceAction
 * \brief The DeviceAction class describes an action that can be run on a device.
 *
 * The description consists of a human-readable string that will be displayed
 * on a button which, when clicked, executes a functor, and the functor itself.
 * This is typically some sort of dialog or wizard, so \a parent widget is provided.
 */

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
 * \fn Utils::Id ProjectExplorer::IDevice::invalidId()
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
 * \fn void ProjectExplorer::IDevice::addDeviceAction(const DeviceAction &deviceAction)
 * Adds an actions that can be run on this device.
 * These actions will be available in the \gui Devices options page.
 */

/*!
 * \fn ProjectExplorer::IDevice::Ptr ProjectExplorer::IDevice::clone() const
 * Creates an identical copy of a device object.
 */

using namespace Utils;

namespace ProjectExplorer {

static Id newId()
{
    return Id::fromString(QUuid::createUuid().toString());
}

const char DisplayNameKey[] = "Name";
const char TypeKey[] = "OsType";
const char IdKey[] = "InternalId";
const char OriginKey[] = "Origin";
const char MachineTypeKey[] = "Type";
const char VersionKey[] = "Version";
const char ExtraDataKey[] = "ExtraData";

// Connection
const char HostKey[] = "Host";
const char SshPortKey[] = "SshPort";
const char PortsSpecKey[] = "FreePortsSpec";
const char UserNameKey[] = "Uname";
const char AuthKey[] = "Authentication";
const char KeyFileKey[] = "KeyFile";
const char TimeoutKey[] = "Timeout";
const char HostKeyCheckingKey[] = "HostKeyChecking";

const char DebugServerKey[] = "DebugServerKey";
const char QmlRuntimeKey[] = "QmlsceneKey";

using AuthType = SshParameters::AuthenticationType;
const AuthType DefaultAuthType = SshParameters::AuthenticationTypeAll;
const IDevice::MachineType DefaultMachineType = IDevice::Hardware;

const int DefaultTimeout = 10;

namespace Internal {
class IDevicePrivate
{
public:
    IDevicePrivate() = default;

    DisplayName displayName;
    QString displayType;
    Id type;
    IDevice::Origin origin = IDevice::AutoDetected;
    Id id;
    IDevice::DeviceState deviceState = IDevice::DeviceStateUnknown;
    IDevice::MachineType machineType = IDevice::Hardware;
    OsType osType = OsTypeOther;
    DeviceFileAccess *fileAccess = nullptr;
    int version = 0; // This is used by devices that have been added by the SDK.

    QReadWriteLock lock; // Currently used to protect sshParameters only
    SshParameters sshParameters;
    PortList freePorts;
    FilePath debugServerPath;
    FilePath debugDumperPath = Core::ICore::resourcePath("debugger/");
    FilePath qmlRunCommand;
    bool emptyCommandAllowed = false;

    QList<Icon> deviceIcons;
    QList<IDevice::DeviceAction> deviceActions;
    QVariantMap extraData;
    IDevice::OpenTerminal openTerminal;
};
} // namespace Internal

DeviceTester::DeviceTester(QObject *parent) : QObject(parent) { }

IDevice::IDevice() : d(new Internal::IDevicePrivate)
{
}

IDevice::~IDevice() = default;

void IDevice::setOpenTerminal(const IDevice::OpenTerminal &openTerminal)
{
    d->openTerminal = openTerminal;
}

void IDevice::setupId(Origin origin, Id id)
{
    d->origin = origin;
    QTC_CHECK(origin == ManuallyAdded || id.isValid());
    d->id = id.isValid() ? id : newId();
}

bool IDevice::canOpenTerminal() const
{
    return bool(d->openTerminal);
}

void IDevice::openTerminal(const Environment &env, const FilePath &workingDir) const
{
    QTC_ASSERT(canOpenTerminal(), return);
    d->openTerminal(env, workingDir);
}

bool IDevice::isEmptyCommandAllowed() const
{
    return d->emptyCommandAllowed;
}

void IDevice::setAllowEmptyCommand(bool allow)
{
    d->emptyCommandAllowed = allow;
}

bool IDevice::isAnyUnixDevice() const
{
    return d->osType == OsTypeLinux || d->osType == OsTypeMac || d->osType == OsTypeOtherUnix;
}

DeviceFileAccess *IDevice::fileAccess() const
{
    return d->fileAccess;
}

FilePath IDevice::filePath(const QString &pathOnDevice) const
{
    // match DeviceManager::deviceForPath
    return FilePath::fromParts(u"device", id().toString(), pathOnDevice);
}

bool IDevice::handlesFile(const FilePath &filePath) const
{
    if (filePath.scheme() == u"device" && filePath.host() == id().toString())
        return true;
    return false;
}

FilePath IDevice::searchExecutableInPath(const QString &fileName) const
{
    FilePaths paths;
    for (const FilePath &pathEntry : systemEnvironment().path())
        paths.append(filePath(pathEntry.path()));
    return searchExecutable(fileName, paths);
}

FilePath IDevice::searchExecutable(const QString &fileName, const FilePaths &dirs) const
{
    for (FilePath dir : dirs) {
        if (!handlesFile(dir)) // Allow device-local dirs to be used.
            dir = filePath(dir.path());
        QTC_CHECK(handlesFile(dir));
        const FilePath candidate = dir / fileName;
        if (candidate.isExecutableFile())
            return candidate;
    }

    return {};
}

ProcessInterface *IDevice::createProcessInterface() const
{
    QTC_CHECK(false);
    return nullptr;
}

FileTransferInterface *IDevice::createFileTransferInterface(
        const FileTransferSetupData &setup) const
{
    Q_UNUSED(setup)
    QTC_CHECK(false);
    return nullptr;
}

Environment IDevice::systemEnvironment() const
{
    DeviceFileAccess *access = fileAccess();
    QTC_ASSERT(access, return Environment::systemEnvironment());
    return access->deviceEnvironment();
}

/*!
    Specifies a free-text name for the device to be displayed in GUI elements.
*/

QString IDevice::displayName() const
{
    return d->displayName.value();
}

void IDevice::setDisplayName(const QString &name)
{
    d->displayName.setValue(name);
}

void IDevice::setDefaultDisplayName(const QString &name)
{
    d->displayName.setDefaultValue(name);
}

QString IDevice::displayType() const
{
    return d->displayType;
}

void IDevice::setDisplayType(const QString &type)
{
    d->displayType = type;
}

void IDevice::setOsType(OsType osType)
{
    d->osType = osType;
}

void IDevice::setFileAccess(DeviceFileAccess *fileAccess)
{
    d->fileAccess = fileAccess;
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

Id IDevice::type() const
{
    return d->type;
}

void IDevice::setType(Id type)
{
    d->type = type;
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

Id IDevice::id() const
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
    return DeviceTypeKitAspect::deviceTypeId(k) == type();
}

QList<Task> IDevice::validate() const
{
    return {};
}

void IDevice::addDeviceAction(const DeviceAction &deviceAction)
{
    d->deviceActions.append(deviceAction);
}

const QList<IDevice::DeviceAction> IDevice::deviceActions() const
{
    return d->deviceActions;
}

DeviceProcessList *IDevice::createProcessListModel(QObject *parent) const
{
    Q_UNUSED(parent)
    QTC_ASSERT(false, qDebug("This should not have been called..."); return nullptr);
    return nullptr;
}

DeviceTester *IDevice::createDeviceTester() const
{
    QTC_ASSERT(false, qDebug("This should not have been called..."));
    return nullptr;
}

OsType IDevice::osType() const
{
    return d->osType;
}

DeviceProcessSignalOperation::Ptr IDevice::signalOperation() const
{
    return {};
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

Id IDevice::typeFromMap(const QVariantMap &map)
{
    return Id::fromSetting(map.value(QLatin1String(TypeKey)));
}

Id IDevice::idFromMap(const QVariantMap &map)
{
    return Id::fromSetting(map.value(QLatin1String(IdKey)));
}

/*!
    Restores a device object from a serialized state as written by toMap().
    If subclasses override this to restore additional state, they must call the
    base class implementation.
*/

void IDevice::fromMap(const QVariantMap &map)
{
    d->type = typeFromMap(map);
    d->displayName.fromMap(map, DisplayNameKey);
    d->id = Id::fromSetting(map.value(QLatin1String(IdKey)));
    if (!d->id.isValid())
        d->id = newId();
    d->origin = static_cast<Origin>(map.value(QLatin1String(OriginKey), ManuallyAdded).toInt());

    QWriteLocker locker(&d->lock);
    d->sshParameters.setHost(map.value(QLatin1String(HostKey)).toString());
    d->sshParameters.setPort(map.value(QLatin1String(SshPortKey), 22).toInt());
    d->sshParameters.setUserName(map.value(QLatin1String(UserNameKey)).toString());

    // Pre-4.9, the authentication enum used to have more values
    const int storedAuthType = map.value(QLatin1String(AuthKey), DefaultAuthType).toInt();
    const bool outdatedAuthType = storedAuthType
            > SshParameters::AuthenticationTypeSpecificKey;
    d->sshParameters.authenticationType = outdatedAuthType
            ? SshParameters::AuthenticationTypeAll
            : static_cast<AuthType>(storedAuthType);

    d->sshParameters.privateKeyFile =
        FilePath::fromVariant(map.value(QLatin1String(KeyFileKey), defaultPrivateKeyFilePath()));
    d->sshParameters.timeout = map.value(QLatin1String(TimeoutKey), DefaultTimeout).toInt();
    d->sshParameters.hostKeyCheckingMode = static_cast<SshHostKeyCheckingMode>
            (map.value(QLatin1String(HostKeyCheckingKey), SshHostKeyCheckingNone).toInt());

    QString portsSpec = map.value(PortsSpecKey).toString();
    if (portsSpec.isEmpty())
        portsSpec = "10000-10100";
    d->freePorts = PortList::fromString(portsSpec);
    d->machineType = static_cast<MachineType>(map.value(QLatin1String(MachineTypeKey), DefaultMachineType).toInt());
    d->version = map.value(QLatin1String(VersionKey), 0).toInt();

    d->debugServerPath = FilePath::fromVariant(map.value(QLatin1String(DebugServerKey)));
    d->qmlRunCommand = FilePath::fromVariant(map.value(QLatin1String(QmlRuntimeKey)));
    d->extraData = map.value(ExtraDataKey).toMap();
}

/*!
    Serializes a device object, for example to save it to a file.
    If subclasses override this function to save additional state, they must
    call the base class implementation.
*/

QVariantMap IDevice::toMap() const
{
    QVariantMap map;
    d->displayName.toMap(map, DisplayNameKey);
    map.insert(QLatin1String(TypeKey), d->type.toString());
    map.insert(QLatin1String(IdKey), d->id.toSetting());
    map.insert(QLatin1String(OriginKey), d->origin);

    QReadLocker locker(&d->lock);
    map.insert(QLatin1String(MachineTypeKey), d->machineType);
    map.insert(QLatin1String(HostKey), d->sshParameters.host());
    map.insert(QLatin1String(SshPortKey), d->sshParameters.port());
    map.insert(QLatin1String(UserNameKey), d->sshParameters.userName());
    map.insert(QLatin1String(AuthKey), d->sshParameters.authenticationType);
    map.insert(QLatin1String(KeyFileKey), d->sshParameters.privateKeyFile.toVariant());
    map.insert(QLatin1String(TimeoutKey), d->sshParameters.timeout);
    map.insert(QLatin1String(HostKeyCheckingKey), d->sshParameters.hostKeyCheckingMode);

    map.insert(QLatin1String(PortsSpecKey), d->freePorts.toString());
    map.insert(QLatin1String(VersionKey), d->version);

    map.insert(QLatin1String(DebugServerKey), d->debugServerPath.toVariant());
    map.insert(QLatin1String(QmlRuntimeKey), d->qmlRunCommand.toVariant());
    map.insert(ExtraDataKey, d->extraData);

    return map;
}

IDevice::Ptr IDevice::clone() const
{
    IDeviceFactory *factory = IDeviceFactory::find(d->type);
    QTC_ASSERT(factory, return {});
    IDevice::Ptr device = factory->construct();
    QTC_ASSERT(device, return {});
    device->d->deviceState = d->deviceState;
    device->d->deviceActions = d->deviceActions;
    device->d->deviceIcons = d->deviceIcons;
    // Os type is only set in the constructor, always to the same value.
    // But make sure we notice if that changes in the future (which it shouldn't).
    QTC_CHECK(device->d->osType == d->osType);
    device->d->osType = d->osType;
    device->fromMap(toMap());
    return device;
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

SshParameters IDevice::sshParameters() const
{
    QReadLocker locker(&d->lock);
    return d->sshParameters;
}

void IDevice::setSshParameters(const SshParameters &sshParameters)
{
    QWriteLocker locker(&d->lock);
    d->sshParameters = sshParameters;
}

QUrl IDevice::toolControlChannel(const ControlChannelHint &) const
{
    QUrl url;
    url.setScheme(urlTcpScheme());
    QReadLocker locker(&d->lock);
    url.setHost(d->sshParameters.host());
    return url;
}

void IDevice::setFreePorts(const PortList &freePorts)
{
    d->freePorts = freePorts;
}

PortList IDevice::freePorts() const
{
    return d->freePorts;
}

IDevice::MachineType IDevice::machineType() const
{
    return d->machineType;
}

void IDevice::setMachineType(MachineType machineType)
{
    d->machineType = machineType;
}

FilePath IDevice::rootPath() const
{
    return FilePath::fromParts(u"device", id().toString(), u"/");
}

FilePath IDevice::debugServerPath() const
{
    return d->debugServerPath;
}

void IDevice::setDebugServerPath(const FilePath &path)
{
    d->debugServerPath = path;
}

FilePath IDevice::debugDumperPath() const
{
    return d->debugDumperPath;
}

void IDevice::setDebugDumperPath(const FilePath &path)
{
    d->debugDumperPath = path;
}

FilePath IDevice::qmlRunCommand() const
{
    return d->qmlRunCommand;
}

void IDevice::setQmlRunCommand(const FilePath &path)
{
    d->qmlRunCommand = path;
}

void IDevice::setExtraData(Id kind, const QVariant &data)
{
    d->extraData.insert(kind.toString(), data);
}

QVariant IDevice::extraData(Id kind) const
{
    return d->extraData.value(kind.toString());
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

bool IDevice::ensureReachable(const FilePath &other) const
{
    return handlesFile(other); // Some first approximation.
}

expected_str<FilePath> IDevice::localSource(const Utils::FilePath &other) const
{
    Q_UNUSED(other);
    return make_unexpected(Tr::tr("localSource() not implemented for this device type."));
}

bool IDevice::prepareForBuild(const Target *target)
{
    Q_UNUSED(target)
    return true;
}

std::optional<Utils::FilePath> IDevice::clangdExecutable() const
{
    return std::nullopt;
}

void DeviceProcessSignalOperation::setDebuggerCommand(const FilePath &cmd)
{
    m_debuggerCommand = cmd;
}

DeviceProcessSignalOperation::DeviceProcessSignalOperation() = default;

void DeviceProcessKiller::start()
{
    m_signalOperation.reset();
    m_errorString.clear();

    const IDevice::ConstPtr device = DeviceManager::deviceForPath(m_processPath);
    if (!device) {
        m_errorString = tr("No device for given path: \"%1\".").arg(m_processPath.toUserOutput());
        emit done(false);
        return;
    }

    m_signalOperation = device->signalOperation();
    if (!m_signalOperation) {
        m_errorString = tr("Device for path \"%1\" does not support killing processes.")
                       .arg(m_processPath.toUserOutput());
        emit done(false);
        return;
    }

    connect(m_signalOperation.get(), &DeviceProcessSignalOperation::finished,
            this, [this](const QString &errorMessage) {
        m_errorString = errorMessage;
        emit done(m_errorString.isEmpty());
    });

    m_signalOperation->killProcess(m_processPath.path());
}

KillerAdapter::KillerAdapter()
{
    connect(task(), &DeviceProcessKiller::done, this, &KillerAdapter::done);
}

void KillerAdapter::start()
{
    task()->start();
}

} // namespace ProjectExplorer
