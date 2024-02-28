// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qcompilerdetection.h>

// The (Apple) Clang implementation of span is incomplete until LLVM 15 / Xcode 14.3 / macOS 13
#if __cplusplus >= 202002L \
    && !(defined(__apple_build_version__) && __apple_build_version__ < 14030022)
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

// disable automatic usage of std::span in span-lite
// since we make that decision ourselves at the top of this header
#define span_CONFIG_SELECT_SPAN span_SPAN_NONSTD

#include <3rdparty/span/span.hpp>
namespace Utils {
using namespace nonstd;
}

QT_WARNING_POP
#endif
