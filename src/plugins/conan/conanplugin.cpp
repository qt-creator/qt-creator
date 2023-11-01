// Copyright (C) 2018 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "conaninstallstep.h"

#include <extensionsystem/iplugin.h>

namespace Conan::Internal {

class ConanPluginPrivate
{
public:
    ConanInstallStepFactory conanInstallStepFactory;
};

class ConanPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Conan.json")

    void initialize()
    {
        d = std::make_unique<ConanPluginPrivate>();
    }

    std::unique_ptr<ConanPluginPrivate> d;
};

} // Conan::Internal

#include "conanplugin.moc"
