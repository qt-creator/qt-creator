// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace Nim {

class NimPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Nim.json")

public:
    NimPlugin() = default;
    ~NimPlugin() final;

    void initialize() final;
    void extensionsInitialized() final;

#ifdef WITH_TESTS
private slots:
    void testNimParser_data();
    void testNimParser();
#endif

private:
    class NimPluginPrivate *d = nullptr;
};

} // Nim
