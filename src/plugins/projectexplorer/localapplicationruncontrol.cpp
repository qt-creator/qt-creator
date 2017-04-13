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

#include "localapplicationruncontrol.h"
#include "runnables.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>

#include <utils/utilsicons.h>
#include <utils/qtcassert.h>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

static bool isLocal(RunConfiguration *runConfiguration)
{
    Target *target = runConfiguration ? runConfiguration->target() : nullptr;
    Kit *kit = target ? target->kit() : nullptr;
    return DeviceTypeKitInformation::deviceTypeId(kit) == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
}

bool LocalApplicationRunControlFactory::canRun(RunConfiguration *runConfiguration, Core::Id mode) const
{
    if (mode != Constants::NORMAL_RUN_MODE)
        return false;
    const Runnable runnable = runConfiguration->runnable();
    if (!runnable.is<StandardRunnable>())
        return false;
    const IDevice::ConstPtr device = runnable.as<StandardRunnable>().device;
    if (device && device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
        return true;
    return isLocal(runConfiguration);
}

RunControl *LocalApplicationRunControlFactory::create(RunConfiguration *runConfiguration, Core::Id mode, QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    auto runControl = new RunControl(runConfiguration, mode);
    (void) new SimpleTargetRunner(runControl);
    return runControl;
}

} // namespace Internal
} // namespace ProjectExplorer
