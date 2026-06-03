// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Feature/regression tests for the CTF 1.8 TSDL metadata parser
// (TsdlParser). Each test documents the behaviour it guards against.

#include <commontraceformat/metadata/tsdlparser.h>

#include <commontraceformat/schema/scalarfieldclasses.h>
#include <commontraceformat/schema/compoundfieldclasses.h>
#include <commontraceformat/schema/datastreamclass.h>
#include <commontraceformat/schema/eventrecordclass.h>
#include <commontraceformat/schema/schema.h>

#include <utils/result.h>

#include <QtTest>

using namespace CommonTraceFormat;
using namespace Qt::StringLiterals;

namespace {

// Locate the payload structure of the first event record class in stream 0.
const StructureFC *firstEventPayload(const Schema &schema)
{
    for (const auto &dsc : schema.dataStreamClasses) {
        if (dsc.eventRecordClasses.isEmpty())
            continue;
        const FieldClassPtr &p = dsc.eventRecordClasses.first().payloadFieldClass;
        if (p && p->kind == FieldClassKind::Structure)
            return static_cast<const StructureFC *>(p.get());
    }
    return nullptr;
}

// Find a member field class by name within a structure.
const FieldClass *member(const StructureFC *s, const QString &name)
{
    if (!s)
        return nullptr;
    for (const auto &m : s->members)
        if (m.name == name)
            return m.fieldClass.get();
    return nullptr;
}

// Find an event record class by id across all streams.
const EventRecordClass *eventById(const Schema &schema, quint64 id)
{
    for (const auto &dsc : schema.dataStreamClasses)
        for (const auto &erc : dsc.eventRecordClasses)
            if (erc.id == id)
                return &erc;
    return nullptr;
}

const StructureFC *asStruct(const FieldClassPtr &p)
{
    return (p && p->kind == FieldClassKind::Structure)
        ? static_cast<const StructureFC *>(p.get()) : nullptr;
}

// First variant member within a structure.
const VariantFC *firstVariant(const StructureFC *s)
{
    if (!s)
        return nullptr;
    for (const auto &m : s->members)
        if (m.fieldClass && m.fieldClass->kind == FieldClassKind::Variant)
            return static_cast<const VariantFC *>(m.fieldClass.get());
    return nullptr;
}

} // namespace

class tst_TsdlParser : public QObject
{
    Q_OBJECT
private slots:
    // A `byte_order = network` attribute is an alias for big-endian (TSDL maps
    // "network" to network byte order). The float factory previously honoured
    // only "be" and silently fell back to little-endian for "network".
    void floatByteOrderNetwork()
    {
        const QByteArray tsdl =
            "trace { major = 1; minor = 8; byte_order = le; };\n"
            "event {\n"
            "  name = \"ev\";\n"
            "  fields := struct {\n"
            "    floating_point { exp_dig = 8; mant_dig = 24; byte_order = network; align = 8; } f;\n"
            "  };\n"
            "};\n";

        auto r = TsdlParser::parse(tsdl);
        QVERIFY2(r.has_value(), qPrintable(r ? QString() : r.error()));

        const FieldClass *f = member(firstEventPayload(*r), u"f"_s);
        QVERIFY(f);
        QCOMPARE(f->kind, FieldClassKind::FixedLengthFloat);
        QCOMPARE(static_cast<const FixedLengthFloatFC *>(f)->byteOrder, ByteOrder::BigEndian);
    }

    // An integer `byte_order = network` is equally big-endian. (The integer
    // factory already handled this; kept as a sibling regression guard.)
    void integerByteOrderNetwork()
    {
        const QByteArray tsdl =
            "trace { major = 1; minor = 8; byte_order = le; };\n"
            "event {\n"
            "  name = \"ev\";\n"
            "  fields := struct {\n"
            "    integer { size = 32; byte_order = network; } n;\n"
            "  };\n"
            "};\n";

        auto r = TsdlParser::parse(tsdl);
        QVERIFY2(r.has_value(), qPrintable(r ? QString() : r.error()));

        const FieldClass *n = member(firstEventPayload(*r), u"n"_s);
        QVERIFY(n);
        QCOMPARE(n->kind, FieldClassKind::FixedLengthUInt);
        QCOMPARE(static_cast<const FixedLengthUIntFC *>(n)->byteOrder, ByteOrder::BigEndian);
    }

    // A variant's option selector values are derived from the selecting enum's
    // labels. This must work even when the enum and the variant are NOT members
    // of the same struct body: here `tag` is a top-level payload field while the
    // variant lives one struct deeper. The old per-struct fixup looked only at
    // sibling fields and silently dropped the selector values (making the CTF1
    // trace unreadable); resolution now happens in buildSchema() across the
    // whole field-class tree.
    void variantSelectorNonSibling()
    {
        const QByteArray tsdl =
            "trace { major = 1; minor = 8; byte_order = le; };\n"
            "event {\n"
            "  name = \"ev\";\n"
            "  fields := struct {\n"
            "    enum : integer { size = 8; } { A = 0, B = 1 } tag;\n"
            "    struct {\n"
            "      variant <tag> {\n"
            "        integer { size = 8; } A;\n"
            "        integer { size = 16; } B;\n"
            "      } v;\n"
            "    } inner;\n"
            "  };\n"
            "};\n";

        auto r = TsdlParser::parse(tsdl);
        QVERIFY2(r.has_value(), qPrintable(r ? QString() : r.error()));

        const StructureFC *payload = firstEventPayload(*r);
        const FieldClass *innerFc = member(payload, u"inner"_s);
        QVERIFY(innerFc);
        QCOMPARE(innerFc->kind, FieldClassKind::Structure);

        const FieldClass *vFc = member(static_cast<const StructureFC *>(innerFc), u"v"_s);
        QVERIFY(vFc);
        QCOMPARE(vFc->kind, FieldClassKind::Variant);
        const auto &vfc = *static_cast<const VariantFC *>(vFc);

        QCOMPARE(vfc.options.size(), 2);
        for (const auto &opt : vfc.options) {
            QCOMPARE(opt.selectorRanges.size(), 0);
            QCOMPARE(opt.selectorValues.size(), 1);
            if (opt.name == u"A"_s)
                QCOMPARE(opt.selectorValues.first(), qint64(0));
            else if (opt.name == u"B"_s)
                QCOMPARE(opt.selectorValues.first(), qint64(1));
            else
                QFAIL("unexpected variant option name");
        }
    }

    // A named struct/typealias may be referenced from several scopes (spec
    // 7.3.1), each carrying its own dynamic-scope root (spec 7.3.2). Each
    // reference must yield an independent field-class instance: sharing one
    // would let a later reference's origin assignment (updateOrigins) overwrite
    // an earlier reference's, corrupting the schema. Here `struct shared` is
    // used as one event's payload and another event's specific context.
    void namedStructReusedAcrossScopes()
    {
        const QByteArray tsdl =
            "trace { major = 1; minor = 8; byte_order = le; };\n"
            "struct shared {\n"
            "  enum : integer { size = 8; } { A = 0, B = 1 } tag;\n"
            "  variant <tag> { integer { size = 8; } A; integer { size = 16; } B; } v;\n"
            "};\n"
            "event { name = \"ev_payload\"; id = 0; stream_id = 0; fields := struct shared; };\n"
            "event { name = \"ev_ctx\";     id = 1; stream_id = 0; context := struct shared; };\n";

        auto r = TsdlParser::parse(tsdl);
        QVERIFY2(r.has_value(), qPrintable(r ? QString() : r.error()));

        const EventRecordClass *evP = eventById(*r, 0);
        const EventRecordClass *evC = eventById(*r, 1);
        QVERIFY(evP && evC);

        const VariantFC *vP = firstVariant(asStruct(evP->payloadFieldClass));
        const VariantFC *vC = firstVariant(asStruct(evC->specificContextFieldClass));
        QVERIFY(vP && vC);

        // The two uses must be distinct objects (the heart of the bug).
        QVERIFY(vP != vC);

        // Each use's selector location must carry the origin of the scope it was
        // used in; a shared instance would give both the last-written origin.
        QCOMPARE(vP->selectorFieldLocation.origin, FieldLocation::Origin::EventRecordPayload);
        QCOMPARE(vC->selectorFieldLocation.origin, FieldLocation::Origin::EventRecordSpecificContext);

        // Both must still have their selector values resolved from `tag`.
        QCOMPARE(vP->options.size(), 2);
        QCOMPARE(vC->options.size(), 2);
        for (const VariantFC *v : {vP, vC})
            for (const auto &opt : v->options)
                QCOMPARE(opt.selectorValues.size(), 1);
    }

    // TSDL integer literals follow C conventions: a leading `0` introduces an
    // octal value (`010` == 8). The lexer previously only special-cased the
    // `0x` hex prefix and decoded `010` as decimal 10.
    void integerOctalLiteral()
    {
        const QByteArray tsdl =
            "trace { major = 1; minor = 8; byte_order = le; };\n"
            "event {\n"
            "  name = \"ev\";\n"
            "  fields := struct {\n"
            "    integer { size = 010; align = 010; signed = false; } o;\n"
            "  };\n"
            "};\n";

        auto r = TsdlParser::parse(tsdl);
        QVERIFY2(r.has_value(), qPrintable(r ? QString() : r.error()));

        const FieldClass *o = member(firstEventPayload(*r), u"o"_s);
        QVERIFY(o);
        QCOMPARE(o->kind, FieldClassKind::FixedLengthUInt);
        const auto *u = static_cast<const FixedLengthUIntFC *>(o);
        QCOMPARE(u->length, 8); // 010 octal == 8, not decimal 10
        QCOMPARE(u->alignment, 8);
    }

    // A `typealias` of a variant is stored as a factory and instantiated once
    // per referencing field. Each instance must be independent: the variant
    // factory previously returned a single shared object, so a second use's
    // origin assignment (updateOrigins) clobbered the first. Mirrors
    // namedStructReusedAcrossScopes for a directly-aliased variant.
    void variantTypealiasReusedAcrossScopes()
    {
        const QByteArray tsdl =
            "trace { major = 1; minor = 8; byte_order = le; };\n"
            "typealias variant <tag> {\n"
            "  integer { size = 8; } A;\n"
            "  integer { size = 16; } B;\n"
            "} := myvar;\n"
            "event { name = \"ev_payload\"; id = 0; stream_id = 0; fields := struct {\n"
            "  enum : integer { size = 8; } { A = 0, B = 1 } tag;\n"
            "  myvar v;\n"
            "}; };\n"
            "event { name = \"ev_ctx\"; id = 1; stream_id = 0; context := struct {\n"
            "  enum : integer { size = 8; } { A = 0, B = 1 } tag;\n"
            "  myvar v;\n"
            "}; };\n";

        auto r = TsdlParser::parse(tsdl);
        QVERIFY2(r.has_value(), qPrintable(r ? QString() : r.error()));

        const EventRecordClass *evP = eventById(*r, 0);
        const EventRecordClass *evC = eventById(*r, 1);
        QVERIFY(evP && evC);

        const VariantFC *vP = firstVariant(asStruct(evP->payloadFieldClass));
        const VariantFC *vC = firstVariant(asStruct(evC->specificContextFieldClass));
        QVERIFY(vP && vC);

        // The two uses must be distinct objects (the heart of the bug).
        QVERIFY(vP != vC);

        // Each use's selector location must carry the origin of its own scope; a
        // shared instance would give both the last-written origin.
        QCOMPARE(vP->selectorFieldLocation.origin, FieldLocation::Origin::EventRecordPayload);
        QCOMPARE(
            vC->selectorFieldLocation.origin, FieldLocation::Origin::EventRecordSpecificContext);

        QCOMPARE(vP->options.size(), 2);
        QCOMPARE(vC->options.size(), 2);
    }

    // A variant tag MUST be an enumeration (CTF 1.8). A selector that resolves to
    // a non-enum field (here a struct) must be rejected at parse time, rather
    // than silently producing a variant whose options never match a value.
    void variantSelectorMustBeEnum()
    {
        const QByteArray tsdl =
            "trace { major = 1; minor = 8; byte_order = le; };\n"
            "event {\n"
            "  name = \"ev\";\n"
            "  fields := struct {\n"
            "    struct { integer { size = 8; } hello; } selector;\n"
            "    variant <selector> { integer { size = 8; } a; integer { size = 8; } b; } v;\n"
            "  };\n"
            "};\n";

        auto r = TsdlParser::parse(tsdl);
        QVERIFY2(!r.has_value(), "expected a variant-selector validation error");
        QVERIFY(r.error().contains(u"selector"_s));
    }

    // A dynamic-length array's length MUST be an unsigned integer (CTF 1.8). A
    // length that resolves to a non-integer field (here a struct) is rejected.
    void sequenceLengthMustBeUInt()
    {
        const QByteArray tsdl =
            "trace { major = 1; minor = 8; byte_order = le; };\n"
            "event {\n"
            "  name = \"ev\";\n"
            "  fields := struct {\n"
            "    struct { integer { size = 8; } hello; } len;\n"
            "    integer { size = 8; } x[len];\n"
            "  };\n"
            "};\n";

        auto r = TsdlParser::parse(tsdl);
        QVERIFY2(!r.has_value(), "expected a sequence-length validation error");
        QVERIFY(r.error().contains(u"len"_s));
    }

    // An integer literal that overflows 64 bits is rejected, even when it sits
    // in an attribute the parser otherwise ignores (here trace `major`).
    void integerLiteralOutOfRange()
    {
        const QByteArray tsdl =
            "trace {\n"
            "  major = 234523978563489756238975628937465892374652893746589237645982376458;\n"
            "  minor = 8; byte_order = le;\n"
            "};\n"
            "event { name = \"ev\"; fields := struct { integer { size = 8; } x; }; };\n";

        auto r = TsdlParser::parse(tsdl);
        QVERIFY2(!r.has_value(), "expected an out-of-range integer error");
        QVERIFY(r.error().contains(u"out of range"_s));
    }

    // A NUL byte cannot appear inside a string literal; it must be rejected
    // rather than silently truncating the string at the embedded NUL.
    void nulByteInStringLiteral()
    {
        QByteArray tsdl =
            "trace { major = 1; minor = 8; byte_order = le; test = \"aXb\"; };\n"
            "event { name = \"ev\"; fields := struct { integer { size = 8; } x; }; };\n";
        tsdl.replace('X', '\0'); // embed an actual NUL inside the "aXb" literal

        auto r = TsdlParser::parse(tsdl);
        QVERIFY2(!r.has_value(), "expected a NUL-in-string error");
        QVERIFY(r.error().contains(u"NUL"_s));
    }
};

QTEST_APPLESS_MAIN(tst_TsdlParser)
#include "tst_tsdlparser.moc"
