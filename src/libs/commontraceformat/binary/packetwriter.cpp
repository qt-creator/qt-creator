// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "packetwriter.h"

#include "fieldwriter.h"

#include "../schema/blobfieldclasses.h"
#include "../schema/compoundfieldclasses.h"
#include "../schema/scalarfieldclasses.h"
#include "metadata/metadatareader.h"

#include <QBuffer>
#include <QIODevice>

namespace CommonTraceFormat {
using namespace Qt::StringLiterals;

static constexpr quint32 CTF2_MAGIC = 0xC1FC1FC1u;

PacketWriter::PacketWriter(
    QIODevice *dev, const DataStreamClass &dsc, const TraceClass *tc, std::optional<QUuid> uuid)
    : m_dev(dev)
    , m_dsc(dsc)
    , m_tc(tc)
    , m_uuid(uuid)
{}

// Name of the first top-level member carrying the unsigned-integer `role`, or
// empty if there is none.
static QString memberNameWithRole(const FieldClass *fc, UIntRole role)
{
    if (!fc || fc->kind != FieldClassKind::Structure)
        return {};
    for (const auto &m : static_cast<const StructureFC &>(*fc).members) {
        const FieldClass *mfc = m.fieldClass.get();
        if (!mfc)
            continue;
        if (mfc->kind == FieldClassKind::FixedLengthUInt
            && static_cast<const FixedLengthUIntFC &>(*mfc).roles.contains(role))
            return m.name;
        if (mfc->kind == FieldClassKind::VariableLengthUInt
            && static_cast<const VariableLengthUIntFC &>(*mfc).roles.contains(role))
            return m.name;
    }
    return {};
}

// Name of the first packet-header member that is a static-length BLOB carrying
// the metadata-stream-uuid role, or empty if there is none.
static QString uuidMemberName(const FieldClass *fc)
{
    if (!fc || fc->kind != FieldClassKind::Structure)
        return {};
    for (const auto &m : static_cast<const StructureFC &>(*fc).members) {
        if (m.fieldClass && m.fieldClass->kind == FieldClassKind::StaticLengthBlob) {
            const auto &b = static_cast<const StaticLengthBlobFC &>(*m.fieldClass);
            if (b.roles.contains(BlobRole::MetadataStreamUuid))
                return m.name;
        }
    }
    return {};
}

// Whether a variant option's selector ranges/values cover `v`.
static bool optionCovers(const VariantOption &opt, qint64 v)
{
    if (opt.selectorValues.contains(v))
        return true;
    for (const auto &r : opt.selectorRanges)
        if (v >= r.lo && v <= r.hi)
            return true;
    return false;
}

// A selector value guaranteed to land in this option (for the top-level
// selector field), or 0 if the option matches everything.
static qint64 anyOptionSelector(const VariantOption &opt)
{
    if (!opt.selectorValues.isEmpty())
        return opt.selectorValues.first();
    if (!opt.selectorRanges.isEmpty())
        return opt.selectorRanges.first().lo;
    return 0;
}

// Build an event-record-header value that encodes `eventClassId`/`timestamp`
// into whatever role-bearing fields the header field class declares. Handles
// both the simple flat layout (top-level id/timestamp roles) and the CTF 1.8
// compact/extended layout where those fields live inside a variant option
// selected by the top-level id (the inverse of PacketReader's special case).
static StructureValue makeEventHeader(const StructureFC &hfc, quint64 eventClassId, quint64 timestamp)
{
    StructureValue hdr;

    const VariantFC *var = nullptr;
    QString varName;
    for (const auto &m : hfc.members) {
        if (m.fieldClass && m.fieldClass->kind == FieldClassKind::Variant) {
            var = static_cast<const VariantFC *>(m.fieldClass.get());
            varName = m.name;
            break;
        }
    }

    quint64 topId = eventClassId;

    if (var) {
        // Prefer a "compact" option (no nested id role) whose selector range
        // covers the id; otherwise an "extended" option that carries the id in a
        // nested field, with the top-level selector set into its range.
        int chosen = -1;
        for (int i = 0; i < var->options.size(); ++i) {
            const VariantOption &opt = var->options[i];
            const bool hasNestedId
                = !memberNameWithRole(opt.fieldClass.get(), UIntRole::EventRecordClassId).isEmpty();
            if (!hasNestedId && optionCovers(opt, qint64(eventClassId))) {
                chosen = i;
                topId = eventClassId;
                break;
            }
        }
        if (chosen < 0) {
            for (int i = 0; i < var->options.size(); ++i) {
                const VariantOption &opt = var->options[i];
                if (!memberNameWithRole(opt.fieldClass.get(), UIntRole::EventRecordClassId)
                         .isEmpty()) {
                    chosen = i;
                    topId = quint64(anyOptionSelector(opt));
                    break;
                }
            }
        }
        if (chosen < 0)
            chosen = 0;

        const VariantOption &opt = var->options[chosen];
        StructureValue nested;
        if (opt.fieldClass && opt.fieldClass->kind == FieldClassKind::Structure) {
            const auto &osfc = static_cast<const StructureFC &>(*opt.fieldClass);
            const QString nId = memberNameWithRole(&osfc, UIntRole::EventRecordClassId);
            if (!nId.isEmpty())
                nested.set(nId, eventClassId);
            const QString nTs = memberNameWithRole(&osfc, UIntRole::DefaultClockTimestamp);
            if (!nTs.isEmpty())
                nested.set(nTs, timestamp);
        }
        auto vv = std::make_shared<VariantValue>();
        vv->selectedIndex = chosen;
        vv->selectedOption = opt.name;
        vv->value = std::make_unique<FieldValue>(makeStructureValue(std::move(nested)));
        hdr.set(varName, FieldValue(std::move(vv)));
    }

    const QString idName = memberNameWithRole(&hfc, UIntRole::EventRecordClassId);
    if (!idName.isEmpty())
        hdr.set(idName, topId);
    const QString tsName = memberNameWithRole(&hfc, UIntRole::DefaultClockTimestamp);
    if (!tsName.isEmpty())
        hdr.set(tsName, timestamp);

    return hdr;
}

// Out-of-line so the unique_ptr members can be destroyed where QBuffer/BitBuffer
// are complete types. m_buf (the BitBuffer) is declared after m_qbuf, so it is
// destroyed first — i.e. before the QBuffer it wraps.
PacketWriter::~PacketWriter() = default;

static qint64 alignUp(qint64 v, int alignment)
{
    if (alignment <= 1)
        return v;
    return ((v + alignment - 1) / alignment) * alignment;
}

// Location (after alignment) of the first top-level member carrying `role`,
// mirroring the per-member alignment the FieldWriter applies (spec 6.4.1).
// `bitOffset` is -1 if the role is absent or a member of an unmeasurable kind
// precedes it (in which case the field is simply not back-patched).
static RoleFieldPos findRoleField(const FieldClass *fc, UIntRole role, qint64 baseOffset = 0)
{
    if (!fc || fc->kind != FieldClassKind::Structure)
        return {};

    const auto &sfc = static_cast<const StructureFC &>(*fc);
    qint64 bitPos = alignUp(baseOffset, sfc.minimumAlignment);
    for (const auto &member : sfc.members) {
        const FieldClass *mfc = member.fieldClass.get();
        if (!mfc)
            return {};
        int alignment = 1;
        int length = -1;
        switch (mfc->kind) {
        case FieldClassKind::FixedLengthUInt: {
            const auto &ufc = static_cast<const FixedLengthUIntFC &>(*mfc);
            bitPos = alignUp(bitPos, ufc.alignment);
            if (ufc.roles.contains(role))
                return {bitPos, ufc.length, ufc.byteOrder, ufc.bitOrder};
            length = ufc.length;
            break;
        }
        case FieldClassKind::FixedLengthSInt:
            alignment = static_cast<const FixedLengthSIntFC &>(*mfc).alignment;
            length = static_cast<const FixedLengthSIntFC &>(*mfc).length;
            bitPos = alignUp(bitPos, alignment);
            break;
        case FieldClassKind::FixedLengthBitArray:
        case FieldClassKind::FixedLengthBitMap:
            alignment = static_cast<const FixedLengthBitArrayFC &>(*mfc).alignment;
            length = static_cast<const FixedLengthBitArrayFC &>(*mfc).length;
            bitPos = alignUp(bitPos, alignment);
            break;
        case FieldClassKind::FixedLengthBool:
            alignment = static_cast<const FixedLengthBoolFC &>(*mfc).alignment;
            length = static_cast<const FixedLengthBoolFC &>(*mfc).length;
            bitPos = alignUp(bitPos, alignment);
            break;
        case FieldClassKind::FixedLengthFloat:
            alignment = static_cast<const FixedLengthFloatFC &>(*mfc).alignment;
            length = static_cast<const FixedLengthFloatFC &>(*mfc).length;
            bitPos = alignUp(bitPos, alignment);
            break;
        default:
            // Variable-length or compound member: size not statically known.
            return {};
        }
        bitPos += length;
    }
    return {};
}

void PacketWriter::beginPacket(const StructureValue &packetContext)
{
    m_scratch.clear();
    m_scratch.reserve(65536);

    // Reset m_buf (wraps m_qbuf) before m_qbuf so the wrapper never outlives it.
    m_buf.reset();
    m_qbuf = std::make_unique<QBuffer>(&m_scratch);
    m_qbuf->open(QIODevice::WriteOnly);
    m_buf = std::make_unique<BitBuffer>(m_qbuf.get());

    m_totalLength = {};
    m_contentLength = {};
    m_beginClock = {};
    m_endClock = {};
    m_firstTimestamp.reset();
    m_lastTimestamp.reset();
    m_inPacket = true;
    m_packetCtxValue = packetContext;

    if (m_tc && m_tc->packetHeaderFieldClass) {
        const FieldClass *phfc = m_tc->packetHeaderFieldClass.get();
        StructureValue hdr;
        hdr.set(u"magic"_s, quint64(CTF2_MAGIC));
        // Fill the metadata-stream-uuid blob from the preamble UUID; the reader
        // checks the two match (spec 5.6.1).
        if (m_uuid) {
            const QString name = uuidMemberName(phfc);
            if (!name.isEmpty())
                hdr.set(name, m_uuid->toRfc4122());
        }
        // Identify which data stream class this packet belongs to, so a reader
        // re-selects it from the header (spec 6.1) instead of the default class 0.
        const QString dscIdName = memberNameWithRole(phfc, UIntRole::DataStreamClassId);
        if (!dscIdName.isEmpty())
            hdr.set(dscIdName, m_dsc.id);
        FieldWriter(*m_buf, makeResolver(packetContext)).write(*phfc, makeStructureValue(hdr));
    }

    if (m_dsc.packetContextFieldClass) {
        const FieldClass *ctxFc = m_dsc.packetContextFieldClass.get();
        const qint64 ctxBase = m_buf->writeBitOffset();
        m_totalLength = findRoleField(ctxFc, UIntRole::PacketTotalLength, ctxBase);
        m_contentLength = findRoleField(ctxFc, UIntRole::PacketContentLength, ctxBase);
        // Packet beginning/end default-clock values are back-patched in
        // finalize() from the first/last event timestamps, so that the reader
        // can re-extend the (possibly partial) per-event timestamps (spec 6.3).
        // A value the caller put in the packet context takes precedence and is
        // left untouched.
        m_beginClock = findRoleField(ctxFc, UIntRole::DefaultClockTimestamp, ctxBase);
        m_endClock = findRoleField(ctxFc, UIntRole::PacketEndDefaultClockTimestamp, ctxBase);
        const QString beginName = memberNameWithRole(ctxFc, UIntRole::DefaultClockTimestamp);
        if (!beginName.isEmpty() && packetContext.get(beginName))
            m_beginClock = {};
        const QString endName = memberNameWithRole(ctxFc, UIntRole::PacketEndDefaultClockTimestamp);
        if (!endName.isEmpty() && packetContext.get(endName))
            m_endClock = {};

        StructureValue ctx = packetContext;
        FieldWriter(*m_buf, makeResolver(packetContext))
            .write(*m_dsc.packetContextFieldClass, makeStructureValue(ctx));
    }

    m_contentBitOffset = m_buf->writeBitOffset();
}

void PacketWriter::writeEventRecord(
    quint64 eventClassId,
    const StructureValue &payload,
    const StructureValue &specificCtx,
    const StructureValue &commonCtx,
    quint64 timestamp)
{
    if (!m_inPacket)
        return;

    if (!m_firstTimestamp)
        m_firstTimestamp = timestamp;
    m_lastTimestamp = timestamp;

    auto resolver = makeResolver(m_packetCtxValue);

    if (m_dsc.eventRecordHeaderFieldClass) {
        const FieldClass *hfc = m_dsc.eventRecordHeaderFieldClass.get();
        StructureValue hdr;
        if (hfc->kind == FieldClassKind::Structure)
            hdr = makeEventHeader(static_cast<const StructureFC &>(*hfc), eventClassId, timestamp);
        FieldWriter(*m_buf, resolver).write(*hfc, makeStructureValue(hdr));
    }

    if (m_dsc.eventRecordCommonContextFieldClass)
        FieldWriter(*m_buf, resolver)
            .write(*m_dsc.eventRecordCommonContextFieldClass, makeStructureValue(commonCtx));

    const EventRecordClass *erc = nullptr;
    for (const auto &e : m_dsc.eventRecordClasses) {
        if (e.id == eventClassId) {
            erc = &e;
            break;
        }
    }

    if (erc) {
        if (erc->specificContextFieldClass)
            FieldWriter(*m_buf, resolver)
                .write(*erc->specificContextFieldClass, makeStructureValue(specificCtx));
        if (erc->payloadFieldClass)
            FieldWriter(*m_buf, resolver).write(*erc->payloadFieldClass, makeStructureValue(payload));
    }
}

Utils::Result<> PacketWriter::finalize()
{
    if (!m_inPacket)
        return Utils::ResultError(u"PacketWriter: finalize() called outside a packet"_s);

    // Content length is the bits occupied by content (header, context and all
    // event records), captured before the trailing packet byte-alignment
    // padding (spec 4.2.1 / 6.1). Total length includes that padding.
    const qint64 contentBits = m_buf->writeBitOffset();
    m_buf->alignWriteTo(8);
    const qint64 totalBits = m_buf->writeBitOffset();

    // The length roles are expressed in bits (spec 6.1) and patched using the
    // field's own width and byte/bit order, which need not be 32-bit LE.
    if (m_totalLength.found())
        m_buf->patchBits(
            m_totalLength.bitOffset,
            static_cast<quint64>(totalBits),
            m_totalLength.length,
            m_totalLength.byteOrder,
            m_totalLength.bitOrder);
    if (m_contentLength.found())
        m_buf->patchBits(
            m_contentLength.bitOffset,
            static_cast<quint64>(contentBits),
            m_contentLength.length,
            m_contentLength.byteOrder,
            m_contentLength.bitOrder);

    // Fill the packet beginning/end default-clock values from the first/last
    // event timestamps so a reader can re-extend the partial per-event
    // timestamps (spec 6.3). A field narrower than 64 bits stores the low bits.
    if (m_beginClock.found() && m_firstTimestamp)
        m_buf->patchBits(
            m_beginClock.bitOffset,
            *m_firstTimestamp,
            m_beginClock.length,
            m_beginClock.byteOrder,
            m_beginClock.bitOrder);
    if (m_endClock.found() && m_lastTimestamp)
        m_buf->patchBits(
            m_endClock.bitOffset,
            *m_lastTimestamp,
            m_endClock.length,
            m_endClock.byteOrder,
            m_endClock.bitOrder);

    Utils::Result<> flushed = m_buf->flush();
    m_inPacket = false;

    if (flushed) {
        qint64 written = m_dev->write(m_scratch);
        if (written != m_scratch.size())
            flushed = Utils::ResultError(u"PacketWriter: short write to device"_s);
    }

    m_scratch.clear();
    return flushed;
}

FieldLocationResolver PacketWriter::makeResolver(const StructureValue &packetCtx) const
{
    return [&packetCtx](const FieldLocation &loc) -> std::optional<quint64> {
        if (loc.path.isEmpty())
            return std::nullopt;
        const QString *name = loc.path.first() ? &*loc.path.first() : nullptr;
        if (!name)
            return std::nullopt;
        const FieldValue *fv = packetCtx.get(*name);
        if (!fv)
            return std::nullopt;
        if (std::holds_alternative<quint64>(*fv))
            return std::get<quint64>(*fv);
        if (std::holds_alternative<qint64>(*fv))
            return static_cast<quint64>(std::get<qint64>(*fv));
        return std::nullopt;
    };
}

} // namespace CommonTraceFormat
