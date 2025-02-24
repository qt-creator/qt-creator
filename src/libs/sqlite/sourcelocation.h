// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cstdint>

namespace Sqlite {

struct source_location
{
public:
    static consteval source_location current(const char *fileName = __builtin_FILE(),
                                             const char *functionName = __builtin_FUNCTION(),
                                             const uint_least32_t line = __builtin_LINE()) noexcept
    {
        return {fileName, functionName, line};
    }

    constexpr source_location() noexcept = default;

    constexpr std::uint_least32_t line() const noexcept { return m_line; }

    constexpr const char *file_name() const noexcept { return m_fileName; }

    constexpr const char *function_name() const noexcept { return m_functionName; }

private:
    consteval source_location(const char *fileName, const char *functionName, const uint_least32_t line)
        : m_fileName{fileName}
        , m_functionName{functionName}
        , m_line{line}
    {}

private:
    const char *m_fileName = "";
    const char *m_functionName = "";
    std::uint_least32_t m_line = 0;
};

} // namespace Sqlite
