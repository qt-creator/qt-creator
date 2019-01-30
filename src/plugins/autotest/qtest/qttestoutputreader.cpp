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

#include "qttestoutputreader.h"
#include "qttestresult.h"
#include "../testtreeitem.h"

#include <qtsupport/qtoutputformatter.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

namespace Autotest {
namespace Internal {

static QString decode(const QString& original)
{
    QString result(original);
    static const QRegularExpression regex("&#((x[[:xdigit:]]+)|(\\d+));");

    QRegularExpressionMatchIterator it = regex.globalMatch(original);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QString value = match.captured(1);
        if (value.startsWith('x'))
            result.replace(match.captured(0), QChar(value.midRef(1).toInt(nullptr, 16)));
        else
            result.replace(match.captured(0), QChar(value.toInt(nullptr, 10)));
    }

    return result;
}

// adapted from qplaintestlogger.cpp
static QString formatResult(double value)
{
    //NAN is not supported with visual studio 2010
    if (value < 0)// || value == NAN)
        return QString("NAN");
    if (value == 0)
        return QString("0");

    int significantDigits = 0;
    qreal divisor = 1;

    while (value / divisor >= 1) {
        divisor *= 10;
        ++significantDigits;
    }

    QString beforeDecimalPoint = QString::number(value, 'f', 0);
    QString afterDecimalPoint = QString::number(value, 'f', 20);
    afterDecimalPoint.remove(0, beforeDecimalPoint.count() + 1);

    const int beforeUse = qMin(beforeDecimalPoint.count(), significantDigits);
    const int beforeRemove = beforeDecimalPoint.count() - beforeUse;

    beforeDecimalPoint.chop(beforeRemove);
    for (int i = 0; i < beforeRemove; ++i)
        beforeDecimalPoint.append('0');

    int afterUse = significantDigits - beforeUse;
    if (beforeDecimalPoint == QString("0") && !afterDecimalPoint.isEmpty()) {
        ++afterUse;
        int i = 0;
        while (i < afterDecimalPoint.count() && afterDecimalPoint.at(i) == '0')
            ++i;
        afterUse += i;
    }

    const int afterRemove = afterDecimalPoint.count() - afterUse;
    afterDecimalPoint.chop(afterRemove);

    QString result = beforeDecimalPoint;
    if (afterUse > 0)
        result.append('.');
    result += afterDecimalPoint;

    return result;
}

static QString constructBenchmarkInformation(const QString &metric, double value, int iterations)
{
    QString metricsText;
    if (metric == "WalltimeMilliseconds")         // default
        metricsText = "msecs";
    else if (metric == "CPUTicks")                // -tickcounter
        metricsText = "CPU ticks";
    else if (metric == "Events")                  // -eventcounter
        metricsText = "events";
    else if (metric == "InstructionReads")        // -callgrind
        metricsText = "instruction reads";
    else if (metric == "CPUCycles")               // -perf
        metricsText = "CPU cycles";
    return QtTestOutputReader::tr("%1 %2 per iteration (total: %3, iterations: %4)")
            .arg(formatResult(value))
            .arg(metricsText)
            .arg(formatResult(value * double(iterations)))
            .arg(iterations);
}

static QString constructSourceFilePath(const QString &path, const QString &filePath)
{
    return QFileInfo(path, filePath).canonicalFilePath();
}

QtTestOutputReader::QtTestOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                                       QProcess *testApplication, const QString &buildDirectory,
                                       const QString &projectFile, OutputMode mode, TestType type)
    : TestOutputReader(futureInterface, testApplication, buildDirectory)
    , m_projectFile(projectFile)
    , m_mode(mode)
    , m_testType(type)
{
}

void QtTestOutputReader::processOutputLine(const QByteArray &outputLine)
{
    static const QByteArray qmlDebug = "QML Debugger: Waiting for connection on port";
    switch (m_mode) {
    case PlainText:
        processPlainTextOutput(outputLine);
        break;
    case XML:
        if (m_xmlReader.tokenType() == QXmlStreamReader::NoToken && outputLine.startsWith(qmlDebug))
            return;
        processXMLOutput(outputLine);
        break;
    }
}

TestResultPtr QtTestOutputReader::createDefaultResult() const
{
    QtTestResult *result = new QtTestResult(id(), m_projectFile, m_testType, m_className);
    result->setFunctionName(m_testCase);
    result->setDataTag(m_dataTag);
    return TestResultPtr(result);
}

static QString trQtVersion(const QString &version)
{
    return QtTestOutputReader::tr("Qt version: %1").arg(version);
}

static QString trQtBuild(const QString &build)
{
    return QtTestOutputReader::tr("Qt build: %1").arg(build);
}

static QString trQtestVersion(const QString &test)
{
    return QtTestOutputReader::tr("QTest version: %1").arg(test);
}

void QtTestOutputReader::processXMLOutput(const QByteArray &outputLine)
{
    static QStringList validEndTags = {QStringLiteral("Incident"),
                                       QStringLiteral("Message"),
                                       QStringLiteral("BenchmarkResult"),
                                       QStringLiteral("QtVersion"),
                                       QStringLiteral("QtBuild"),
                                       QStringLiteral("QTestVersion")};

    if (m_className.isEmpty() && outputLine.trimmed().isEmpty())
        return;

    m_xmlReader.addData(QString::fromUtf8(outputLine));
    while (!m_xmlReader.atEnd()) {
        if (m_futureInterface.isCanceled())
            return;
        QXmlStreamReader::TokenType token = m_xmlReader.readNext();
        switch (token) {
        case QXmlStreamReader::StartDocument:
            m_className.clear();
            break;
        case QXmlStreamReader::EndDocument:
            m_xmlReader.clear();
            return;
        case QXmlStreamReader::StartElement: {
            const QString currentTag = m_xmlReader.name().toString();
            if (currentTag == QStringLiteral("TestCase")) {
                m_className = m_xmlReader.attributes().value(QStringLiteral("name")).toString();
                QTC_ASSERT(!m_className.isEmpty(), continue);
                sendStartMessage(false);
            } else if (currentTag == QStringLiteral("TestFunction")) {
                m_testCase = m_xmlReader.attributes().value(QStringLiteral("name")).toString();
                QTC_ASSERT(!m_testCase.isEmpty(), continue);
                if (m_testCase == m_formerTestCase)  // don't report "Executing..." more than once
                    continue;
                sendStartMessage(true);
                sendMessageCurrentTest();
            } else if (currentTag == QStringLiteral("Duration")) {
                m_duration = m_xmlReader.attributes().value(QStringLiteral("msecs")).toString();
                QTC_ASSERT(!m_duration.isEmpty(), continue);
            } else if (currentTag == QStringLiteral("Message")
                       || currentTag == QStringLiteral("Incident")) {
                m_description.clear();
                m_duration.clear();
                m_file.clear();
                m_result = Result::Invalid;
                m_lineNumber = 0;
                const QXmlStreamAttributes &attributes = m_xmlReader.attributes();
                m_result = TestResult::resultFromString(
                            attributes.value(QStringLiteral("type")).toString());
                m_file = decode(attributes.value(QStringLiteral("file")).toString());
                if (!m_file.isEmpty())
                    m_file = constructSourceFilePath(m_buildDir, m_file);
                m_lineNumber = attributes.value(QStringLiteral("line")).toInt();
            } else if (currentTag == QStringLiteral("BenchmarkResult")) {
                const QXmlStreamAttributes &attributes = m_xmlReader.attributes();
                const QString metric = attributes.value(QStringLiteral("metric")).toString();
                const double value = attributes.value(QStringLiteral("value")).toDouble();
                const int iterations = attributes.value(QStringLiteral("iterations")).toInt();
                m_dataTag = attributes.value(QStringLiteral("tag")).toString();
                m_description = constructBenchmarkInformation(metric, value, iterations);
                m_result = Result::Benchmark;
            } else if (currentTag == QStringLiteral("DataTag")) {
                m_cdataMode = DataTag;
            } else if (currentTag == QStringLiteral("Description")) {
                m_cdataMode = Description;
            } else if (currentTag == QStringLiteral("QtVersion")) {
                m_result = Result::MessageInternal;
                m_cdataMode = QtVersion;
            } else if (currentTag == QStringLiteral("QtBuild")) {
                m_result = Result::MessageInternal;
                m_cdataMode = QtBuild;
            } else if (currentTag == QStringLiteral("QTestVersion")) {
                m_result = Result::MessageInternal;
                m_cdataMode = QTestVersion;
            }
            break;
        }
        case QXmlStreamReader::Characters: {
            QStringRef text = m_xmlReader.text().trimmed();
            if (text.isEmpty())
                break;

            switch (m_cdataMode) {
            case DataTag:
                m_dataTag = text.toString();
                break;
            case Description:
                if (!m_description.isEmpty())
                    m_description.append('\n');
                m_description.append(text);
                break;
            case QtVersion:
                m_description = trQtVersion(text.toString());
                break;
            case QtBuild:
                m_description = trQtBuild(text.toString());
                break;
            case QTestVersion:
                m_description = trQtestVersion(text.toString());
                break;
            default:
                // this must come from plain printf() calls - but this will be ignored anyhow
                qWarning() << "AutoTest.Run: Ignored plain output:" << text.toString();
                break;
            }
            break;
        }
        case QXmlStreamReader::EndElement: {
            m_cdataMode = None;
            const QStringRef currentTag = m_xmlReader.name();
            if (currentTag == QStringLiteral("TestFunction")) {
                sendFinishMessage(true);
                m_futureInterface.setProgressValue(m_futureInterface.progressValue() + 1);
                m_dataTag.clear();
                m_formerTestCase = m_testCase;
                m_testCase.clear();
            } else if (currentTag == QStringLiteral("TestCase")) {
                sendFinishMessage(false);
            } else if (validEndTags.contains(currentTag.toString())) {
                sendCompleteInformation();
                if (currentTag == QStringLiteral("Incident"))
                    m_dataTag.clear();
            }
            break;
        }
        default:
            // premature end happens e.g. if not all data has been added to the reader yet
            if (m_xmlReader.error() != QXmlStreamReader::NoError
                    && m_xmlReader.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
                createAndReportResult(tr("XML parsing failed.")
                                      + QString(" (%1) ").arg(m_xmlReader.error())
                                      + m_xmlReader.errorString(), Result::MessageFatal);
            }
            break;
        }
    }
}

static QStringList extractFunctionInformation(const QString &testClassName,
                                              const QString &lineWithoutResultType,
                                              Result::Type resultType)
{
    static QRegularExpression classInformation("^(.+?)\\((.*?)\\)(.*)$");
    QStringList result;
    const QRegularExpressionMatch match = classInformation.match(lineWithoutResultType);
    if (match.hasMatch()) {
        QString fullQualifiedFunc = match.captured(1);
        QTC_ASSERT(fullQualifiedFunc.startsWith(testClassName + "::"), return result);
        fullQualifiedFunc = fullQualifiedFunc.mid(testClassName.length() + 2);
        result.append(fullQualifiedFunc);
        if (resultType == Result::Benchmark) { // tag is displayed differently
            QString possiblyTag = match.captured(3);
            if (!possiblyTag.isEmpty())
                possiblyTag = possiblyTag.mid(2, possiblyTag.length() - 4);
            result.append(possiblyTag);
            result.append(QString());
        } else {
            result.append(match.captured(2));
            result.append(match.captured(3));
        }
    }
    return result;
}

void QtTestOutputReader::processPlainTextOutput(const QByteArray &outputLineWithNewLine)
{
    static const QRegularExpression start("^[*]{9} Start testing of (.*) [*]{9}$");
    static const QRegularExpression config("^Config: Using QtTest library (.*), "
                                           "(Qt (\\d+(\\.\\d+){2}) \\(.*\\))$");
    static const QRegularExpression summary("^Totals: \\d+ passed, \\d+ failed, "
                                            "\\d+ skipped(, \\d+ blacklisted)?$");
    static const QRegularExpression finish("^[*]{9} Finished testing of (.*) [*]{9}$");

    static const QRegularExpression result("^(PASS   |FAIL!  |XFAIL  |XPASS  |SKIP   |RESULT "
                                           "|BPASS  |BFAIL  |BXPASS |BXFAIL "
                                           "|INFO   |QWARN  |WARNING|QDEBUG |QSYSTEM): (.*)$");

    static const QRegularExpression benchDetails("^\\s+([\\d,.]+ .* per iteration "
                                                 "\\(total: [\\d,.]+, iterations: \\d+\\))$");
    static const QRegularExpression locationUnix(QT_TEST_FAIL_UNIX_REGEXP);
    static const QRegularExpression locationWin(QT_TEST_FAIL_WIN_REGEXP);

    if (m_futureInterface.isCanceled())
        return;

    const QString line = QString::fromUtf8(chopLineBreak(outputLineWithNewLine));
    QRegularExpressionMatch match;

    auto hasMatch = [&match, line](const QRegularExpression &regex) {
        match = regex.match(line);
        return match.hasMatch();
    };

    if (hasMatch(result)) {
        processResultOutput(match.captured(1).toLower().trimmed(), match.captured(2));
    } else if (hasMatch(locationUnix)) {
        processLocationOutput(match.captured(1));
    } else if (hasMatch(locationWin)) {
        processLocationOutput(match.captured(1));
    } else if (hasMatch(benchDetails)) {
        m_description = match.captured(1);
    } else if (hasMatch(config)) {
        handleAndSendConfigMessage(match);
    } else if (hasMatch(start)) {
        m_className = match.captured(1);
        QTC_CHECK(!m_className.isEmpty());
        sendStartMessage(false);
    } else if (summary.match(line).hasMatch() || finish.match(line).hasMatch()) {
        processSummaryFinishOutput();
    } else { // we have some plain output, but we cannot say where for sure it belongs to..
        if (!m_description.isEmpty())
            m_description.append('\n');
        m_description.append(line);
    }
}

void QtTestOutputReader::processResultOutput(const QString &result, const QString &message)
{
    if (!m_testCase.isEmpty()) { // report the former result if there is any
        sendCompleteInformation();
        m_dataTag.clear();
        m_description.clear();
        m_file.clear();
        m_lineNumber = 0;
    }
    m_result = TestResult::resultFromString(result);
    const QStringList funcWithTag = extractFunctionInformation(m_className, message, m_result);
    QTC_ASSERT(funcWithTag.size() == 3, return);
    m_testCase = funcWithTag.at(0);
    if (m_testCase != m_formerTestCase) { // new test function executed
        if (!m_formerTestCase.isEmpty()) {
            using namespace std;
            swap(m_testCase, m_formerTestCase); // we want formerTestCase to be reported
            sendFinishMessage(true);
            swap(m_testCase, m_formerTestCase);
        }
        sendStartMessage(true);
        sendMessageCurrentTest();
    }
    m_dataTag = funcWithTag.at(1);
    const QString description = funcWithTag.at(2);
    if (!description.isEmpty()) {
        if (!m_description.isEmpty())
            m_description.append('\n');
        m_description.append(description.midRef(1)); // cut the first whitespace
    }
    m_formerTestCase = m_testCase;
}

void QtTestOutputReader::processLocationOutput(const QString &fileWithLine)
{
    QTC_ASSERT(fileWithLine.endsWith(')'), return);
    int openBrace = fileWithLine.lastIndexOf('(');
    QTC_ASSERT(openBrace != -1, return);
    m_file = constructSourceFilePath(m_buildDir, fileWithLine.left(openBrace));
    QString numberStr = fileWithLine.mid(openBrace + 1);
    numberStr.chop(1);
    m_lineNumber = numberStr.toInt();
}

void QtTestOutputReader::processSummaryFinishOutput()
{
    if (m_className.isEmpty()) // we have reported already
        return;
    // we still have something to report
    sendCompleteInformation();
    m_dataTag.clear();
    // report finished function
    sendFinishMessage(true);
    m_testCase.clear();
    m_formerTestCase.clear();
    // create and report the finish message for this test class
    sendFinishMessage(false);
    m_className.clear();
    m_description.clear();
    m_result = Result::Invalid;
    m_file.clear();
    m_lineNumber = 0;
}

void QtTestOutputReader::sendCompleteInformation()
{
    TestResultPtr testResult = createDefaultResult();
    testResult->setResult(m_result);

    if (m_lineNumber) {
        testResult->setFileName(m_file);
        testResult->setLine(m_lineNumber);
    } else {
        const TestTreeItem *testItem = testResult->findTestTreeItem();
        if (testItem && testItem->line()) {
            testResult->setFileName(testItem->filePath());
            testResult->setLine(static_cast<int>(testItem->line()));
        }
    }
    testResult->setDescription(m_description);
    reportResult(testResult);
}

void QtTestOutputReader::sendMessageCurrentTest()
{
    TestResultPtr testResult = TestResultPtr(new QtTestResult(m_projectFile, m_testType));
    testResult->setResult(Result::MessageCurrentTest);
    testResult->setDescription(tr("Entering test function %1::%2").arg(m_className, m_testCase));
    reportResult(testResult);
}

void QtTestOutputReader::sendStartMessage(bool isFunction)
{
    TestResultPtr testResult = createDefaultResult();
    testResult->setResult(Result::MessageTestCaseStart);
    testResult->setDescription(isFunction ? tr("Executing test function %1").arg(m_testCase)
                                          : tr("Executing test case %1").arg(m_className));
    const TestTreeItem *testItem = testResult->findTestTreeItem();
    if (testItem && testItem->line()) {
        testResult->setFileName(testItem->filePath());
        testResult->setLine(static_cast<int>(testItem->line()));
    }
    reportResult(testResult);
}

void QtTestOutputReader::sendFinishMessage(bool isFunction)
{
    TestResultPtr testResult = createDefaultResult();
    testResult->setResult(Result::MessageTestCaseEnd);
    if (!m_duration.isEmpty()) {
        testResult->setDescription(isFunction ? tr("Execution took %1 ms.").arg(m_duration)
                                              : tr("Test execution took %1 ms.").arg(m_duration));
    } else {
        testResult->setDescription(isFunction ? tr("Test function finished.")
                                              : tr("Test finished."));
    }
    reportResult(testResult);
}

void QtTestOutputReader::handleAndSendConfigMessage(const QRegularExpressionMatch &config)
{
    TestResultPtr testResult = createDefaultResult();
    testResult->setResult(Result::MessageInternal);
    testResult->setDescription(trQtVersion(config.captured(3)));
    reportResult(testResult);
    testResult = createDefaultResult();
    testResult->setResult(Result::MessageInternal);
    testResult->setDescription(trQtBuild(config.captured(2)));
    reportResult(testResult);
    testResult = createDefaultResult();
    testResult->setResult(Result::MessageInternal);
    testResult->setDescription(trQtestVersion(config.captured(1)));
    reportResult(testResult);
}

} // namespace Internal
} // namespace Autotest
