// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyosdevice.h"

#include "harmonyosconstants.h"
#include "harmonyossdk.h"
#include "harmonyossettings.h"
#include "harmonyostr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/devicemanager.h>

#include <utils/algorithm.h>
#include <utils/devicefileaccess.h>
#include <utils/processinterface.h>
#include <utils/qtcprocess.h>
#include <utils/result.h>
#include <utils/synchronizedvalue.h>
#include <utils/temporaryfile.h>

#include <QInputDialog>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QTimer>

using namespace ProjectExplorer;
using namespace Utils;
using namespace std::chrono_literals;

namespace HarmonyOs::Internal {

static Q_LOGGING_CATEGORY(deviceLog, "qtc.harmonyos.device", QtWarningMsg)

static Result<QStringList> connectedSerialNumbers(const FilePath &hdc);
static void updateDeviceState(const IDevice::Ptr &device);
static DeviceTester *createHarmonyOsDeviceTester(const IDevice::Ptr &device);

// Shared, because both users outlive a single call and run in other threads.
class AccessData
{
public:
    SynchronizedValue<QString> serialNumber;
    SynchronizedValue<FilePath> hdcCommand;
};

static const char s_exitCodeMarker[] = "__qtc_exit_code=";

class HarmonyOsFileAccess final : public UnixDeviceFileAccess
{
public:
    explicit HarmonyOsFileAccess(const std::shared_ptr<AccessData> &data)
        : m_data(data)
    {}

    // hdc merges the remote stderr into stdout, so dd's report would end up in the
    // data. Read the file with the transfer instead.
    Result<QByteArray> fileContents(const FilePath &filePath, qint64 limit, qint64 offset)
        const final
    {
        TemporaryFile local("qtc-harmonyos-XXXXXX");
        if (!local.open())
            return ResultError(local.errorString());
        local.close();

        if (const Result<> transferred = transfer({"file", "recv"}, filePath.path(),
                                                  local.filePath().nativePath());
            !transferred) {
            return ResultError(transferred.error());
        }

        const Result<QByteArray> contents = local.filePath().fileContents(limit, offset);
        if (!contents)
            return ResultError(contents.error());
        return *contents;
    }

    // hdc shell does not forward stdin, so anything that would be piped in goes
    // over the file transfer instead.
    Result<qint64> writeFileContents(const FilePath &filePath, const QByteArray &data) const final
    {
        TemporaryFile local("qtc-harmonyos-XXXXXX");
        if (!local.open())
            return ResultError(local.errorString());
        if (local.write(data) != data.size())
            return ResultError(local.errorString());
        local.close();

        if (const Result<> transferred = transfer({"file", "send"},
                                                  local.filePath().nativePath(), filePath.path());
            !transferred) {
            return ResultError(transferred.error());
        }
        return data.size();
    }

    Result<> transfer(const QStringList &mode, const QString &from, const QString &to) const
    {
        CommandLine cmd(*m_data->hdcCommand.readLocked());
        cmd.addArgs({"-t", *m_data->serialNumber.readLocked()});
        cmd.addArgs(mode);
        cmd.addArgs({from, to});

        Process process;
        process.setCommand(cmd);
        process.runBlocking();
        const QString output = process.allOutput();
        qCDebug(deviceLog) << cmd.toUserOutput() << "->" << output;
        // hdc reports a failed transfer in its output, not in its exit code.
        if (process.result() != ProcessResult::FinishedWithSuccess || output.contains("[Fail]"))
            return ResultError(output.trimmed());
        return ResultOk;
    }

protected:
    Result<RunResult> runInShellImpl(const CommandLine &cmdLine,
                                     const QByteArray &inputData) const final
    {
        CommandLine cmd(*m_data->hdcCommand.readLocked());
        cmd.addArgs({"-t", *m_data->serialNumber.readLocked(), "shell"});
        // hdc exits successfully whatever the command did, so have the shell report
        // the status itself.
        CommandLine remote = cmdLine;
        remote.addArgs(QString("; echo %1$?").arg(QLatin1String(s_exitCodeMarker)), CommandLine::Raw);
        cmd.addCommandLineAsSingleArg(remote);

        Process process;
        process.setWriteData(inputData);
        process.setCommand(cmd);
        process.runBlocking();
        QByteArray stdOut = process.readAllRawStandardOutput();
        int exitCode = process.resultData().m_exitCode;
        const qsizetype markerAt = stdOut.lastIndexOf(s_exitCodeMarker);
        if (markerAt >= 0) {
            exitCode = stdOut.mid(markerAt + qstrlen(s_exitCodeMarker)).trimmed().toInt();
            stdOut.truncate(markerAt);
        }

        const RunResult result{exitCode, stdOut, process.readAllRawStandardError()};
        qCDebug(deviceLog) << cmd.toUserOutput() << "->" << result.exitCode
                           << "out:" << result.stdOut << "err:" << result.stdErr;
        return result;
    }

private:
    std::shared_ptr<AccessData> m_data;
};

class HarmonyOsDevice::Private
{
public:
    std::shared_ptr<AccessData> accessData = std::make_shared<AccessData>();
    std::shared_ptr<HarmonyOsFileAccess> fileAccess;
};

HarmonyOsDevice::~HarmonyOsDevice() = default;

HarmonyOsDevice::HarmonyOsDevice()
    : d(new Private)
{
    setType(Constants::HARMONYOS_DEVICE_TYPE);
    setDefaultDisplayName(Tr::tr("Run on HarmonyOS"));
    setDisplayType(Tr::tr("HarmonyOS Device"));
    setMachineType(IDevice::Hardware);
    setOsType(OsTypeOtherUnix);
    setDeviceState(IDevice::DeviceStateUnknown);

    addDeviceAction({Tr::tr("Refresh"), [](const IDevice::Ptr &device) {
        updateDeviceState(device);
    }});
}

IDevice::Ptr HarmonyOsDevice::create()
{
    return IDevice::Ptr(new HarmonyOsDevice);
}

IDeviceWidget *HarmonyOsDevice::createWidget()
{
    return nullptr;
}

DeviceTester *HarmonyOsDevice::createDeviceTester()
{
    return createHarmonyOsDeviceTester(shared_from_this());
}

QString HarmonyOsDevice::serialNumber() const
{
    return extraData(Constants::HARMONYOS_SERIAL_NUMBER).toString();
}

void HarmonyOsDevice::setSerialNumber(const QString &serial)
{
    setExtraData(Constants::HARMONYOS_SERIAL_NUMBER, serial);
    *d->accessData->serialNumber.writeLocked() = serial;
    updateFileAccess();
}

void HarmonyOsDevice::updateFileAccess()
{
    const FilePath hdc = Sdk::hdcCommand(settings().sdkLocation());
    *d->accessData->hdcCommand.writeLocked() = hdc;

    if (hdc.isEmpty() || serialNumber().isEmpty() || deviceState() != IDevice::DeviceReadyToUse) {
        setFileAccess(nullptr);
        d->fileAccess.reset();
        return;
    }
    if (!d->fileAccess) {
        d->fileAccess = std::make_shared<HarmonyOsFileAccess>(d->accessData);
        setFileAccess(d->fileAccess);
    }
}

ProcessInterface *HarmonyOsDevice::createProcessInterface() const
{
    const FilePath hdc = *d->accessData->hdcCommand.readLocked();
    const QString serial = *d->accessData->serialNumber.readLocked();
    const auto wrapCommandLine = [hdc, serial](const ProcessSetupData &setupData,
                                               const QString &pidMarker) -> Result<CommandLine> {
        if (hdc.isEmpty())
            return ResultError(Tr::tr("No hdc command is available."));
        CommandLine cmd(hdc);
        cmd.addArgs({"-t", serial, "shell"});
        CommandLine inner("echo", {pidMarker.arg("1234")}); // No pid to report, as with Android.
        if (!setupData.rawWorkingDirectory().isEmpty())
            inner.addCommandLineWithAnd({"cd", {setupData.rawWorkingDirectory().path()}});
        inner.addCommandLineWithAnd(setupData.m_commandLine);
        inner.addArgs(QString("; echo %1$?").arg(QLatin1String(s_exitCodeMarker)), CommandLine::Raw);
        cmd.addCommandLineAsSingleArg(inner);
        return cmd;
    };
    const auto controlSignal = [](ControlSignal, qint64) {
        // hdc shell leaves no process of ours behind to signal.
    };
    return new WrappedProcessInterface(wrapCommandLine, controlSignal, s_exitCodeMarker);
}

static QStringList serialNumbers(const QString &hdcOutput)
{
    QStringList serials;
    for (const QString &line : hdcOutput.split('\n', Qt::SkipEmptyParts)) {
        const QString serial = line.trimmed();
        if (!serial.isEmpty() && serial != "[Empty]")
            serials.append(serial);
    }
    return serials;
}

// Runs "hdc list targets" and returns the serial numbers of the connected devices.
static Result<QStringList> connectedSerialNumbers(const FilePath &hdc)
{
    Process process;
    process.setCommand({hdc, {"list", "targets"}});
    process.runBlocking(5s);
    if (process.result() != ProcessResult::FinishedWithSuccess)
        return ResultError(process.exitMessage());

    return serialNumbers(process.cleanedStdOut());
}

// Re-checks with hdc whether this device is still attached and updates its state.
// There is no hdc device-tracking stream, so this runs on demand (the Refresh action).
static void updateDeviceState(const IDevice::Ptr &device)
{
    const FilePath hdc = Sdk::hdcCommand(settings().sdkLocation());
    if (hdc.isEmpty()) {
        QMessageBox::warning(
            Core::ICore::dialogParent(),
            Tr::tr("HarmonyOS"),
            Tr::tr("No HarmonyOS SDK is configured. Set it up in "
                   "Preferences > SDKs > HarmonyOS."));
        return;
    }
    const Result<QStringList> serials = connectedSerialNumbers(hdc);
    if (!serials) {
        DeviceManager::setDeviceState(device->id(), IDevice::DeviceStateUnknown);
        return;
    }
    const QString serial = static_cast<HarmonyOsDevice *>(device.get())->serialNumber();
    const bool connected = serials->contains(serial);
    DeviceManager::setDeviceState(device->id(),
                                  connected ? IDevice::DeviceReadyToUse
                                            : IDevice::DeviceDisconnected);
}

// Checks with hdc whether this device is attached. Doubles as the way to refresh a
// device's state, which is otherwise only done by the Refresh action.
class HarmonyOsDeviceTester final : public DeviceTester
{
public:
    explicit HarmonyOsDeviceTester(const IDevice::Ptr &device)
        : DeviceTester(device)
    {
        connect(&m_process, &Process::done,
                this, &HarmonyOsDeviceTester::handleDone);
    }

    void testDevice() final
    {
        const FilePath hdc = Sdk::hdcCommand(settings().sdkLocation());
        if (hdc.isEmpty()) {
            emit errorMessage(Tr::tr("No HarmonyOS SDK is configured. Set it up in "
                                     "Preferences > SDKs > HarmonyOS."));
            emit finished(TestFailure);
            return;
        }
        emit progressMessage(Tr::tr("Looking for device \"%1\"...").arg(serialNumber()));
        m_process.setCommand({hdc, {"list", "targets"}});
        m_process.start();
    }

    void stopTest() final { m_process.close(); }

private:
    QString serialNumber() const
    {
        return static_cast<HarmonyOsDevice *>(device().get())->serialNumber();
    }

    void handleDone()
    {
        if (m_process.result() != ProcessResult::FinishedWithSuccess) {
            emit errorMessage(Tr::tr("Failed to query HarmonyOS devices: %1")
                                  .arg(m_process.exitMessage()));
            DeviceManager::setDeviceState(device()->id(), IDevice::DeviceStateUnknown);
            emit finished(TestFailure);
            return;
        }

        const QStringList serials = m_process.cleanedStdOut().split('\n', Qt::SkipEmptyParts);
        const bool connected = Utils::anyOf(serials, [this](const QString &line) {
            return line.trimmed() == serialNumber();
        });
        DeviceManager::setDeviceState(device()->id(), connected ? IDevice::DeviceReadyToUse
                                                               : IDevice::DeviceDisconnected);
        if (connected)
            emit progressMessage(Tr::tr("The device is connected."));
        else
            emit errorMessage(Tr::tr("The device is not connected."));
        emit finished(connected ? TestSuccess : TestFailure);
    }

    Process m_process;
};

static DeviceTester *createHarmonyOsDeviceTester(const IDevice::Ptr &device)
{
    return new HarmonyOsDeviceTester(device);
}

// Detects connected devices with hdc when the user adds a device from the Devices page.
static IDevice::Ptr createHarmonyOsDevice()
{
    const FilePath hdc = Sdk::hdcCommand(settings().sdkLocation());
    if (hdc.isEmpty()) {
        QMessageBox::warning(
            Core::ICore::dialogParent(),
            Tr::tr("HarmonyOS"),
            Tr::tr("No HarmonyOS SDK is configured. Set it up in "
                   "Preferences > SDKs > HarmonyOS."));
        return {};
    }

    const Result<QStringList> serials = connectedSerialNumbers(hdc);
    if (!serials) {
        QMessageBox::warning(
            Core::ICore::dialogParent(),
            Tr::tr("HarmonyOS"),
            Tr::tr("Failed to query HarmonyOS devices: %1").arg(serials.error()));
        return {};
    }
    if (serials->isEmpty()) {
        QMessageBox::information(
            Core::ICore::dialogParent(),
            Tr::tr("HarmonyOS"),
            Tr::tr("No connected HarmonyOS devices were found."));
        return {};
    }

    QString serial = serials->first();
    if (serials->size() > 1) {
        bool ok = false;
        serial = QInputDialog::getItem(
            Core::ICore::dialogParent(),
            Tr::tr("Select HarmonyOS Device"),
            Tr::tr("Device:"),
            *serials,
            0,
            false,
            &ok);
        if (!ok)
            return {};
    }

    const IDevice::Ptr device = HarmonyOsDevice::create();
    auto harmonyDevice = static_cast<HarmonyOsDevice *>(device.get());
    harmonyDevice->setupId(IDevice::ManuallyAdded,
                           Utils::Id::fromString(QString("HarmonyOS.Device.%1").arg(serial)));
    harmonyDevice->setDisplayName(Tr::tr("HarmonyOS Device (%1)").arg(serial));
    harmonyDevice->setSerialNumber(serial);
    harmonyDevice->setDeviceState(IDevice::DeviceReadyToUse);
    return device;
}

// Device detection
//
// hdc has no device-tracking stream, so attached devices are found by asking it
// every few seconds. Polling only happens while an SDK is configured.
class HarmonyOsDeviceDetector final : public QObject
{
public:
    HarmonyOsDeviceDetector()
    {
        m_timer.setInterval(5s);
        connect(&m_timer, &QTimer::timeout, this, &HarmonyOsDeviceDetector::poll);
        connect(&m_process, &Process::done, this, [this] {
            if (m_process.result() == ProcessResult::FinishedWithSuccess)
                updateDevices(serialNumbers(m_process.cleanedStdOut()));
        });
        connect(&settings(), &AspectContainer::applied, this, [this] { poll(); });
        poll();
        m_timer.start();
    }

private:
    void poll()
    {
        if (m_process.state() != ProcessState::NotRunning)
            return;
        const FilePath hdc = Sdk::hdcCommand(settings().sdkLocation());
        if (hdc.isEmpty())
            return;
        m_process.setCommand({hdc, {"list", "targets"}});
        m_process.start();
    }

    static void updateDevices(const QStringList &serials)
    {
        for (const QString &serial : serials) {
            const Id id = deviceId(serial);
            if (DeviceManager::find(id))
                continue;
            const IDevice::Ptr device = HarmonyOsDevice::create();
            auto harmonyDevice = static_cast<HarmonyOsDevice *>(device.get());
            harmonyDevice->setupId(IDevice::AutoDetected, id);
            harmonyDevice->setDisplayName(Tr::tr("HarmonyOS Device (%1)").arg(serial));
            harmonyDevice->setSerialNumber(serial);
            harmonyDevice->setDeviceState(IDevice::DeviceReadyToUse);
            DeviceManager::addDevice(device);
        }

        // Devices that went away stay in the list, so that kits and run configurations
        // keep pointing at them, but they are no longer ready to use.
        DeviceManager::forEachDevice([&serials](const IDeviceConstPtr &device) {
            if (device->type() != Constants::HARMONYOS_DEVICE_TYPE)
                return;
            const QString serial
                = static_cast<const HarmonyOsDevice *>(device.get())->serialNumber();
            const bool connected = !serial.isEmpty() && serials.contains(serial);
            DeviceManager::setDeviceState(device->id(),
                                         connected ? IDevice::DeviceReadyToUse
                                                   : IDevice::DeviceDisconnected);
        });
    }

    static Id deviceId(const QString &serial)
    {
        return Id::fromString(QString("HarmonyOS.Device.%1").arg(serial));
    }

    QTimer m_timer;
    Process m_process;
};

void setupHarmonyOsDeviceDetection()
{
    static HarmonyOsDeviceDetector theHarmonyOsDeviceDetector;

    QObject::connect(DeviceManager::instance(), &DeviceManager::deviceUpdated,
                     DeviceManager::instance(), [](const Id &id) {
        if (const IDevice::Ptr device = DeviceManager::find(id)) {
            if (const auto harmonyDevice = dynamic_cast<HarmonyOsDevice *>(device.get()))
                harmonyDevice->updateFileAccess();
        }
    });
}

// Factory

HarmonyOsDeviceFactory::HarmonyOsDeviceFactory()
    : IDeviceFactory(Constants::HARMONYOS_DEVICE_TYPE)
{
    setDisplayName(Tr::tr("HarmonyOS Device"));
    setConstructionFunction(&HarmonyOsDevice::create);
    setCreator(&createHarmonyOsDevice);
}

void setupHarmonyOsDevice()
{
    static HarmonyOsDeviceFactory theHarmonyOsDeviceFactory;
}

} // namespace HarmonyOs::Internal
