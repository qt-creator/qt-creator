// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dotnetproject.h"

#include <extensionsystem/iplugin.h>

namespace Dotnet::Internal {

class DotnetPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Dotnet.json")

public:
    DotnetPlugin() = default;
    ~DotnetPlugin() final = default;

    void initialize() final;
};

void DotnetPlugin::initialize()
{
    setupDotnetProject();
}

} // namespace Dotnet::Internal

#include "dotnetplugin.moc"
