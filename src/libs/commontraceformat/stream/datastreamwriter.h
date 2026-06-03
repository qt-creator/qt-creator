// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include "../binary/packetwriter.h"
#include "../schema/datastreamclass.h"
#include "../schema/traceclass.h"

#include <utils/result.h>

#include <memory>

class QIODevice;

namespace CommonTraceFormat {

// Writes events to one CTF2 data stream. Manages packet boundaries.
class CMNTRCEFMT_EXPORT DataStreamWriter
{
public:
    DataStreamWriter(
        QIODevice *dev,
        const DataStreamClass &dsc,
        const TraceClass *tc = nullptr,
        qint64 packetSizeLimitBytes = 65536,
        std::optional<QUuid> uuid = {});

    // Write one event record. Opens a new packet if needed. Returns an error
    // only if rolling to a new packet fails to flush the previous one.
    Utils::Result<> writeEvent(
        quint64 eventClassId,
        const StructureValue &payload = {},
        const StructureValue &specificCtx = {},
        const StructureValue &commonCtx = {},
        quint64 timestamp = 0);

    // Flush and close the current packet.
    Utils::Result<> flush();

private:
    void ensurePacketOpen();

    QIODevice *m_dev;
    const DataStreamClass &m_dsc;
    const TraceClass *m_tc;
    qint64 m_packetSizeLimitBytes;
    std::optional<QUuid> m_uuid;

    // Smallest partial default-clock-timestamp width in the event header, used to
    // decide when a packet must be rolled to keep timestamps re-extendable.
    int m_clockTimestampBits = 64;
    quint64 m_lastTimestamp = 0;
    bool m_haveLastTimestamp = false;
    // Set when the packet-total-length field is too narrow to batch events.
    bool m_oneEventPerPacket = false;
    bool m_packetHasEvent = false;

    std::unique_ptr<PacketWriter> m_packet;
    bool m_packetOpen = false;
};

} // namespace CommonTraceFormat
