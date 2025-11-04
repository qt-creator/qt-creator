// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cargoproject.h"

#include <extensionsystem/iplugin.h>

namespace Cargo::Internal {

class CargoPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Cargo.json")

public:
    CargoPlugin() = default;
    ~CargoPlugin() final = default;

    void initialize() final;
};

void CargoPlugin::initialize()
{
    setupCargoProject();
}

} // namespace Cargo::Internal

#include "cargoplugin.moc"
