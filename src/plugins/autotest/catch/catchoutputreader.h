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

#pragma once

#include "../testoutputreader.h"

#include <QCoreApplication>
#include <QStack>
#include <QXmlStreamReader>

namespace Autotest {
namespace Internal {

class CatchOutputReader : public TestOutputReader
{
    Q_DECLARE_TR_FUNCTIONS(Autotest::Internal::CatchOutputReader)

public:
    CatchOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                      QProcess *testApplication, const QString &buildDirectory,
                      const QString &projectFile);

protected:
    void processOutputLine(const QByteArray &outputLineWithNewLine) override;
    TestResultPtr createDefaultResult() const override;

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

    QString m_projectFile;
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
