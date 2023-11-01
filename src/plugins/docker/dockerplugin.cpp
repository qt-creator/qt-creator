// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dockerapi.h"
#include "dockerconstants.h"
#include "dockerdevice.h"

#include <extensionsystem/iplugin.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/fsengine/fsengine.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Docker::Internal {

class DockerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Docker.json")

public:
    DockerPlugin()
    {
        FSEngine::registerDeviceScheme(Constants::DOCKER_DEVICE_SCHEME);
    }

private:
    ~DockerPlugin() final
    {
        FSEngine::unregisterDeviceScheme(Constants::DOCKER_DEVICE_SCHEME);
        m_deviceFactory->shutdownExistingDevices();
    }

    void initialize() final
    {
        m_deviceFactory = std::make_unique<DockerDeviceFactory>();
        m_dockerApi = std::make_unique<DockerApi>();
    }

    std::unique_ptr<DockerDeviceFactory> m_deviceFactory;
    std::unique_ptr<DockerApi> m_dockerApi;
};

} // Docker::Internal

#include "dockerplugin.moc"
