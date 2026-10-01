// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilertracebackend.h"

#include "profilertr.h"
#include "qmlprofilerclientmanager.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerplainviewmanager.h"
#include "qmlprofilerstatemanager.h"
#include "qmlprofilerstatewidget.h"
#include "qmlprofilerstatisticsview.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <tracing/timelinenotesmodel.h>

#include <utils/stylehelper.h>

#include <QElapsedTimer>
#include <QLabel>
#include <QTimer>

using namespace QmlDebug;
using namespace Utils;

using namespace std::chrono;

namespace Profiler::Internal {

class QmlProfilerTraceBackendPrivate
{
public:
    explicit QmlProfilerTraceBackendPrivate(Timeline::RangeDetailsWidget *details)
        : viewManager(details)
    {}

    QmlProfilerPlainViewManager viewManager;
    QmlProfilerStateManager stateManager;
    QmlProfilerClientManager clientManager;

    QLabel timeLabel;

    QTimer recordingTimer;
    QElapsedTimer recordingElapsedTime;

    QPointer<QmlProfilerStatisticsView> statisticsView; // Owns this trace's text marks.
};

QmlProfilerTraceBackend::QmlProfilerTraceBackend(Timeline::RangeDetailsWidget *details,
                                                 QObject *parent)
    : ProfilerTraceBackend(parent)
    , d(new QmlProfilerTraceBackendPrivate(details))
{
    connect(&d->viewManager, &QmlProfilerPlainViewManager::error,
            this, &QmlProfilerTraceBackend::error);
    connect(&d->viewManager, &QmlProfilerPlainViewManager::loadFinished, this, [this] {
        emit busyChanged(false);
        emit loadFinished();
    });
    connect(&d->viewManager, &QmlProfilerPlainViewManager::gotoSourceLocation,
            this, [this](const QString &fileUrl, int line, int column) {
        if (line < 0 || fileUrl.isEmpty())
            return;
        const FilePath file = modelManager()->findLocalFile(fileUrl);
        if (!file.exists() || !file.isReadableFile())
            return;
        // Recorded locations count columns from 1, the editor from 0.
        emit gotoSourceLocation({file, line == 0 ? 1 : line, column - 1});
    });

    QmlProfilerModelManager *models = modelManager();
    connect(models, &QmlProfilerModelManager::traceChanged,
            this, &QmlProfilerTraceBackend::traceChanged);
    connect(models, &QmlProfilerModelManager::saveFinished, this, [this] {
        emit busyChanged(false);
        emit saved();
    });
    connect(models, &QmlProfilerModelManager::error,
            this, &QmlProfilerTraceBackend::error);
    connect(models, &QmlProfilerModelManager::availableFeaturesChanged,
            this, [this](quint64 features) {
        if (features != d->stateManager.requestedFeatures())
            d->stateManager.setRequestedFeatures(features); // By default, enable them all.
    });

    models->registerFeatures(
        0,
        [] {},
        [this] {
            updateTimeDisplay();
            createTextMarks();
        },
        [this] {
            d->clientManager.clearBufferedData();
            updateTimeDisplay();
        });

    d->clientManager.setModelManager(models);
    d->clientManager.setProfilerStateManager(&d->stateManager);
    connect(&d->clientManager, &QmlProfilerClientManager::connectionClosed,
            this, &QmlProfilerTraceBackend::clientsDisconnected);

    connect(&d->stateManager, &QmlProfilerStateManager::stateChanged,
            this, &QmlProfilerTraceBackend::profilerStateChanged);
    connect(&d->stateManager, &QmlProfilerStateManager::serverRecordingChanged,
            this, &QmlProfilerTraceBackend::serverRecordingChanged);

    models->populateFileFinder();
    setupToolBar();
}

QmlProfilerTraceBackend::~QmlProfilerTraceBackend()
{
    delete d;
}

QWidgetList QmlProfilerTraceBackend::views(QWidget *parent)
{
    const QWidgetList views = d->viewManager.views(parent);
    for (QWidget *view : views) {
        new QmlProfilerStateWidget(&d->stateManager, modelManager(), view);
        if (auto statistics = qobject_cast<QmlProfilerStatisticsView *>(view))
            d->statisticsView = statistics;
    }

    // Annotate the source with this trace's timings, in the editors that are
    // open now and in any opened later.
    createTextMarks();
    connect(Core::EditorManager::instance(), &Core::EditorManager::editorCreated,
            this, [this](Core::IEditor *, const FilePath &filePath) {
        if (d->statisticsView)
            d->statisticsView->createMarks(filePath.toUrlishString());
    });
    return views;
}

void QmlProfilerTraceBackend::createTextMarks()
{
    if (!d->statisticsView)
        return;
    const QList<Core::IDocument *> documents = Core::DocumentModel::openedDocuments();
    for (Core::IDocument *document : documents)
        d->statisticsView->createMarks(document->filePath().toUrlishString());
}

QList<QWidget *> QmlProfilerTraceBackend::toolBarWidgets()
{
    return {&d->timeLabel};
}

void QmlProfilerTraceBackend::load(const FilePath &path)
{
    emit busyChanged(true);
    modelManager()->populateFileFinder();
    Core::ProgressManager::addTask(modelManager()->load(path.toUrlishString()),
                                   Tr::tr("Loading Trace Data"), Constants::TASK_LOAD);
}

Result<> QmlProfilerTraceBackend::save(const FilePath &path)
{
    emit busyChanged(true);
    Core::ProgressManager::addTask(modelManager()->save(path.toUrlishString()),
                                   Tr::tr("Saving Trace Data"), Constants::TASK_SAVE,
                                   Core::ProgressManager::ShowInApplicationIcon);
    return ResultOk;
}

bool QmlProfilerTraceBackend::isModified() const
{
    const Timeline::TimelineNotesModel *notes = modelManager()->notesModel();
    return notes && notes->isModified();
}

void QmlProfilerTraceBackend::clear()
{
    d->viewManager.clear();
}

milliseconds QmlProfilerTraceBackend::traceDuration() const
{
    return d->viewManager.traceDuration();
}

QmlProfilerModelManager *QmlProfilerTraceBackend::modelManager() const
{
    return d->viewManager.modelManager();
}

QmlProfilerStateManager *QmlProfilerTraceBackend::stateManager() const
{
    return &d->stateManager;
}

QmlProfilerClientManager *QmlProfilerTraceBackend::clientManager() const
{
    return &d->clientManager;
}

bool QmlProfilerTraceBackend::aggregatesTraces() const
{
    return modelManager()->aggregateTraces();
}


void QmlProfilerTraceBackend::setupToolBar()
{
    StyleHelper::setPanelWidget(&d->timeLabel);
    d->timeLabel.setIndent(StyleHelper::SpacingTokens::PaddingHL);

    d->recordingTimer.setInterval(100);
    connect(&d->recordingTimer, &QTimer::timeout,
            this, &QmlProfilerTraceBackend::updateTimeDisplay);
    updateTimeDisplay();
}

void QmlProfilerTraceBackend::updateTimeDisplay()
{
    double seconds = 0;
    switch (d->stateManager.currentState()) {
    case QmlProfilerStateManager::AppStopRequested:
    case QmlProfilerStateManager::AppDying:
        return; // Transitional state: don't update the display.
    case QmlProfilerStateManager::AppRunning:
        if (d->stateManager.serverRecording()) {
            seconds = d->recordingElapsedTime.elapsed() / 1000.0;
            break;
        }
        Q_FALLTHROUGH();
    case QmlProfilerStateManager::Idle:
        if (modelManager()->traceDuration() > 0)
            seconds = modelManager()->traceDuration() / 1.0e9;
        break;
    }
    const QString timeString = QString::number(seconds, 'f', 1);
    d->timeLabel.setText(Tr::tr("Elapsed: %1").arg(Tr::tr("%1 s").arg(timeString, 6)));
}

void QmlProfilerTraceBackend::clearEvents()
{
    modelManager()->clear();
    d->clientManager.clearEvents();
    d->stateManager.setRecordedFeatures(0);
}

void QmlProfilerTraceBackend::prepareRun(const ProjectExplorer::BuildConfiguration *bc,
                                         bool aggregateTraces, int flushInterval)
{
    d->clientManager.setFlushInterval(flushInterval);
    modelManager()->setAggregateTraces(aggregateTraces);
    modelManager()->populateFileFinder(bc);
    d->stateManager.setCurrentState(QmlProfilerStateManager::AppRunning);
}

void QmlProfilerTraceBackend::handleStop()
{
    if (d->clientManager.isConnecting()) {
        emit error(Tr::tr("The application finished before a connection could be established. "
                          "No data was loaded."));
    }
    d->clientManager.disconnectFromServer();
}

void QmlProfilerTraceBackend::profilerStateChanged()
{
    switch (d->stateManager.currentState()) {
    case QmlProfilerStateManager::AppDying:
        // If already disconnected when dying, check again that all data was read.
        if (!d->clientManager.isConnected())
            clientsDisconnected();
        break;
    case QmlProfilerStateManager::AppStopRequested:
        // Don't allow toggling the recording while data is loaded when the
        // application quits.
        if (d->stateManager.serverRecording()) {
            d->clientManager.stopRecording(); // Wait for the remaining data.
        } else {
            QTimer::singleShot(0, &d->stateManager, [this] {
                d->stateManager.setCurrentState(QmlProfilerStateManager::Idle);
            });
        }
        break;
    default:
        break;
    }
}

void QmlProfilerTraceBackend::serverRecordingChanged()
{
    if (d->stateManager.currentState() == QmlProfilerStateManager::AppRunning) {
        if (d->stateManager.serverRecording()) {
            // A new recording discards the previous data. The session cannot be
            // stopped here, so offer to save rather than to cancel.
            if (isModified())
                emit saveBeforeRecordingRequested();
            d->recordingTimer.start();
            d->recordingElapsedTime.start();
            if (!modelManager()->aggregateTraces())
                clearEvents();
            modelManager()->initialize();
        } else {
            d->recordingTimer.stop();
            if (!modelManager()->aggregateTraces())
                modelManager()->finalize();
        }
    } else if (d->stateManager.currentState() == QmlProfilerStateManager::AppStopRequested) {
        modelManager()->finalize();
        d->stateManager.setCurrentState(QmlProfilerStateManager::Idle);
    }
}

void QmlProfilerTraceBackend::clientsDisconnected()
{
    if (d->stateManager.currentState() != QmlProfilerStateManager::Idle) {
        if (modelManager()->aggregateTraces()) {
            modelManager()->finalize();
        } else if (d->stateManager.serverRecording()
                   && d->stateManager.currentState()
                          != QmlProfilerStateManager::AppStopRequested) {
            // The application went away while still recording, so no more data
            // is coming. Finalize what arrived, so it is displayed and the
            // "Profiling application" overlay goes away.
            emit error(Tr::tr("Application finished before loading profiled data.\n"
                              "Please use the stop button instead."));
            modelManager()->finalize();
        }
    }

    if (d->stateManager.currentState() == QmlProfilerStateManager::AppDying) {
        QTimer::singleShot(0, &d->stateManager, [this] {
            d->stateManager.setCurrentState(QmlProfilerStateManager::Idle);
        });
    }
}

} // namespace Profiler::Internal
