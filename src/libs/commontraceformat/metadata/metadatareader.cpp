// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "metadatareader.h"

#include "../schema/blobfieldclasses.h"
#include "../schema/compoundfieldclasses.h"
#include "../schema/scalarfieldclasses.h"
#include "../schema/stringfieldclasses.h"

#include <utils/result.h>

#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QPair>
#include <QSet>

#include <algorithm>
#include <limits>

namespace CommonTraceFormat {
using namespace Qt::StringLiterals;

static QString parseErr(const QString &msg, qint64 offset = -1, int fragmentIdx = -1)
{
    QString s = msg;
    if (offset >= 0)
        s += u" (at byte offset %1)"_s.arg(offset);
    if (fragmentIdx >= 0)
        s += u" (fragment %1)"_s.arg(fragmentIdx);
    return s;
}

// Any `extensions` on a metadata object MUST be declared in the preamble (spec
// 5.1). Since this consumer supports no extensions, the preamble must declare
// none — so any non-empty `extensions` anywhere is unsupported and rejected.
static Utils::Result<void> checkNoExtensions(const QJsonObject &obj, int idx)
{
    const QJsonValue e = obj[u"extensions"];
    if (e.isObject() && !e.toObject().isEmpty())
        return Utils::ResultError(
            parseErr(u"unsupported extension(s) present (this consumer supports none)"_s, -1, idx));
    return Utils::ResultOk;
}

// The (optional) `attributes` object of a metadata JSON object (spec 5.2).
// Preserved verbatim for round-tripping; a consumer never needs it to decode.
static QJsonObject parseAttributes(const QJsonObject &obj)
{
    return obj[u"attributes"].toObject();
}

// Which root structure a field class is, for role-scope validation (spec
// 5.6.1 / 5.8.1): each root accepts a different set of roles, and the packet
// header and event-record header additionally constrain every member.
enum class RootScope { TracePacketHeader, DataStreamPacketContext, EventRecordHeader };

// Validates the role MUST-constraints (spec 5.6.1 / 5.8.1) on the top-level
// members of a root structure field class. `dscHasClock` is true when the
// owning data stream class has a default-clock-class-id.
static Utils::Result<void> validateRootRoles(
    const FieldClassPtr &rootFc, const Schema &schema, RootScope scope, bool dscHasClock, int idx);

MetadataReader::MetadataReader(QIODevice *dev)
    : m_dev(dev)
{}

QList<QByteArray> MetadataReader::readFragments(qint64 &errorOffset)
{
    QByteArray all = m_dev->readAll();
    QList<QByteArray> fragments;
    int pos = 0;

    while (pos < all.size()) {
        if (static_cast<uint8_t>(all[pos]) != 0x1E) {
            errorOffset = pos;
            return {};
        }
        ++pos;
        // find next RS or EOF
        int next = pos;
        while (next < all.size() && static_cast<uint8_t>(all[next]) != 0x1E)
            ++next;
        QByteArray fragment = all.mid(pos, next - pos).trimmed();
        if (!fragment.isEmpty())
            fragments.append(fragment);
        pos = next;
    }
    errorOffset = -1;
    return fragments;
}

Utils::Result<Schema> MetadataReader::read()
{
    qint64 errorOffset = -1;
    QList<QByteArray> fragments = readFragments(errorOffset);
    if (errorOffset >= 0)
        return Utils::ResultError(
            parseErr(u"Expected RS byte (0x1E) at start of fragment"_s, errorOffset));

    Schema schema;
    int preambleCount = 0;
    bool hasTraceClass = false;
    bool hasDataStreamClass = false;

    for (int idx = 0; idx < fragments.size(); ++idx) {
        QJsonParseError jsonErr;
        QJsonDocument doc = QJsonDocument::fromJson(fragments[idx], &jsonErr);
        if (doc.isNull())
            return Utils::ResultError(
                parseErr(u"JSON parse error: "_s + jsonErr.errorString(), -1, idx));

        QJsonObject obj = doc.object();
        QString type = obj[u"type"].toString();

        // The metadata stream MUST start with the (single) preamble fragment.
        if (idx == 0 && type != u"preamble")
            return Utils::ResultError(u"Metadata stream must start with a preamble fragment"_s);

        if (type == u"preamble") {
            if (++preambleCount > 1)
                return Utils::ResultError(parseErr(
                    u"Metadata stream must contain exactly one preamble fragment"_s, -1, idx));
            auto r = parsePreamble(obj, schema, idx);
            if (!r)
                return Utils::ResultError(r.error());
        } else if (type == u"field-class-alias") {
            auto r = parseFieldClassAlias(obj, schema, idx);
            if (!r)
                return Utils::ResultError(r.error());
        } else if (type == u"trace-class") {
            if (hasTraceClass)
                return Utils::ResultError(parseErr(
                    u"metadata stream must contain at most one trace-class fragment"_s, -1, idx));
            if (hasDataStreamClass)
                return Utils::ResultError(parseErr(
                    u"trace-class fragment must precede any data-stream-class fragment"_s, -1, idx));
            auto r = parseTraceClass(obj, schema, idx);
            if (!r)
                return Utils::ResultError(r.error());
            hasTraceClass = true;
        } else if (type == u"clock-class") {
            auto r = parseClockClass(obj, schema, idx);
            if (!r)
                return Utils::ResultError(r.error());
        } else if (type == u"data-stream-class") {
            auto r = parseDataStreamClass(obj, schema, idx);
            if (!r)
                return Utils::ResultError(r.error());
            hasDataStreamClass = true;
        } else if (type == u"event-record-class") {
            auto r = parseEventRecordClass(obj, schema, idx);
            if (!r)
                return Utils::ResultError(r.error());
        }
        // Unknown fragment types are silently ignored per spec.
    }

    if (preambleCount == 0)
        return Utils::ResultError(u"Missing preamble fragment"_s);
    // Note: the spec requires at least one data-stream-class fragment in a
    // complete metadata stream, but this is not enforced here so that partial
    // streams (e.g. a live-tracing prefix that hasn't sent its DSCs yet) parse.
    Q_UNUSED(hasTraceClass)
    Q_UNUSED(hasDataStreamClass)

    return schema;
}

Utils::Result<void> MetadataReader::parsePreamble(const QJsonObject &obj, Schema &schema, int idx)
{
    int version = obj[u"version"].toInt(-1);
    if (version != 2)
        return Utils::ResultError(parseErr(u"Preamble version must be 2"_s, -1, idx));

    // Extensions declared here enable format-altering features. This consumer
    // supports none, so any declared extension means it MUST NOT consume the
    // data streams (spec 5.1).
    const QJsonObject exts = obj[u"extensions"].toObject();
    if (!exts.isEmpty()) {
        QStringList names;
        for (auto ns = exts.begin(); ns != exts.end(); ++ns) {
            const QJsonObject nsObj = ns.value().toObject();
            for (auto e = nsObj.begin(); e != nsObj.end(); ++e)
                names.append(ns.key() + u'/' + e.key());
        }
        return Utils::ResultError(parseErr(
            u"Unsupported metadata extension(s) declared: %1"_s.arg(names.join(u", ")), -1, idx));
    }

    if (obj.contains(u"uuid")) {
        // Spec 5.4: uuid MUST be a JSON array of 16 JSON integers.
        const QJsonValue uuidVal = obj[u"uuid"];
        if (!uuidVal.isArray())
            return Utils::ResultError(
                parseErr(u"Preamble uuid must be an array of 16 integers"_s, -1, idx));
        const QJsonArray arr = uuidVal.toArray();
        if (arr.size() != 16)
            return Utils::ResultError(
                parseErr(u"Preamble uuid must have exactly 16 elements"_s, -1, idx));
        QByteArray bytes(16, '\0');
        for (int i = 0; i < 16; ++i) {
            // Each element MUST be a byte value; reject out-of-range integers
            // rather than silently truncating on the narrowing cast.
            const qint64 b = arr[i].toInteger(-1);
            if (b < 0 || b > 255)
                return Utils::ResultError(
                    parseErr(u"Preamble uuid elements must be integers in [0, 255]"_s, -1, idx));
            bytes[i] = static_cast<char>(b);
        }
        const QUuid uuid = QUuid::fromRfc4122(bytes);
        if (uuid.isNull())
            return Utils::ResultError(parseErr(u"Invalid UUID in preamble"_s, -1, idx));
        schema.uuid = uuid;
    }
    return Utils::ResultOk;
}

Utils::Result<void> MetadataReader::parseFieldClassAlias(
    const QJsonObject &obj, Schema &schema, int idx)
{
    if (auto r = checkNoExtensions(obj, idx); !r)
        return Utils::ResultError(r.error());
    QString name = obj[u"name"].toString();
    if (name.isEmpty())
        return Utils::ResultError(parseErr(u"field-class-alias missing name"_s, -1, idx));
    // Spec 5.5: two field-class-alias fragments MUST NOT share the same name.
    if (schema.fieldClassAliases.contains(name))
        return Utils::ResultError(
            parseErr(u"duplicate field-class-alias name: %1"_s.arg(name), -1, idx));

    auto fcResult = parseFieldClass(obj[u"field-class"], schema, idx);
    if (!fcResult)
        return Utils::ResultError(fcResult.error());

    schema.fieldClassAliases[name] = std::move(*fcResult);
    schema.fieldClassAliasOrder.append(name);
    return Utils::ResultOk;
}

// Parse a fragment property that must reference a structure field class
// (directly or via an alias). Stores the result in `out`.
Utils::Result<void> MetadataReader::parseRootStructureFieldClass(
    const QJsonObject &obj,
    const QString &key,
    const Schema &schema,
    int idx,
    FieldClassPtr &out) const
{
    if (!obj.contains(key))
        return Utils::ResultOk;
    auto r = parseFieldClass(obj[key], schema, idx);
    if (!r)
        return Utils::ResultError(r.error());
    if (!*r || (*r)->kind != FieldClassKind::Structure)
        return Utils::ResultError(
            parseErr(u"%1 must be a structure field class"_s.arg(key), -1, idx));
    out = std::move(*r);
    return Utils::ResultOk;
}

Utils::Result<void> MetadataReader::parseTraceClass(const QJsonObject &obj, Schema &schema, int idx)
{
    if (auto r = checkNoExtensions(obj, idx); !r)
        return Utils::ResultError(r.error());
    TraceClass tc;
    tc.attributes = parseAttributes(obj);
    tc.namespaceName = obj[u"namespace"].toString();
    tc.name = obj[u"name"].toString();
    tc.uid = obj[u"uid"].toString();
    // Environment entries: JSON string or integer values (spec 5.6).
    const QJsonObject env = obj[u"environment"].toObject();
    for (auto it = env.begin(); it != env.end(); ++it) {
        const QJsonValue ev = it.value();
        if (ev.isString())
            tc.environment.insert(it.key(), ev.toString());
        else if (ev.isDouble())
            tc.environment.insert(it.key(), QString::number(ev.toInteger()));
    }
    if (auto r = parseRootStructureFieldClass(
            obj, u"packet-header-field-class"_s, schema, idx, tc.packetHeaderFieldClass);
        !r)
        return Utils::ResultError(r.error());
    if (auto r = validateRootRoles(
            tc.packetHeaderFieldClass,
            schema,
            RootScope::TracePacketHeader,
            /*dscHasClock=*/false,
            idx);
        !r)
        return Utils::ResultError(r.error());
    schema.traceClass = std::move(tc);
    return Utils::ResultOk;
}

Utils::Result<void> MetadataReader::parseClockClass(const QJsonObject &obj, Schema &schema, int idx)
{
    if (auto r = checkNoExtensions(obj, idx); !r)
        return Utils::ResultError(r.error());
    ClockClass cc;
    cc.attributes = parseAttributes(obj);
    cc.id = obj[u"id"].toString();
    if (cc.id.isEmpty())
        return Utils::ResultError(parseErr(u"clock-class missing required id"_s, -1, idx));
    for (const auto &existing : schema.clockClasses) {
        if (existing.id == cc.id)
            return Utils::ResultError(
                parseErr(u"duplicate clock-class id: %1"_s.arg(cc.id), -1, idx));
    }
    cc.namespaceName = obj[u"namespace"].toString();
    cc.name = obj[u"name"].toString();
    cc.uid = obj[u"uid"].toString();

    // frequency/precision/accuracy/offset members are JSON integers (spec 5.7);
    // read them as such to avoid the precision loss of a double round-trip.
    if (!obj.contains(u"frequency"))
        return Utils::ResultError(parseErr(u"clock-class missing required frequency"_s, -1, idx));
    const qint64 freq = obj[u"frequency"].toInteger(0);
    if (freq <= 0)
        return Utils::ResultError(
            parseErr(u"clock-class frequency must be greater than zero"_s, -1, idx));
    cc.frequency = static_cast<quint64>(freq);

    // precision/accuracy MUST be >= 0 when present (spec 5.7).
    if (obj.contains(u"precision")) {
        const qint64 p = obj[u"precision"].toInteger(0);
        if (p < 0)
            return Utils::ResultError(parseErr(u"clock-class precision must be >= 0"_s, -1, idx));
        cc.precision = static_cast<quint64>(p);
    }
    if (obj.contains(u"accuracy")) {
        const qint64 a = obj[u"accuracy"].toInteger(0);
        if (a < 0)
            return Utils::ResultError(parseErr(u"clock-class accuracy must be >= 0"_s, -1, idx));
        cc.accuracy = static_cast<quint64>(a);
    }
    cc.description = obj[u"description"].toString();

    // Note: CTF2 clock classes have no `uuid` property (spec 5.7); clock identity
    // is namespace/name/uid. Any `uuid` key is a CTF1 leftover and is ignored.

    // Spec 5.7 names the property `offset-from-origin` only.
    if (obj.contains(u"offset-from-origin")) {
        QJsonObject off = obj[u"offset-from-origin"].toObject();
        cc.offsetSeconds = off[u"seconds"].toInteger(0);
        // cycles MUST be >= 0 and < frequency (spec 5.7.2).
        const qint64 cycles = off[u"cycles"].toInteger(0);
        if (cycles < 0)
            return Utils::ResultError(
                parseErr(u"clock-class offset cycles must be >= 0"_s, -1, idx));
        if (static_cast<quint64>(cycles) >= cc.frequency)
            return Utils::ResultError(
                parseErr(u"clock-class offset cycles must be less than frequency"_s, -1, idx));
        cc.offsetCycles = static_cast<quint64>(cycles);
    }

    if (obj.contains(u"origin")) {
        QJsonValue origin = obj[u"origin"];
        if (origin.isString()) {
            if (origin.toString() != u"unix-epoch")
                return Utils::ResultError(
                    parseErr(u"clock-class origin string must be \"unix-epoch\""_s, -1, idx));
            cc.origin.isUnixEpoch = true;
        } else if (origin.isObject()) {
            QJsonObject oo = origin.toObject();
            cc.origin.name = oo[u"name"].toString();
            cc.origin.namespaceName = oo[u"namespace"].toString();
            cc.origin.uid = oo[u"uid"].toString();
            // name and uid are required in a clock-origin object (spec 5.7.1).
            if (cc.origin.name.isEmpty() || cc.origin.uid.isEmpty())
                return Utils::ResultError(
                    parseErr(u"clock-class custom origin must have name and uid"_s, -1, idx));
        } else {
            return Utils::ResultError(parseErr(
                u"clock-class origin must be \"unix-epoch\" or a clock origin object"_s, -1, idx));
        }
    }

    schema.clockClasses.append(std::move(cc));
    return Utils::ResultOk;
}

Utils::Result<void> MetadataReader::parseDataStreamClass(
    const QJsonObject &obj, Schema &schema, int idx)
{
    if (auto r = checkNoExtensions(obj, idx); !r)
        return Utils::ResultError(r.error());
    DataStreamClass dsc;
    dsc.attributes = parseAttributes(obj);
    // toInteger() is exact across the full qint64 range; toDouble() would lose
    // precision for ids above 2^53 (see parseIntegerRangeSet).
    const qint64 idd = obj[u"id"].toInteger(0);
    if (idd < 0)
        return Utils::ResultError(parseErr(u"data-stream-class id must be >= 0"_s, -1, idx));
    dsc.id = static_cast<quint64>(idd);
    for (const auto &existing : schema.dataStreamClasses) {
        if (existing.id == dsc.id)
            return Utils::ResultError(
                parseErr(u"duplicate data-stream-class id: %1"_s.arg(dsc.id), -1, idx));
    }
    dsc.namespaceName = obj[u"namespace"].toString();
    dsc.name = obj[u"name"].toString();
    dsc.uid = obj[u"uid"].toString();
    // Spec 5.8: a data-stream-class references its default clock class by the
    // clock class `id` property via `default-clock-class-id`.
    dsc.defaultClockClassName = obj[u"default-clock-class-id"].toString();
    // The referenced clock class MUST occur before this fragment (spec 5.8).
    if (!dsc.defaultClockClassName.isEmpty()) {
        const QString &ref = dsc.defaultClockClassName;
        const bool found = std::any_of(
            schema.clockClasses.cbegin(), schema.clockClasses.cend(), [&](const ClockClass &cc) {
                return cc.id == ref;
            });
        if (!found)
            return Utils::ResultError(parseErr(
                u"default-clock-class-id references unknown clock class: %1"_s.arg(ref), -1, idx));
    }
    const bool dscHasClock = !dsc.defaultClockClassName.isEmpty();

    if (auto r = parseRootStructureFieldClass(
            obj, u"packet-context-field-class"_s, schema, idx, dsc.packetContextFieldClass);
        !r)
        return Utils::ResultError(r.error());
    if (auto r = parseRootStructureFieldClass(
            obj, u"event-record-header-field-class"_s, schema, idx, dsc.eventRecordHeaderFieldClass);
        !r)
        return Utils::ResultError(r.error());
    if (auto r = parseRootStructureFieldClass(
            obj,
            u"event-record-common-context-field-class"_s,
            schema,
            idx,
            dsc.eventRecordCommonContextFieldClass);
        !r)
        return Utils::ResultError(r.error());

    if (auto r = validateRootRoles(
            dsc.packetContextFieldClass,
            schema,
            RootScope::DataStreamPacketContext,
            dscHasClock,
            idx);
        !r)
        return Utils::ResultError(r.error());
    if (auto r = validateRootRoles(
            dsc.eventRecordHeaderFieldClass, schema, RootScope::EventRecordHeader, dscHasClock, idx);
        !r)
        return Utils::ResultError(r.error());

    schema.dataStreamClasses.append(std::move(dsc));
    return Utils::ResultOk;
}

Utils::Result<void> MetadataReader::parseEventRecordClass(
    const QJsonObject &obj, Schema &schema, int idx)
{
    if (auto r = checkNoExtensions(obj, idx); !r)
        return Utils::ResultError(r.error());
    EventRecordClass erc;
    erc.attributes = parseAttributes(obj);
    // toInteger() is exact across the full qint64 range; toDouble() would lose
    // precision for ids above 2^53 (see parseIntegerRangeSet).
    const qint64 ercIdd = obj[u"id"].toInteger(0);
    if (ercIdd < 0)
        return Utils::ResultError(parseErr(u"event-record-class id must be >= 0"_s, -1, idx));
    erc.id = static_cast<quint64>(ercIdd);
    erc.namespaceName = obj[u"namespace"].toString();
    erc.name = obj[u"name"].toString();
    erc.uid = obj[u"uid"].toString();

    if (auto r = parseRootStructureFieldClass(
            obj, u"specific-context-field-class"_s, schema, idx, erc.specificContextFieldClass);
        !r)
        return Utils::ResultError(r.error());
    if (auto r = parseRootStructureFieldClass(
            obj, u"payload-field-class"_s, schema, idx, erc.payloadFieldClass);
        !r)
        return Utils::ResultError(r.error());

    const qint64 dscIdd = obj[u"data-stream-class-id"].toInteger(0);
    if (dscIdd < 0)
        return Utils::ResultError(
            parseErr(u"event-record-class data-stream-class-id must be >= 0"_s, -1, idx));
    quint64 dscId = static_cast<quint64>(dscIdd);
    for (auto &dsc : schema.dataStreamClasses) {
        if (dsc.id == dscId) {
            for (const auto &existing : dsc.eventRecordClasses) {
                if (existing.id == erc.id)
                    return Utils::ResultError(parseErr(
                        u"duplicate event-record-class id %1 within data-stream-class %2"_s
                            .arg(erc.id)
                            .arg(dscId),
                        -1,
                        idx));
            }
            dsc.eventRecordClasses.append(std::move(erc));
            return Utils::ResultOk;
        }
    }
    return Utils::ResultError(parseErr(
        u"event-record-class references unknown data-stream-class-id %1"_s.arg(dscId), -1, idx));
}

// ── Helper parsers ──────────────────────────────────────────────────────────

// Parse the (required) `byte-order` property of a fixed-length field class.
// Spec: the value MUST be present and one of "big-endian"/"little-endian".
static Utils::Result<ByteOrder> parseByteOrder(const QJsonObject &obj, int idx)
{
    const QJsonValue v = obj[u"byte-order"];
    if (!v.isString())
        return Utils::ResultError(
            parseErr(u"byte-order is required and must be a string"_s, -1, idx));
    const QString s = v.toString();
    if (s == u"big-endian")
        return ByteOrder::BigEndian;
    if (s == u"little-endian")
        return ByteOrder::LittleEndian;
    return Utils::ResultError(
        parseErr(u"byte-order must be \"big-endian\" or \"little-endian\""_s, -1, idx));
}

// Resolve the bit-order property of a fixed-length field class. Per spec, the
// default is byte-order-dependent: "first-to-last" if the byte order is
// little-endian, otherwise "last-to-first". In the bit array value, the first
// decoded bit is element 0; "first-to-last" maps it to the least-significant
// position (LeastSignificantFirst), "last-to-first" to the most-significant.
static BitOrder resolveBitOrder(const QJsonObject &obj, ByteOrder byteOrder)
{
    const QJsonValue v = obj[u"bit-order"];
    if (v.isString()) {
        if (v.toString() == u"last-to-first")
            return BitOrder::MostSignificantFirst;
        return BitOrder::LeastSignificantFirst; // "first-to-last"
    }
    return byteOrder == ByteOrder::LittleEndian ? BitOrder::LeastSignificantFirst
                                                : BitOrder::MostSignificantFirst;
}

// Parse the optional `preferred-display-base` property (default 10). Spec: when
// present, the value MUST be one of 2, 8, 10, 16.
static Utils::Result<DisplayBase> parseDisplayBase(const QJsonValue &v, int idx)
{
    if (v.isUndefined() || v.isNull())
        return DisplayBase::Decimal;
    if (!v.isDouble())
        return Utils::ResultError(parseErr(u"preferred-display-base must be an integer"_s, -1, idx));
    switch (v.toInt()) {
    case 2:
        return DisplayBase::Binary;
    case 8:
        return DisplayBase::Octal;
    case 10:
        return DisplayBase::Decimal;
    case 16:
        return DisplayBase::Hexadecimal;
    }
    return Utils::ResultError(
        parseErr(u"preferred-display-base must be one of 2, 8, 10, 16"_s, -1, idx));
}

// Parse the optional `encoding` property of a string field class (default
// "utf-8"). Spec: when present, the value MUST be one of the five encodings.
static Utils::Result<StringEncoding> parseStringEncoding(const QJsonValue &v, int idx)
{
    if (v.isUndefined() || v.isNull())
        return StringEncoding::Utf8;
    if (!v.isString())
        return Utils::ResultError(parseErr(u"encoding must be a string"_s, -1, idx));
    const QString s = v.toString();
    if (s == u"utf-8")
        return StringEncoding::Utf8;
    if (s == u"utf-16be")
        return StringEncoding::Utf16Be;
    if (s == u"utf-16le")
        return StringEncoding::Utf16Le;
    if (s == u"utf-32be")
        return StringEncoding::Utf32Be;
    if (s == u"utf-32le")
        return StringEncoding::Utf32Le;
    return Utils::ResultError(
        parseErr(u"encoding must be one of utf-8, utf-16be, utf-16le, utf-32be, utf-32le"_s, -1, idx));
}

static Utils::Result<UIntRole> parseUIntRole(const QString &s, int idx)
{
    if (s == u"packet-magic-number")
        return UIntRole::PacketMagicNumber;
    if (s == u"event-record-class-id")
        return UIntRole::EventRecordClassId;
    if (s == u"data-stream-id")
        return UIntRole::DataStreamId;
    if (s == u"data-stream-class-id")
        return UIntRole::DataStreamClassId;
    if (s == u"packet-total-length")
        return UIntRole::PacketTotalLength;
    if (s == u"packet-content-length")
        return UIntRole::PacketContentLength;
    if (s == u"default-clock-timestamp")
        return UIntRole::DefaultClockTimestamp;
    if (s == u"packet-end-default-clock-timestamp")
        return UIntRole::PacketEndDefaultClockTimestamp;
    if (s == u"discarded-event-record-counter-snapshot")
        return UIntRole::DiscardedEventRecordCounterSnapshot;
    if (s == u"packet-sequence-number")
        return UIntRole::PacketSequenceNumber;
    return Utils::ResultError(parseErr(u"unknown unsigned-integer role \"%1\""_s.arg(s), -1, idx));
}

static bool isPositivePowerOfTwo(int v)
{
    return v > 0 && (v & (v - 1)) == 0;
}

// Whether `role` may appear in the given root structure (spec 5.6.1 / 5.8.1).
static bool roleAllowedInScope(UIntRole role, RootScope scope)
{
    switch (scope) {
    case RootScope::TracePacketHeader:
        return role == UIntRole::DataStreamClassId || role == UIntRole::DataStreamId
               || role == UIntRole::PacketMagicNumber;
    case RootScope::DataStreamPacketContext:
        return role == UIntRole::DefaultClockTimestamp
               || role == UIntRole::DiscardedEventRecordCounterSnapshot
               || role == UIntRole::PacketContentLength
               || role == UIntRole::PacketEndDefaultClockTimestamp
               || role == UIntRole::PacketSequenceNumber || role == UIntRole::PacketTotalLength;
    case RootScope::EventRecordHeader:
        return role == UIntRole::DefaultClockTimestamp || role == UIntRole::EventRecordClassId;
    }
    return false;
}

static QString roleName(UIntRole role)
{
    switch (role) {
    case UIntRole::PacketMagicNumber:
        return u"packet-magic-number"_s;
    case UIntRole::EventRecordClassId:
        return u"event-record-class-id"_s;
    case UIntRole::DataStreamId:
        return u"data-stream-id"_s;
    case UIntRole::DataStreamClassId:
        return u"data-stream-class-id"_s;
    case UIntRole::PacketTotalLength:
        return u"packet-total-length"_s;
    case UIntRole::PacketContentLength:
        return u"packet-content-length"_s;
    case UIntRole::DefaultClockTimestamp:
        return u"default-clock-timestamp"_s;
    case UIntRole::PacketEndDefaultClockTimestamp:
        return u"packet-end-default-clock-timestamp"_s;
    case UIntRole::DiscardedEventRecordCounterSnapshot:
        return u"discarded-event-record-counter-snapshot"_s;
    case UIntRole::PacketSequenceNumber:
        return u"packet-sequence-number"_s;
    case UIntRole::None:
        break;
    }
    return u"(unknown)"_s;
}

static Utils::Result<void> validateRootRoles(
    const FieldClassPtr &rootFc, const Schema &schema, RootScope scope, bool dscHasClock, int idx)
{
    if (!rootFc || rootFc->kind != FieldClassKind::Structure)
        return Utils::ResultOk;
    // The packet header and event-record header additionally require every
    // contained member to be "useful" (spec 5.6 / 5.8); the packet context has
    // no such constraint.
    const bool enforceMemberUse = scope == RootScope::TracePacketHeader
                                  || scope == RootScope::EventRecordHeader;
    const auto &sfc = static_cast<const StructureFC &>(*rootFc);
    for (int i = 0; i < sfc.members.size(); ++i) {
        const FieldClass *mfc = sfc.members[i].fieldClass.get();

        // Unsigned-integer roles (roles are only parsed onto unsigned integers).
        QList<UIntRole> roles;
        int fixedLen = -1;
        bool isFixedUint = false;
        if (mfc->kind == FieldClassKind::FixedLengthUInt) {
            const auto &u = static_cast<const FixedLengthUIntFC &>(*mfc);
            roles = u.roles;
            fixedLen = u.length;
            isFixedUint = true;
        } else if (mfc->kind == FieldClassKind::VariableLengthUInt) {
            roles = static_cast<const VariableLengthUIntFC &>(*mfc).roles;
        }
        bool hasValidRole = false;
        for (UIntRole role : roles) {
            if (role == UIntRole::None)
                continue;
            // The role must be accepted in this particular root (spec 5.6.1 / 5.8.1).
            if (!roleAllowedInScope(role, scope))
                return Utils::ResultError(parseErr(
                    u"role %1 is not allowed in this root structure"_s.arg(roleName(role)),
                    -1,
                    idx));
            hasValidRole = true;
            switch (role) {
            case UIntRole::PacketMagicNumber:
                if (!isFixedUint || fixedLen != 32)
                    return Utils::ResultError(parseErr(
                        u"packet-magic-number role requires a 32-bit fixed-length unsigned integer"_s,
                        -1,
                        idx));
                if (i != 0)
                    return Utils::ResultError(parseErr(
                        u"packet-magic-number field must be the first packet header member"_s,
                        -1,
                        idx));
                break;
            case UIntRole::DefaultClockTimestamp:
            case UIntRole::PacketEndDefaultClockTimestamp:
                if (!dscHasClock)
                    return Utils::ResultError(parseErr(
                        u"clock-timestamp role requires the data stream class to have a default-clock-class-id"_s,
                        -1,
                        idx));
                break;
            default:
                break;
            }
        }

        // metadata-stream-uuid role on a static-length BLOB.
        bool hasBlobRole = false;
        if (mfc->kind == FieldClassKind::StaticLengthBlob) {
            const auto &b = static_cast<const StaticLengthBlobFC &>(*mfc);
            if (b.roles.contains(BlobRole::MetadataStreamUuid)) {
                // The metadata-stream-uuid role is only accepted in the trace
                // packet header (spec 5.6.1).
                if (scope != RootScope::TracePacketHeader)
                    return Utils::ResultError(parseErr(
                        u"metadata-stream-uuid role is only allowed in the packet header"_s,
                        -1,
                        idx));
                if (b.length != 16)
                    return Utils::ResultError(parseErr(
                        u"metadata-stream-uuid role requires a static-length-blob of length 16"_s,
                        -1,
                        idx));
                if (!schema.uuid || schema.uuid->isNull())
                    return Utils::ResultError(parseErr(
                        u"metadata-stream-uuid role requires the preamble to declare a uuid"_s,
                        -1,
                        idx));
                hasBlobRole = true;
            }
        }

        // Spec 5.6 / 5.8 "at least one of": a packet-header / event-record-header
        // member must have a valid role, be a structure/optional/variant, or be
        // an integer/boolean (which may serve as a selector or length field of an
        // optional, variant or dynamic-length field elsewhere in the same scope).
        if (enforceMemberUse && !hasValidRole && !hasBlobRole) {
            const bool ok = mfc->kind == FieldClassKind::Structure
                            || mfc->kind == FieldClassKind::Optional
                            || mfc->kind == FieldClassKind::Variant
                            || mfc->kind == FieldClassKind::FixedLengthUInt
                            || mfc->kind == FieldClassKind::FixedLengthSInt
                            || mfc->kind == FieldClassKind::FixedLengthBool
                            || mfc->kind == FieldClassKind::VariableLengthUInt
                            || mfc->kind == FieldClassKind::VariableLengthSInt;
            if (!ok)
                return Utils::ResultError(parseErr(
                    u"member '%1' has no valid role and is not a structure, optional, variant, "
                    "or potential selector/length field"_s.arg(sfc.members[i].name),
                    -1,
                    idx));
        }
    }
    return Utils::ResultOk;
}

// Parse an integer-range-set ([[lo, hi], ...]) into pairs. Returns false on a
// malformed range (wrong shape, or upper bound below lower bound).
static bool parseIntegerRangeSet(const QJsonValue &v, QList<QPair<qint64, qint64>> &out)
{
    if (!v.isArray())
        return false;
    const QJsonArray ranges = v.toArray();
    if (ranges.isEmpty())
        return false;
    for (const auto &rv : ranges) {
        if (!rv.isArray())
            return false;
        const QJsonArray r = rv.toArray();
        if (r.size() != 2)
            return false;
        // Use toInteger() (exact for the full qint64 range in Qt 6) rather than
        // toDouble(), which loses precision for integers above 2^53.
        const qint64 lo = r[0].toInteger();
        const qint64 hi = r[1].toInteger();
        if (hi < lo)
            return false;
        out.append({lo, hi});
    }
    return true;
}

static IntMappings parseMappings(const QJsonObject &obj)
{
    IntMappings mappings;
    const QJsonObject m = obj[u"mappings"].toObject();
    for (auto it = m.begin(); it != m.end(); ++it) {
        QList<QPair<qint64, qint64>> ranges;
        if (!parseIntegerRangeSet(it.value(), ranges))
            continue;
        QList<IntMappingRange> mapped;
        for (const auto &p : ranges)
            mapped.append({p.first, p.second});
        mappings.insert(it.key(), mapped);
    }
    return mappings;
}

Utils::Result<FieldLocation> MetadataReader::parseFieldLocation(const QJsonObject &obj, int idx) const
{
    FieldLocation loc;

    const QJsonValue originVal = obj[u"origin"];
    if (originVal.isString()) {
        const QString originStr = originVal.toString();
        loc.hasOrigin = true;
        if (originStr == u"packet-header")
            loc.origin = FieldLocation::Origin::PacketHeader;
        else if (originStr == u"packet-context")
            loc.origin = FieldLocation::Origin::PacketContext;
        else if (originStr == u"event-record-header")
            loc.origin = FieldLocation::Origin::EventRecordHeader;
        else if (originStr == u"event-record-common-context")
            loc.origin = FieldLocation::Origin::EventRecordCommonContext;
        else if (originStr == u"event-record-specific-context")
            loc.origin = FieldLocation::Origin::EventRecordSpecificContext;
        else if (originStr == u"event-record-payload")
            loc.origin = FieldLocation::Origin::EventRecordPayload;
        else
            return Utils::ResultError(
                parseErr(u"Invalid field-location origin: %1"_s.arg(originStr), -1, idx));
    }

    if (!obj[u"path"].isArray() || obj[u"path"].toArray().isEmpty())
        return Utils::ResultError(
            parseErr(u"field-location path must be a non-empty array"_s, -1, idx));

    const QJsonArray path = obj[u"path"].toArray();
    for (const auto &seg : path) {
        if (seg.isNull())
            loc.path.append(std::nullopt);
        else if (seg.isString())
            loc.path.append(seg.toString());
        else
            return Utils::ResultError(
                parseErr(u"field-location path elements must be strings or null"_s, -1, idx));
    }
    if (!loc.path.last().has_value())
        return Utils::ResultError(
            parseErr(u"last field-location path element must not be null"_s, -1, idx));
    return loc;
}

Utils::Result<FieldClassPtr> MetadataReader::parseFieldClass(
    const QJsonValue &val, const Schema &schema, int idx, int depth) const
{
    // Guard against deeply nested (or maliciously crafted) field classes that
    // would otherwise overflow the stack via the recursion below.
    static constexpr int kMaxFieldClassDepth = 64;
    if (depth > kMaxFieldClassDepth)
        return Utils::ResultError(parseErr(u"field class nesting is too deep"_s, -1, idx));

    // A string value is a field class alias reference.
    if (val.isString()) {
        const QString name = val.toString();
        auto it = schema.fieldClassAliases.find(name);
        if (it == schema.fieldClassAliases.end())
            return Utils::ResultError(
                parseErr(u"Unknown field class alias: %1"_s.arg(name), -1, idx));
        return *it;
    }

    const QJsonObject obj = val.toObject();
    if (auto r = checkNoExtensions(obj, idx); !r)
        return Utils::ResultError(r.error());
    const QString type = obj[u"type"].toString();

    // Common validation for fixed-length bit-array-based classes. `length` is a
    // required property (spec 5.3.4–5.3.8), so the caller MUST have verified its
    // presence before calling. This implementation represents bit-array/bit-map/
    // boolean/integer values as a 64-bit word, so lengths above 64 bits are not
    // supported (and would otherwise be silently truncated).
    auto checkLengthAlignment = [&](int length, int alignment) -> Utils::Result<void> {
        if (length <= 0)
            return Utils::ResultError(parseErr(u"length must be greater than zero"_s, -1, idx));
        if (length > 64)
            return Utils::ResultError(parseErr(
                u"fixed-length bit array lengths greater than 64 bits are not supported"_s,
                -1,
                idx));
        if (!isPositivePowerOfTwo(alignment))
            return Utils::ResultError(
                parseErr(u"alignment must be a positive power of two"_s, -1, idx));
        return Utils::ResultOk;
    };
    // `length` is required (spec): reject its absence rather than substituting a
    // fabricated default.
    auto requireLength = [&]() -> Utils::Result<void> {
        if (!obj.contains(u"length"))
            return Utils::ResultError(parseErr(u"%1 missing required length"_s.arg(type), -1, idx));
        return Utils::ResultOk;
    };

    if (type == u"fixed-length-bit-array") {
        if (auto r = requireLength(); !r)
            return Utils::ResultError(r.error());
        auto f = std::make_shared<FixedLengthBitArrayFC>();
        f->attributes = parseAttributes(obj);
        f->length = obj[u"length"].toInt(0);
        auto bo = parseByteOrder(obj, idx);
        if (!bo)
            return Utils::ResultError(bo.error());
        f->byteOrder = *bo;
        f->bitOrder = resolveBitOrder(obj, *bo);
        f->alignment = obj[u"alignment"].toInt(1);
        if (auto r = checkLengthAlignment(f->length, f->alignment); !r)
            return Utils::ResultError(r.error());
        return f;
    }
    if (type == u"fixed-length-bit-map") {
        if (auto r = requireLength(); !r)
            return Utils::ResultError(r.error());
        auto f = std::make_shared<FixedLengthBitMapFC>();
        f->attributes = parseAttributes(obj);
        f->length = obj[u"length"].toInt(0);
        auto bo = parseByteOrder(obj, idx);
        if (!bo)
            return Utils::ResultError(bo.error());
        f->byteOrder = *bo;
        f->bitOrder = resolveBitOrder(obj, *bo);
        f->alignment = obj[u"alignment"].toInt(1);
        if (auto r = checkLengthAlignment(f->length, f->alignment); !r)
            return Utils::ResultError(r.error());
        const QJsonObject flags = obj[u"flags"].toObject();
        if (flags.isEmpty())
            return Utils::ResultError(parseErr(
                u"fixed-length-bit-map flags must contain one or more properties"_s, -1, idx));
        for (auto it = flags.begin(); it != flags.end(); ++it) {
            QList<QPair<qint64, qint64>> ranges;
            if (!parseIntegerRangeSet(it.value(), ranges))
                return Utils::ResultError(
                    parseErr(u"invalid bit index range set for flag %1"_s.arg(it.key()), -1, idx));
            QList<BitFlagRange> bitRanges;
            for (const auto &p : ranges) {
                // Bit positions must fall within the field's length so they fit
                // the int BitFlagRange fields without wrapping on the cast.
                if (p.first < 0 || p.second >= f->length)
                    return Utils::ResultError(parseErr(
                        u"bit index range [%1, %2] for flag %3 is out of range [0, %4]"_s
                            .arg(p.first)
                            .arg(p.second)
                            .arg(it.key())
                            .arg(f->length - 1),
                        -1,
                        idx));
                bitRanges.append({static_cast<int>(p.first), static_cast<int>(p.second)});
            }
            f->flags.insert(it.key(), bitRanges);
        }
        return f;
    }
    if (type == u"fixed-length-boolean") {
        if (auto r = requireLength(); !r)
            return Utils::ResultError(r.error());
        auto f = std::make_shared<FixedLengthBoolFC>();
        f->attributes = parseAttributes(obj);
        f->length = obj[u"length"].toInt(0);
        auto bo = parseByteOrder(obj, idx);
        if (!bo)
            return Utils::ResultError(bo.error());
        f->byteOrder = *bo;
        f->bitOrder = resolveBitOrder(obj, *bo);
        f->alignment = obj[u"alignment"].toInt(1);
        if (auto r = checkLengthAlignment(f->length, f->alignment); !r)
            return Utils::ResultError(r.error());
        return f;
    }
    if (type == u"fixed-length-unsigned-integer") {
        if (auto r = requireLength(); !r)
            return Utils::ResultError(r.error());
        auto f = std::make_shared<FixedLengthUIntFC>();
        f->attributes = parseAttributes(obj);
        f->length = obj[u"length"].toInt(0);
        auto bo = parseByteOrder(obj, idx);
        if (!bo)
            return Utils::ResultError(bo.error());
        f->byteOrder = *bo;
        f->bitOrder = resolveBitOrder(obj, *bo);
        f->alignment = obj[u"alignment"].toInt(1);
        auto db = parseDisplayBase(obj[u"preferred-display-base"], idx);
        if (!db)
            return Utils::ResultError(db.error());
        f->displayBase = *db;
        f->mappings = parseMappings(obj);
        if (auto r = checkLengthAlignment(f->length, f->alignment); !r)
            return Utils::ResultError(r.error());
        for (const auto &rv : obj[u"roles"].toArray()) {
            auto role = parseUIntRole(rv.toString(), idx);
            if (!role)
                return Utils::ResultError(role.error());
            f->roles.append(*role);
        }
        return f;
    }
    if (type == u"fixed-length-signed-integer") {
        if (auto r = requireLength(); !r)
            return Utils::ResultError(r.error());
        auto f = std::make_shared<FixedLengthSIntFC>();
        f->attributes = parseAttributes(obj);
        f->length = obj[u"length"].toInt(0);
        auto bo = parseByteOrder(obj, idx);
        if (!bo)
            return Utils::ResultError(bo.error());
        f->byteOrder = *bo;
        f->bitOrder = resolveBitOrder(obj, *bo);
        f->alignment = obj[u"alignment"].toInt(1);
        auto db = parseDisplayBase(obj[u"preferred-display-base"], idx);
        if (!db)
            return Utils::ResultError(db.error());
        f->displayBase = *db;
        f->mappings = parseMappings(obj);
        if (auto r = checkLengthAlignment(f->length, f->alignment); !r)
            return Utils::ResultError(r.error());
        if (obj.contains(u"roles"))
            return Utils::ResultError(
                parseErr(u"roles may only exist on an unsigned integer field class"_s, -1, idx));
        return f;
    }
    if (type == u"fixed-length-floating-point-number") {
        if (auto r = requireLength(); !r)
            return Utils::ResultError(r.error());
        auto f = std::make_shared<FixedLengthFloatFC>();
        f->attributes = parseAttributes(obj);
        f->length = obj[u"length"].toInt(0);
        auto bo = parseByteOrder(obj, idx);
        if (!bo)
            return Utils::ResultError(bo.error());
        f->byteOrder = *bo;
        f->bitOrder = resolveBitOrder(obj, *bo);
        f->alignment = obj[u"alignment"].toInt(1);
        if (!isPositivePowerOfTwo(f->alignment))
            return Utils::ResultError(
                parseErr(u"alignment must be a positive power of two"_s, -1, idx));
        // Cap K well above binary256 (the largest standard IEEE float) while
        // keeping byte buffers and the writer's exponent width (w grows with
        // log2(K)) bounded against crafted metadata.
        static constexpr int kMaxFloatLength = 2048;
        const bool validLen = f->length == 16 || f->length == 32 || f->length == 64
                              || f->length == 128
                              || (f->length > 128 && f->length <= kMaxFloatLength
                                  && f->length % 32 == 0);
        if (!validLen)
            return Utils::ResultError(parseErr(
                u"floating point length must be 16, 32, 64, 128, or a multiple of 32 "
                u"between 160 and 2048"_s,
                -1,
                idx));
        return f;
    }
    if (type == u"variable-length-unsigned-integer") {
        auto f = std::make_shared<VariableLengthUIntFC>();
        f->attributes = parseAttributes(obj);
        auto db = parseDisplayBase(obj[u"preferred-display-base"], idx);
        if (!db)
            return Utils::ResultError(db.error());
        f->displayBase = *db;
        f->mappings = parseMappings(obj);
        for (const auto &rv : obj[u"roles"].toArray()) {
            auto role = parseUIntRole(rv.toString(), idx);
            if (!role)
                return Utils::ResultError(role.error());
            f->roles.append(*role);
        }
        return f;
    }
    if (type == u"variable-length-signed-integer") {
        auto f = std::make_shared<VariableLengthSIntFC>();
        f->attributes = parseAttributes(obj);
        auto db = parseDisplayBase(obj[u"preferred-display-base"], idx);
        if (!db)
            return Utils::ResultError(db.error());
        f->displayBase = *db;
        f->mappings = parseMappings(obj);
        if (obj.contains(u"roles"))
            return Utils::ResultError(
                parseErr(u"roles may only exist on an unsigned integer field class"_s, -1, idx));
        return f;
    }
    if (type == u"null-terminated-string") {
        auto f = std::make_shared<NullTerminatedStringFC>();
        f->attributes = parseAttributes(obj);
        auto enc = parseStringEncoding(obj[u"encoding"], idx);
        if (!enc)
            return Utils::ResultError(enc.error());
        f->encoding = *enc;
        return f;
    }
    if (type == u"static-length-string") {
        auto f = std::make_shared<StaticLengthStringFC>();
        f->attributes = parseAttributes(obj);
        f->length = obj[u"length"].toInteger(-1);
        if (f->length < 0 || f->length > std::numeric_limits<int>::max())
            return Utils::ResultError(parseErr(
                u"static-length-string length must be present and in [0, INT_MAX]"_s, -1, idx));
        auto enc = parseStringEncoding(obj[u"encoding"], idx);
        if (!enc)
            return Utils::ResultError(enc.error());
        f->encoding = *enc;
        return f;
    }
    if (type == u"dynamic-length-string") {
        auto f = std::make_shared<DynamicLengthStringFC>();
        f->attributes = parseAttributes(obj);
        auto locResult = parseFieldLocation(obj[u"length-field-location"].toObject(), idx);
        if (!locResult)
            return Utils::ResultError(locResult.error());
        f->lengthFieldLocation = std::move(*locResult);
        auto enc = parseStringEncoding(obj[u"encoding"], idx);
        if (!enc)
            return Utils::ResultError(enc.error());
        f->encoding = *enc;
        return f;
    }
    if (type == u"static-length-blob") {
        auto f = std::make_shared<StaticLengthBlobFC>();
        f->attributes = parseAttributes(obj);
        f->length = obj[u"length"].toInteger(-1);
        if (f->length < 0 || f->length > std::numeric_limits<int>::max())
            return Utils::ResultError(
                parseErr(u"static-length-blob length must be present and in [0, INT_MAX]"_s, -1, idx));
        f->mediaType = obj[u"media-type"].toString(u"application/octet-stream"_s);
        for (const auto &rv : obj[u"roles"].toArray()) {
            if (rv.toString() != u"metadata-stream-uuid")
                return Utils::ResultError(
                    parseErr(u"unknown BLOB role \"%1\""_s.arg(rv.toString()), -1, idx));
            f->roles.append(BlobRole::MetadataStreamUuid);
        }
        return f;
    }
    if (type == u"dynamic-length-blob") {
        auto f = std::make_shared<DynamicLengthBlobFC>();
        f->attributes = parseAttributes(obj);
        auto locResult = parseFieldLocation(obj[u"length-field-location"].toObject(), idx);
        if (!locResult)
            return Utils::ResultError(locResult.error());
        f->lengthFieldLocation = std::move(*locResult);
        f->mediaType = obj[u"media-type"].toString(u"application/octet-stream"_s);
        return f;
    }
    if (type == u"structure") {
        auto f = std::make_shared<StructureFC>();
        f->attributes = parseAttributes(obj);
        f->minimumAlignment = obj[u"minimum-alignment"].toInt(1);
        if (!isPositivePowerOfTwo(f->minimumAlignment))
            return Utils::ResultError(
                parseErr(u"minimum-alignment must be a positive power of two"_s, -1, idx));
        const QJsonArray members = obj[u"member-classes"].toArray();
        QSet<QString> memberNames;
        for (const auto &mVal : members) {
            const QJsonObject mObj = mVal.toObject();
            if (auto r = checkNoExtensions(mObj, idx); !r)
                return Utils::ResultError(r.error());
            const QString memberName = mObj[u"name"].toString();
            if (memberName.isEmpty())
                return Utils::ResultError(
                    parseErr(u"structure member class missing name"_s, -1, idx));
            if (memberNames.contains(memberName))
                return Utils::ResultError(
                    parseErr(u"duplicate structure member name: %1"_s.arg(memberName), -1, idx));
            memberNames.insert(memberName);
            if (!mObj.contains(u"field-class"))
                return Utils::ResultError(
                    parseErr(u"structure member %1 missing field-class"_s.arg(memberName), -1, idx));
            auto memberFc = parseFieldClass(mObj[u"field-class"], schema, idx, depth + 1);
            if (!memberFc)
                return Utils::ResultError(memberFc.error());
            StructureMember member;
            member.name = memberName;
            member.fieldClass = std::move(*memberFc);
            member.attributes = parseAttributes(mObj);
            f->members.append(std::move(member));
        }
        return f;
    }
    if (type == u"static-length-array") {
        auto f = std::make_shared<StaticLengthArrayFC>();
        f->attributes = parseAttributes(obj);
        if (!obj.contains(u"length"))
            return Utils::ResultError(
                parseErr(u"static-length-array missing required length"_s, -1, idx));
        f->length = obj[u"length"].toInteger(-1);
        if (f->length < 0 || f->length > std::numeric_limits<int>::max())
            return Utils::ResultError(parseErr(
                u"static-length-array length must be present and in [0, INT_MAX]"_s, -1, idx));
        f->minimumAlignment = obj[u"minimum-alignment"].toInt(1);
        if (!isPositivePowerOfTwo(f->minimumAlignment))
            return Utils::ResultError(
                parseErr(u"minimum-alignment must be a positive power of two"_s, -1, idx));
        if (!obj.contains(u"element-field-class"))
            return Utils::ResultError(
                parseErr(u"static-length-array missing element-field-class"_s, -1, idx));
        auto elem = parseFieldClass(obj[u"element-field-class"], schema, idx, depth + 1);
        if (!elem)
            return Utils::ResultError(elem.error());
        f->elementFieldClass = std::move(*elem);
        return f;
    }
    if (type == u"dynamic-length-array") {
        auto f = std::make_shared<DynamicLengthArrayFC>();
        f->attributes = parseAttributes(obj);
        f->minimumAlignment = obj[u"minimum-alignment"].toInt(1);
        if (!isPositivePowerOfTwo(f->minimumAlignment))
            return Utils::ResultError(
                parseErr(u"minimum-alignment must be a positive power of two"_s, -1, idx));
        auto locResult = parseFieldLocation(obj[u"length-field-location"].toObject(), idx);
        if (!locResult)
            return Utils::ResultError(locResult.error());
        f->lengthFieldLocation = std::move(*locResult);
        if (!obj.contains(u"element-field-class"))
            return Utils::ResultError(
                parseErr(u"dynamic-length-array missing element-field-class"_s, -1, idx));
        auto elem = parseFieldClass(obj[u"element-field-class"], schema, idx, depth + 1);
        if (!elem)
            return Utils::ResultError(elem.error());
        f->elementFieldClass = std::move(*elem);
        return f;
    }
    if (type == u"variant") {
        auto f = std::make_shared<VariantFC>();
        f->attributes = parseAttributes(obj);
        auto locResult = parseFieldLocation(obj[u"selector-field-location"].toObject(), idx);
        if (!locResult)
            return Utils::ResultError(locResult.error());
        f->selectorFieldLocation = std::move(*locResult);
        const QJsonArray options = obj[u"options"].toArray();
        if (options.isEmpty())
            return Utils::ResultError(
                parseErr(u"variant options must contain one or more elements"_s, -1, idx));
        QSet<QString> optionNames;
        // For the non-intersection check: each range tagged with its owning
        // option index, so ranges within one option are never compared (spec
        // 5.3.23 only forbids intersection between two *distinct* options).
        struct TaggedRange
        {
            int option;
            qint64 lo, hi;
        };
        QList<TaggedRange> allSelectorRanges;
        int optionIndex = 0;
        for (const auto &optVal : options) {
            const QJsonObject optObj = optVal.toObject();
            if (auto r = checkNoExtensions(optObj, idx); !r)
                return Utils::ResultError(r.error());
            VariantOption opt;
            opt.attributes = parseAttributes(optObj);
            opt.name = optObj[u"name"].toString();
            if (!opt.name.isEmpty()) {
                if (optionNames.contains(opt.name))
                    return Utils::ResultError(
                        parseErr(u"duplicate variant option name: %1"_s.arg(opt.name), -1, idx));
                optionNames.insert(opt.name);
            }
            if (!optObj.contains(u"field-class"))
                return Utils::ResultError(
                    parseErr(u"variant option missing field-class"_s, -1, idx));
            auto optFc = parseFieldClass(optObj[u"field-class"], schema, idx, depth + 1);
            if (!optFc)
                return Utils::ResultError(optFc.error());
            opt.fieldClass = std::move(*optFc);
            QList<QPair<qint64, qint64>> ranges;
            if (!parseIntegerRangeSet(optObj[u"selector-field-ranges"], ranges))
                return Utils::ResultError(
                    parseErr(u"variant option missing or invalid selector-field-ranges"_s, -1, idx));
            for (const auto &p : ranges) {
                allSelectorRanges.append({optionIndex, p.first, p.second});
                if (p.first == p.second)
                    opt.selectorValues.append(p.first);
                else
                    opt.selectorRanges.append({p.first, p.second});
            }
            f->options.append(std::move(opt));
            ++optionIndex;
        }
        // Spec 5.3.23: the integer ranges of two given options MUST NOT intersect.
        // Ranges within the same option are exempt.
        for (int a = 0; a < allSelectorRanges.size(); ++a) {
            for (int b = a + 1; b < allSelectorRanges.size(); ++b) {
                if (allSelectorRanges[a].option != allSelectorRanges[b].option
                    && allSelectorRanges[a].lo <= allSelectorRanges[b].hi
                    && allSelectorRanges[b].lo <= allSelectorRanges[a].hi)
                    return Utils::ResultError(parseErr(
                        u"variant option selector-field-ranges must not intersect"_s, -1, idx));
            }
        }
        return f;
    }
    if (type == u"optional") {
        auto f = std::make_shared<OptionalFC>();
        f->attributes = parseAttributes(obj);
        auto locResult = parseFieldLocation(obj[u"selector-field-location"].toObject(), idx);
        if (!locResult)
            return Utils::ResultError(locResult.error());
        f->selectorFieldLocation = std::move(*locResult);
        if (!obj.contains(u"field-class"))
            return Utils::ResultError(parseErr(u"optional missing field-class"_s, -1, idx));
        auto inner = parseFieldClass(obj[u"field-class"], schema, idx, depth + 1);
        if (!inner)
            return Utils::ResultError(inner.error());
        f->fieldClass = std::move(*inner);
        // selector-field-ranges is present for an integer selector, absent for
        // a boolean selector (spec 5.3.22). When present it must be a valid set.
        if (obj.contains(u"selector-field-ranges")) {
            QList<QPair<qint64, qint64>> ranges;
            if (!parseIntegerRangeSet(obj[u"selector-field-ranges"], ranges))
                return Utils::ResultError(
                    parseErr(u"invalid optional selector-field-ranges"_s, -1, idx));
            for (const auto &p : ranges) {
                if (p.first == p.second)
                    f->selectorValues.append(p.first);
                else
                    f->selectorRanges.append({p.first, p.second});
            }
        }
        return f;
    }

    return Utils::ResultError(parseErr(u"Unknown field class type: %1"_s.arg(type), -1, idx));
}

} // namespace CommonTraceFormat
