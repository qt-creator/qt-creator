// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "result.h"

#include "utilstr.h"

namespace Utils {

const Result<> ResultOk;

static QString messageForCode(ResultSpecialErrorCode code)
{
    switch (code) {
    case ResultAssert:
        return Tr::tr("Internal error: %1.");
    case ResultUnimplemented:
        return Tr::tr("Not implemented error: %1.");
    default:
        return Tr::tr("Unknown error: %1.");
    }
}

ResultError::ResultError(const QString &errorMessage)
    : m_error(errorMessage)
{}

ResultError::ResultError(ResultSpecialErrorCode code, const QString &errorMessage)
    : m_error(messageForCode(code).arg(
          errorMessage.isEmpty() ? Tr::tr("Unknown reason.") : errorMessage))
{}

Result<> makeResult(bool ok, const QString &errorMessage)
{
    if (ok)
        return ResultOk;
    return ResultError(errorMessage);
}

} // Utils
