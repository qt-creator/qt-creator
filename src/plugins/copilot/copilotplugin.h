// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "copilotclient.h"

#include <extensionsystem/iplugin.h>

#include <QPointer>

namespace Copilot {
namespace Internal {

class CopilotPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Copilot.json")

public:
    void initialize() override;
    bool delayedInitialize() override;
    void restartClient();
    ShutdownFlag aboutToShutdown() override;

private:
    QPointer<CopilotClient> m_client;
};

} // namespace Internal
} // namespace Copilot
