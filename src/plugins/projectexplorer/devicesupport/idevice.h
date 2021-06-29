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

#pragma once

#include "../projectexplorer_export.h"

#include <utils/id.h>
#include <utils/hostosinfo.h>

#include <QAbstractSocket>
#include <QCoreApplication>
#include <QDir>
#include <QList>
#include <QObject>
#include <QSharedPointer>
#include <QUrl>
#include <QVariantMap>

#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace QSsh { class SshConnectionParameters; }

namespace Utils {
class CommandLine;
class Environment;
class FilePath;
class Icon;
class PortList;
class Port;
class QtcProcess;
} // Utils

namespace ProjectExplorer {

class Connection;
class DeviceProcess;
class DeviceProcessList;
class Kit;
class Runnable;

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

    void setDebuggerCommand(const QString &cmd);

signals:
    // If the error message is empty the operation was successful
    void finished(const QString &errorMessage);

protected:
    explicit DeviceProcessSignalOperation();

    QString m_debuggerCommand;
    QString m_errorMessage;
};

class PROJECTEXPLORER_EXPORT DeviceEnvironmentFetcher : public QObject
{
    Q_OBJECT
public:
    using Ptr = QSharedPointer<DeviceEnvironmentFetcher>;

    virtual void start() = 0;

signals:
    void finished(const Utils::Environment &env, bool success);

protected:
    explicit DeviceEnvironmentFetcher();
};

class PROJECTEXPLORER_EXPORT PortsGatheringMethod
{
public:
    using Ptr = QSharedPointer<const PortsGatheringMethod>;

    virtual ~PortsGatheringMethod() = default;
    virtual Runnable runnable(QAbstractSocket::NetworkLayerProtocol protocol) const = 0;
    virtual QList<Utils::Port> usedPorts(const QByteArray &commandOutput) const = 0;
};

// See cpp file for documentation.
class PROJECTEXPLORER_EXPORT IDevice : public QEnableSharedFromThis<IDevice>
{
    friend class Internal::IDevicePrivate;
public:
    using Ptr = QSharedPointer<IDevice>;
    using ConstPtr = QSharedPointer<const IDevice>;

    enum Origin { ManuallyAdded, AutoDetected };
    enum MachineType { Hardware, Emulator };

    virtual ~IDevice();

    Ptr clone() const;

    QString displayName() const;
    void setDisplayName(const QString &name);
    void setDefaultDisplayName(const QString &name);

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

    QString displayType() const;
    Utils::OsType osType() const;

    virtual IDeviceWidget *createWidget() = 0;

    struct DeviceAction {
        QString display;
        std::function<void(const IDevice::Ptr &device, QWidget *parent)> execute;
    };
    void addDeviceAction(const DeviceAction &deviceAction);
    const QList<DeviceAction> deviceActions() const;

    // Devices that can auto detect ports need not return a ports gathering method. Such devices can
    // obtain a free port on demand. eg: Desktop device.
    virtual bool canAutoDetectPorts() const { return false; }
    virtual PortsGatheringMethod::Ptr portsGatheringMethod() const;
    virtual bool canCreateProcessModel() const { return false; }
    virtual DeviceProcessList *createProcessListModel(QObject *parent = nullptr) const;
    virtual bool hasDeviceTester() const { return false; }
    virtual DeviceTester *createDeviceTester() const;

    virtual bool canCreateProcess() const { return false; }
    virtual DeviceProcess *createProcess(QObject *parent) const;
    virtual DeviceProcessSignalOperation::Ptr signalOperation() const = 0;
    virtual DeviceEnvironmentFetcher::Ptr environmentFetcher() const;

    enum DeviceState { DeviceReadyToUse, DeviceConnected, DeviceDisconnected, DeviceStateUnknown };
    DeviceState deviceState() const;
    void setDeviceState(const DeviceState state);
    QString deviceStateToString() const;

    virtual void fromMap(const QVariantMap &map);
    virtual QVariantMap toMap() const;

    static Utils::Id typeFromMap(const QVariantMap &map);
    static Utils::Id idFromMap(const QVariantMap &map);

    static QString defaultPrivateKeyFilePath();
    static QString defaultPublicKeyFilePath();

    QSsh::SshConnectionParameters sshParameters() const;
    void setSshParameters(const QSsh::SshConnectionParameters &sshParameters);

    enum ControlChannelHint { QmlControlChannel };
    virtual QUrl toolControlChannel(const ControlChannelHint &) const;

    Utils::PortList freePorts() const;
    void setFreePorts(const Utils::PortList &freePorts);

    MachineType machineType() const;
    void setMachineType(MachineType machineType);

    QString debugServerPath() const;
    void setDebugServerPath(const QString &path);

    QString qmlsceneCommand() const;
    void setQmlsceneCommand(const QString &path);

    void setExtraData(Utils::Id kind, const QVariant &data);
    QVariant extraData(Utils::Id kind) const;

    void setupId(Origin origin, Utils::Id id = Utils::Id());

    bool canOpenTerminal() const;
    void openTerminal(const Utils::Environment &env, const QString &workingDir) const;

    bool isEmptyCommandAllowed() const;
    void setAllowEmptyCommand(bool allow);

    bool isWindowsDevice() const { return osType() == Utils::OsTypeWindows; }
    bool isLinuxDevice() const { return osType() == Utils::OsTypeLinux; }
    bool isMacDevice() const { return osType() == Utils::OsTypeMac; }
    bool isAnyUnixDevice() const;

    virtual Utils::FilePath mapToGlobalPath(const Utils::FilePath &pathOnDevice) const;

    virtual bool handlesFile(const Utils::FilePath &filePath) const;
    virtual bool isExecutableFile(const Utils::FilePath &filePath) const;
    virtual bool isReadableFile(const Utils::FilePath &filePath) const;
    virtual bool isWritableFile(const Utils::FilePath &filePath) const;
    virtual bool isReadableDirectory(const Utils::FilePath &filePath) const;
    virtual bool isWritableDirectory(const Utils::FilePath &filePath) const;
    virtual bool ensureWritableDirectory(const Utils::FilePath &filePath) const;
    virtual bool ensureExistingFile(const Utils::FilePath &filePath) const;
    virtual bool createDirectory(const Utils::FilePath &filePath) const;
    virtual bool exists(const Utils::FilePath &filePath) const;
    virtual bool removeFile(const Utils::FilePath &filePath) const;
    virtual bool removeRecursively(const Utils::FilePath &filePath) const;
    virtual bool copyFile(const Utils::FilePath &filePath, const Utils::FilePath &target) const;
    virtual bool renameFile(const Utils::FilePath &filePath, const Utils::FilePath &target) const;
    virtual Utils::FilePath searchInPath(const Utils::FilePath &filePath) const;
    virtual QList<Utils::FilePath> directoryEntries(const Utils::FilePath &filePath,
                                                    const QStringList &nameFilters,
                                                    QDir::Filters filters) const;
    virtual QByteArray fileContents(const Utils::FilePath &filePath, int limit) const;
    virtual bool writeFileContents(const Utils::FilePath &filePath, const QByteArray &data) const;
    virtual QDateTime lastModified(const Utils::FilePath &filePath) const;
    virtual QFile::Permissions permissions(const Utils::FilePath &filePath) const;
    virtual void runProcess(Utils::QtcProcess &process) const;
    virtual Utils::Environment systemEnvironment() const;

    virtual void aboutToBeRemoved() const {}

protected:
    IDevice();

    using OpenTerminal = std::function<void(const Utils::Environment &, const QString &)>;
    void setOpenTerminal(const OpenTerminal &openTerminal);
    void setDisplayType(const QString &type);
    void setOsType(Utils::OsType osType);

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

} // namespace ProjectExplorer
