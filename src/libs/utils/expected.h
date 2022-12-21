// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtcassert.h"

#include "../3rdparty/tl_expected/include/tl/expected.hpp"

namespace Utils {

template<class T, class E>
using expected = tl::expected<T, E>;

template<class T>
using expected_str = tl::expected<T, QString>;

template<class E>
using unexpected = tl::unexpected<E>;
using unexpect_t = tl::unexpect_t;

static constexpr unexpect_t unexpect{};

template<class E>
constexpr unexpected<std::decay_t<E>> make_unexpected(E &&e)
{
    return tl::make_unexpected(e);
}

} // namespace Utils

//! If 'expected' has an error the error will be printed and the 'action' will be executed.
#define QTC_ASSERT_EXPECTED(expected, action) \
    { \
        if (Q_LIKELY(expected)) { \
        } else { \
            ::Utils::writeAssertLocation(QString("%1:%2: %3") \
                                             .arg(__FILE__) \
                                             .arg(__LINE__) \
                                             .arg(expected.error()) \
                                             .toUtf8() \
                                             .data()); \
            action; \
        } \
        do { \
        } while (0); \
    }
