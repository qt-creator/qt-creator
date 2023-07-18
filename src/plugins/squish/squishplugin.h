// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace Squish {
namespace Internal {

class SquishPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Squish.json")
public:
    SquishPlugin() = default;
    ~SquishPlugin() override;

    void initialize() override;
    bool delayedInitialize() override;
    ShutdownFlag aboutToShutdown() override;
};

} // namespace Internal
} // namespace Squish
