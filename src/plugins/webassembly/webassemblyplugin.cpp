// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "webassemblyplugin.h"

#ifdef WITH_TESTS
#include "webassembly_test.h"
#endif // WITH_TESTS
#include "webassemblydevice.h"
#include "webassemblyqtversion.h"
#include "webassemblyrunconfiguration.h"
#include "webassemblytoolchain.h"

#include <extensionsystem/iplugin.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace WebAssembly::Internal {

class WebAssemblyPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "WebAssembly.json")

public:
    void initialize() final
    {
        setupWebAssemblyToolchain();
        setupWebAssemblyDevice();
        setupWebAssemblyQtVersion();
        setupEmrunRunSupport();

#ifdef WITH_TESTS
        addTest<WebAssemblyTest>();
#endif // WITH_TESTS
    }
};

} // WebAssembly::Internal

#include "webassemblyplugin.moc"
