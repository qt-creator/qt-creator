// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace Terminal {

class TerminalPane;
namespace Internal {

class TerminalPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Terminal.json")

public:
    TerminalPlugin();
    ~TerminalPlugin() override;

    void extensionsInitialized() override;
    bool delayedInitialize() override;

private:
    TerminalPane *m_terminalPane{nullptr};
};

} // namespace Internal
} // namespace Terminal
