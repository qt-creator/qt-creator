// Copyright (C) 2019 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "catchoutputreader.h"

#include "catchresult.h"

#include "../autotesttr.h"

using namespace Utils;

namespace Autotest {
namespace Internal {

namespace CatchXml {
    const char GroupElement[]          = "Group";
    const char TestCaseElement[]       = "TestCase";
    const char SectionElement[]        = "Section";
    const char ExceptionElement[]      = "Exception";
    const char InfoElement[]           = "Info";
    const char WarningElement[]        = "Warning";
    const char FailureElement[]        = "Failure";
    const char ExpressionElement[]     = "Expression";
    const char ExpandedElement[]       = "Expanded";
    const char BenchmarkResults[]      = "BenchmarkResults";
    const char MeanElement[]           = "mean";
    const char StandardDevElement[]    = "standardDeviation";
    const char SectionResultElement[]  = "OverallResults";
    const char TestCaseResultElement[] = "OverallResult";
}

CatchOutputReader::CatchOutputReader(Process *testApplication,
                                     const FilePath &buildDirectory,
                                     const FilePath &projectFile)
    : TestOutputReader(testApplication, buildDirectory)
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
            } else if (m_currentTagName == CatchXml::ExceptionElement) {
                recordTestInformation(m_xmlReader.attributes());
                m_currentExpression = m_currentExpression.prepend(Tr::tr("Exception:"));
                m_currentResult = ResultType::MessageFatal;
            } else if (m_currentTagName == CatchXml::ExpressionElement) {
                recordTestInformation(m_xmlReader.attributes());
                if (m_xmlReader.attributes().value("success").toString() == QStringLiteral("true"))
                    m_currentResult = m_shouldFail ? ResultType::UnexpectedPass : ResultType::Pass;
                else
                    m_currentResult = m_mayFail || m_shouldFail ? ResultType::ExpectedFail : ResultType::Fail;
            } else if (m_currentTagName == CatchXml::WarningElement) {
                m_currentResult = ResultType::MessageWarn;
            } else if (m_currentTagName == CatchXml::InfoElement) {
                m_currentResult = ResultType::MessageInfo;
            } else if (m_currentTagName == CatchXml::FailureElement) {
                m_currentResult = ResultType::Fail;
                recordTestInformation(m_xmlReader.attributes());
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
            const auto text = m_xmlReader.text();
            if (m_currentTagName == CatchXml::ExpandedElement) {
                m_currentExpression.append(text);
            } else if (m_currentTagName == CatchXml::ExceptionElement
                       || m_currentTagName == CatchXml::InfoElement
                       || m_currentTagName == CatchXml::WarningElement
                       || m_currentTagName == CatchXml::FailureElement) {
                m_currentExpression.append('\n').append(text.trimmed());
            }
            break;
        }
        case QXmlStreamReader::EndElement: {
            const auto currentTag = m_xmlReader.name();

            if (currentTag == QLatin1String(CatchXml::SectionElement)) {
                sendResult(ResultType::TestEnd);
                testOutputNodeFinished(SectionNode);
            } else if (currentTag == QLatin1String(CatchXml::TestCaseElement)) {
                sendResult(ResultType::TestEnd);
                testOutputNodeFinished(TestCaseNode);
            } else if (currentTag == QLatin1String(CatchXml::GroupElement)) {
                testOutputNodeFinished(GroupNode);
            } else if (currentTag == QLatin1String(CatchXml::ExpressionElement)
                       || currentTag == QLatin1String(CatchXml::FailureElement)
                       || currentTag == QLatin1String(CatchXml::BenchmarkResults)) {
                sendResult(m_currentResult);
                m_currentExpression.clear();
                m_testCaseInfo.pop();
            } else if (currentTag == QLatin1String(CatchXml::WarningElement)
                       || currentTag == QLatin1String(CatchXml::InfoElement)) {
                sendResult(m_currentResult);
                m_currentExpression.clear();
            }
            break;
        }
        default:
            // ignore
            break;
        }
    }
}

TestResult CatchOutputReader::createDefaultResult() const
{
    if (m_testCaseInfo.size() == 0)
        return CatchResult(id(), {}, m_sectionDepth);
    CatchResult result = CatchResult(id(), m_testCaseInfo.first().name, m_sectionDepth);
    result.setDescription(m_testCaseInfo.last().name);
    result.setLine(m_testCaseInfo.last().line);
    const QString givenPath = m_testCaseInfo.last().filename;
    if (!givenPath.isEmpty())
        result.setFileName(constructSourceFilePath(m_buildDir, givenPath));
    return result;
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
    TestResult catchResult = createDefaultResult();
    catchResult.setResult(result);

    if (result == ResultType::TestStart && m_testCaseInfo.size() > 0) {
        catchResult.setDescription(Tr::tr("Executing %1 \"%2\"...")
                   .arg(testOutputNodeToString().toLower(), catchResult.description()));
    } else if (result == ResultType::Pass || result == ResultType::UnexpectedPass) {
        if (result == ResultType::UnexpectedPass)
            ++m_xpassCount;

        if (m_currentExpression.isEmpty()) {
            catchResult.setDescription(Tr::tr("%1 \"%2\" passed.")
                       .arg(testOutputNodeToString(), catchResult.description()));
        } else {
            catchResult.setDescription(Tr::tr("Expression passed.")
                                        .append('\n').append(m_currentExpression));
        }
        m_reportedSectionResult = true;
        m_reportedResult = true;
    } else if (result == ResultType::Fail || result == ResultType::ExpectedFail) {
        catchResult.setDescription(Tr::tr("Expression failed: %1")
                   .arg(m_currentExpression.trimmed()));
        if (!m_reportedSectionResult)
            m_reportedSectionResult = true;
        m_reportedResult = true;
    } else if (result == ResultType::TestEnd) {
        catchResult.setDescription(Tr::tr("Finished executing %1 \"%2\".")
                   .arg(testOutputNodeToString().toLower(), catchResult.description()));
    } else if (result == ResultType::Benchmark || result == ResultType::MessageFatal) {
        catchResult.setDescription(m_currentExpression);
    } else if (result == ResultType::MessageWarn || result == ResultType::MessageInfo) {
        catchResult.setDescription(m_currentExpression.trimmed());
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
    return {};
}

} // namespace Internal
} // namespace Autotest
