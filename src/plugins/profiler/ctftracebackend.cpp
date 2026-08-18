// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctftracebackend.h"

#include "ctfplainviewmanager.h"

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
    connect(&d->viewManager, &CtfPlainViewManager::error, this, &CtfTraceBackend::error);
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
