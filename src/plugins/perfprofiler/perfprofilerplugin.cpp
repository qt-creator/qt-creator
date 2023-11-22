// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfprofilerruncontrol.h"
#include "perfprofilertool.h"
#include "perfrunconfigurationaspect.h"

#if WITH_TESTS
//#  include "tests/perfprofilertracefile_test.h"    // FIXME has to be rewritten
#  include "tests/perfresourcecounter_test.h"
#endif // WITH_TESTS

#include <extensionsystem/iplugin.h>

namespace PerfProfiler::Internal {

class PerfProfilerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "PerfProfiler.json")

    void initialize() final
    {
        setupPerfProfilerTool();
        setupPerfProfilerRunWorker();
        setupPerfRunConfigurationAspect();

#if WITH_TESTS
        //   addTest<PerfProfilerTraceFileTest>();  // FIXME these tests have to get rewritten
        addTest<PerfResourceCounterTest>();
#endif // WITH_TESTS
    }

    ShutdownFlag aboutToShutdown() final
    {
        destroyPerfProfilerTool();

        return SynchronousShutdown;
    }
};

} // PerfProfiler::Internal

#include "perfprofilerplugin.moc"
