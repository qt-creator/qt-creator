// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace WebAssembly::Internal {

class WebAssemblyPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "WebAssembly.json")

public:
    WebAssemblyPlugin();
    ~WebAssemblyPlugin() override;

    void initialize() override;
    void extensionsInitialized() override;
    static void askUserAboutEmSdkSetup();
};

} // WebAssembly::Internal
