// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callstacksampler.h"

#include "macsampler.h"

#include "profilertr.h"

#include <QtTaskTree/QThreadFunction>

using namespace Profiler;
using namespace QtTaskTree;
using namespace Utils;

namespace QmlProfiler::Internal {

QString CallStackSampler::displayName() const
{
    return Tr::tr("Call-Stack Sampler");
}

bool CallStackSampler::isAvailable(QString *error) const
{
#ifdef Q_OS_MACOS
    Q_UNUSED(error)
    return true;
#else
    if (error)
        *error = Tr::tr("Call-stack sampling is only implemented on macOS.");
    return false;
#endif
}

ExecutableItem CallStackSampler::recordRecipe(const std::shared_ptr<RecordingSession> &session) const
{
    const auto onSetup = [session](QThreadFunction<Result<FilePath>> &sampling) {
        // recordSampleTrace blocks until session->stop is set, so it runs on a
        // worker thread; the target stays alive (the launched process is only
        // terminated once this task finishes) so it can still be symbolized.
        sampling.setThreadFunctionData([session] {
            SamplerOptions opts;
            opts.pid = session->pid.load();
            opts.processName = session->processName;
            opts.intervalUs = session->intervalUs;
            return recordSampleTrace(opts, session->stop, &session->progress);
        });
    };
    const auto onDone = [session](const QThreadFunction<Result<FilePath>> &sampling,
                                  DoneWith result) {
        if (result != DoneWith::Cancel)
            session->result = sampling.result();
    };
    return QThreadFunctionTask<Result<FilePath>>(onSetup, onDone);
}

} // namespace QmlProfiler::Internal
