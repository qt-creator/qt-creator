// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace FakeVim {
namespace Internal {

class FakeVimHandler;

class FakeVimPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "FakeVim.json")

public:
    FakeVimPlugin();
    ~FakeVimPlugin() override;

private:
    // implementation of ExtensionSystem::IPlugin
    void initialize() override;
    ShutdownFlag aboutToShutdown() override;
    void extensionsInitialized() override;

private:
    friend class FakeVimPluginPrivate;

};

} // namespace Internal
} // namespace FakeVim
