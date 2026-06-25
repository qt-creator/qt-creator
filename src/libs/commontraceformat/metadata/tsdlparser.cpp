// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tsdlparser.h"

#include "../binary/bitbuffer.h"
#include "../schema/blobfieldclasses.h"
#include "../schema/clockclass.h"
#include "../schema/compoundfieldclasses.h"
#include "../schema/datastreamclass.h"
#include "../schema/eventrecordclass.h"
#include "../schema/scalarfieldclasses.h"
#include "../schema/stringfieldclasses.h"
#include "../schema/traceclass.h"

#include <utils/result.h>

#include <QHash>
#include <QScopeGuard>
#include <QSet>
#include <QUuid>

#include <cctype>
#include <functional>
#include <limits>

namespace CommonTraceFormat {
using namespace Qt::StringLiterals;

// ── Lexer ──────────────────────────────────────────────────────────────────

enum class TT {
    Ident,
    IntLit,
    StrLit,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    LParen,
    RParen,
    Semi,
    Colon,
    Equals,
    Comma,
    Dot,
    Star,
    Lt,
    Gt,
    ColonEq,   // :=
    DotDotDot, // ...
    Eof,
    Error,
};

struct Tok
{
    TT type = TT::Error;
    QString text;
    qint64 intVal = 0;
};

class Lexer
{
public:
    explicit Lexer(const QByteArray &src)
        : m_pos(src.constData())
        , m_end(m_pos + src.size())
    {
        advance();
    }

    Tok peek() const { return m_current; }
    Tok next()
    {
        Tok t = m_current;
        advance();
        return t;
    }
    bool at(TT t) const { return m_current.type == t; }
    bool atIdent(const char *s) const
    {
        return m_current.type == TT::Ident && m_current.text == QLatin1StringView(s);
    }

    // First lexical error encountered (malformed literal), empty if none. Set as
    // tokens are produced — even for tokens in skipped regions — so the parser
    // can reject a malformed literal it would otherwise step over.
    const QString &error() const { return m_error; }

private:
    void skipWhitespace()
    {
        while (m_pos < m_end) {
            if (*m_pos == '/' && m_pos + 1 < m_end && m_pos[1] == '*') {
                m_pos += 2;
                while (m_pos + 1 < m_end && !(m_pos[0] == '*' && m_pos[1] == '/'))
                    ++m_pos;
                if (m_pos + 1 < m_end)
                    m_pos += 2; // consume the closing */
                else
                    m_pos = m_end; // unterminated /*: consume to end, don't spin
            } else if (*m_pos == '/' && m_pos + 1 < m_end && m_pos[1] == '/') {
                while (m_pos < m_end && *m_pos != '\n')
                    ++m_pos;
            } else if (std::isspace(static_cast<unsigned char>(*m_pos))) {
                ++m_pos;
            } else {
                break;
            }
        }
    }

    void advance()
    {
        skipWhitespace();
        if (m_pos >= m_end) {
            m_current = {.type = TT::Eof, .text = {}};
            return;
        }
        const char c = *m_pos;

        if (c == '"') {
            ++m_pos;
            QByteArray s;
            bool hadNul = false;
            while (m_pos < m_end && *m_pos != '"') {
                if (*m_pos == '\0')
                    hadNul = true; // a NUL cannot appear inside a TSDL string
                if (*m_pos == '\\' && m_pos + 1 < m_end)
                    ++m_pos;
                s.append(*m_pos++);
            }
            if (m_pos < m_end)
                ++m_pos;
            // Consume the whole literal either way so tokenization stays aligned,
            // but a NUL makes it a fatal error rather than a string token.
            if (hadNul) {
                m_current = makeError(u"NUL byte inside string literal"_s);
                return;
            }
            m_current = {.type = TT::StrLit, .text = QString::fromUtf8(s), .intVal = 0};
            return;
        }

        if (std::isdigit(static_cast<unsigned char>(c))
            || (c == '-' && m_pos + 1 < m_end
                && std::isdigit(static_cast<unsigned char>(m_pos[1])))) {
            bool neg = (c == '-');
            if (neg)
                ++m_pos;
            int base = 10;
            if (m_pos + 1 < m_end && *m_pos == '0' && (m_pos[1] == 'x' || m_pos[1] == 'X')) {
                base = 16;
                m_pos += 2;
            } else if (m_pos + 1 < m_end && *m_pos == '0' && m_pos[1] >= '0' && m_pos[1] <= '7') {
                // C-style octal: a leading 0 followed by octal digits (a lone 0,
                // or 0 followed by a non-octal char, stays base 10 → value 0).
                base = 8;
                ++m_pos; // consume the leading 0; the octal digits follow
            }
            // Consume only digits valid for the base; a trailing non-digit would
            // otherwise be swallowed and decoded as 0 with the error flag ignored.
            QByteArray digits;
            const auto isDigitForBase = [base](unsigned char ch) {
                if (base == 16)
                    return std::isxdigit(ch) != 0;
                if (base == 8)
                    return ch >= '0' && ch <= '7';
                return std::isdigit(ch) != 0;
            };
            while (m_pos < m_end && isDigitForBase(static_cast<unsigned char>(*m_pos)))
                digits.append(*m_pos++);
            bool ok = false;
            quint64 uval = digits.toULongLong(&ok, base);
            if (!ok) {
                m_current = makeError(
                    u"integer literal '%1' is malformed or out of range"_s.arg(
                        QString::fromLatin1(digits)));
                return;
            }
            // Guard the unsigned→signed conversion: a value beyond the signed
            // range (or negating LLONG_MIN's magnitude) would be undefined.
            constexpr quint64 maxMag = static_cast<quint64>(std::numeric_limits<qint64>::max());
            qint64 val;
            if (neg) {
                if (uval > maxMag + 1) {
                    m_current = makeError(
                        u"integer literal '%1' is out of range"_s.arg(QString::fromLatin1(digits)));
                    return;
                }
                val = (uval == maxMag + 1) ? std::numeric_limits<qint64>::min()
                                           : -static_cast<qint64>(uval);
            } else {
                if (uval > maxMag) {
                    m_current = makeError(
                        u"integer literal '%1' is out of range"_s.arg(QString::fromLatin1(digits)));
                    return;
                }
                val = static_cast<qint64>(uval);
            }
            m_current = {.type = TT::IntLit, .text = QString::fromLatin1(digits), .intVal = val};
            return;
        }

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            QByteArray id;
            while (m_pos < m_end
                   && (std::isalnum(static_cast<unsigned char>(*m_pos)) || *m_pos == '_'))
                id.append(*m_pos++);
            m_current = {.type = TT::Ident, .text = QString::fromLatin1(id), .intVal = 0};
            return;
        }

        if (c == ':' && m_pos + 1 < m_end && m_pos[1] == '=') {
            m_pos += 2;
            m_current = {.type = TT::ColonEq, .text = {}};
            return;
        }
        if (c == '.' && m_pos + 2 < m_end && m_pos[1] == '.' && m_pos[2] == '.') {
            m_pos += 3;
            m_current = {.type = TT::DotDotDot, .text = {}};
            return;
        }

        ++m_pos;
        switch (c) {
        case '{':
            m_current = {.type = TT::LBrace, .text = {}};
            return;
        case '}':
            m_current = {.type = TT::RBrace, .text = {}};
            return;
        case '[':
            m_current = {.type = TT::LBracket, .text = {}};
            return;
        case ']':
            m_current = {.type = TT::RBracket, .text = {}};
            return;
        case '(':
            m_current = {.type = TT::LParen, .text = {}};
            return;
        case ')':
            m_current = {.type = TT::RParen, .text = {}};
            return;
        case ';':
            m_current = {.type = TT::Semi, .text = {}};
            return;
        case ':':
            m_current = {.type = TT::Colon, .text = {}};
            return;
        case '=':
            m_current = {.type = TT::Equals, .text = {}};
            return;
        case ',':
            m_current = {.type = TT::Comma, .text = {}};
            return;
        case '.':
            m_current = {.type = TT::Dot, .text = {}};
            return;
        case '*':
            m_current = {.type = TT::Star, .text = {}};
            return;
        case '<':
            m_current = {.type = TT::Lt, .text = {}};
            return;
        case '>':
            m_current = {.type = TT::Gt, .text = {}};
            return;
        default:
            // An untokenizable character is not fatal on its own: valid traces
            // carry such bytes (e.g. char literals, '+') inside attributes this
            // minimal lexer ignores, so emit a plain Error token that skipToSemi
            // steps over. Only makeError() (malformed literals) is fatal.
            m_current = {.type = TT::Error, .text = {}};
            return;
        }
    }

    // Record the first *fatal* lexical error (a malformed literal that a valid
    // trace cannot contain) and return an Error token to emit for it.
    Tok makeError(const QString &msg)
    {
        if (m_error.isEmpty())
            m_error = msg;
        return {.type = TT::Error, .text = {}, .intVal = 0};
    }

    const char *m_pos;
    const char *m_end;
    Tok m_current;
    QString m_error;
};

// ── Type factory ────────────────────────────────────────────────────────────
// Creates a fresh FieldClassPtr each call (avoids sharing modifiable state).

using TypeFactory = std::function<FieldClassPtr()>;

// ── Helpers ─────────────────────────────────────────────────────────────────

// Walk a FieldClass tree and update all FieldLocation origins.
static void updateOrigins(FieldClass *fc, FieldLocation::Origin origin)
{
    if (!fc)
        return;
    switch (fc->kind) {
    case FieldClassKind::Structure: {
        auto &s = static_cast<StructureFC &>(*fc);
        for (auto &m : s.members)
            updateOrigins(m.fieldClass.get(), origin);
        break;
    }
    case FieldClassKind::StaticLengthArray:
        updateOrigins(static_cast<StaticLengthArrayFC &>(*fc).elementFieldClass.get(), origin);
        break;
    case FieldClassKind::DynamicLengthArray: {
        auto &a = static_cast<DynamicLengthArrayFC &>(*fc);
        a.lengthFieldLocation.origin = origin;
        updateOrigins(a.elementFieldClass.get(), origin);
        break;
    }
    case FieldClassKind::Variant: {
        auto &v = static_cast<VariantFC &>(*fc);
        v.selectorFieldLocation.origin = origin;
        for (auto &opt : v.options)
            updateOrigins(opt.fieldClass.get(), origin);
        break;
    }
    case FieldClassKind::Optional: {
        auto &o = static_cast<OptionalFC &>(*fc);
        o.selectorFieldLocation.origin = origin;
        updateOrigins(o.fieldClass.get(), origin);
        break;
    }
    default:
        break;
    }
}

// Deep-clone a field-class tree. A named TSDL struct/typealias may be referenced
// from several scopes (spec 7.3.1), each of which assigns its own field-location
// origins (spec 7.3.2); sharing one instance would let a later reference's
// updateOrigins()/role assignment overwrite an earlier one's. Each reference
// therefore gets an independent copy. The FieldClass base copy constructor is
// deleted, so members are copied explicitly.
static FieldClassPtr cloneFieldClass(const FieldClassPtr &fc)
{
    if (!fc)
        return nullptr;

    switch (fc->kind) {
    case FieldClassKind::FixedLengthBitArray: {
        const auto &s = static_cast<const FixedLengthBitArrayFC &>(*fc);
        auto c = std::make_shared<FixedLengthBitArrayFC>();
        c->attributes = s.attributes;
        c->length = s.length;
        c->byteOrder = s.byteOrder;
        c->bitOrder = s.bitOrder;
        c->alignment = s.alignment;
        return c;
    }
    case FieldClassKind::FixedLengthBitMap: {
        const auto &s = static_cast<const FixedLengthBitMapFC &>(*fc);
        auto c = std::make_shared<FixedLengthBitMapFC>();
        c->attributes = s.attributes;
        c->length = s.length;
        c->byteOrder = s.byteOrder;
        c->bitOrder = s.bitOrder;
        c->alignment = s.alignment;
        c->flags = s.flags;
        return c;
    }
    case FieldClassKind::FixedLengthBool: {
        const auto &s = static_cast<const FixedLengthBoolFC &>(*fc);
        auto c = std::make_shared<FixedLengthBoolFC>();
        c->attributes = s.attributes;
        c->length = s.length;
        c->byteOrder = s.byteOrder;
        c->bitOrder = s.bitOrder;
        c->alignment = s.alignment;
        return c;
    }
    case FieldClassKind::FixedLengthUInt: {
        const auto &s = static_cast<const FixedLengthUIntFC &>(*fc);
        auto c = std::make_shared<FixedLengthUIntFC>();
        c->attributes = s.attributes;
        c->length = s.length;
        c->byteOrder = s.byteOrder;
        c->bitOrder = s.bitOrder;
        c->alignment = s.alignment;
        c->displayBase = s.displayBase;
        c->mappings = s.mappings;
        c->roles = s.roles;
        return c;
    }
    case FieldClassKind::FixedLengthSInt: {
        const auto &s = static_cast<const FixedLengthSIntFC &>(*fc);
        auto c = std::make_shared<FixedLengthSIntFC>();
        c->attributes = s.attributes;
        c->length = s.length;
        c->byteOrder = s.byteOrder;
        c->bitOrder = s.bitOrder;
        c->alignment = s.alignment;
        c->displayBase = s.displayBase;
        c->mappings = s.mappings;
        return c;
    }
    case FieldClassKind::FixedLengthFloat: {
        const auto &s = static_cast<const FixedLengthFloatFC &>(*fc);
        auto c = std::make_shared<FixedLengthFloatFC>();
        c->attributes = s.attributes;
        c->length = s.length;
        c->byteOrder = s.byteOrder;
        c->bitOrder = s.bitOrder;
        c->alignment = s.alignment;
        return c;
    }
    case FieldClassKind::VariableLengthUInt: {
        const auto &s = static_cast<const VariableLengthUIntFC &>(*fc);
        auto c = std::make_shared<VariableLengthUIntFC>();
        c->attributes = s.attributes;
        c->displayBase = s.displayBase;
        c->mappings = s.mappings;
        c->roles = s.roles;
        return c;
    }
    case FieldClassKind::VariableLengthSInt: {
        const auto &s = static_cast<const VariableLengthSIntFC &>(*fc);
        auto c = std::make_shared<VariableLengthSIntFC>();
        c->attributes = s.attributes;
        c->displayBase = s.displayBase;
        c->mappings = s.mappings;
        return c;
    }
    case FieldClassKind::NullTerminatedString: {
        const auto &s = static_cast<const NullTerminatedStringFC &>(*fc);
        auto c = std::make_shared<NullTerminatedStringFC>();
        c->attributes = s.attributes;
        c->encoding = s.encoding;
        return c;
    }
    case FieldClassKind::StaticLengthString: {
        const auto &s = static_cast<const StaticLengthStringFC &>(*fc);
        auto c = std::make_shared<StaticLengthStringFC>();
        c->attributes = s.attributes;
        c->length = s.length;
        c->encoding = s.encoding;
        return c;
    }
    case FieldClassKind::DynamicLengthString: {
        const auto &s = static_cast<const DynamicLengthStringFC &>(*fc);
        auto c = std::make_shared<DynamicLengthStringFC>();
        c->attributes = s.attributes;
        c->lengthFieldLocation = s.lengthFieldLocation;
        c->encoding = s.encoding;
        return c;
    }
    case FieldClassKind::StaticLengthBlob: {
        const auto &s = static_cast<const StaticLengthBlobFC &>(*fc);
        auto c = std::make_shared<StaticLengthBlobFC>();
        c->attributes = s.attributes;
        c->length = s.length;
        c->mediaType = s.mediaType;
        c->roles = s.roles;
        return c;
    }
    case FieldClassKind::DynamicLengthBlob: {
        const auto &s = static_cast<const DynamicLengthBlobFC &>(*fc);
        auto c = std::make_shared<DynamicLengthBlobFC>();
        c->attributes = s.attributes;
        c->lengthFieldLocation = s.lengthFieldLocation;
        c->mediaType = s.mediaType;
        return c;
    }
    case FieldClassKind::Structure: {
        const auto &s = static_cast<const StructureFC &>(*fc);
        auto c = std::make_shared<StructureFC>();
        c->attributes = s.attributes;
        c->minimumAlignment = s.minimumAlignment;
        for (const auto &m : s.members)
            c->members.append({m.name, cloneFieldClass(m.fieldClass), m.attributes});
        return c;
    }
    case FieldClassKind::StaticLengthArray: {
        const auto &s = static_cast<const StaticLengthArrayFC &>(*fc);
        auto c = std::make_shared<StaticLengthArrayFC>();
        c->attributes = s.attributes;
        c->length = s.length;
        c->minimumAlignment = s.minimumAlignment;
        c->elementFieldClass = cloneFieldClass(s.elementFieldClass);
        return c;
    }
    case FieldClassKind::DynamicLengthArray: {
        const auto &s = static_cast<const DynamicLengthArrayFC &>(*fc);
        auto c = std::make_shared<DynamicLengthArrayFC>();
        c->attributes = s.attributes;
        c->lengthFieldLocation = s.lengthFieldLocation;
        c->minimumAlignment = s.minimumAlignment;
        c->elementFieldClass = cloneFieldClass(s.elementFieldClass);
        return c;
    }
    case FieldClassKind::Variant: {
        const auto &s = static_cast<const VariantFC &>(*fc);
        auto c = std::make_shared<VariantFC>();
        c->attributes = s.attributes;
        c->selectorFieldLocation = s.selectorFieldLocation;
        for (const auto &opt : s.options) {
            VariantOption o;
            o.selectorValues = opt.selectorValues;
            o.selectorRanges = opt.selectorRanges;
            o.name = opt.name;
            o.attributes = opt.attributes;
            o.fieldClass = cloneFieldClass(opt.fieldClass);
            c->options.append(std::move(o));
        }
        return c;
    }
    case FieldClassKind::Optional: {
        const auto &s = static_cast<const OptionalFC &>(*fc);
        auto c = std::make_shared<OptionalFC>();
        c->attributes = s.attributes;
        c->selectorFieldLocation = s.selectorFieldLocation;
        c->selectorValues = s.selectorValues;
        c->selectorRanges = s.selectorRanges;
        c->fieldClass = cloneFieldClass(s.fieldClass);
        return c;
    }
    }
    return nullptr;
}

static void assignPacketContextRoles(FieldClass *fc)
{
    if (!fc || fc->kind != FieldClassKind::Structure)
        return;
    for (auto &m : static_cast<StructureFC &>(*fc).members) {
        if (!m.fieldClass || m.fieldClass->kind != FieldClassKind::FixedLengthUInt)
            continue;
        auto &u = static_cast<FixedLengthUIntFC &>(*m.fieldClass);
        if (m.name == u"content_size"_s && !u.roles.contains(UIntRole::PacketContentLength))
            u.roles.append(UIntRole::PacketContentLength);
        else if (m.name == u"packet_size"_s && !u.roles.contains(UIntRole::PacketTotalLength))
            u.roles.append(UIntRole::PacketTotalLength);
        else if (m.name == u"packet_seq_num"_s && !u.roles.contains(UIntRole::PacketSequenceNumber))
            u.roles.append(UIntRole::PacketSequenceNumber);
        else if (
            m.name == u"events_discarded"_s
            && !u.roles.contains(UIntRole::DiscardedEventRecordCounterSnapshot))
            u.roles.append(UIntRole::DiscardedEventRecordCounterSnapshot);
        // timestamp_begin seeds the default clock for the packet; timestamp_end
        // bounds it (CTF 1.8 packet.context). Without these the packet-begin
        // clock value is never established and partial event-header timestamps
        // extend against 0, yielding wrong absolute times.
        else if (m.name == u"timestamp_begin"_s && !u.roles.contains(UIntRole::DefaultClockTimestamp))
            u.roles.append(UIntRole::DefaultClockTimestamp);
        else if (
            m.name == u"timestamp_end"_s
            && !u.roles.contains(UIntRole::PacketEndDefaultClockTimestamp))
            u.roles.append(UIntRole::PacketEndDefaultClockTimestamp);
    }
}

static void assignPacketHeaderRoles(FieldClass *fc)
{
    if (!fc || fc->kind != FieldClassKind::Structure)
        return;
    for (auto &m : static_cast<StructureFC &>(*fc).members) {
        if (!m.fieldClass || m.fieldClass->kind != FieldClassKind::FixedLengthUInt)
            continue;
        auto &u = static_cast<FixedLengthUIntFC &>(*m.fieldClass);
        if (m.name == u"magic"_s && !u.roles.contains(UIntRole::PacketMagicNumber))
            u.roles.append(UIntRole::PacketMagicNumber);
        else if (m.name == u"stream_id"_s && !u.roles.contains(UIntRole::DataStreamClassId))
            u.roles.append(UIntRole::DataStreamClassId);
    }
}

// Index every enum-typed integer field in a field-class tree by member name.
// An enum's labels survive on its field class as IntMappings (label → ranges);
// a variant resolves its option selector values from that table.
static void indexEnumMappings(const FieldClass *fc, QHash<QString, IntMappings> &out)
{
    if (!fc)
        return;
    switch (fc->kind) {
    case FieldClassKind::Structure:
        for (const auto &m : static_cast<const StructureFC &>(*fc).members) {
            const FieldClass *mfc = m.fieldClass.get();
            if (mfc && mfc->kind == FieldClassKind::FixedLengthUInt) {
                const auto &u = static_cast<const FixedLengthUIntFC &>(*mfc);
                if (!u.mappings.isEmpty())
                    out.insert(m.name, u.mappings);
            } else if (mfc && mfc->kind == FieldClassKind::FixedLengthSInt) {
                const auto &s = static_cast<const FixedLengthSIntFC &>(*mfc);
                if (!s.mappings.isEmpty())
                    out.insert(m.name, s.mappings);
            }
            indexEnumMappings(mfc, out);
        }
        break;
    case FieldClassKind::StaticLengthArray:
        indexEnumMappings(static_cast<const StaticLengthArrayFC &>(*fc).elementFieldClass.get(), out);
        break;
    case FieldClassKind::DynamicLengthArray:
        indexEnumMappings(static_cast<const DynamicLengthArrayFC &>(*fc).elementFieldClass.get(), out);
        break;
    case FieldClassKind::Variant:
        for (const auto &opt : static_cast<const VariantFC &>(*fc).options)
            indexEnumMappings(opt.fieldClass.get(), out);
        break;
    case FieldClassKind::Optional:
        indexEnumMappings(static_cast<const OptionalFC &>(*fc).fieldClass.get(), out);
        break;
    default:
        break;
    }
}

// Fill each variant option's selector values/ranges from its selector field's
// enum table. Unlike a struct-local fixup, this works even when the selector
// enum and the variant are not siblings of the same struct (CTF 1.8): the
// labels are read from the resolved selector field class, keyed by name across
// the whole root tree, not from the lexical struct scope.
static void resolveVariantSelectors(FieldClass *fc, const QHash<QString, IntMappings> &enumByName)
{
    if (!fc)
        return;
    switch (fc->kind) {
    case FieldClassKind::Structure:
        for (auto &m : static_cast<StructureFC &>(*fc).members)
            resolveVariantSelectors(m.fieldClass.get(), enumByName);
        break;
    case FieldClassKind::StaticLengthArray:
        resolveVariantSelectors(
            static_cast<StaticLengthArrayFC &>(*fc).elementFieldClass.get(), enumByName);
        break;
    case FieldClassKind::DynamicLengthArray:
        resolveVariantSelectors(
            static_cast<DynamicLengthArrayFC &>(*fc).elementFieldClass.get(), enumByName);
        break;
    case FieldClassKind::Optional:
        resolveVariantSelectors(static_cast<OptionalFC &>(*fc).fieldClass.get(), enumByName);
        break;
    case FieldClassKind::Variant: {
        auto &vfc = static_cast<VariantFC &>(*fc);
        const auto &path = vfc.selectorFieldLocation.path;
        if (!path.isEmpty() && path.last()) {
            auto it = enumByName.find(*path.last());
            if (it != enumByName.end()) {
                for (auto &opt : vfc.options) {
                    // Don't clobber selector values already established elsewhere.
                    if (!opt.selectorValues.isEmpty() || !opt.selectorRanges.isEmpty())
                        continue;
                    auto mit = it->find(opt.name);
                    if (mit == it->end())
                        continue;
                    for (const auto &r : *mit) {
                        if (r.lower == r.upper)
                            opt.selectorValues.append(r.lower);
                        else
                            opt.selectorRanges.append({r.lower, r.upper});
                    }
                }
            }
        }
        for (auto &opt : vfc.options)
            resolveVariantSelectors(opt.fieldClass.get(), enumByName);
        break;
    }
    default:
        break;
    }
}

// Collect the names of all fixed-length unsigned-integer fields (enumerations
// included — they are unsigned integers with mappings). A dynamic-length array
// length must reference one (CTF 1.8: the sequence length is an unsigned int).
static void indexUIntFieldNames(const FieldClass *fc, QSet<QString> &out)
{
    if (!fc)
        return;
    switch (fc->kind) {
    case FieldClassKind::Structure:
        for (const auto &m : static_cast<const StructureFC &>(*fc).members) {
            if (m.fieldClass && m.fieldClass->kind == FieldClassKind::FixedLengthUInt)
                out.insert(m.name);
            indexUIntFieldNames(m.fieldClass.get(), out);
        }
        break;
    case FieldClassKind::StaticLengthArray:
        indexUIntFieldNames(
            static_cast<const StaticLengthArrayFC &>(*fc).elementFieldClass.get(), out);
        break;
    case FieldClassKind::DynamicLengthArray:
        indexUIntFieldNames(
            static_cast<const DynamicLengthArrayFC &>(*fc).elementFieldClass.get(), out);
        break;
    case FieldClassKind::Variant:
        for (const auto &opt : static_cast<const VariantFC &>(*fc).options)
            indexUIntFieldNames(opt.fieldClass.get(), out);
        break;
    case FieldClassKind::Optional:
        indexUIntFieldNames(static_cast<const OptionalFC &>(*fc).fieldClass.get(), out);
        break;
    default:
        break;
    }
}

// Validate that every dependent field references a field of the required class:
// a variant selector MUST be an enumeration and a dynamic-length array length
// MUST be an unsigned integer (CTF 1.8). References are matched by name — the
// same tying resolveVariantSelectors() uses. A missing or wrong-typed target is
// rejected so a corrupt schema cannot reach the reader (e.g. babeltrace's
// invalid-variant-selector-field-class / invalid-sequence-length-field-class).
static Utils::Result<> validateDependentFields(
    const FieldClass *fc,
    const QHash<QString, IntMappings> &enumByName,
    const QSet<QString> &uintNames)
{
    if (!fc)
        return Utils::ResultOk;
    switch (fc->kind) {
    case FieldClassKind::Structure:
        for (const auto &m : static_cast<const StructureFC &>(*fc).members)
            if (auto r = validateDependentFields(m.fieldClass.get(), enumByName, uintNames); !r)
                return r;
        break;
    case FieldClassKind::StaticLengthArray:
        return validateDependentFields(
            static_cast<const StaticLengthArrayFC &>(*fc).elementFieldClass.get(),
            enumByName, uintNames);
    case FieldClassKind::DynamicLengthArray: {
        const auto &a = static_cast<const DynamicLengthArrayFC &>(*fc);
        const auto &path = a.lengthFieldLocation.path;
        if (!path.isEmpty() && path.last() && !uintNames.contains(*path.last()))
            return Utils::ResultError(
                u"dynamic-length array length field '%1' is not an unsigned integer"_s
                    .arg(*path.last()));
        return validateDependentFields(a.elementFieldClass.get(), enumByName, uintNames);
    }
    case FieldClassKind::Variant: {
        const auto &v = static_cast<const VariantFC &>(*fc);
        const auto &path = v.selectorFieldLocation.path;
        if (!path.isEmpty() && path.last() && !enumByName.contains(*path.last()))
            return Utils::ResultError(
                u"variant selector field '%1' is not an enumeration"_s.arg(*path.last()));
        for (const auto &opt : v.options)
            if (auto r = validateDependentFields(opt.fieldClass.get(), enumByName, uintNames); !r)
                return r;
        break;
    }
    case FieldClassKind::Optional:
        return validateDependentFields(
            static_cast<const OptionalFC &>(*fc).fieldClass.get(), enumByName, uintNames);
    default:
        break;
    }
    return Utils::ResultOk;
}

// ── Parser implementation ───────────────────────────────────────────────────

struct StreamDef
{
    quint64 id = 0;
    FieldClassPtr eventHeaderFC;
    FieldClassPtr eventCommonContextFC;
    FieldClassPtr packetContextFC;
    // Clock referenced (via `map = clock.X.value`) by a timestamp field of this
    // stream; empty when the stream has no clocked field.
    QString clockName;
};

struct EventDef
{
    QString name;
    quint64 id = 0;
    quint64 streamId = 0;
    FieldClassPtr payload;
    FieldClassPtr specificContext;
};

class TsdlParserImpl
{
public:
    explicit TsdlParserImpl(const QByteArray &tsdl)
        : lex(tsdl)
    {}

    Utils::Result<Schema> parse()
    {
        while (!lex.at(TT::Eof) && !m_error)
            parseTopDecl();
        if (m_error)
            return Utils::ResultError(m_errorMsg);
        // A malformed literal anywhere — even one stepped over by skipToSemi in
        // an unhandled declaration — is fatal: every token has been lexed by the
        // time we reach EOF, so the lexer's first error is now visible.
        if (!lex.error().isEmpty())
            return Utils::ResultError(u"TSDL: "_s + lex.error());
        return buildSchema();
    }

private:
    Lexer lex;
    bool m_error = false;
    QString m_errorMsg;
    int m_structDepth = 0; // nesting guard for parseStructBody recursion

    ByteOrder m_defaultByteOrder = ByteOrder::LittleEndian;

    // Type alias name → factory (creates a fresh FC per call)
    QHash<QString, TypeFactory> m_typeFactories;
    // Clock-mapped type alias name → clock name
    QHash<QString, QString> m_clockMappings;
    // Named struct definitions (shared; used once per role assignment)
    QHash<QString, FieldClassPtr> m_namedStructs;

    QHash<quint64, StreamDef> m_streamDefs;
    QList<EventDef> m_eventDefs;
    QList<ClockClass> m_clockClasses;
    FieldClassPtr m_packetHeaderFC;

    // Most recent clock name seen via `map = clock.X.value` while parsing the
    // current stream/event scope; used to link a stream to its default clock.
    QString m_pendingClockName;

    // ── Error / consume helpers ────────────────────────────────────────────

    void error(const QString &msg)
    {
        if (!m_error) {
            m_error = true;
            m_errorMsg = u"TSDL: "_s + msg;
        }
    }

    bool expect(TT t, const char *what)
    {
        if (!lex.at(t)) {
            error(u"expected %1"_s.arg(QString::fromLatin1(what)));
            return false;
        }
        lex.next();
        return true;
    }

    QString consumeIdent()
    {
        if (!lex.at(TT::Ident)) {
            error(u"expected identifier"_s);
            return {};
        }
        return lex.next().text;
    }

    qint64 consumeInt()
    {
        if (!lex.at(TT::IntLit)) {
            error(u"expected integer"_s);
            return 0;
        }
        return lex.next().intVal;
    }

    // Consume an integer and reject values outside [minV, maxV] before narrowing
    // to int, so a hostile metadata value can't wrap on the cast.
    int consumeIntAsInt(qint64 minV = 0, qint64 maxV = std::numeric_limits<int>::max())
    {
        const qint64 v = consumeInt();
        if (m_error)
            return 0;
        if (v < minV || v > maxV) {
            error(u"integer value %1 is out of range [%2, %3]"_s.arg(v).arg(minV).arg(maxV));
            return 0;
        }
        return static_cast<int>(v);
    }

    // Consume an alignment value (bits). It MUST be a positive power of two,
    // matching the stricter validation the JSON metadata reader applies; an
    // invalid value would otherwise enter the schema and misbehave downstream.
    int consumeAlignment()
    {
        const qint64 v = consumeInt();
        if (m_error)
            return 1;
        if (v <= 0 || (v & (v - 1)) != 0 || v > std::numeric_limits<int>::max()) {
            error(u"alignment %1 must be a positive power of two"_s.arg(v));
            return 1;
        }
        return static_cast<int>(v);
    }

    QString consumeStr()
    {
        if (!lex.at(TT::StrLit)) {
            error(u"expected string"_s);
            return {};
        }
        return lex.next().text;
    }

    // An event/stream name may be a quoted string (required when it contains a
    // separator, e.g. "subsystem:event") or, per TSDL, a bare identifier — even
    // one spelled like a reserved keyword (`name = string;`).
    QString consumeName()
    {
        if (lex.at(TT::Ident))
            return lex.next().text;
        return consumeStr();
    }

    QString consumeAttrVal()
    {
        if (lex.at(TT::StrLit))
            return lex.next().text;
        if (lex.at(TT::IntLit))
            return QString::number(lex.next().intVal);
        if (lex.at(TT::Ident))
            return lex.next().text;
        error(u"expected attribute value"_s);
        return {};
    }

    // Skip tokens until semicolon (brace-balanced, top-level scope)
    void skipToSemi()
    {
        int depth = 0;
        while (!lex.at(TT::Eof)) {
            auto t = lex.next();
            if (t.type == TT::LBrace)
                ++depth;
            else if (t.type == TT::RBrace) {
                if (depth)
                    --depth;
            } else if (t.type == TT::Semi && depth == 0)
                return;
        }
    }

    // ── Top-level dispatch ────────────────────────────────────────────────

    void parseTopDecl()
    {
        if (lex.atIdent("typealias")) {
            parseTypealias();
            expect(TT::Semi, ";");
        } else if (lex.atIdent("typedef")) {
            parseTypedef();
            expect(TT::Semi, ";");
        } else if (lex.atIdent("trace")) {
            parseTraceBlock();
            expect(TT::Semi, ";");
        } else if (lex.atIdent("env")) {
            parseEnvBlock();
            expect(TT::Semi, ";");
        } else if (lex.atIdent("clock")) {
            parseClockBlock();
            expect(TT::Semi, ";");
        } else if (lex.atIdent("stream")) {
            parseStreamBlock();
            expect(TT::Semi, ";");
        } else if (lex.atIdent("event")) {
            parseEventBlock();
            expect(TT::Semi, ";");
        } else if (lex.atIdent("struct")) {
            parseNamedStruct();
            expect(TT::Semi, ";");
        } else
            skipToSemi();
    }

    // ── typealias / typedef ────────────────────────────────────────────────

    void parseTypealias()
    {
        lex.next(); // "typealias"
        QString clockName;
        TypeFactory factory = parseTypeFactory(clockName);
        if (m_error || !factory)
            return;
        if (!expect(TT::ColonEq, ":="))
            return;
        QString alias = consumeIdent();
        if (m_error)
            return;
        while (lex.at(TT::Ident))
            alias += u" "_s + lex.next().text;
        m_typeFactories[alias] = factory;
        if (!clockName.isEmpty())
            m_clockMappings[alias] = clockName;
    }

    void parseTypedef()
    {
        lex.next(); // "typedef"
        QString clockName;
        TypeFactory factory = parseTypeFactory(clockName);
        if (m_error || !factory)
            return;
        QString alias = consumeIdent();
        if (m_error)
            return;
        while (lex.at(TT::Ident))
            alias += u" "_s + lex.next().text;
        m_typeFactories[alias] = factory;
        if (!clockName.isEmpty())
            m_clockMappings[alias] = clockName;
    }

    // ── trace ──────────────────────────────────────────────────────────────

    void parseTraceBlock()
    {
        lex.next();
        if (!expect(TT::LBrace, "{"))
            return;
        while (!lex.at(TT::RBrace) && !lex.at(TT::Eof) && !m_error) {
            if (lex.atIdent("byte_order")) {
                lex.next();
                expect(TT::Equals, "=");
                QString v = consumeAttrVal();
                m_defaultByteOrder = (v == u"be"_s || v == u"network"_s) ? ByteOrder::BigEndian
                                                                         : ByteOrder::LittleEndian;
                expect(TT::Semi, ";");
            } else if (lex.atIdent("packet")) {
                lex.next();
                expect(TT::Dot, ".");
                consumeIdent(); // "header"
                expect(TT::ColonEq, ":=");
                m_packetHeaderFC = parseTypeRef();
                if (!m_error)
                    assignPacketHeaderRoles(m_packetHeaderFC.get());
                expect(TT::Semi, ";");
            } else {
                skipToSemi();
            }
        }
        expect(TT::RBrace, "}");
    }

    // ── env ────────────────────────────────────────────────────────────────

    void parseEnvBlock()
    {
        lex.next();
        expect(TT::LBrace, "{");
        while (!lex.at(TT::RBrace) && !lex.at(TT::Eof) && !m_error)
            skipToSemi();
        expect(TT::RBrace, "}");
    }

    // ── clock ─────────────────────────────────────────────────────────────

    void parseClockBlock()
    {
        lex.next();
        if (!expect(TT::LBrace, "{"))
            return;
        ClockClass cc;
        while (!lex.at(TT::RBrace) && !lex.at(TT::Eof) && !m_error) {
            if (lex.atIdent("name")) {
                lex.next();
                expect(TT::Equals, "=");
                cc.name = consumeAttrVal();
                expect(TT::Semi, ";");
            } else if (lex.atIdent("uuid")) {
                lex.next();
                expect(TT::Equals, "=");
                cc.uuid = QUuid::fromString(consumeStr());
                expect(TT::Semi, ";");
            } else if (lex.atIdent("description")) {
                lex.next();
                expect(TT::Equals, "=");
                cc.description = consumeStr();
                expect(TT::Semi, ";");
            } else if (lex.atIdent("freq")) {
                lex.next();
                expect(TT::Equals, "=");
                qint64 fv = consumeInt();
                if (fv <= 0)
                    error(u"clock freq must be greater than zero"_s);
                else
                    cc.frequency = static_cast<quint64>(fv);
                expect(TT::Semi, ";");
            } else if (lex.atIdent("offset")) {
                lex.next();
                expect(TT::Equals, "=");
                cc.offsetCycles = static_cast<quint64>(consumeInt());
                expect(TT::Semi, ";");
            } else if (lex.atIdent("offset_s")) {
                lex.next();
                expect(TT::Equals, "=");
                cc.offsetSeconds = consumeInt();
                expect(TT::Semi, ";");
            } else if (lex.atIdent("absolute")) {
                lex.next();
                expect(TT::Equals, "=");
                QString v = consumeAttrVal();
                if (v == u"true"_s)
                    cc.origin.isUnixEpoch = true;
                expect(TT::Semi, ";");
            } else if (lex.atIdent("precision")) {
                lex.next();
                expect(TT::Equals, "=");
                cc.precision = static_cast<quint64>(consumeInt());
                expect(TT::Semi, ";");
            } else
                skipToSemi();
        }
        expect(TT::RBrace, "}");
        if (!cc.name.isEmpty())
            m_clockClasses.append(cc);
    }

    // ── stream ────────────────────────────────────────────────────────────

    void parseStreamBlock()
    {
        lex.next();
        if (!expect(TT::LBrace, "{"))
            return;
        StreamDef sd;
        m_pendingClockName.clear();
        while (!lex.at(TT::RBrace) && !lex.at(TT::Eof) && !m_error) {
            if (lex.atIdent("id")) {
                lex.next();
                expect(TT::Equals, "=");
                sd.id = static_cast<quint64>(consumeInt());
                expect(TT::Semi, ";");
            } else if (lex.atIdent("event")) {
                lex.next();
                expect(TT::Dot, ".");
                QString sub = consumeIdent();
                expect(TT::ColonEq, ":=");
                if (sub == u"header"_s)
                    sd.eventHeaderFC = parseTypeRef(FieldLocation::Origin::EventRecordHeader);
                else // event.context: per-event common context (e.g. _vpid)
                    sd.eventCommonContextFC = parseTypeRef(
                        FieldLocation::Origin::EventRecordCommonContext);
                expect(TT::Semi, ";");
            } else if (lex.atIdent("packet")) {
                lex.next();
                expect(TT::Dot, ".");
                consumeIdent();
                expect(TT::ColonEq, ":=");
                sd.packetContextFC = parseTypeRef(FieldLocation::Origin::PacketContext);
                expect(TT::Semi, ";");
            } else {
                skipToSemi();
            }
        }
        expect(TT::RBrace, "}");
        sd.clockName = m_pendingClockName;
        m_streamDefs[sd.id] = sd;
    }

    // ── event ─────────────────────────────────────────────────────────────

    void parseEventBlock()
    {
        lex.next();
        if (!expect(TT::LBrace, "{"))
            return;
        EventDef ed;
        while (!lex.at(TT::RBrace) && !lex.at(TT::Eof) && !m_error) {
            if (lex.atIdent("name")) {
                lex.next();
                expect(TT::Equals, "=");
                ed.name = consumeName();
                expect(TT::Semi, ";");
            } else if (lex.atIdent("id")) {
                lex.next();
                expect(TT::Equals, "=");
                ed.id = static_cast<quint64>(consumeInt());
                expect(TT::Semi, ";");
            } else if (lex.atIdent("stream_id")) {
                lex.next();
                expect(TT::Equals, "=");
                ed.streamId = static_cast<quint64>(consumeInt());
                expect(TT::Semi, ";");
            } else if (lex.atIdent("fields")) {
                lex.next();
                expect(TT::ColonEq, ":=");
                ed.payload = parseTypeRef(FieldLocation::Origin::EventRecordPayload);
                expect(TT::Semi, ";");
            } else if (lex.atIdent("context")) {
                lex.next();
                expect(TT::ColonEq, ":=");
                ed.specificContext = parseTypeRef(FieldLocation::Origin::EventRecordSpecificContext);
                expect(TT::Semi, ";");
            } else {
                skipToSemi();
            }
        }
        expect(TT::RBrace, "}");
        m_eventDefs.append(ed);
    }

    // ── named struct ──────────────────────────────────────────────────────

    void parseNamedStruct()
    {
        lex.next(); // "struct"
        if (!lex.at(TT::Ident)) {
            error(u"expected struct name"_s);
            return;
        }
        QString name = lex.next().text;
        if (!lex.at(TT::LBrace)) {
            error(u"expected '{'"_s);
            return;
        }
        auto fc = parseStructBody(FieldLocation::Origin::EventRecordPayload);
        if (!m_error && lex.atIdent("align")) {
            lex.next();
            expect(TT::LParen, "(");
            int al = consumeAlignment();
            expect(TT::RParen, ")");
            if (fc && fc->kind == FieldClassKind::Structure)
                static_cast<StructureFC &>(*fc).minimumAlignment = al;
        }
        m_namedStructs[name] = fc;
    }

    // ── Type reference (for := assignments) ──────────────────────────────

    FieldClassPtr parseTypeRef(
        FieldLocation::Origin origin = FieldLocation::Origin::EventRecordPayload)
    {
        if (lex.atIdent("struct")) {
            lex.next();
            if (lex.at(TT::LBrace)) {
                auto fc = parseStructBody(origin);
                if (!m_error && lex.atIdent("align")) {
                    lex.next();
                    expect(TT::LParen, "(");
                    int al = consumeAlignment();
                    expect(TT::RParen, ")");
                    if (fc && fc->kind == FieldClassKind::Structure)
                        static_cast<StructureFC &>(*fc).minimumAlignment = al;
                }
                return fc;
            }
            if (!lex.at(TT::Ident)) {
                error(u"expected struct name"_s);
                return {};
            }
            QString name = lex.next().text;
            auto it = m_namedStructs.find(name);
            if (it == m_namedStructs.end()) {
                error(u"unknown struct '%1'"_s.arg(name));
                return {};
            }
            // Clone so this use's origin assignment doesn't mutate the shared
            // definition (which other scopes may reference with a different root).
            auto cloned = cloneFieldClass(*it);
            updateOrigins(cloned.get(), origin);
            return cloned;
        }
        // Fall through to factory-based type expression
        QString clockName;
        TypeFactory f = parseTypeFactory(clockName, origin);
        if (!f)
            return {};
        return f();
    }

    // ── Factory-based type expression ─────────────────────────────────────
    // Returns a TypeFactory (callable → fresh FieldClassPtr each invocation).

    TypeFactory parseTypeFactory(
        QString &clockNameOut,
        FieldLocation::Origin origin = FieldLocation::Origin::EventRecordPayload)
    {
        if (lex.atIdent("integer") || lex.atIdent("int"))
            return parseIntegerFactory(clockNameOut);
        if (lex.atIdent("floating_point") || lex.atIdent("float"))
            return parseFloatFactory();
        if (lex.atIdent("string"))
            return parseStringFactory();
        if (lex.atIdent("enum"))
            return parseEnumFactory(origin);
        if (lex.atIdent("struct")) {
            lex.next();
            if (lex.at(TT::LBrace)) {
                auto fc = parseStructBody(origin);
                if (!m_error && lex.atIdent("align")) {
                    lex.next();
                    expect(TT::LParen, "(");
                    int al = consumeAlignment();
                    expect(TT::RParen, ")");
                    if (fc && fc->kind == FieldClassKind::Structure)
                        static_cast<StructureFC &>(*fc).minimumAlignment = al;
                }
                // Clone per invocation: a typealias of this struct may be used by
                // several fields, each of which needs an independent instance.
                return [fc] { return cloneFieldClass(fc); };
            }
            if (!lex.at(TT::Ident)) {
                error(u"expected struct name"_s);
                return {};
            }
            QString name = lex.next().text;
            auto it = m_namedStructs.find(name);
            if (it == m_namedStructs.end()) {
                error(u"unknown struct '%1'"_s.arg(name));
                return {};
            }
            auto fc = *it;
            return [fc] { return cloneFieldClass(fc); };
        }
        if (lex.atIdent("variant"))
            return parseVariantFactory(origin);

        // Type alias lookup (possibly multi-word like "unsigned long")
        if (lex.at(TT::Ident)) {
            QString name = lex.next().text;
            if (lex.at(TT::Ident)) {
                QString combined = name + u" "_s + lex.peek().text;
                if (m_typeFactories.contains(combined)) {
                    lex.next();
                    name = combined;
                }
            }
            auto fit = m_typeFactories.find(name);
            if (fit != m_typeFactories.end()) {
                clockNameOut = m_clockMappings.value(name);
                return *fit; // return the factory (caller will invoke it)
            }
            auto sit = m_namedStructs.find(name);
            if (sit != m_namedStructs.end()) {
                auto fc = *sit;
                return [fc] { return cloneFieldClass(fc); };
            }
            error(u"unknown type '%1'"_s.arg(name));
            return {};
        }
        error(u"expected type expression"_s);
        return {};
    }

    // ── integer factory ───────────────────────────────────────────────────

    TypeFactory parseIntegerFactory(QString &clockNameOut)
    {
        lex.next(); // "integer" or "int"
        if (!expect(TT::LBrace, "{"))
            return {};

        int size = 32, align = 1;
        bool isSigned = false;
        ByteOrder bo = m_defaultByteOrder;
        DisplayBase base = DisplayBase::Decimal;
        QString clockName;

        while (!lex.at(TT::RBrace) && !lex.at(TT::Eof) && !m_error) {
            QString attr = consumeIdent();
            if (m_error)
                break;
            if (!expect(TT::Equals, "="))
                break;
            if (attr == u"size"_s)
                size = consumeIntAsInt(1, 64);
            else if (attr == u"align"_s)
                align = consumeAlignment();
            else if (attr == u"signed"_s) {
                if (lex.at(TT::IntLit))
                    isSigned = (lex.next().intVal != 0);
                else {
                    QString v = consumeAttrVal();
                    isSigned = (v == u"true"_s || v == u"1"_s);
                }
            } else if (attr == u"byte_order"_s) {
                QString v = consumeAttrVal();
                bo = (v == u"be"_s || v == u"network"_s) ? ByteOrder::BigEndian
                                                         : ByteOrder::LittleEndian;
            } else if (attr == u"base"_s) {
                QString v = consumeAttrVal();
                if (v == u"hex"_s || v == u"hexadecimal"_s || v == u"16"_s)
                    base = DisplayBase::Hexadecimal;
                else if (v == u"oct"_s || v == u"octal"_s || v == u"8"_s)
                    base = DisplayBase::Octal;
                else if (v == u"bin"_s || v == u"binary"_s || v == u"2"_s)
                    base = DisplayBase::Binary;
                else
                    base = DisplayBase::Decimal;
            } else if (attr == u"encoding"_s)
                consumeAttrVal();
            else if (attr == u"map"_s) {
                // map = clock.X.value
                QString v = consumeAttrVal();
                if (v == u"clock"_s) {
                    if (lex.at(TT::Dot)) {
                        lex.next();
                        clockName = consumeIdent();
                    }
                    if (lex.at(TT::Dot)) {
                        lex.next();
                        consumeAttrVal();
                    } // "value"
                }
            } else
                consumeAttrVal();
            if (!lex.at(TT::RBrace))
                expect(TT::Semi, ";");
        }
        expect(TT::RBrace, "}");
        if (m_error)
            return {};

        // Fixed-length integers are decoded into a 64-bit word (see
        // scalarfieldclasses.h), so a size outside 1..64 is unsupported. Reject
        // it here rather than emit a schema that always fails at read time.
        if (size <= 0 || size > 64) {
            error(u"integer size %1 is out of range (1..64)"_s.arg(size));
            return {};
        }

        clockNameOut = clockName;
        bool isClocked = !clockName.isEmpty();
        if (isClocked)
            m_pendingClockName = clockName;

        // CTF 1.8 §4.1.2: bit order is not separately specified in TSDL; it
        // defaults to the byte order (big-endian → most-significant-bit first).
        const BitOrder bitOrder = bo == ByteOrder::BigEndian
                                      ? BitOrder::MostSignificantFirst
                                      : BitOrder::LeastSignificantFirst;

        if (isSigned) {
            return [size, align, bo, bitOrder, base]() -> FieldClassPtr {
                auto fc = std::make_shared<FixedLengthSIntFC>();
                fc->length = size;
                fc->alignment = align;
                fc->byteOrder = bo;
                fc->bitOrder = bitOrder;
                fc->displayBase = base;
                return fc;
            };
        } else {
            return [size, align, bo, bitOrder, base, isClocked]() -> FieldClassPtr {
                auto fc = std::make_shared<FixedLengthUIntFC>();
                fc->length = size;
                fc->alignment = align;
                fc->byteOrder = bo;
                fc->bitOrder = bitOrder;
                fc->displayBase = base;
                if (isClocked)
                    fc->roles.append(UIntRole::DefaultClockTimestamp);
                return fc;
            };
        }
    }

    // ── float factory ─────────────────────────────────────────────────────

    TypeFactory parseFloatFactory()
    {
        lex.next();
        if (!expect(TT::LBrace, "{"))
            return {};
        int expDig = 8, mantDig = 24, align = 1;
        ByteOrder bo = m_defaultByteOrder;
        while (!lex.at(TT::RBrace) && !lex.at(TT::Eof) && !m_error) {
            QString attr = consumeIdent();
            if (m_error)
                break;
            if (!expect(TT::Equals, "="))
                break;
            if (attr == u"exp_dig"_s)
                expDig = consumeIntAsInt();
            else if (attr == u"mant_dig"_s)
                mantDig = consumeIntAsInt();
            else if (attr == u"align"_s)
                align = consumeAlignment();
            else if (attr == u"byte_order"_s) {
                QString v = consumeAttrVal();
                bo = (v == u"be"_s || v == u"network"_s) ? ByteOrder::BigEndian
                                                         : ByteOrder::LittleEndian;
            } else
                consumeAttrVal();
            if (!lex.at(TT::RBrace))
                expect(TT::Semi, ";");
        }
        expect(TT::RBrace, "}");
        int len = expDig + mantDig;
        const BitOrder bitOrder = bo == ByteOrder::BigEndian
                                      ? BitOrder::MostSignificantFirst
                                      : BitOrder::LeastSignificantFirst;
        return [len, align, bo, bitOrder]() -> FieldClassPtr {
            auto fc = std::make_shared<FixedLengthFloatFC>();
            fc->length = len;
            fc->alignment = align;
            fc->byteOrder = bo;
            fc->bitOrder = bitOrder;
            return fc;
        };
    }

    // ── string factory ────────────────────────────────────────────────────

    TypeFactory parseStringFactory()
    {
        lex.next(); // "string"
        if (lex.at(TT::LBrace)) {
            lex.next();
            while (!lex.at(TT::RBrace) && !lex.at(TT::Eof) && !m_error)
                skipToSemi();
            expect(TT::RBrace, "}");
        }
        return []() -> FieldClassPtr { return std::make_shared<NullTerminatedStringFC>(); };
    }

    // ── enum factory ──────────────────────────────────────────────────────
    // Returns a factory producing the base-type FC, with enum mappings attached.
    // The mappings (label → ranges) double as the variant selector table, read
    // back in buildSchema() via resolveVariantSelectors().

    TypeFactory parseEnumFactory(FieldLocation::Origin origin)
    {
        lex.next(); // "enum"
        if (lex.at(TT::Ident))
            lex.next(); // optional enum name
        if (!expect(TT::Colon, ":"))
            return {};
        QString dummy;
        TypeFactory baseFactory = parseTypeFactory(dummy, origin);
        if (m_error || !baseFactory)
            return {};

        if (!expect(TT::LBrace, "{"))
            return {};

        QHash<QString, QList<QPair<qint64, qint64>>> labels;
        qint64 nextVal = 0;
        while (!lex.at(TT::RBrace) && !lex.at(TT::Eof) && !m_error) {
            QString label;
            if (lex.at(TT::Ident))
                label = lex.next().text;
            else if (lex.at(TT::StrLit))
                label = lex.next().text;
            else {
                error(u"expected enum label"_s);
                break;
            }
            if (m_error)
                break;
            if (lex.at(TT::Equals)) {
                lex.next();
                qint64 lo = consumeInt();
                if (lex.at(TT::DotDotDot)) {
                    lex.next();
                    qint64 hi = consumeInt();
                    labels[label].append({lo, hi});
                    nextVal = hi + 1;
                } else {
                    labels[label].append({lo, lo});
                    nextVal = lo + 1;
                }
            } else {
                labels[label].append({nextVal, nextVal});
                ++nextVal;
            }
            if (lex.at(TT::Comma))
                lex.next();
        }
        expect(TT::RBrace, "}");

        // Build IntMappings for display
        IntMappings mappings;
        for (auto it = labels.begin(); it != labels.end(); ++it) {
            QList<IntMappingRange> ranges;
            for (const auto &r : it.value())
                ranges.append({static_cast<qint64>(r.first), static_cast<qint64>(r.second)});
            mappings[it.key()] = ranges;
        }

        return [baseFactory = std::move(baseFactory), mappings = std::move(mappings)]()
                   -> FieldClassPtr {
            auto fc = baseFactory();
            if (fc && fc->kind == FieldClassKind::FixedLengthUInt)
                static_cast<FixedLengthUIntFC &>(*fc).mappings = mappings;
            else if (fc && fc->kind == FieldClassKind::FixedLengthSInt)
                static_cast<FixedLengthSIntFC &>(*fc).mappings = mappings;
            return fc;
        };
    }

    // ── struct body ───────────────────────────────────────────────────────

    FieldClassPtr parseStructBody(FieldLocation::Origin origin)
    {
        // Guard against deeply nested structures that would otherwise overflow
        // the stack via parseTypeFactory -> parseStructBody recursion.
        static constexpr int kMaxStructDepth = 64;
        if (m_structDepth >= kMaxStructDepth) {
            error(u"struct nesting is too deep"_s);
            return {};
        }
        ++m_structDepth;
        const auto depthGuard = qScopeGuard([this] { --m_structDepth; });

        if (!expect(TT::LBrace, "{"))
            return {};
        auto sfc = std::make_shared<StructureFC>();

        while (!lex.at(TT::RBrace) && !lex.at(TT::Eof) && !m_error) {
            QString clockName;
            TypeFactory factory = parseTypeFactory(clockName, origin);
            if (m_error || !factory) {
                skipToSemi();
                continue;
            }

            if (!lex.at(TT::Ident)) {
                error(u"expected field name"_s);
                return {};
            }
            QString fieldName = lex.next().text;

            // Create the field's FC
            FieldClassPtr typeFC = factory();
            if (m_error || !typeFC) {
                skipToSemi();
                continue;
            }

            // Clock role already embedded by factory; also annotate struct members
            // inside variant option structs later. Nothing more needed here.

            // Array suffix
            if (lex.at(TT::LBracket)) {
                lex.next();
                if (lex.at(TT::IntLit)) {
                    int count = consumeIntAsInt();
                    if (m_error) {
                        skipToSemi();
                        continue;
                    }
                    expect(TT::RBracket, "]");
                    auto arr = std::make_shared<StaticLengthArrayFC>();
                    arr->elementFieldClass = std::move(typeFC);
                    arr->length = count;
                    sfc->members.append({fieldName, arr, {}});
                } else if (lex.at(TT::Ident)) {
                    QString lenField = lex.next().text;
                    expect(TT::RBracket, "]");
                    auto arr = std::make_shared<DynamicLengthArrayFC>();
                    arr->elementFieldClass = std::move(typeFC);
                    arr->lengthFieldLocation.origin = origin;
                    arr->lengthFieldLocation.path = {std::optional<QString>(lenField)};
                    sfc->members.append({fieldName, arr, {}});
                } else {
                    error(u"expected array length"_s);
                    return {};
                }
            } else {
                sfc->members.append({fieldName, std::move(typeFC), {}});
            }
            expect(TT::Semi, ";");
        }
        expect(TT::RBrace, "}");

        // Variant selector values are resolved later, in buildSchema(), from the
        // selector field's enum mappings — see resolveVariantSelectors(). Doing it
        // there (rather than per-struct here) handles selectors that aren't
        // siblings of the variant within the same struct body.
        return sfc;
    }

    // ── variant factory ───────────────────────────────────────────────────

    TypeFactory parseVariantFactory(FieldLocation::Origin origin)
    {
        lex.next(); // "variant"
        if (!expect(TT::Lt, "<"))
            return {};
        QString selectorName = consumeIdent();
        if (!expect(TT::Gt, ">"))
            return {};
        if (lex.at(TT::Ident))
            lex.next(); // optional variant name

        if (!expect(TT::LBrace, "{"))
            return {};

        auto vfc = std::make_shared<VariantFC>();
        vfc->selectorFieldLocation.origin = origin;
        vfc->selectorFieldLocation.path = {std::optional<QString>(selectorName)};

        while (!lex.at(TT::RBrace) && !lex.at(TT::Eof) && !m_error) {
            QString clockName;
            TypeFactory f = parseTypeFactory(clockName, origin);
            if (m_error || !f) {
                skipToSemi();
                continue;
            }
            if (!lex.at(TT::Ident)) {
                error(u"expected variant option name"_s);
                return {};
            }
            QString optName = lex.next().text;
            VariantOption opt;
            opt.fieldClass = f();
            opt.name = optName;
            vfc->options.append(std::move(opt));
            expect(TT::Semi, ";");
        }
        expect(TT::RBrace, "}");

        // Clone per invocation: a typealias of this variant may be used by
        // several fields, each of which needs an independent instance (otherwise
        // a later reference's updateOrigins()/role assignment overwrites others).
        return [vfc] { return cloneFieldClass(vfc); };
    }

    // ── Finalize / build Schema ───────────────────────────────────────────

    // Mark the CTF 1.8 event-header fields with their CTF2 roles so the binary
    // reader can locate them by role rather than by name: the top-level `id`
    // and, inside the compact/extended variant, the per-option `id` and
    // `timestamp` (CTF 1.8 §6.1 recommended event-header layout).
    void markEventHeaderId(FieldClass *fc)
    {
        if (!fc || fc->kind != FieldClassKind::Structure)
            return;
        for (auto &m : static_cast<StructureFC &>(*fc).members) {
            if (m.name == u"id"_s && m.fieldClass
                && m.fieldClass->kind == FieldClassKind::FixedLengthUInt) {
                auto &u = static_cast<FixedLengthUIntFC &>(*m.fieldClass);
                if (!u.roles.contains(UIntRole::EventRecordClassId))
                    u.roles.append(UIntRole::EventRecordClassId);
            } else if (m.fieldClass && m.fieldClass->kind == FieldClassKind::Variant) {
                auto &vfc = static_cast<VariantFC &>(*m.fieldClass);
                for (auto &opt : vfc.options) {
                    if (!opt.fieldClass || opt.fieldClass->kind != FieldClassKind::Structure)
                        continue;
                    for (auto &im : static_cast<StructureFC &>(*opt.fieldClass).members) {
                        if (!im.fieldClass || im.fieldClass->kind != FieldClassKind::FixedLengthUInt)
                            continue;
                        auto &iu = static_cast<FixedLengthUIntFC &>(*im.fieldClass);
                        if (im.name == u"id"_s && !iu.roles.contains(UIntRole::EventRecordClassId))
                            iu.roles.append(UIntRole::EventRecordClassId);
                        else if (
                            im.name == u"timestamp"_s
                            && !iu.roles.contains(UIntRole::DefaultClockTimestamp))
                            iu.roles.append(UIntRole::DefaultClockTimestamp);
                    }
                }
            }
        }
    }

    Utils::Result<Schema> buildSchema()
    {
        Schema schema;
        schema.clockClasses = m_clockClasses;

        if (m_packetHeaderFC) {
            schema.traceClass.emplace();
            schema.traceClass->packetHeaderFieldClass = m_packetHeaderFC;
        }

        // Build DataStreamClasses from stream definitions
        for (auto it = m_streamDefs.begin(); it != m_streamDefs.end(); ++it) {
            const StreamDef &sd = it.value();
            DataStreamClass dsc;
            dsc.id = sd.id;

            if (sd.packetContextFC) {
                updateOrigins(sd.packetContextFC.get(), FieldLocation::Origin::PacketContext);
                assignPacketContextRoles(sd.packetContextFC.get());
                dsc.packetContextFieldClass = sd.packetContextFC;
            }

            if (sd.eventHeaderFC) {
                updateOrigins(sd.eventHeaderFC.get(), FieldLocation::Origin::EventRecordHeader);
                markEventHeaderId(sd.eventHeaderFC.get());
                dsc.eventRecordHeaderFieldClass = sd.eventHeaderFC;
            }

            if (sd.eventCommonContextFC) {
                updateOrigins(
                    sd.eventCommonContextFC.get(), FieldLocation::Origin::EventRecordCommonContext);
                dsc.eventRecordCommonContextFieldClass = sd.eventCommonContextFC;
            }

            // Default clock: the clock this stream's timestamp field maps to,
            // falling back to the first declared clock class.
            if (!sd.clockName.isEmpty())
                dsc.defaultClockClassName = sd.clockName;
            else if (!m_clockClasses.isEmpty())
                dsc.defaultClockClassName = m_clockClasses.first().name;

            schema.dataStreamClasses.append(dsc);
        }

        // If no streams defined, create default stream 0
        if (schema.dataStreamClasses.isEmpty()) {
            DataStreamClass dsc;
            dsc.id = 0;
            if (!m_clockClasses.isEmpty())
                dsc.defaultClockClassName = m_clockClasses.first().name;
            schema.dataStreamClasses.append(dsc);
        }

        // Add event record classes to their respective streams
        for (const auto &ed : m_eventDefs) {
            DataStreamClass *dsc = nullptr;
            for (auto &d : schema.dataStreamClasses)
                if (d.id == ed.streamId) {
                    dsc = &d;
                    break;
                }
            if (!dsc) {
                DataStreamClass newDsc;
                newDsc.id = ed.streamId;
                if (!m_clockClasses.isEmpty())
                    newDsc.defaultClockClassName = m_clockClasses.first().name;
                schema.dataStreamClasses.append(newDsc);
                dsc = &schema.dataStreamClasses.last();
            }
            EventRecordClass erc;
            erc.id = ed.id;
            erc.name = ed.name;
            erc.payloadFieldClass = ed.payload;
            erc.specificContextFieldClass = ed.specificContext;
            // Re-stamp origins (as done above for the stream-level fields): a
            // typealias bakes in the origin of its definition scope, so an
            // aliased variant/array selector nested in the payload or specific
            // context would otherwise keep the wrong origin.
            if (erc.payloadFieldClass)
                updateOrigins(
                    erc.payloadFieldClass.get(), FieldLocation::Origin::EventRecordPayload);
            if (erc.specificContextFieldClass)
                updateOrigins(
                    erc.specificContextFieldClass.get(),
                    FieldLocation::Origin::EventRecordSpecificContext);
            dsc->eventRecordClasses.append(erc);
        }

        // Resolve variant option selector values from the selector field's enum
        // mappings (CTF 1.8 ties them by name). Index every enum across all roots
        // first, then fill each variant — so a selector enum need not be a sibling
        // of the variant within one struct body.
        QList<FieldClass *> roots;
        if (schema.traceClass)
            roots.append(schema.traceClass->packetHeaderFieldClass.get());
        for (auto &dsc : schema.dataStreamClasses) {
            roots.append(dsc.packetContextFieldClass.get());
            roots.append(dsc.eventRecordHeaderFieldClass.get());
            roots.append(dsc.eventRecordCommonContextFieldClass.get());
            for (auto &erc : dsc.eventRecordClasses) {
                roots.append(erc.payloadFieldClass.get());
                roots.append(erc.specificContextFieldClass.get());
            }
        }
        QHash<QString, IntMappings> enumByName;
        for (FieldClass *root : roots)
            indexEnumMappings(root, enumByName);
        for (FieldClass *root : roots)
            resolveVariantSelectors(root, enumByName);

        // Reject schemas whose dependent fields point at the wrong class: a
        // variant tag must be an enumeration, a sequence length an unsigned
        // integer. Otherwise the broken reference reaches the reader unchecked.
        QSet<QString> uintNames;
        for (const FieldClass *root : roots)
            indexUIntFieldNames(root, uintNames);
        for (const FieldClass *root : roots)
            if (auto r = validateDependentFields(root, enumByName, uintNames); !r)
                return Utils::ResultError(r.error());

        return schema;
    }
};

// ── Public API ──────────────────────────────────────────────────────────────

Utils::Result<Schema> TsdlParser::parse(const QByteArray &tsdl)
{
    TsdlParserImpl impl(tsdl);
    return impl.parse();
}

} // namespace CommonTraceFormat
