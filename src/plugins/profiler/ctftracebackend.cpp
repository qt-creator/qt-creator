// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctftracebackend.h"

#include "ctfplainviewmanager.h"
#include "profilertr.h"

#include <coreplugin/progressmanager/taskprogress.h>

using namespace Utils;

using namespace std::chrono;

namespace Profiler::Internal {

class CtfTraceBackendPrivate
{
public:
    explicit CtfTraceBackendPrivate(Timeline::RangeDetailsWidget *details)
        : viewManager(details)
    {}

    CtfPlainViewManager viewManager;
};

CtfTraceBackend::CtfTraceBackend(Timeline::RangeDetailsWidget *details, QObject *parent)
    : ProfilerTraceBackend(parent)
    , d(new CtfTraceBackendPrivate(details))
{
    d->viewManager.setTaskTreeSetup([](QtTaskTree::QTaskTree &taskTree) {
        (new Core::TaskProgress(&taskTree))->setDisplayName(Tr::tr("Loading Trace"));
    });

    connect(&d->viewManager, &CtfPlainViewManager::error, this, &CtfTraceBackend::error);
    connect(&d->viewManager, &CtfPlainViewManager::gotoSourceLocation, this,
            [this](const QString &file, int line, int column) {
        // The path is the one the machine that produced the trace saw, and a
        // trace recorded elsewhere names files this one does not have.
        const FilePath path = FilePath::fromUserInput(file);
        if (!path.isAbsolutePath() || !path.isReadableFile())
            return;
        emit gotoSourceLocation({path, line, column});
    });
    connect(&d->viewManager, &CtfPlainViewManager::loadFinished, this, [this] {
        emit loadFinished();
        emit traceChanged();
    });
}

CtfTraceBackend::~CtfTraceBackend()
{
    delete d;
}

QWidgetList CtfTraceBackend::views(QWidget *parent)
{
    return d->viewManager.views(parent);
}

void CtfTraceBackend::load(const FilePath &path)
{
    if (path.isDir())
        d->viewManager.loadCtf2(path);
    else
        d->viewManager.loadJson(path);
}

void CtfTraceBackend::clear()
{
    d->viewManager.clear();
}

milliseconds CtfTraceBackend::traceDuration() const
{
    return d->viewManager.traceDuration();
}

} // namespace Profiler::Internal
