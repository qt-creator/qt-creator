// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"
#include "sampletrace.h"

#include <utils/result.h>

#include <QByteArray>

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>

namespace Profiler::Internal {

// A thread-safe FIFO of raw bytes: the only thing that crosses from the GUI
// thread (where "perf record"'s Process lives) to the worker thread that
// parses its output (see PerfRecordReader). Deliberately simpler than a
// QIODevice/moveToThread setup -- nothing here needs Qt's device abstraction,
// just an ordered handoff of bytes.
class PerfByteQueue
{
public:
    void push(const QByteArray &data);
    // Marks that no more data will arrive; wakes up a blocked pop().
    void close();

    // Blocks until data is available or the queue is closed and drained.
    // Returns an empty QByteArray exactly at end-of-stream; check atEnd() to
    // distinguish that from a (never-happening, but let's be precise) empty
    // push.
    QByteArray pop();
    bool atEnd() const;

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<QByteArray> m_chunks;
    bool m_closed = false;
};

// Parses the raw byte stream produced by "perf record -o -" (perf.data's
// "pipe mode" framing: an 8-byte magic + size, then a stream of
// perf_event_header-tagged records) directly into a SampleTraceData -- no
// intermediate wire format, no external process. A clean-room implementation
// against the (public, kernel-defined) perf.data ABI; see perfrecordreader.cpp
// for the specific record shapes this handles and why.
//
// Supports "--call-graph fp" and "--call-graph lbr" recordings, where the
// kernel hands back an already-unwound frame-pointer chain, or a hardware
// branch-history buffer usable as one directly, in each sample; and, in a
// build with libdw, "--call-graph dwarf" recordings, whose captured user
// registers and stack PerfDwarfUnwinder unwinds (see perfdwarfunwinder.h).
// PerfSamplerSettings::createSession() rejects "dwarf" in a build without.
// Symbolication is ELF symbol-table (via
// Utils::ElfReader) for userspace frames and /proc/kallsyms for kernel
// frames, demangled (Itanium C++ ABI) userspace names, and a best-effort
// source file/line for userspace frames via a small hand-rolled DWARF2-5
// ".debug_line" reader (see dwarflinetable.h) -- no full .debug_info, so no
// inline-frame expansion. For stripped userspace modules the separate debug
// file (source of both symbols and ".debug_line") is looked up by build-id
// in the shared debuginfod cache and, if downloading is enabled (see
// setDownloadDebugInfo()) and DEBUGINFOD_URLS is set, fetched from a
// debuginfod server (same environment variables as elfutils/gdb/perf) --
// all such fetches run in parallel via QtTaskTree; on any failure a module
// falls back to the on-disk ".dynsym" and has no line info.
// Kernel frames are typically unresolvable without root (kptr_restrict
// zeroes /proc/kallsyms addresses by default on most distros), same
// limitation "perf record" itself warns about. Multiple simultaneous events
// ("-e a,b") and tracepoint-triggered samples both decode -- their call
// stacks resolve normally, though a tracepoint's own fields (e.g.
// sched_switch's prev_pid/next_pid) are not decoded or exposed. Linux only,
// matching PerfSampler itself.
class PROFILER_EXPORT PerfRecordReader
{
public:
    // Whether debug information missing locally may be fetched from the
    // debuginfod servers in DEBUGINFOD_URLS. Off by default; the shared cache
    // is consulted either way.
    void setDownloadDebugInfo(bool download) { m_downloadDebugInfo = download; }

    // Called on the reading thread while debug information is fetched, with
    // the share of fetches done (0..100) and the servers asked (whitespace-
    // separated, like DEBUGINFOD_URLS), and once with -1 and no servers when
    // fetching is over.
    void setDebugInfoDownloadHandler(
        const std::function<void(int percent, const QString &urls)> &handler)
    {
        m_debugInfoDownloadHandler = handler;
    }

    // Reads from `queue` until it reports atEnd(), or a fatal parse error
    // occurs (e.g. an unsupported perf_event_attr::sample_type bit). Returns
    // the populated data, or an error suitable for surfacing to the user.
    // A stream without a single sample that has a call stack, including no
    // stream at all, is not an error but empty data: why it is empty is for
    // the caller to tell, which knows how "perf record" fared.
    // `progress`, if set, is called with 0..100 while post-recording
    // symbolication runs (debuginfod fetches, then per-sample resolution) --
    // the part that only starts once recording stops and can take a while
    // for a large trace, previously with no feedback at all.
    Utils::Result<SampleTraceData> read(PerfByteQueue &queue,
                                        const std::function<void(int)> &progress = {});

    // The samples the last read() decoded, including those dropped for having
    // no call stack.
    int receivedSamples() const { return m_receivedSamples; }
    qint64 firstTimestampNs() const { return m_firstTimestampNs; }
    void setSampleFilter(const std::function<bool(qint64)> &filter) { m_sampleFilter = filter; }
    // Polled while reading, and while debug information is fetched and
    // samples are resolved; once it returns true, read() gives up with an
    // error.
    void setCancelCheck(const std::function<bool()> &isCanceled) { m_isCanceled = isCanceled; }

private:
    bool m_downloadDebugInfo = false;
    std::function<void(int, const QString &)> m_debugInfoDownloadHandler;
    std::function<bool(qint64)> m_sampleFilter;
    std::function<bool()> m_isCanceled;
    int m_receivedSamples = 0;
    qint64 m_firstTimestampNs = -1;
};

} // namespace Profiler::Internal
