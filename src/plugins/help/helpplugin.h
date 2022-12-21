// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/helpmanager.h>
#include <extensionsystem/iplugin.h>

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Help {
namespace Internal {

class HelpWidget;
class HelpViewer;

class HelpPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Help.json")

public:
    HelpPlugin();
    ~HelpPlugin() final;

    static void showHelpUrl(const QUrl &url, Core::HelpManager::HelpViewerLocation location);
    static HelpViewer *createHelpViewer();
    static HelpWidget *modeHelpWidget();

private:
    bool initialize(const QStringList &arguments, QString *errorMessage) final;
    void extensionsInitialized() final;
    bool delayedInitialize() final;
    ShutdownFlag aboutToShutdown() final;
};

} // namespace Internal
} // namespace Help
