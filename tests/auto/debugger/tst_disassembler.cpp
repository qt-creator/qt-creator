// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "disassemblerlines.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

//TESTED_COMPONENT=src/plugins/debugger/gdb

Q_DECLARE_METATYPE(Debugger::Internal::DisassemblerLine)

using namespace Debugger::Internal;

class tst_disassembler : public QObject
{
    Q_OBJECT

public:
    tst_disassembler() {}

private slots:
    void parse();
    void parse_data();
    void mixedSourceNamesItsFile();
    void appendedSourceLineIsTheOneAsked();

private:
    DisassemblerLines lines;
};

void tst_disassembler::parse()
{
    QFETCH(QString, raw);
    QFETCH(QString, cooked);
    QFETCH(int, bytes);
    QFETCH(Debugger::Internal::DisassemblerLine, line);

    lines.appendUnparsed(raw);
    DisassemblerLine parsed = lines.at(lines.size() - 1);

    QCOMPARE(parsed.address, line.address);
    QCOMPARE(parsed.function, line.function);
    QCOMPARE(parsed.offset, line.offset);
    QCOMPARE(parsed.lineNumber, line.lineNumber);
    QCOMPARE(parsed.rawData, line.rawData);
    QCOMPARE(parsed.data, line.data);

    QString out___ = parsed.toString(bytes);
    QCOMPARE(out___, cooked);
}

void tst_disassembler::parse_data()
{
    QTest::addColumn<QString>("raw");
    QTest::addColumn<QString>("cooked");
    QTest::addColumn<int>("bytes");
    QTest::addColumn<Debugger::Internal::DisassemblerLine>("line");

    DisassemblerLine line;

    line.address = 0x40f39e;
    line.offset  = 18;
    line.data    = "mov    %rax,%rdi";
    QTest::newRow("plain")
           << "0x000000000040f39e <+18>:\tmov    %rax,%rdi"
           << "0x40f39e  <+   18>         mov    %rax,%rdi"
           << 0 << line;

    line.address = 0x40f3a1;
    line.offset  = 21;
    line.data    = "callq  0x420d2c <_ZN7qobject5Names3Bar10TestObjectC2EPN4Myns7QObjectE>";
    QTest::newRow("call")
           << "0x000000000040f3a1 <+21>:\tcallq  "
                "0x420d2c <_ZN7qobject5Names3Bar10TestObjectC2EPN4Myns7QObjectE>"
           << "0x40f3a1  <+   21>         callq  "
                "0x420d2c <_ZN7qobject5Names3Bar10TestObjectC2EPN4Myns7QObjectE>"
           << 0 << line;


    line.address = 0x000000000041cd73;
    line.offset  = 0;
    line.data    = "mov    %rax,%rdi";
    QTest::newRow("set print max-symbolic-offset 1, plain")
            << "0x000000000041cd73:\tmov    %rax,%rdi"
            << "0x41cd73                   mov    %rax,%rdi"
            << 0 << line;

    line.address = 0x000000000041cd73;
    line.offset  = 0;
    line.data    = "callq  0x420d2c <_ZN4Myns12QApplicationC1ERiPPci@plt>";
    QTest::newRow("set print max-symbolic-offset 1, call")
            << "0x00000000041cd73:\tcallq  0x420d2c <_ZN4Myns12QApplicationC1ERiPPci@plt>"
            << "0x41cd73                   callq  0x420d2c <_ZN4Myns12QApplicationC1ERiPPci@plt>"
            << 0 << line;

    // With raw bytes:
    line.address  = 0x00000000004010d3;
    line.offset   = 0;
    line.function = "main()";
    line.offset   = 132;
    line.bytes    = "48 89 c7";
    line.data     = "mov    %rax,%rdi";
    QTest::newRow("with raw bytes")
            << "   0x00000000004010d3 <main()+132>:\t48 89 c7\tmov    %rax,%rdi"
            << "0x4010d3  <+  132>        48 89 c7   mov    %rax,%rdi"
            << 10 << line;
 }


void tst_disassembler::mixedSourceNamesItsFile()
{
    const QString dump =
        "Dump of assembler code for function main():\n"
        "main.cpp:\n"
        "5\t{\n"
        "   0x0000000000401126 <+0>:\tpush   %rbp\n"
        "Address range 0x401130 to 0x401140:\n"
        "6\t  return answer();\n"
        "   0x000000000040112a <+4>:\tmov    $0x2a,%eax\n"
        "\n"
        "/usr/include/answer.h:\n"
        "10\t  return 42;\n"
        "   0x000000000040112f <+9>:\tret\n"
        "End of assembler dump.\n";
    const DisassemblerLines lines = parseCliDisassembly(dump);

    QStringList sources;
    for (int i = 0; i < lines.size(); ++i) {
        const DisassemblerLine &line = lines.at(i);
        if (!line.isAssembler() && line.isCode())
            sources.append(line.fileName + ':' + QString::number(line.lineNumber));
    }
    QCOMPARE(sources, QStringList({"main.cpp:5", "main.cpp:6", "/usr/include/answer.h:10"}));
}

void tst_disassembler::appendedSourceLineIsTheOneAsked()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString fileName = dir.filePath("main.cpp");
    QFile file(fileName);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("first\nsecond\nthird\n");
    file.close();

    DisassemblerLines lines;
    lines.appendSourceLine(fileName, 1);
    lines.appendSourceLine(fileName, 3);
    QCOMPARE(lines.size(), 2);
    QCOMPARE(lines.at(0).lineNumber, 1);
    QCOMPARE(lines.at(0).data, QString("first"));
    QCOMPARE(lines.at(0).fileName, fileName);
    QCOMPARE(lines.at(1).lineNumber, 3);
    QCOMPARE(lines.at(1).data, QString("third"));

    lines.appendSourceLine(fileName, 5);
    QCOMPARE(lines.size(), 2);
}

QTEST_APPLESS_MAIN(tst_disassembler);

#include "tst_disassembler.moc"

