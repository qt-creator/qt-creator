// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testoutputreader.h"

#include "autotesttr.h"
#include "testtreeitem.h"

#include <utils/qtcprocess.h>
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

// Everything the accumulators below are allowed to hold. The child process controls how much it
// writes, so the budget is in characters, not lines: a few very long lines cost the same as many
// short ones.
constexpr qsizetype MaxAccumulatedChars = 512 * 1024;
constexpr qsizetype MaxOutputLineLength = 64 * 1024;

// Removes escape sequences matching "\u001B\\[.*?m". Repeatedly matching and removing from the
// front of the string instead is quadratic, and the length is attacker-controlled. Removing a
// sequence can put a leftover escape next to a following bracket and so create a sequence that was
// not in the input; the old code caught those by restarting, this catches them by testing the
// bracket against the end of the result.
QString TestOutputReader::removeCommandlineColors(const QString &original)
{
    static const QChar escape(0x1B);
    if (!original.contains(escape))
        return original;

    const QStringView input(original);
    QString result;
    result.reserve(input.size());
    qsizetype pos = 0;
    while (pos < input.size()) {
        const QChar current = input.at(pos);
        if (current != '[' || result.isEmpty() || result.back() != escape) {
            result.append(current);
            ++pos;
            continue;
        }
        qsizetype end = pos + 1;
        while (end < input.size() && input.at(end) != 'm' && input.at(end) != '\n')
            ++end;
        if (end < input.size() && input.at(end) == 'm') {
            result.chop(1); // the escape this bracket belongs to
            pos = end + 1;
            continue;
        }
        // No terminator before the end of the line, so nothing starting on it can match either.
        const qsizetype lineEnd = qMin(end + 1, input.size());
        result.append(input.mid(pos, lineEnd - pos));
        pos = lineEnd;
    }
    return result;
}

void TestOutputReader::appendBounded(QString &accumulated, const QString &line, bool separate)
{
    if (accumulated.size() >= MaxAccumulatedChars)
        return;
    if (separate && !accumulated.isEmpty())
        accumulated.append('\n');
    if (accumulated.size() + line.size() >= MaxAccumulatedChars) {
        accumulated.append(QStringView(line).left(MaxAccumulatedChars - accumulated.size()));
        accumulated.append(Tr::tr("... (output truncated)"));
        return;
    }
    accumulated.append(line);
}

// Truncates in place. Deliberately not sendAndResetSanitizerResult(): flushing here would reset
// m_sanitizerOutputMode, and every later line of the report would take the generic path and be
// dropped without a trace.
void TestOutputReader::appendSanitizerLine(const QString &line)
{
    if (m_sanitizerChars >= MaxAccumulatedChars)
        return;
    m_sanitizerChars += line.size() + 1;
    if (m_sanitizerChars >= MaxAccumulatedChars) {
        m_sanitizerLines.append(Tr::tr("... (output truncated)"));
        return;
    }
    m_sanitizerLines.append(line);
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
    QString truncated = QString::fromUtf8(line);
    if (truncated.size() > MaxOutputLineLength)
        truncated.truncate(MaxOutputLineLength);
    const QString lineStr = removeCommandlineColors(truncated);
    if (m_sanitizerOutputMode == SanitizerOutputMode::Asan) {
        // append the new line and check for end
        appendSanitizerLine(lineStr);
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
            appendSanitizerLine(lineStr);
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
        appendSanitizerLine("Sanitizer Issue");
        appendSanitizerLine(lineStr);
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
    m_sanitizerChars = 0;
    m_sanitizerResult = {};
    m_sanitizerOutputMode = SanitizerOutputMode::None;
}

} // namespace Autotest
