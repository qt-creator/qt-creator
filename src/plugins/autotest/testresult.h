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

#pragma once

#include "autotestconstants.h"

#include <QString>
#include <QColor>
#include <QMetaType>
#include <QSharedPointer>

namespace Autotest {
namespace Internal {

class TestTreeItem;

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
    MessageInfo,
    MessageWarn,
    MessageFatal,
    MessageSystem,

    MessageInternal, INTERNAL_MESSAGES_BEGIN = MessageInternal,
    MessageDisabledTests,
    MessageTestCaseStart,
    MessageTestCaseSuccess,
    MessageTestCaseSuccessWarn,
    MessageTestCaseFail,
    MessageTestCaseFailWarn,
    MessageTestCaseEnd,
    MessageIntermediate,
    MessageCurrentTest, INTERNAL_MESSAGES_END = MessageCurrentTest,

    Invalid,
    LAST_TYPE = Invalid
};
}

class TestResult
{
public:
    TestResult();
    explicit TestResult(const QString &name);
    TestResult(const QString &executable, const QString &name);
    virtual ~TestResult() {}

    virtual const QString outputString(bool selected) const;
    virtual const TestTreeItem *findTestTreeItem() const;

    QString executable() const { return m_executable; }
    QString name() const { return m_name; }
    Result::Type result() const { return m_result; }
    QString description() const { return m_description; }
    QString fileName() const { return m_file; }
    int line() const { return m_line; }

    void setDescription(const QString &description) { m_description = description; }
    void setFileName(const QString &fileName) { m_file = fileName; }
    void setLine(int line) { m_line = line; }
    void setResult(Result::Type type) { m_result = type; }

    static Result::Type resultFromString(const QString &resultString);
    static Result::Type toResultType(int rt);
    static QString resultToString(const Result::Type type);
    static QColor colorForType(const Result::Type type);
    static bool isMessageCaseStart(const Result::Type type);

    virtual bool isDirectParentOf(const TestResult *other, bool *needsIntermediate) const;
    virtual bool isIntermediateFor(const TestResult *other) const;
    virtual TestResult *createIntermediateResultFor(const TestResult *other);

private:
    QString m_executable;
    QString m_name;
    Result::Type m_result = Result::Invalid;
    QString m_description;
    QString m_file;
    int m_line = 0;
};

using TestResultPtr = QSharedPointer<TestResult>;

class FaultyTestResult : public TestResult
{
public:
    FaultyTestResult(Result::Type result, const QString &description);
};

} // namespace Internal
} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::Internal::TestResult)
Q_DECLARE_METATYPE(Autotest::Internal::Result::Type)
