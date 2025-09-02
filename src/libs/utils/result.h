// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "expected.h"
#include "qtcassert.h"

#include <QString>

namespace Utils {

#ifdef Q_QDOC
template<typename T = void>
class Result
{};
#endif

template<typename T = void>
using Result = Utils::expected<T, QString>;

QTCREATOR_UTILS_EXPORT extern const Result<> ResultOk;

QTCREATOR_UTILS_EXPORT Result<> makeResult(bool ok, const QString &errorMessage);

enum ResultSpecialErrorCode {
    ResultAssert,
    ResultUnimplemented,
};

class QTCREATOR_UTILS_EXPORT ResultError
{
public:
    ResultError(const QString &errorMessage);
    ResultError(ResultSpecialErrorCode code, const QString &errorMessage = {});

    template<typename T> operator Result<T>() { return tl::make_unexpected(m_error); }

private:
    QString m_error;
};

#define QTC_ASSERT_AND_ERROR_OUT(cond) \
  QTC_ASSERT(cond, \
    return Result::Error(QString("The condition %1 failed unexpectedly in %2:%3") \
       .arg(#cond).arg(__FILE__).arg(__LINE__)))

} // namespace Utils


//! If \a result has an error the error will be printed and the \a action will be executed.
#define QTC_ASSERT_RESULT(result, action) \
    if (Q_LIKELY(result)) { \
    } else { \
        ::Utils::writeAssertLocation(QString("%1:%2: %3") \
                                         .arg(__FILE__) \
                                         .arg(__LINE__) \
                                         .arg(result.error()) \
                                         .toUtf8() \
                                         .data()); \
        action; \
    } \
    do { \
    } while (0)

#define QTC_CHECK_RESULT(result) \
    if (Q_LIKELY(result)) { \
    } else { \
        ::Utils::writeAssertLocation( \
            QString("%1:%2: %3").arg(__FILE__).arg(__LINE__).arg(result.error()).toUtf8().data()); \
    } \
    do { \
    } while (0)

#define QVERIFY_RESULT(result) \
    QVERIFY2(result, result ? #result : static_cast<const char*>(result.error().toUtf8()))
