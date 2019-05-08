/****************************************************************************
**
** Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "baremetalconstants.h"
#include "baremetalcustomrunconfiguration.h"
#include "baremetaldebugsupport.h"
#include "baremetaldevice.h"
#include "baremetalplugin.h"
#include "baremetalrunconfiguration.h"

#include "gdbserverprovidermanager.h"
#include "gdbserverproviderssettingspage.h"

#include "iarewtoolchain.h"
#include "keiltoolchain.h"
#include "sdcctoolchain.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

// BareMetalPluginPrivate

class BareMetalPluginPrivate
{
public:
    IarToolChainFactory iarToolChainFactory;
    KeilToolchainFactory keilToolChainFactory;
    SdccToolChainFactory sdccToolChainFactory;
    BareMetalDeviceFactory deviceFactory;
    BareMetalRunConfigurationFactory runConfigurationFactory;
    BareMetalCustomRunConfigurationFactory customRunConfigurationFactory;
    GdbServerProvidersSettingsPage gdbServerProviderSettinsPage;
    GdbServerProviderManager gdbServerProviderManager;
};

// BareMetalPlugin

BareMetalPlugin::~BareMetalPlugin()
{
    delete d;
}

bool BareMetalPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    d = new BareMetalPluginPrivate;

    auto constraint = [](RunConfiguration *runConfig) {
        const QByteArray idStr = runConfig->id().name();
        const bool res = idStr.startsWith(BareMetalRunConfiguration::IdPrefix)
                || idStr == BareMetalCustomRunConfiguration::Id;
        return res;
    };

    RunControl::registerWorker<BareMetalDebugSupport>
            (ProjectExplorer::Constants::NORMAL_RUN_MODE, constraint);
    RunControl::registerWorker<BareMetalDebugSupport>
            (ProjectExplorer::Constants::DEBUG_RUN_MODE, constraint);

    return true;
}

void BareMetalPlugin::extensionsInitialized()
{
    GdbServerProviderManager::instance()->restoreProviders();
}

} // namespace Internal
} // namespace BareMetal
