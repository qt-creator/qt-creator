// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gradleproject.h"

#include <extensionsystem/iplugin.h>

namespace Gradle::Internal {

class GradlePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Gradle.json")

public:
    GradlePlugin() = default;
    ~GradlePlugin() final = default;

    void initialize() final;
};

void GradlePlugin::initialize()
{
    setupGradleProject();
}

} // namespace Gradle::Internal

#include "gradleplugin.moc"
