// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "metadatawriter.h"

#include "../schema/blobfieldclasses.h"
#include "../schema/compoundfieldclasses.h"
#include "../schema/scalarfieldclasses.h"
#include "../schema/stringfieldclasses.h"

#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace CommonTraceFormat {
using namespace Qt::StringLiterals;

static const char RS = '\x1E';
static const char LF = '\x0A';

// Attach the non-semantic `attributes` object (spec 5.2) if non-empty.
static void writeAttributes(QJsonObject &obj, const QJsonObject &attrs)
{
    if (!attrs.isEmpty())
        obj[u"attributes"_s] = attrs;
}

MetadataWriter::MetadataWriter(QIODevice *dev)
    : m_dev(dev)
{}

Utils::Result<> MetadataWriter::writeFragment(const QByteArray &json)
{
    // A field-class graph that exceeded the recursion guard left no usable JSON.
    if (m_fieldClassError)
        return Utils::ResultError(*m_fieldClassError);
    if (m_dev->write(&RS, 1) != 1)
        return Utils::ResultError(u"MetadataWriter: failed to write record separator"_s);
    if (m_dev->write(json) != json.size())
        return Utils::ResultError(u"MetadataWriter: failed to write fragment body"_s);
    if (m_dev->write(&LF, 1) != 1)
        return Utils::ResultError(u"MetadataWriter: failed to write line feed"_s);
    return Utils::ResultOk;
}

Utils::Result<> MetadataWriter::write(const Schema &schema)
{
    if (auto r = writePreamble(schema); !r)
        return r;

    // Emit aliases in declaration order: a later alias may reference an earlier
    // one by name (spec 5.5), and QHash iteration order is undefined. Fall back
    // to hash order for any alias missing from the order list (defensive).
    QStringList aliasNames = schema.fieldClassAliasOrder;
    for (auto it = schema.fieldClassAliases.begin(); it != schema.fieldClassAliases.end(); ++it)
        if (!aliasNames.contains(it.key()))
            aliasNames.append(it.key());
    for (const QString &name : aliasNames) {
        auto it = schema.fieldClassAliases.constFind(name);
        if (it == schema.fieldClassAliases.constEnd())
            continue;
        if (auto r = writeFieldClassAlias(name, *it.value()); !r)
            return r;
    }

    if (schema.traceClass)
        if (auto r = writeTraceClass(*schema.traceClass); !r)
            return r;

    for (const auto &cc : schema.clockClasses)
        if (auto r = writeClockClass(cc); !r)
            return r;

    for (const auto &dsc : schema.dataStreamClasses) {
        if (auto r = writeDataStreamClass(dsc); !r)
            return r;
        for (const auto &erc : dsc.eventRecordClasses)
            if (auto r = writeEventRecordClass(erc, dsc.id); !r)
                return r;
    }

    return Utils::ResultOk;
}

Utils::Result<> MetadataWriter::writePreamble(const Schema &schema)
{
    QJsonObject obj;
    obj[u"type"] = u"preamble"_s;
    obj[u"version"] = 2;
    if (schema.uuid) {
        // Spec 5.4: uuid MUST be a JSON array of 16 JSON integers.
        const QByteArray bytes = schema.uuid->toRfc4122();
        QJsonArray uuidArr;
        for (char b : bytes)
            uuidArr.append(static_cast<int>(static_cast<quint8>(b)));
        obj[u"uuid"] = uuidArr;
    }
    return writeFragment(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

Utils::Result<> MetadataWriter::writeFieldClassAlias(const QString &name, const FieldClass &fc)
{
    QJsonObject obj;
    obj[u"type"] = u"field-class-alias"_s;
    obj[u"name"] = name;
    obj[u"field-class"] = fieldClassToJson(fc);
    return writeFragment(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

Utils::Result<> MetadataWriter::writeTraceClass(const TraceClass &tc)
{
    QJsonObject obj;
    obj[u"type"] = u"trace-class"_s;
    if (!tc.namespaceName.isEmpty())
        obj[u"namespace"] = tc.namespaceName;
    if (!tc.name.isEmpty())
        obj[u"name"] = tc.name;
    if (!tc.uid.isEmpty())
        obj[u"uid"] = tc.uid;
    if (!tc.environment.isEmpty()) {
        QJsonObject env;
        for (auto it = tc.environment.cbegin(); it != tc.environment.cend(); ++it)
            env[it.key()] = it.value();
        obj[u"environment"] = env;
    }
    if (tc.packetHeaderFieldClass)
        obj[u"packet-header-field-class"] = fieldClassToJson(*tc.packetHeaderFieldClass);
    writeAttributes(obj, tc.attributes);
    return writeFragment(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

Utils::Result<> MetadataWriter::writeClockClass(const ClockClass &cc)
{
    QJsonObject obj;
    obj[u"type"] = u"clock-class"_s;
    // `id` is required for linkage; fall back to the name if unset.
    obj[u"id"] = cc.id.isEmpty() ? cc.name : cc.id;
    if (!cc.namespaceName.isEmpty())
        obj[u"namespace"] = cc.namespaceName;
    if (!cc.name.isEmpty())
        obj[u"name"] = cc.name;
    if (!cc.uid.isEmpty())
        obj[u"uid"] = cc.uid;
    obj[u"frequency"] = static_cast<qint64>(cc.frequency);
    // precision is optional: write it whenever present, including a value of 0
    // (which is distinct from "unknown"/absent, spec 5.7).
    if (cc.precision)
        obj[u"precision"] = static_cast<qint64>(*cc.precision);
    if (cc.accuracy)
        obj[u"accuracy"] = static_cast<qint64>(*cc.accuracy);
    if (!cc.description.isEmpty())
        obj[u"description"] = cc.description;

    if (cc.offsetSeconds != 0 || cc.offsetCycles != 0) {
        // Spec 5.7.2: cycles MUST be < frequency. Refuse to emit a clock-class a
        // conforming reader (including ours) would reject.
        if (cc.offsetCycles >= cc.frequency)
            return Utils::ResultError(
                u"MetadataWriter: clock-class offset cycles (%1) must be less than frequency (%2)"_s
                    .arg(cc.offsetCycles)
                    .arg(cc.frequency));
        QJsonObject offsetObj;
        offsetObj[u"seconds"] = static_cast<qint64>(cc.offsetSeconds);
        offsetObj[u"cycles"] = static_cast<qint64>(cc.offsetCycles);
        obj[u"offset-from-origin"] = offsetObj;
    }

    if (cc.origin.isUnixEpoch) {
        obj[u"origin"] = u"unix-epoch"_s;
    } else if (!cc.origin.name.isEmpty()) {
        QJsonObject originObj;
        originObj[u"name"] = cc.origin.name;
        if (!cc.origin.namespaceName.isEmpty())
            originObj[u"namespace"] = cc.origin.namespaceName;
        if (!cc.origin.uid.isEmpty())
            originObj[u"uid"] = cc.origin.uid;
        obj[u"origin"] = originObj;
    }

    writeAttributes(obj, cc.attributes);
    return writeFragment(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

Utils::Result<> MetadataWriter::writeDataStreamClass(const DataStreamClass &dsc)
{
    QJsonObject obj;
    obj[u"type"] = u"data-stream-class"_s;
    obj[u"id"] = static_cast<qint64>(dsc.id);
    if (!dsc.namespaceName.isEmpty())
        obj[u"namespace"] = dsc.namespaceName;
    if (!dsc.name.isEmpty())
        obj[u"name"] = dsc.name;
    if (!dsc.uid.isEmpty())
        obj[u"uid"] = dsc.uid;
    if (!dsc.defaultClockClassName.isEmpty())
        obj[u"default-clock-class-id"] = dsc.defaultClockClassName;
    if (dsc.packetContextFieldClass)
        obj[u"packet-context-field-class"] = fieldClassToJson(*dsc.packetContextFieldClass);
    if (dsc.eventRecordHeaderFieldClass)
        obj[u"event-record-header-field-class"] = fieldClassToJson(*dsc.eventRecordHeaderFieldClass);
    if (dsc.eventRecordCommonContextFieldClass)
        obj[u"event-record-common-context-field-class"] = fieldClassToJson(
            *dsc.eventRecordCommonContextFieldClass);
    writeAttributes(obj, dsc.attributes);
    return writeFragment(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

Utils::Result<> MetadataWriter::writeEventRecordClass(
    const EventRecordClass &erc, quint64 dataStreamClassId)
{
    QJsonObject obj;
    obj[u"type"] = u"event-record-class"_s;
    obj[u"id"] = static_cast<qint64>(erc.id);
    obj[u"data-stream-class-id"] = static_cast<qint64>(dataStreamClassId);
    if (!erc.namespaceName.isEmpty())
        obj[u"namespace"] = erc.namespaceName;
    if (!erc.name.isEmpty())
        obj[u"name"] = erc.name;
    if (!erc.uid.isEmpty())
        obj[u"uid"] = erc.uid;
    if (erc.specificContextFieldClass)
        obj[u"specific-context-field-class"] = fieldClassToJson(*erc.specificContextFieldClass);
    if (erc.payloadFieldClass)
        obj[u"payload-field-class"] = fieldClassToJson(*erc.payloadFieldClass);
    writeAttributes(obj, erc.attributes);
    return writeFragment(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

static QString byteOrderStr(ByteOrder bo)
{
    return bo == ByteOrder::BigEndian ? u"big-endian"_s : u"little-endian"_s;
}

static QString bitOrderStr(BitOrder bo)
{
    // Spec 5.3.4: the first decoded bit is element 0. LeastSignificantFirst maps
    // it to "first-to-last"; MostSignificantFirst to "last-to-first".
    return bo == BitOrder::MostSignificantFirst ? u"last-to-first"_s : u"first-to-last"_s;
}

static int displayBaseInt(DisplayBase db)
{
    switch (db) {
    case DisplayBase::Binary:
        return 2;
    case DisplayBase::Octal:
        return 8;
    case DisplayBase::Hexadecimal:
        return 16;
    default:
        return 10;
    }
}

static QString encodingStr(StringEncoding enc)
{
    switch (enc) {
    case StringEncoding::Utf16Be:
        return u"utf-16be"_s;
    case StringEncoding::Utf16Le:
        return u"utf-16le"_s;
    case StringEncoding::Utf32Be:
        return u"utf-32be"_s;
    case StringEncoding::Utf32Le:
        return u"utf-32le"_s;
    default:
        return u"utf-8"_s;
    }
}

// Serialize integer field-class mappings (spec 5.3.7.1): name → integer-range-set.
static QJsonObject intMappingsToJson(const IntMappings &mappings)
{
    QJsonObject out;
    for (auto it = mappings.cbegin(); it != mappings.cend(); ++it) {
        QJsonArray rangeSet;
        for (const auto &r : it.value()) {
            QJsonArray range;
            range.append(static_cast<qint64>(r.lower));
            range.append(static_cast<qint64>(r.upper));
            rangeSet.append(range);
        }
        out[it.key()] = rangeSet;
    }
    return out;
}

// Serialize discrete selector values and ranges as an integer-range-set.
static QJsonArray selectorRangeSet(
    const QList<qint64> &values, const QList<VariantSelectorRange> &ranges)
{
    QJsonArray out;
    for (auto v : values) {
        QJsonArray r;
        r.append(static_cast<qint64>(v));
        r.append(static_cast<qint64>(v));
        out.append(r);
    }
    for (const auto &rng : ranges) {
        QJsonArray r;
        r.append(static_cast<qint64>(rng.lo));
        r.append(static_cast<qint64>(rng.hi));
        out.append(r);
    }
    return out;
}

static QString uintRoleStr(UIntRole r)
{
    switch (r) {
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
    default:
        return {};
    }
}

static QString originStr(FieldLocation::Origin o)
{
    switch (o) {
    case FieldLocation::Origin::PacketHeader:
        return u"packet-header"_s;
    case FieldLocation::Origin::PacketContext:
        return u"packet-context"_s;
    case FieldLocation::Origin::EventRecordHeader:
        return u"event-record-header"_s;
    case FieldLocation::Origin::EventRecordCommonContext:
        return u"event-record-common-context"_s;
    case FieldLocation::Origin::EventRecordSpecificContext:
        return u"event-record-specific-context"_s;
    default:
        return u"event-record-payload"_s;
    }
}

QJsonObject MetadataWriter::fieldLocationToJson(const FieldLocation &loc) const
{
    QJsonObject obj;
    if (loc.hasOrigin)
        obj[u"origin"] = originStr(loc.origin);
    QJsonArray path;
    for (const auto &seg : loc.path)
        path.append(seg ? QJsonValue(*seg) : QJsonValue(QJsonValue::Null));
    obj[u"path"] = path;
    return obj;
}

QJsonObject MetadataWriter::fieldClassToJson(const FieldClass &fc, int depth) const
{
    // Guard against an over-deep (or cyclic) field-class graph that would
    // otherwise overflow the stack via the recursion below. Mirrors the reader's
    // kMaxFieldClassDepth limit.
    static constexpr int kMaxFieldClassDepth = 64;
    if (depth > kMaxFieldClassDepth) {
        if (!m_fieldClassError)
            m_fieldClassError = u"MetadataWriter: field class nesting is too deep"_s;
        return {};
    }

    QJsonObject obj;

    switch (fc.kind) {
    case FieldClassKind::FixedLengthBitArray: {
        const auto &f = static_cast<const FixedLengthBitArrayFC &>(fc);
        obj[u"type"] = u"fixed-length-bit-array"_s;
        obj[u"length"] = f.length;
        obj[u"byte-order"] = byteOrderStr(f.byteOrder);
        obj[u"bit-order"] = bitOrderStr(f.bitOrder);
        obj[u"alignment"] = f.alignment;
        break;
    }
    case FieldClassKind::FixedLengthBitMap: {
        const auto &f = static_cast<const FixedLengthBitMapFC &>(fc);
        obj[u"type"] = u"fixed-length-bit-map"_s;
        obj[u"length"] = f.length;
        obj[u"byte-order"] = byteOrderStr(f.byteOrder);
        obj[u"bit-order"] = bitOrderStr(f.bitOrder);
        obj[u"alignment"] = f.alignment;
        QJsonObject flags;
        for (auto it = f.flags.begin(); it != f.flags.end(); ++it) {
            QJsonArray rangeSet;
            for (const auto &br : it.value()) {
                QJsonArray range;
                range.append(br.start);
                range.append(br.end);
                rangeSet.append(range);
            }
            flags[it.key()] = rangeSet;
        }
        obj[u"flags"] = flags;
        break;
    }
    case FieldClassKind::FixedLengthBool: {
        const auto &f = static_cast<const FixedLengthBoolFC &>(fc);
        obj[u"type"] = u"fixed-length-boolean"_s;
        obj[u"length"] = f.length;
        obj[u"byte-order"] = byteOrderStr(f.byteOrder);
        obj[u"bit-order"] = bitOrderStr(f.bitOrder);
        obj[u"alignment"] = f.alignment;
        break;
    }
    case FieldClassKind::FixedLengthUInt: {
        const auto &f = static_cast<const FixedLengthUIntFC &>(fc);
        obj[u"type"] = u"fixed-length-unsigned-integer"_s;
        obj[u"length"] = f.length;
        obj[u"byte-order"] = byteOrderStr(f.byteOrder);
        obj[u"bit-order"] = bitOrderStr(f.bitOrder);
        obj[u"alignment"] = f.alignment;
        obj[u"preferred-display-base"] = displayBaseInt(f.displayBase);
        if (!f.mappings.isEmpty())
            obj[u"mappings"] = intMappingsToJson(f.mappings);
        if (!f.roles.isEmpty()) {
            QJsonArray roles;
            for (auto r : f.roles)
                roles.append(uintRoleStr(r));
            obj[u"roles"] = roles;
        }
        break;
    }
    case FieldClassKind::FixedLengthSInt: {
        const auto &f = static_cast<const FixedLengthSIntFC &>(fc);
        obj[u"type"] = u"fixed-length-signed-integer"_s;
        obj[u"length"] = f.length;
        obj[u"byte-order"] = byteOrderStr(f.byteOrder);
        obj[u"bit-order"] = bitOrderStr(f.bitOrder);
        obj[u"alignment"] = f.alignment;
        obj[u"preferred-display-base"] = displayBaseInt(f.displayBase);
        if (!f.mappings.isEmpty())
            obj[u"mappings"] = intMappingsToJson(f.mappings);
        break;
    }
    case FieldClassKind::FixedLengthFloat: {
        const auto &f = static_cast<const FixedLengthFloatFC &>(fc);
        obj[u"type"] = u"fixed-length-floating-point-number"_s;
        obj[u"length"] = f.length;
        obj[u"byte-order"] = byteOrderStr(f.byteOrder);
        obj[u"bit-order"] = bitOrderStr(f.bitOrder);
        obj[u"alignment"] = f.alignment;
        break;
    }
    case FieldClassKind::VariableLengthUInt: {
        const auto &f = static_cast<const VariableLengthUIntFC &>(fc);
        obj[u"type"] = u"variable-length-unsigned-integer"_s;
        obj[u"preferred-display-base"] = displayBaseInt(f.displayBase);
        if (!f.mappings.isEmpty())
            obj[u"mappings"] = intMappingsToJson(f.mappings);
        if (!f.roles.isEmpty()) {
            QJsonArray roles;
            for (auto r : f.roles)
                roles.append(uintRoleStr(r));
            obj[u"roles"] = roles;
        }
        break;
    }
    case FieldClassKind::VariableLengthSInt: {
        const auto &f = static_cast<const VariableLengthSIntFC &>(fc);
        obj[u"type"] = u"variable-length-signed-integer"_s;
        obj[u"preferred-display-base"] = displayBaseInt(f.displayBase);
        if (!f.mappings.isEmpty())
            obj[u"mappings"] = intMappingsToJson(f.mappings);
        break;
    }
    case FieldClassKind::NullTerminatedString: {
        const auto &f = static_cast<const NullTerminatedStringFC &>(fc);
        obj[u"type"] = u"null-terminated-string"_s;
        obj[u"encoding"] = encodingStr(f.encoding);
        break;
    }
    case FieldClassKind::StaticLengthString: {
        const auto &f = static_cast<const StaticLengthStringFC &>(fc);
        obj[u"type"] = u"static-length-string"_s;
        obj[u"length"] = f.length;
        obj[u"encoding"] = encodingStr(f.encoding);
        break;
    }
    case FieldClassKind::DynamicLengthString: {
        const auto &f = static_cast<const DynamicLengthStringFC &>(fc);
        obj[u"type"] = u"dynamic-length-string"_s;
        obj[u"length-field-location"] = fieldLocationToJson(f.lengthFieldLocation);
        obj[u"encoding"] = encodingStr(f.encoding);
        break;
    }
    case FieldClassKind::StaticLengthBlob: {
        const auto &f = static_cast<const StaticLengthBlobFC &>(fc);
        obj[u"type"] = u"static-length-blob"_s;
        obj[u"length"] = f.length;
        obj[u"media-type"] = f.mediaType;
        if (!f.roles.isEmpty()) {
            QJsonArray roles;
            for (auto r : f.roles) {
                if (r == BlobRole::MetadataStreamUuid)
                    roles.append(u"metadata-stream-uuid"_s);
            }
            if (!roles.isEmpty())
                obj[u"roles"] = roles;
        }
        break;
    }
    case FieldClassKind::DynamicLengthBlob: {
        const auto &f = static_cast<const DynamicLengthBlobFC &>(fc);
        obj[u"type"] = u"dynamic-length-blob"_s;
        obj[u"length-field-location"] = fieldLocationToJson(f.lengthFieldLocation);
        obj[u"media-type"] = f.mediaType;
        break;
    }
    case FieldClassKind::Structure: {
        const auto &f = static_cast<const StructureFC &>(fc);
        obj[u"type"] = u"structure"_s;
        obj[u"minimum-alignment"] = f.minimumAlignment;
        QJsonArray members;
        for (const auto &m : f.members) {
            QJsonObject mObj;
            mObj[u"name"] = m.name;
            mObj[u"field-class"] = fieldClassToJson(*m.fieldClass, depth + 1);
            writeAttributes(mObj, m.attributes);
            members.append(mObj);
        }
        obj[u"member-classes"] = members;
        break;
    }
    case FieldClassKind::StaticLengthArray: {
        const auto &f = static_cast<const StaticLengthArrayFC &>(fc);
        obj[u"type"] = u"static-length-array"_s;
        obj[u"length"] = f.length;
        obj[u"minimum-alignment"] = f.minimumAlignment;
        obj[u"element-field-class"] = fieldClassToJson(*f.elementFieldClass, depth + 1);
        break;
    }
    case FieldClassKind::DynamicLengthArray: {
        const auto &f = static_cast<const DynamicLengthArrayFC &>(fc);
        obj[u"type"] = u"dynamic-length-array"_s;
        obj[u"length-field-location"] = fieldLocationToJson(f.lengthFieldLocation);
        obj[u"minimum-alignment"] = f.minimumAlignment;
        obj[u"element-field-class"] = fieldClassToJson(*f.elementFieldClass, depth + 1);
        break;
    }
    case FieldClassKind::Variant: {
        const auto &f = static_cast<const VariantFC &>(fc);
        obj[u"type"] = u"variant"_s;
        obj[u"selector-field-location"] = fieldLocationToJson(f.selectorFieldLocation);
        QJsonArray options;
        for (const auto &opt : f.options) {
            QJsonObject optObj;
            if (!opt.name.isEmpty())
                optObj[u"name"] = opt.name;
            optObj[u"field-class"] = fieldClassToJson(*opt.fieldClass, depth + 1);
            optObj[u"selector-field-ranges"]
                = selectorRangeSet(opt.selectorValues, opt.selectorRanges);
            writeAttributes(optObj, opt.attributes);
            options.append(optObj);
        }
        obj[u"options"] = options;
        break;
    }
    case FieldClassKind::Optional: {
        const auto &f = static_cast<const OptionalFC &>(fc);
        obj[u"type"] = u"optional"_s;
        obj[u"field-class"] = fieldClassToJson(*f.fieldClass, depth + 1);
        obj[u"selector-field-location"] = fieldLocationToJson(f.selectorFieldLocation);
        if (!f.selectorValues.isEmpty() || !f.selectorRanges.isEmpty())
            obj[u"selector-field-ranges"] = selectorRangeSet(f.selectorValues, f.selectorRanges);
        break;
    }
    }

    writeAttributes(obj, fc.attributes);
    return obj;
}

} // namespace CommonTraceFormat
