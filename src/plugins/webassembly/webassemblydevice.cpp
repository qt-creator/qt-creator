// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "webassemblydevice.h"

#include "webassemblyconstants.h"
#include "webassemblyqtversion.h"
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
        displayName.setDefaultValue(displayNameAndType);
        setDisplayType(displayNameAndType);
        setDeviceState(IDevice::DeviceStateUnknown);
        setMachineType(IDevice::Hardware);
        setOsType(OsTypeOther);
        setFileAccess(nullptr);
    }
};

static IDevicePtr createWebAssemblyDevice()
{
    return IDevicePtr(new WebAssemblyDevice);
}

static void askUserAboutEmSdkSetup()
{
    const char setupWebAssemblyEmSdk[] = "SetupWebAssemblyEmSdk";

    if (!ICore::infoBar()->canInfoBeAdded(setupWebAssemblyEmSdk)
            || !WebAssemblyQtVersion::isQtVersionInstalled()
            || areToolChainsRegistered())
        return;

    InfoBarEntry info(setupWebAssemblyEmSdk,
                      Tr::tr("Setup Emscripten SDK for WebAssembly? "
                             "To do it later, select Edit > Preferences > Devices > WebAssembly."),
                      InfoBarEntry::GlobalSuppression::Enabled);
    info.addCustomButton(Tr::tr("Setup Emscripten SDK"), [setupWebAssemblyEmSdk] {
        ICore::infoBar()->removeInfo(setupWebAssemblyEmSdk);
        QTimer::singleShot(0, []() { ICore::showOptionsDialog(Constants::SETTINGS_ID); });
    });
    ICore::infoBar()->addInfo(info);
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
        DeviceManager::instance()->addDevice(createWebAssemblyDevice());
        askUserAboutEmSdkSetup();
    });
}

} // WebAssembly::Internal
