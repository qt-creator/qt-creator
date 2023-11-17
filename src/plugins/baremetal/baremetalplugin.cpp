// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "baremetalplugin.h"

#include "baremetaldebugsupport.h"
#include "baremetaldevice.h"
#include "baremetalrunconfiguration.h"

#include "debugserverprovidermanager.h"

#include "iarewtoolchain.h"
#include "keiltoolchain.h"
#include "sdcctoolchain.h"

namespace BareMetal::Internal {

// BareMetalPlugin

void BareMetalPlugin::initialize()
{
    setupBareMetalDevice();

    setupIarToolChain();
    setupKeilToolChain();
    setupSdccToolChain();

    setupBareMetalDeployAndRunConfigurations();
    setupBareMetalDebugSupport();
}

void BareMetalPlugin::extensionsInitialized()
{
    setupDebugServerProviderManager(this);
}

} // BareMetal::Internal
