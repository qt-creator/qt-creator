// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QPair>
#include <QVariant>

#include <optional>

namespace Profiler::Internal {

// One field of a tracepoint's record, as its format describes it:
// "field:unsigned int prev_pid; offset:24; size:4; signed:0;".
struct TracepointField
{
    enum Kind {
        Number,    // an integer of `size` bytes
        String,    // a char array, NUL-terminated within it
        DataLoc,   // __data_loc: a u32 at `offset` locating a string elsewhere in the record
        Bytes,     // any other array, as its bytes
    };

    QByteArray name;
    int offset = 0;
    int size = 0;
    bool isSigned = false;
    Kind kind = Number;
};

struct TracepointFormat
{
    QByteArray system;             // e.g. "sched"
    QByteArray name;               // e.g. "sched_switch"
    QList<TracepointField> fields; // without the common_* ones every record has

    // The fields of `record`, a sample's PERF_SAMPLE_RAW data, by name.
    QList<QPair<QByteArray, QVariant>> decode(const QByteArray &record) const;
};

// The tracepoint formats in perf's tracing data -- what the kernel's tracefs
// "format" files say about each tracepoint recorded -- by tracepoint id,
// which is the config of an attr of PERF_TYPE_TRACEPOINT.
class PROFILER_EXPORT PerfTracepointFormats
{
public:
    // Adds the formats in `tracingData`, as perf writes it into a recording.
    // Anything it cannot make sense of is skipped.
    void parse(const QByteArray &tracingData);

    // Parses one format file's text.
    static std::optional<QPair<quint64, TracepointFormat>> parseFormat(
        const QByteArray &system, const QByteArray &text);

    const TracepointFormat *format(quint64 id) const;

private:
    QHash<quint64, TracepointFormat> m_formats;
};

} // namespace Profiler::Internal
