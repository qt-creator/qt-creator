// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "expected.h"

#include <QString>

namespace Utils {

template<typename T = void>
using Result = Utils::expected<T, QString>;

QTCREATOR_UTILS_EXPORT extern const Result<> ResultOk;

QTCREATOR_UTILS_EXPORT Result<> makeResult(bool ok, const QString &errorMessage);

enum ResultUnimplementedType { ResultUnimplemented };
enum ResultAssertType { ResultAssert };

class QTCREATOR_UTILS_EXPORT ResultError
{
public:
    ResultError(const QString &errorMessage);
    ResultError(ResultUnimplementedType);
    ResultError(ResultAssertType, const QString &errorMessage = {});
    template<typename T> operator Result<T>() { return tl::make_unexpected(m_error); }

private:
    QString m_error;
};

#define QTC_ASSERT_AND_ERROR_OUT(cond) \
  QTC_ASSERT(cond, \
    return Result::Error(QString("The condition %1 failed unexpectedly in %2:%3") \
       .arg(#cond).arg(__FILE__).arg(__LINE__)))

} // namespace Utils
