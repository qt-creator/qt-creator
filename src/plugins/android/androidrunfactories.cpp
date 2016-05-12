/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidrunfactories.h"

#include "androidconstants.h"
#include "androiddebugsupport.h"
#include "androidanalyzesupport.h"
#include "androidrunconfiguration.h"
#include "androidruncontrol.h"
#include "androidmanager.h"

#include <debugger/debuggerconstants.h>

using namespace ProjectExplorer;

namespace Android {
namespace Internal {

AndroidRunControlFactory::AndroidRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

bool AndroidRunControlFactory::canRun(RunConfiguration *runConfiguration, Core::Id mode) const
{
    if (mode != ProjectExplorer::Constants::NORMAL_RUN_MODE
            && mode != ProjectExplorer::Constants::DEBUG_RUN_MODE
            && mode != ProjectExplorer::Constants::DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN
            && mode != ProjectExplorer::Constants::QML_PROFILER_RUN_MODE) {
        return false;
    }
    return qobject_cast<AndroidRunConfiguration *>(runConfiguration);
}

RunControl *AndroidRunControlFactory::create(RunConfiguration *runConfig,
                                        Core::Id mode, QString *errorMessage)
{
    Q_ASSERT(canRun(runConfig, mode));
    AndroidRunConfiguration *rc = qobject_cast<AndroidRunConfiguration *>(runConfig);
    Q_ASSERT(rc);
    if (mode == ProjectExplorer::Constants::NORMAL_RUN_MODE)
        return new AndroidRunControl(rc);
    if (mode == ProjectExplorer::Constants::DEBUG_RUN_MODE || mode == ProjectExplorer::Constants::DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN)
        return AndroidDebugSupport::createDebugRunControl(rc, errorMessage);
    if (mode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE)
        return AndroidAnalyzeSupport::createAnalyzeRunControl(rc, mode);
    QTC_CHECK(false); // The other run modes are not supported
    return 0;
}

} // namespace Internal
} // namespace Android
