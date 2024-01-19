// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "haskell_global.h"

#include <extensionsystem/iplugin.h>

namespace Haskell {
namespace Internal {

class HaskellPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Haskell.json")

public:
    HaskellPlugin() = default;
    ~HaskellPlugin() final;

private:
    bool initialize(const QStringList &arguments, QString *errorString) final;
    void extensionsInitialized() final {}

    class HaskellPluginPrivate *d = nullptr;
};

} // namespace Internal
} // namespace Haskell
