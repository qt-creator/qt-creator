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

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

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
            result.replace(regex.cap(0), QChar(value.mid(1).toInt(0, 16)));
        else
            result.replace(regex.cap(0), QChar(value.toInt(0, 10)));
        pos += regex.matchedLength();
    }

    return result;
}

static QString constructSourceFilePath(const QString &path, const QString &filePath,
                                       const QString &app)
{
    if (Utils::HostOsInfo::isMacHost() && !app.isEmpty()) {
        const QString fileName(QFileInfo(app).fileName());
        return QFileInfo(path.left(path.lastIndexOf(fileName + QLatin1String(".app"))), filePath)
                   .canonicalFilePath();
    }
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

TestXmlOutputReader::TestXmlOutputReader(QProcess *testApplication)
    :m_testApplication(testApplication)
{
    connect(m_testApplication, &QProcess::readyReadStandardOutput,
        this, &TestXmlOutputReader::processOutput);
}

enum CDATAMode {
    None,
    DataTag,
    Description,
    QtVersion,
    QTestVersion
};

void TestXmlOutputReader::processOutput()
{
    if (!m_testApplication || m_testApplication->state() != QProcess::Running)
        return;
    static QStringList validEndTags = { QStringLiteral("Incident"),
                                        QStringLiteral("Message"),
                                        QStringLiteral("BenchmarkResult"),
                                        QStringLiteral("QtVersion"),
                                        QStringLiteral("QTestVersion") };
    static CDATAMode cdataMode = None;
    static QString className;
    static QString testCase;
    static QString dataTag;
    static Result::Type result = Result::INVALID;
    static QString description;
    static QString file;
    static int lineNumber = 0;
    static QString duration;
    static QXmlStreamReader xmlReader;

    while (m_testApplication->canReadLine()) {
        xmlReader.addData(m_testApplication->readLine());
        while (!xmlReader.atEnd()) {
            QXmlStreamReader::TokenType token = xmlReader.readNext();
            switch (token) {
            case QXmlStreamReader::StartDocument:
                className.clear();
                break;
            case QXmlStreamReader::EndDocument:
                xmlReader.clear();
                return;
            case QXmlStreamReader::StartElement: {
                const QString currentTag = xmlReader.name().toString();
                if (currentTag == QStringLiteral("TestCase")) {
                    className = xmlReader.attributes().value(QStringLiteral("name")).toString();
                    QTC_ASSERT(!className.isEmpty(), continue);
                    auto testResult = new TestResult(className);
                    testResult->setResult(Result::MESSAGE_TEST_CASE_START);
                    testResult->setDescription(tr("Executing test case %1").arg(className));
                    testResultCreated(testResult);
                } else if (currentTag == QStringLiteral("TestFunction")) {
                    testCase = xmlReader.attributes().value(QStringLiteral("name")).toString();
                    QTC_ASSERT(!testCase.isEmpty(), continue);
                    auto testResult = new TestResult();
                    testResult->setResult(Result::MESSAGE_CURRENT_TEST);
                    testResult->setDescription(tr("Entering test function %1::%2").arg(className,
                                                                                       testCase));
                    testResultCreated(testResult);
                } else if (currentTag == QStringLiteral("Duration")) {
                    duration = xmlReader.attributes().value(QStringLiteral("msecs")).toString();
                    QTC_ASSERT(!duration.isEmpty(), continue);
                } else if (currentTag == QStringLiteral("Message")
                           || currentTag == QStringLiteral("Incident")) {
                    dataTag.clear();
                    description.clear();
                    duration.clear();
                    file.clear();
                    result = Result::INVALID;
                    lineNumber = 0;
                    const QXmlStreamAttributes &attributes = xmlReader.attributes();
                    result = TestResult::resultFromString(
                                attributes.value(QStringLiteral("type")).toString());
                    file = decode(attributes.value(QStringLiteral("file")).toString());
                    if (!file.isEmpty())
                        file = constructSourceFilePath(m_testApplication->workingDirectory(), file,
                                                       m_testApplication->program());
                    lineNumber = attributes.value(QStringLiteral("line")).toInt();
                } else if (currentTag == QStringLiteral("BenchmarkResult")) {
                    const QXmlStreamAttributes &attributes = xmlReader.attributes();
                    const QString metric = attributes.value(QStringLiteral("metrics")).toString();
                    const double value = attributes.value(QStringLiteral("value")).toDouble();
                    const int iterations = attributes.value(QStringLiteral("iterations")).toInt();
                    description = constructBenchmarkInformation(metric, value, iterations);
                    result = Result::BENCHMARK;
                } else if (currentTag == QStringLiteral("DataTag")) {
                    cdataMode = DataTag;
                } else if (currentTag == QStringLiteral("Description")) {
                    cdataMode = Description;
                } else if (currentTag == QStringLiteral("QtVersion")) {
                    result = Result::MESSAGE_INTERNAL;
                    cdataMode = QtVersion;
                } else if (currentTag == QStringLiteral("QTestVersion")) {
                    result = Result::MESSAGE_INTERNAL;
                    cdataMode = QTestVersion;
                }
                break;
            }
            case QXmlStreamReader::Characters: {
                QStringRef text = xmlReader.text().trimmed();
                if (text.isEmpty())
                    break;

                switch (cdataMode) {
                case DataTag:
                    dataTag = text.toString();
                    break;
                case Description:
                    if (!description.isEmpty())
                        description.append(QLatin1Char('\n'));
                    description.append(text);
                    break;
                case QtVersion:
                    description = tr("Qt version: %1").arg(text.toString());
                    break;
                case QTestVersion:
                    description = tr("QTest version: %1").arg(text.toString());
                    break;
                default:
                    QString message = QString::fromLatin1("unexpected cdatamode %1 for text \"%2\"")
                            .arg(cdataMode)
                            .arg(text.toString());
                    QTC_ASSERT(false, qWarning() << message);
                    break;
                }
                break;
            }
            case QXmlStreamReader::EndElement: {
                cdataMode = None;
                const QStringRef currentTag = xmlReader.name();
                if (currentTag == QStringLiteral("TestFunction")) {
                    if (!duration.isEmpty()) {
                        auto testResult = new TestResult(className);
                        testResult->setTestCase(testCase);
                        testResult->setResult(Result::MESSAGE_INTERNAL);
                        testResult->setDescription(tr("Execution took %1 ms.").arg(duration));
                        testResultCreated(testResult);
                    }
                    emit increaseProgress();
                } else if (currentTag == QStringLiteral("TestCase") && !duration.isEmpty()) {
                    auto testResult = new TestResult(className);
                    testResult->setResult(Result::MESSAGE_TEST_CASE_END);
                    testResult->setDescription(tr("Test execution took %1 ms.").arg(duration));
                    testResultCreated(testResult);
                } else if (validEndTags.contains(currentTag.toString())) {
                    auto testResult = new TestResult(className);
                    testResult->setTestCase(testCase);
                    testResult->setDataTag(dataTag);
                    testResult->setResult(result);
                    testResult->setFileName(file);
                    testResult->setLine(lineNumber);
                    testResult->setDescription(description);
                    testResultCreated(testResult);
                }
                break;
            }
            default:
                break;
            }
        }
    }
}

} // namespace Internal
} // namespace Autotest
