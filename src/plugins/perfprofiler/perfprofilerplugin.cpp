// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfprofilerplugin.h"

#include "perfprofilerruncontrol.h"
#include "perfprofilertool.h"
#include "perfrunconfigurationaspect.h"

#if WITH_TESTS
//#  include "tests/perfprofilertracefile_test.h"    // FIXME has to be rewritten
#  include "tests/perfresourcecounter_test.h"
#endif // WITH_TESTS


using namespace ProjectExplorer;

namespace PerfProfiler::Internal {

class PerfProfilerPluginPrivate
{
public:
    PerfProfilerPluginPrivate()
    {
        RunConfiguration::registerAspect<PerfRunConfigurationAspect>();
    }

    PerfProfilerRunWorkerFactory profilerWorkerFactory;
    PerfProfilerTool profilerTool;
};

PerfProfilerPlugin::~PerfProfilerPlugin()
{
    delete d;
}

void PerfProfilerPlugin::initialize()
{
    d = new PerfProfilerPluginPrivate;

#if WITH_TESTS
//   addTest<PerfProfilerTraceFileTest>();  // FIXME these tests have to get rewritten
    addTest<PerfResourceCounterTest>();
#endif // WITH_TESTS
}

} // PerfProfiler::Internal
