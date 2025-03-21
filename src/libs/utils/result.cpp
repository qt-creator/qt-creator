// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "result.h"

#include "utilstr.h"

namespace Utils {

const Result Result::Ok;

Result::Result(bool success, const QString &errorString)
{
    if (!success)
        m_error = errorString;
}

Result::Result(const expected_str<void> &res)
{
    if (!res)
        m_error = res.error();
}

Result::~Result() = default;

Result Result::Error(const QString &errorString)
{
    return Result(errorString);
}

Result Result::Error(SpecialError specialError)
{
    switch (specialError) {
    case Unimplemented:
        return Error(Tr::tr("Not implemented"));
    case Assert:
        return Error(Tr::tr("Internal error"));
    }
    return Error(Tr::tr("Internal error"));
}

} // Utils
