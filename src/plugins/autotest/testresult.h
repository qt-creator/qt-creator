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

#ifndef TESTRESULT_H
#define TESTRESULT_H

#include "autotestconstants.h"

#include <QString>
#include <QColor>
#include <QMetaType>
#include <QSharedPointer>

namespace Autotest {
namespace Internal {

namespace Result{
enum Type {
    Pass, FIRST_TYPE = Pass,
    Fail,
    ExpectedFail,
    UnexpectedPass,
    Skip,
    BlacklistedPass,
    BlacklistedFail,
    Benchmark,
    MessageDebug,
    MessageWarn,
    MessageFatal,

    MessageInternal, INTERNAL_MESSAGES_BEGIN = MessageInternal,
    MessageDisabledTests,
    MessageTestCaseStart,
    MessageTestCaseSuccess,
    MessageTestCaseWarn,
    MessageTestCaseFail,
    MessageTestCaseEnd,
    MessageTestCaseRepetition,
    MessageCurrentTest, INTERNAL_MESSAGES_END = MessageCurrentTest,

    Invalid,
    LAST_TYPE = Invalid
};
}

class TestResult
{
public:
    TestResult();
    TestResult(const QString &className);

    QString className() const { return m_class; }
    QString testCase() const { return m_case; }
    QString dataTag() const { return m_dataTag; }
    Result::Type result() const { return m_result; }
    QString description() const { return m_description; }
    QString fileName() const { return m_file; }
    int line() const { return m_line; }
    TestType type() const { return m_type; }

    void setDescription(const QString &description) { m_description = description; }
    void setFileName(const QString &fileName) { m_file = fileName; }
    void setLine(int line) { m_line = line; }
    void setResult(Result::Type type) { m_result = type; }
    void setTestCase(const QString &testCase) { m_case = testCase; }
    void setDataTag(const QString &dataTag) { m_dataTag = dataTag; }
    void setTestType(TestType type) { m_type = type; }

    static Result::Type resultFromString(const QString &resultString);
    static Result::Type toResultType(int rt);
    static QString resultToString(const Result::Type type);
    static QColor colorForType(const Result::Type type);

private:
    QString m_class;
    QString m_case;
    QString m_dataTag;
    Result::Type m_result;
    QString m_description;
    QString m_file;
    int m_line;
    TestType m_type;
    // environment?
};

using TestResultPtr = QSharedPointer<TestResult>;

class FaultyTestResult : public TestResult
{
public:
    FaultyTestResult(Result::Type result, const QString &description);
};

class QTestResult : public TestResult
{
public:
    QTestResult(const QString &className = QString());
};

class GTestResult : public TestResult
{
public:
    GTestResult(const QString &className = QString());
};

bool operator==(const TestResult &t1, const TestResult &t2);

} // namespace Internal
} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::Internal::TestResult)
Q_DECLARE_METATYPE(Autotest::Internal::Result::Type)

#endif // TESTRESULT_H
