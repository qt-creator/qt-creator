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

#ifndef TESTRESULT_H
#define TESTRESULT_H

#include <QString>
#include <QColor>
#include <QMetaType>

namespace Autotest {
namespace Internal {

namespace Result{
enum Type {
    PASS, FIRST_TYPE = PASS,
    FAIL,
    EXPECTED_FAIL,
    UNEXPECTED_PASS,
    SKIP,
    BLACKLISTED_PASS,
    BLACKLISTED_FAIL,
    BENCHMARK,
    MESSAGE_DEBUG,
    MESSAGE_WARN,
    MESSAGE_FATAL,

    MESSAGE_INTERNAL, INTERNAL_MESSAGES_BEGIN = MESSAGE_INTERNAL,
    MESSAGE_TEST_CASE_START,
    MESSAGE_TEST_CASE_SUCCESS,
    MESSAGE_TEST_CASE_WARN,
    MESSAGE_TEST_CASE_FAIL,
    MESSAGE_TEST_CASE_END,
    MESSAGE_CURRENT_TEST, INTERNAL_MESSAGES_END = MESSAGE_CURRENT_TEST,

    INVALID,
    LAST_TYPE = INVALID
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

    void setDescription(const QString &description) { m_description = description; }
    void setFileName(const QString &fileName) { m_file = fileName; }
    void setLine(int line) { m_line = line; }
    void setResult(Result::Type type) { m_result = type; }
    void setTestCase(const QString &testCase) { m_case = testCase; }
    void setDataTag(const QString &dataTag) { m_dataTag = dataTag; }

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
    // environment?
};

class FaultyTestResult : public TestResult
{
public:
    FaultyTestResult(Result::Type result, const QString &description);
};

bool operator==(const TestResult &t1, const TestResult &t2);

} // namespace Internal
} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::Internal::TestResult)
Q_DECLARE_METATYPE(Autotest::Internal::Result::Type)

#endif // TESTRESULT_H
