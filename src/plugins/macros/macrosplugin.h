// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace Macros {
namespace Internal {

class MacrosPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Macros.json")

public:
    ~MacrosPlugin() final;

    void initialize() final;

private:
    class MacrosPluginPrivate *d = nullptr;
};

} // namespace Internal
} // namespace Macros
