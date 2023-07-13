// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "webassemblyplugin.h"

#ifdef WITH_TESTS
#include "webassembly_test.h"
#endif // WITH_TESTS
#include "webassemblyconstants.h"
#include "webassemblydevice.h"
#include "webassemblyqtversion.h"
#include "webassemblyrunconfiguration.h"
#include "webassemblysettings.h"
#include "webassemblytoolchain.h"
#include "webassemblytr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitmanager.h>

#include <utils/infobar.h>

#include <QTimer>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace WebAssembly::Internal {

class WebAssemblyPluginPrivate
{
public:
    WebAssemblyToolChainFactory toolChainFactory;
    WebAssemblyDeviceFactory deviceFactory;
    WebAssemblyQtVersionFactory qtVersionFactory;
    EmrunRunConfigurationFactory emrunRunConfigurationFactory;
    EmrunRunWorkerFactory emrunRunWorkerFactory;
};

static WebAssemblyPluginPrivate *dd = nullptr;

WebAssemblyPlugin::WebAssemblyPlugin()
{
    setObjectName("WebAssemblyPlugin");
}

WebAssemblyPlugin::~WebAssemblyPlugin()
{
    delete dd;
    dd = nullptr;
}

void WebAssemblyPlugin::initialize()
{
    dd = new WebAssemblyPluginPrivate;

#ifdef WITH_TESTS
    addTest<WebAssemblyTest>();
#endif // WITH_TESTS
}

void WebAssemblyPlugin::extensionsInitialized()
{
    connect(KitManager::instance(), &KitManager::kitsLoaded, this, [] {
        DeviceManager::instance()->addDevice(WebAssemblyDevice::create());
        askUserAboutEmSdkSetup();
    });
}

void WebAssemblyPlugin::askUserAboutEmSdkSetup()
{
    const char setupWebAssemblyEmSdk[] = "SetupWebAssemblyEmSdk";

    if (!ICore::infoBar()->canInfoBeAdded(setupWebAssemblyEmSdk)
            || !WebAssemblyQtVersion::isQtVersionInstalled()
            || WebAssemblyToolChain::areToolChainsRegistered())
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

} // WebAssembly::Internal
