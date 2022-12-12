// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#pragma once

#include <extensionsystem/iplugin.h>

namespace Axivion::Internal {

class AxivionPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Axivion.json")

public:
    AxivionPlugin() = default;
    ~AxivionPlugin() final;

private:
    bool initialize(const QStringList &arguments, QString *errorMessage) final;
    void extensionsInitialized() final {}

    class AxivionPluginPrivate *d = nullptr;
};

} // Axivion::Internal

