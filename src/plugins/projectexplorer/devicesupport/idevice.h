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

#include <coreplugin/id.h>
#include <utils/hostosinfo.h>

#include <QAbstractSocket>
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
class Environment;
class Icon;
class PortList;
class Port;
} // Utils

namespace ProjectExplorer {

class Connection;
class DeviceProcess;
class DeviceProcessList;
class Kit;
class Runnable;
class RunControl;
class RunWorker;

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
public:
    using Ptr = QSharedPointer<IDevice>;
    using ConstPtr = QSharedPointer<const IDevice>;

    enum Origin { ManuallyAdded, AutoDetected };
    enum MachineType { Hardware, Emulator };

    IDevice &operator=(const IDevice &) = delete;
    virtual ~IDevice();

    QString displayName() const;
    void setDisplayName(const QString &name);

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

    Core::Id type() const;
    void setType(Core::Id type);

    bool isAutoDetected() const;
    Core::Id id() const;

    virtual bool isCompatibleWith(const Kit *k) const;

    virtual QString displayType() const = 0;
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
    virtual Utils::OsType osType() const;

    virtual bool canCreateProcess() const { return false; }
    virtual DeviceProcess *createProcess(QObject *parent) const;
    virtual DeviceProcessSignalOperation::Ptr signalOperation() const = 0;
    virtual DeviceEnvironmentFetcher::Ptr environmentFetcher() const;

    virtual std::function<RunWorker *(RunControl *)> workerCreator(Core::Id) const { return {}; }

    enum DeviceState { DeviceReadyToUse, DeviceConnected, DeviceDisconnected, DeviceStateUnknown };
    DeviceState deviceState() const;
    void setDeviceState(const DeviceState state);
    QString deviceStateToString() const;

    virtual void fromMap(const QVariantMap &map);
    virtual QVariantMap toMap() const;
    virtual Ptr clone() const = 0;

    static Core::Id typeFromMap(const QVariantMap &map);
    static Core::Id idFromMap(const QVariantMap &map);

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

    void setExtraData(Core::Id kind, const QVariant &data);
    QVariant extraData(Core::Id kind) const;

    void setupId(Origin origin, Core::Id id = Core::Id());

protected:
    IDevice();
    IDevice(const IDevice &other);

private:
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
