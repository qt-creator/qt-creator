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

#include "testresult.h"

namespace Autotest {
namespace Internal {

FaultyTestResult::FaultyTestResult(Result::Type result, const QString &description)
{
    setResult(result);
    setDescription(description);
}

TestResult::TestResult()
    : TestResult(QString())
{
}

TestResult::TestResult(const QString &className)
    : m_class(className)
    , m_result(Result::Invalid)
    , m_line(0)
{
}

Result::Type TestResult::resultFromString(const QString &resultString)
{
    if (resultString == QLatin1String("pass"))
        return Result::Pass;
    if (resultString == QLatin1String("fail"))
        return Result::Fail;
    if (resultString == QLatin1String("xfail"))
        return Result::ExpectedFail;
    if (resultString == QLatin1String("xpass"))
        return Result::UnexpectedPass;
    if (resultString == QLatin1String("skip"))
        return Result::Skip;
    if (resultString == QLatin1String("qdebug"))
        return Result::MessageDebug;
    if (resultString == QLatin1String("warn") || resultString == QLatin1String("qwarn"))
        return Result::MessageWarn;
    if (resultString == QLatin1String("qfatal"))
        return Result::MessageFatal;
    if (resultString == QLatin1String("bpass"))
        return Result::BlacklistedPass;
    if (resultString == QLatin1String("bfail"))
        return Result::BlacklistedFail;
    qDebug("Unexpected test result: %s", qPrintable(resultString));
    return Result::Invalid;
}

Result::Type TestResult::toResultType(int rt)
{
    if (rt < Result::FIRST_TYPE || rt > Result::LAST_TYPE)
        return Result::Invalid;

    return (Result::Type)rt;
}

QString TestResult::resultToString(const Result::Type type)
{
    if (type >= Result::INTERNAL_MESSAGES_BEGIN && type <= Result::INTERNAL_MESSAGES_END)
        return QString();

    switch (type) {
    case Result::Pass:
        return QLatin1String("PASS");
    case Result::Fail:
        return QLatin1String("FAIL");
    case Result::ExpectedFail:
        return QLatin1String("XFAIL");
    case Result::UnexpectedPass:
        return QLatin1String("XPASS");
    case Result::Skip:
        return QLatin1String("SKIP");
    case Result::Benchmark:
        return QLatin1String("BENCH");
    case Result::MessageDebug:
        return QLatin1String("DEBUG");
    case Result::MessageWarn:
        return QLatin1String("WARN");
    case Result::MessageFatal:
        return QLatin1String("FATAL");
    case Result::BlacklistedPass:
        return QLatin1String("BPASS");
    case Result::BlacklistedFail:
        return QLatin1String("BFAIL");
    default:
        return QLatin1String("UNKNOWN");
    }
}

QColor TestResult::colorForType(const Result::Type type)
{
    if (type >= Result::INTERNAL_MESSAGES_BEGIN && type <= Result::INTERNAL_MESSAGES_END)
        return QColor("transparent");

    switch (type) {
    case Result::Pass:
        return QColor("#009900");
    case Result::Fail:
        return QColor("#a00000");
    case Result::ExpectedFail:
        return QColor("#00ff00");
    case Result::UnexpectedPass:
        return QColor("#ff0000");
    case Result::Skip:
        return QColor("#787878");
    case Result::BlacklistedPass:
        return QColor(0, 0, 0);
    case Result::BlacklistedFail:
        return QColor(0, 0, 0);
    case Result::MessageDebug:
        return QColor("#329696");
    case Result::MessageWarn:
        return QColor("#d0bb00");
    case Result::MessageFatal:
        return QColor("#640000");
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
