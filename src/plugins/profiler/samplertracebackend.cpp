// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "samplertracebackend.h"

#include "samplerviewmanager.h"

using namespace Utils;

using namespace std::chrono;

namespace Profiler::Internal {

class SamplerTraceBackendPrivate
{
public:
    explicit SamplerTraceBackendPrivate(Timeline::RangeDetailsWidget *details)
        : viewManager(details)
    {}

    SamplerViewManager viewManager;
};

SamplerTraceBackend::SamplerTraceBackend(Timeline::RangeDetailsWidget *details, QObject *parent)
    : ProfilerTraceBackend(parent)
    , d(new SamplerTraceBackendPrivate(details))
{
    connect(&d->viewManager, &SamplerViewManager::error, this, &SamplerTraceBackend::error);
    connect(&d->viewManager, &SamplerViewManager::loadFinished, this, [this] {
        emit loadFinished();
        emit traceChanged();
    });
    connect(&d->viewManager, &SamplerViewManager::gotoSourceLocation, this,
            [this](const QString &file, int line, int column, const QString &, quint64) {
        // A sampled frame outside any source file carries only module and
        // offset; there is nothing to open for it.
        if (file.isEmpty() || line < 0)
            return;
        emit gotoSourceLocation({FilePath::fromUserInput(file), line, column});
    });
}

SamplerTraceBackend::~SamplerTraceBackend()
{
    delete d;
}

QWidgetList SamplerTraceBackend::views(QWidget *parent)
{
    return d->viewManager.views(parent);
}

void SamplerTraceBackend::load(const FilePath &path)
{
    d->viewManager.load(path);
}

void SamplerTraceBackend::clear()
{
    d->viewManager.clear();
}

milliseconds SamplerTraceBackend::traceDuration() const
{
    return d->viewManager.traceDuration();
}

} // namespace Profiler::Internal
