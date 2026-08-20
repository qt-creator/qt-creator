// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "samplertracebackend.h"

#include "samplerviewmanager.h"

#ifndef __EMSCRIPTEN__ // QtSupport is excluded from the WebAssembly build
#include <qtsupport/baseqtversion.h>
#endif

#include <utils/fileinprojectfinder.h>

#include <QUrl>

using namespace Utils;

using namespace std::chrono;

namespace Profiler::Internal {

// A sampled frame names its source the way the compiler or the QML engine did:
// a path on disk, or a URL when the file came out of a resource. A one-letter
// scheme is a Windows drive rather than a scheme.
static QUrl sourceUrl(const QString &file)
{
    const QUrl url(file);
    return url.scheme().size() > 1 ? url : QUrl::fromLocalFile(file);
}

class SamplerTraceBackendPrivate
{
public:
    explicit SamplerTraceBackendPrivate(Timeline::RangeDetailsWidget *details)
        : viewManager(details)
    {}

    SamplerViewManager viewManager;

    // A sampled frame names the file the compiler or the QML engine knew, which
    // is not always one on disk: a QML module bundles its sources, so the JS
    // frames spliced into a combined trace point at "qrc:/" instead.
    FileInProjectFinder fileFinder;
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
        FilePath path = FilePath::fromUserInput(file);
        if (!path.isAbsolutePath() || !path.isReadableFile()) {
            // A finder that resolves nothing answers with what it was given, so
            // only `resolved` tells a hit from a miss.
            bool resolved = false;
            const FilePaths found = d->fileFinder.findFile(sourceUrl(file), &resolved);
            if (!resolved)
                return;
            path = found.constFirst();
            if (!path.isReadableFile())
                return;
        }
        emit gotoSourceLocation({path, line, column});
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
    // Which projects are open, and what their resources map to, can have changed
    // since the last trace, so this is answered per load rather than once.
#ifndef __EMSCRIPTEN__
    QtSupport::QtVersion::populateQmlFileFinder(&d->fileFinder, nullptr);
#endif
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
