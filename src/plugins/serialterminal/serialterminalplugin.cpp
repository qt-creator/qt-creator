// Copyright (C) 2018 Benjamin Balga
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "serialcontrol.h"
#include "serialoutputpane.h"
#include "serialterminalsettings.h"

#include <coreplugin/icore.h>

#include <extensionsystem/iplugin.h>

#include <memory>

namespace SerialTerminal::Internal {

class SerialTerminalPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "SerialTerminal.json")

public:
    explicit SerialTerminalPlugin() = default;

    void initialize() final
    {
        m_settings.load(Core::ICore::settings());

        // Create serial output pane
        m_serialOutputPane = std::make_unique<SerialOutputPane>(m_settings);
        connect(m_serialOutputPane.get(), &SerialOutputPane::settingsChanged,
                this, &SerialTerminalPlugin::settingsChanged);

        connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
                this, [this] { m_settings.save(Core::ICore::settings()); });
    }

    ShutdownFlag aboutToShutdown() final
    {
        m_serialOutputPane->closeTabs(SerialOutputPane::CloseTabNoPrompt);

        return SynchronousShutdown;
    }

    void settingsChanged(const Settings &settings)
    {
        m_settings = settings;
        m_settings.save(Core::ICore::settings());

        m_serialOutputPane->setSettings(m_settings);
    }

    Settings m_settings;
    std::unique_ptr<SerialOutputPane> m_serialOutputPane;
};

} // SerialTerminal::Internal

#include "serialterminalplugin.moc"
