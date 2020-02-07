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
//#  include "tests/perfprofilertracefile_test.h"    // FIXME has to be rewritten
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

using namespace ProjectExplorer;

namespace PerfProfiler {
namespace Internal {

Q_GLOBAL_STATIC(PerfSettings, perfGlobalSettings)

class PerfProfilerPluginPrivate
{
public:
    PerfProfilerPluginPrivate()
    {
        RunConfiguration::registerAspect<PerfRunConfigurationAspect>();
    }

    RunWorkerFactory profilerWorkerFactory{
        RunWorkerFactory::make<PerfProfilerRunner>(),
        {ProjectExplorer::Constants::PERFPROFILER_RUN_MODE}
    };

    PerfOptionsPage optionsPage{perfGlobalSettings()};
    PerfProfilerTool profilerTool;
};

PerfProfilerPlugin::~PerfProfilerPlugin()
{
    delete d;
}

bool PerfProfilerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    d = new PerfProfilerPluginPrivate;
    return true;
}

PerfSettings *PerfProfilerPlugin::globalSettings()
{
    return perfGlobalSettings();
}

QVector<QObject *> PerfProfilerPlugin::createTestObjects() const
{
    QVector<QObject *> tests;
#if WITH_TESTS
//    tests << new PerfProfilerTraceFileTest;  // FIXME these tests have to get rewritten
    tests << new PerfResourceCounterTest;
#endif // WITH_TESTS
    return tests;
}

} // namespace Internal
} // namespace PerfProfiler
