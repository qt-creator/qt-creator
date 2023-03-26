// Copyright (C) 2018 Benjamin Balga
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "serialoutputpane.h"
#include "serialterminalsettings.h"

#include <extensionsystem/iplugin.h>

#include <memory>

namespace SerialTerminal {
namespace Internal {

class SerialTerminalPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "SerialTerminal.json")

public:
    explicit SerialTerminalPlugin() = default;

    void initialize() final;
    ShutdownFlag aboutToShutdown() final;

private:
    void settingsChanged(const Settings &settings);

    Settings m_settings;
    std::unique_ptr<SerialOutputPane> m_serialOutputPane;
};

} // namespace Internal
} // namespace SerialTerminal
