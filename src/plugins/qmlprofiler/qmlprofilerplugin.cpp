/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmlprofilerplugin.h"
#include "qmlprofilerrunconfigurationaspect.h"
#include "qmlprofileroptionspage.h"
#include "qmlprofilerruncontrol.h"
#include "qmlprofilertool.h"
#include "qmlprofilertimelinemodel.h"

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
#include "tests/qmlprofilerconfigwidget_test.h"
#include "tests/qmlprofilerdetailsrewriter_test.h"
#include "tests/qmlprofilertraceview_test.h"

// Force QML Debugging to be enabled, so that we can selftest the profiler
#define QT_QML_DEBUG_NO_WARNING
#include <QQmlDebuggingEnabler>
#include <QQmlEngine>
#undef QT_QML_DEBUG_NO_WARNING

#endif // WITH_TESTS

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QtPlugin>

using namespace ProjectExplorer;

namespace QmlProfiler {
namespace Internal {

Q_GLOBAL_STATIC(QmlProfilerSettings, qmlProfilerGlobalSettings)

bool QmlProfilerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)

    if (!Utils::HostOsInfo::canCreateOpenGLContext(errorString))
        return false;

    return true;
}

void QmlProfilerPlugin::extensionsInitialized()
{
    m_profilerTool = new QmlProfilerTool(this);

    addAutoReleasedObject(new QmlProfilerOptionsPage);

    RunConfiguration::registerAspect<QmlProfilerRunConfigurationAspect>();

    auto constraint = [](RunConfiguration *runConfiguration) {
        Target *target = runConfiguration ? runConfiguration->target() : nullptr;
        Kit *kit = target ? target->kit() : nullptr;
        return DeviceTypeKitInformation::deviceTypeId(kit)
                == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
    };

    RunControl::registerWorkerCreator(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE,
        [](RunControl *runControl) { return new QmlProfilerRunner(runControl); });

    RunControl::registerWorker<LocalQmlProfilerSupport>
            (ProjectExplorer::Constants::QML_PROFILER_RUN_MODE, constraint);
}

ExtensionSystem::IPlugin::ShutdownFlag QmlProfilerPlugin::aboutToShutdown()
{
    delete m_profilerTool;
    m_profilerTool = nullptr;

    // Save settings.
    // Disconnect from signals that are not needed during shutdown
    // Hide UI (if you add UI that is not in the main window directly)
    return SynchronousShutdown;
}

QmlProfilerSettings *QmlProfilerPlugin::globalSettings()
{
    return qmlProfilerGlobalSettings();
}

QList<QObject *> QmlProfiler::Internal::QmlProfilerPlugin::createTestObjects() const
{
    QList<QObject *> tests;
#ifdef WITH_TESTS
    tests << new DebugMessagesModelTest;
    tests << new FlameGraphModelTest;
    tests << new FlameGraphViewTest;
    tests << new InputEventsModelTest;
    tests << new LocalQmlProfilerRunnerTest;
    tests << new MemoryUsageModelTest;
    tests << new PixmapCacheModelTest;
    tests << new QmlEventTest;
    tests << new QmlEventLocationTest;
    tests << new QmlEventTypeTest;
    tests << new QmlNoteTest;
    tests << new QmlProfilerAnimationsModelTest;
    tests << new QmlProfilerAttachDialogTest;
    tests << new QmlProfilerBindingLoopsRenderPassTest;
    tests << new QmlProfilerClientManagerTest;
    tests << new QmlProfilerConfigWidgetTest;
    tests << new QmlProfilerDetailsRewriterTest;
    tests << new QmlProfilerTraceViewTest;

    tests << new QQmlEngine; // Trigger debug connector to be started
#endif
    return tests;
}

} // namespace Internal
} // namespace QmlProfiler
