// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerrunconfigurationaspect.h"
#include "qmlprofilerruncontrol.h"
#include "qmlprofilersettings.h"
#include "qmlprofilertool.h"

#ifdef WITH_TESTS

#include "tests/debugmessagesmodel_test.h"
#include "tests/flamegraphmodel_test.h"
#include "tests/flamegraphview_test.h"
#include "tests/inputeventsmodel_test.h"
#include "tests/localqmlprofilerrunner_test.h"
#include "tests/memoryusagemodel_test.h"
#include "tests/pixmapcachemodel_test.h"
#include "tests/qmlevent_test.h"
#include "tests/qmleventlocation_test.h"
#include "tests/qmleventtype_test.h"
#include "tests/qmlnote_test.h"
#include "tests/qmlprofileranimationsmodel_test.h"
#include "tests/qmlprofilerattachdialog_test.h"
#include "tests/qmlprofilerbindingloopsrenderpass_test.h"
#include "tests/qmlprofilerclientmanager_test.h"
#include "tests/qmlprofilerdetailsrewriter_test.h"
#include "tests/qmlprofilertool_test.h"
#include "tests/qmlprofilertraceclient_test.h"
#include "tests/qmlprofilertraceview_test.h"

// Force QML Debugging to be enabled, so that we can selftest the profiler
#define QT_QML_DEBUG_NO_WARNING
#include <QQmlDebuggingEnabler>
#include <QQmlEngine>
#undef QT_QML_DEBUG_NO_WARNING

#endif // WITH_TESTS

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <extensionsystem/iplugin.h>

using namespace ProjectExplorer;

namespace QmlProfiler::Internal {

class QmlProfilerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlProfiler.json")

    bool initialize(const QStringList &arguments, QString *errorString) final
    {
        Q_UNUSED(arguments)

        setupQmlProfilerTool();
        setupQmlProfilerRunning();

#ifdef WITH_TESTS
        addTest<DebugMessagesModelTest>();
        addTest<FlameGraphModelTest>();
        addTest<FlameGraphViewTest>();
        addTest<InputEventsModelTest>();
        addTest<LocalQmlProfilerRunnerTest>();
        addTest<MemoryUsageModelTest>();
        addTest<PixmapCacheModelTest>();
        addTest<QmlEventTest>();
        addTest<QmlEventLocationTest>();
        addTest<QmlEventTypeTest>();
        addTest<QmlNoteTest>();
        addTest<QmlProfilerAnimationsModelTest>();
        addTest<QmlProfilerAttachDialogTest>();
        addTest<QmlProfilerBindingLoopsRenderPassTest>();
        addTest<QmlProfilerClientManagerTest>();
        addTest<QmlProfilerDetailsRewriterTest>();
        addTest<QmlProfilerToolTest>();
        addTest<QmlProfilerTraceClientTest>();
        addTest<QmlProfilerTraceViewTest>();

        addTest<QQmlEngine>(); // Trigger debug connector to be started
#endif

        return Utils::HostOsInfo::canCreateOpenGLContext(errorString);
    }

    void extensionsInitialized() final
    {
        RunConfiguration::registerAspect<QmlProfilerRunConfigurationAspect>();
    }

    ShutdownFlag aboutToShutdown() final
    {
        destroyQmlProfilerTool();

        return SynchronousShutdown;
    }
};

} // QmlProfiler::Internal

#include "qmlprofilerplugin.moc"
