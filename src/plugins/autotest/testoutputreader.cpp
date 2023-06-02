// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testoutputreader.h"

#include "autotesttr.h"
#include "testtreeitem.h"

#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QRegularExpression>

using namespace Utils;

namespace Autotest {

FilePath TestOutputReader::constructSourceFilePath(const FilePath &path, const QString &file)
{
    const FilePath filePath = path.resolvePath(file);
    return filePath.isReadableFile() ? filePath : FilePath();
}

TestOutputReader::TestOutputReader(Process *testApplication, const FilePath &buildDirectory)
    : m_buildDir(buildDirectory)
{
    auto chopLineBreak = [](QByteArray line) {
        if (line.endsWith('\n'))
            line.chop(1);
        if (line.endsWith('\r'))
            line.chop(1);
        return line;
    };

    if (testApplication) {
        connect(testApplication, &Process::started, this, [this, testApplication] {
            m_id = testApplication->commandLine().executable().toUserOutput();
        });
        testApplication->setStdOutLineCallback([this, &chopLineBreak](const QString &line) {
            processStdOutput(chopLineBreak(line.toUtf8()));
        });
        testApplication->setStdErrLineCallback([this, &chopLineBreak](const QString &line) {
            processStdError(chopLineBreak(line.toUtf8()));
        });
    }
}

TestOutputReader::~TestOutputReader()
{
    if (m_sanitizerResult.isValid())
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
    TestResult result = createDefaultResult();
    result.setDescription(Tr::tr("Test executable crashed."));
    result.setResult(ResultType::MessageFatal);
    emit newResult(result);
}

void TestOutputReader::createAndReportResult(const QString &message, ResultType type)
{
    TestResult result = createDefaultResult();
    result.setDescription(message);
    result.setResult(type);
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

void TestOutputReader::reportResult(const TestResult &result)
{
    if (m_sanitizerResult.isValid())
        sendAndResetSanitizerResult();
    emit newResult(result);
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
        if (m_sanitizerResult.isValid()) // we have a result that has not been reported yet
            sendAndResetSanitizerResult();

        m_sanitizerOutputMode = mode;
        m_sanitizerResult = createDefaultResult();
        m_sanitizerLines.append("Sanitizer Issue");
        m_sanitizerLines.append(lineStr);
        if (m_sanitizerOutputMode == SanitizerOutputMode::Ubsan) {
            const FilePath path = constructSourceFilePath(m_buildDir, match.captured(1));
            // path may be empty if not existing - so, provide at least what we have
            m_sanitizerResult.setFileName(
                path.exists() ? path : FilePath::fromString(match.captured(1)));
            m_sanitizerResult.setLine(match.captured(2).toInt());
        }
    }
}

void TestOutputReader::sendAndResetSanitizerResult()
{
    QTC_ASSERT(m_sanitizerResult.isValid(), return);
    m_sanitizerResult.setDescription(m_sanitizerLines.join('\n'));
    m_sanitizerResult.setResult(m_sanitizerOutputMode == SanitizerOutputMode::Ubsan
                                ? ResultType::Fail : ResultType::MessageFatal);

    if (m_sanitizerResult.fileName().isEmpty()) {
        const ITestTreeItem *testItem = m_sanitizerResult.findTestTreeItem();
        if (testItem && testItem->line()) {
            m_sanitizerResult.setFileName(testItem->filePath());
            m_sanitizerResult.setLine(testItem->line());
        }
    }

    emit newResult(m_sanitizerResult);
    m_hadValidOutput = true;
    m_sanitizerLines.clear();
    m_sanitizerResult = {};
    m_sanitizerOutputMode = SanitizerOutputMode::None;
}

} // namespace Autotest
