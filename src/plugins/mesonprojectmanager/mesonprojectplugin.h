// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace MesonProjectManager {
namespace Internal {

class MesonProjectPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "MesonProjectManager.json")

public:
    ~MesonProjectPlugin() final;

private:
    bool initialize(const QStringList &arguments, QString *errorMessage) final;

    class MesonProjectPluginPrivate *d = nullptr;
};

} // namespace Internal
} // namespace MesonProjectManager
