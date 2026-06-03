// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Reproduction / regression tests for robustness issues found in the
// CommonTraceFormat library: malformed-input handling, integer overflow,
// unbounded loops/allocations, and out-of-bounds writes. Each test documents
// the failure scenario it guards against.

#include <commontraceformat/binary/bitbuffer.h>
#include <commontraceformat/binary/fieldreader.h>
#include <commontraceformat/binary/packetreader.h>
#include <commontraceformat/metadata/ctf1packets.h>
#include <commontraceformat/metadata/metadatareader.h>
#include <commontraceformat/metadata/tsdlparser.h>

#include <commontraceformat/schema/scalarfieldclasses.h>
#include <commontraceformat/schema/compoundfieldclasses.h>
#include <commontraceformat/schema/datastreamclass.h>
#include <commontraceformat/schema/eventrecordclass.h>
#include <commontraceformat/stream/datastreamreader.h>

#include <utils/result.h>

#include <QBuffer>
#include <QtTest>

using namespace CommonTraceFormat;
using namespace Qt::StringLiterals;

namespace {

// Append a 32-bit little-endian value to a byte array.
void appendU32Le(QByteArray &b, quint32 v)
{
    b.append(char(v & 0xFF));
    b.append(char((v >> 8) & 0xFF));
    b.append(char((v >> 16) & 0xFF));
    b.append(char((v >> 24) & 0xFF));
}

// Overwrite a 32-bit little-endian value at a fixed offset.
void putU32Le(QByteArray &b, int off, quint32 v)
{
    b[off + 0] = char(v & 0xFF);
    b[off + 1] = char((v >> 8) & 0xFF);
    b[off + 2] = char((v >> 16) & 0xFF);
    b[off + 3] = char((v >> 24) & 0xFF);
}

// A minimal read-only sequential QIODevice (pipe/socket-like): no seeking, the
// stream position only ever moves forward. Used to exercise the code paths that
// cannot rewind the device after an over-read.
class SequentialDevice : public QIODevice
{
public:
    explicit SequentialDevice(QByteArray data) : m_data(std::move(data)) {}

    bool isSequential() const override { return true; }
    bool atEnd() const override { return m_pos >= m_data.size(); }
    qint64 bytesAvailable() const override
    {
        return (m_data.size() - m_pos) + QIODevice::bytesAvailable();
    }

protected:
    qint64 readData(char *out, qint64 maxlen) override
    {
        const qint64 n = qMin<qint64>(maxlen, m_data.size() - m_pos);
        if (n > 0) {
            memcpy(out, m_data.constData() + m_pos, n);
            m_pos += n;
        }
        return n;
    }
    qint64 writeData(const char *, qint64) override { return -1; }

private:
    QByteArray m_data;
    qint64     m_pos = 0;
};

// A packet context field class: { packet_size:u32, content_size:u32 } carrying
// the total/content-length roles. Lengths are expressed in *bits*.
FieldClassPtr makePacketContext()
{
    auto pktSize = std::make_shared<FixedLengthUIntFC>();
    pktSize->length = 32;
    pktSize->roles.append(UIntRole::PacketTotalLength);

    auto contentSize = std::make_shared<FixedLengthUIntFC>();
    contentSize->length = 32;
    contentSize->roles.append(UIntRole::PacketContentLength);

    auto sfc = std::make_shared<StructureFC>();
    sfc->members.append({u"packet_size"_s, pktSize, {}});
    sfc->members.append({u"content_size"_s, contentSize, {}});
    return sfc;
}

// A data stream class whose single event record class (id 0) has an 8-bit
// payload field, so each event consumes exactly one byte.
DataStreamClass makeOneBytePayloadDsc()
{
    DataStreamClass dsc;
    dsc.id = 0;
    dsc.packetContextFieldClass = makePacketContext();

    auto payloadField = std::make_shared<FixedLengthUIntFC>();
    payloadField->length = 8;
    auto payload = std::make_shared<StructureFC>();
    payload->members.append({u"v"_s, payloadField, {}});

    EventRecordClass erc;
    erc.id = 0;
    erc.payloadFieldClass = payload;
    dsc.eventRecordClasses.append(erc);
    return dsc;
}

// One on-wire packet for makeOneBytePayloadDsc: 8-byte context + one payload
// byte = 9 bytes, content==total==72 bits.
QByteArray makeOneEventPacket(quint8 payload)
{
    QByteArray b;
    appendU32Le(b, 72); // packet_size (bits)
    appendU32Le(b, 72); // content_size (bits)
    b.append(char(payload));
    return b;
}

// Build a CTF 2 metadata stream from JSON fragments, each prefixed with the
// mandatory record-separator (0x1E) byte.
QByteArray makeMetadata(std::initializer_list<QByteArray> fragments)
{
    QByteArray out;
    for (const QByteArray &f : fragments) {
        out.append(char(0x1E));
        out.append(f);
        out.append('\n');
    }
    return out;
}

} // namespace

class tst_Robustness : public QObject
{
    Q_OBJECT
private slots:
    // ── BitBuffer ──────────────────────────────────────────────────────────

    // alignWriteTo must advance the write cursor to the requested alignment even
    // when the required padding exceeds 64 bits (writeBits rejects counts > 64,
    // so a single padding write silently does nothing pre-fix).
    void alignWriteTo_largePadding()
    {
        QBuffer dev;
        dev.open(QIODevice::WriteOnly);
        BitBuffer wb(&dev);
        wb.writeBits(1, 1, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
        QCOMPARE(wb.writeBitOffset(), 1);
        wb.alignWriteTo(128); // needs 127 bits of padding
        QCOMPARE(wb.writeBitOffset(), 128);
    }

    // patchBits with an offset past the end of the buffer must not write out of
    // bounds (heap corruption). After the fix it is a no-op and leaves the
    // buffer untouched.
    void patchBits_outOfBounds()
    {
        QBuffer dev;
        dev.open(QIODevice::WriteOnly);
        BitBuffer wb(&dev);
        wb.writeBits(0, 8, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
        // Offset 1000 bits lies far past the 8-bit buffer.
        wb.patchBits(1000, 0xFFFFFFFFu, 32, ByteOrder::LittleEndian,
                     BitOrder::LeastSignificantFirst);
        QVERIFY_RESULT(wb.flush());
        QCOMPARE(dev.data().size(), 1); // still a single byte, unchanged
        QCOMPARE(static_cast<quint8>(dev.data().at(0)), 0u);
    }

    // ── FieldReader ────────────────────────────────────────────────────────

    // A ULEB128 field must reject after at most ceil(64/7)=10 bytes rather than
    // consuming (and buffering) the whole packet content area.
    void leb128_runawayBounded()
    {
        QByteArray data(20, char(0xFF)); // every byte has the continuation bit set
        BitBuffer buf(data);
        buf.setContentEndBit(data.size() * 8LL);

        auto fc = std::make_shared<VariableLengthUIntFC>();
        FieldReader fr(buf);
        auto r = fr.read(*fc);
        QVERIFY(!r); // never a valid 64-bit ULEB128
        // The reader must have given up well before the end of the buffer.
        QVERIFY2(buf.readBitOffset() <= 10 * 8,
                 qPrintable(u"consumed %1 bits"_s.arg(buf.readBitOffset())));
    }

    // A dynamic-length array of zero-width elements with a huge length field must
    // not loop (near-)forever appending empty values; it must error out.
    void dynamicArray_zeroWidthHugeLength()
    {
        QByteArray data(8, char(0xFF)); // length field = 0xFFFFFFFFFFFFFFFF
        BitBuffer buf(data);
        buf.setContentEndBit(data.size() * 8LL);

        auto len = std::make_shared<FixedLengthUIntFC>();
        len->length = 64;

        auto emptyElem = std::make_shared<StructureFC>(); // zero-width

        auto arr = std::make_shared<DynamicLengthArrayFC>();
        arr->elementFieldClass = emptyElem;
        arr->lengthFieldLocation.hasOrigin = false;
        arr->lengthFieldLocation.path = {std::optional<QString>(u"len"_s)};

        auto root = std::make_shared<StructureFC>();
        root->members.append({u"len"_s, len, {}});
        root->members.append({u"arr"_s, arr, {}});

        FieldReader fr(buf);
        auto r = fr.read(*root);
        QVERIFY(!r);
    }

    // ── TSDL parser ──────────────────────────────────────────────────────────

    // A fixed-length integer size greater than 64 bits is unsupported and must be
    // rejected at parse time instead of producing a schema that fails on read.
    void tsdl_integerSizeTooLarge()
    {
        auto r = TsdlParser::parse("typealias integer { size = 128; align = 8; } := big;\n");
        QVERIFY(!r);
    }

    // A size of 0 is equally invalid.
    void tsdl_integerSizeZero()
    {
        auto r = TsdlParser::parse("typealias integer { size = 0; align = 8; } := empty;\n");
        QVERIFY(!r);
    }

    // Circular type aliases must be rejected (or simply impossible to express)
    // rather than recursing without bound. This must return promptly.
    void tsdl_circularAliasTerminates()
    {
        auto r = TsdlParser::parse("typealias A := B;\ntypealias B := A;\n");
        QVERIFY(!r); // unresolved forward reference, not a hang/crash
    }

    // An alignment that is not a positive power of two must be rejected at parse
    // time (mirrors the JSON metadata reader), rather than entering the schema.
    void tsdl_alignmentNotPowerOfTwo()
    {
        QVERIFY(!TsdlParser::parse("typealias integer { size = 8; align = 3; } := x;\n"));
        QVERIFY(!TsdlParser::parse("typealias integer { size = 8; align = 0; } := x;\n"));
        // A valid power-of-two alignment still parses.
        QVERIFY(TsdlParser::parse("typealias integer { size = 8; align = 8; } := x;\n"));
    }

    // A preamble uuid element outside [0, 255] must be a parse error, not
    // silently truncated on the narrowing cast to a byte.
    void metadata_uuidByteOutOfRange()
    {
        QByteArray meta = "\x1e{\"type\":\"preamble\",\"version\":2,"
                          "\"uuid\":[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,256]}\n";
        QBuffer dev(&meta);
        dev.open(QIODevice::ReadOnly);
        MetadataReader reader(&dev);
        QVERIFY(!reader.read());
    }

    // An unknown unsigned-integer role string must be rejected at parse time
    // rather than silently mapped to UIntRole::None: a dropped role makes the
    // field look role-less, so a typo'd or unsupported role slips past
    // validateRootRoles and misparses the trace.
    void metadata_unknownUIntRole()
    {
        QByteArray meta = makeMetadata({
            R"({"type":"preamble","version":2})",
            R"({"type":"trace-class","packet-header-field-class":{"type":"structure",)"
            R"("member-classes":[{"name":"magic","field-class":{)"
            R"("type":"fixed-length-unsigned-integer","length":32,)"
            R"("byte-order":"little-endian","roles":["bogus-role"]}}]}})",
        });
        QBuffer dev(&meta);
        dev.open(QIODevice::ReadOnly);
        MetadataReader reader(&dev);
        QVERIFY(!reader.read());
    }

    // Likewise, an unknown role on a static-length BLOB (the only field class
    // with BLOB roles) must be rejected, not silently dropped.
    void metadata_unknownBlobRole()
    {
        QByteArray meta = makeMetadata({
            R"({"type":"preamble","version":2})",
            R"({"type":"trace-class","packet-header-field-class":{"type":"structure",)"
            R"("member-classes":[{"name":"uuid","field-class":{)"
            R"("type":"static-length-blob","length":16,"roles":["bogus-role"]}}]}})",
        });
        QBuffer dev(&meta);
        dev.open(QIODevice::ReadOnly);
        MetadataReader reader(&dev);
        QVERIFY(!reader.read());
    }

    // Guard against over-strict role parsing: a known-valid role must still be
    // accepted (packet-magic-number on a first, 32-bit packet-header member).
    void metadata_validRoleAccepted()
    {
        QByteArray meta = makeMetadata({
            R"({"type":"preamble","version":2})",
            R"({"type":"trace-class","packet-header-field-class":{"type":"structure",)"
            R"("member-classes":[{"name":"magic","field-class":{)"
            R"("type":"fixed-length-unsigned-integer","length":32,)"
            R"("byte-order":"little-endian","roles":["packet-magic-number"]}}]}})",
        });
        QBuffer dev(&meta);
        dev.open(QIODevice::ReadOnly);
        MetadataReader reader(&dev);
        QVERIFY_RESULT(reader.read());
    }

    // ── CTF 1.8 metadata packets ─────────────────────────────────────────────

    // content_size > packet_size must be rejected; otherwise the padding
    // computation underflows a quint32 and skips ~4 GiB, misaligning the stream.
    void ctf1_contentLargerThanPacket()
    {
        QByteArray pkt(37, char(0)); // CTF1_PACKET_HDR_SIZE
        putU32Le(pkt, 0, 0x75D11D57u);  // LE magic
        putU32Le(pkt, 24, 100 * 8);     // content_size = 100 bytes (bits)
        putU32Le(pkt, 28, 40 * 8);      //  packet_size =  40 bytes (bits)
        pkt.append(QByteArray(63, char('x'))); // tsdl payload (contentBytes-37)

        QBuffer dev(&pkt);
        dev.open(QIODevice::ReadOnly);
        auto r = readCTF1TsdlText(&dev);
        QVERIFY(!r);
    }

    // ── PacketReader / DataStreamReader ──────────────────────────────────────

    // A packet whose declared total length exceeds the bytes actually available
    // (truncated/slow device) must surface as an error, not be processed as if
    // complete.
    void packet_truncatedDeviceErrors()
    {
        QByteArray pkt;
        appendU32Le(pkt, 64 * 8); // packet_size says 64 bytes...
        appendU32Le(pkt, 64 * 8); // content_size says 64 bytes...
        pkt.append(QByteArray(12, char(0))); // ...but only 20 bytes are present

        QBuffer dev(&pkt);
        dev.open(QIODevice::ReadOnly);

        DataStreamClass dsc;
        dsc.id = 0;
        dsc.packetContextFieldClass = makePacketContext();

        PacketReader reader(&dev, dsc);
        auto r = reader.openPacket();
        QVERIFY(!r);
    }

    // On a sequential (non-seekable) device, an over-read while probing one
    // packet must not discard the bytes belonging to the next packet.
    void packet_sequentialOverReadPreservesNext()
    {
        QByteArray stream = makeOneEventPacket(0xAA) + makeOneEventPacket(0xBB);
        SequentialDevice dev(std::move(stream));
        dev.open(QIODevice::ReadOnly);

        DataStreamClass dsc = makeOneBytePayloadDsc();
        DataStreamReader reader(&dev, dsc);

        int events = 0;
        while (!reader.atEnd()) {
            auto evt = reader.nextEvent();
            if (!evt)
                break;
            ++events;
            QVERIFY(events <= 10); // guard against accidental loops
        }
        QCOMPARE(events, 2);
    }
};

QTEST_APPLESS_MAIN(tst_Robustness)
#include "tst_robustness.moc"
