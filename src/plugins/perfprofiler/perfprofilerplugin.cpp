// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfprofilerruncontrol.h"
#include "perfprofilertool.h"
#include "perfrunconfigurationaspect.h"

#include <coreplugin/idocumentfactory.h>

#if WITH_TESTS
//#  include "tests/perfprofilertracefile_test.h"    // FIXME has to be rewritten
#  include "tests/perfnativemixed_test.h"
#  include "tests/perfresourcecounter_test.h"
#endif // WITH_TESTS

#include <extensionsystem/iplugin.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace PerfProfiler::Internal {

class PerfProfilerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "PerfProfiler.json")

    void initialize() final
    {
        setupPerfProfilerTool();
        setupPerfProfilerRunWorker();
        RunConfiguration::registerAspect<PerfRunConfigurationAspect>();

        m_traceFileFactory.addMimeType("application/x-perfprofiler-trace");
        m_traceFileFactory.setOpener([](const Utils::FilePath &filePath) -> Core::IDocument * {
            PerfProfilerTool::instance()->loadTraceFile(filePath);
            return nullptr;
        });

#if WITH_TESTS
        //   addTest<PerfProfilerTraceFileTest>();  // FIXME these tests have to get rewritten
        addTestCreator(createPerfNativeMixedTest);
        addTestCreator(createPerfResourceCounterTest);
#endif // WITH_TESTS
    }

    ShutdownFlag aboutToShutdown() final
    {
        destroyPerfProfilerTool();

        return SynchronousShutdown;
    }

    Core::IDocumentFactory m_traceFileFactory;
};

} // namespace PerfProfiler::Internal

#include "perfprofilerplugin.moc"
