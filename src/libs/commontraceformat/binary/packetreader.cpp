// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "packetreader.h"

#include "fieldreader.h"

#include "../metadata/metadatareader.h"
#include "../schema/blobfieldclasses.h"
#include "../schema/compoundfieldclasses.h"
#include "../schema/scalarfieldclasses.h"
#include "../schema/schema.h"

#include <QIODevice>

#include <limits>

namespace CommonTraceFormat {
using namespace Qt::StringLiterals;

PacketReader::PacketReader(
    QIODevice *dev, const DataStreamClass &dsc, const TraceClass *tc, const Schema *schema)
    : m_dev(dev)
    , m_dsc(&dsc)
    , m_tc(tc)
    , m_schema(schema)
{}

namespace {

// A role-bearing unsigned integer field located in a structure value.
struct RoleField
{
    bool found = false;
    quint64 value = 0;
    int lengthBits = 0; // bit length L for the clock value update procedure
};

// Minimal number of LEB128 bytes needed to encode `value`. Used only as a
// fallback when the exact on-wire byte count of a variable-length integer
// wasn't recorded during decoding.
int minLeb128Bytes(quint64 value)
{
    int n = 1;
    while (value >>= 7)
        ++n;
    return n;
}

// Find the first top-level member of `sfc`/`sv` whose unsigned integer field
// class carries `role`, returning its value and clock length L (spec 6.3). For
// variable-length integers, `clockBits` provides the exact on-wire L recorded
// during decoding; without it we fall back to the minimal-encoding estimate.
RoleField findUIntRole(
    const FieldClass *fc,
    const StructureValue &sv,
    UIntRole role,
    const QHash<QString, int> &clockBits = {})
{
    if (!fc || fc->kind != FieldClassKind::Structure)
        return {};
    const auto &sfc = static_cast<const StructureFC &>(*fc);
    for (const auto &member : sfc.members) {
        const FieldClass *mfc = member.fieldClass.get();
        bool has = false;
        int lengthBits = 0;
        if (mfc->kind == FieldClassKind::FixedLengthUInt) {
            const auto &ufc = static_cast<const FixedLengthUIntFC &>(*mfc);
            has = ufc.roles.contains(role);
            lengthBits = ufc.length;
        } else if (mfc->kind == FieldClassKind::VariableLengthUInt) {
            const auto &ufc = static_cast<const VariableLengthUIntFC &>(*mfc);
            has = ufc.roles.contains(role);
        }
        if (!has)
            continue;
        const FieldValue *fv = sv.get(member.name);
        if (fv && std::holds_alternative<quint64>(*fv)) {
            const quint64 v = std::get<quint64>(*fv);
            if (mfc->kind == FieldClassKind::VariableLengthUInt)
                lengthBits = clockBits.value(member.name, minLeb128Bytes(v) * 7);
            return {true, v, lengthBits};
        }
    }
    return {};
}

quint64 extractRole(
    const StructureValue &sv,
    const FieldClass *fc,
    UIntRole role,
    const QHash<QString, int> &clockBits = {})
{
    return findUIntRole(fc, sv, role, clockBits).value;
}

// Find the bytes of the first top-level static-length BLOB member of `sfc`/`sv`
// carrying `role` (spec 5.3.3). Returns nullopt when no such member exists.
std::optional<QByteArray> findBlobRole(const FieldClass *fc, const StructureValue &sv, BlobRole role)
{
    if (!fc || fc->kind != FieldClassKind::Structure)
        return std::nullopt;
    const auto &sfc = static_cast<const StructureFC &>(*fc);
    for (const auto &member : sfc.members) {
        const FieldClass *mfc = member.fieldClass.get();
        if (!mfc || mfc->kind != FieldClassKind::StaticLengthBlob)
            continue;
        if (!static_cast<const StaticLengthBlobFC &>(*mfc).roles.contains(role))
            continue;
        const FieldValue *fv = sv.get(member.name);
        if (fv && std::holds_alternative<QByteArray>(*fv))
            return std::get<QByteArray>(*fv);
    }
    return std::nullopt;
}

// Clock value update procedure (spec 6.3): extend DEF_CLK_VAL using the
// (possibly partial) timestamp value V whose field has bit length L.
void updateClockValue(quint64 &defClkVal, quint64 v, int lengthBits)
{
    if (lengthBits <= 0)
        return;
    if (lengthBits >= 64) {
        defClkVal = v;
        return;
    }
    const quint64 mask = (quint64(1) << lengthBits) - 1;
    const quint64 high = defClkVal & ~mask;
    const quint64 cur = defClkVal & mask;
    defClkVal = (v >= cur) ? (high + v) : (high + mask + 1 + v);
}

// CTF 1.8 compact/extended event-record headers carry the event-class id and
// default-clock timestamp inside a variant option (the timestamp is a 32/27-bit
// *partial* value in the compact option, a full 64-bit value in the extended
// one). findUIntRole only scans top-level members, so it never sees these
// nested fields. Locate the header's variant member and its selected option,
// returning the option's structure field class and decoded value so the caller
// can resolve the nested fields by role. Returns nullopt when the header has no
// variant member or the selected option is not a structure.
std::optional<std::pair<const FieldClass *, StructureValue>> selectedVariantOption(
    const FieldClass *headerFc, const StructureValue &header)
{
    if (!headerFc || headerFc->kind != FieldClassKind::Structure)
        return std::nullopt;
    const auto &sfc = static_cast<const StructureFC &>(*headerFc);
    for (const auto &member : sfc.members) {
        if (!member.fieldClass || member.fieldClass->kind != FieldClassKind::Variant)
            continue;
        const FieldValue *fv = header.get(member.name);
        if (!fv || !std::holds_alternative<std::shared_ptr<VariantValue>>(*fv))
            continue;
        const auto &vvPtr = std::get<std::shared_ptr<VariantValue>>(*fv);
        if (!vvPtr)
            continue;
        const auto &vv = *vvPtr;
        if (!vv.value || !std::holds_alternative<std::shared_ptr<StructureValue>>(*vv.value))
            continue;
        const auto &vfc = static_cast<const VariantFC &>(*member.fieldClass);
        // Prefer the unambiguous option index (option names are optional and may
        // be empty or duplicated, spec 5.3.23.1). Fall back to matching by name
        // only when no index was recorded.
        const auto matchOption = [&](const VariantOption &opt)
            -> std::optional<std::pair<const FieldClass *, StructureValue>> {
            if (opt.fieldClass && opt.fieldClass->kind == FieldClassKind::Structure)
                return std::pair{
                    opt.fieldClass.get(), *std::get<std::shared_ptr<StructureValue>>(*vv.value)};
            return std::nullopt;
        };
        if (vv.selectedIndex >= 0 && vv.selectedIndex < vfc.options.size())
            return matchOption(vfc.options[vv.selectedIndex]);
        for (const auto &opt : vfc.options) {
            if (opt.name == vv.selectedOption)
                return matchOption(opt);
        }
    }
    return std::nullopt;
}

} // namespace

Utils::Result<> PacketReader::openPacket()
{
    if (m_prefix.isEmpty() && m_dev->atEnd())
        return Utils::ResultError(u"No packet: device at end of stream"_s);

    // Consume any bytes pre-read for this packet (e.g. handed over from a
    // previous reader's sequential over-read) before touching the device.
    QByteArray initial = std::exchange(m_prefix, {});
    if (initial.size() < 512)
        initial += m_dev->read(512 - initial.size());
    if (initial.isEmpty())
        return Utils::ResultError(u"Failed to read initial packet bytes"_s);

    BitBuffer probe(initial);
    quint64 totalBytes = 0;

    if (m_tc && m_tc->packetHeaderFieldClass) {
        FieldReader fr(probe, {}, FieldLocation::Origin::PacketHeader);
        auto hdr = fr.read(*m_tc->packetHeaderFieldClass);
        // Pick the data stream class from the header before decoding the context
        // (the context field class belongs to the selected class; spec 6.1).
        // Best-effort during probing (only to pick the right context field class
        // for the total-length read); an unresolved id is reported on the real
        // pass below, not here.
        if (hdr && isStructure(*hdr))
            (void) selectDsc(asStructure(*hdr), fr.rootMemberClockBits());
    }

    StructureValue ctxValue;
    if (m_dsc->packetContextFieldClass) {
        FieldReader fr(probe, {}, FieldLocation::Origin::PacketContext);
        auto ctxResult = fr.read(*m_dsc->packetContextFieldClass);
        if (ctxResult && isStructure(*ctxResult))
            ctxValue = asStructure(*ctxResult);

        quint64 totalBits = extractRole(
            ctxValue,
            m_dsc->packetContextFieldClass.get(),
            UIntRole::PacketTotalLength,
            fr.rootMemberClockBits());
        if (totalBits > 0 && totalBits <= std::numeric_limits<quint64>::max() - 7)
            totalBytes = (totalBits + 7) / 8;
    }

    if (totalBytes > static_cast<quint64>(initial.size())) {
        qint64 remaining = static_cast<qint64>(totalBytes) - initial.size();
        initial += m_dev->read(remaining);
        // A device that returns fewer bytes than the declared packet length (a
        // truncated file, or a short read from a slow stream) must not be
        // processed as though the packet were complete.
        if (static_cast<quint64>(initial.size()) < totalBytes)
            return Utils::ResultError(
                u"Truncated packet: %1 bytes available, %2 declared"_s.arg(initial.size())
                    .arg(totalBytes));
    } else if (totalBytes == 0) {
        initial += m_dev->readAll();
        totalBytes = static_cast<quint64>(initial.size());
    } else {
        // Over-read: trim to the packet boundary. On a seekable device rewind it;
        // on a sequential one (pipe/socket) the excess can't be pushed back, so
        // hand it to the next reader via takeLeftover() instead of dropping it.
        qint64 excess = static_cast<qint64>(initial.size()) - static_cast<qint64>(totalBytes);
        if (excess > 0) {
            if (m_dev->isSequential())
                m_leftover = initial.right(static_cast<qsizetype>(excess));
            else
                m_dev->seek(m_dev->pos() - excess);
            initial.resize(static_cast<qsizetype>(totalBytes));
        }
    }

    m_packetData = std::move(initial);
    m_buf.emplace(m_packetData);
    m_buf->resetLastByteOrder();
    // Bound the content area to this packet's physical buffer until the real
    // content length is resolved at the end of this function. The header/context
    // re-parse below still needs to read within the buffer, so we can't clamp to
    // zero — but this guarantees an openPacket() that aborts early (a malformed
    // header on this pass) leaves a boundary tied to the *current* packet rather
    // than a stale, possibly larger value from a previous one.
    m_contentEndBit = m_packetData.size() * 8LL;
    m_buf->setContentEndBit(m_contentEndBit);
    m_pktHeader = {};
    m_pktContext = {};
    m_discErSnap = std::nullopt;
    m_pktSeqNum = std::nullopt;
    m_dsId = std::nullopt;
    m_pktEndDefClkVal = std::nullopt;
    // Spec 6.1: the packet decoding state initializes DEF_CLK_VAL to 0 for each
    // packet. The packet's beginning timestamp (if any) re-establishes it below.
    m_defClkVal = 0;

    // Re-parse using the full packet buffer to capture roots and roles.
    if (m_tc && m_tc->packetHeaderFieldClass) {
        FieldReader fr(*m_buf, makeResolver(), FieldLocation::Origin::PacketHeader);
        auto hdr = fr.read(*m_tc->packetHeaderFieldClass);
        if (!hdr)
            return Utils::ResultError(hdr.error());
        if (isStructure(*hdr))
            m_pktHeader = asStructure(*hdr);
        // Select the data stream class from the header (spec 6.1). An id that
        // matches no data stream class is an error that aborts decoding.
        if (auto r = selectDsc(m_pktHeader, fr.rootMemberClockBits()); !r)
            return Utils::ResultError(r.error());
        // Validate the packet magic number, if present (spec 6.1).
        const RoleField magic = findUIntRole(
            m_tc->packetHeaderFieldClass.get(),
            m_pktHeader,
            UIntRole::PacketMagicNumber,
            fr.rootMemberClockBits());
        if (magic.found && magic.value != 0xc1fc1fc1ULL)
            return Utils::ResultError(
                u"Invalid packet magic number: 0x%1"_s.arg(magic.value, 0, 16));

        // Capture the data stream ID, if the header carries the role (spec 6.1
        // DS_ID): the ID of this stream within its class.
        const RoleField dsId = findUIntRole(
            m_tc->packetHeaderFieldClass.get(),
            m_pktHeader,
            UIntRole::DataStreamId,
            fr.rootMemberClockBits());
        if (dsId.found)
            m_dsId = dsId.value;

        // Validate the metadata-stream-uuid BLOB against the preamble UUID, if
        // both the role field and a preamble UUID are present (spec 6.1).
        if (m_schema && m_schema->uuid && !m_schema->uuid->isNull()) {
            const auto uuidBytes = findBlobRole(
                m_tc->packetHeaderFieldClass.get(), m_pktHeader, BlobRole::MetadataStreamUuid);
            if (uuidBytes && *uuidBytes != m_schema->uuid->toRfc4122())
                return Utils::ResultError(
                    u"Packet metadata-stream-uuid does not match the preamble UUID"_s);
        }
    }

    // Packet lengths are ∞ (std::nullopt) unless the context provides them, so a
    // genuine length of 0 is distinguishable from "absent" (spec 6.1).
    std::optional<quint64> totalBits, contentBits;
    if (m_dsc->packetContextFieldClass) {
        FieldReader fr(*m_buf, makeResolver(), FieldLocation::Origin::PacketContext);
        auto ctxResult = fr.read(*m_dsc->packetContextFieldClass);
        if (!ctxResult)
            return Utils::ResultError(ctxResult.error());
        if (isStructure(*ctxResult))
            m_pktContext = asStructure(*ctxResult);

        const FieldClass *cfc = m_dsc->packetContextFieldClass.get();
        const QHash<QString, int> &clockBits = fr.rootMemberClockBits();

        const RoleField totalRole
            = findUIntRole(cfc, m_pktContext, UIntRole::PacketTotalLength, clockBits);
        const RoleField contentRole
            = findUIntRole(cfc, m_pktContext, UIntRole::PacketContentLength, clockBits);
        if (totalRole.found)
            totalBits = totalRole.value;
        if (contentRole.found)
            contentBits = contentRole.value;

        // Initialize DEF_CLK_VAL from the packet beginning timestamp, if any.
        const RoleField begin
            = findUIntRole(cfc, m_pktContext, UIntRole::DefaultClockTimestamp, clockBits);
        if (begin.found)
            updateClockValue(m_defClkVal, begin.value, begin.lengthBits);

        // Packet end timestamp: the field holds a (possibly partial) value that
        // must be extended via the clock value update procedure (spec 6.3),
        // seeded from the begin timestamp, before comparing. Report an error if
        // the packet begins after it ends (spec 6.1).
        const RoleField end
            = findUIntRole(cfc, m_pktContext, UIntRole::PacketEndDefaultClockTimestamp, clockBits);
        if (end.found) {
            quint64 endVal = m_defClkVal;
            updateClockValue(endVal, end.value, end.lengthBits);
            m_pktEndDefClkVal = endVal;
            if (begin.found && m_defClkVal > endVal)
                return Utils::ResultError(
                    u"Packet beginning timestamp is greater than its end timestamp"_s);
        }

        // Capture the discarded-event-record-counter snapshot and sequence
        // number, if present (spec 6.1 DISC_ER_SNAP / PKT_SEQ_NUM).
        const RoleField disc = findUIntRole(
            cfc, m_pktContext, UIntRole::DiscardedEventRecordCounterSnapshot, clockBits);
        if (disc.found)
            m_discErSnap = disc.value;
        const RoleField seq
            = findUIntRole(cfc, m_pktContext, UIntRole::PacketSequenceNumber, clockBits);
        if (seq.found)
            m_pktSeqNum = seq.value;
    }

    // Reconcile total/content lengths (spec 6.1): each defaults to the other
    // when only one is given; content must not exceed total.
    if (!totalBits && contentBits)
        totalBits = contentBits;
    if (!contentBits && totalBits)
        contentBits = totalBits;
    if (totalBits && contentBits && *contentBits > *totalBits)
        return Utils::ResultError(u"Packet content length exceeds total length"_s);

    // Clamp the content boundary to the physical buffer. Comparing in the
    // unsigned domain first avoids casting a content length above INT64_MAX
    // (which would overflow to a negative qint64 and corrupt the boundary).
    const qint64 bufBits = m_packetData.size() * 8LL;
    m_contentEndBit = (contentBits && *contentBits <= static_cast<quint64>(bufBits))
                          ? static_cast<qint64>(*contentBits)
                          : bufBits;
    m_buf->setContentEndBit(m_contentEndBit);

    return Utils::ResultOk;
}

Utils::Result<EventRecord> PacketReader::nextEvent()
{
    if (!m_buf)
        return Utils::ResultError(u"No packet open"_s);
    if (m_buf->readBitOffset() >= m_contentEndBit)
        return Utils::ResultError(u"End of packet content area"_s);

    qint64 startBitOffset = m_buf->readBitOffset();

    EventRecord rec;
    m_evHeader = {};
    m_evCommonContext = {};
    m_evSpecificContext = {};

    if (m_dsc->eventRecordHeaderFieldClass) {
        FieldReader fr(*m_buf, makeResolver(), FieldLocation::Origin::EventRecordHeader);
        auto r = fr.read(*m_dsc->eventRecordHeaderFieldClass);
        if (!r)
            return Utils::ResultError(r.error());
        if (isStructure(*r))
            rec.header = asStructure(*r);
        m_evHeader = rec.header;

        const FieldClass *hfc = m_dsc->eventRecordHeaderFieldClass.get();
        const QHash<QString, int> &clockBits = fr.rootMemberClockBits();

        // Event class id from the event-record-class-id role.
        const RoleField idRole
            = findUIntRole(hfc, rec.header, UIntRole::EventRecordClassId, clockBits);
        if (idRole.found)
            rec.eventClassId = idRole.value;

        // Default clock timestamp: update DEF_CLK_VAL per the clock value update
        // procedure, handling partial timestamps and wraparound (spec 6.3).
        const RoleField tsRole
            = findUIntRole(hfc, rec.header, UIntRole::DefaultClockTimestamp, clockBits);
        if (tsRole.found)
            updateClockValue(m_defClkVal, tsRole.value, tsRole.lengthBits);
        rec.timestamp = m_defClkVal;

        // CTF 1.8 compact/extended event header: the event-record-class id and
        // default-clock timestamp live inside a variant option. Locate the
        // variant member and read the nested fields by role (spec roles, set by
        // the metadata reader / TSDL parser) rather than by name.
        if (auto opt = selectedVariantOption(hfc, rec.header)) {
            const auto &[optFc, optSv] = *opt;
            const RoleField extId = findUIntRole(optFc, optSv, UIntRole::EventRecordClassId);
            if (extId.found)
                rec.eventClassId = extId.value;
            const RoleField extTs = findUIntRole(optFc, optSv, UIntRole::DefaultClockTimestamp);
            if (extTs.found) {
                // The nested timestamp is the default clock value; in the
                // compact option it is a partial value that must be extended
                // against DEF_CLK_VAL (spec 6.3), not taken verbatim.
                updateClockValue(
                    m_defClkVal, extTs.value, extTs.lengthBits > 0 ? extTs.lengthBits : 64);
                rec.timestamp = m_defClkVal;
            }
        }

        // Spec 4.2.2: with a default clock, the timestamp of an event record
        // (the clock value after its header field is decoded) MUST be >= that of
        // the previous event record in the same data stream.
        if (!m_dsc->defaultClockClassName.isEmpty()) {
            if (m_prevEventTimestamp && rec.timestamp < *m_prevEventTimestamp)
                return Utils::ResultError(
                    u"Event record timestamp %1 is less than the previous timestamp %2"_s
                        .arg(rec.timestamp)
                        .arg(*m_prevEventTimestamp));
            m_prevEventTimestamp = rec.timestamp;
        }
    }

    if (m_dsc->eventRecordCommonContextFieldClass) {
        auto r = FieldReader(*m_buf, makeResolver(), FieldLocation::Origin::EventRecordCommonContext)
                     .read(*m_dsc->eventRecordCommonContextFieldClass);
        if (!r)
            return Utils::ResultError(r.error());
        if (isStructure(*r))
            rec.commonContext = asStructure(*r);
        m_evCommonContext = rec.commonContext;
    }

    for (const auto &erc : m_dsc->eventRecordClasses) {
        if (erc.id == rec.eventClassId) {
            rec.eventClass = &erc;
            break;
        }
    }

    // Spec 6.2: an event record class with this id MUST exist in the data stream
    // class; otherwise abort.
    if (!rec.eventClass)
        return Utils::ResultError(
            u"No event record class with id %1 in data stream class %2"_s.arg(rec.eventClassId)
                .arg(m_dsc->id));

    {
        if (rec.eventClass->specificContextFieldClass) {
            auto r = FieldReader(
                         *m_buf, makeResolver(), FieldLocation::Origin::EventRecordSpecificContext)
                         .read(*rec.eventClass->specificContextFieldClass);
            if (!r)
                return Utils::ResultError(r.error());
            if (isStructure(*r))
                rec.specificContext = asStructure(*r);
            m_evSpecificContext = rec.specificContext;
        }
        if (rec.eventClass->payloadFieldClass) {
            auto r = FieldReader(*m_buf, makeResolver(), FieldLocation::Origin::EventRecordPayload)
                         .read(*rec.eventClass->payloadFieldClass);
            if (!r)
                return Utils::ResultError(r.error());
            if (isStructure(*r))
                rec.payload = asStructure(*r);
        }
    }

    if (m_buf->readBitOffset() == startBitOffset && startBitOffset < m_contentEndBit)
        return Utils::ResultError(u"Zero-size event in non-empty packet"_s);

    return rec;
}

Utils::Result<> PacketReader::selectDsc(
    const StructureValue &hdr, const QHash<QString, int> &clockBits)
{
    // Only when the trace carries a packet-header data-stream-class-id role and
    // we have the full schema to resolve it against (spec 6.1).
    if (!m_schema || !m_tc || !m_tc->packetHeaderFieldClass)
        return Utils::ResultOk;
    const RoleField id = findUIntRole(
        m_tc->packetHeaderFieldClass.get(), hdr, UIntRole::DataStreamClassId, clockBits);
    if (!id.found)
        return Utils::ResultOk;
    if (const DataStreamClass *sel = m_schema->findDataStreamClass(id.value)) {
        m_dsc = sel;
        return Utils::ResultOk;
    }
    // Spec 6.1: if no data stream class has the ID DSC_ID, report an error and
    // abort the data stream decoding process.
    return Utils::ResultError(u"No data stream class with id %1 for this packet"_s.arg(id.value));
}

FieldLocationResolver PacketReader::makeResolver()
{
    return [this](const FieldLocation &loc) -> std::optional<quint64> {
        return resolveFromRoots(loc);
    };
}

std::optional<quint64> PacketReader::resolveFromRoots(const FieldLocation &loc) const
{
    // Only origin-anchored locations into an already-decoded root are resolved
    // here; same-root and no-origin lookups are handled by FieldReader's scope
    // stack. Returns nullopt (not 0) when nothing matches, so a real value of 0
    // is never mistaken for "not found".
    if (!loc.hasOrigin || loc.path.isEmpty())
        return std::nullopt;

    const StructureValue *root = nullptr;
    switch (loc.origin) {
    case FieldLocation::Origin::PacketHeader:
        root = &m_pktHeader;
        break;
    case FieldLocation::Origin::PacketContext:
        root = &m_pktContext;
        break;
    case FieldLocation::Origin::EventRecordHeader:
        root = &m_evHeader;
        break;
    case FieldLocation::Origin::EventRecordCommonContext:
        root = &m_evCommonContext;
        break;
    case FieldLocation::Origin::EventRecordSpecificContext:
        root = &m_evSpecificContext;
        break;
    case FieldLocation::Origin::EventRecordPayload:
        return std::nullopt; // in-progress; scope stack handles it
    }
    if (!root)
        return std::nullopt;

    // Descend the path through the decoded value tree. `null` (parent) elements
    // aren't representable here and are skipped.
    const StructureValue *cur = root;
    const FieldValue *value = nullptr;
    for (const auto &seg : loc.path) {
        if (!seg.has_value())
            continue;
        if (!cur)
            return std::nullopt;
        value = cur->get(*seg);
        if (!value)
            return std::nullopt;
        cur = isStructure(*value) ? &asStructure(*value) : nullptr;
    }
    if (!value)
        return std::nullopt;
    if (std::holds_alternative<quint64>(*value))
        return std::get<quint64>(*value);
    if (std::holds_alternative<qint64>(*value))
        return static_cast<quint64>(std::get<qint64>(*value));
    if (std::holds_alternative<bool>(*value))
        return std::get<bool>(*value) ? quint64(1) : quint64(0);
    return std::nullopt;
}

bool PacketReader::atEndOfPacket() const
{
    return !m_buf || m_buf->readBitOffset() >= m_contentEndBit;
}

bool PacketReader::atEnd() const
{
    return m_dev->atEnd() && atEndOfPacket();
}

} // namespace CommonTraceFormat
