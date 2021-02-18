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
#include "testtreeitem.h"

#include <utils/qtcassert.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
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

TestOutputReader::~TestOutputReader()
{
    if (m_sanitizerResult)
        sendAndResetSanitizerResult();
}

void TestOutputReader::processStdOutput(const QByteArray &outputLine)
{
    processOutputLine(outputLine);
    emit newOutputLineAvailable(outputLine, OutputChannel::StdOut);
}

void TestOutputReader::processStdError(const QByteArray &outputLine)
{
    checkForSanitizerOutput(outputLine);
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
    if (m_sanitizerResult)
        sendAndResetSanitizerResult();
    m_futureInterface.reportResult(result);
    m_hadValidOutput = true;
}

void TestOutputReader::checkForSanitizerOutput(const QByteArray &line)
{
    const QString lineStr = removeCommandlineColors(QString::fromUtf8(line));
    if (m_sanitizerOutputMode == SanitizerOutputMode::Asan) {
        // append the new line and check for end
        m_sanitizerLines.append(lineStr);
        static const QRegularExpression regex("^==\\d+==\\s*ABORTING.*");
        if (regex.match(lineStr).hasMatch())
            sendAndResetSanitizerResult();
        return;
    }

    static const QRegularExpression regex("^==\\d+==\\s*(ERROR|WARNING|Sanitizer CHECK failed):.*");
    static const QRegularExpression ubsanRegex("^(.*):(\\d+):(\\d+): runtime error:.*");
    QRegularExpressionMatch match = regex.match(lineStr);
    SanitizerOutputMode mode = SanitizerOutputMode::None;
    if (match.hasMatch()) {
        mode = SanitizerOutputMode::Asan;
    } else {
        match = ubsanRegex.match(lineStr);
        if (m_sanitizerOutputMode == SanitizerOutputMode::Ubsan && !match.hasMatch()) {
            m_sanitizerLines.append(lineStr);
            return;
        }
        if (match.hasMatch())
            mode = SanitizerOutputMode::Ubsan;
    }
    if (mode != SanitizerOutputMode::None) {
        if (m_sanitizerResult) // we have a result that has not been reported yet
            sendAndResetSanitizerResult();

        m_sanitizerOutputMode = mode;
        m_sanitizerResult = createDefaultResult();
        m_sanitizerLines.append("Sanitizer Issue");
        m_sanitizerLines.append(lineStr);
        if (m_sanitizerOutputMode == SanitizerOutputMode::Ubsan) {
            const QString path = QFileInfo(m_buildDir, match.captured(1)).canonicalFilePath();
            // path may be empty if not existing - so, provide at least what we have
            m_sanitizerResult->setFileName(path.isEmpty() ? match.captured(1) : path);
            m_sanitizerResult->setLine(match.captured(2).toInt());
        }
    }
}

void TestOutputReader::sendAndResetSanitizerResult()
{
    QTC_ASSERT(m_sanitizerResult, return);
    m_sanitizerResult->setDescription(m_sanitizerLines.join('\n'));
    m_sanitizerResult->setResult(m_sanitizerOutputMode == SanitizerOutputMode::Ubsan
                                 ? ResultType::Fail : ResultType::MessageFatal);

    if (m_sanitizerResult->fileName().isEmpty()) {
        const ITestTreeItem *testItem = m_sanitizerResult->findTestTreeItem();
        if (testItem && testItem->line()) {
            m_sanitizerResult->setFileName(testItem->filePath());
            m_sanitizerResult->setLine(testItem->line());
        }
    }

    m_futureInterface.reportResult(m_sanitizerResult);
    m_hadValidOutput = true;
    m_sanitizerLines.clear();
    m_sanitizerResult.reset();
    m_sanitizerOutputMode = SanitizerOutputMode::None;
}

} // namespace Autotest
