/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "ansifilterparser.h"

namespace {
enum AnsiState {
    PLAIN,
    ANSI_START,
    ANSI_CSI,
    ANSI_SEQUENCE,
    ANSI_WAITING_FOR_ST,
    ANSI_ST_STARTED
};
} // namespace

using namespace ProjectExplorer;

AnsiFilterParser::AnsiFilterParser()
{
    setObjectName(QLatin1String("AnsiFilterParser"));
}

void AnsiFilterParser::stdOutput(const QString &line)
{
    IOutputParser::stdOutput(filterLine(line));
}

void AnsiFilterParser::stdError(const QString &line)
{
    IOutputParser::stdError(filterLine(line));
}

QString AnsiFilterParser::filterLine(const QString &line)
{
    QString result;
    result.reserve(line.count());

    static AnsiState state = PLAIN;
    foreach (const QChar c, line) {
        unsigned int val = c.unicode();
        switch (state) {
        case PLAIN:
            if (val == 27) // 'ESC'
                state = ANSI_START;
            else if (val == 155) // equivalent to 'ESC'-'['
                state = ANSI_CSI;
            else
                result.append(c);
            break;
        case ANSI_START:
            if (val == 91) // [
                state = ANSI_CSI;
            else if (val == 80 || val == 93 || val == 94 || val == 95) // 'P', ']', '^' and '_'
                state = ANSI_WAITING_FOR_ST;
            else if (val >= 64 && val <= 95)
                state = PLAIN;
            else
                state = ANSI_SEQUENCE;
            break;
        case ANSI_CSI:
            if (val >= 64 && val <= 126) // Anything between '@' and '~'
                state = PLAIN;
            break;
        case ANSI_SEQUENCE:
            if (val >= 64 && val <= 95) // Anything between '@' and '_'
                state = PLAIN;
            break;
        case ANSI_WAITING_FOR_ST:
            if (val == 7) // 'BEL'
                state = PLAIN;
            if (val == 27) // 'ESC'
                state = ANSI_ST_STARTED;
            break;
        case ANSI_ST_STARTED:
            if (val == 92) // '\'
                state = PLAIN;
            else
                state = ANSI_WAITING_FOR_ST;
            break;
        }
    }
    return result;
}

// Unit tests:
#ifdef WITH_TESTS
#   include <QTest>

#   include "projectexplorer.h"
#   include "outputparser_test.h"
#   include "task.h"

void ProjectExplorerPlugin::testAnsiFilterOutputParser_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<QString>("outputLines");

    QTest::newRow("pass-through stdout")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
            << QString::fromLatin1("Sometext\n") << QString();
    QTest::newRow("pass-through stderr")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("Sometext\n");

    QString input = QString::fromLatin1("te") + QChar(27) + QString::fromLatin1("Nst");
    QTest::newRow("ANSI: ESC-N")
            << input << OutputParserTester::STDOUT
            << QString::fromLatin1("test\n") << QString();
    input = QString::fromLatin1("te") + QChar(27) + QLatin1String("^ignored") + QChar(27) + QLatin1String("\\st");
    QTest::newRow("ANSI: ESC-^ignoredESC-\\")
            << input << OutputParserTester::STDOUT
            << QString::fromLatin1("test\n") << QString();
    input = QString::fromLatin1("te") + QChar(27) + QLatin1String("]0;ignored") + QChar(7) + QLatin1String("st");
    QTest::newRow("ANSI: window title change")
            << input << OutputParserTester::STDOUT
            << QString::fromLatin1("test\n") << QString();
    input = QString::fromLatin1("te") + QChar(27) + QLatin1String("[Ast");
    QTest::newRow("ANSI: cursor up")
            << input << OutputParserTester::STDOUT
            << QString::fromLatin1("test\n") << QString();
    input = QString::fromLatin1("te") + QChar(27) + QLatin1String("[2Ast");
    QTest::newRow("ANSI: cursor up (with int parameter)")
            << input << OutputParserTester::STDOUT
            << QString::fromLatin1("test\n") << QString();
    input = QString::fromLatin1("te") + QChar(27) + QLatin1String("[2;3Hst");
    QTest::newRow("ANSI: position cursor")
            << input << OutputParserTester::STDOUT
            << QString::fromLatin1("test\n") << QString();
    input = QString::fromLatin1("te") + QChar(27) + QLatin1String("[31;1mst");
    QTest::newRow("ANSI: bold red")
            << input << OutputParserTester::STDOUT
            << QString::fromLatin1("test\n") << QString();
}

void ProjectExplorerPlugin::testAnsiFilterOutputParser()
{
    OutputParserTester testbench;
    testbench.appendOutputParser(new AnsiFilterParser);
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);

    testbench.testParsing(input, inputChannel,
                          QList<Task>(), childStdOutLines, childStdErrLines,
                          QString());
}

#endif
