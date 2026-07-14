// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyosdevice.h"

#include "harmonyosconstants.h"
#include "harmonyossdk.h"
#include "harmonyossettings.h"
#include "harmonyostr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/devicemanager.h>

#include <utils/qtcprocess.h>
#include <utils/result.h>

#include <QInputDialog>
#include <QMessageBox>

using namespace ProjectExplorer;
using namespace Utils;
using namespace std::chrono_literals;

namespace HarmonyOs::Internal {

static Result<QStringList> connectedSerialNumbers(const FilePath &hdc);
static void updateDeviceState(const IDevice::Ptr &device);

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

QString HarmonyOsDevice::serialNumber() const
{
    return extraData(Constants::HARMONYOS_SERIAL_NUMBER).toString();
}

void HarmonyOsDevice::setSerialNumber(const QString &serial)
{
    setExtraData(Constants::HARMONYOS_SERIAL_NUMBER, serial);
}

// Runs "hdc list targets" and returns the serial numbers of the connected devices.
static Result<QStringList> connectedSerialNumbers(const FilePath &hdc)
{
    Process process;
    process.setCommand({hdc, {"list", "targets"}});
    process.runBlocking(5s);
    if (process.result() != ProcessResult::FinishedWithSuccess)
        return ResultError(process.exitMessage());

    QStringList serials;
    const QString output = process.cleanedStdOut();
    for (const QString &line : output.split('\n', Qt::SkipEmptyParts)) {
        const QString serial = line.trimmed();
        if (!serial.isEmpty() && serial != "[Empty]")
            serials.append(serial);
    }
    return serials;
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
