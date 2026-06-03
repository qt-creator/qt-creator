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

#include <QUuid>

#include <memory>
#include <optional>

class QIODevice;
class QBuffer;

namespace CommonTraceFormat {

// Location of a back-patchable fixed-length unsigned-integer role field within
// a root structure (packet context), used to fill length/clock roles whose
// values are only known once the packet is finalized.
struct RoleFieldPos
{
    qint64 bitOffset = -1;
    int length = 0;
    ByteOrder byteOrder = ByteOrder::LittleEndian;
    BitOrder bitOrder = BitOrder::LeastSignificantFirst;
    bool found() const { return bitOffset >= 0; }
};

// Writes a single CTF2 packet to a QIODevice.
// Accumulates event records in an internal QBuffer scratch space, then flushes
// atomically on finalize(). Back-patches total-length and content-length roles.
class CMNTRCEFMT_EXPORT PacketWriter
{
public:
    PacketWriter(
        QIODevice *dev,
        const DataStreamClass &dsc,
        const TraceClass *tc = nullptr,
        std::optional<QUuid> uuid = {});
    ~PacketWriter();

    // Write packet header (magic, UUID, etc.) and context.
    // Must be called before the first writeEventRecord().
    void beginPacket(const StructureValue &packetContext = {});

    // Write one event record into the packet.
    void writeEventRecord(
        quint64 eventClassId,
        const StructureValue &payload = {},
        const StructureValue &specificCtx = {},
        const StructureValue &commonCtx = {},
        quint64 timestamp = 0);

    // Finalize and flush the packet to the underlying QIODevice.
    // After this, the PacketWriter may be reused by calling beginPacket() again.
    Utils::Result<> finalize();

    qint64 contentBits() const { return m_contentBitOffset; }

private:
    FieldLocationResolver makeResolver(const StructureValue &packetCtx) const;

    QIODevice *m_dev;
    const DataStreamClass &m_dsc;
    const TraceClass *m_tc;
    std::optional<QUuid> m_uuid; // preamble UUID, for the packet-header blob

    QByteArray m_scratch;             // Accumulation buffer (packet bytes)
    std::unique_ptr<QBuffer> m_qbuf;  // QIODevice view over m_scratch
    std::unique_ptr<BitBuffer> m_buf; // Wraps m_scratch via m_qbuf

    RoleFieldPos m_totalLength;   // packet-total-length role field
    RoleFieldPos m_contentLength; // packet-content-length role field
    qint64 m_contentBitOffset = 0;

    // Packet-context default-clock begin/end role fields, back-patched in
    // finalize() from the first/last event timestamps seen in the packet.
    RoleFieldPos m_beginClock;
    RoleFieldPos m_endClock;
    std::optional<quint64> m_firstTimestamp;
    std::optional<quint64> m_lastTimestamp;

    bool m_inPacket = false;

    StructureValue m_packetCtxValue; // stored for resolver
};

} // namespace CommonTraceFormat
