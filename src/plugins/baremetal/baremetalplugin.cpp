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

// GDB debug servers.
#include "debugservers/gdb/genericgdbserverprovider.h"
#include "debugservers/gdb/openocdgdbserverprovider.h"
#include "debugservers/gdb/stlinkutilgdbserverprovider.h"
#include "debugservers/gdb/jlinkgdbserverprovider.h"
#include "debugservers/gdb/eblinkgdbserverprovider.h"

// UVSC debug servers.
#include "debugservers/uvsc/simulatoruvscserverprovider.h"
#include "debugservers/uvsc/stlinkuvscserverprovider.h"
#include "debugservers/uvsc/jlinkuvscserverprovider.h"

#include <extensionsystem/iplugin.h>

namespace BareMetal::Internal {

class BareMetalPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "BareMetal.json")

    void initialize() final
    {
        setupBareMetalDevice();

        setupGenericGdbServerProvider();
        setupJLinkGdbServerProvider();
        setupOpenOcdGdbServerProvider();
        setupStLinkUtilGdbServerProvider();
        setupEBlinkGdbServerProvider();
        setupSimulatorUvscServerProvider();
        setupStLinkUvscServerProvider();
        setupJLinkUvscServerProvider();

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
