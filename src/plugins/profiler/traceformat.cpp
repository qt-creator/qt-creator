// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "traceformat.h"

#include "combinedsampler.h"
#include "samplerviewmanager.h"

using namespace Utils;
using namespace Qt::StringLiterals;

namespace Profiler::Internal {

TraceFile identifyTrace(const FilePath &path)
{
    // "metadata" names a Common Trace Format metadata file, which selects the
    // directory containing it.
    if (path.isDir() || path.fileName() == "metadata"_L1) {
        const FilePath dir = path.isDir() ? path : path.parentDir();
        if (CombinedSampler::isCombinedTrace(dir))
            return {TraceFormat::Combined, dir};
        if (SamplerViewManager::isSamplerTrace(dir))
            return {TraceFormat::Sampler, dir};
        return {TraceFormat::Ctf, dir};
    }
    if (path.suffix() == "json"_L1)
        return {TraceFormat::Ctf, path};
    if (path.suffix() == "ptq"_L1)
        return {TraceFormat::Perf, path};
    return {TraceFormat::Qml, path};
}

} // namespace Profiler::Internal
