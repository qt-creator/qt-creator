// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "webassembly_global.h"

#include <extensionsystem/iplugin.h>

namespace WebAssembly::Internal {

class WebAssemblyPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "WebAssembly.json")

public:
    WebAssemblyPlugin();
    ~WebAssemblyPlugin() override;

    bool initialize(const QStringList &arguments, QString *errorString) override;
    void extensionsInitialized() override;
    static void askUserAboutEmSdkSetup();

#ifdef WITH_TESTS
private slots:
    void testEmSdkEnvParsing();
    void testEmSdkEnvParsing_data();
    void testEmrunBrowserListParsing();
    void testEmrunBrowserListParsing_data();
#endif // WITH_TESTS
};

} // WebAssembly::Internal
