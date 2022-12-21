// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dockerapi.h"

#include <extensionsystem/iplugin.h>

#include <optional>

namespace Docker::Internal {

class DockerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Docker.json")

public:
    DockerPlugin();

    static DockerApi *dockerApi();

private:
    ~DockerPlugin() final;

    bool initialize(const QStringList &arguments, QString *errorString) final;

    class DockerPluginPrivate *d = nullptr;
};

} // Docker::Internal
