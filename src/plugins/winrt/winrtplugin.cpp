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
#include "winrtrunfactories.h"
#include "winrtdevice.h"
#include "winrtdevicefactory.h"
#include "winrtdeployconfiguration.h"
#include "winrtqtversionfactory.h"
#include "winrtrunconfiguration.h"
#include "winrtruncontrol.h"
#include "winrtdebugsupport.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>

#include <QtPlugin>
#include <QSysInfo>

using namespace ProjectExplorer;

namespace WinRt {
namespace Internal {

WinRtPlugin::WinRtPlugin()
{
    setObjectName(QLatin1String("WinRtPlugin"));
}

bool WinRtPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    addAutoReleasedObject(new Internal::WinRtRunConfigurationFactory);
    addAutoReleasedObject(new Internal::WinRtQtVersionFactory);
    addAutoReleasedObject(new Internal::WinRtDeployConfigurationFactory);
    addAutoReleasedObject(new Internal::WinRtDeployStepFactory);

    auto runConstraint = [](RunConfiguration *runConfig) {
        IDevice::ConstPtr device = DeviceKitInformation::device(runConfig->target()->kit());
        if (!device)
            return false;
        return qobject_cast<WinRtRunConfiguration *>(runConfig) != nullptr;
    };

    auto debugConstraint = [](RunConfiguration *runConfig) {
        IDevice::ConstPtr device = DeviceKitInformation::device(runConfig->target()->kit());
        if (!device)
            return false;
        if (device->type() != Internal::Constants::WINRT_DEVICE_TYPE_LOCAL)
            return false;
        return qobject_cast<WinRtRunConfiguration *>(runConfig) != nullptr;
    };

    RunControl::registerWorker<WinRtRunner>
        (ProjectExplorer::Constants::NORMAL_RUN_MODE, runConstraint);
    RunControl::registerWorker<WinRtDebugSupport>
        (ProjectExplorer::Constants::DEBUG_RUN_MODE, debugConstraint);

    return true;
}

void WinRtPlugin::extensionsInitialized()
{
    addAutoReleasedObject(new Internal::WinRtDeviceFactory);
}

} // namespace Internal
} // namespace WinRt
