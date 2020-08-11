/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

QTEST_APPLESS_MAIN(tst_protocol);

#include "tst_protocol.moc"

