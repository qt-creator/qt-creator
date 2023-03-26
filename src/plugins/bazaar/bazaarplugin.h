// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace Bazaar::Internal {

class BazaarPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Bazaar.json")

    ~BazaarPlugin() final;

    void initialize() final;
    void extensionsInitialized() final;

#ifdef WITH_TESTS
private slots:
    void testDiffFileResolving_data();
    void testDiffFileResolving();
    void testLogResolving();
#endif

private:
    class BazaarPluginPrivate *d = nullptr;
};

} // Bazaar::Internal
