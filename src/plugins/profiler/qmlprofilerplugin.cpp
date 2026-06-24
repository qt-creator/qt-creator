// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctfvisualizertool.h"
#include "qmlprofilerrunconfigurationaspect.h"
#include "qmlprofilerruncontrol.h"
#include "qmlprofilertool.h"

#include "perfprofilerruncontrol.h"
#include "perfprofilertool.h"
#include "perfrunconfigurationaspect.h"

#include <coreplugin/idocumentfactory.h>

#ifdef WITH_TESTS

#include "tests/perfnativemixed_test.h"
#include "tests/perfresourcecounter_test.h"

#include "tests/debugmessagesmodel_test.h"
#include "tests/flamegraphmodel_test.h"
#include "tests/flamegraphview_test.h"
#include "tests/inputeventsmodel_test.h"
#include "tests/localqmlprofilerrunner_test.h"
#include "tests/memoryusagemodel_test.h"
#include "tests/pixmapcachemodel_test.h"
#include "tests/qmlnote_test.h"
#include "tests/qmlprofileranimationsmodel_test.h"
#include "tests/qmlprofilerattachdialog_test.h"
#include "tests/qmlprofilerclientmanager_test.h"
#include "tests/qmlprofilerdetailsrewriter_test.h"
#include "tests/qmlprofilertool_test.h"
#include "tests/qmlprofilertraceview_test.h"

#endif // WITH_TESTS

#include <extensionsystem/iplugin.h>

using namespace ProjectExplorer;
using namespace Profiler::Internal;

namespace Profiler::Internal {

class QmlProfilerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlProfiler.json")

    void initialize() final
    {
        Profiler::Internal::setupCtfVisualizerTool();
        setupQmlProfilerTool();
        setupQmlProfilerRunning();

        setupPerfProfilerTool();
        setupPerfProfilerRunWorker();

        m_traceFileFactory.addMimeType("application/x-qmlprofiler-trace");
        m_traceFileFactory.setOpener([](const Utils::FilePath &filePath) -> Core::IDocument * {
            QmlProfilerTool::instance()->loadFile(filePath);
            return nullptr;
        });

        m_perfTraceFileFactory.addMimeType("application/x-perfprofiler-trace");
        m_perfTraceFileFactory.setOpener([](const Utils::FilePath &filePath) -> Core::IDocument * {
            PerfProfilerTool::instance()->loadTraceFile(filePath);
            return nullptr;
        });

#ifdef WITH_TESTS
        addTest<DebugMessagesModelTest>();
        addTest<FlameGraphModelTest>();
        addTest<FlameGraphViewTest>();
        addTest<InputEventsModelTest>();
        addTest<LocalQmlProfilerRunnerTest>();
        addTest<MemoryUsageModelTest>();
        addTest<PixmapCacheModelTest>();
        addTest<QmlNoteTest>();
        addTest<QmlProfilerAnimationsModelTest>();
        addTest<QmlProfilerAttachDialogTest>();
        addTest<QmlProfilerClientManagerTest>();
        addTest<QmlProfilerDetailsRewriterTest>();
        addTest<QmlProfilerToolTest>();
        addTest<QmlProfilerTraceViewTest>();

        addTestCreator(createPerfNativeMixedTest);
        addTestCreator(createPerfResourceCounterTest);
#endif
    }

    void extensionsInitialized() final
    {
        RunConfiguration::registerAspect<QmlProfilerRunConfigurationAspect>();
        RunConfiguration::registerAspect<PerfRunConfigurationAspect>();
    }

    ShutdownFlag aboutToShutdown() final
    {
        destroyPerfProfilerTool();
        return SynchronousShutdown;
    }

    Core::IDocumentFactory m_traceFileFactory;
    Core::IDocumentFactory m_perfTraceFileFactory;
};

} // namespace Profiler::Internal

#include "qmlprofilerplugin.moc"
