/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "disassemblerlines.h"

#include <QtTest>

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

private:
    DisassemblerLines lines;
};

void tst_disassembler::parse()
{
    QFETCH(QString, raw);
    QFETCH(QString, cooked);
    QFETCH(Debugger::Internal::DisassemblerLine, line);

    lines.appendUnparsed(raw);
    DisassemblerLine parsed = lines.at(lines.size() - 1);

    QCOMPARE(parsed.address, line.address);
    QCOMPARE(parsed.function, line.function);
    QCOMPARE(parsed.offset, line.offset);
    QCOMPARE(parsed.lineNumber, line.lineNumber);
    QCOMPARE(parsed.rawData, line.rawData);
    QCOMPARE(parsed.data, line.data);

    QString out___ = parsed.toString();
    QCOMPARE(out___, cooked);
}

void tst_disassembler::parse_data()
{
    QTest::addColumn<QString>("raw");
    QTest::addColumn<QString>("cooked");
    QTest::addColumn<Debugger::Internal::DisassemblerLine>("line");

    DisassemblerLine line;

    line.address = 0x40f39e;
    line.offset  = 18;
    line.data    = "mov    %rax,%rdi";
    QTest::newRow("plain")
           << "0x000000000040f39e <+18>:\tmov    %rax,%rdi"
           << "0x40f39e  <+0x0012>         mov    %rax,%rdi"
           << line;

    line.address = 0x40f3a1;
    line.offset  = 21;
    line.data    = "callq  0x420d2c <_ZN7qobject5Names3Bar10TestObjectC2EPN4Myns7QObjectE>";
    QTest::newRow("call")
           << "0x000000000040f3a1 <+21>:\tcallq  0x420d2c <_ZN7qobject5Names3Bar10TestObjectC2EPN4Myns7QObjectE>"
           << "0x40f3a1  <+0x0015>         callq  0x420d2c <_ZN7qobject5Names3Bar10TestObjectC2EPN4Myns7QObjectE>"
           << line;


    line.address = 0x000000000041cd73;
    line.offset  = 0;
    line.data    = "mov    %rax,%rdi";
    QTest::newRow("set print max-symbolic-offset 1, plain")
            << "0x000000000041cd73:\tmov    %rax,%rdi"
            << "0x41cd73                    mov    %rax,%rdi"
            << line;

    line.address = 0x000000000041cd73;
    line.offset  = 0;
    line.data    = "callq  0x420d2c <_ZN4Myns12QApplicationC1ERiPPci@plt>";
    QTest::newRow("set print max-symbolic-offset 1, call")
            << "0x00000000041cd73:\tcallq  0x420d2c <_ZN4Myns12QApplicationC1ERiPPci@plt>"
            << "0x41cd73                    callq  0x420d2c <_ZN4Myns12QApplicationC1ERiPPci@plt>"
            << line;
 }


QTEST_APPLESS_MAIN(tst_disassembler);

#include "tst_disassembler.moc"

