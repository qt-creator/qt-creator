// Copyright (C) 2018 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "conaninstallstep.h"

#include <extensionsystem/iplugin.h>

namespace Conan::Internal {

class ConanPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Conan.json")

    void initialize() final
    {
        setupConanInstallStep();
    }
};

} // Conan::Internal

#include "conanplugin.moc"
