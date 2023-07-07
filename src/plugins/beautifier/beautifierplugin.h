// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>
#include <texteditor/command.h>

namespace Beautifier::Internal {

class BeautifierPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Beautifier.json")

    void initialize() override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;
};

} // Beautifier::Internal
