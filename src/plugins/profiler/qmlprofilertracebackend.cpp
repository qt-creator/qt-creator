// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilertracebackend.h"

#include "qmlprofilerconstants.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerplainviewmanager.h"
#include "profilertr.h"

#include <coreplugin/progressmanager/progressmanager.h>

#include <tracing/timelinenotesmodel.h>

#include <utils/qtcassert.h>

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
        const FilePath file = d->viewManager.modelManager()->findLocalFile(fileUrl);
        if (!file.exists() || !file.isReadableFile())
            return;
        // Recorded locations count columns from 1, the editor from 0.
        emit gotoSourceLocation({file, line == 0 ? 1 : line, column - 1});
    });
    connect(d->viewManager.modelManager(), &QmlProfilerModelManager::traceChanged,
            this, &QmlProfilerTraceBackend::traceChanged);
    connect(d->viewManager.modelManager(), &QmlProfilerModelManager::saveFinished, this, [this] {
        emit busyChanged(false);
        emit saved();
    });
}

QmlProfilerTraceBackend::~QmlProfilerTraceBackend()
{
    delete d;
}

QWidgetList QmlProfilerTraceBackend::views(QWidget *parent)
{
    return d->viewManager.views(parent);
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

} // namespace Profiler::Internal
