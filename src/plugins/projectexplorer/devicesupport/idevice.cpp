// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "idevice.h"

#include "devicekitaspects.h"
#include "devicemanager.h"
#include "idevicefactory.h"
#include "sshparameters.h"

#include "../kit.h"
#include "../projectexplorericons.h"
#include "../projectexplorertr.h"
#include "../target.h"

#include <coreplugin/icore.h>

#include <utils/commandline.h>
#include <utils/devicefileaccess.h>
#include <utils/displayname.h>
#include <utils/environment.h>
#include <utils/icon.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <utils/synchronizedvalue.h>
#include <utils/url.h>

#include <QCoreApplication>
#include <QStandardPaths>

#include <QDateTime>
#include <QReadWriteLock>
#include <QStandardItem>
#include <QString>

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

using namespace Tasking;
using namespace Utils;

namespace ProjectExplorer {

static Id newId()
{
    return Id::generate();
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

const char SshForwardDebugServerPortKey[] = "SshForwardDebugServerPort";
const char LinkDeviceKey[] = "LinkDevice";

using AuthType = SshParameters::AuthenticationType;
const AuthType DefaultAuthType = SshParameters::AuthenticationTypeAll;
const IDevice::MachineType DefaultMachineType = IDevice::Hardware;

const int DefaultTimeout = 10;

namespace Internal {

class IDevicePrivate
{
public:
    QString displayType;
    Id type;
    IDevice::Origin origin = IDevice::AutoDetected;
    Id id;
    IDevice::DeviceState deviceState = IDevice::DeviceStateUnknown;
    IDevice::MachineType machineType = IDevice::Hardware;
    OsType osType = OsTypeOther;
    DeviceFileAccess *fileAccess = nullptr;
    std::function<DeviceFileAccess *()> fileAccessFactory;
    int version = 0; // This is used by devices that have been added by the SDK.

    Utils::SynchronizedValue<SshParameters> sshParameters;

    QList<Icon> deviceIcons;
    QList<IDevice::DeviceAction> deviceActions;
    Store extraData;
    IDevice::OpenTerminal openTerminal;

    Utils::StringAspect displayName;

    SshParametersAspectContainer sshParametersAspectContainer;

    bool isTesting = false;
};

} // namespace Internal

DeviceTester::DeviceTester(const IDevice::Ptr &device, QObject *parent)
    : QObject(parent)
    , m_device(device)
{
    m_device->setIsTesting(true);
}

DeviceTester::~DeviceTester()
{
    m_device->setIsTesting(false);
}

IDevice::IDevice()
    : d(new Internal::IDevicePrivate)
{
    setAutoApply(false);

    registerAspect(&d->sshParametersAspectContainer);

    connect(&d->sshParametersAspectContainer, &AspectContainer::applied, this, [this]() {
        *d->sshParameters.writeLocked() = d->sshParametersAspectContainer.sshParameters();
    });

    registerAspect(&d->displayName);
    d->displayName.setSettingsKey(DisplayNameKey);
    d->displayName.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);

    // allowEmptyCommand.setSettingsKey() intentionally omitted, this is not persisted.

    sshForwardDebugServerPort.setSettingsKey(SshForwardDebugServerPortKey);
    sshForwardDebugServerPort.setLabelText(Tr::tr("Use SSH port forwarding for debugging"));
    sshForwardDebugServerPort.setToolTip(
        Tr::tr("Enable debugging on remote targets which cannot expose gdbserver ports.\n"
               "The ssh tunneling is used to map the remote gdbserver port to localhost.\n"
               "The local and remote ports are determined automatically."));
    sshForwardDebugServerPort.setDefaultValue(false);
    sshForwardDebugServerPort.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBox);

    linkDevice.setSettingsKey(LinkDeviceKey);
    linkDevice.setLabelText(Tr::tr("Access via:"));
    linkDevice.setToolTip(Tr::tr("Select the device to connect through."));
    linkDevice.setDefaultValue("Direct");
    linkDevice.setComboBoxEditable(false);
    linkDevice.setFillCallback([this](const StringSelectionAspect::ResultCallback &cb) {
        QList<QStandardItem *> items;
        auto defaultItem = new QStandardItem(Tr::tr("Direct"));
        defaultItem->setData("direct");
        items.append(defaultItem);
        for (int i = 0, n = DeviceManager::deviceCount(); i < n; ++i) {
            const auto device = DeviceManager::deviceAt(i);
            if (device->id() == this->id())
                continue;
            QStandardItem *newItem = new QStandardItem(device->displayName());
            newItem->setData(device->id().toSetting());
            items.append(newItem);
        }
        cb(items);
    });

    auto validateDisplayName = [](const QString &old, const QString &newValue) -> Result<> {
        if (old == newValue)
            return ResultOk;

        if (newValue.trimmed().isEmpty())
            return ResultError(Tr::tr("The device name cannot be empty."));

        if (DeviceManager::hasDevice(newValue))
            return ResultError(Tr::tr("A device with this name already exists."));

        return ResultOk;
    };

    d->displayName.setValidationFunction(
        [this, validateDisplayName](const QString &text) -> Result<> {
            return validateDisplayName(d->displayName.value(), text);
        });

    d->displayName.setValueAcceptor(
        [validateDisplayName](const QString &old,
                              const QString &newValue) -> std::optional<QString> {
            if (!validateDisplayName(old, newValue))
                return std::nullopt;

            return newValue;
        });

    debugServerPathAspect.setSettingsKey(DebugServerKey);
    debugServerPathAspect.setLabelText(Tr::tr("GDB server executable:"));
    debugServerPathAspect.setToolTip(Tr::tr("The GDB server executable to use on the device."));
    debugServerPathAspect.setPlaceHolderText(Tr::tr("Leave empty to look up executable in $PATH"));
    debugServerPathAspect.setHistoryCompleter("GdbServer");
    debugServerPathAspect.setAllowPathFromDevice(true);
    debugServerPathAspect.setExpectedKind(PathChooser::ExistingCommand);

    qmlRunCommandAspect.setSettingsKey(QmlRuntimeKey);
    qmlRunCommandAspect.setLabelText(Tr::tr("QML runtime executable:"));
    qmlRunCommandAspect.setToolTip(Tr::tr("The QML runtime executable to use on the device."));
    qmlRunCommandAspect.setPlaceHolderText(Tr::tr("Leave empty to look up executable in $PATH"));
    qmlRunCommandAspect.setHistoryCompleter("QmlRuntime");
    qmlRunCommandAspect.setAllowPathFromDevice(true);
    qmlRunCommandAspect.setExpectedKind(PathChooser::ExistingCommand);

    freePortsAspect.setSettingsKey(PortsSpecKey);
    freePortsAspect.setLabelText(Tr::tr("Free ports:"));
    freePortsAspect.setToolTip(
        Tr::tr("You can enter lists and ranges like this: '1024,1026-1028,1030'."));
    freePortsAspect.setHistoryCompleter("PortRange");
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

Result<> IDevice::openTerminal(const Environment &env, const FilePath &workingDir) const
{
    QTC_ASSERT(canOpenTerminal(),
               return make_unexpected(Tr::tr("Opening a terminal is not supported.")));
    return d->openTerminal(env, workingDir);
}

bool IDevice::isAnyUnixDevice() const
{
    return d->osType == OsTypeLinux || d->osType == OsTypeMac || d->osType == OsTypeOtherUnix;
}

DeviceFileAccess *IDevice::fileAccess() const
{
    if (d->fileAccessFactory)
        return d->fileAccessFactory();

    return d->fileAccess;
}

FilePath IDevice::filePath(const QString &pathOnDevice) const
{
    return rootPath().withNewPath(pathOnDevice);
}

FilePath IDevice::debugServerPath() const
{
    return debugServerPathAspect();
}

void IDevice::setDebugServerPath(const FilePath &path)
{
    debugServerPathAspect.setValue(path);
}

FilePath IDevice::qmlRunCommand() const
{
    return qmlRunCommandAspect();
}

void IDevice::setQmlRunCommand(const FilePath &path)
{
    qmlRunCommandAspect.setValue(path);
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
    Result<Environment> env = systemEnvironmentWithError();
    QTC_ASSERT_RESULT(env, return {});
    return *env;
}

Result<Environment> IDevice::systemEnvironmentWithError() const
{
    DeviceFileAccess *access = fileAccess();
    QTC_ASSERT(access, return Environment::systemEnvironment());
    return access->deviceEnvironment();
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

void IDevice::setFileAccessFactory(std::function<DeviceFileAccess *()> fileAccessFactory)
{
    d->fileAccessFactory = fileAccessFactory;
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

ExecutableItem IDevice::portsGatheringRecipe(const Storage<PortsOutputData> &output) const
{
    const Storage<PortsInputData> input;

    const auto onSetup = [this, input] {
        const CommandLine cmd = filePath("/proc/net").isReadableDir()
                              ? CommandLine{filePath("/bin/sh"), {"-c", "cat /proc/net/tcp*"}}
                              : CommandLine{filePath("netstat"), {"-a", "-n"}};
        *input = {freePorts(), cmd};
    };

    return Group {
        input,
        onGroupSetup(onSetup),
        portsFromProcessRecipe(input, output)
    };
}

DeviceTester *IDevice::createDeviceTester()
{
    QTC_ASSERT(false, qDebug("This should not have been called..."));
    return nullptr;
}

void IDevice::setIsTesting(bool isTesting)
{
    d->isTesting = isTesting;
}

bool IDevice::isTesting() const
{
    return d->isTesting;
}

bool IDevice::canMount(const Utils::FilePath &) const
{
    return false;
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

// Backwards compatibility: Pre 17.0 a bunch of settings were stored in the extra data
namespace {

static const char LinkDevice[] = "RemoteLinux.LinkDevice";
static const char SSH_FORWARD_DEBUGSERVER_PORT[] = "RemoteLinux.SshForwardDebugServerPort";

static void backwardsFromExtraData(IDevice *device, const Store &map)
{
    if (map.contains(LinkDevice))
        device->linkDevice.setValue(Id::fromSetting(map.value(LinkDevice)).toString());

    if (map.contains(SSH_FORWARD_DEBUGSERVER_PORT))
        device->sshForwardDebugServerPort.setValue(map.value(SSH_FORWARD_DEBUGSERVER_PORT).toBool());
}

static void backwardsToExtraData(const IDevice *const device, Store &map)
{
    map.insert(LinkDevice, device->linkDevice());
    map.insert(SSH_FORWARD_DEBUGSERVER_PORT, device->sshForwardDebugServerPort());
}

} // namespace

/*!
    Restores a device object from a serialized state as written by toMap().
    If subclasses override this to restore additional state, they must call the
    base class implementation.
*/
void IDevice::fromMap(const Store &map)
{
    AspectContainer::fromMap(map);
    d->type = typeFromMap(map);

    d->id = Id::fromSetting(map.value(IdKey));
    d->osType = osTypeFromString(map.value(ClientOsTypeKey).toString()).value_or(OsTypeLinux);
    if (!d->id.isValid())
        d->id = newId();
    d->origin = static_cast<Origin>(map.value(OriginKey, ManuallyAdded).toInt());

    d->machineType = static_cast<MachineType>(map.value(MachineTypeKey, DefaultMachineType).toInt());
    d->version = map.value(VersionKey, 0).toInt();

    d->extraData = storeFromVariant(map.value(ExtraDataKey));

    backwardsFromExtraData(this, d->extraData);

    SshParameters ssh;
    ssh.setHost(map.value(HostKey).toString());
    ssh.setPort(map.value(SshPortKey, 22).toInt());
    ssh.setUserName(map.value(UserNameKey).toString());

    // Pre-4.9, the authentication enum used to have more values
    const int storedAuthType = map.value(AuthKey, DefaultAuthType).toInt();
    const bool outdatedAuthType = storedAuthType > SshParameters::AuthenticationTypeSpecificKey;
    ssh.setAuthenticationType(
        outdatedAuthType ? SshParameters::AuthenticationTypeAll
                         : static_cast<AuthType>(storedAuthType));

    ssh.setPrivateKeyFile(
        FilePath::fromSettings(map.value(KeyFileKey, defaultPrivateKeyFilePath())));
    ssh.setTimeout(map.value(TimeoutKey, DefaultTimeout).toInt());
    ssh.setHostKeyCheckingMode(static_cast<SshHostKeyCheckingMode>(
        map.value(HostKeyCheckingKey, SshHostKeyCheckingNone).toInt()));

    d->sshParametersAspectContainer.setSshParameters(ssh);
}

/*!
    Serializes a device object, for example to save it to a file.
    If subclasses override this function to save additional state, they must
    call the base class implementation.
*/

void IDevice::toMap(Store &map) const
{
    AspectContainer::toMap(map);

    map.insert(TypeKey, d->type.toString());
    map.insert(ClientOsTypeKey, osTypeToString(d->osType));
    map.insert(IdKey, d->id.toSetting());
    map.insert(OriginKey, d->origin);

    map.insert(MachineTypeKey, d->machineType);
    map.insert(VersionKey, d->version);

    Store extraData = d->extraData;
    backwardsToExtraData(this, extraData);

    map.insert(ExtraDataKey, variantFromStore(extraData));

    SshParameters ssh = d->sshParametersAspectContainer.sshParameters();
    map.insert(HostKey, ssh.host());
    map.insert(SshPortKey, ssh.port());
    map.insert(UserNameKey, ssh.userName());
    map.insert(AuthKey, ssh.authenticationType());
    map.insert(KeyFileKey, ssh.privateKeyFile().toSettings());
    map.insert(TimeoutKey, ssh.timeout());
    map.insert(HostKeyCheckingKey, ssh.hostKeyCheckingMode());
}

QString IDevice::displayName() const
{
    return d->displayName();
}

void IDevice::setDisplayName(const QString &name)
{
    d->displayName.setValue(name);
}

QString IDevice::defaultDisplayName() const
{
    return d->displayName.defaultValue();
}

void IDevice::setDefaultDisplayName(const QString &name)
{
    d->displayName.setDefaultValue(name);
}

void IDevice::addDisplayNameToLayout(Layouting::Layout &layout) const
{
    d->displayName.addToLayout(layout);
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

QPixmap IDevice::deviceStateIcon() const
{
    switch (deviceState()) {
    case IDevice::DeviceReadyToUse: return Icons::DEVICE_READY_INDICATOR.pixmap();
    case IDevice::DeviceConnected: return Icons::DEVICE_CONNECTED_INDICATOR.pixmap();
    case IDevice::DeviceDisconnected: return Icons::DEVICE_DISCONNECTED_INDICATOR.pixmap();
    case IDevice::DeviceStateUnknown: break;
    }
    return {};
}

SshParameters IDevice::sshParameters() const
{
    return *d->sshParameters.readLocked();
}

void IDevice::setDefaultSshParameters(const SshParameters &sshParameters)
{
    QTC_ASSERT(QThread::currentThread() == qApp->thread(), return);

    sshParametersAspectContainer().host.setDefaultValue(sshParameters.host());
    sshParametersAspectContainer().port.setDefaultValue(sshParameters.port());
    sshParametersAspectContainer().userName.setDefaultValue(sshParameters.userName());
    sshParametersAspectContainer().privateKeyFile.setDefaultPathValue(
        sshParameters.privateKeyFile());
    sshParametersAspectContainer().timeout.setDefaultValue(sshParameters.timeout());
    sshParametersAspectContainer().useKeyFile.setDefaultValue(
        sshParameters.authenticationType() == SshParameters::AuthenticationTypeSpecificKey);
    sshParametersAspectContainer().hostKeyCheckingMode.setDefaultValue(
        sshParameters.hostKeyCheckingMode());

    *d->sshParameters.writeLocked() = sshParametersAspectContainer().sshParameters();
}

QUrl IDevice::toolControlChannel(const ControlChannelHint &) const
{
    QUrl url;
    url.setScheme(urlTcpScheme());
    url.setHost(d->sshParameters.readLocked()->host());
    return url;
}

void IDevice::setFreePorts(const PortList &freePorts)
{
    freePortsAspect.setPortList(freePorts);
}

PortList IDevice::freePorts() const
{
    return freePortsAspect.portList();
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
    // match DeviceManager::deviceForPath
    return FilePath::fromParts(u"device", id().toString(), u"/");
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

Result<FilePath> IDevice::localSource(const Utils::FilePath &other) const
{
    Q_UNUSED(other)
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

void IDevice::doApply() const
{
    const_cast<IDevice *>(this)->apply();
}

void DeviceProcessSignalOperation::setDebuggerCommand(const FilePath &cmd)
{
    m_debuggerCommand = cmd;
}

DeviceProcessSignalOperation::DeviceProcessSignalOperation() = default;

void DeviceProcessKiller::start()
{
    m_signalOperation.reset();
    m_result = ResultOk;

    const IDevice::ConstPtr device = DeviceManager::deviceForPath(m_processPath);
    if (!device) {
        m_result = ResultError(Tr::tr("No device for given path: \"%1\".")
                                     .arg(m_processPath.toUserOutput()));
        emit done(DoneResult::Error);
        return;
    }

    m_signalOperation = device->signalOperation();
    if (!m_signalOperation) {
        m_result = ResultError(Tr::tr("Device for path \"%1\" does not support killing processes.")
                                     .arg(m_processPath.toUserOutput()));
        emit done(DoneResult::Error);
        return;
    }

    connect(m_signalOperation.get(), &DeviceProcessSignalOperation::finished,
            this, [this](const Result<> &result) {
        m_result = result;
        emit done(toDoneResult(result.has_value()));
    });

    m_signalOperation->killProcess(m_processPath.path());
}

// DeviceConstRef

DeviceConstRef::DeviceConstRef(const IDevice::ConstPtr &device)
    : m_constDevice(device)
{}

DeviceConstRef::DeviceConstRef(const IDevice::Ptr &device)
    : m_constDevice(device)
{}

IDevice::ConstPtr DeviceConstRef::lock() const
{
    return m_constDevice.lock();
}

DeviceConstRef::~DeviceConstRef() = default;

Id DeviceConstRef::id() const
{
    const IDevice::ConstPtr device = m_constDevice.lock();
    QTC_ASSERT(device, return {});
    return device->id();
}

QString DeviceConstRef::displayName() const
{
    const IDevice::ConstPtr device = m_constDevice.lock();
    QTC_ASSERT(device, return {});
    return device->displayName();
}

SshParameters DeviceConstRef::sshParameters() const
{
    const IDevice::ConstPtr device = m_constDevice.lock();
    QTC_ASSERT(device, return {});
    return device->sshParameters();
}

QVariant DeviceConstRef::extraData(Id kind) const
{
    const IDevice::ConstPtr device = m_constDevice.lock();
    QTC_ASSERT(device, return {});
    return device->extraData(kind);
}

Id DeviceConstRef::linkDeviceId() const
{
    const IDevice::ConstPtr device = m_constDevice.lock();
    QTC_ASSERT(device, return {});
    return Id::fromString(device->linkDevice.value());
}

FilePath DeviceConstRef::filePath(const QString &pathOnDevice) const
{
    const IDevice::ConstPtr device = m_constDevice.lock();
    QTC_ASSERT(device, return {});
    return device->filePath(pathOnDevice);
}

// DeviceRef, mutable

DeviceRef::DeviceRef(const IDevice::Ptr &device)
    : DeviceConstRef(device), m_mutableDevice(device)
{}

IDevice::Ptr DeviceRef::lock() const
{
    return m_mutableDevice.lock();
}

void DeviceRef::setDisplayName(const QString &displayName)
{
    const IDevice::Ptr device = m_mutableDevice.lock();
    QTC_ASSERT(device, return);
    device->setDisplayName(displayName);
}

void DeviceRef::setSshParameters(const SshParameters &params)
{
    const IDevice::Ptr device = m_mutableDevice.lock();
    QTC_ASSERT(device, return);
    device->sshParametersAspectContainer().setSshParameters(params);
}

SshParametersAspectContainer &IDevice::sshParametersAspectContainer() const
{
    return d->sshParametersAspectContainer;
}

} // namespace ProjectExplorer
