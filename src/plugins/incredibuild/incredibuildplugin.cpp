// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildconsolebuildstep.h"
#include "ibconsolebuildstep.h"

#include <extensionsystem/iplugin.h>

namespace IncrediBuild::Internal {

class IncrediBuildPluginPrivate
{
public:
    BuildConsoleStepFactory buildConsoleStepFactory;
    IBConsoleStepFactory ibConsolStepFactory;
};

class IncrediBuildPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "IncrediBuild.json")

public:
    void initialize()
    {
        d = std::make_unique<IncrediBuildPluginPrivate>();
    }

    std::unique_ptr<IncrediBuildPluginPrivate> d;
};

} // IncrediBuild::Internal

#include "incredibuildplugin.moc"
