// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dockerplugin.h"

#include "dockerapi.h"
#include "dockerconstants.h"
#include "dockerdevice.h"
#include "dockersettings.h"

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
    ~DockerPluginPrivate() {
        m_deviceFactory.shutdownExistingDevices();
    }

    DockerSettings m_settings;
    DockerDeviceFactory m_deviceFactory{&m_settings};
    DockerSettingsPage m_settingPage{&m_settings};
    DockerApi m_dockerApi{&m_settings};
};

static DockerPlugin *s_instance = nullptr;

DockerPlugin::DockerPlugin()
{
    s_instance = this;
    FSEngine::registerDeviceScheme(Constants::DOCKER_DEVICE_SCHEME);
}

DockerApi *DockerPlugin::dockerApi()
{
    QTC_ASSERT(s_instance, return nullptr);
    return &s_instance->d->m_dockerApi;
}

DockerPlugin::~DockerPlugin()
{
    FSEngine::unregisterDeviceScheme(Constants::DOCKER_DEVICE_SCHEME);
    s_instance = nullptr;
    delete d;
}

bool DockerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    d = new DockerPluginPrivate;

    return true;
}

} // Docker::Interanl
