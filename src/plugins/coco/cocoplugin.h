// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace Coco {

class CocoPluginPrivate;

class CocoPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Coco.json")

public:
    CocoPlugin();
    ~CocoPlugin() override;

    void initialize() override;
private:
    CocoPluginPrivate *d;
};

} // namespace Coco
