// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <nanotrace/nanotracehr.h>

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

    template<typename String>
    friend void convertToString(String &string, source_location sourceLocation)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("file", sourceLocation.m_fileName),
                              keyValue("function", sourceLocation.m_functionName),
                              keyValue("line", sourceLocation.m_line));
        convertToString(string, dict);

        string.append(',');
        convertToString(string, "id");
        string.append(':');
        string.append('\"');
        string.append(sourceLocation.m_functionName);
        string.append(':');
        string.append(sourceLocation.m_line);
        string.append('\"');
    }

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
