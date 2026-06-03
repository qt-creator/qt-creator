// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include "../binary/packetreader.h"
#include "../schema/datastreamclass.h"
#include "../schema/traceclass.h"

#include <utils/result.h>

#include <QString>

#include <optional>

class QIODevice;

namespace CommonTraceFormat {

struct Schema;

// Iterates all event records across all packets of one CTF2 data stream.
class CMNTRCEFMT_EXPORT DataStreamReader
{
public:
    // `dsc` is the default class; when `schema` is given, each packet's class is
    // re-selected from its header data-stream-class-id role (spec 6.1).
    DataStreamReader(
        QIODevice *dev,
        const DataStreamClass &dsc,
        const TraceClass *tc = nullptr,
        const Schema *schema = nullptr);

    // Returns the next event, or error. When nextEvent() fails, atEnd() tells a
    // clean end-of-stream apart from a genuine read/parse error.
    Utils::Result<EventRecord> nextEvent();

    bool atEnd() const;

    // Number of packets missing from the stream, inferred from gaps in packet
    // sequence numbers (spec 4.2.1). 0 when no gaps (or no sequence numbers).
    quint64 lostPacketCount() const { return m_lostPacketCount; }

    // The decode error that stopped reading, if any. The reader tolerates a
    // truncated/corrupt packet (e.g. an lttng-crash dump) by ending the stream
    // gracefully — atEnd() still becomes true and every event decoded before the
    // bad data is returned — but records the error here so a caller can tell a
    // clean end of stream (nullopt) from one cut short by corruption.
    const std::optional<QString> &readError() const { return m_readError; }

private:
    Utils::Result<> openNextPacket();

    QIODevice *m_dev;
    const DataStreamClass &m_dsc;
    const TraceClass *m_tc;
    const Schema *m_schema;

    std::optional<PacketReader> m_reader;
    // Bytes over-read past a packet boundary on a sequential device, carried to
    // the next packet's reader (which cannot rewind the stream itself).
    QByteArray m_pendingBytes;
    bool m_atEnd = false;
    // Set to the decode error that ended the stream early (truncated/corrupt
    // packet); empty after a clean end of stream. See readError().
    std::optional<QString> m_readError;
    // Default-clock timestamp of the previous event record, to enforce the
    // monotonicity requirement across the whole data stream (spec 4.2.2).
    std::optional<quint64> m_lastTimestamp;
    // Packet-sequence-number tracking for missing-packet detection (spec 4.2.1).
    std::optional<quint64> m_lastPktSeqNum;
    quint64 m_lostPacketCount = 0;
};

} // namespace CommonTraceFormat
