// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Utils {

template<typename Enumeration>
[[nodiscard]] constexpr std::underlying_type_t<Enumeration> to_underlying(Enumeration enumeration) noexcept
{
    return static_cast<std::underlying_type_t<Enumeration>>(enumeration);
}

} // namespace Utils
