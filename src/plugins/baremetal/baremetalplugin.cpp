// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "baremetalplugin.h"

#include "baremetalconstants.h"
#include "baremetaldebugsupport.h"
#include "baremetaldevice.h"
#include "baremetalrunconfiguration.h"
#include "baremetaltr.h"

#include "debugserverprovidermanager.h"

#include "iarewtoolchain.h"
#include "keiltoolchain.h"
#include "sdcctoolchain.h"

namespace BareMetal::Internal {

// BareMetalPluginPrivate

class BareMetalPluginPrivate
{
public:
    DebugServerProviderManager debugServerProviderManager;
};

// BareMetalPlugin

BareMetalPlugin::~BareMetalPlugin()
{
    delete d;
}

void BareMetalPlugin::initialize()
{
    d = new BareMetalPluginPrivate;

    setupBareMetalDevice();

    setupIarToolChain();
    setupKeilToolChain();
    setupSdccToolChain();

    setupBareMetalDeployAndRunConfigurations();
    setupBareMetalDebugSupport();
}

void BareMetalPlugin::extensionsInitialized()
{
    DebugServerProviderManager::instance()->restoreProviders();
}

} // BareMetal::Internal
