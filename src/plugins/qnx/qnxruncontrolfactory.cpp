/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include "qnxruncontrolfactory.h"
#include "qnxconstants.h"
#include "qnxrunconfiguration.h"
#include "qnxdebugsupport.h"
#include "qnxdevice.h"
#include "qnxanalyzesupport.h"
#include "qnxqtversion.h"
#include "slog2inforunner.h"
#include "qnxutils.h"

#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerkitinformation.h>
#include <debugger/analyzer/analyzermanager.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/portlist.h>

using namespace Debugger;
using namespace ProjectExplorer;

namespace Qnx {
namespace Internal {

QnxRunControlFactory::QnxRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

bool QnxRunControlFactory::canRun(RunConfiguration *runConfiguration, Core::Id mode) const
{
    if (mode != ProjectExplorer::Constants::NORMAL_RUN_MODE
            && mode != ProjectExplorer::Constants::DEBUG_RUN_MODE
            && mode != ProjectExplorer::Constants::QML_PROFILER_RUN_MODE) {
        return false;
    }

    if (!runConfiguration->isEnabled()
            || !runConfiguration->id().name().startsWith(Constants::QNX_QNX_RUNCONFIGURATION_PREFIX)) {
        return false;
    }

    const QnxDevice::ConstPtr dev = DeviceKitInformation::device(runConfiguration->target()->kit())
            .dynamicCast<const QnxDevice>();
    if (dev.isNull())
        return false;

    return true;
}

RunControl *QnxRunControlFactory::create(RunConfiguration *runConfig, Core::Id mode, QString *)
{
    QTC_ASSERT(canRun(runConfig, mode), return 0);

    if (mode == ProjectExplorer::Constants::NORMAL_RUN_MODE) {
        auto runControl = new RunControl(runConfig, mode);
        (void) new SimpleTargetRunner(runControl);
        return runControl;
    }

    if (mode == ProjectExplorer::Constants::DEBUG_RUN_MODE) {
        auto runControl = new RunControl(runConfig, mode);
        (void) new QnxDebugSupport(runControl);
        return runControl;
    }

    if (mode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE) {
        RunControl *runControl = new RunControl(runConfig, mode);
        runControl->createWorker(mode);
        (void) new QnxAnalyzeSupport(runControl);
        return runControl;
    }

    QTC_CHECK(false);
    return 0;
}

} // namespace Internal
} // namespace Qnx
