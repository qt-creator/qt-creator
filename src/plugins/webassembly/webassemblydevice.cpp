// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "webassemblydevice.h"

#include "webassemblyconstants.h"
#include "webassemblyqtversion.h"
#include "webassemblysettings.h"
#include "webassemblytoolchain.h"
#include "webassemblytr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/desktopdevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitmanager.h>

#include <utils/infobar.h>

#include <QTimer>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace WebAssembly::Internal {

class WebAssemblyDevice final : public DesktopDevice
{
public:
    WebAssemblyDevice()
    {
        setupId(IDevice::AutoDetected, Constants::WEBASSEMBLY_DEVICE_DEVICE_ID);
        setType(Constants::WEBASSEMBLY_DEVICE_TYPE);
        const QString displayNameAndType = Tr::tr("Web Browser");
        setDefaultDisplayName(displayNameAndType);
        setDisplayType(displayNameAndType);
        setMachineType(IDevice::Hardware);
        setOsType(OsTypeOther);
        setFileAccess(nullptr);
        setDeviceState(IDevice::DeviceStateUnknown);
    }

    bool canCreateProcessModel() const override
    {
        return false;
    }

    Utils::Result<> handlesFile(const Utils::FilePath &) const final
    {
        return Utils::ResultError(Tr::tr("File handling is not supported."));
    }
};

static IDevicePtr createWebAssemblyDevice()
{
    return IDevicePtr(new WebAssemblyDevice);
}

static void askUserAboutEmSdkSetup()
{
    const char setupWebAssemblyEmSdk[] = "SetupWebAssemblyEmSdk";

    InfoBar *infoBar = ICore::popupInfoBar();
    if (!infoBar->canInfoBeAdded(setupWebAssemblyEmSdk)
            || !WebAssemblyQtVersion::isQtVersionInstalled()
            || areToolChainsRegistered())
        return;

    InfoBarEntry info(setupWebAssemblyEmSdk,
                      Tr::tr("Setup Emscripten SDK for WebAssembly? "
                             "To do it later, select Edit > Preferences > Devices > WebAssembly."),
                      InfoBarEntry::GlobalSuppression::Enabled);
    info.setTitle(Tr::tr("Set up WebAssembly?"));
    info.setInfoType(InfoLabel::Information);
    info.addCustomButton(
        Tr::tr("Setup Emscripten SDK"),
        [] { QTimer::singleShot(0, []() { ICore::showSettings(Constants::SETTINGS_ID); }); },
        {},
        InfoBarEntry::ButtonAction::Hide);
    infoBar->addInfo(info);
}

class WebAssemblyDeviceFactory final : public IDeviceFactory
{
public:
    WebAssemblyDeviceFactory()
        : IDeviceFactory(Constants::WEBASSEMBLY_DEVICE_TYPE)
    {
        setDisplayName(Tr::tr("WebAssembly Runtime"));
        setCombinedIcon(":/webassembly/images/webassemblydevicesmall.png",
                        ":/webassembly/images/webassemblydevice.png");
        setConstructionFunction(&createWebAssemblyDevice);
        setCreator(&createWebAssemblyDevice);
    }
};

void setupWebAssemblyDevice()
{
    static WebAssemblyDeviceFactory theWebAssemblyDeviceFactory;

    QObject::connect(KitManager::instance(), &KitManager::kitsLoaded, [] {
        DeviceManager::addDevice(createWebAssemblyDevice());
        askUserAboutEmSdkSetup();
        DeviceManager::setDeviceState(
            Constants::WEBASSEMBLY_DEVICE_DEVICE_ID,
            areToolChainsRegistered() ? IDevice::DeviceReadyToUse : IDevice::DeviceDisconnected);
    });
}

} // WebAssembly::Internal
