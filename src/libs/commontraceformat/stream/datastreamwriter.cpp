// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "datastreamwriter.h"

#include "../schema/compoundfieldclasses.h"
#include "../schema/scalarfieldclasses.h"

#include <QIODevice>

namespace CommonTraceFormat {
using namespace Qt::StringLiterals;

// Smallest default-clock-timestamp field width (bits) anywhere in an event
// header field class — top-level, or nested in a variant option (the CTF 1.8
// compact header). 64 if none is narrower, meaning timestamps are stored in
// full and never need re-extension.
static int minClockTimestampBits(const FieldClass *fc)
{
    if (!fc)
        return 64;
    switch (fc->kind) {
    case FieldClassKind::FixedLengthUInt: {
        const auto &u = static_cast<const FixedLengthUIntFC &>(*fc);
        return u.roles.contains(UIntRole::DefaultClockTimestamp) ? u.length : 64;
    }
    case FieldClassKind::Structure: {
        int best = 64;
        for (const auto &m : static_cast<const StructureFC &>(*fc).members)
            best = std::min(best, minClockTimestampBits(m.fieldClass.get()));
        return best;
    }
    case FieldClassKind::Variant: {
        int best = 64;
        for (const auto &opt : static_cast<const VariantFC &>(*fc).options)
            best = std::min(best, minClockTimestampBits(opt.fieldClass.get()));
        return best;
    }
    default:
        return 64;
    }
}

// Width (bits) of the top-level packet-total-length role field, or 64 if there
// is none. The role value counts bits, so the field can express a packet of at
// most (2^width - 1) bits — narrow fields force small packets.
static int packetTotalLengthBits(const FieldClass *fc)
{
    if (!fc || fc->kind != FieldClassKind::Structure)
        return 64;
    for (const auto &m : static_cast<const StructureFC &>(*fc).members) {
        const FieldClass *mfc = m.fieldClass.get();
        if (mfc && mfc->kind == FieldClassKind::FixedLengthUInt) {
            const auto &u = static_cast<const FixedLengthUIntFC &>(*mfc);
            if (u.roles.contains(UIntRole::PacketTotalLength))
                return u.length;
        }
    }
    return 64;
}

DataStreamWriter::DataStreamWriter(
    QIODevice *dev,
    const DataStreamClass &dsc,
    const TraceClass *tc,
    qint64 packetSizeLimitBytes,
    std::optional<QUuid> uuid)
    : m_dev(dev)
    , m_dsc(dsc)
    , m_tc(tc)
    , m_packetSizeLimitBytes(packetSizeLimitBytes)
    , m_uuid(uuid)
{
    m_clockTimestampBits = minClockTimestampBits(dsc.eventRecordHeaderFieldClass.get());

    // If the packet-total-length field is too narrow to safely batch events (its
    // max-expressible packet is smaller than the byte budget), fall back to one
    // event per packet — always representable, since each event fit in some
    // packet in the source trace.
    const int lenBits = packetTotalLengthBits(dsc.packetContextFieldClass.get());
    if (lenBits < 32) {
        const qint64 maxBytes = ((qint64(1) << lenBits) - 1) / 8;
        m_oneEventPerPacket = maxBytes < m_packetSizeLimitBytes;
    }

    m_packet = std::make_unique<PacketWriter>(dev, dsc, tc, m_uuid);
}

void DataStreamWriter::ensurePacketOpen()
{
    if (!m_packetOpen) {
        m_packet->beginPacket();
        m_packetOpen = true;
    }
}

Utils::Result<> DataStreamWriter::writeEvent(
    quint64 eventClassId,
    const StructureValue &payload,
    const StructureValue &specificCtx,
    const StructureValue &commonCtx,
    quint64 timestamp)
{
    // Timestamps must be monotonic: a default-clock timestamp counts cycles since
    // the clock origin, so a value below the previous one is unrepresentable and
    // would be misread as a forward wraparound (spec 6.3). Reject rather than
    // emit data the reader cannot recover.
    if (m_haveLastTimestamp && timestamp < m_lastTimestamp)
        return Utils::ResultError(
            u"DataStreamWriter: event timestamp %1 is earlier than the previous timestamp %2"_s
                .arg(timestamp)
                .arg(m_lastTimestamp));

    ensurePacketOpen();

    // Roll to a new packet when the byte budget is exhausted, or when a partial
    // per-event timestamp can no longer be re-extended within the current
    // packet: if the gap since the previous event reaches the field's window,
    // the reader's wraparound (spec 6.3) would under-count. A new packet
    // re-seeds the 64-bit begin clock from this event, keeping it exact.
    bool rollForClock = false;
    if (m_haveLastTimestamp && m_clockTimestampBits < 64) {
        // timestamp >= m_lastTimestamp is guaranteed by the monotonicity check above.
        const quint64 window = quint64(1) << m_clockTimestampBits;
        if ((timestamp - m_lastTimestamp) >= window)
            rollForClock = true;
    }
    const bool rollForLengthField = m_oneEventPerPacket && m_packetHasEvent;
    if (m_packet->contentBits() / 8 >= m_packetSizeLimitBytes || rollForClock
        || rollForLengthField) {
        if (auto r = m_packet->finalize(); !r)
            return r;
        m_packet = std::make_unique<PacketWriter>(m_dev, m_dsc, m_tc, m_uuid);
        m_packet->beginPacket();
        m_packetHasEvent = false;
    }

    m_packet->writeEventRecord(eventClassId, payload, specificCtx, commonCtx, timestamp);
    m_packetHasEvent = true;
    m_lastTimestamp = timestamp;
    m_haveLastTimestamp = true;
    return Utils::ResultOk;
}

Utils::Result<> DataStreamWriter::flush()
{
    if (!m_packetOpen)
        return Utils::ResultOk;
    Utils::Result<> result = m_packet->finalize();
    m_packetOpen = false;
    return result;
}

} // namespace CommonTraceFormat
