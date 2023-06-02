// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

#if defined(QTSUPPORT_LIBRARY)
#  define QTSUPPORT_EXPORT Q_DECL_EXPORT
#elif defined(QTSUPPORT_STATIC_LIBRARY)
#  define QTSUPPORT_EXPORT
#else
#  define QTSUPPORT_EXPORT Q_DECL_IMPORT
#endif

#if defined(WITH_TESTS)
#  if defined(QTSUPPORT_LIBRARY)
#    define QTSUPPORT_TEST_EXPORT Q_DECL_EXPORT
#  elif defined(QTSUPPORT_STATIC_LIBRARY)
#    define QTSUPPORT_TEST_EXPORT
#  else
#    define QTSUPPORT_TEST_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define QTSUPPORT_TEST_EXPORT
#endif
