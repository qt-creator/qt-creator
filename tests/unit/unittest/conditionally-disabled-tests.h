// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtGlobal>

#ifdef CLANG_UNIT_TESTS
#include <clang-c/Index.h>

#if CINDEX_VERSION_MAJOR == 0 && CINDEX_VERSION_MINOR == 59
#  define DISABLED_FOR_CLANG_10(x) DISABLED_##x
#else
#  define DISABLED_FOR_CLANG_10(x) x
#endif
#endif

#ifdef Q_OS_WIN
#  define DISABLED_ON_WINDOWS(x) DISABLED_##x
#else
#  define DISABLED_ON_WINDOWS(x) x
#endif

#ifndef Q_OS_WIN
#  define DISABLED_ON_NON_WINDOWS(x) DISABLED_##x
#else
#  define DISABLED_ON_NON_WINDOWS(x) x
#endif
