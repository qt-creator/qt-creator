/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "winrtplugin.h"

#include "winrtconstants.h"
#include "winrtdebugsupport.h"
#include "winrtdeployconfiguration.h"
#include "winrtdevice.h"
#include "winrtpackagedeploymentstep.h"
#include "winrtphoneqtversion.h"
#include "winrtqtversion.h"
#include "winrtrunconfiguration.h"
#include "winrtruncontrol.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;

namespace WinRt {
namespace Internal {

class WinRtPluginPrivate
{
public:
    WinRtRunConfigurationFactory runConfigFactory;
    WinRtQtVersionFactory qtVersionFactory;
    WinRtPhoneQtVersionFactory phoneQtVersionFactory;
    WinRtAppDeployConfigurationFactory appDeployConfigFactory;
    WinRtPhoneDeployConfigurationFactory phoneDeployConfigFactory;
    WinRtEmulatorDeployConfigurationFactory emulatorDeployFactory;
    WinRtDeployStepFactory deployStepFactory;
    WinRtDeviceFactory localDeviceFactory{Constants::WINRT_DEVICE_TYPE_LOCAL};
    WinRtDeviceFactory phoneDeviceFactory{Constants::WINRT_DEVICE_TYPE_PHONE};
    WinRtDeviceFactory emulatorDeviceFactory{Constants::WINRT_DEVICE_TYPE_EMULATOR};

    RunWorkerFactory runWorkerFactory{
        RunWorkerFactory::make<WinRtRunner>(),
        {ProjectExplorer::Constants::NORMAL_RUN_MODE},
        {runConfigFactory.runConfigurationId()}
    };

    RunWorkerFactory debugWorkerFactory{
        RunWorkerFactory::make<WinRtDebugSupport>(),
        {ProjectExplorer::Constants::DEBUG_RUN_MODE},
        {runConfigFactory.runConfigurationId()},
        {Internal::Constants::WINRT_DEVICE_TYPE_LOCAL}
    };
};

WinRtPlugin::~WinRtPlugin()
{
    delete d;
}

bool WinRtPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    d = new WinRtPluginPrivate;

    return true;
}

} // namespace Internal
} // namespace WinRt
