/****************************************************************************
**
** Copyright (C) 2019 Jochen Seemann
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

#include "catchoutputreader.h"
#include "catchresult.h"

#include "../testtreeitem.h"

#include <utils/qtcassert.h>

#include <QFileInfo>

namespace Autotest {
namespace Internal {

namespace CatchXml {
    const char GroupElement[]          = "Group";
    const char TestCaseElement[]       = "TestCase";
    const char SectionElement[]        = "Section";
    const char ExpressionElement[]     = "Expression";
    const char ExpandedElement[]       = "Expanded";
    const char BenchmarkResults[]      = "BenchmarkResults";
    const char MeanElement[]           = "mean";
    const char StandardDevElement[]    = "standardDeviation";
    const char SectionResultElement[]  = "OverallResults";
    const char TestCaseResultElement[] = "OverallResult";
}

CatchOutputReader::CatchOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                                     QProcess *testApplication, const QString &buildDirectory,
                                     const QString &projectFile)
    : TestOutputReader (futureInterface, testApplication, buildDirectory)
    , m_projectFile(projectFile)
{
}

void CatchOutputReader::processOutputLine(const QByteArray &outputLineWithNewLine)
{
    if (outputLineWithNewLine.trimmed().isEmpty())
        return;

    m_xmlReader.addData(QString::fromUtf8(outputLineWithNewLine));
    while (!m_xmlReader.atEnd()) {
        QXmlStreamReader::TokenType token = m_xmlReader.readNext();

        switch (token) {
        case QXmlStreamReader::StartDocument:
            break;
        case QXmlStreamReader::EndDocument:
            m_xmlReader.clear();
            break;
        case QXmlStreamReader::StartElement: {
            m_currentTagName = m_xmlReader.name().toString();

            if (m_currentTagName == CatchXml::GroupElement) {
                testOutputNodeStarted(GroupNode);
            } else if (m_currentTagName == CatchXml::TestCaseElement) {
                m_reportedResult = false;
                testOutputNodeStarted(TestCaseNode);
                recordTestInformation(m_xmlReader.attributes());
                sendResult(ResultType::TestStart);
            } else if (m_currentTagName == CatchXml::SectionElement) {
                testOutputNodeStarted(SectionNode);
                recordTestInformation(m_xmlReader.attributes());
                sendResult(ResultType::TestStart);
            } else if (m_currentTagName == CatchXml::TestCaseResultElement) {
                if (m_currentTestNode == OverallNode || m_currentTestNode == GroupNode)
                    continue;
                if (m_reportedResult)
                    continue;
                if (m_xmlReader.attributes().value("success").toString() == QStringLiteral("true"))
                    sendResult(ResultType::Pass);
                else if (m_shouldFail)
                    sendResult(ResultType::UnexpectedPass);
            } else if (m_currentTagName == CatchXml::SectionResultElement) {
                const QXmlStreamAttributes attributes = m_xmlReader.attributes();
                if (m_currentTestNode == OverallNode) { // the final results for the executable
                    int passes = attributes.value("successes").toInt();
                    int fails = attributes.value("failures").toInt();
                    int xfails = attributes.value("expectedFailures").toInt();
                    m_summary[ResultType::Pass] = passes - m_xpassCount;
                    m_summary[ResultType::Fail] = fails;
                    if (xfails)
                        m_summary[ResultType::ExpectedFail] = xfails;
                    if (m_xpassCount)
                        m_summary[ResultType::UnexpectedPass] = m_xpassCount;
                }
                if (m_currentTestNode == OverallNode || m_currentTestNode == GroupNode)
                    continue;
                if (attributes.value("failures").toInt() == 0)
                    if (!m_reportedSectionResult)
                      sendResult(ResultType::Pass);
            } else if (m_currentTagName == CatchXml::ExpressionElement) {
                recordTestInformation(m_xmlReader.attributes());
                if (m_xmlReader.attributes().value("success").toString() == QStringLiteral("true"))
                    m_currentResult = m_shouldFail ? ResultType::UnexpectedPass : ResultType::Pass;
                else
                    m_currentResult = m_mayFail || m_shouldFail ? ResultType::ExpectedFail : ResultType::Fail;
            } else if (m_currentTagName == CatchXml::BenchmarkResults) {
                recordBenchmarkInformation(m_xmlReader.attributes());
                m_currentResult = ResultType::Benchmark;
            } else if (m_currentTagName == CatchXml::MeanElement) {
                recordBenchmarkDetails(m_xmlReader.attributes(), {{{"mean"}, {"value"}},
                                                                  {{"low mean"}, {"lowerBound"}},
                                                                  {{"high mean"}, {"upperBound"}}});
            } else if (m_currentTagName == CatchXml::StandardDevElement) {
                recordBenchmarkDetails(m_xmlReader.attributes(), {
                                           {{"standard deviation"}, {"value"}},
                                           {{"low std dev"}, {"lowerBound"}},
                                           {{"high std dev"}, {"upperBound"}}});
            }
            break;
        }
        case QXmlStreamReader::Characters: {
            const QStringRef text = m_xmlReader.text();
            if (m_currentTagName == CatchXml::ExpandedElement) {
                m_currentExpression.append(text);
            }
            break;
        }
        case QXmlStreamReader::EndElement: {
            const QStringRef currentTag = m_xmlReader.name();

            if (currentTag == CatchXml::SectionElement) {
                sendResult(ResultType::TestEnd);
                testOutputNodeFinished(SectionNode);
            } else if (currentTag == CatchXml::TestCaseElement) {
                sendResult(ResultType::TestEnd);
                testOutputNodeFinished(TestCaseNode);
            } else if (currentTag == CatchXml::GroupElement) {
                testOutputNodeFinished(GroupNode);
            } else if (currentTag == CatchXml::ExpressionElement
                       || currentTag == CatchXml::BenchmarkResults) {
                sendResult(m_currentResult);
                m_currentExpression.clear();
                m_testCaseInfo.pop();
            }
            break;
        }
        default:
            // ignore
            break;
        }
    }
}

TestResultPtr CatchOutputReader::createDefaultResult() const
{
    CatchResult *result = nullptr;
    if (m_testCaseInfo.size() > 0) {
        result = new CatchResult(id(), m_testCaseInfo.first().name);
        result->setDescription(m_testCaseInfo.last().name);
        result->setLine(m_testCaseInfo.last().line);
        const QString &relativePathFromBuildDir = m_testCaseInfo.last().filename;
        if (!relativePathFromBuildDir.isEmpty()) {
            const QFileInfo fileInfo(m_buildDir + '/' + relativePathFromBuildDir);
            result->setFileName(fileInfo.canonicalFilePath());
        }
    } else {
        result = new CatchResult(id(), QString());
    }
    result->setSectionDepth(m_sectionDepth);

    return TestResultPtr(result);
}

void CatchOutputReader::recordTestInformation(const QXmlStreamAttributes &attributes)
{
    QString name;
    if (attributes.hasAttribute("name"))  // successful expressions do not have one
        name = attributes.value("name").toString();
    else if (!m_testCaseInfo.isEmpty())
        name = m_testCaseInfo.top().name;

    m_testCaseInfo.append(TestOutputNode{
        name,
        attributes.value("filename").toString(),
        attributes.value("line").toInt()
    });
    if (attributes.hasAttribute("tags")) {
        const QString tags = attributes.value("tags").toString();
        m_mayFail = tags.contains("[!mayfail]");
        m_shouldFail = tags.contains("[!shouldfail]");
    }
}

void CatchOutputReader::recordBenchmarkInformation(const QXmlStreamAttributes &attributes)
{
    QString name = attributes.value("name").toString();
    QString fileName;
    int line = 0;
    if (!m_testCaseInfo.isEmpty()) {
        fileName = m_testCaseInfo.top().filename;
        line = m_testCaseInfo.top().line;
    }
    m_testCaseInfo.append(TestOutputNode{name, fileName, line});

    m_currentExpression.append(name);
    recordBenchmarkDetails(attributes, {{{"samples"}, {"samples"}},
                                        {{"iterations"}, {"iterations"}},
                                        {{"estimated duration"}, {"estimatedDuration"}}});
    m_currentExpression.append(" ms");  // ugly
}

void CatchOutputReader::recordBenchmarkDetails(
        const QXmlStreamAttributes &attributes,
        const QList<QPair<QString, QString>> &stringAndAttrNames)
{
    m_currentExpression.append('\n');
    int counter = 0;
    for (const QPair<QString, QString> &curr : stringAndAttrNames) {
        m_currentExpression.append(curr.first).append(": ");
        m_currentExpression.append(attributes.value(curr.second).toString());
        if (counter < stringAndAttrNames.size() - 1)
            m_currentExpression.append(", ");
        ++counter;
    }
}

void CatchOutputReader::sendResult(const ResultType result)
{
    TestResultPtr catchResult = createDefaultResult();
    catchResult->setResult(result);

    if (result == ResultType::TestStart && m_testCaseInfo.size() > 0) {
        catchResult->setDescription(tr("Executing %1 \"%2\"").arg(testOutputNodeToString().toLower())
                                    .arg(catchResult->description()));
    } else if (result == ResultType::Pass || result == ResultType::UnexpectedPass) {
        if (result == ResultType::UnexpectedPass)
            ++m_xpassCount;

        if (m_currentExpression.isEmpty()) {
            catchResult->setDescription(tr("%1 \"%2\" passed").arg(testOutputNodeToString())
                                        .arg(catchResult->description()));
        } else {
            catchResult->setDescription(tr("Expression passed")
                                        .append('\n').append(m_currentExpression));
        }
        m_reportedSectionResult = true;
        m_reportedResult = true;
    } else if (result == ResultType::Fail || result == ResultType::ExpectedFail) {
        catchResult->setDescription(tr("Expression failed: %1").arg(m_currentExpression.trimmed()));
        if (!m_reportedSectionResult)
            m_reportedSectionResult = true;
        m_reportedResult = true;
    } else if (result == ResultType::TestEnd) {
        catchResult->setDescription(tr("Finished executing %1 \"%2\"").arg(testOutputNodeToString().toLower())
                                    .arg(catchResult->description()));
    } else if (result == ResultType::Benchmark) {
        catchResult->setDescription(m_currentExpression);
    }

    reportResult(catchResult);
}

void CatchOutputReader::testOutputNodeStarted(CatchOutputReader::TestOutputNodeType type)
{
    m_currentTestNode = type;
    if (type == SectionNode) {
        ++m_sectionDepth;
        m_reportedSectionResult = false;
    }
}

void CatchOutputReader::testOutputNodeFinished(CatchOutputReader::TestOutputNodeType type)
{
    switch (type) {
    case GroupNode:
        m_currentTestNode = OverallNode;
        return;
    case TestCaseNode:
        m_currentTestNode = GroupNode;
        m_testCaseInfo.pop();
        return;
    case SectionNode:
        --m_sectionDepth;
        m_testCaseInfo.pop();
        m_currentTestNode = m_sectionDepth == 0 ? TestCaseNode : SectionNode;
        return;
    default:
        return;
    }
}

QString CatchOutputReader::testOutputNodeToString() const
{
    switch (m_currentTestNode) {
    case OverallNode:
        return QStringLiteral("Overall");
    case GroupNode:
        return QStringLiteral("Group");
    case TestCaseNode:
        return QStringLiteral("Test case");
    case SectionNode:
        return QStringLiteral("Section");
    }

    return QString();
}

} // namespace Internal
} // namespace Autotest
