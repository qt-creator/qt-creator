// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profilertracedocument_test.h"

#include <profiler/profilertracebackend.h>
#include <profiler/profilertracedocument.h>
#include <profiler/profilertraceeditor.h>
#include <profiler/traceformat.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/modemanager.h>

#include <utils/link.h>
#include <utils/temporaryfile.h>

#include <QAction>
#include <QTest>

using namespace Core;
using namespace Utils;

namespace Profiler::Internal {

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

} // namespace Profiler::Internal
