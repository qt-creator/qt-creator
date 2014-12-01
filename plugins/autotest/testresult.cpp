/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
**
****************************************************************************/

#include "testresult.h"

namespace Autotest {
namespace Internal {

TestResult::TestResult(const QString &className, const QString &testCase, const QString &dataTag,
                       ResultType result, const QString &description)
    : m_class(className),
      m_case(testCase),
      m_dataTag(dataTag),
      m_result(result),
      m_description(description),
      m_line(0)
{
}

ResultType TestResult::resultFromString(const QString &resultString)
{
    if (resultString == QLatin1String("pass"))
        return PASS;
    if (resultString == QLatin1String("fail"))
        return FAIL;
    if (resultString == QLatin1String("xfail"))
        return EXPECTED_FAIL;
    if (resultString == QLatin1String("xpass"))
        return UNEXPECTED_PASS;
    if (resultString == QLatin1String("skip"))
        return SKIP;
    if (resultString == QLatin1String("qdebug"))
        return MESSAGE_DEBUG;
    if (resultString == QLatin1String("warn") || resultString == QLatin1String("qwarn"))
        return MESSAGE_WARN;
    if (resultString == QLatin1String("qfatal"))
        return MESSAGE_FATAL;
    if (resultString == QLatin1String("bpass"))
        return BLACKLISTED_PASS;
    if (resultString == QLatin1String("bfail"))
        return BLACKLISTED_FAIL;
    qDebug(" unexpected testresult...");
    qDebug(resultString.toLatin1());
    return UNKNOWN;
}

ResultType TestResult::toResultType(int rt)
{
    switch(rt) {
    case PASS:
        return PASS;
    case FAIL:
        return FAIL;
    case EXPECTED_FAIL:
        return EXPECTED_FAIL;
    case UNEXPECTED_PASS:
        return UNEXPECTED_PASS;
    case SKIP:
        return SKIP;
    case BLACKLISTED_PASS:
        return BLACKLISTED_PASS;
    case BLACKLISTED_FAIL:
        return BLACKLISTED_FAIL;
    case BENCHMARK:
        return BENCHMARK;
    case MESSAGE_DEBUG:
        return MESSAGE_DEBUG;
    case MESSAGE_WARN:
        return MESSAGE_WARN;
    case MESSAGE_FATAL:
        return MESSAGE_FATAL;
    case MESSAGE_INTERNAL:
        return MESSAGE_INTERNAL;
    default:
        return UNKNOWN;
    }
}

QString TestResult::resultToString(const ResultType type)
{
    switch(type) {
    case PASS:
        return QLatin1String("PASS");
    case FAIL:
        return QLatin1String("FAIL");
    case EXPECTED_FAIL:
        return QLatin1String("XFAIL");
    case UNEXPECTED_PASS:
        return QLatin1String("XPASS");
    case SKIP:
        return QLatin1String("SKIP");
    case BENCHMARK:
        return QLatin1String("BENCH");
    case MESSAGE_DEBUG:
        return QLatin1String("DEBUG");
    case MESSAGE_WARN:
        return QLatin1String("WARN");
    case MESSAGE_FATAL:
        return QLatin1String("FATAL");
    case MESSAGE_INTERNAL:
        return QString();
    case BLACKLISTED_PASS:
        return QLatin1String("BPASS");
    case BLACKLISTED_FAIL:
        return QLatin1String("BFAIL");
    default:
        return QLatin1String("UNKNOWN");
    }
}

QColor TestResult::colorForType(const ResultType type)
{
    switch(type) {
    case PASS:
        return QColor("#009900");
    case FAIL:
        return QColor("#a00000");
    case EXPECTED_FAIL:
        return QColor("#00ff00");
    case UNEXPECTED_PASS:
        return QColor("#ff0000");
    case SKIP:
        return QColor("#787878");
    case BLACKLISTED_PASS:
        return QColor(0, 0, 0);
    case BLACKLISTED_FAIL:
        return QColor(0, 0, 0);
    case MESSAGE_DEBUG:
        return QColor("#329696");
    case MESSAGE_WARN:
        return QColor("#d0bb00");
    case MESSAGE_FATAL:
        return QColor("#640000");
    case MESSAGE_INTERNAL:
        return QColor("transparent");
    default:
        return QColor("#000000");
    }
}

bool operator==(const TestResult &t1, const TestResult &t2)
{
    return t1.className() == t2.className()
            && t1.testCase() == t2.testCase()
            && t1.dataTag() == t2.dataTag()
            && t1.result() == t2.result();
}

} // namespace Internal
} // namespace Autotest
