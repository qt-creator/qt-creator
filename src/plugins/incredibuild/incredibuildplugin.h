// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace IncrediBuild::Internal {

class IncrediBuildPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "IncrediBuild.json")

public:
    IncrediBuildPlugin() = default;
    ~IncrediBuildPlugin() final;

    void initialize() final;

private:
    class IncrediBuildPluginPrivate *d = nullptr;
};

} // IncrediBuild::Internal
