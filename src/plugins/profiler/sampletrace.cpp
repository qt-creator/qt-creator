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

#include <QFile>

#include <algorithm>

using namespace CommonTraceFormat;
using namespace Profiler;
using namespace Utils;
using namespace Qt::StringLiterals;

namespace QmlProfiler::Internal {

// Event class ids in the sampler stream.
enum SamplerEventClass : quint64 { LabelEvent = 0, ThreadEvent = 1, SampleEvent = 2 };

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

    // label: { uint64 id, string name } — one per distinct frame label.
    {
        EventRecordClass erc;
        erc.id = LabelEvent;
        erc.name = u"label"_s;
        auto payload = std::make_shared<StructureFC>();
        payload->members.append({u"id"_s, u64Field(), {}});
        payload->members.append({u"name"_s, std::make_shared<NullTerminatedStringFC>(), {}});
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

    schema.dataStreamClasses.append(std::move(dsc));
    return schema;
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
        StructureValue payload;
        payload.set(u"id"_s, quint64(i));
        payload.set(u"name"_s, data.labels.at(i));
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

    const int total = std::max<int>(1, int(data.samples.size()));
    int written = 0;
    for (const SampleTraceData::ThreadSample &sample : data.samples) {
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

    if (auto r = tw.close(); !r)
        return ResultError(r.error());
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

Result<SampleTraceData> readSampleTrace(const FilePath &dir)
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

    SampleTraceData data;
    while (true) {
        auto rec = stream->nextEvent();
        if (!rec) {
            if (stream->atEnd())
                break;
            return ResultError(rec.error());
        }
        data.pid = uintField(rec->commonContext, u"pid"_s);
        switch (rec->eventClassId) {
        case LabelEvent: {
            const int id = int(uintField(rec->payload, u"id"_s));
            if (data.labels.size() <= id)
                data.labels.resize(id + 1);
            data.labels[id] = stringField(rec->payload, u"name"_s);
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
        default:
            break; // unknown event classes: skip for forward compatibility
        }
    }
    if (stream->readError())
        return ResultError(*stream->readError());
    return data;
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

} // namespace QmlProfiler::Internal
