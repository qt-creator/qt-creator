// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "westbuildstep.h"
#include "zephyrdebug.h"
#include "zephyrproject.h"
#include "zephyrrun.h"

#include <extensionsystem/iplugin.h>

namespace Zephyr::Internal {

class ZephyrPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Zephyr.json")

public:
    void initialize() final
    {
        setupWestBuildSteps();
        setupZephyrProject();
        setupZephyrRun();
        setupZephyrDebug();
    }
};

} // namespace Zephyr::Internal

#include <zephyrplugin.moc>
