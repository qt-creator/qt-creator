// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"
#include "idevicefwd.h"

#include <solutions/tasking/tasktree.h>

#include <utils/aspects.h>
#include <utils/expected.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/id.h>
#include <utils/store.h>

#include <QAbstractSocket>
#include <QCoreApplication>
#include <QList>
#include <QObject>
#include <QSharedPointer>
#include <QUrl>

#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Utils {
class CommandLine;
class DeviceFileAccess;
class Environment;
class PortList;
class Port;
class Process;
class ProcessInterface;
} // Utils

namespace ProjectExplorer {

class FileTransferInterface;
class FileTransferSetupData;
class Kit;
class SshParameters;
class Target;
class Task;

namespace Internal { class IDevicePrivate; }

class IDeviceWidget;
class DeviceTester;

class PROJECTEXPLORER_EXPORT DeviceProcessSignalOperation : public QObject
{
    Q_OBJECT
public:
    using Ptr = QSharedPointer<DeviceProcessSignalOperation>;

    virtual void killProcess(qint64 pid) = 0;
    virtual void killProcess(const QString &filePath) = 0;
    virtual void interruptProcess(qint64 pid) = 0;
    virtual void interruptProcess(const QString &filePath) = 0;

    void setDebuggerCommand(const Utils::FilePath &cmd);

signals:
    // If the error message is empty the operation was successful
    void finished(const QString &errorMessage);

protected:
    explicit DeviceProcessSignalOperation();

    Utils::FilePath m_debuggerCommand;
    QString m_errorMessage;
};

class PROJECTEXPLORER_EXPORT PortsGatheringMethod final
{
public:
    std::function<Utils::CommandLine(QAbstractSocket::NetworkLayerProtocol protocol)> commandLine;
    std::function<QList<Utils::Port>(const QByteArray &commandOutput)> parsePorts;
};

class PROJECTEXPLORER_EXPORT DeviceSettings : public Utils::AspectContainer
{
public:
    DeviceSettings();

    Utils::StringAspect displayName{this};
};

// See cpp file for documentation.
class PROJECTEXPLORER_EXPORT IDevice : public QEnableSharedFromThis<IDevice>
{
    friend class Internal::IDevicePrivate;
public:
    using Ptr = IDevicePtr;
    using ConstPtr = IDeviceConstPtr;
    template <class ...Args> using Continuation = std::function<void(Args...)>;

    enum Origin { ManuallyAdded, AutoDetected };
    enum MachineType { Hardware, Emulator };

    virtual ~IDevice();

    Ptr clone() const;

    DeviceSettings *settings() const;

    QString displayName() const;

    // Provide some information on the device suitable for formated
    // output, e.g. in tool tips. Get a list of name value pairs.
    class DeviceInfoItem {
    public:
        DeviceInfoItem(const QString &k, const QString &v) : key(k), value(v) { }

        QString key;
        QString value;
    };
    using DeviceInfo = QList<DeviceInfoItem>;
    virtual DeviceInfo deviceInformation() const;

    Utils::Id type() const;
    void setType(Utils::Id type);

    bool isAutoDetected() const;
    Utils::Id id() const;

    virtual bool isCompatibleWith(const Kit *k) const;
    virtual QList<Task> validate() const;

    virtual bool usableAsBuildDevice() const { return false; }

    QString displayType() const;
    Utils::OsType osType() const;

    virtual IDeviceWidget *createWidget() = 0;

    struct DeviceAction {
        QString display;
        std::function<void(const IDevice::Ptr &device, QWidget *parent)> execute;
    };
    void addDeviceAction(const DeviceAction &deviceAction);
    const QList<DeviceAction> deviceActions() const;

    virtual PortsGatheringMethod portsGatheringMethod() const;
    virtual bool canCreateProcessModel() const { return false; }
    virtual bool hasDeviceTester() const { return false; }
    virtual DeviceTester *createDeviceTester() const;

    virtual DeviceProcessSignalOperation::Ptr signalOperation() const;

    enum DeviceState { DeviceReadyToUse, DeviceConnected, DeviceDisconnected, DeviceStateUnknown };
    DeviceState deviceState() const;
    void setDeviceState(const DeviceState state);
    QString deviceStateToString() const;

    static Utils::Id typeFromMap(const Utils::Store &map);
    static Utils::Id idFromMap(const Utils::Store &map);

    static QString defaultPrivateKeyFilePath();
    static QString defaultPublicKeyFilePath();

    SshParameters sshParameters() const;
    void setSshParameters(const SshParameters &sshParameters);

    enum ControlChannelHint { QmlControlChannel };
    virtual QUrl toolControlChannel(const ControlChannelHint &) const;

    Utils::PortList freePorts() const;
    void setFreePorts(const Utils::PortList &freePorts);

    MachineType machineType() const;
    void setMachineType(MachineType machineType);

    virtual Utils::FilePath rootPath() const;
    virtual Utils::FilePath filePath(const QString &pathOnDevice) const;

    Utils::FilePath debugServerPath() const;
    void setDebugServerPath(const Utils::FilePath &path);

    Utils::FilePath qmlRunCommand() const;
    void setQmlRunCommand(const Utils::FilePath &path);

    void setExtraData(Utils::Id kind, const QVariant &data);
    QVariant extraData(Utils::Id kind) const;

    void setupId(Origin origin, Utils::Id id = Utils::Id());

    bool canOpenTerminal() const;
    Utils::expected_str<void> openTerminal(const Utils::Environment &env,
                                           const Utils::FilePath &workingDir) const;

    bool isEmptyCommandAllowed() const;
    void setAllowEmptyCommand(bool allow);

    bool isWindowsDevice() const { return osType() == Utils::OsTypeWindows; }
    bool isLinuxDevice() const { return osType() == Utils::OsTypeLinux; }
    bool isMacDevice() const { return osType() == Utils::OsTypeMac; }
    bool isAnyUnixDevice() const;

    Utils::DeviceFileAccess *fileAccess() const;
    virtual bool handlesFile(const Utils::FilePath &filePath) const;

    virtual Utils::FilePath searchExecutableInPath(const QString &fileName) const;
    virtual Utils::FilePath searchExecutable(const QString &fileName,
                                             const Utils::FilePaths &dirs) const;

    virtual Utils::ProcessInterface *createProcessInterface() const;
    virtual FileTransferInterface *createFileTransferInterface(
            const FileTransferSetupData &setup) const;

    Utils::Environment systemEnvironment() const;
    virtual Utils::expected_str<Utils::Environment> systemEnvironmentWithError() const;

    virtual void aboutToBeRemoved() const {}

    virtual bool ensureReachable(const Utils::FilePath &other) const;
    virtual Utils::expected_str<Utils::FilePath> localSource(const Utils::FilePath &other) const;

    virtual bool prepareForBuild(const Target *target);
    virtual std::optional<Utils::FilePath> clangdExecutable() const;

    virtual void checkOsType() {}

protected:
    IDevice(std::unique_ptr<DeviceSettings> settings = nullptr);

    virtual void fromMap(const Utils::Store &map);
    virtual Utils::Store toMap() const;

    using OpenTerminal = std::function<Utils::expected_str<void>(const Utils::Environment &,
                                                                 const Utils::FilePath &)>;
    void setOpenTerminal(const OpenTerminal &openTerminal);
    void setDisplayType(const QString &type);
    void setOsType(Utils::OsType osType);
    void setFileAccess(Utils::DeviceFileAccess *fileAccess);

private:
    IDevice(const IDevice &) = delete;
    IDevice &operator=(const IDevice &) = delete;

    int version() const;

    const std::unique_ptr<Internal::IDevicePrivate> d;
    friend class DeviceManager;
};

class PROJECTEXPLORER_EXPORT DeviceTester : public QObject
{
    Q_OBJECT

public:
    enum TestResult { TestSuccess, TestFailure };

    virtual void testDevice(const ProjectExplorer::IDevice::Ptr &deviceConfiguration) = 0;
    virtual void stopTest() = 0;

signals:
    void progressMessage(const QString &message);
    void errorMessage(const QString &message);
    void finished(ProjectExplorer::DeviceTester::TestResult result);

protected:
    explicit DeviceTester(QObject *parent = nullptr);
};

class PROJECTEXPLORER_EXPORT DeviceProcessKiller : public QObject
{
    Q_OBJECT

public:
    void setProcessPath(const Utils::FilePath &path) { m_processPath = path; }
    void start();
    QString errorString() const { return m_errorString; }

signals:
    void done(bool success);

private:
    Utils::FilePath m_processPath;
    DeviceProcessSignalOperation::Ptr m_signalOperation;
    QString m_errorString;
};

class PROJECTEXPLORER_EXPORT DeviceProcessKillerTaskAdapter
    : public Tasking::TaskAdapter<DeviceProcessKiller>
{
public:
    DeviceProcessKillerTaskAdapter();
    void start() final;
};

using DeviceProcessKillerTask = Tasking::CustomTask<DeviceProcessKillerTaskAdapter>;

} // namespace ProjectExplorer
