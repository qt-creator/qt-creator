// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "datastreamreader.h"

#include "metadata/metadatareader.h"

#include <QIODevice>

#include <utility>

namespace CommonTraceFormat {
using namespace Qt::StringLiterals;

DataStreamReader::DataStreamReader(
    QIODevice *dev, const DataStreamClass &dsc, const TraceClass *tc, const Schema *schema)
    : m_dev(dev)
    , m_dsc(dsc)
    , m_tc(tc)
    , m_schema(schema)
{}

Utils::Result<> DataStreamReader::openNextPacket()
{
    if (m_pendingBytes.isEmpty() && m_dev->atEnd()) {
        m_atEnd = true;
        return Utils::ResultError(u"End of stream"_s);
    }
    m_reader.emplace(m_dev, m_dsc, m_tc, m_schema);
    m_reader->setPrefix(std::exchange(m_pendingBytes, {}));
    auto r = m_reader->openPacket();
    if (!r) {
        // A packet that fails to open (truncated/corrupt, e.g. a crash dump)
        // ends the stream gracefully, but record why so readError() can report
        // that the tail was lost rather than a clean end of stream.
        m_atEnd = true;
        m_readError = r.error();
        return r;
    }
    // Pick up any bytes the reader over-read into the next packet (sequential
    // devices only) so they aren't lost when the reader is replaced.
    m_pendingBytes = m_reader->takeLeftover();

    // Spec 4.2.1: consecutive packets of a stream carry consecutive sequence
    // numbers, so a gap reveals missing (e.g. overwritten) packets. This is not
    // an error — record the count so a consumer can report it.
    if (const auto seq = m_reader->packetSequenceNumber()) {
        if (m_lastPktSeqNum && *seq > *m_lastPktSeqNum + 1)
            m_lostPacketCount += *seq - *m_lastPktSeqNum - 1;
        m_lastPktSeqNum = seq;
    }
    return Utils::ResultOk;
}

Utils::Result<EventRecord> DataStreamReader::nextEvent()
{
    if (m_atEnd)
        return Utils::ResultError(u"End of stream"_s);

    for (;;) {
        // Open the first packet lazily and skip over any already-exhausted one
        // (including a fresh packet whose content is empty).
        if (!m_reader || m_reader->atEndOfPacket()) {
            if (auto r = openNextPacket(); !r)
                return Utils::ResultError(r.error());
            continue;
        }

        auto result = m_reader->nextEvent();
        if (result) {
            // Spec 4.2.2: an event record's default-clock timestamp MUST be >=
            // that of the previous record in the same data stream. (With no
            // default clock every timestamp is 0 and this trivially holds.)
            if (m_lastTimestamp && result->timestamp < *m_lastTimestamp) {
                m_readError =
                    u"Event record timestamp %1 is less than the previous timestamp %2 (spec 4.2.2)"_s
                        .arg(result->timestamp)
                        .arg(*m_lastTimestamp);
                return Utils::ResultError(*m_readError);
            }
            m_lastTimestamp = result->timestamp;
            return result;
        }

        // A read failed within the packet content. An overrun (a "BitBuffer: …"
        // error) means the writer left sub-event bits at the tail that can't form
        // a complete event — tolerate it silently, like a truncated crash dump.
        // Any other failure is a structurally-complete but invalid event (e.g. a
        // bad event-class id): record it as a read error.
        const bool exhaustion = result.error().startsWith(u"BitBuffer: "_s);
        if (!exhaustion)
            m_readError = result.error();

        // With content still remaining the failure is surfaced to the caller; a
        // failure that consumed up to the packet boundary is tolerated by moving
        // on to the next packet (a clean EOF if there is none).
        if (!m_reader->atEndOfPacket())
            return result;
    }
}

bool DataStreamReader::atEnd() const
{
    if (m_atEnd)
        return true;
    // Bytes over-read into the next packet mean there is still data to decode,
    // even once the underlying device is drained.
    if (!m_pendingBytes.isEmpty())
        return false;
    return m_reader && m_reader->atEnd();
}

} // namespace CommonTraceFormat
