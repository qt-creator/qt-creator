// Copyright (C) 2019 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../testoutputreader.h"

#include <QStack>
#include <QXmlStreamReader>

namespace Autotest {
namespace Internal {

class CatchOutputReader : public TestOutputReader
{
public:
    CatchOutputReader(Utils::Process *testApplication, const Utils::FilePath &buildDirectory,
                      const Utils::FilePath &projectFile);

protected:
    void processOutputLine(const QByteArray &outputLineWithNewLine) override;
    TestResult createDefaultResult() const override;

private:
    enum TestOutputNodeType {
        OverallNode,
        GroupNode,
        TestCaseNode,
        SectionNode
    } m_currentTestNode = OverallNode;

    struct TestOutputNode {
        QString name;
        QString filename;
        int line;
    };

    void recordTestInformation(const QXmlStreamAttributes &attributes);
    void recordBenchmarkInformation(const QXmlStreamAttributes &attributes);
    void recordBenchmarkDetails(const QXmlStreamAttributes &attributes,
                                const QList<QPair<QString, QString>> &stringAndAttrNames);
    void sendResult(const ResultType result);

    void testOutputNodeStarted(TestOutputNodeType type);
    void testOutputNodeFinished(TestOutputNodeType type);

    QString testOutputNodeToString() const;

    QStack<TestOutputNode> m_testCaseInfo;
    int m_sectionDepth = 0;

    Utils::FilePath m_projectFile;
    QString m_currentTagName;
    QString m_currentExpression;
    QXmlStreamReader m_xmlReader;
    ResultType m_currentResult = ResultType::Invalid;
    int m_xpassCount = 0;
    bool m_mayFail = false;
    bool m_shouldFail = false;
    bool m_reportedResult = false;
    bool m_reportedSectionResult = false;
};

} // namespace Internal
} // namespace Autotest
