// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#if __cplusplus >= 202002L
#define constexpr20 constexpr
#else
#define constexpr20 inline
#endif

using uint = unsigned int;

namespace Utils {

class SmallStringView;
template <uint Size>
class BasicSmallString;
using SmallString = BasicSmallString<31>;
using PathString = BasicSmallString<190>;

} // namespace Utils
