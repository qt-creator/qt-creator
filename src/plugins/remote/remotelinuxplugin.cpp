// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customcommanddeploystep.h"
#include "flatpakdevice.h"
#include "killappstep.h"
#include "linuxdevice.h"
#include "macdevice.h"
#include "remotelinux_constants.h"
#include "remotelinuxcustomrunconfiguration.h"
#include "remotelinuxdebugsupport.h"
#include "remotelinuxdeploysupport.h"
#include "remotelinuxrunconfiguration.h"
#include "remotelinuxtr.h"
#include "sshconnectionsharing.h"
#include "tarpackagecreationstep.h"
#include "tarpackagedeploystep.h"
#include "windowsdevice.h"
#include "wslconstants.h"
#include "wsldevice.h"

#include <extensionsystem/iplugin.h>

#include <utils/async.h>
#include <utils/fsengine/fsengine.h>
#include <utils/futuresynchronizer.h>
#include <utils/hostosinfo.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>

#ifdef WITH_TESTS
#include "filesystemaccess_test.h"
#include "flatpakapi_test.h"
#include "remoterun_test.h"
#include "windowsdevicedetection_test.h"
#endif

using namespace Utils;

namespace Remote::Internal {

class RsyncToolFactory : public ProjectExplorer::DeviceToolAspectFactory
{
public:
    RsyncToolFactory()
    {
        setToolId(ProjectExplorer::Constants::RSYNC_TOOL_ID);
        setToolType(ProjectExplorer::DeviceToolAspect::BuildTool);
        setFilePattern({"rsync"});
        setLabelText(Tr::tr("Rsync executable:"));
        setDisplayName(Tr::tr("Rsync"));
    }
};

class RemoteLinuxPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Remote.json")

public:
    RemoteLinuxPlugin()
    {
        FSEngine::registerDeviceScheme(u"ssh");
        FSEngine::registerDeviceScheme(Constants::FlatpakDeviceScheme);
        if (HostOsInfo::isWindowsHost())
            FSEngine::registerDeviceScheme(Wsl::Constants::WSL_DEVICE_SCHEME);
    }

    ~RemoteLinuxPlugin() final
    {
        FSEngine::unregisterDeviceScheme(Constants::FlatpakDeviceScheme);
        FSEngine::unregisterDeviceScheme(u"ssh");
        if (HostOsInfo::isWindowsHost())
            FSEngine::unregisterDeviceScheme(Wsl::Constants::WSL_DEVICE_SCHEME);
        m_linuxDeviceFactory.reset();
    }

    void initialize() final
    {
        setupSshConnectionSharing();
        m_linuxDeviceFactory = std::make_unique<LinuxDeviceFactory>();
        m_windowsDeviceFactory = std::make_unique<WindowsDeviceFactory>();
        m_macDeviceFactory = std::make_unique<MacDeviceFactory>();
        setupFlatpakDevice();
        if (HostOsInfo::isWindowsHost())
            Wsl::Internal::setupWslDevice();
        setupRemoteLinuxRunConfiguration();
        setupRemoteLinuxCustomRunConfiguration();
        setupRemoteLinuxRunAndDebugSupport();
        setupRemoteLinuxDeploySupport();
        setupTarPackageCreationStep();
        setupTarPackageDeployStep();
        setupCustomCommandDeployStep();
        setupKillAppStep();

#ifdef WITH_TESTS
        addTest<AccessViaTest>();
        addTest<FileSystemAccessTest>();
        addTest<FlatpakApiTest>();
        addTest<RemoteRunTest>();
        addTest<WindowsDeviceDetectionTest>();
#endif
    }

    void extensionsInitialized() final
    {
        addMissingFlatpakDevice();

        if (!HostOsInfo::isWindowsHost())
            return;

        // Enumerating costs a round trip to the WSL service, which is not
        // worth holding up the start for. It starts no distribution, so the
        // devices are there by the time anything asks for one.
        QFuture<QStringList> future = Utils::asyncRun(&Wsl::Internal::detectWslDistributions);
        future.then(this, &Wsl::Internal::addMissingWslDevices);
        Utils::futureSynchronizer()->addFuture(future);
    }

private:
    std::unique_ptr<LinuxDeviceFactory> m_linuxDeviceFactory;
    std::unique_ptr<WindowsDeviceFactory> m_windowsDeviceFactory;
    std::unique_ptr<MacDeviceFactory> m_macDeviceFactory;
    RsyncToolFactory m_rsyncToolFactory;
};

} // Remote::Internal

#include "remotelinuxplugin.moc"
