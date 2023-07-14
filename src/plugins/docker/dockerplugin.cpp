// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dockerplugin.h"

#include "dockerapi.h"
#include "dockerconstants.h"
#include "dockerdevice.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/fsengine/fsengine.h>
#include <utils/qtcassert.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Docker::Internal {

class DockerPluginPrivate
{
public:
    ~DockerPluginPrivate()
    {
        m_deviceFactory.shutdownExistingDevices();
    }

    DockerDeviceFactory m_deviceFactory;
    DockerApi m_dockerApi;
};

DockerPlugin::DockerPlugin()
{
    FSEngine::registerDeviceScheme(Constants::DOCKER_DEVICE_SCHEME);
}

DockerPlugin::~DockerPlugin()
{
    FSEngine::unregisterDeviceScheme(Constants::DOCKER_DEVICE_SCHEME);
    delete d;
}

void DockerPlugin::initialize()
{
    d = new DockerPluginPrivate;
}

} // Docker::Interanl
