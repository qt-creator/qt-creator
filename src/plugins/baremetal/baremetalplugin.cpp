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

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>

using namespace ProjectExplorer;

namespace BareMetal::Internal {

class BareMetalDeployConfigurationFactory : public DeployConfigurationFactory
{
public:
    BareMetalDeployConfigurationFactory()
    {
        setConfigBaseId("BareMetal.DeployConfiguration");
        setDefaultDisplayName(Tr::tr("Deploy to BareMetal Device"));
        addSupportedTargetDeviceType(Constants::BareMetalOsType);
    }
};

// BareMetalPluginPrivate

class BareMetalPluginPrivate
{
public:
    IarToolChainFactory iarToolChainFactory;
    KeilToolChainFactory keilToolChainFactory;
    SdccToolChainFactory sdccToolChainFactory;
    BareMetalDeviceFactory deviceFactory;
    BareMetalRunConfigurationFactory runConfigurationFactory;
    BareMetalCustomRunConfigurationFactory customRunConfigurationFactory;
    DebugServerProviderManager debugServerProviderManager;
    BareMetalDeployConfigurationFactory deployConfigurationFactory;
    BareMetalDebugSupportFactory runWorkerFactory;
};

// BareMetalPlugin

BareMetalPlugin::~BareMetalPlugin()
{
    delete d;
}

void BareMetalPlugin::initialize()
{
    d = new BareMetalPluginPrivate;
}

void BareMetalPlugin::extensionsInitialized()
{
    DebugServerProviderManager::instance()->restoreProviders();
}

} // BareMetal::Internal
