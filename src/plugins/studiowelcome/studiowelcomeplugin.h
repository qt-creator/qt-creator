// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <QTimer>

namespace StudioWelcome {
namespace Internal {

class StudioWelcomePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "StudioWelcome.json")

public slots:
    void closeSplashScreen();

public:
    StudioWelcomePlugin();
    ~StudioWelcomePlugin() final;

    void initialize() override;
    void extensionsInitialized() override;
    bool delayedInitialize() override;
    void setTelemetryEnabled(bool value);

private:
    class WelcomeMode *m_welcomeMode = nullptr;
    bool m_telemetryEnabled = false;
};

} // namespace Internal
} // namespace StudioWelcome
