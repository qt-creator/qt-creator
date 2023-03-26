// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <debuggerprotocol.h>

#include <QtTest>

//TESTED_COMPONENT=src/plugins/debugger

class tst_protocol : public QObject
{
    Q_OBJECT

public:
    tst_protocol() {}

private slots:
    void parseCString();
    void parseCString_data();
    void gdbmiFromString();
    void gdbmiFromString_data();
};

void tst_protocol::parseCString()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);

    Debugger::Internal::DebuggerOutputParser parser(input);
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
    QChar escapes[] = {'\a', '\b', '\f', '\n', '\r', '\t', '\v', '"', '\'', '\\'};
    QTest::newRow("escaped")
        << R"("\a\b\c\d\e\f\g\h\i\j\k\l\m\n\o\p\q\r\s\t\u\v\w\y\z\"\'\\")"
        << QString(escapes, sizeof(escapes)/sizeof(escapes[0]));

    QTest::newRow("octal")
        << R"("abc\303\244\303\251def\303\261")"
        << R"(abcäédefñ)";

    QTest::newRow("hex")
        << R"("abc\xc3\xa4\xc3\xa9def\xc3\xb1")"
        << R"(abcäédefñ)";
}

void tst_protocol::gdbmiFromString()
{
    QFETCH(QString, input);

    Debugger::Internal::GdbMi gdbmi;
    gdbmi.fromString(input);
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

QTEST_APPLESS_MAIN(tst_protocol);

#include "tst_protocol.moc"

