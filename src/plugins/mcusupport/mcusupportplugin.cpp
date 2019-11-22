/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "mcusupportplugin.h"
#include "mcusupportconstants.h"
#include "mcusupportdevice.h"
#include "mcusupportoptionspage.h"
#include "mcusupportrunconfiguration.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/jsonwizard/jsonwizardfactory.h>

using namespace ProjectExplorer;

namespace McuSupport {
namespace Internal {

class McuSupportPluginPrivate
{
public:
    McuSupportDeviceFactory deviceFactory;
    McuSupportRunConfigurationFactory runConfigurationFactory;
    RunWorkerFactory runWorkerFactory{
        makeFlashAndRunWorker(),
        {ProjectExplorer::Constants::NORMAL_RUN_MODE},
        {Constants::RUNCONFIGURATION}
    };
    McuSupportOptionsPage optionsPage;
};

static McuSupportPluginPrivate *dd = nullptr;

McuSupportPlugin::McuSupportPlugin()
{
    setObjectName("McuSupportPlugin");
}

McuSupportPlugin::~McuSupportPlugin()
{
    delete dd;
    dd = nullptr;
}

bool McuSupportPlugin::initialize(const QStringList& arguments, QString* errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    dd = new McuSupportPluginPrivate;

    ProjectExplorer::JsonWizardFactory::addWizardPath(
                Utils::FilePath::fromString(":/mcusupport/wizards/"));

    return true;
}

void McuSupportPlugin::extensionsInitialized()
{
    ProjectExplorer::DeviceManager::instance()->addDevice(McuSupportDevice::create());
}

} // namespace Internal
} // namespace McuSupport
