// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "idevice.h"

#include "devicemanager.h"
#include "idevicefactory.h"
#include "processlist.h"
#include "sshparameters.h"

#include "../kit.h"
#include "../kitaspects.h"
#include "../projectexplorertr.h"
#include "../target.h"

#include <coreplugin/icore.h>

#include <utils/commandline.h>
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
const char ClientOsTypeKey[] = "ClientOsType";
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
    IDevicePrivate(std::unique_ptr<DeviceSettings> s)
        : settings(std::move(s))
    {
        if (!settings)
            settings = std::make_unique<DeviceSettings>();
    }

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
    Store extraData;
    IDevice::OpenTerminal openTerminal;

    std::unique_ptr<DeviceSettings> settings;
};
} // namespace Internal

DeviceSettings::DeviceSettings()
{
    setAutoApply(false);

    displayName.setSettingsKey(DisplayNameKey);
    displayName.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);

    auto validateDisplayName = [](const QString &old,
                                  const QString &newValue) -> expected_str<void> {
        if (old == newValue)
            return {};

        if (newValue.trimmed().isEmpty())
            return make_unexpected(Tr::tr("The device name cannot be empty."));

        if (DeviceManager::clonedInstance()->hasDevice(newValue))
            return make_unexpected(Tr::tr("A device with this name already exists."));

        return {};
    };

    displayName.setValidationFunction(
        [this, validateDisplayName](FancyLineEdit *edit, QString *errorMsg) -> bool {
            auto result = validateDisplayName(displayName.value(), edit->text());
            if (result)
                return true;

            if (errorMsg)
                *errorMsg = result.error();

            return false;
        });

    displayName.setValueAcceptor(
        [validateDisplayName](const QString &old,
                              const QString &newValue) -> std::optional<QString> {
            if (validateDisplayName(old, newValue))
                return std::nullopt;

            return old;
        });
}

DeviceTester::DeviceTester(QObject *parent) : QObject(parent) { }

IDevice::IDevice(std::unique_ptr<DeviceSettings> settings)
    : d(new Internal::IDevicePrivate(std::move(settings)))
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

expected_str<void> IDevice::openTerminal(const Environment &env, const FilePath &workingDir) const
{
    QTC_ASSERT(canOpenTerminal(),
               return make_unexpected(Tr::tr("Opening a terminal is not supported.")));
    return d->openTerminal(env, workingDir);
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
    expected_str<Environment> env = systemEnvironmentWithError();
    QTC_ASSERT_EXPECTED(env, return {});
    return *env;
}

expected_str<Environment> IDevice::systemEnvironmentWithError() const
{
    DeviceFileAccess *access = fileAccess();
    QTC_ASSERT(access, return Environment::systemEnvironment());
    return access->deviceEnvironment();
}

QString IDevice::displayName() const
{
    return d->settings->displayName();
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
    const QString key = Tr::tr("Device");
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

PortsGatheringMethod IDevice::portsGatheringMethod() const
{
    return {[this](QAbstractSocket::NetworkLayerProtocol protocol) -> CommandLine {
                // We might encounter the situation that protocol is given IPv6
                // but the consumer of the free port information decides to open
                // an IPv4(only) port. As a result the next IPv6 scan will
                // report the port again as open (in IPv6 namespace), while the
                // same port in IPv4 namespace might still be blocked, and
                // re-use of this port fails.
                // GDBserver behaves exactly like this.

                Q_UNUSED(protocol)

                if (filePath("/proc/net").isReadableDir())
                    return {filePath("/bin/sh"), {"-c", "cat /proc/net/tcp*"}};

                return {filePath("netstat"), {"-a", "-n"}};
            },
            &Port::parseFromCommandOutput};
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

Id IDevice::typeFromMap(const Store &map)
{
    return Id::fromSetting(map.value(TypeKey));
}

Id IDevice::idFromMap(const Store &map)
{
    return Id::fromSetting(map.value(IdKey));
}

/*!
    Restores a device object from a serialized state as written by toMap().
    If subclasses override this to restore additional state, they must call the
    base class implementation.
*/

void IDevice::fromMap(const Store &map)
{
    d->type = typeFromMap(map);
    settings()->fromMap(map);

    d->id = Id::fromSetting(map.value(IdKey));
    d->osType = osTypeFromString(map.value(ClientOsTypeKey, osTypeToString(OsTypeLinux)).toString());
    if (!d->id.isValid())
        d->id = newId();
    d->origin = static_cast<Origin>(map.value(OriginKey, ManuallyAdded).toInt());

    QWriteLocker locker(&d->lock);
    d->sshParameters.setHost(map.value(HostKey).toString());
    d->sshParameters.setPort(map.value(SshPortKey, 22).toInt());
    d->sshParameters.setUserName(map.value(UserNameKey).toString());

    // Pre-4.9, the authentication enum used to have more values
    const int storedAuthType = map.value(AuthKey, DefaultAuthType).toInt();
    const bool outdatedAuthType = storedAuthType
            > SshParameters::AuthenticationTypeSpecificKey;
    d->sshParameters.authenticationType = outdatedAuthType
            ? SshParameters::AuthenticationTypeAll
            : static_cast<AuthType>(storedAuthType);

    d->sshParameters.privateKeyFile =
        FilePath::fromSettings(map.value(KeyFileKey, defaultPrivateKeyFilePath()));
    d->sshParameters.timeout = map.value(TimeoutKey, DefaultTimeout).toInt();
    d->sshParameters.hostKeyCheckingMode = static_cast<SshHostKeyCheckingMode>
            (map.value(HostKeyCheckingKey, SshHostKeyCheckingNone).toInt());

    QString portsSpec = map.value(PortsSpecKey).toString();
    if (portsSpec.isEmpty())
        portsSpec = "10000-10100";
    d->freePorts = PortList::fromString(portsSpec);
    d->machineType = static_cast<MachineType>(map.value(MachineTypeKey, DefaultMachineType).toInt());
    d->version = map.value(VersionKey, 0).toInt();

    d->debugServerPath = FilePath::fromSettings(map.value(DebugServerKey));
    const FilePath qmlRunCmd = FilePath::fromSettings(map.value(QmlRuntimeKey));
    d->qmlRunCommand = qmlRunCmd;
    d->extraData = storeFromVariant(map.value(ExtraDataKey));
}

/*!
    Serializes a device object, for example to save it to a file.
    If subclasses override this function to save additional state, they must
    call the base class implementation.
*/

Store IDevice::toMap() const
{
    Store map;
    d->settings->toMap(map);

    map.insert(TypeKey, d->type.toString());
    map.insert(ClientOsTypeKey, osTypeToString(d->osType));
    map.insert(IdKey, d->id.toSetting());
    map.insert(OriginKey, d->origin);

    QReadLocker locker(&d->lock);
    map.insert(MachineTypeKey, d->machineType);
    map.insert(HostKey, d->sshParameters.host());
    map.insert(SshPortKey, d->sshParameters.port());
    map.insert(UserNameKey, d->sshParameters.userName());
    map.insert(AuthKey, d->sshParameters.authenticationType);
    map.insert(KeyFileKey, d->sshParameters.privateKeyFile.toSettings());
    map.insert(TimeoutKey, d->sshParameters.timeout);
    map.insert(HostKeyCheckingKey, d->sshParameters.hostKeyCheckingMode);

    map.insert(PortsSpecKey, d->freePorts.toString());
    map.insert(VersionKey, d->version);

    map.insert(DebugServerKey, d->debugServerPath.toSettings());
    map.insert(QmlRuntimeKey, d->qmlRunCommand.toSettings());

    map.insert(ExtraDataKey, variantFromStore(d->extraData));

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
    device->d->osType = d->osType;
    device->fromMap(toMap());
    return device;
}

DeviceSettings *IDevice::settings() const
{
    return d->settings.get();
}

QString IDevice::deviceStateToString() const
{
    switch (d->deviceState) {
    case IDevice::DeviceReadyToUse: return Tr::tr("Ready to use");
    case IDevice::DeviceConnected: return Tr::tr("Connected");
    case IDevice::DeviceDisconnected: return Tr::tr("Disconnected");
    case IDevice::DeviceStateUnknown: return Tr::tr("Unknown");
    default: return Tr::tr("Invalid");
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
    d->extraData.insert(keyFromString(kind.toString()), data);
}

QVariant IDevice::extraData(Id kind) const
{
    return d->extraData.value(keyFromString(kind.toString()));
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
        m_errorString = Tr::tr("No device for given path: \"%1\".").arg(m_processPath.toUserOutput());
        emit done(false);
        return;
    }

    m_signalOperation = device->signalOperation();
    if (!m_signalOperation) {
        m_errorString = Tr::tr("Device for path \"%1\" does not support killing processes.")
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

DeviceProcessKillerTaskAdapter::DeviceProcessKillerTaskAdapter()
{
    connect(task(), &DeviceProcessKiller::done, this, &TaskInterface::done);
}

void DeviceProcessKillerTaskAdapter::start()
{
    task()->start();
}

} // namespace ProjectExplorer
