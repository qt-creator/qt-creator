// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <utils/smallstringview.h>

#if !(defined(__cpp_lib_to_chars) && (__cpp_lib_to_chars >= 201611L))
#  include <QLocale>
#endif

#include <array>
#include <charconv>
#include <limits>

namespace NanotraceHR {

template<std::size_t Capacity>
class StaticString
{
public:
    StaticString() = default;
    StaticString(const StaticString &) = delete;
    StaticString &operator=(const StaticString &) = delete;

    char *data() { return m_data.data(); }

    const char *data() const { return m_data.data(); }

    void append(Utils::SmallStringView string) noexcept
    {
        auto newSize = m_size + string.size();

        if (newSize < Capacity) {
            std::char_traits<char>::copy(std::next(data(), static_cast<std::ptrdiff_t>(m_size)),
                                         string.data(),
                                         string.size());
            m_size = newSize;
        } else {
            m_size = std::numeric_limits<std::size_t>::max();
        }
    }

    void append(char character) noexcept
    {
        auto newSize = m_size + 1;

        if (newSize < Capacity) {
            auto current = std::next(data(), static_cast<std::ptrdiff_t>(m_size));
            *current = character;

            m_size = newSize;
        } else {
            m_size = std::numeric_limits<std::size_t>::max();
        }
    }

    template<typename Type, typename std::enable_if_t<std::is_arithmetic_v<Type>, bool> = true>
    void append(Type number)
    {
#if !(defined(__cpp_lib_to_chars) && (__cpp_lib_to_chars >= 201611L))
        if constexpr (std::is_floating_point_v<Type>) {
            QLocale locale{QLocale::Language::C};
            append(locale.toString(number).toStdString());
            return;
        }
#endif
        // 2 bytes for the sign and because digits10 returns the floor
        char buffer[std::numeric_limits<Type>::digits10 + 2];
        auto result = std::to_chars(buffer, buffer + sizeof(buffer), number);
        auto endOfConversionString = result.ptr;

        append({buffer, endOfConversionString});
    }

    void pop_back() { --m_size; }

    StaticString &operator+=(Utils::SmallStringView string) noexcept
    {
        append(string);

        return *this;
    }

    StaticString &operator+=(char character) noexcept
    {
        append(character);

        return *this;
    }

    template<typename Type, typename = std::enable_if_t<std::is_arithmetic_v<Type>>>
    StaticString &operator+=(Type number) noexcept
    {
        append(number);

        return *this;
    }

    bool isValid() const { return m_size != std::numeric_limits<std::size_t>::max(); }

    std::size_t size() const { return m_size; }

    friend std::ostream &operator<<(std::ostream &out, const StaticString &text)
    {
        return out << std::string_view{text.data(), text.size()};
    }

    void clear() { m_size = 0; }

private:
    std::array<char, Capacity> m_data;
    std::size_t m_size = 0;
};

} // namespace NanotraceHR
