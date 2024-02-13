// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "baremetaldebugsupport.h"
#include "baremetaldevice.h"
#include "baremetalrunconfiguration.h"

#include "debugserverprovidermanager.h"

#include "iarewparser.h"
#include "iarewtoolchain.h"
#include "keilparser.h"
#include "keiltoolchain.h"
#include "sdccparser.h"
#include "sdcctoolchain.h"

#include <extensionsystem/iplugin.h>

namespace BareMetal::Internal {

class BareMetalPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "BareMetal.json")

    void initialize() final
    {
        setupBareMetalDevice();

        setupIarToolchain();
        setupKeilToolchain();
        setupSdccToolchain();

        setupBareMetalDeployAndRunConfigurations();
        setupBareMetalDebugSupport();

#ifdef WITH_TESTS
        addTestCreator(createIarParserTest);
        addTestCreator(createKeilParserTest);
        addTestCreator(createSdccParserTest);
#endif
    }

    void extensionsInitialized() final
    {
        setupDebugServerProviderManager(this);
    }
};

} // BareMetal::Internal

#include "baremetalplugin.moc"
