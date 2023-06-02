// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../testoutputreader.h"

#include "qttestconstants.h"

#include <QXmlStreamReader>

namespace Autotest {
namespace Internal {

class QtTestResult;

class QtTestOutputReader : public TestOutputReader
{
public:
    enum OutputMode
    {
        XML,
        PlainText
    };

    QtTestOutputReader(Utils::Process *testApplication, const Utils::FilePath &buildDirectory,
                       const Utils::FilePath &projectFile, OutputMode mode, TestType type);
protected:
    void processOutputLine(const QByteArray &outputLine) override;
    TestResult createDefaultResult() const override;

private:
    void processXMLOutput(const QByteArray &outputLine);
    void processPlainTextOutput(const QByteArray &outputLine);
    void processResultOutput(const QString &result, const QString &message);
    void processLocationOutput(const QString &fileWithLine);
    void processSummaryFinishOutput();
    // helper functions
    void sendCompleteInformation();
    void sendMessageCurrentTest();
    void sendStartMessage(bool isFunction);
    void sendFinishMessage(bool isFunction);
    void handleAndSendConfigMessage(const QRegularExpressionMatch &config);

    enum CDATAMode
    {
        None,
        DataTag,
        Description,
        QtVersion,
        QtBuild,
        QTestVersion
    };

    CDATAMode m_cdataMode = None;
    Utils::FilePath m_projectFile;
    QString m_className;
    QString m_testCase;
    QString m_formerTestCase;
    QString m_dataTag;
    ResultType m_result = ResultType::Invalid;
    QString m_description;
    Utils::FilePath m_file;
    int m_lineNumber = 0;
    QString m_duration;
    QXmlStreamReader m_xmlReader;
    OutputMode m_mode = XML;
    TestType m_testType = TestType::QtTest;
    bool m_expectTag = true;
};

} // namespace Internal
} // namespace Autotest
