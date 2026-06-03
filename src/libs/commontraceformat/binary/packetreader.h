// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include "bitbuffer.h"
#include "fieldvalue.h"

#include "../schema/datastreamclass.h"
#include "../schema/fieldlocation.h"
#include "../schema/traceclass.h"

#include <utils/result.h>

#include <optional>

class QIODevice;

namespace CommonTraceFormat {

struct Schema;

struct EventRecord
{
    const EventRecordClass *eventClass = nullptr;
    quint64 eventClassId = 0;
    StructureValue header;
    StructureValue commonContext;
    StructureValue specificContext;
    StructureValue payload;
    quint64 timestamp = 0;
};

// Reads one CTF2 packet at a time from a QIODevice.
// Call openPacket() to load the next packet, then nextEvent() repeatedly.
class CMNTRCEFMT_EXPORT PacketReader
{
public:
    // `dsc` is the default data stream class. If `schema` is given and a packet
    // header carries a `data-stream-class-id` role, the class is re-selected per
    // packet from that id (spec 6.1).
    PacketReader(
        QIODevice *dev,
        const DataStreamClass &dsc,
        const TraceClass *tc = nullptr,
        const Schema *schema = nullptr);

    // Bytes already read from the stream that belong to this packet (e.g. left
    // over from a previous reader's over-read of a sequential device). They are
    // consumed before reading the device. Must be set before openPacket().
    void setPrefix(QByteArray bytes) { m_prefix = std::move(bytes); }

    // Bytes read past the end of the packet just opened. On a non-seekable
    // (sequential) device these cannot be pushed back, so the caller must feed
    // them to the next reader via setPrefix(). Empty for seekable devices.
    QByteArray takeLeftover() { return std::exchange(m_leftover, {}); }

    // Load the next packet from the device. Returns error on EOF or read failure.
    Utils::Result<> openPacket();

    // Read the next event record. Returns error if packet is exhausted.
    Utils::Result<EventRecord> nextEvent();

    bool atEndOfPacket() const;
    bool atEnd() const;

    // Snapshot of the discarded-event-record counter of the stream at the end of
    // the current packet (spec 6.1 DISC_ER_SNAP), if the packet context carries
    // the role. std::nullopt when absent.
    std::optional<quint64> discardedEventRecordSnapshot() const { return m_discErSnap; }
    // Sequence number of the current packet (spec 6.1 PKT_SEQ_NUM), if present.
    std::optional<quint64> packetSequenceNumber() const { return m_pktSeqNum; }
    // Fully-extended end-of-packet default clock value (spec 6.1
    // PKT_END_DEF_CLK_VAL), if the packet context carries the role.
    std::optional<quint64> packetEndDefaultClockValue() const { return m_pktEndDefClkVal; }
    // ID of the current data stream within its class (spec 6.1 DS_ID), set from
    // the packet header's data-stream-id role. std::nullopt when absent.
    std::optional<quint64> dataStreamId() const { return m_dsId; }

private:
    // Resolver for cross-root field locations (variant/optional selectors,
    // dynamic-length lengths) that reference an already-decoded root structure.
    FieldLocationResolver makeResolver();
    std::optional<quint64> resolveFromRoots(const FieldLocation &loc) const;

    // Re-select m_dsc from a decoded packet header's data-stream-class-id role.
    // An id matching no data stream class is reported as an error (spec 6.1).
    Utils::Result<> selectDsc(const StructureValue &hdr, const QHash<QString, int> &clockBits);

    QIODevice *m_dev;
    const DataStreamClass *m_dsc; // current class; may change per packet (spec 6.1)
    const TraceClass *m_tc;
    const Schema *m_schema;

    QByteArray m_packetData;
    QByteArray m_prefix;            // pre-read bytes to consume first
    QByteArray m_leftover;          // over-read bytes for the next packet
    std::optional<BitBuffer> m_buf; // valid after openPacket()
    qint64 m_contentEndBit = 0;     // bit offset where content area ends

    // Default clock value (DEF_CLK_VAL), carried across events within a packet;
    // reset to 0 at each openPacket() per the packet decoding state (spec 6.1).
    quint64 m_defClkVal = 0;

    // Timestamp of the previous event record in this stream, for the monotonicity
    // check (spec 4.2.2: a timestamp MUST be >= the previous one). std::nullopt
    // until the first timestamped event. Persists across packets of the stream.
    std::optional<quint64> m_prevEventTimestamp;

    // Per-packet snapshots from the packet context (spec 6.1), reset on each
    // openPacket(). std::nullopt when the corresponding role is absent.
    std::optional<quint64> m_discErSnap;
    std::optional<quint64> m_pktSeqNum;
    std::optional<quint64> m_dsId;
    std::optional<quint64> m_pktEndDefClkVal;

    // Decoded root structures available to the field-location resolver. Packet
    // roots persist for the packet; event roots are refreshed per event record.
    StructureValue m_pktHeader;
    StructureValue m_pktContext;
    StructureValue m_evHeader;
    StructureValue m_evCommonContext;
    StructureValue m_evSpecificContext;
};

} // namespace CommonTraceFormat
