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
#include <utils/qtcprocess.h>
#include <utils/result.h>

#include <QInputDialog>
#include <QMessageBox>
#include <QTimer>

using namespace ProjectExplorer;
using namespace Utils;
using namespace std::chrono_literals;

namespace HarmonyOs::Internal {

static Result<QStringList> connectedSerialNumbers(const FilePath &hdc);
static void updateDeviceState(const IDevice::Ptr &device);
static DeviceTester *createHarmonyOsDeviceTester(const IDevice::Ptr &device);

HarmonyOsDevice::HarmonyOsDevice()
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
