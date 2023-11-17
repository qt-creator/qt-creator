// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxplugin.h"

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

#include <utils/fsengine/fsengine.h>

#ifdef WITH_TESTS
#include "filesystemaccess_test.h"
#endif

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

RemoteLinuxPlugin::RemoteLinuxPlugin()
{
    setObjectName(QLatin1String("RemoteLinuxPlugin"));
    FSEngine::registerDeviceScheme(u"ssh");
}

RemoteLinuxPlugin::~RemoteLinuxPlugin()
{
    FSEngine::unregisterDeviceScheme(u"ssh");
}

void RemoteLinuxPlugin::initialize()
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

} // namespace Internal
} // namespace RemoteLinux
