// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qcompilerdetection.h>

#if __cplusplus >= 202002L
#include <span>

namespace Utils {
using std::as_bytes;
using std::as_writable_bytes;
using std::get;
using std::span;
} // namespace Utils
#else
QT_WARNING_PUSH

#if defined(Q_CC_MSVC)
#pragma system_header
#elif defined(Q_CC_GNU) || defined(Q_CC_CLANG)
#pragma GCC system_header
#endif
#include <3rdparty/span/span.hpp>
namespace Utils {
using namespace nonstd;
}

QT_WARNING_POP
#endif
