// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "wslconstants.h"
#include "wsldevice.h"

#include <extensionsystem/iplugin.h>

#include <utils/async.h>
#include <utils/fsengine/fsengine.h>
#include <utils/futuresynchronizer.h>

namespace Wsl::Internal {

class WslPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Wsl.json")

public:
    WslPlugin() { Utils::FSEngine::registerDeviceScheme(Constants::WSL_DEVICE_SCHEME); }

private:
    ~WslPlugin() final
    {
        Utils::FSEngine::unregisterDeviceScheme(Constants::WSL_DEVICE_SCHEME);
        if (m_deviceFactory)
            m_deviceFactory->shutdownExistingDevices();
    }

    void initialize() final
    {
        m_deviceFactory = std::make_unique<WslDeviceFactory>();
    }

    void extensionsInitialized() final
    {
        // Enumerating costs a round trip to the WSL service, which is not
        // worth holding up the start for. It starts no distribution, so the
        // devices are there by the time anything asks for one.
        QFuture<QStringList> future = Utils::asyncRun(&detectWslDistributions);
        future.then(this, &addMissingWslDevices);
        Utils::futureSynchronizer()->addFuture(future);
    }

    std::unique_ptr<WslDeviceFactory> m_deviceFactory;
};

} // namespace Wsl::Internal

#include "wslplugin.moc"
