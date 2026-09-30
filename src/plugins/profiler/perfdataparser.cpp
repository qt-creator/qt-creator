// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfdataparser.h"

#include "perfrecordreader.h"
#include "profilertr.h"

#include <QFile>
#include <QtEndian>

#include <algorithm>
#include <bit>

using namespace Utils;
using namespace Qt::StringLiterals;

namespace Profiler::Internal::PerfData {

namespace {

// perf_event_header::type (include/uapi/linux/perf_event.h), and the types
// perf itself adds to a recording (tools/perf/util/event.h: user types start
// at 64).
enum RecordType : quint32 {
    RecordMmap = 1,
    RecordLost = 2,
    RecordComm = 3,
    RecordExit = 4,
    RecordThrottle = 5,
    RecordUnthrottle = 6,
    RecordFork = 7,
    RecordSample = 9,
    RecordMmap2 = 10,
    RecordLostSamples = 13,
    RecordSwitch = 14,
    RecordSwitchCpuWide = 15,
    RecordHeaderAttr = 64,
    RecordHeaderTracingData = 66,
    RecordHeaderBuildId = 67,
    RecordFinishedRound = 68,
    RecordEventUpdate = 78,
    RecordHeaderFeature = 80,
    RecordCompressed = 81,
    RecordCompressed2 = 83,
};

// perf_event_header::misc
constexpr quint16 MiscCpuModeMask = 7;
constexpr quint16 MiscSwitchOut = 1 << 13;
constexpr quint16 MiscCommExec = 1 << 13;
constexpr quint16 MiscMmapBuildId = 1 << 14;
constexpr quint16 MiscBuildIdSize = 1 << 15;

// perf_event_attr::sample_type, in the order a sample carries the fields.
constexpr quint64 SampleIp = 1ull << 0;
constexpr quint64 SampleTid = 1ull << 1;
constexpr quint64 SampleTime = 1ull << 2;
constexpr quint64 SampleAddr = 1ull << 3;
constexpr quint64 SampleRead = 1ull << 4;
constexpr quint64 SampleCallchain = 1ull << 5;
constexpr quint64 SampleIdBit = 1ull << 6;
constexpr quint64 SampleCpu = 1ull << 7;
constexpr quint64 SamplePeriod = 1ull << 8;
constexpr quint64 SampleStreamId = 1ull << 9;
constexpr quint64 SampleRaw = 1ull << 10;
constexpr quint64 SampleBranchStack = 1ull << 11;
constexpr quint64 SampleRegsUser = 1ull << 12;
constexpr quint64 SampleStackUser = 1ull << 13;
constexpr quint64 SampleIdentifier = 1ull << 16;

constexpr quint64 SupportedSampleBits = SampleIdentifier | SampleIp | SampleTid | SampleTime
                                        | SampleAddr | SampleRead | SampleCallchain | SampleIdBit
                                        | SampleCpu | SamplePeriod | SampleStreamId | SampleRaw
                                        | SampleBranchStack | SampleRegsUser | SampleStackUser;

// perf_event_attr::read_format
constexpr quint64 FormatTotalTimeEnabled = 1ull << 0;
constexpr quint64 FormatTotalTimeRunning = 1ull << 1;
constexpr quint64 FormatId = 1ull << 2;
constexpr quint64 FormatGroup = 1ull << 3;
constexpr quint64 FormatLost = 1ull << 4;

// perf_event_attr flags (the bitfield at offset 40)
constexpr quint64 AttrFlagFreq = 1ull << 10;
constexpr quint64 AttrFlagSampleIdAll = 1ull << 18;

// perf_event_attr::branch_sample_type
constexpr quint64 BranchHwIndex = 1ull << 17;

// perf.data header features (tools/perf/util/header.h), by bit number.
enum Feature : quint64 {
    FeatureTracingData = 1,
    FeatureBuildId = 2,
    FeatureArch = 6,
    FeatureEventDesc = 12,
};

// perf_event__update_type
constexpr quint64 EventUpdateName = 2;

constexpr qsizetype PipeHeaderSize = 16;
constexpr qsizetype FileHeaderSize = 104;

quint64 readU64(const char *p) { return qFromLittleEndian<quint64>(p); }
quint32 readU32(const char *p) { return qFromLittleEndian<quint32>(p); }
quint16 readU16(const char *p) { return qFromLittleEndian<quint16>(p); }

// A NUL-terminated string within [p, end), or up to end without a NUL.
QByteArray readCString(const char *p, const char *end)
{
    const char *nul = std::find(p, end, '\0');
    return QByteArray(p, nul - p);
}

// perf_header_string: u32 len, then len bytes, NUL-padded.
QByteArray readHeaderString(const char *&p, const char *end)
{
    if (p + 4 > end)
        return {};
    const quint32 len = readU32(p);
    p += 4;
    if (p + len > end) {
        p = end;
        return {};
    }
    const QByteArray string = readCString(p, p + len);
    p += len;
    return string;
}

CpuMode cpuMode(quint16 misc)
{
    switch (misc & MiscCpuModeMask) {
    case 1: return CpuMode::Kernel;
    case 2: return CpuMode::User;
    case 3: return CpuMode::Hypervisor;
    case 4: return CpuMode::GuestKernel;
    case 5: return CpuMode::GuestUser;
    default: return CpuMode::Unknown;
    }
}

Result<> truncated()
{
    return ResultError(Tr::tr("The perf recording is truncated or corrupt."));
}

} // namespace

Result<int> PerfDataParser::addAttr(const char *attr, quint32 attrSize, const QList<quint64> &ids)
{
    // perf_event_attr: u32 type (0); u32 size (4); u64 config (8); u64
    // sample_period/freq (16); u64 sample_type (24); u64 read_format (32);
    // u64 flags (40); ... u64 branch_sample_type (72); u64 sample_regs_user
    // (80). Shorter attrs, from older kernels, lack the later fields.
    if (attrSize < 40)
        return ResultError(Tr::tr("The perf recording has a truncated event attribute."));

    EventAttr info;
    info.type = readU32(attr);
    info.config = readU64(attr + 8);
    info.samplePeriodOrFreq = readU64(attr + 16);
    info.sampleType = readU64(attr + 24);
    info.readFormat = readU64(attr + 32);
    const quint64 flags = attrSize >= 48 ? readU64(attr + 40) : 0;
    info.freq = flags & AttrFlagFreq;
    info.sampleIdAll = flags & AttrFlagSampleIdAll;
    info.branchSampleType = attrSize >= 80 ? readU64(attr + 72) : 0;
    info.sampleRegsUser = attrSize >= 88 ? readU64(attr + 80) : 0;
    info.ids = ids;

    if (info.sampleType & ~SupportedSampleBits) {
        return ResultError(
            Tr::tr("The perf recording uses a sample format that cannot be decoded "
                   "(sample_type=0x%1).").arg(QString::number(info.sampleType, 16)));
    }

    const int index = int(m_attrs.size());
    m_attrs.append(info);
    for (quint64 id : ids)
        m_attrIndexById.insert(id, index);
    return index;
}

Result<> PerfDataParser::handleAttrRecord(const QByteArray &record, PerfDataHandler &handler)
{
    // PERF_RECORD_HEADER_ATTR: the attr, attr.size bytes of it, and then the
    // ids of the events opened with it, filling out the rest of the record.
    const char *attr = record.constData() + 8;
    if (record.size() < 8 + 8)
        return truncated();
    const quint32 attrSize = readU32(attr + 4);
    if (8 + qsizetype(attrSize) > record.size())
        return truncated();
    QList<quint64> ids;
    for (const char *p = attr + attrSize; p + 8 <= record.constData() + record.size(); p += 8)
        ids.append(readU64(p));
    const Result<int> index = addAttr(attr, attrSize, ids);
    if (!index)
        return ResultError(index.error());
    return handler.attrAdded(*index);
}

const EventAttr *PerfDataParser::attrForTrailer(const QByteArray &record) const
{
    if (m_attrs.isEmpty())
        return nullptr;
    if (m_attrs.size() == 1)
        return &m_attrs.first();
    // Multiplexed events all carry PERF_SAMPLE_IDENTIFIER, which is the last
    // field of the trailer, so that it can be found without knowing the attr.
    if ((m_attrs.first().sampleType & SampleIdentifier) && record.size() >= 16) {
        const int index = attrIndexForId(readU64(record.constData() + record.size() - 8));
        if (index >= 0)
            return &m_attrs.at(index);
    }
    return &m_attrs.first();
}

SampleId PerfDataParser::readSampleId(const QByteArray &record, qsizetype bodyEnd) const
{
    SampleId sampleId;
    const EventAttr *attr = attrForTrailer(record);
    if (!attr || !attr->sampleIdAll)
        return sampleId;
    const quint64 type = attr->sampleType;
    qsizetype size = 0;
    for (quint64 bit : {SampleTid, SampleTime, SampleIdBit, SampleStreamId, SampleCpu,
                        SampleIdentifier}) {
        if (type & bit)
            size += 8;
    }
    const qsizetype start = record.size() - size;
    if (start < bodyEnd)
        return sampleId;
    const char *p = record.constData() + start;
    if (type & SampleTid) {
        sampleId.pid = readU32(p);
        sampleId.tid = readU32(p + 4);
        p += 8;
    }
    if (type & SampleTime) {
        sampleId.time = readU64(p);
        sampleId.hasTime = true;
        p += 8;
    }
    if (type & SampleIdBit) {
        sampleId.id = readU64(p);
        p += 8;
    }
    if (type & SampleStreamId)
        p += 8;
    if (type & SampleCpu) {
        sampleId.cpu = readU32(p);
        p += 8;
    }
    if (type & SampleIdentifier)
        sampleId.id = readU64(p);
    return sampleId;
}

Result<> PerfDataParser::handleSample(quint16 misc, const QByteArray &record,
                                      PerfDataHandler &handler)
{
    if (m_attrs.isEmpty())
        return ResultError(Tr::tr("The perf recording has a sample before any event attribute."));

    const char *base = record.constData() + 8;
    const char *end = record.constData() + record.size();

    // Which attr a sample belongs to decides its layout. With several, the
    // kernel adds PERF_SAMPLE_IDENTIFIER, at offset 0, for exactly this; a bare
    // PERF_SAMPLE_ID is only unambiguous while every attr has the same layout.
    int attrIndex = 0;
    if (m_attrs.size() > 1) {
        const quint64 refType = m_attrs.first().sampleType;
        qsizetype idOffset = -1;
        if (refType & SampleIdentifier) {
            idOffset = 0;
        } else if (refType & SampleIdBit) {
            idOffset = 0;
            for (quint64 bit : {SampleIp, SampleTid, SampleTime, SampleAddr}) {
                if (refType & bit)
                    idOffset += 8;
            }
        }
        if (idOffset >= 0 && base + idOffset + 8 <= end) {
            const int index = attrIndexForId(readU64(base + idOffset));
            if (index >= 0)
                attrIndex = index;
        }
    }
    const EventAttr &attr = m_attrs.at(attrIndex);
    const quint64 type = attr.sampleType;

    Sample sample;
    sample.attrIndex = attrIndex;
    sample.mode = cpuMode(misc);

    const char *p = base;
    const auto take = [&](quint64 &out) {
        if (p + 8 > end)
            return false;
        out = readU64(p);
        p += 8;
        return true;
    };
    quint64 scratch = 0;
    if ((type & SampleIdentifier) && !take(sample.id))
        return truncated();
    if ((type & SampleIp) && !take(sample.ip))
        return truncated();
    if (type & SampleTid) {
        if (!take(scratch))
            return truncated();
        sample.pid = quint32(scratch);
        sample.tid = quint32(scratch >> 32);
    }
    if ((type & SampleTime) && !take(sample.time))
        return truncated();
    if ((type & SampleAddr) && !take(sample.addr))
        return truncated();
    if ((type & SampleIdBit) && !take(sample.id))
        return truncated();
    if ((type & SampleStreamId) && !take(scratch))
        return truncated();
    if (type & SampleCpu) {
        if (!take(scratch))
            return truncated();
        sample.cpu = quint32(scratch);
    }
    if ((type & SamplePeriod) && !take(sample.period))
        return truncated();

    if (type & SampleRead) {
        // Non-group: { value; [time_enabled]; [time_running]; [id]; [lost] }.
        // Group: { nr; [time_enabled]; [time_running]; { value; [id]; [lost] }[nr] }.
        const quint64 format = attr.readFormat;
        const auto readValue = [&](Sample::ReadValue &value) {
            if (!take(value.value))
                return false;
            if ((format & FormatId) && !take(value.id))
                return false;
            if ((format & FormatLost) && !take(scratch))
                return false;
            return true;
        };
        if (format & FormatGroup) {
            quint64 nr = 0;
            if (!take(nr))
                return truncated();
            if ((format & FormatTotalTimeEnabled) && !take(scratch))
                return truncated();
            if ((format & FormatTotalTimeRunning) && !take(scratch))
                return truncated();
            if (nr > quint64(end - p) / 8)
                return truncated();
            for (quint64 i = 0; i < nr; ++i) {
                Sample::ReadValue value;
                if (!readValue(value))
                    return truncated();
                sample.readValues.append(value);
            }
        } else {
            Sample::ReadValue value;
            if (!take(value.value))
                return truncated();
            if ((format & FormatTotalTimeEnabled) && !take(scratch))
                return truncated();
            if ((format & FormatTotalTimeRunning) && !take(scratch))
                return truncated();
            if ((format & FormatId) && !take(value.id))
                return truncated();
            if ((format & FormatLost) && !take(scratch))
                return truncated();
            sample.readValues.append(value);
        }
    }

    if (type & SampleCallchain) {
        quint64 nr = 0;
        if (!take(nr) || nr > quint64(end - p) / 8)
            return truncated();
        sample.callchain.reserve(qsizetype(nr));
        for (quint64 i = 0; i < nr; ++i) {
            sample.callchain.append(readU64(p));
            p += 8;
        }
    } else if (type & SampleIp) {
        sample.callchain.append(sample.ip);
    }

    if (type & SampleRaw) {
        if (p + 4 > end)
            return truncated();
        const quint32 size = readU32(p);
        p += 4;
        if (size > quint64(end - p))
            return truncated();
        sample.raw = QByteArray(p, size);
        p += size;
    }

    if (type & SampleBranchStack) {
        // { nr; [hw_idx]; { from; to; flags }[nr] }
        quint64 nr = 0;
        if (!take(nr))
            return truncated();
        if ((attr.branchSampleType & BranchHwIndex) && !take(scratch))
            return truncated();
        constexpr qsizetype EntrySize = 24;
        if (nr > quint64(end - p) / EntrySize)
            return truncated();
        for (quint64 i = 0; i < nr; ++i) {
            sample.branchFroms.append(readU64(p));
            p += EntrySize;
        }
    }

    if (type & SampleRegsUser) {
        // { abi; regs[popcount(sample_regs_user)] }, the registers in
        // ascending bit order, and none at all where abi is 0.
        quint64 abi = 0;
        if (!take(abi))
            return truncated();
        if (abi != 0) {
            const quint64 mask = attr.sampleRegsUser;
            const int count = std::popcount(mask);
            if (p + qsizetype(count) * 8 > end)
                return truncated();
            sample.userRegs.resize(64 - std::countl_zero(mask), 0);
            for (int bit = 0; bit < 64; ++bit) {
                if (mask & (1ull << bit)) {
                    sample.userRegs[bit] = readU64(p);
                    p += 8;
                }
            }
        }
    }

    if (type & SampleStackUser) {
        // { size; data[size]; dyn_size }, dyn_size only where size is not 0:
        // how much of the requested size the kernel could copy.
        quint64 size = 0;
        if (!take(size) || size > quint64(end - p))
            return truncated();
        const char *stack = p;
        p += size;
        quint64 dynSize = 0;
        if (size != 0 && !take(dynSize))
            return truncated();
        sample.userStack = QByteArray(stack, qsizetype(qMin(dynSize, size)));
    }

    return handler.sample(sample);
}

void PerfDataParser::handleEventDesc(const QByteArray &data)
{
    // HEADER_EVENT_DESC: { u32 nr; u32 attr_size; { attr; u32 nr_ids;
    // perf_header_string name; u64 ids[nr_ids]; }[nr] }
    const char *p = data.constData();
    const char *end = p + data.size();
    if (p + 8 > end)
        return;
    const quint32 nr = readU32(p);
    const quint32 attrSize = readU32(p + 4);
    p += 8;
    for (quint32 i = 0; i < nr; ++i) {
        if (p + attrSize + 4 > end)
            return;
        p += attrSize;
        const quint32 nrIds = readU32(p);
        p += 4;
        const QByteArray name = readHeaderString(p, end);
        if (p + qsizetype(nrIds) * 8 > end)
            return;
        for (quint32 j = 0; j < nrIds; ++j) {
            const int index = attrIndexForId(readU64(p));
            if (index >= 0 && m_attrs[index].name.isEmpty())
                m_attrs[index].name = name;
            p += 8;
        }
    }
}

void PerfDataParser::handleBuildIds(const QByteArray &data, PerfDataHandler &handler)
{
    // A sequence of build id records: header; s32 pid; u8 build_id[24];
    // filename. With PERF_RECORD_MISC_BUILD_ID_SIZE, build_id[20] is its size.
    const char *p = data.constData();
    const char *end = p + data.size();
    while (p + 8 <= end) {
        const quint16 misc = readU16(p + 4);
        const quint16 size = readU16(p + 6);
        if (size < 8 + 4 + 24 || p + size > end)
            return;
        BuildId buildId;
        buildId.pid = qint32(readU32(p + 8));
        const char *id = p + 12;
        int idSize = 20;
        if (misc & MiscBuildIdSize)
            idSize = qMin(int(quint8(id[20])), 20);
        buildId.buildId = QByteArray(id, idSize);
        buildId.path = QString::fromUtf8(readCString(p + 12 + 24, p + size));
        handler.buildId(buildId);
        p += size;
    }
}

void PerfDataParser::handleFeature(quint64 feature, const QByteArray &data,
                                   PerfDataHandler &handler)
{
    switch (feature) {
    case FeatureTracingData:
        handler.tracingData(data);
        break;
    case FeatureBuildId:
        handleBuildIds(data, handler);
        break;
    case FeatureArch: {
        const char *p = data.constData();
        m_architecture = readHeaderString(p, p + data.size());
        break;
    }
    case FeatureEventDesc:
        handleEventDesc(data);
        break;
    default:
        break;
    }
}

void PerfDataParser::handleEventUpdate(const QByteArray &record)
{
    // PERF_RECORD_EVENT_UPDATE: { u64 type; u64 id; data }
    if (record.size() < 8 + 16)
        return;
    const char *p = record.constData() + 8;
    if (readU64(p) != EventUpdateName)
        return;
    const int index = attrIndexForId(readU64(p + 8));
    if (index >= 0)
        m_attrs[index].name = readCString(p + 16, record.constData() + record.size());
}

Result<> PerfDataParser::handleRecord(const QByteArray &record, PerfDataHandler &handler)
{
    const char *r = record.constData();
    const char *end = r + record.size();
    const quint32 type = readU32(r);
    const quint16 misc = readU16(r + 4);
    const char *p = r + 8;

    switch (type) {
    case RecordHeaderAttr:
        return handleAttrRecord(record, handler);
    case RecordSample:
        return handleSample(misc, record, handler);
    case RecordMmap:
    case RecordMmap2: {
        // MMAP: { pid; tid; addr; len; pgoff; filename }. MMAP2 adds, before
        // the filename, either { maj; min; ino; ino_generation } or, with
        // PERF_RECORD_MISC_MMAP_BUILD_ID, { u8 size; u8 res; u16 res; build_id[20] },
        // and then { prot; flags }.
        if (p + 32 > end)
            return truncated();
        Mmap mmap;
        mmap.mode = cpuMode(misc);
        mmap.pid = readU32(p);
        mmap.tid = readU32(p + 4);
        mmap.addr = readU64(p + 8);
        mmap.len = readU64(p + 16);
        mmap.pgoff = readU64(p + 24);
        p += 32;
        if (type == RecordMmap2) {
            if (p + 24 + 8 > end)
                return truncated();
            if (misc & MiscMmapBuildId)
                mmap.buildId = QByteArray(p + 4, qMin(int(quint8(*p)), 20));
            p += 24;
            constexpr quint32 ProtExec = 4;
            mmap.executable = readU32(p) & ProtExec;
            p += 8;
        }
        mmap.path = QString::fromUtf8(readCString(p, end));
        mmap.sampleId = readSampleId(record, p - r);
        handler.mmap(mmap);
        return ResultOk;
    }
    case RecordComm: {
        // { pid; tid; comm }
        if (p + 8 > end)
            return truncated();
        Comm comm;
        comm.pid = readU32(p);
        comm.tid = readU32(p + 4);
        comm.name = QString::fromUtf8(readCString(p + 8, end));
        comm.exec = misc & MiscCommExec;
        comm.sampleId = readSampleId(record, 8 + 8);
        handler.comm(comm);
        return ResultOk;
    }
    case RecordFork:
    case RecordExit: {
        // { pid; ppid; tid; ptid; time }
        if (p + 24 > end)
            return truncated();
        Task task;
        task.exit = type == RecordExit;
        task.pid = readU32(p);
        task.ppid = readU32(p + 4);
        task.tid = readU32(p + 8);
        task.ptid = readU32(p + 12);
        task.time = readU64(p + 16);
        handler.task(task);
        return ResultOk;
    }
    case RecordSwitch:
    case RecordSwitchCpuWide: {
        // SWITCH: only the trailer. SWITCH_CPU_WIDE: { next_prev_pid;
        // next_prev_tid } before it; the task switched is the trailer's.
        ContextSwitch contextSwitch;
        contextSwitch.out = misc & MiscSwitchOut;
        contextSwitch.sampleId = readSampleId(record, type == RecordSwitch ? 8 : 16);
        handler.contextSwitch(contextSwitch);
        return ResultOk;
    }
    case RecordLost: {
        // { id; lost }
        if (p + 16 > end)
            return truncated();
        Lost lost;
        lost.id = readU64(p);
        lost.count = readU64(p + 8);
        lost.sampleId = readSampleId(record, 8 + 16);
        handler.lost(lost);
        return ResultOk;
    }
    case RecordLostSamples:
        // { lost }
        if (p + 8 > end)
            return truncated();
        handler.lostSamples(readU64(p));
        return ResultOk;
    case RecordThrottle:
    case RecordUnthrottle: {
        // { time; id; stream_id }
        if (p + 16 > end)
            return truncated();
        Throttle throttle;
        throttle.unthrottle = type == RecordUnthrottle;
        throttle.time = readU64(p);
        throttle.id = readU64(p + 8);
        handler.throttle(throttle);
        return ResultOk;
    }
    case RecordFinishedRound:
        handler.finishedRound();
        return ResultOk;
    case RecordHeaderBuildId:
        handleBuildIds(record, handler);
        return ResultOk;
    case RecordEventUpdate:
        handleEventUpdate(record);
        return ResultOk;
    case RecordHeaderFeature:
        // { u64 feat_id; the feature's data as a file section holds it }
        if (p + 8 > end)
            return truncated();
        handleFeature(readU64(p), record.mid(16), handler);
        return ResultOk;
    case RecordCompressed:
    case RecordCompressed2:
        return ResultError(Tr::tr("The perf recording is compressed (\"perf record -z\"), which "
                                  "cannot be decoded. Record without compression."));
    default:
        return ResultOk; // not needed to tell what ran
    }
}

Result<> PerfDataParser::parseRecords(const std::function<bool(qsizetype, QByteArray &)> &fill,
                                      PerfDataHandler &handler,
                                      const std::function<void(qint64)> &consumed)
{
    QByteArray buffer;
    qint64 total = 0;
    for (;;) {
        if (!fill(8, buffer))
            return ResultOk; // the end, or a stop mid-record: keep what was decoded
        const quint32 type = readU32(buffer.constData());
        const quint16 size = readU16(buffer.constData() + 6);
        if (size < 8)
            return truncated();
        if (!fill(size, buffer))
            return ResultOk;
        const QByteArray record = buffer.left(size);
        buffer.remove(0, size);
        total += size;

        if (type == RecordHeaderTracingData) {
            // { u32 size }, and then that much tracing data -- padded to 8
            // bytes -- following the record rather than inside it.
            if (record.size() < 12)
                return truncated();
            const quint32 dataSize = readU32(record.constData() + 8);
            if (!fill(dataSize, buffer))
                return ResultOk;
            handler.tracingData(buffer.left(dataSize));
            buffer.remove(0, dataSize);
            total += dataSize;
        } else if (Result<> r = handleRecord(record, handler); !r) {
            return r;
        }
        if (consumed)
            consumed(total);
    }
}

Result<> PerfDataParser::parseStream(PerfByteQueue &queue, PerfDataHandler &handler)
{
    return parsePipe([&queue](qsizetype need, QByteArray &buffer) {
        while (buffer.size() < need) {
            if (queue.atEnd())
                return false;
            buffer += queue.pop();
        }
        return true;
    }, handler);
}

Result<> PerfDataParser::parsePipe(const std::function<bool(qsizetype, QByteArray &)> &fill,
                                   PerfDataHandler &handler,
                                   const std::function<void(qint64)> &consumed)
{
    // Pipe-mode framing: "PERFILE2", then the header's own size, 16.
    QByteArray header;
    if (!fill(PipeHeaderSize, header)) {
        if (header.isEmpty())
            return ResultOk;
        return truncated();
    }
    if (header.startsWith("2ELIFREP"))
        return ResultError(Tr::tr("The perf recording is big-endian, which cannot be decoded."));
    if (!header.startsWith("PERFILE2"))
        return ResultError(Tr::tr("This is not a perf recording."));
    const quint64 headerSize = readU64(header.constData() + 8);
    if (headerSize < PipeHeaderSize || !fill(qsizetype(headerSize), header))
        return truncated();
    header.remove(0, qsizetype(headerSize));

    return parseRecords([&header, &fill](qsizetype need, QByteArray &buffer) {
        if (!header.isEmpty()) {
            buffer += header;
            header.clear();
        }
        return fill(need, buffer);
    }, handler, consumed ? [&consumed, headerSize](qint64 bytes) {
        consumed(qint64(headerSize) + bytes);
    } : std::function<void(qint64)>());
}

Result<> PerfDataParser::parseFile(const FilePath &path, PerfDataHandler &handler,
                                   const std::function<void(int)> &progress)
{
    QFile file(path.toFSPathString());
    if (!file.open(QIODevice::ReadOnly))
        return ResultError(Tr::tr("Cannot read %1.").arg(path.toUserOutput()));

    const QByteArray header = file.read(FileHeaderSize);
    if (header.size() < PipeHeaderSize)
        return ResultError(Tr::tr("%1 is not a perf recording.").arg(path.toUserOutput()));
    if (header.startsWith("2ELIFREP")) {
        return ResultError(Tr::tr("%1 is a big-endian perf recording, which cannot be decoded.")
                               .arg(path.toUserOutput()));
    }
    if (!header.startsWith("PERFILE2"))
        return ResultError(Tr::tr("%1 is not a perf recording.").arg(path.toUserOutput()));

    const qint64 fileSize = file.size();
    int lastPercent = -1;
    const auto reportProgress = [&progress, &lastPercent](qint64 consumed, qint64 total) {
        const int percent = int(qMin<qint64>(consumed * 100 / qMax<qint64>(1, total), 100));
        if (progress && percent != lastPercent) {
            lastPercent = percent;
            progress(percent);
        }
    };

    // A stream "perf record -o -" wrote to a file has the stream's framing.
    if (readU64(header.constData() + 8) == PipeHeaderSize) {
        if (!file.seek(0))
            return ResultError(Tr::tr("Cannot read %1.").arg(path.toUserOutput()));
        const Result<> result = parsePipe([&file](qsizetype need, QByteArray &buffer) {
            while (buffer.size() < need) {
                const QByteArray chunk = file.read(1 << 20);
                if (chunk.isEmpty())
                    return false;
                buffer += chunk;
            }
            return true;
        }, handler, [&reportProgress, fileSize](qint64 consumed) {
            reportProgress(consumed, fileSize);
        });
        if (result)
            reportProgress(fileSize, fileSize);
        return result;
    }

    // perf_file_header: magic; size; attr_size; attrs {offset, size}; data
    // {offset, size}; event_types {offset, size}; adds_features[4].
    if (header.size() < FileHeaderSize)
        return ResultError(Tr::tr("%1 is truncated.").arg(path.toUserOutput()));
    const char *h = header.constData();
    const quint64 attrEntrySize = readU64(h + 16);
    const quint64 attrsOffset = readU64(h + 24);
    const quint64 attrsSize = readU64(h + 32);
    const quint64 dataOffset = readU64(h + 40);
    const quint64 dataSize = readU64(h + 48);
    quint64 features[4];
    for (int i = 0; i < 4; ++i)
        features[i] = readU64(h + 72 + i * 8);

    // Where a corrupt header points beyond the file, rather than reading
    // (or allocating) what is not there.
    const auto readSection = [&file, fileSize](quint64 offset,
                                               quint64 size) -> std::optional<QByteArray> {
        if (offset > quint64(fileSize) || size > quint64(fileSize) - offset)
            return std::nullopt;
        if (!file.seek(qint64(offset)))
            return std::nullopt;
        QByteArray data = file.read(qint64(size));
        if (quint64(data.size()) != size)
            return std::nullopt;
        return data;
    };

    // Attrs: { attr (attr_size - 16 bytes); ids {offset, size} }[].
    if (attrEntrySize < 16 + 40)
        return ResultError(Tr::tr("%1 is corrupt.").arg(path.toUserOutput()));
    const std::optional<QByteArray> attrs = readSection(attrsOffset, attrsSize);
    if (!attrs)
        return ResultError(Tr::tr("%1 is truncated.").arg(path.toUserOutput()));
    for (quint64 at = 0; at + attrEntrySize <= quint64(attrs->size()); at += attrEntrySize) {
        const char *attr = attrs->constData() + at;
        const quint32 attrSize = quint32(attrEntrySize - 16);
        const char *idsSection = attr + attrSize;
        const std::optional<QByteArray> idBytes = readSection(readU64(idsSection),
                                                              readU64(idsSection + 8));
        if (!idBytes)
            return ResultError(Tr::tr("%1 is truncated.").arg(path.toUserOutput()));
        QList<quint64> ids;
        for (qsizetype i = 0; i + 8 <= idBytes->size(); i += 8)
            ids.append(readU64(idBytes->constData() + i));
        const Result<int> index = addAttr(attr, attrSize, ids);
        if (!index)
            return ResultError(index.error());
        if (Result<> r = handler.attrAdded(*index); !r)
            return r;
    }

    // Features: one {offset, size} per set bit, in bit order, after the data.
    QList<quint64> presentFeatures;
    for (quint64 bit = 0; bit < 256; ++bit) {
        if (features[bit / 64] & (1ull << (bit % 64)))
            presentFeatures.append(bit);
    }
    const std::optional<QByteArray> featureSections
        = readSection(dataOffset + dataSize, quint64(presentFeatures.size()) * 16);
    if (featureSections) {
        for (qsizetype i = 0; i < presentFeatures.size(); ++i) {
            const char *section = featureSections->constData() + i * 16;
            if (const std::optional<QByteArray> data = readSection(readU64(section),
                                                                   readU64(section + 8))) {
                handleFeature(presentFeatures.at(i), *data, handler);
            }
        }
    }

    if (!file.seek(qint64(dataOffset)))
        return ResultError(Tr::tr("%1 is truncated.").arg(path.toUserOutput()));
    qint64 remaining = qint64(dataSize);
    const auto fill = [&file, &remaining](qsizetype need, QByteArray &buffer) {
        while (buffer.size() < need) {
            if (remaining <= 0)
                return false;
            const QByteArray chunk = file.read(qMin<qint64>(remaining, 1 << 20));
            if (chunk.isEmpty())
                return false;
            remaining -= chunk.size();
            buffer += chunk;
        }
        return true;
    };
    const Result<> result = parseRecords(fill, handler, [&reportProgress, dataSize](qint64 bytes) {
        reportProgress(bytes, qint64(dataSize));
    });
    if (result)
        reportProgress(qint64(dataSize), qint64(dataSize));
    return result;
}

} // namespace Profiler::Internal::PerfData
