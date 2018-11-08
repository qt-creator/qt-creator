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
namespace Internal {

TestOutputReader::TestOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                                   QProcess *testApplication, const QString &buildDirectory)
    : m_futureInterface(futureInterface)
    , m_testApplication(testApplication)
    , m_buildDir(buildDirectory)
    , m_id(testApplication ? testApplication->program() : QString())
{
    if (m_testApplication) {
        connect(m_testApplication, &QProcess::readyRead,
                this, [this] () {
            while (m_testApplication->canReadLine()) {
                const QByteArray output = m_testApplication->readLine();
                processOutput(output);
            }
        });
        connect(m_testApplication, &QProcess::readyReadStandardError,
                this, [this] () {
            const QByteArray output = m_testApplication->readAllStandardError();
            processStdError(output);
        });
    }
}

void TestOutputReader::processOutput(const QByteArray &output)
{
    processOutputLine(output);
    emit newOutputAvailable(output);
}

void TestOutputReader::processStdError(const QByteArray &outputLineWithNewLine)
{
    qWarning() << "AutoTest.Run: Ignored plain output:" << outputLineWithNewLine;
    emit newOutputAvailable(outputLineWithNewLine);
}

void TestOutputReader::reportCrash()
{
    TestResultPtr result = createDefaultResult();
    result->setDescription(tr("Test executable crashed."));
    result->setResult(Result::MessageFatal);
    m_futureInterface.reportResult(result);
}

void TestOutputReader::createAndReportResult(const QString &message, Result::Type type)
{
    TestResultPtr result = createDefaultResult();
    result->setDescription(message);
    result->setResult(type);
    reportResult(result);
}

QByteArray TestOutputReader::chopLineBreak(const QByteArray &original)
{
    QTC_ASSERT(original.endsWith('\n'), return original);
    QByteArray output(original);
    output.chop(1); // remove the newline from the output
    if (output.endsWith('\r'))
        output.chop(1);
    return output;
}

void TestOutputReader::reportResult(const TestResultPtr &result)
{
    m_futureInterface.reportResult(result);
    m_hadValidOutput = true;
}

} // namespace Internal
} // namespace Autotest
