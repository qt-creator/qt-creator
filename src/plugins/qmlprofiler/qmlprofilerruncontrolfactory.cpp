/****************************************************************************
**
** Copyright (C) 2016 Kläralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
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

#include "qmlprofilerruncontrolfactory.h"
#include "localqmlprofilerrunner.h"
#include "qmlprofilerruncontrol.h"
#include "qmlprofilerrunconfigurationaspect.h"

#include <debugger/analyzer/analyzermanager.h>
#include <debugger/analyzer/analyzerstartparameters.h>
#include <debugger/debuggerrunconfigurationaspect.h>

#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/qtcassert.h>

using namespace Debugger;
using namespace ProjectExplorer;

namespace QmlProfiler {
namespace Internal {

static bool isLocal(RunConfiguration *runConfiguration)
{
    Target *target = runConfiguration ? runConfiguration->target() : 0;
    Kit *kit = target ? target->kit() : 0;
    return DeviceTypeKitInformation::deviceTypeId(kit) == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
}

QmlProfilerRunControlFactory::QmlProfilerRunControlFactory(QObject *parent) :
    IRunControlFactory(parent)
{
}

bool QmlProfilerRunControlFactory::canRun(RunConfiguration *runConfiguration, Core::Id mode) const
{
    return mode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE && isLocal(runConfiguration);
}

RunControl *QmlProfilerRunControlFactory::create(RunConfiguration *runConfiguration, Core::Id mode, QString *errorMessage)
{
    QTC_ASSERT(canRun(runConfiguration, mode), return 0);
    QTC_ASSERT(runConfiguration->runnable().is<StandardRunnable>(), return 0);
    auto runnable = runConfiguration->runnable().as<StandardRunnable>();

    if (runnable.executable.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("No executable file to launch.");
        return 0;
    }

    Kit *kit = runConfiguration->target()->kit();
    AnalyzerConnection connection;
    const QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(kit);
    if (version) {
        if (version->qtVersion() >= QtSupport::QtVersionNumber(5, 6, 0))
            connection.analyzerSocket = LocalQmlProfilerRunner::findFreeSocket();
        else
            connection.analyzerPort = LocalQmlProfilerRunner::findFreePort(connection.analyzerHost);
    } else {
        qWarning() << "Running QML profiler on Kit without Qt version??";
        connection.analyzerPort = LocalQmlProfilerRunner::findFreePort(connection.analyzerHost);
    }

    auto runControl = qobject_cast<QmlProfilerRunControl *>
             (Debugger::createAnalyzerRunControl(runConfiguration, mode));
    QTC_ASSERT(runControl, return 0);

    runControl->setRunnable(runnable);
    runControl->setConnection(connection);

    LocalQmlProfilerRunner::Configuration conf;
    conf.debuggee = runnable;
    if (EnvironmentAspect *environment = runConfiguration->extraAspect<EnvironmentAspect>())
        conf.debuggee.environment = environment->environment();
    conf.socket = connection.analyzerSocket;
    conf.port = connection.analyzerPort;

    (void) new LocalQmlProfilerRunner(conf, runControl);
    return runControl;
}

ProjectExplorer::IRunConfigurationAspect *
QmlProfilerRunControlFactory::createRunConfigurationAspect(ProjectExplorer::RunConfiguration *rc)
{
    return new QmlProfilerRunConfigurationAspect(rc);
}

} // namespace Internal
} // namespace QmlProfiler
