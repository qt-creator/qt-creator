/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "perfoptionspage.h"
#include "perfprofilerconstants.h"
#include "perfprofilerplugin.h"
#include "perfprofilerruncontrol.h"
#include "perfprofilertool.h"
#include "perfrunconfigurationaspect.h"
#include "perftimelinemodelmanager.h"

#if WITH_TESTS
#  include "tests/perfprofilertracefile_test.h"
#  include "tests/perfresourcecounter_test.h"
#endif // WITH_TESTS

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <debugger/analyzer/analyzermanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <QAction>
#include <QDebug>
#include <QMenu>
#include <QtPlugin>

using namespace ProjectExplorer;

namespace PerfProfiler {

Q_GLOBAL_STATIC(PerfSettings, perfGlobalSettings)

bool PerfProfilerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    (void) new Internal::PerfOptionsPage(this);

    RunConfiguration::registerAspect<PerfRunConfigurationAspect>();

    (void) new Internal::PerfProfilerTool(this);

    RunControl::registerWorkerCreator(ProjectExplorer::Constants::PERFPROFILER_RUN_MODE,
        [](RunControl *runControl){ return new Internal::PerfProfilerRunner(runControl); });

    auto constraint = [](RunConfiguration *) { return true; };
    RunControl::registerWorker<Internal::PerfProfilerRunner>
            (ProjectExplorer::Constants::PERFPROFILER_RUN_MODE, constraint);

    return true;
}

void PerfProfilerPlugin::extensionsInitialized()
{
}

PerfProfiler::PerfSettings *PerfProfilerPlugin::globalSettings()
{
    return perfGlobalSettings();
}

QList<QObject *> PerfProfiler::PerfProfilerPlugin::createTestObjects() const
{
    QList<QObject *> tests;
#if WITH_TESTS
    tests << new Internal::PerfProfilerTraceFileTest;
    tests << new Internal::PerfResourceCounterTest;
#endif // WITH_TESTS
    return tests;
}

} // namespace PerfProfiler
