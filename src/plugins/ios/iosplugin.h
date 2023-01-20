// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace Ios::Internal {

class IosPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Ios.json")

public:
    IosPlugin() = default;
    ~IosPlugin() final;

private:
    void initialize() final;

    class IosPluginPrivate *d = nullptr;
};

} // Ios::Internal
