// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "perfdataparser.h"
#include "perfregisters.h"
#include "perfsymbolizer.h"
#include "perftracepointformats.h"

#include <QByteArray>
#include <QDataStream>
#include <QList>
#include <QHash>

#include <atomic>
#include <functional>

namespace Profiler::Internal {

// Turns a perf recording into the stream the CPU Usage analyzer reads (see
// PerfProfilerTraceFile::readMessages()): definitions of the strings,
// attributes, locations, symbols, tracepoints and threads the events refer
// to, each before its first use, and the events themselves -- samples, thread
// starts and ends, context switches and losses -- in time order.
//
// A location is an address as sampled, with its source line. Its parent is
// the location of the function the address is in, which carries the
// function's symbol, so that the analyzer can aggregate by function.
class PROFILER_EXPORT PerfTraceConverter final : public PerfData::PerfDataHandler
{
public:
    // Decodes with `parser`'s attrs; `write` receives the stream, in chunks.
    PerfTraceConverter(const PerfData::PerfDataParser &parser,
                       const std::function<void(const QByteArray &)> &write);

    PerfSymbolizer &symbolizer() { return m_symbolizer; }
    // The architecture the recording was taken on, for unwinding and for
    // telling whether this machine's kernel symbols are the recording's:
    // unless set, what the recording itself says, or else this machine's.
    void setArchitecture(PerfArchitecture arch) { m_architecture = arch; }
    // Kernel addresses never resolve against this machine's symbols, as for
    // a recording taken on another machine.
    // Nor do JIT symbol maps, which are where the recorded process left them.
    // Nor does this machine's CPU tell how return addresses are signed.
    void setForeignRecording(bool foreign)
    {
        m_foreignRecording = foreign;
        m_symbolizer.setUseJitMaps(!foreign);
        m_symbolizer.setUseHostPointerAuthentication(!foreign);
    }
    // Once set, the conversion stops with an error at the next sample.
    void setCancelFlag(const std::atomic_bool *canceled) { m_canceled = canceled; }
    // Called, on the converting thread, with what the user should know about
    // a conversion that goes on regardless.
    void setWarningHandler(const std::function<void(const QString &)> &handler)
    {
        m_onWarning = handler;
    }
    // Whether user stacks are unwound; what the build can, unless set.
    void setCanUnwind(bool canUnwind) { m_canUnwind = canUnwind; }

    Utils::Result<> attrAdded(int index) override;
    void mmap(const PerfData::Mmap &mmap) override;
    void comm(const PerfData::Comm &comm) override;
    void task(const PerfData::Task &task) override;
    void contextSwitch(const PerfData::ContextSwitch &contextSwitch) override;
    void lost(const PerfData::Lost &lost) override;
    Utils::Result<> sample(const PerfData::Sample &sample) override;
    void finishedRound() override;
    void tracingData(const QByteArray &data) override;
    void buildId(const PerfData::BuildId &buildId) override;

    // Writes out what is still held back. Call once the recording has ended.
    void finish();

private:
    struct Event
    {
        quint64 time = 0;
        QByteArray message;
    };

    // A definition: `fill` streams its payload, which goes out at once.
    void writeMessage(const std::function<void(QDataStream &)> &fill);
    // An event: held back until no earlier one can come any more, then written
    // in time order.
    void writeEvent(quint8 feature, quint32 pid, quint32 tid, quint64 time, quint32 cpu,
                    const std::function<void(QDataStream &)> &fill = {});
    void flush();
    // Writes the events up to `until`, in time order.
    void flushEvents(quint64 until);

    qint32 stringId(const QByteArray &string);
    qint32 attributeId(int attrIndex);
    qint32 locationId(quint32 pid, quint64 addr, bool isKernel);
    qint32 defineLocation(quint64 address, quint32 pid, const QString &file, int line,
                          qint32 parentLocationId, quint64 relAddr);
    // `path` is the binary's as recorded, `actualPath` where it is here.
    void defineSymbol(qint32 locationId, const QString &name, const QString &binary,
                      const QString &path, const QString &actualPath, bool isKernel,
                      quint64 relAddr, quint64 size);
    PerfArchitecture architecture();

    const PerfData::PerfDataParser &m_parser;
    const std::function<void(const QByteArray &)> m_write;
    PerfSymbolizer m_symbolizer;
    PerfTracepointFormats m_tracepointFormats;

    QByteArray m_output;
    QList<Event> m_round;
    quint64 m_roundEnd = 0;         // the latest event since the last round ended
    quint64 m_previousRoundEnd = 0; // and before that

    QHash<QByteArray, qint32> m_stringIds;
    QHash<int, qint32> m_attributeIds;
    qint32 m_nextLocationId = 0;
    struct LocationKey
    {
        quint32 pid;
        quint64 addr;
        bool isKernel;
        friend bool operator==(const LocationKey &, const LocationKey &) = default;
        friend size_t qHash(const LocationKey &k, size_t seed = 0)
        {
            return qHashMulti(seed, k.pid, k.addr, k.isKernel);
        }
    };
    QHash<LocationKey, qint32> m_locationIds;
    // The location carrying a function's symbol, by pid, module, and the
    // function's link-time start.
    struct FunctionKey
    {
        quint32 pid;
        QString module;
        quint64 start;
        friend bool operator==(const FunctionKey &, const FunctionKey &) = default;
        friend size_t qHash(const FunctionKey &k, size_t seed = 0)
        {
            return qHashMulti(seed, k.pid, k.module, k.start);
        }
    };
    QHash<FunctionKey, qint32> m_functionLocationIds;
    // The location carrying an inlined function's symbol, by the location of
    // what it was inlined into, and the function and call site.
    struct InlineKey
    {
        qint32 parent;
        QString function;
        QString callFile;
        int callLine;
        friend bool operator==(const InlineKey &, const InlineKey &) = default;
        friend size_t qHash(const InlineKey &k, size_t seed = 0)
        {
            return qHashMulti(seed, k.parent, k.function, k.callFile, k.callLine);
        }
    };
    QHash<InlineKey, qint32> m_inlineLocationIds;
    quint64 m_latestTime = 0;
    const std::atomic_bool *m_canceled = nullptr;
    std::function<void(const QString &)> m_onWarning;
    bool m_canUnwind = PerfSymbolizer::canUnwind();
    bool m_warnedCannotUnwind = false;
    PerfArchitecture m_architecture = PerfArchitecture::Unknown;
    bool m_foreignRecording = false;
    bool m_kernelSymbolsDecided = false;
};

} // namespace Profiler::Internal
