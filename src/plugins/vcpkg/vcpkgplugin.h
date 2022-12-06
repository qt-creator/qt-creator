// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace ProjectExplorer { class Project; }

namespace Vcpkg::Internal {

class VcpkgPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Vcpkg.json")

public:
    ~VcpkgPlugin();

    void initialize() final;

private:
    class VcpkgPluginPrivate *d = nullptr;
};

} // namespace Vcpkg::Internal
