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
#include "tarpackagecreationstep.h"
#include "tarpackagedeploystep.h"

#include <extensionsystem/iplugin.h>

#include <utils/fsengine/fsengine.h>

#ifdef WITH_TESTS
#include "filesystemaccess_test.h"
#endif

using namespace Utils;

namespace RemoteLinux::Internal {

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
    }

    void initialize() final
    {
        setupLinuxDevice();
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
};

} // RemoteLinux::Internal

#include "remotelinuxplugin.moc"
