// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbaseplugin.h>

namespace Fossil {
namespace Internal {

class FossilClient;

class FossilPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Fossil.json")

    ~FossilPlugin() final;

    bool initialize(const QStringList &arguments, QString *errorMessage) final;
    void extensionsInitialized() final;

public:
    static FossilClient *client();

#ifdef WITH_TESTS
private slots:
    void testDiffFileResolving_data();
    void testDiffFileResolving();
    void testLogResolving();
#endif
};

} // namespace Internal
} // namespace Fossil
