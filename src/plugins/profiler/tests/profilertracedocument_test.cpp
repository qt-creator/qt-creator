// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profilertracedocument_test.h"

#include <profiler/profilertracebackend.h>
#include <profiler/profilertracedocument.h>
#include <profiler/profilertraceeditor.h>
#include <profiler/qmlprofilerconstants.h>
#include <profiler/qmlprofilermodelmanager.h>
#include <profiler/qmlprofilertracebackend.h>
#include <profiler/traceformat.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/modemanager.h>

#include <utils/async.h>
#include <utils/link.h>
#include <utils/temporaryfile.h>

#include <QAction>
#include <QScopeGuard>
#include <QSemaphore>
#include <QSignalSpy>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>

#include <atomic>
#include <memory>

using namespace Core;
using namespace QmlDebug;
using namespace Utils;

namespace Profiler::Internal {

// Writes a trace of `ranges` JavaScript ranges, long enough to be still loading while a test
// holds the loading thread.
static void writeLongTrace(const QString &path, int ranges)
{
    QmlProfilerModelManager saved;
    saved.initialize();
    const int type = saved.appendEventType(
        QmlEventType(UndefinedMessage, Javascript, -1, QmlEventLocation("main.qml", 1, 1), "f"));
    for (int i = 0; i < ranges; ++i) {
        QmlEvent event(i * 10, type, {});
        event.setRangeStage(RangeStart);
        saved.appendEvent(QmlEvent(event));
        event.setRangeStage(RangeEnd);
        event.setTimestamp(i * 10 + 5);
        saved.appendEvent(std::move(event));
    }
    saved.decreaseTraceStart(0);
    saved.increaseTraceEnd(qint64(ranges) * 10);
    saved.finalize();
    QSignalSpy saveSpy(&saved, &QmlProfilerModelManager::saveFinished);
    saved.save(path);
    QTRY_COMPARE_WITH_TIMEOUT(saveSpy.count(), 1, 60000);
}

namespace {

// A loader that holds the loading thread in its first event until the gate opens, and counts
// the events that arrive once the test has stopped the load.
struct GatedLoader
{
    QSemaphore started;
    QSemaphore gate;
    std::atomic<int> events = 0;
    std::atomic<bool> stopped = false;
    std::atomic<int> eventsAfterStop = 0;
};

} // namespace

static void addGatedLoader(QmlProfilerModelManager *manager,
                           const std::shared_ptr<GatedLoader> &gated)
{
    manager->qmlLoaders.append({1ULL << ProfileJavaScript,
                                [gated](const QmlEvent &, const QmlEventType &) {
        if (gated->stopped)
            ++gated->eventsAfterStop;
        if (gated->events++ == 0) {
            gated->started.release();
            gated->gate.acquire();
        }
    }});
}

static IEditor *editorForDocument(IDocument *document)
{
    const QList<IEditor *> editors = DocumentModel::editorsForDocument(document);
    return editors.isEmpty() ? nullptr : editors.first();
}

static IEditor *editorForFile(const FilePath &filePath)
{
    const QList<IEditor *> editors = DocumentModel::editorsForFilePath(filePath);
    return editors.isEmpty() ? nullptr : editors.first();
}

static void triggerCommand(const Id &id)
{
    Command *command = ActionManager::command(id);
    QVERIFY(command);
    QVERIFY(command->action()->isEnabled());
    command->action()->trigger();
}

static void resetEditorArea()
{
    ModeManager::activateMode(Core::Constants::MODE_EDIT);
    EditorManager::closeAllEditors(false);
    if (Command *removeSplits = ActionManager::command(Core::Constants::REMOVE_ALL_SPLITS))
        removeSplits->action()->trigger();
}

enum { DashboardTab = 0, TimelineTab = 1, FlameGraphTab = 2 };

static QTabWidget *tabsOf(IEditor *editor)
{
    return editor->widget()->findChild<QTabWidget *>();
}

// Records a trace like a live recording does, ending in finalize(). It has a few
// debug messages, which give the dashboard nothing to show, and animation frames
// on the GUI thread if `withFrames` is set.
static void record(QmlProfilerModelManager *manager, bool withFrames)
{
    manager->initialize();
    QmlEvent event;
    event.setTimestamp(100);
    event.setString("message");
    event.setTypeIndex(manager->appendEventType(QmlEventType(
        DebugMessage, UndefinedRangeType, QtDebugMsg, QmlEventLocation("main.qml", 1, 1))));
    manager->appendEvent(std::move(event));
    if (withFrames) {
        QmlEvent frame;
        frame.setTypeIndex(manager->appendEventType(
            QmlEventType(Event, UndefinedRangeType, AnimationFrame)));
        for (int i = 0; i < 5; ++i) {
            frame.setTimestamp(200 + i);
            frame.setNumbers<int>({60, 1, GuiThread});
            manager->appendEvent(QmlEvent(frame));
        }
    }
    manager->decreaseTraceStart(0);
    manager->increaseTraceEnd(1000);
    manager->finalize();
}

void ProfilerTraceDocumentTest::init()
{
    resetEditorArea();
    m_document = openLiveTrace(TraceFormat::Qml, "Test Trace", "profiler-trace-document-test");
    QVERIFY(m_document);
    QVERIFY(editorForDocument(m_document));
}

void ProfilerTraceDocumentTest::cleanup()
{
    resetEditorArea();
    m_document = nullptr;
}

void ProfilerTraceDocumentTest::selectEvent(const FilePath &source)
{
    QVERIFY(!m_document->backends().isEmpty());
    emit m_document->backends().first()->gotoSourceLocation(Link(source, 1, 0));
}

void ProfilerTraceDocumentTest::testSourceOpensNextToTrace()
{
    TemporaryFile source("profilertestsource");
    QVERIFY(source.open());
    IEditor *traceEditor = editorForDocument(m_document);
    const int traceViewId = EditorManager::viewIdForEditor(traceEditor);
    QVERIFY(traceViewId != 0);

    selectEvent(source.filePath());

    IEditor *sourceEditor = editorForFile(source.filePath());
    QVERIFY(sourceEditor);
    const int sourceViewId = EditorManager::viewIdForEditor(sourceEditor);
    QVERIFY(sourceViewId != 0);
    QVERIFY(sourceViewId != traceViewId);
    QCOMPARE(EditorManager::viewIdForEditor(traceEditor), traceViewId);
    QCOMPARE(EditorManager::visibleEditors(), (QList<IEditor *>{traceEditor, sourceEditor}));
}

void ProfilerTraceDocumentTest::testSourceReusesTheSameSplit()
{
    TemporaryFile first("profilertestsourcefirst");
    TemporaryFile second("profilertestsourcesecond");
    QVERIFY(first.open());
    QVERIFY(second.open());
    IEditor *traceEditor = editorForDocument(m_document);

    selectEvent(first.filePath());
    IEditor *firstEditor = editorForFile(first.filePath());
    QVERIFY(firstEditor);
    const int sourceViewId = EditorManager::viewIdForEditor(firstEditor);

    selectEvent(second.filePath());

    IEditor *secondEditor = editorForFile(second.filePath());
    QVERIFY(secondEditor);
    QCOMPARE(EditorManager::viewIdForEditor(secondEditor), sourceViewId);
    QCOMPARE(EditorManager::visibleEditors(), (QList<IEditor *>{traceEditor, secondEditor}));
}

void ProfilerTraceDocumentTest::testSourceIgnoresLaterSplits()
{
    TemporaryFile first("profilertestsourcefirst");
    TemporaryFile second("profilertestsourcesecond");
    TemporaryFile other("profilertestother");
    QVERIFY(first.open());
    QVERIFY(second.open());
    QVERIFY(other.open());
    IEditor *traceEditor = editorForDocument(m_document);
    const int traceViewId = EditorManager::viewIdForEditor(traceEditor);

    selectEvent(first.filePath());
    IEditor *firstEditor = editorForFile(first.filePath());
    QVERIFY(firstEditor);
    const int sourceViewId = EditorManager::viewIdForEditor(firstEditor);

    EditorManager::splitSideBySide();
    IEditor *otherEditor = EditorManager::openEditor(other.filePath());
    QVERIFY(otherEditor);
    const int otherViewId = EditorManager::viewIdForEditor(otherEditor);
    QVERIFY(otherViewId != traceViewId);
    QVERIFY(otherViewId != sourceViewId);

    selectEvent(second.filePath());

    IEditor *secondEditor = editorForFile(second.filePath());
    QVERIFY(secondEditor);
    QCOMPARE(EditorManager::viewIdForEditor(secondEditor), sourceViewId);
    QCOMPARE(EditorManager::viewIdForEditor(otherEditor), otherViewId);
    QVERIFY(EditorManager::visibleEditors().contains(otherEditor));
}

// The remembered split is the one the sources went to, not whichever split
// happens to sit next to the trace: splitting the trace's own view puts a new
// one there, and the sources still go where they went before.
void ProfilerTraceDocumentTest::testSourceKeepsItsSplitWhenTheTraceIsSplit()
{
    TemporaryFile first("profilertestsourcefirst");
    TemporaryFile second("profilertestsourcesecond");
    TemporaryFile other("profilertestother");
    QVERIFY(first.open());
    QVERIFY(second.open());
    QVERIFY(other.open());
    IEditor *traceEditor = editorForDocument(m_document);
    const int traceViewId = EditorManager::viewIdForEditor(traceEditor);

    selectEvent(first.filePath());
    IEditor *firstEditor = editorForFile(first.filePath());
    QVERIFY(firstEditor);
    const int sourceViewId = EditorManager::viewIdForEditor(firstEditor);

    // split the trace's own view, so that the split beside the trace is the
    // new one rather than the one the source went to
    EditorManager::activateEditor(traceEditor);
    triggerCommand(Core::Constants::SPLIT);
    IEditor *otherEditor = EditorManager::openEditor(other.filePath());
    QVERIFY(otherEditor);
    const int otherViewId = EditorManager::viewIdForEditor(otherEditor);
    QVERIFY(otherViewId != traceViewId);
    QVERIFY(otherViewId != sourceViewId);
    QCOMPARE(EditorManager::viewIdForEditor(traceEditor), traceViewId);

    selectEvent(second.filePath());

    IEditor *secondEditor = editorForFile(second.filePath());
    QVERIFY(secondEditor);
    QCOMPARE(EditorManager::viewIdForEditor(secondEditor), sourceViewId);
    QCOMPARE(EditorManager::viewIdForEditor(otherEditor), otherViewId);
}

// Closing the split the sources went to forgets it, so the next selection
// opens a fresh one instead of naming a split that is gone.
void ProfilerTraceDocumentTest::testClosedSourceSplitIsForgotten()
{
    TemporaryFile first("profilertestsourcefirst");
    TemporaryFile second("profilertestsourcesecond");
    QVERIFY(first.open());
    QVERIFY(second.open());
    IEditor *traceEditor = editorForDocument(m_document);
    const int traceViewId = EditorManager::viewIdForEditor(traceEditor);

    selectEvent(first.filePath());
    IEditor *firstEditor = editorForFile(first.filePath());
    QVERIFY(firstEditor);
    const int sourceViewId = EditorManager::viewIdForEditor(firstEditor);

    EditorManager::activateEditor(firstEditor);
    triggerCommand(Core::Constants::REMOVE_CURRENT_SPLIT);

    selectEvent(second.filePath());

    IEditor *secondEditor = editorForFile(second.filePath());
    QVERIFY(secondEditor);
    const int secondViewId = EditorManager::viewIdForEditor(secondEditor);
    QVERIFY(secondViewId != 0);
    QVERIFY(secondViewId != sourceViewId);
    QVERIFY(secondViewId != traceViewId);
    QCOMPARE(EditorManager::viewIdForEditor(traceEditor), traceViewId);
}

// The trace moved into the split its sources go to would have the next source
// take its place, so that selection anchors on a new split.
void ProfilerTraceDocumentTest::testTraceMovedIntoTheSourceSplitGetsANewOne()
{
    TemporaryFile first("profilertestsourcefirst");
    TemporaryFile second("profilertestsourcesecond");
    QVERIFY(first.open());
    QVERIFY(second.open());
    IEditor *traceEditor = editorForDocument(m_document);

    selectEvent(first.filePath());
    IEditor *firstEditor = editorForFile(first.filePath());
    QVERIFY(firstEditor);
    const int sourceViewId = EditorManager::viewIdForEditor(firstEditor);

    EditorManager::activateEditor(traceEditor);
    triggerCommand(Core::Constants::MOVE_TO_NEXT_SPLIT);
    QCOMPARE(EditorManager::viewIdForEditor(editorForDocument(m_document)), sourceViewId);

    selectEvent(second.filePath());

    IEditor *secondEditor = editorForFile(second.filePath());
    QVERIFY(secondEditor);
    QVERIFY(EditorManager::viewIdForEditor(secondEditor) != sourceViewId);
    QCOMPARE(EditorManager::viewIdForEditor(editorForDocument(m_document)), sourceViewId);
}

void ProfilerTraceDocumentTest::testEmptyTraceShowsTimeline()
{
    auto backend = qobject_cast<QmlProfilerTraceBackend *>(m_document->backends().first());
    QVERIFY(backend);
    QTabWidget *tabs = tabsOf(editorForDocument(m_document));
    QVERIFY(tabs);
    QCOMPARE(tabs->currentIndex(), int(DashboardTab));

    QSignalSpy switchSpy(backend, &ProfilerTraceBackend::viewSwitchRequested);
    record(backend->modelManager(), false);
    QCOMPARE(switchSpy.count(), 1);
    QCOMPARE(tabs->currentIndex(), int(TimelineTab));
}

void ProfilerTraceDocumentTest::testTraceWithFramesStaysOnDashboard()
{
    auto backend = qobject_cast<QmlProfilerTraceBackend *>(m_document->backends().first());
    QVERIFY(backend);
    QTabWidget *tabs = tabsOf(editorForDocument(m_document));
    QVERIFY(tabs);

    QSignalSpy switchSpy(backend, &ProfilerTraceBackend::viewSwitchRequested);
    record(backend->modelManager(), true);
    QCOMPARE(switchSpy.count(), 0);
    QCOMPARE(tabs->currentIndex(), int(DashboardTab));
}

// The dashboard asks for the switch, but the editor leaves a tab alone that the
// user has chosen meanwhile.
void ProfilerTraceDocumentTest::testOtherTabIsKept()
{
    auto backend = qobject_cast<QmlProfilerTraceBackend *>(m_document->backends().first());
    QVERIFY(backend);
    QTabWidget *tabs = tabsOf(editorForDocument(m_document));
    QVERIFY(tabs);
    tabs->setCurrentIndex(FlameGraphTab);

    QSignalSpy switchSpy(backend, &ProfilerTraceBackend::viewSwitchRequested);
    record(backend->modelManager(), false);
    QCOMPARE(switchSpy.count(), 1);
    QCOMPARE(tabs->currentIndex(), int(FlameGraphTab));
}

void ProfilerTraceDocumentTest::testOnlyFirstFinalizeSwitches()
{
    auto backend = qobject_cast<QmlProfilerTraceBackend *>(m_document->backends().first());
    QVERIFY(backend);

    QSignalSpy switchSpy(backend, &ProfilerTraceBackend::viewSwitchRequested);
    record(backend->modelManager(), false);
    QCOMPARE(switchSpy.count(), 1);

    backend->modelManager()->finalize();
    QCOMPARE(switchSpy.count(), 1);
}

void ProfilerTraceDocumentTest::testNewRecordingSwitchesAgain()
{
    auto backend = qobject_cast<QmlProfilerTraceBackend *>(m_document->backends().first());
    QVERIFY(backend);
    QTabWidget *tabs = tabsOf(editorForDocument(m_document));
    QVERIFY(tabs);

    QSignalSpy switchSpy(backend, &ProfilerTraceBackend::viewSwitchRequested);
    record(backend->modelManager(), false);
    QCOMPARE(switchSpy.count(), 1);

    tabs->setCurrentIndex(DashboardTab);
    backend->modelManager()->clear();
    record(backend->modelManager(), false);
    QCOMPARE(switchSpy.count(), 2);
    QCOMPARE(tabs->currentIndex(), int(TimelineTab));
}

// The views of a trace own models that its load calls into. Closing the editor while the trace
// loads stops the load before they go away.
void ProfilerTraceDocumentTest::testClosingTheEditorStopsTheLoad()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath path = FilePath::fromString(dir.filePath("trace.qzt"));
    writeLongTrace(path.toFSPathString(), 100000);
    if (QTest::currentTestFailed())
        return;

    auto editor = new ProfilerTraceEditor(
        std::make_unique<ProfilerTraceDocument>(Constants::QML_TRACE_EDITOR_ID, TraceFormat::Qml));
    auto document = static_cast<ProfilerTraceDocument *>(editor->document());
    auto backend = qobject_cast<QmlProfilerTraceBackend *>(document->backends().first());
    QVERIFY(backend);
    auto gated = std::make_shared<GatedLoader>();
    const auto open = qScopeGuard([gated] { gated->gate.release(); });
    addGatedLoader(backend->modelManager(), gated);
    QVERIFY(document->open(path, path));
    QVERIFY(gated->started.tryAcquire(1, 10000));

    gated->gate.release();
    delete editor;
    gated->stopped = true;
    Utils::asyncThreadPool(QThread::InheritPriority)->waitForDone();
    QCOMPARE(gated->eventsAfterStop.load(), 0);
}

} // namespace Profiler::Internal
