/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "testoutputreader.h"
#include "testresult.h"
#include "testresultspane.h"

#include <utils/qtcassert.h>

#include <QDebug>
#include <QProcess>

namespace Autotest {

TestOutputReader::TestOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                                   QProcess *testApplication, const QString &buildDirectory)
    : m_futureInterface(futureInterface)
    , m_testApplication(testApplication)
    , m_buildDir(buildDirectory)
    , m_id(testApplication ? testApplication->program() : QString())
{
    auto chopLineBreak = [](QByteArray line) {
        if (line.endsWith('\n'))
            line.chop(1);
        if (line.endsWith('\r'))
            line.chop(1);
        return line;
    };

    if (m_testApplication) {
        connect(m_testApplication, &QProcess::readyReadStandardOutput,
                this, [chopLineBreak, this] () {
            m_testApplication->setReadChannel(QProcess::StandardOutput);
            while (m_testApplication->canReadLine())
                processStdOutput(chopLineBreak(m_testApplication->readLine()));
        });
        connect(m_testApplication, &QProcess::readyReadStandardError,
                this, [chopLineBreak, this] () {
            m_testApplication->setReadChannel(QProcess::StandardError);
            while (m_testApplication->canReadLine())
                processStdError(chopLineBreak(m_testApplication->readLine()));
        });
    }
}

void TestOutputReader::processStdOutput(const QByteArray &outputLine)
{
    processOutputLine(outputLine);
    emit newOutputLineAvailable(outputLine, OutputChannel::StdOut);
}

void TestOutputReader::processStdError(const QByteArray &outputLine)
{
    qWarning() << "AutoTest.Run: Ignored plain output:" << outputLine;
    emit newOutputLineAvailable(outputLine, OutputChannel::StdErr);
}

void TestOutputReader::reportCrash()
{
    TestResultPtr result = createDefaultResult();
    result->setDescription(tr("Test executable crashed."));
    result->setResult(ResultType::MessageFatal);
    m_futureInterface.reportResult(result);
}

void TestOutputReader::createAndReportResult(const QString &message, ResultType type)
{
    TestResultPtr result = createDefaultResult();
    result->setDescription(message);
    result->setResult(type);
    reportResult(result);
}

void TestOutputReader::resetCommandlineColor()
{
    emit newOutputLineAvailable("\u001B[m", OutputChannel::StdOut);
    emit newOutputLineAvailable("\u001B[m", OutputChannel::StdErr);
}

QString TestOutputReader::removeCommandlineColors(const QString &original)
{
    static const QRegularExpression pattern("\u001B\\[.*?m");
    QString result = original;
    while (!result.isEmpty()) {
        QRegularExpressionMatch match = pattern.match(result);
        if (match.hasMatch())
            result.remove(match.capturedStart(), match.captured().length());
        else
            break;
    }
    return result;
}

void TestOutputReader::reportResult(const TestResultPtr &result)
{
    m_futureInterface.reportResult(result);
    m_hadValidOutput = true;
}

} // namespace Autotest
