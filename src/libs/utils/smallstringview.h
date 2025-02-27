// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "algorithm.h"
#include "smallstringfwd.h"
#include "smallstringiterator.h"

#include <QString>

#include <cstring>
#include <string>
#include <string_view>

namespace Utils {

template <typename String>
using enable_if_has_char_data_pointer = typename std::enable_if_t<
                                            std::is_same<
                                                std::remove_const_t<
                                                    std::remove_pointer_t<
                                                        decltype(std::declval<const String>().data())
                                                        >
                                                    >, char>::value
                                            , int>;

class SmallStringView : public std::string_view
{
public:
    using std::string_view::string_view;

    template<typename String, typename Utils::enable_if_has_char_data_pointer<String> = 0>
    constexpr SmallStringView(const String &string) noexcept
        : std::string_view{string.data(), static_cast<std::size_t>(string.size())}
    {}

    constexpr size_type isEmpty() const noexcept { return empty(); }

    operator std::string() const { return std::string(data(), size()); }

    explicit operator QString() const { return QString::fromUtf8(data(), int(size())); }

    operator QByteArrayView() const { return QByteArrayView(data(), Utils::ssize(*this)); }

    explicit operator QByteArray() const { return QByteArrayView{*this}.toByteArray(); }

    QByteArray toByteArray() const { return QByteArrayView{*this}.toByteArray(); }

    explicit operator QLatin1StringView() const noexcept
    {
        return QLatin1StringView(data(), Utils::ssize(*this));
    }

    operator QUtf8StringView() const noexcept
    {
        return QUtf8StringView(data(), Utils::ssize(*this));
    }
};

inline constexpr auto operator<=>(const SmallStringView &first, const SmallStringView &second)
{
    return std::string_view{first} <=> std::string_view{second};
}

inline constexpr bool operator==(const SmallStringView &first, const SmallStringView &second)
{
    return std::string_view{first} == std::string_view{second};
}

constexpr int compare(SmallStringView first, SmallStringView second) noexcept
{
    return first.compare(second);
}

} // namespace Utils

constexpr Utils::SmallStringView operator""_sv(const char *const string, size_t size)
{
    return Utils::SmallStringView(string, size);
}
