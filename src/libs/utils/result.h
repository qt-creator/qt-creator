// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "expected.h"

#include <QString>

namespace Utils {

class QTCREATOR_UTILS_EXPORT Result
{
    Result() = default;
    explicit Result(const std::optional<QString> &err) : m_error(err) {}

public:
    Result(bool success, const QString &errorString);
    Result(const expected_str<void> &res);
    ~Result();

    static const Result Ok;
    static Result Error(const QString &errorString) { return Result(errorString); };

    QString error() const { return m_error ? *m_error : QString(); }
    operator bool() const { return !m_error; }

private:
    std::optional<QString> m_error;
};

} // namespace Utils
