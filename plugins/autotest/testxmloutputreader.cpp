/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#include "testxmloutputreader.h"
#include "testresult.h"

#include <QXmlStreamReader>
#include <QProcess>
#include <QFileInfo>
#include <QDir>

namespace Autotest {
namespace Internal {

static bool xmlStartsWith(const QString &code, const QString &start, QString &result)
{
    if (code.startsWith(start)) {
        result = code.mid(start.length());
        result = result.left(result.indexOf(QLatin1Char('"')));
        result = result.left(result.indexOf(QLatin1String("</")));
        return !result.isEmpty();
    }
    return false;
}

static bool xmlCData(const QString &code, const QString &start, QString &result)
{
    if (code.startsWith(start)) {
        int index = code.indexOf(QLatin1String("<![CDATA[")) + 9;
        result = code.mid(index, code.indexOf(QLatin1String("]]>"), index) - index);
        return !result.isEmpty();
    }
    return false;
}

static bool xmlExtractTypeFileLine(const QString &code, const QString &tagStart,
    Result::Type &result, QString &file, int &line)
{
    if (code.startsWith(tagStart)) {
        int start = code.indexOf(QLatin1String(" type=\"")) + 7;
        result = TestResult::resultFromString(
            code.mid(start, code.indexOf(QLatin1Char('"'), start) - start));
        start = code.indexOf(QLatin1String(" file=\"")) + 7;
        file = code.mid(start, code.indexOf(QLatin1Char('"'), start) - start);
        start = code.indexOf(QLatin1String(" line=\"")) + 7;
        line = code.mid(start, code.indexOf(QLatin1Char('"'), start) - start).toInt();
        return true;
    }
    return false;
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

static bool xmlExtractBenchmarkInformation(const QString &code, const QString &tagStart,
    QString &description)
{
    if (code.startsWith(tagStart)) {
        int start = code.indexOf(QLatin1String(" metric=\"")) + 9;
        const QString metric = code.mid(start, code.indexOf(QLatin1Char('"'), start) - start);
        start = code.indexOf(QLatin1String(" value=\"")) + 8;
        const double value = code.mid(start, code.indexOf(QLatin1Char('"'), start) - start).toDouble();
        start = code.indexOf(QLatin1String(" iterations=\"")) + 13;
        const int iterations = code.mid(start, code.indexOf(QLatin1Char('"'), start) - start).toInt();
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
        description = QObject::tr("%1 %2 per iteration (total: %3, iterations: %4)")
                .arg(formatResult(value))
                .arg(metricsText)
                .arg(formatResult(value * (double)iterations))
                .arg(iterations);
        return true;
    }
    return false;
}

TestXmlOutputReader::TestXmlOutputReader(QProcess *testApplication)
    :m_testApplication(testApplication)
{
    connect(m_testApplication, &QProcess::readyReadStandardOutput,
        this, &TestXmlOutputReader::processOutput);
}

TestXmlOutputReader::~TestXmlOutputReader()
{
}

void TestXmlOutputReader::processOutput()
{
    if (!m_testApplication || m_testApplication->state() != QProcess::Running)
        return;
    static QString className;
    static QString testCase;
    static QString dataTag;
    static Result::Type result = Result::UNKNOWN;
    static QString description;
    static QString file;
    static int lineNumber = 0;
    static QString duration;
    static bool readingDescription = false;
    static QString qtVersion;
    static QString qtestVersion;
    static QString benchmarkDescription;

    while (m_testApplication->canReadLine()) {
        // TODO Qt5 uses UTF-8 - while Qt4 uses ISO-8859-1 - could this be a problem?
        const QString line = QString::fromUtf8(m_testApplication->readLine()).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1String("<?xml version"))) {
            className = QString();
            continue;
        }
        if (xmlStartsWith(line, QLatin1String("<TestCase name=\""), className))
            continue;
        if (xmlStartsWith(line, QLatin1String("<TestFunction name=\""), testCase)) {
            dataTag = QString();
            description = QString();
            duration = QString();
            file = QString();
            result = Result::UNKNOWN;
            lineNumber = 0;
            readingDescription = false;
            testResultCreated(TestResult(QString(), QString(), QString(), Result::MESSAGE_CURRENT_TEST,
                QObject::tr("Entering test function %1::%2").arg(className).arg(testCase)));
            continue;
        }
        if (xmlStartsWith(line, QLatin1String("<Duration msecs=\""), duration)) {
            continue;
        }
        if (xmlExtractTypeFileLine(line, QLatin1String("<Message"), result, file, lineNumber))
            continue;
        if (xmlCData(line, QLatin1String("<DataTag>"), dataTag))
            continue;
        if (xmlCData(line, QLatin1String("<Description>"), description)) {
            if (!line.endsWith(QLatin1String("</Description>")))
                readingDescription = true;
            continue;
        }
        if (xmlExtractTypeFileLine(line, QLatin1String("<Incident"), result, file, lineNumber)) {
            if (line.endsWith(QLatin1String("/>"))) {
                TestResult testResult(className, testCase, dataTag, result, description);
                if (!file.isEmpty())
                    file = QFileInfo(m_testApplication->workingDirectory(), file).canonicalFilePath();
                testResult.setFileName(file);
                testResult.setLine(lineNumber);
                testResultCreated(testResult);
            }
            continue;
        }
        if (xmlExtractBenchmarkInformation(line, QLatin1String("<BenchmarkResult"), benchmarkDescription)) {
            TestResult testResult(className, testCase, dataTag, Result::BENCHMARK, benchmarkDescription);
            testResultCreated(testResult);
            continue;
        }
        if (line == QLatin1String("</Message>") || line == QLatin1String("</Incident>")) {
            TestResult testResult(className, testCase, dataTag, result, description);
            if (!file.isEmpty())
                file = QFileInfo(m_testApplication->workingDirectory(), file).canonicalFilePath();
            testResult.setFileName(file);
            testResult.setLine(lineNumber);
            testResultCreated(testResult);
            description = QString();
        } else if (line == QLatin1String("</TestFunction>") && !duration.isEmpty()) {
            TestResult testResult(className, testCase, QString(), Result::MESSAGE_INTERNAL,
                                  QObject::tr("Execution took %1 ms.").arg(duration));
            testResultCreated(testResult);
            emit increaseProgress();
        } else if (line == QLatin1String("</TestCase>") && !duration.isEmpty()) {
            TestResult testResult(className, QString(), QString(), Result::MESSAGE_INTERNAL,
                                  QObject::tr("Test execution took %1 ms.").arg(duration));
            testResultCreated(testResult);
        } else if (readingDescription) {
            if (line.endsWith(QLatin1String("]]></Description>"))) {
                description.append(QLatin1Char('\n'));
                description.append(line.left(line.indexOf(QLatin1String("]]></Description>"))));
                readingDescription = false;
            } else {
                description.append(QLatin1Char('\n'));
                description.append(line);
            }
        } else if (xmlStartsWith(line, QLatin1String("<QtVersion>"), qtVersion)) {
            testResultCreated(FaultyTestResult(Result::MESSAGE_INTERNAL,
                QObject::tr("Qt version: %1").arg(qtVersion)));
        } else if (xmlStartsWith(line, QLatin1String("<QTestVersion>"), qtestVersion)) {
            testResultCreated(FaultyTestResult(Result::MESSAGE_INTERNAL,
                QObject::tr("QTest version: %1").arg(qtestVersion)));
        } else {
//            qDebug() << "Unhandled line:" << line; // TODO remove
        }
    }
}

} // namespace Internal
} // namespace Autotest
