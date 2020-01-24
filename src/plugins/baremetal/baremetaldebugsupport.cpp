/****************************************************************************
**
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

#include "baremetaldebugsupport.h"
#include "baremetaldevice.h"

#include "debugserverprovidermanager.h"
#include "idebugserverprovider.h"

#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal {
namespace Internal {

// BareMetalDebugSupport

BareMetalDebugSupport::BareMetalDebugSupport(RunControl *runControl)
    : Debugger::DebuggerRunTool(runControl)
{
    const auto dev = qSharedPointerCast<const BareMetalDevice>(device());
    if (!dev) {
        reportFailure(tr("Cannot debug: Kit has no device."));
        return;
    }

    const QString providerId = dev->debugServerProviderId();
    IDebugServerProvider *p = DebugServerProviderManager::findProvider(providerId);
    if (!p) {
        reportFailure(tr("No debug server provider found for %1").arg(providerId));
        return;
    }

    if (RunWorker *runner = p->targetRunner(runControl))
        addStartDependency(runner);
}

void BareMetalDebugSupport::start()
{
    const auto dev = qSharedPointerCast<const BareMetalDevice>(device());
    QTC_ASSERT(dev, reportFailure(); return);
    IDebugServerProvider *p = DebugServerProviderManager::findProvider(
                dev->debugServerProviderId());
    QTC_ASSERT(p, reportFailure(); return);

    QString errorMessage;
    if (!p->aboutToRun(this, errorMessage))
        reportFailure(errorMessage);
    else
        DebuggerRunTool::start();
}

} // namespace Internal
} // namespace BareMetal
