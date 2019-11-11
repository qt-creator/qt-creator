/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#pragma once

#include "../testoutputreader.h"

namespace Autotest {
namespace Internal {

class BoostTestResult;
enum class LogLevel;
enum class ReportLevel;

class BoostTestOutputReader : public TestOutputReader
{
    Q_OBJECT
public:
    BoostTestOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                          QProcess *testApplication, const QString &buildDirectory,
                          const QString &projectFile, LogLevel log, ReportLevel report);
protected:
    void processOutputLine(const QByteArray &outputLine) override;
    void processStdError(const QByteArray &outputLine) override;
    TestResultPtr createDefaultResult() const override;

private:
    void onFinished(int exitCode, QProcess::ExitStatus /*exitState*/);
    void sendCompleteInformation();
    void handleMessageMatch(const QRegularExpressionMatch &match);
    void reportNoOutputFinish(const QString &description, ResultType type);
    QString m_projectFile;
    QString m_currentModule;
    QString m_currentSuite;
    QString m_currentTest;
    QString m_description;
    QString m_fileName;
    ResultType m_result = ResultType::Invalid;
    int m_lineNumber = 0;
    int m_testCaseCount = -1;
    LogLevel m_logLevel;
    ReportLevel m_reportLevel;
};

} // namespace Internal
} // namespace Autotest
