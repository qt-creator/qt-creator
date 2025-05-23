// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Utils {

// https://www.cppstories.com/2018/09/visit-variants/
// https://en.cppreference.com/w/cpp/utility/variant/visit
// https://en.cppreference.com/w/cpp/utility/variant/visit2
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };

template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

} // Utils
