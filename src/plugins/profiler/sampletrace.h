// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"

#include <utils/filepath.h>
#include <utils/result.h>

#include <QHash>
#include <QLatin1StringView>
#include <QList>

#include <functional>
#include <utility>

namespace QmlProfiler::Internal {

// Name of the CTF2 data stream class that marks a call-stack sampler trace.
inline constexpr QLatin1StringView samplerStreamName = QLatin1StringView("sampler-stacks");

// In-memory representation of one sampling session: every thread's call stack
// at every sampling tick, plus whether the thread was running at that moment.
class SampleTraceData
{
public:
    struct ThreadSample
    {
        quint64 tsUs = 0;     // microseconds since recording start
        quint64 tid = 0;
        bool running = false; // scheduler run state at capture (best-effort "on-CPU")
        QList<int> frames;    // root-first indices into `labels`; may be empty

        friend bool operator==(const ThreadSample &, const ThreadSample &) = default;
    };

    struct Label
    {
        QString name;
        QString file;       // source path from debug info; empty if unavailable
        int line = 0;
        QString module;     // binary/dylib name; empty if no loaded image
        quint64 offset = 0; // offset within the module (or the address if no image)

        Label() = default;
        Label(const char *n) : name(QString::fromUtf8(n)) {}
        Label(QString n, QString f = {}, int l = 0, QString m = {}, quint64 o = 0)
            : name(std::move(n)), file(std::move(f)), line(l), module(std::move(m)), offset(o) {}

        // A QML/JS frame spliced in from the QML profiler (see mergeQmlIntoSamples):
        // it has no loaded image and a QML/JS source file. Native C++ frames always
        // carry a module, so this cleanly tells the two apart for colouring.
        bool isJs() const
        {
            return module.isEmpty()
                   && (file.endsWith(QLatin1String(".qml"), Qt::CaseInsensitive)
                       || file.endsWith(QLatin1String(".js"), Qt::CaseInsensitive));
        }

        friend bool operator==(const Label &, const Label &) = default;
    };

    quint64 pid = 0;
    QList<Label> labels;                 // index = label id
    QHash<quint64, QString> threadNames; // tid -> name (entries may be empty)
    QList<ThreadSample> samples;         // ordered by tsUs

    friend bool operator==(const SampleTraceData &, const SampleTraceData &) = default;
};

// Writes `data` as a CTF2 trace ("metadata" + "stream0" files) into the
// existing directory `dir`. `progress`, if set, is called with 0..100 while
// the samples are written.
Utils::Result<> writeSampleTrace(const SampleTraceData &data, const Utils::FilePath &dir,
                                 const std::function<void(int)> &progress = {});

// Reads back a trace written by writeSampleTrace.
Utils::Result<SampleTraceData> readSampleTrace(const Utils::FilePath &dir);

// True if `dir` holds a CTF2 trace whose schema contains a data stream class
// named samplerStreamName. Reads only the metadata file.
bool isSamplerTrace(const Utils::FilePath &dir);

} // namespace QmlProfiler::Internal
