// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QLoggingCategory>
#include <qglobal.h>

#if defined(EXTENSIONSYSTEM_LIBRARY)
#  define EXTENSIONSYSTEM_EXPORT Q_DECL_EXPORT
#elif defined(EXTENSIONSYSTEM_STATIC_LIBRARY)
#  define EXTENSIONSYSTEM_EXPORT
#else
#  define EXTENSIONSYSTEM_EXPORT Q_DECL_IMPORT
#endif

#if defined(WITH_TESTS)
#  if defined(EXTENSIONSYSTEM_LIBRARY)
#    define EXTENSIONSYSTEM_TEST_EXPORT Q_DECL_EXPORT
#  elif defined(EXTENSIONSYSTEM_STATIC_LIBRARY)
#    define EXTENSIONSYSTEM_TEST_EXPORT
#  else
#    define EXTENSIONSYSTEM_TEST_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define EXTENSIONSYSTEM_TEST_EXPORT
#endif

Q_DECLARE_LOGGING_CATEGORY(pluginLog)
