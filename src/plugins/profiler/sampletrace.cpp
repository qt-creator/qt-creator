// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sampletrace.h"

#include "profilertr.h"

#include <commontraceformat/binary/fieldvalue.h>
#include <commontraceformat/schema/clockclass.h>
#include <commontraceformat/schema/compoundfieldclasses.h>
#include <commontraceformat/schema/scalarfieldclasses.h>
#include <commontraceformat/schema/stringfieldclasses.h>
#include <commontraceformat/stream/datastreamreader.h>
#include <commontraceformat/stream/datastreamwriter.h>
#include <commontraceformat/stream/tracereader.h>
#include <commontraceformat/stream/tracewriter.h>

#include <QDateTime>
#include <QFile>
#include <QStandardPaths>

#include <algorithm>
#include <limits>

using namespace CommonTraceFormat;
using namespace Utils;
using namespace Qt::StringLiterals;

namespace Profiler::Internal {

// Event class ids in the sampler stream.
enum SamplerEventClass : quint64 {
    LabelEvent = 0,
    ThreadEvent = 1,
    SampleEvent = 2,
    LostEvent = 3,
    ThrottleEvent = 4,
};

static std::shared_ptr<FixedLengthUIntFC> u64Field()
{
    auto f = std::make_shared<FixedLengthUIntFC>();
    f->length = 64;
    return f;
}

// The sampler trace is one data stream: per-label and per-thread definition
// events (timestamp 0) followed by one `sample` event per thread per tick,
// carrying the on-CPU flag and the root-first stack as label ids.
static Schema buildSamplerSchema()
{
    Schema schema;

    ClockClass clock;
    clock.id = u"us"_s;
    clock.name = u"us"_s;
    clock.frequency = 1000000ULL; // timestamps are microseconds
    schema.clockClasses.append(clock);

    DataStreamClass dsc;
    dsc.id = 0;
    dsc.name = samplerStreamName;
    dsc.defaultClockClassName = u"us"_s;

    // Header: { uint64 id, uint64 timestamp }, filled from the role annotations.
    {
        auto header = std::make_shared<StructureFC>();
        auto idField = std::make_shared<FixedLengthUIntFC>();
        idField->length = 64;
        idField->roles = {UIntRole::EventRecordClassId};
        header->members.append({u"id"_s, std::move(idField), {}});

        auto tsField = std::make_shared<FixedLengthUIntFC>();
        tsField->length = 64;
        tsField->roles = {UIntRole::DefaultClockTimestamp};
        header->members.append({u"timestamp"_s, std::move(tsField), {}});
        dsc.eventRecordHeaderFieldClass = std::move(header);
    }

    // Common context for every event: { uint64 pid, uint64 tid }.
    {
        auto ctx = std::make_shared<StructureFC>();
        ctx->members.append({u"pid"_s, u64Field(), {}});
        ctx->members.append({u"tid"_s, u64Field(), {}});
        dsc.eventRecordCommonContextFieldClass = std::move(ctx);
    }

    // label: { uint64 id, string name, string file, uint64 line } — one per distinct frame label.
    {
        EventRecordClass erc;
        erc.id = LabelEvent;
        erc.name = u"label"_s;
        auto payload = std::make_shared<StructureFC>();
        payload->members.append({u"id"_s, u64Field(), {}});
        payload->members.append({u"name"_s, std::make_shared<NullTerminatedStringFC>(), {}});
        payload->members.append({u"file"_s, std::make_shared<NullTerminatedStringFC>(), {}});
        payload->members.append({u"line"_s, u64Field(), {}});
        payload->members.append({u"module"_s, std::make_shared<NullTerminatedStringFC>(), {}});
        payload->members.append({u"offset"_s, u64Field(), {}});
        erc.payloadFieldClass = std::move(payload);
        dsc.eventRecordClasses.append(std::move(erc));
    }

    // thread: { uint64 tid, string name } — one per captured thread.
    {
        EventRecordClass erc;
        erc.id = ThreadEvent;
        erc.name = u"thread"_s;
        auto payload = std::make_shared<StructureFC>();
        payload->members.append({u"tid"_s, u64Field(), {}});
        payload->members.append({u"name"_s, std::make_shared<NullTerminatedStringFC>(), {}});
        erc.payloadFieldClass = std::move(payload);
        dsc.eventRecordClasses.append(std::move(erc));
    }

    // sample: { uint8 running, uint64 depth, uint64 stack[depth] }. The array
    // length must live in a sibling member referenced by location (CTF2 5.3.21).
    {
        EventRecordClass erc;
        erc.id = SampleEvent;
        erc.name = u"sample"_s;
        auto payload = std::make_shared<StructureFC>();

        auto runningField = std::make_shared<FixedLengthUIntFC>();
        runningField->length = 8;
        payload->members.append({u"running"_s, std::move(runningField), {}});

        payload->members.append({u"depth"_s, u64Field(), {}});

        auto stack = std::make_shared<DynamicLengthArrayFC>();
        stack->elementFieldClass = u64Field();
        stack->lengthFieldLocation.hasOrigin = false;
        stack->lengthFieldLocation.path = {std::optional<QString>(u"depth"_s)};
        payload->members.append({u"stack"_s, std::move(stack), {}});

        erc.payloadFieldClass = std::move(payload);
        dsc.eventRecordClasses.append(std::move(erc));
    }

    // lost: { uint64 count } -- samples dropped around the event's timestamp.
    {
        EventRecordClass erc;
        erc.id = LostEvent;
        erc.name = u"lost"_s;
        auto payload = std::make_shared<StructureFC>();
        payload->members.append({u"count"_s, u64Field(), {}});
        erc.payloadFieldClass = std::move(payload);
        dsc.eventRecordClasses.append(std::move(erc));
    }

    // throttle: {} -- sampling was throttled at the event's timestamp.
    {
        EventRecordClass erc;
        erc.id = ThrottleEvent;
        erc.name = u"throttle"_s;
        erc.payloadFieldClass = std::make_shared<StructureFC>();
        dsc.eventRecordClasses.append(std::move(erc));
    }

    schema.dataStreamClasses.append(std::move(dsc));
    return schema;
}

static const QString pausedRangesFileName = u"paused-ranges"_s;

QString incompleteTraceWarning(const SampleTraceData &data)
{
    quint64 lost = 0;
    for (const SampleTraceData::LostSamples &gap : data.lostSamples)
        lost += gap.count;
    const int throttled = int(data.throttledTsUs.size());
    if (lost == 0 && throttled == 0)
        return {};

    QStringList reasons;
    if (lost > 0) {
        reasons << Tr::tr("%n sample(s) were lost because the profiler could not keep up.",
                          nullptr, int(qMin<quint64>(lost, std::numeric_limits<int>::max())));
    }
    if (throttled > 0) {
        reasons << Tr::tr("The system throttled sampling %n time(s) because samples were due "
                          "faster than it takes them.", nullptr, throttled);
    }
    return Tr::tr("The trace does not show everything that ran: %1 Time spent around those "
                  "points is underrepresented. Record with a lower sampling frequency to avoid "
                  "this.")
        .arg(reasons.join(u' '));
}

Result<> writeSampleTrace(const SampleTraceData &data, const FilePath &dir,
                          const std::function<void(int)> &progress)
{
    QFile metaFile(dir.pathAppended(u"metadata"_s).toFSPathString());
    if (!metaFile.open(QIODevice::WriteOnly))
        return ResultError(Tr::tr("Cannot write %1.").arg(metaFile.fileName()));

    QFile dataFile(dir.pathAppended(u"stream0"_s).toFSPathString());
    if (!dataFile.open(QIODevice::WriteOnly))
        return ResultError(Tr::tr("Cannot write %1.").arg(dataFile.fileName()));

    auto twResult = TraceWriter::create(buildSamplerSchema(), &metaFile);
    if (!twResult)
        return ResultError(twResult.error());
    TraceWriter &tw = *twResult;

    DataStreamWriter *writer = tw.openStreamById(0, &dataFile);
    if (!writer)
        return ResultError(Tr::tr("Cannot open the sample data stream."));

    StructureValue processCtx;
    processCtx.set(u"pid"_s, data.pid);
    processCtx.set(u"tid"_s, quint64(0));

    for (qsizetype i = 0; i < data.labels.size(); ++i) {
        const SampleTraceData::Label &label = data.labels.at(i);
        StructureValue payload;
        payload.set(u"id"_s, quint64(i));
        payload.set(u"name"_s, label.name);
        payload.set(u"file"_s, label.file);
        payload.set(u"line"_s, quint64(std::max(0, label.line)));
        payload.set(u"module"_s, label.module);
        payload.set(u"offset"_s, label.offset);
        if (auto r = writer->writeEvent(LabelEvent, payload, {}, processCtx, 0); !r)
            return ResultError(r.error());
    }

    for (auto it = data.threadNames.cbegin(); it != data.threadNames.cend(); ++it) {
        StructureValue payload;
        payload.set(u"tid"_s, it.key());
        payload.set(u"name"_s, it.value());
        if (auto r = writer->writeEvent(ThreadEvent, payload, {}, processCtx, 0); !r)
            return ResultError(r.error());
    }

    // Written in timestamp order along with the samples, as a stream's clock
    // only moves forward: everything up to `tsUs` that is not a sample.
    qsizetype nextLost = 0;
    qsizetype nextThrottle = 0;
    const auto writeGapsUpTo = [&](quint64 tsUs) -> Result<> {
        for (;;) {
            const bool haveLost = nextLost < data.lostSamples.size()
                                  && data.lostSamples.at(nextLost).tsUs <= tsUs;
            const bool haveThrottle = nextThrottle < data.throttledTsUs.size()
                                      && data.throttledTsUs.at(nextThrottle) <= tsUs;
            if (!haveLost && !haveThrottle)
                return ResultOk;
            // Whichever comes first, the two being in time order each.
            if (haveLost && (!haveThrottle
                             || data.lostSamples.at(nextLost).tsUs
                                    <= data.throttledTsUs.at(nextThrottle))) {
                const SampleTraceData::LostSamples &lost = data.lostSamples.at(nextLost++);
                StructureValue payload;
                payload.set(u"count"_s, lost.count);
                if (auto r = writer->writeEvent(LostEvent, payload, {}, processCtx, lost.tsUs); !r)
                    return r;
            } else {
                const quint64 throttledUs = data.throttledTsUs.at(nextThrottle++);
                if (auto r = writer->writeEvent(ThrottleEvent, {}, {}, processCtx, throttledUs);
                    !r) {
                    return r;
                }
            }
        }
    };

    const int total = std::max<int>(1, int(data.samples.size()));
    int written = 0;
    for (const SampleTraceData::ThreadSample &sample : data.samples) {
        if (Result<> r = writeGapsUpTo(sample.tsUs); !r)
            return r;

        StructureValue payload;
        payload.set(u"running"_s, quint64(sample.running ? 1 : 0));
        payload.set(u"depth"_s, quint64(sample.frames.size()));
        ArrayValue stack;
        stack.elements.reserve(sample.frames.size());
        for (int frame : sample.frames)
            stack.elements.append(quint64(frame));
        payload.set(u"stack"_s, makeArrayValue(std::move(stack)));

        StructureValue ctx;
        ctx.set(u"pid"_s, data.pid);
        ctx.set(u"tid"_s, sample.tid);

        if (auto r = writer->writeEvent(SampleEvent, payload, {}, ctx, sample.tsUs); !r)
            return ResultError(r.error());
        if (progress && (++written & 0x3ff) == 0)
            progress(written * 100 / total);
    }
    if (Result<> r = writeGapsUpTo(std::numeric_limits<quint64>::max()); !r)
        return r;

    if (auto r = tw.close(); !r)
        return ResultError(r.error());

    if (!data.pausedRangesUs.isEmpty()) {
        QFile pausedFile(dir.pathAppended(pausedRangesFileName).toFSPathString());
        if (!pausedFile.open(QIODevice::WriteOnly | QIODevice::Text))
            return ResultError(Tr::tr("Cannot write %1.").arg(pausedFile.fileName()));
        for (const auto &[start, end] : data.pausedRangesUs)
            pausedFile.write(QByteArray::number(start) + ' ' + QByteArray::number(end) + '\n');
    }
    return ResultOk;
}

static quint64 uintField(const StructureValue &sv, const QString &name)
{
    const FieldValue *v = sv.get(name);
    if (!v)
        return 0;
    if (const auto *u = std::get_if<quint64>(v))
        return *u;
    if (const auto *i = std::get_if<qint64>(v))
        return quint64(*i);
    return 0;
}

static QString stringField(const StructureValue &sv, const QString &name)
{
    const FieldValue *v = sv.get(name);
    if (!v)
        return {};
    if (const auto *s = std::get_if<QString>(v))
        return *s;
    return {};
}

Result<SampleTraceData> readSampleTrace(const FilePath &dir, const std::function<void(int)> &progress)
{
    QFile metaFile(dir.pathAppended(u"metadata"_s).toFSPathString());
    if (!metaFile.open(QIODevice::ReadOnly))
        return ResultError(Tr::tr("Cannot read %1.").arg(metaFile.fileName()));

    auto readerResult = TraceReader::open(&metaFile);
    if (!readerResult)
        return ResultError(readerResult.error());
    TraceReader &reader = *readerResult;

    const DataStreamClass *dsc = nullptr;
    for (const DataStreamClass &cls : reader.schema().dataStreamClasses) {
        if (cls.name == samplerStreamName)
            dsc = &cls;
    }
    if (!dsc)
        return ResultError(Tr::tr("%1 is not a sampler trace.").arg(dir.toUserOutput()));

    QFile dataFile(dir.pathAppended(u"stream0"_s).toFSPathString());
    if (!dataFile.open(QIODevice::ReadOnly))
        return ResultError(Tr::tr("Cannot read %1.").arg(dataFile.fileName()));

    DataStreamReader *stream = reader.openStream(*dsc, &dataFile);
    if (!stream)
        return ResultError(Tr::tr("Cannot open the sample data stream."));

    // Events are small and numerous, so progress is sampled every so often
    // rather than computed per record.
    const qint64 totalBytes = std::max<qint64>(1, dataFile.size());
    int eventCount = 0;
    int lastPercent = -1;

    SampleTraceData data;
    while (true) {
        auto rec = stream->nextEvent();
        if (!rec) {
            if (stream->atEnd())
                break;
            return ResultError(rec.error());
        }
        if (progress && (++eventCount & 0x3ff) == 0) {
            const int percent = int(stream->bytesDecoded() * 100 / totalBytes);
            if (percent != lastPercent) {
                lastPercent = percent;
                progress(percent);
            }
        }
        data.pid = uintField(rec->commonContext, u"pid"_s);
        switch (rec->eventClassId) {
        case LabelEvent: {
            const int id = int(uintField(rec->payload, u"id"_s));
            if (data.labels.size() <= id)
                data.labels.resize(id + 1);
            data.labels[id] = SampleTraceData::Label{
                stringField(rec->payload, u"name"_s),
                stringField(rec->payload, u"file"_s),
                int(uintField(rec->payload, u"line"_s)),
                stringField(rec->payload, u"module"_s),
                uintField(rec->payload, u"offset"_s)};
            break;
        }
        case ThreadEvent: {
            data.threadNames.insert(uintField(rec->payload, u"tid"_s),
                                    stringField(rec->payload, u"name"_s));
            break;
        }
        case SampleEvent: {
            SampleTraceData::ThreadSample sample;
            sample.tsUs = rec->timestamp;
            sample.tid = uintField(rec->commonContext, u"tid"_s);
            sample.running = uintField(rec->payload, u"running"_s) != 0;
            if (const FieldValue *stack = rec->payload.get(u"stack"_s);
                stack && isArray(*stack)) {
                const ArrayValue &av = asArray(*stack);
                sample.frames.reserve(av.elements.size());
                for (const FieldValue &element : av.elements) {
                    if (const auto *u = std::get_if<quint64>(&element))
                        sample.frames.append(int(*u));
                }
            }
            data.samples.append(std::move(sample));
            break;
        }
        case LostEvent:
            data.lostSamples.append({rec->timestamp, uintField(rec->payload, u"count"_s)});
            break;
        case ThrottleEvent:
            data.throttledTsUs.append(rec->timestamp);
            break;
        default:
            break; // unknown event classes: skip for forward compatibility
        }
    }
    if (stream->readError())
        return ResultError(*stream->readError());

    // Optional, and forgiving: a missing or damaged file costs the shading only.
    QFile pausedFile(dir.pathAppended(pausedRangesFileName).toFSPathString());
    if (pausedFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!pausedFile.atEnd()) {
            const QList<QByteArray> fields = pausedFile.readLine().trimmed().split(' ');
            bool startOk = false;
            bool endOk = false;
            if (fields.size() != 2)
                continue;
            const quint64 start = fields.at(0).toULongLong(&startOk);
            const quint64 end = fields.at(1).toULongLong(&endOk);
            if (startOk && endOk && start <= end)
                data.pausedRangesUs.append({start, end});
        }
    }
    return data;
}

QList<std::pair<quint64, quint64>> pausedRangesUs(
    const std::vector<std::pair<qint64, qint64>> &steadyIntervalsNs, qint64 firstSampleNs,
    quint64 lastSampleUs)
{
    QList<std::pair<quint64, quint64>> ranges;
    for (const auto &[start, end] : steadyIntervalsNs) {
        if (end < 0 || start <= firstSampleNs)
            continue;
        const quint64 startUs = quint64(start - firstSampleNs) / 1000;
        const quint64 endUs = std::min(quint64(end - firstSampleNs) / 1000, lastSampleUs);
        if (startUs < endUs)
            ranges.append({startUs, endUs});
    }
    return ranges;
}

bool isSamplerTrace(const FilePath &dir)
{
    QFile metaFile(dir.pathAppended(u"metadata"_s).toFSPathString());
    if (!metaFile.open(QIODevice::ReadOnly))
        return false;
    const auto reader = TraceReader::open(&metaFile);
    if (!reader)
        return false;
    return std::any_of(reader->schema().dataStreamClasses.cbegin(),
                       reader->schema().dataStreamClasses.cend(),
                       [](const DataStreamClass &cls) { return cls.name == samplerStreamName; });
}

static FilePath uniqueTracePathUnder(const FilePath &parent, const QDateTime &now,
                                     QLatin1StringView prefix, QLatin1StringView suffix)
{
    // No colons or spaces: the name has to survive as a path component on every
    // host, and it ends up on command lines and in log messages.
    const QString stamp = now.toString(u"yyyy-MM-dd-hh-mm-ss"_s);

    FilePath path = parent / u"%1-%2%3"_s.arg(prefix, stamp, suffix);
    for (int counter = 2; path.exists(); ++counter)
        path = parent / u"%1-%2-%3%4"_s.arg(prefix, stamp).arg(counter).arg(suffix);
    return path;
}

FilePath uniqueTracePathAt(const QDateTime &now, QLatin1StringView prefix,
                           QLatin1StringView suffix)
{
    const FilePath tempDir = FilePath::fromString(
        QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    return uniqueTracePathUnder(tempDir, now, prefix, suffix);
}

FilePath uniqueTracePath(QLatin1StringView prefix, QLatin1StringView suffix)
{
    return uniqueTracePathAt(QDateTime::currentDateTime(), prefix, suffix);
}

FilePath uniqueTracePathIn(const FilePath &parent, QLatin1StringView prefix,
                           QLatin1StringView suffix)
{
    return uniqueTracePathUnder(parent, QDateTime::currentDateTime(), prefix, suffix);
}

} // namespace Profiler::Internal
