// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"

#include <utils/filepath.h>
#include <utils/result.h>

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>

#include <functional>

namespace Profiler::Internal {

class PerfByteQueue;

// The records of a perf recording, decoded from the public perf.data ABI
// (include/uapi/linux/perf_event.h and perf's documented file layout) into
// plain structures. What they mean -- which frames a sample has, which
// module an address is in -- is up to a PerfDataHandler.
namespace PerfData {

// Where a sample or mapping was taken: perf_event_header::misc's cpumode.
enum class CpuMode { Unknown, Kernel, User, Hypervisor, GuestKernel, GuestUser };

struct EventAttr
{
    quint32 type = 0;               // PERF_TYPE_*
    quint64 config = 0;             // event within the type; a tracepoint's id
    quint64 samplePeriodOrFreq = 0;
    bool freq = false;              // samplePeriodOrFreq is a frequency
    quint64 sampleType = 0;         // PERF_SAMPLE_* fields each sample carries
    quint64 readFormat = 0;         // layout of PERF_SAMPLE_READ
    bool sampleIdAll = false;       // non-sample records carry a SampleId trailer
    quint64 branchSampleType = 0;
    quint64 sampleRegsUser = 0;     // which user registers PERF_SAMPLE_REGS_USER has
    QByteArray name;                // the event's name, where the recording gives it
    QList<quint64> ids;             // the ids of the events opened with this attr
};

// The time and task of a non-sample record, from its sample_id trailer, where
// the attr asks for one (sample_id_all).
struct SampleId
{
    quint32 pid = 0;
    quint32 tid = 0;
    quint64 time = 0;
    bool hasTime = false;
    quint64 id = 0;
    quint32 cpu = 0;
};

struct Sample
{
    int attrIndex = -1;            // into PerfDataParser::attrs()
    CpuMode mode = CpuMode::Unknown;
    quint64 ip = 0;
    quint32 pid = 0;
    quint32 tid = 0;
    quint64 time = 0;
    quint64 addr = 0;
    quint64 id = 0;
    quint32 cpu = 0;
    quint64 period = 0;
    struct ReadValue
    {
        quint64 id = 0;            // 0 where read_format has no PERF_FORMAT_ID
        quint64 value = 0;
    };
    QList<ReadValue> readValues;   // PERF_SAMPLE_READ
    QList<quint64> callchain;      // innermost first, PERF_CONTEXT_* markers included
    QList<quint64> branchFroms;    // PERF_SAMPLE_BRANCH_STACK call sites, innermost first
    QByteArray raw;                // PERF_SAMPLE_RAW, a tracepoint's record
    QList<quint64> userRegs;       // indexed by perf register number; empty if none
    QByteArray userStack;          // PERF_SAMPLE_STACK_USER, starting at the user SP
};

struct Mmap
{
    CpuMode mode = CpuMode::Unknown;
    quint32 pid = 0;
    quint32 tid = 0;
    quint64 addr = 0;
    quint64 len = 0;
    quint64 pgoff = 0;
    bool executable = true;
    QString path;
    QByteArray buildId;            // raw bytes, where PERF_RECORD_MISC_MMAP_BUILD_ID gives one
    SampleId sampleId;
};

struct Comm
{
    quint32 pid = 0;
    quint32 tid = 0;
    QString name;
    bool exec = false;             // named by an exec, not a rename
    SampleId sampleId;
};

// PERF_RECORD_FORK and PERF_RECORD_EXIT.
struct Task
{
    bool exit = false;
    quint32 pid = 0;
    quint32 ppid = 0;
    quint32 tid = 0;
    quint32 ptid = 0;
    quint64 time = 0;
};

// PERF_RECORD_SWITCH and PERF_RECORD_SWITCH_CPU_WIDE.
struct ContextSwitch
{
    bool out = false;              // the task is switched out, not in
    SampleId sampleId;             // the task switched, and when
};

struct Lost
{
    quint64 id = 0;
    quint64 count = 0;
    SampleId sampleId;
};

struct Throttle
{
    bool unthrottle = false;
    quint64 time = 0;
    quint64 id = 0;
};

struct BuildId
{
    qint32 pid = -1;
    QByteArray buildId;            // raw bytes
    QString path;
};

// Receives the records of a recording in stream order, as PerfDataParser
// decodes them. Anything returning an error stops the parse with it.
class PROFILER_EXPORT PerfDataHandler
{
public:
    virtual ~PerfDataHandler() = default;

    virtual Utils::Result<> attrAdded(int index) { Q_UNUSED(index) return Utils::ResultOk; }
    virtual void mmap(const Mmap &) {}
    virtual void comm(const Comm &) {}
    virtual void task(const Task &) {}
    virtual void contextSwitch(const ContextSwitch &) {}
    virtual void lost(const Lost &) {}
    // PERF_RECORD_LOST_SAMPLES: a total, which includes what lost() reported.
    virtual void lostSamples(quint64 count) { Q_UNUSED(count) }
    virtual void throttle(const Throttle &) {}
    virtual Utils::Result<> sample(const Sample &) { return Utils::ResultOk; }
    // PERF_RECORD_FINISHED_ROUND: the samples so far may be sorted by time.
    virtual void finishedRound() {}
    // The tracepoint formats of the recording, as perf's tracing data holds
    // them (see PerfTracepointFormats).
    virtual void tracingData(const QByteArray &data) { Q_UNUSED(data) }
    virtual void buildId(const BuildId &) {}
};

// Decodes a perf recording -- the stream "perf record -o -" writes, or a
// perf.data file in either layout -- and hands its records to a handler.
// Little-endian recordings only, as perf writes them on every architecture Qt
// Creator profiles; compressed ones ("perf record -z") are rejected.
class PROFILER_EXPORT PerfDataParser
{
public:
    // Reads from `queue` until it reports atEnd(). A stream without a single
    // byte is no error: "perf record" never got to write one.
    Utils::Result<> parseStream(PerfByteQueue &queue, PerfDataHandler &handler);

    // Reads the file at `path`. `progress`, if set, is called with 0..100.
    Utils::Result<> parseFile(const Utils::FilePath &path, PerfDataHandler &handler,
                              const std::function<void(int)> &progress = {});

    const QList<EventAttr> &attrs() const { return m_attrs; }
    // The attr of the event with `id`, or -1.
    int attrIndexForId(quint64 id) const { return m_attrIndexById.value(id, -1); }

    // What the recording says about the machine it was taken on, where it
    // does: perf's "arch" feature, e.g. "x86_64" or "aarch64".
    QByteArray architecture() const { return m_architecture; }

    // Decodes one record: `record` is the whole of it, header included. Only
    // for the parse functions, and for tests.
    Utils::Result<> handleRecord(const QByteArray &record, PerfDataHandler &handler);

private:
    struct Header;

    Utils::Result<> handleAttrRecord(const QByteArray &record, PerfDataHandler &handler);
    Utils::Result<int> addAttr(const char *attr, quint32 attrSize, const QList<quint64> &ids);
    Utils::Result<> handleSample(quint16 misc, const QByteArray &record, PerfDataHandler &handler);
    void handleFeature(quint64 feature, const QByteArray &data, PerfDataHandler &handler);
    void handleEventDesc(const QByteArray &data);
    void handleBuildIds(const QByteArray &data, PerfDataHandler &handler);
    void handleEventUpdate(const QByteArray &record);
    // The attr of a non-sample record, for its trailer: the only one, or the
    // one its trailer's id names.
    const EventAttr *attrForTrailer(const QByteArray &record) const;
    SampleId readSampleId(const QByteArray &record, qsizetype bodyEnd) const;
    // After the pipe-mode header that `fill` starts with.
    Utils::Result<> parsePipe(const std::function<bool(qsizetype, QByteArray &)> &fill,
                              PerfDataHandler &handler,
                              const std::function<void(qint64)> &consumed = {});
    Utils::Result<> parseRecords(const std::function<bool(qsizetype, QByteArray &)> &fill,
                                 PerfDataHandler &handler,
                                 const std::function<void(qint64)> &consumed = {});

    QList<EventAttr> m_attrs;
    QHash<quint64, int> m_attrIndexById;
    QByteArray m_architecture;
};

} // namespace PerfData

} // namespace Profiler::Internal
