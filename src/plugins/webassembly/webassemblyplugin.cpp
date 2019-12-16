/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "webassemblyplugin.h"
#include "webassemblyconstants.h"
#include "webassemblydevice.h"
#include "webassemblyqtversion.h"
#include "webassemblyrunconfiguration.h"
#include "webassemblytoolchain.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitmanager.h>

using namespace ProjectExplorer;

namespace WebAssembly {
namespace Internal {

class WebAssemblyPluginPrivate
{
public:
    WebAssemblyToolChainFactory toolChainFactory;
    WebAssemblyDeviceFactory deviceFactory;
    WebAssemblyQtVersionFactory qtVersionFactory;
    EmrunRunConfigurationFactory emrunRunConfigurationFactory;
    RunWorkerFactory emrunRunWorkerFactory{
        makeEmrunWorker(),
        {ProjectExplorer::Constants::NORMAL_RUN_MODE},
        {Constants::WEBASSEMBLY_RUNCONFIGURATION_EMRUN}
    };
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

bool WebAssemblyPlugin::initialize(const QStringList& arguments, QString* errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    dd = new WebAssemblyPluginPrivate;

    return true;
}

void WebAssemblyPlugin::extensionsInitialized()
{
    connect(KitManager::instance(), &KitManager::kitsLoaded, this, [] {
        DeviceManager::instance()->addDevice(WebAssemblyDevice::create());
    });
}

} // namespace Internal
} // namespace WebAssembly
