// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "antproject.h"

#include <extensionsystem/iplugin.h>

namespace Ant::Internal {

class AntPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Ant.json")

public:
    AntPlugin() = default;
    ~AntPlugin() final = default;

    void initialize() final;
};

void AntPlugin::initialize()
{
    setupAntProject();
}

} // namespace Ant::Internal

#include "antplugin.moc"
