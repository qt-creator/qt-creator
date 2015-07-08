/****************************************************************************
**
** Copyright (C) 2015 Kläralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlprofilerruncontrolfactory.h"
#include "localqmlprofilerrunner.h"
#include "qmlprofilerruncontrol.h"

#include <analyzerbase/ianalyzertool.h>

#include <debugger/debuggerrunconfigurationaspect.h>

#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/localapplicationrunconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <utils/qtcassert.h>

using namespace Analyzer;
using namespace ProjectExplorer;

namespace QmlProfiler {
namespace Internal {

QmlProfilerRunControlFactory::QmlProfilerRunControlFactory(QObject *parent) :
    IRunControlFactory(parent)
{
}

bool QmlProfilerRunControlFactory::canRun(RunConfiguration *runConfiguration, Core::Id mode) const
{
    return mode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE
            && (qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration));
}

static AnalyzerStartParameters createQmlProfilerStartParameters(RunConfiguration *runConfiguration)
{
    AnalyzerStartParameters sp;
    EnvironmentAspect *environment = runConfiguration->extraAspect<EnvironmentAspect>();

    // FIXME: This is only used to communicate the connParams settings.
    LocalApplicationRunConfiguration *rc =
                qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc, return sp);
    if (environment)
        sp.environment = environment->environment();
    sp.workingDirectory = rc->workingDirectory();
    sp.debuggee = rc->executable();
    sp.debuggeeArgs = rc->commandLineArguments();
    sp.displayName = rc->displayName();
    sp.analyzerPort = LocalQmlProfilerRunner::findFreePort(sp.analyzerHost);

    return sp;
}

RunControl *QmlProfilerRunControlFactory::create(RunConfiguration *runConfiguration, Core::Id mode, QString *errorMessage)
{
    QTC_ASSERT(canRun(runConfiguration, mode), return 0);

    AnalyzerStartParameters sp = createQmlProfilerStartParameters(runConfiguration);
    sp.runMode = mode;
    return LocalQmlProfilerRunner::createLocalRunControl(runConfiguration, sp, errorMessage);
}

} // namespace Internal
} // namespace QmlProfiler
