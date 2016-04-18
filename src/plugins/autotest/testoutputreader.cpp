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

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QRegExp>
#include <QProcess>
#include <QFileInfo>
#include <QDir>

namespace Autotest {
namespace Internal {

static QString decode(const QString& original)
{
    QString result(original);
    static QRegExp regex(QLatin1String("&#((x[0-9A-F]+)|([0-9]+));"), Qt::CaseInsensitive);
    regex.setMinimal(true);

    int pos = 0;
    while ((pos = regex.indexIn(original, pos)) != -1) {
        const QString value = regex.cap(1);
        if (value.startsWith(QLatin1Char('x')))
            result.replace(regex.cap(0), QChar(value.midRef(1).toInt(0, 16)));
        else
            result.replace(regex.cap(0), QChar(value.toInt(0, 10)));
        pos += regex.matchedLength();
    }

    return result;
}

static QString constructSourceFilePath(const QString &path, const QString &filePath)
{
    return QFileInfo(path, filePath).canonicalFilePath();
}

// adapted from qplaintestlogger.cpp
static QString formatResult(double value)
{
    //NAN is not supported with visual studio 2010
    if (value < 0)// || value == NAN)
        return QLatin1String("NAN");
    if (value == 0)
        return QLatin1String("0");

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
        beforeDecimalPoint.append(QLatin1Char('0'));

    int afterUse = significantDigits - beforeUse;
    if (beforeDecimalPoint == QLatin1String("0") && !afterDecimalPoint.isEmpty()) {
        ++afterUse;
        int i = 0;
        while (i < afterDecimalPoint.count() && afterDecimalPoint.at(i) == QLatin1Char('0'))
            ++i;
        afterUse += i;
    }

    const int afterRemove = afterDecimalPoint.count() - afterUse;
    afterDecimalPoint.chop(afterRemove);

    QString result = beforeDecimalPoint;
    if (afterUse > 0)
        result.append(QLatin1Char('.'));
    result += afterDecimalPoint;

    return result;
}

static QString constructBenchmarkInformation(const QString &metric, double value, int iterations)
{
    QString metricsText;
    if (metric == QLatin1String("WalltimeMilliseconds"))         // default
        metricsText = QLatin1String("msecs");
    else if (metric == QLatin1String("CPUTicks"))                // -tickcounter
        metricsText = QLatin1String("CPU ticks");
    else if (metric == QLatin1String("Events"))                  // -eventcounter
        metricsText = QLatin1String("events");
    else if (metric == QLatin1String("InstructionReads"))        // -callgrind
        metricsText = QLatin1String("instruction reads");
    else if (metric == QLatin1String("CPUCycles"))               // -perf
        metricsText = QLatin1String("CPU cycles");
    return QObject::tr("%1 %2 per iteration (total: %3, iterations: %4)")
            .arg(formatResult(value))
            .arg(metricsText)
            .arg(formatResult(value * (double)iterations))
            .arg(iterations);
}

TestOutputReader::TestOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                                   QProcess *testApplication, const QString &buildDirectory)
    : m_futureInterface(futureInterface)
    , m_testApplication(testApplication)
    , m_buildDir(buildDirectory)
{
    connect(m_testApplication, &QProcess::readyRead, this, &TestOutputReader::processOutput);
    connect(m_testApplication, &QProcess::readyReadStandardError,
            this, &TestOutputReader::processStdError);
}

void TestOutputReader::processStdError()
{
    qWarning() << "Ignored plain output:" << m_testApplication->readAllStandardError();
}

QtTestOutputReader::QtTestOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                                       QProcess *testApplication, const QString &buildDirectory)
    : TestOutputReader(futureInterface, testApplication, buildDirectory)
{
}

void QtTestOutputReader::processOutput()
{
    if (!m_testApplication || m_testApplication->state() != QProcess::Running)
        return;
    static QStringList validEndTags = { QStringLiteral("Incident"),
                                        QStringLiteral("Message"),
                                        QStringLiteral("BenchmarkResult"),
                                        QStringLiteral("QtVersion"),
                                        QStringLiteral("QtBuild"),
                                        QStringLiteral("QTestVersion") };

    while (m_testApplication->canReadLine()) {
        m_xmlReader.addData(m_testApplication->readLine());
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
                    TestResultPtr testResult = TestResultPtr(new QTestResult(m_className));
                    testResult->setResult(Result::MessageTestCaseStart);
                    testResult->setDescription(tr("Executing test case %1").arg(m_className));
                    m_futureInterface.reportResult(testResult);
                } else if (currentTag == QStringLiteral("TestFunction")) {
                    m_testCase = m_xmlReader.attributes().value(QStringLiteral("name")).toString();
                    QTC_ASSERT(!m_testCase.isEmpty(), continue);
                    TestResultPtr testResult = TestResultPtr(new QTestResult());
                    testResult->setResult(Result::MessageCurrentTest);
                    testResult->setDescription(tr("Entering test function %1::%2").arg(m_className,
                                                                                       m_testCase));
                    m_futureInterface.reportResult(testResult);
                } else if (currentTag == QStringLiteral("Duration")) {
                    m_duration = m_xmlReader.attributes().value(QStringLiteral("msecs")).toString();
                    QTC_ASSERT(!m_duration.isEmpty(), continue);
                } else if (currentTag == QStringLiteral("Message")
                           || currentTag == QStringLiteral("Incident")) {
                    m_dataTag.clear();
                    m_description.clear();
                    m_duration.clear();
                    m_file.clear();
                    m_result = Result::Invalid;
                    m_lineNumber = 0;
                    const QXmlStreamAttributes &attributes = m_xmlReader.attributes();
                    m_result = TestResult::resultFromString(
                                attributes.value(QStringLiteral("type")).toString());
                    m_file = decode(attributes.value(QStringLiteral("file")).toString());
                    if (!m_file.isEmpty()) {
                        m_file = constructSourceFilePath(m_buildDir, m_file);
                    }
                    m_lineNumber = attributes.value(QStringLiteral("line")).toInt();
                } else if (currentTag == QStringLiteral("BenchmarkResult")) {
                    const QXmlStreamAttributes &attributes = m_xmlReader.attributes();
                    const QString metric = attributes.value(QStringLiteral("metrics")).toString();
                    const double value = attributes.value(QStringLiteral("value")).toDouble();
                    const int iterations = attributes.value(QStringLiteral("iterations")).toInt();
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
                        m_description.append(QLatin1Char('\n'));
                    m_description.append(text);
                    break;
                case QtVersion:
                    m_description = tr("Qt version: %1").arg(text.toString());
                    break;
                case QtBuild:
                    m_description = tr("Qt build: %1").arg(text.toString());
                    break;
                case QTestVersion:
                    m_description = tr("QTest version: %1").arg(text.toString());
                    break;
                default:
                    // this must come from plain printf() calls - but this will be ignored anyhow
                    qWarning() << "Ignored plain output:" << text.toString();
                    break;
                }
                break;
            }
            case QXmlStreamReader::EndElement: {
                m_cdataMode = None;
                const QStringRef currentTag = m_xmlReader.name();
                if (currentTag == QStringLiteral("TestFunction")) {
                    if (!m_duration.isEmpty()) {
                        TestResultPtr testResult = TestResultPtr(new QTestResult(m_className));
                        testResult->setTestCase(m_testCase);
                        testResult->setResult(Result::MessageInternal);
                        testResult->setDescription(tr("Execution took %1 ms.").arg(m_duration));
                        m_futureInterface.reportResult(testResult);
                    }
                    m_futureInterface.setProgressValue(m_futureInterface.progressValue() + 1);
                } else if (currentTag == QStringLiteral("TestCase")) {
                    TestResultPtr testResult = TestResultPtr(new QTestResult(m_className));
                    testResult->setResult(Result::MessageTestCaseEnd);
                    testResult->setDescription(
                            m_duration.isEmpty() ? tr("Test finished.")
                                               : tr("Test execution took %1 ms.").arg(m_duration));
                    m_futureInterface.reportResult(testResult);
                } else if (validEndTags.contains(currentTag.toString())) {
                    TestResultPtr testResult = TestResultPtr(new QTestResult(m_className));
                    testResult->setTestCase(m_testCase);
                    testResult->setDataTag(m_dataTag);
                    testResult->setResult(m_result);
                    testResult->setFileName(m_file);
                    testResult->setLine(m_lineNumber);
                    testResult->setDescription(m_description);
                    m_futureInterface.reportResult(testResult);
                }
                break;
            }
            default:
                break;
            }
        }
    }
}

GTestOutputReader::GTestOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                                     QProcess *testApplication, const QString &buildDirectory)
    : TestOutputReader(futureInterface, testApplication, buildDirectory)
{
}

void GTestOutputReader::processOutput()
{
    if (!m_testApplication || m_testApplication->state() != QProcess::Running)
        return;

    static QRegExp newTestStarts(QStringLiteral("^\\[-{10}\\] \\d+ tests? from (.*)$"));
    static QRegExp testEnds(QStringLiteral("^\\[-{10}\\] \\d+ tests? from (.*) \\((.*)\\)$"));
    static QRegExp newTestSetStarts(QStringLiteral("^\\[ RUN      \\] (.*)$"));
    static QRegExp testSetSuccess(QStringLiteral("^\\[       OK \\] (.*) \\((.*)\\)$"));
    static QRegExp testSetFail(QStringLiteral("^\\[  FAILED  \\] (.*) \\((.*)\\)$"));
    static QRegExp disabledTests(QStringLiteral("^  YOU HAVE (\\d+) DISABLED TESTS?$"));
    static QRegExp failureLocation(QStringLiteral("^(.*):(\\d+): Failure$"));
    static QRegExp iterations(QStringLiteral("^Repeating all tests \\(iteration (\\d+)\\) . . .$"));

    while (m_testApplication->canReadLine()) {
        if (m_futureInterface.isCanceled())
            return;
        QByteArray read = m_testApplication->readLine();
        if (!m_unprocessed.isEmpty()) {
            read = m_unprocessed + read;
            m_unprocessed.clear();
        }
        if (!read.endsWith('\n')) {
            m_unprocessed = read;
            continue;
        }
        read.chop(1); // remove the newline from the output
        if (read.endsWith('\r'))
            read.chop(1);

        const QString line = QString::fromLatin1(read);
        if (line.trimmed().isEmpty())
            continue;

        if (!line.startsWith(QLatin1Char('['))) {
            m_description.append(line).append(QLatin1Char('\n'));
            if (iterations.exactMatch(line)) {
                m_iteration = iterations.cap(1).toInt();
                m_description.clear();
            } else if (line.startsWith(QStringLiteral("Note:"))) {
                TestResultPtr testResult = TestResultPtr(new GTestResult());
                testResult->setResult(Result::MessageInternal);
                testResult->setDescription(line);
                m_futureInterface.reportResult(testResult);
                m_description.clear();
            } else if (disabledTests.exactMatch(line)) {
                TestResultPtr testResult = TestResultPtr(new GTestResult());
                testResult->setResult(Result::MessageDisabledTests);
                int disabled = disabledTests.cap(1).toInt();
                testResult->setDescription(tr("You have %n disabled test(s).", 0, disabled));
                testResult->setLine(disabled); // misuse line property to hold number of disabled
                m_futureInterface.reportResult(testResult);
                m_description.clear();
            }
            continue;
        }

        if (testEnds.exactMatch(line)) {
            TestResultPtr testResult = TestResultPtr(new GTestResult(m_currentTestName));
            testResult->setTestCase(m_currentTestSet);
            testResult->setResult(Result::MessageTestCaseEnd);
            testResult->setDescription(tr("Test execution took %1").arg(testEnds.cap(2)));
            m_futureInterface.reportResult(testResult);
            m_currentTestName.clear();
            m_currentTestSet.clear();
        } else if (newTestStarts.exactMatch(line)) {
            m_currentTestName = newTestStarts.cap(1);
            TestResultPtr testResult = TestResultPtr(new GTestResult(m_currentTestName));
            if (m_iteration > 1) {
                testResult->setResult(Result::MessageTestCaseRepetition);
                testResult->setDescription(tr("Repeating test case %1 (iteration %2)")
                                           .arg(m_currentTestName).arg(m_iteration));
            } else {
                testResult->setResult(Result::MessageTestCaseStart);
                testResult->setDescription(tr("Executing test case %1").arg(m_currentTestName));
            }
            m_futureInterface.reportResult(testResult);
        } else if (newTestSetStarts.exactMatch(line)) {
            m_currentTestSet = newTestSetStarts.cap(1);
            TestResultPtr testResult = TestResultPtr(new GTestResult());
            testResult->setResult(Result::MessageCurrentTest);
            testResult->setDescription(tr("Entering test set %1").arg(m_currentTestSet));
            m_futureInterface.reportResult(testResult);
            m_description.clear();
        } else if (testSetSuccess.exactMatch(line)) {
            TestResultPtr testResult = TestResultPtr(new GTestResult(m_currentTestName));
            testResult->setTestCase(m_currentTestSet);
            testResult->setResult(Result::Pass);
            testResult->setDescription(m_description);
            m_futureInterface.reportResult(testResult);
            m_description.clear();
            testResult = TestResultPtr(new GTestResult(m_currentTestName));
            testResult->setTestCase(m_currentTestSet);
            testResult->setResult(Result::MessageInternal);
            testResult->setDescription(tr("Execution took %1.").arg(testSetSuccess.cap(2)));
            m_futureInterface.reportResult(testResult);
            m_futureInterface.setProgressValue(m_futureInterface.progressValue() + 1);
        } else if (testSetFail.exactMatch(line)) {
            TestResultPtr testResult = TestResultPtr(new GTestResult(m_currentTestName));
            testResult->setTestCase(m_currentTestSet);
            testResult->setResult(Result::Fail);
            m_description.chop(1);
            testResult->setDescription(m_description);

            foreach (const QString &output, m_description.split(QLatin1Char('\n'))) {
                if (failureLocation.exactMatch(output)) {
                    QString file = constructSourceFilePath(m_buildDir, failureLocation.cap(1));
                    if (file.isEmpty())
                        continue;
                    testResult->setFileName(file);
                    testResult->setLine(failureLocation.cap(2).toInt());
                    break;
                }
            }
            m_futureInterface.reportResult(testResult);
            m_description.clear();
            testResult = TestResultPtr(new GTestResult(m_currentTestName));
            testResult->setTestCase(m_currentTestSet);
            testResult->setResult(Result::MessageInternal);
            testResult->setDescription(tr("Execution took %1.").arg(testSetFail.cap(2)));
            m_futureInterface.reportResult(testResult);
            m_futureInterface.setProgressValue(m_futureInterface.progressValue() + 1);
        }
    }
}

} // namespace Internal
} // namespace Autotest
