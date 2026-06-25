// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <debuggerprotocol.h>

#include <QTest>

//TESTED_COMPONENT=src/plugins/debugger

class tst_protocol : public QObject
{
    Q_OBJECT

public:
    tst_protocol() {}

private slots:
    void parseCString();
    void parseCString_data();
    void parseCStringLatin1();
    void parseCStringLatin1_data();
    void gdbmiFromString();
    void gdbmiFromString_data();
    void reformatUnsignedIntegerTest();
    void reformatUnsignedIntegerTest_data();
    void reformatIntegerOverload();
    void reformatIntegerOverload_data();
    void reformatCharacter();
    void reformatCharacter_data();
    void reformatCharacterWithFormat();
    void reformatCharacterWithFormat_data();
#if defined(HAVE_INT128)
    void reformatUnsignedInteger128Test();
    void reformatUnsignedInteger128Test_data();
#endif
};

void tst_protocol::parseCString()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);

    QStringDecoder decoder(QStringEncoder::Utf8);
    Debugger::Internal::DebuggerOutputParser parser(input, decoder);
    QString parsed = parser.readCString();

    QCOMPARE(parsed, expected);
}

void tst_protocol::parseCString_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("empty")
        << ""
        << "";

    QTest::newRow("unquoted")
        << "irgendwas"
        << "";

    QTest::newRow("plain")
        << R"("plain")"
        << "plain";

    // This is expected to throw several warnings
    //  "MI Parse Error, unrecognized backslash escape"
    QChar escapes[] = {'\a', '\b', '\033', '\f', '\n', '\r', '\t', '\v', '"', '\'', '\\'};
    QTest::newRow("escaped")
        << R"("\a\b\c\d\e\f\g\h\i\j\k\l\m\n\o\p\q\r\s\t\u\v\w\y\z\"\'\\")"
        << QString(escapes, sizeof(escapes)/sizeof(escapes[0]));

    QTest::newRow("octal")
        << R"("abc\303\244\303\251def\303\261")"
        << R"(abcäédefñ)";

    QTest::newRow("hex")
        << R"("abc\xc3\xa4\xc3\xa9def\xc3\xb1")"
        << R"(abcäédefñ)";

    // GDB may emit non-ASCII chars literally; the process output decoder turns
    // them into QChars before the MI parser sees them.
    QTest::newRow("literal-unicode")
        << (QString("\"przyk") + QChar(0x0142) + "ad\"")
        << (QString("przyk") + QChar(0x0142) + "ad");
}

void tst_protocol::parseCStringLatin1()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);

    QStringDecoder decoder(QStringEncoder::Latin1);
    Debugger::Internal::DebuggerOutputParser parser(input, decoder);
    QString parsed = parser.readCString();

    QCOMPARE(parsed, expected);
}

void tst_protocol::parseCStringLatin1_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    // Octal-escaped Latin-1 bytes for German umlauts: ä=\344, ö=\366, ü=\374.
    QTest::newRow("octal-latin1")
        << R"("B\344r \374ber \366")"
        << (QString("B") + QChar(0x00e4) + "r " + QChar(0x00fc) + "ber " + QChar(0x00f6));

    // GDB may emit non-ASCII chars literally; the process output decoder (here
    // Latin-1) turns them into QChars before the MI parser sees them.
    QTest::newRow("literal-latin1")
        << (QString("\"B") + QChar(0x00e4) + "r " + QChar(0x00fc) + "ber " + QChar(0x00f6) + "\"")
        << (QString("B") + QChar(0x00e4) + "r " + QChar(0x00fc) + "ber " + QChar(0x00f6));
}

void tst_protocol::gdbmiFromString()
{
    QFETCH(QString, input);

    QStringDecoder decoder(QStringEncoder::Utf8);
    Debugger::Internal::GdbMi gdbmi;
    gdbmi.fromString(input, decoder);
}

void tst_protocol::gdbmiFromString_data()
{
    QTest::addColumn<QString>("input");

    const QString testData = R"([Thread 0x7fffa2ffd640 (LWP 263820) exited]
[Thread 0x7fffa1ffb640 (LWP 263815) exited]
[Thread 0x7fffa27fc640 (LWP 263802) exited]
[Thread 0x7fff7ffff640 (LWP 263826) exited]
result={token="0",data=[{iname="local.this",name="this",origaddr="0x7fffffffa3c8",address="0x122c660",autoderefcount="1",address="0x122c660",numchild="1",type="Git::Internal::GitClient",value="",},{iname="local.workingDirectory",name="workingDirectory",numchild="1",address="0x7fffffffa598",type="QString &",valueencoded="utf16",value="2F0068006F006D0065002F006A006100720065006B002F006400650076002F00630072006500610074006F0072002D006D0061007300740065007200",},{iname="local.fileName",name="fileName",numchild="1",address="0x7fffffffa590",type="QString &",valueencoded="utf16",value="7300720063002F006C006900620073002F007500740069006C0073002F007300680065006C006C0063006F006D006D0061006E0064002E00630070007000",},{iname="local.enableAnnotationContextMenu",name="enableAnnotationContextMenu",address="0x7fffffffa3b4",numchild="0",type="bool",value="1",},{iname="local.args",name="args",numchild="0",address="0x7fffffffa588",type="QStringList &",valueencoded="itemcount",value="0",},{iname="local.msgArg",name="msgArg",numchild="1",address="0x7fffffffa408",type="QString",valueencoded="utf16",value="7300720063002F006C006900620073002F007500740069006C0073002F007300680065006C006C0063006F006D006D0061006E0064002E00630070007000",},{iname="local.workingDir",name="workingDir",numchild="1",address="0x7fffffffa400",type="QString",valueencoded="utf16",value="2F0068006F006D0065002F006A006100720065006B002F006400650076002F00630072006500610074006F0072002D006D0061007300740065007200",},{iname="local.title",name="title",numchild="1",address="0x7fffffffa3f8",type="QString",valueencoded="utf16",value="47006900740020004C006F006700200022007300720063002F006C006900620073002F007500740069006C0073002F007300680065006C006C0063006F006D006D0061006E0064002E006300700070002200",},{iname="local.editorId",name="editorId",numchild="1",address="0x7fffffffa3f0",type="Utils::Id",valueencoded="latin1",value="476974204C6F6720456469746F72",},{iname="local.sourceFile",name="sourceFile",numchild="1",address="0x7fffffffa3e8",type="QString",valueencoded="utf16",value="2F0068006F006D0065002F006A006100720065006B002F006400650076002F00630072006500610074006F0072002D006D00610073007400650072002F007300720063002F006C006900620073002F007500740069006C0073002F007300680065006C006C0063006F006D006D0061006E0064002E00630070007000",},{iname="local.editor",name="editor",origaddr="0x7fffffffa520",address="0x27ff400",numchild="1",type="Git::Internal::GitEditorWidget*",value="0x27ff400",},{iname="local.argWidget",name="argWidget",address="0x1fe47c0",numchild="1",type="Git::Internal::GitLogArgumentsWidget*",value="0x1fe47c0",},{iname="local.arguments",name="arguments",numchild="11",address="0x7fffffffa3e0",type="QStringList",valueencoded="itemcount",value="11",},{iname="local.logCount",name="logCount",address="0x7fffffffa51c",numchild="0",type="int",value="100",},{iname="local.grepValue",name="grepValue",address="0x7fffffffa3d8",type="QString",valueencoded="utf16",value="",},{iname="local.pickaxeValue",name="pickaxeValue",address="0x7fffffffa3d0",type="QString",valueencoded="utf16",value="",},{iname="watch.0",wname="266D5F6D75746578",numchild="0",type=" ",value="<no such value>",},{iname="watch.1",wname="266D6F64656C4D616E616765722D3E6D5F6D75746578",numchild="0",type=" ",value="<no such value>",},{iname="watch.2",wname="62696E64696E672D3E73796D626F6C732829",numchild="0",type=" ",value="<no such value>",},{iname="watch.3",wname="64",origaddr="0x7fffc5c0e630",address="0xfe5100",numchild="1",type="VcsBase::VcsOutputWindowPrivate*",value="0xfe5100",},{iname="watch.4",wname="642D3E636F6E74656E7473",numchild="0",type=" ",value="<no such value>",},{iname="watch.5",wname="642D3E64697361626C65436F6465506172736572",numchild="0",type=" ",value="<no such value>",},{iname="watch.6",wname="642D3E6D5F63757272656E74456469746F72",numchild="0",type=" ",value="<no such value>",},{iname="watch.7",wname="642D3E6D5F6578747261456469746F72537570706F727473",numchild="0",type=" ",value="<no such value>",},{iname="watch.8",wname="642D3E6D5F6C6F6164696E6753657373696F6E",numchild="0",type=" ",value="<no such value>",},{iname="watch.9",wname="64656C61796564496E697469616C697A655175657565",numchild="0",type=" ",value="<no such value>",},{iname="watch.10",wname="66756E6374696F6E",numchild="0",type=" ",value="<no such value>",},{iname="watch.11",wname="69636F6E54797065",numchild="0",type=" ",value="<no such value>",},{iname="watch.12",wname="6974656D416464",numchild="0",type=" ",value="<no such value>",},{iname="watch.13",wname="6D5F616363657074",numchild="0",type=" ",value="<no such value>",},{iname="watch.14",wname="6D5F6461746143616C6C6261636B",numchild="0",type=" ",value="<no such value>",},{iname="watch.15",wname="6D5F67656E65726174656446696C654E616D65",numchild="0",type=" ",value="<no such value>",},{iname="watch.16",wname="6D5F6E65656473557064617465",numchild="0",type=" ",value="<no such value>",},{iname="watch.17",wname="6D5F717263436F6E74656E7473",numchild="0",type=" ",value="<no such value>",},{iname="watch.18",wname="6D6F64656C4D616E61676572",numchild="0",type=" ",value="<no such value>",},{iname="watch.19",wname="6E616D65",numchild="0",type=" ",value="<no such value>",},{iname="watch.20",wname="706172736572",numchild="0",type=" ",value="<no such value>",},{iname="watch.21",wname="7061727365722E746F",numchild="0",type=" ",value="<no such value>",},{iname="watch.22",wname="7265736F6C7665546172676574",numchild="0",type=" ",value="<no such value>",},{iname="watch.23",wname="73796D626F6C2D3E656E636C6F73696E6753636F70652829",numchild="0",type=" ",value="<no such value>",},{iname="watch.24",wname="7461675F6E616D65",numchild="0",type=" ",value="<no such value>",},{iname="watch.25",wname="74797065",numchild="0",type=" ",value="<no such value>",},],typeinfo=[],partial="0",counts={},timings=[]} )";

    QTest::newRow("with_thread_finished")
            << testData;
}

using namespace Debugger::Internal;

void tst_protocol::reformatUnsignedIntegerTest()
{
    QFETCH(quint64, value);
    QFETCH(int, format);
    QFETCH(QString, expected);

    QCOMPARE(reformatUnsignedInteger(value, format), expected);
}

void tst_protocol::reformatUnsignedIntegerTest_data()
{
    QTest::addColumn<quint64>("value");
    QTest::addColumn<int>("format");
    QTest::addColumn<QString>("expected");

    QTest::newRow("decimal")
        << quint64(97) << int(DecimalIntegerFormat) << QString("97");
    QTest::newRow("hex")
        << quint64(0x41) << int(HexadecimalIntegerFormat) << QString("(hex) 41");
    QTest::newRow("binary")
        << quint64(7) << int(BinaryIntegerFormat) << QString("(bin) 111");
    QTest::newRow("octal")
        << quint64(0xff) << int(OctalIntegerFormat) << QString("(oct) 377");
    QTest::newRow("charcode-A")
        << quint64(0x41) << int(CharCodeIntegerFormat) << QString("'\\0\\0\\0A'");
    QTest::newRow("charcode-AB")
        << quint64(0x4142) << int(CharCodeIntegerFormat) << QString("'\\0\\0AB'");
}

void tst_protocol::reformatIntegerOverload()
{
    QFETCH(quint64, value);
    QFETCH(int, format);
    QFETCH(int, size);
    QFETCH(bool, isSigned);
    QFETCH(QString, expected);

    QCOMPARE(reformatInteger(value, format, size, isSigned), expected);
}

void tst_protocol::reformatIntegerOverload_data()
{
    QTest::addColumn<quint64>("value");
    QTest::addColumn<int>("format");
    QTest::addColumn<int>("size");
    QTest::addColumn<bool>("isSigned");
    QTest::addColumn<QString>("expected");

    // Masking: 0x1ff masked to 1 byte = 0xff = 255
    QTest::newRow("mask-1byte")
        << quint64(0x1ff) << int(AutomaticFormat) << 1 << false << QString("255");
    QTest::newRow("mask-2byte")
        << quint64(0x10041) << int(AutomaticFormat) << 2 << false << QString("65");
    QTest::newRow("hex-1byte")
        << quint64(0x41) << int(HexadecimalIntegerFormat) << 1 << false << QString("(hex) 41");
    // Non-decimal formats force unsigned display
    QTest::newRow("hex-signed-becomes-unsigned")
        << quint64(0xff) << int(HexadecimalIntegerFormat) << 1 << true << QString("(hex) ff");
    // Decimal signed: value is already masked, sign extension not applied inside
    QTest::newRow("decimal-unsigned")
        << quint64(42) << int(DecimalIntegerFormat) << 4 << false << QString("42");
}

void tst_protocol::reformatCharacter()
{
    QFETCH(int, code);
    QFETCH(int, size);
    QFETCH(bool, isSigned);
    QFETCH(QString, expected);

    QCOMPARE(Debugger::Internal::reformatCharacter(code, size, isSigned), expected);
}

void tst_protocol::reformatCharacter_data()
{
    QTest::addColumn<int>("code");
    QTest::addColumn<int>("size");
    QTest::addColumn<bool>("isSigned");
    QTest::addColumn<QString>("expected");

    // Printable ASCII: trailing space after the character, then tab
    QTest::newRow("A-unsigned")
        << int('A') << 1 << false << QString("'A' \t65\t0x41");
    QTest::newRow("A-signed")
        << int('A') << 1 << true << QString("'A' \t65    \t0x41");
    QTest::newRow("nul")
        << 0 << 1 << false << QString("'\\0'\t0\t0x00");
    QTest::newRow("newline")
        << int('\n') << 1 << false << QString("'\\n'\t10\t0x0a");
    QTest::newRow("cr")
        << int('\r') << 1 << false << QString("'\\r'\t13\t0x0d");
    QTest::newRow("tab")
        << int('\t') << 1 << false << QString("'\\t'\t9\t0x09");
    // Non-printable, non-special: four spaces before the tab
    QTest::newRow("bell")
        << int('\a') << 1 << false << QString("    \t7\t0x07");
    QTest::newRow("signed-minus8")
        << int(-8) << 1 << true
        << (QString("'") + QChar(0xf8) + "' \t-8/248\t0xf8");
    QTest::newRow("signed-minus1")
        << int(-1) << 1 << true
        << (QString("'") + QChar(0xff) + "' \t-1/255\t0xff");
    QTest::newRow("unsigned-minus40")
        << int(-40) << 1 << false
        << (QString("'") + QChar(0xd8) + "' \t216\t0xd8");

    // Debugger reports value as unsigned. Function must re-interpret as signed.
    QTest::newRow("signed-from-unsigned-size1")
        << 216 << 1 << true
        << (QString("'") + QChar(0xd8) + "' \t-40/216\t0xd8");
    QTest::newRow("signed-from-unsigned-size2")
        << 0x8000 << 2 << true
        << (QString("'") + QChar(0x8000) + "' \t-32768/32768\t0x8000");
}

void tst_protocol::reformatCharacterWithFormat()
{
    QFETCH(int, code);
    QFETCH(int, size);
    QFETCH(bool, isSigned);
    QFETCH(int, format);
    QFETCH(QString, expected);

    QCOMPARE(Debugger::Internal::reformatCharacterWithFormat(code, size, isSigned, format),
             expected);
}

void tst_protocol::reformatCharacterWithFormat_data()
{
    QTest::addColumn<int>("code");
    QTest::addColumn<int>("size");
    QTest::addColumn<bool>("isSigned");
    QTest::addColumn<int>("format");
    QTest::addColumn<QString>("expected");

    QTest::newRow("auto-A")
        << int('A') << 1 << false << int(AutomaticFormat)
        << QString("'A' \t65\t0x41");

    QTest::newRow("decimal-A")
        << int('A') << 1 << false << int(DecimalIntegerFormat)
        << QString("65");

    QTest::newRow("decimal-signed-minus8")
        << int(-8) << 1 << true << int(DecimalIntegerFormat)
        << QString("248");

    QTest::newRow("hex-A")
        << int('A') << 1 << false << int(HexadecimalIntegerFormat)
        << QString("(hex) 41");
}

#if defined(HAVE_INT128)
void tst_protocol::reformatUnsignedInteger128Test()
{
    QFETCH(QString, decimalValue);
    QFETCH(int, format);
    QFETCH(QString, expected);

    // Parse decimal string to unsigned __int128 (mirroring the watchhandler.cpp logic)
    unsigned __int128 uval = 0;
    for (QChar c : decimalValue)
        uval = uval * 10 + unsigned(c.unicode() - '0');

    QCOMPARE(reformatUnsignedInteger128(uval, format), expected);
}

void tst_protocol::reformatUnsignedInteger128Test_data()
{
    QTest::addColumn<QString>("decimalValue");
    QTest::addColumn<int>("format");
    QTest::addColumn<QString>("expected");

    // 2^64 = 18446744073709551616
    QTest::newRow("2pow64-hex")
        << "18446744073709551616" << int(HexadecimalIntegerFormat)
        << QString("(hex) 10000000000000000");
    QTest::newRow("2pow64-dec")
        << "18446744073709551616" << int(DecimalIntegerFormat)
        << QString("18446744073709551616");
    QTest::newRow("2pow64-bin")
        << "18446744073709551616" << int(BinaryIntegerFormat)
        << QString("(bin) 10000000000000000000000000000000000000000000000000000000000000000");
    QTest::newRow("2pow64-oct")
        << "18446744073709551616" << int(OctalIntegerFormat)
        << QString("(oct) 2000000000000000000000");

    // UINT128_MAX = 2^128 - 1 = 340282366920938463463374607431768211455
    QTest::newRow("uint128max-hex")
        << "340282366920938463463374607431768211455" << int(HexadecimalIntegerFormat)
        << QString("(hex) ffffffffffffffffffffffffffffffff");
    QTest::newRow("uint128max-dec")
        << "340282366920938463463374607431768211455" << int(DecimalIntegerFormat)
        << QString("340282366920938463463374607431768211455");
}
#endif

QTEST_APPLESS_MAIN(tst_protocol);

#include "tst_protocol.moc"

