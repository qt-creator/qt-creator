// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfoptionspage.h"
#include "perfprofilerplugin.h"
#include "perfprofilerruncontrol.h"
#include "perfprofilertool.h"
#include "perfrunconfigurationaspect.h"

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
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <QAction>
#include <QDebug>
#include <QMenu>

using namespace ProjectExplorer;

namespace PerfProfiler::Internal {

Q_GLOBAL_STATIC(PerfSettings, perfGlobalSettings)

class PerfProfilerPluginPrivate
{
public:
    PerfProfilerPluginPrivate()
    {
        RunConfiguration::registerAspect<PerfRunConfigurationAspect>();
    }

    PerfProfilerRunWorkerFactory profilerWorkerFactory;
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

} // PerfProfiler::Internal
