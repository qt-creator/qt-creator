// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customcommanddeploystep.h"
#include "killappstep.h"
#include "linuxdevice.h"
#include "remotelinux_constants.h"
#include "remotelinuxcustomrunconfiguration.h"
#include "remotelinuxdebugsupport.h"
#include "remotelinuxdeploysupport.h"
#include "remotelinuxrunconfiguration.h"
#include "remotelinuxtr.h"
#include "tarpackagecreationstep.h"
#include "tarpackagedeploystep.h"

#include <extensionsystem/iplugin.h>

#include <utils/fsengine/fsengine.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>

#ifdef WITH_TESTS
#include "filesystemaccess_test.h"
#endif

using namespace Utils;

namespace RemoteLinux::Internal {

class RsyncToolFactory : public ProjectExplorer::DeviceToolAspectFactory
{
public:
    RsyncToolFactory()
    {
        setToolId(ProjectExplorer::Constants::RSYNC_TOOL_ID);
        setToolType(ProjectExplorer::DeviceToolAspect::BuildTool);
        setFilePattern({"rsync"});
        setLabelText(Tr::tr("Rsync executable:"));
    }
};

class RemoteLinuxPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "RemoteLinux.json")

public:
    RemoteLinuxPlugin()
    {
        FSEngine::registerDeviceScheme(u"ssh");
    }

    ~RemoteLinuxPlugin() final
    {
        FSEngine::unregisterDeviceScheme(u"ssh");
        m_linuxDeviceFactory.reset();
    }

    void initialize() final
    {
        m_linuxDeviceFactory = std::make_unique<LinuxDeviceFactory>();
        setupRemoteLinuxRunConfiguration();
        setupRemoteLinuxCustomRunConfiguration();
        setupRemoteLinuxRunAndDebugSupport();
        setupRemoteLinuxDeploySupport();
        setupTarPackageCreationStep();
        setupTarPackageDeployStep();
        setupCustomCommandDeployStep();
        setupKillAppStep();

#ifdef WITH_TESTS
        addTest<FileSystemAccessTest>();
#endif
    }

private:
    std::unique_ptr<LinuxDeviceFactory> m_linuxDeviceFactory;
    RsyncToolFactory m_rsyncToolFactory;
};

} // RemoteLinux::Internal

#include "remotelinuxplugin.moc"
