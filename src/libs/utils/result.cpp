// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "result.h"

#include "utilstr.h"

namespace Utils {

const Result<> ResultOk;

ResultError::ResultError(const QString &errorMessage)
    : m_error(errorMessage)
{}

ResultError::ResultError(ResultUnimplementedType)
    : m_error(Tr::tr("Not implemented"))
{}
ResultError::ResultError(ResultAssertType, const QString &errorMessage)
    : m_error(Tr::tr("Internal error: %1").arg(errorMessage))
{}

Result<> makeResult(bool ok, const QString &errorMessage)
{
    if (ok)
        return ResultOk;
    return tl::make_unexpected(errorMessage);
}

} // Utils
