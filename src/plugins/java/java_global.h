// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(JAVA_LIBRARY)
#  define JAVA_EXPORT Q_DECL_EXPORT
#elif defined(JAVA_STATIC_LIBRARY)
#  define JAVA_EXPORT
#else
#  define JAVA_EXPORT Q_DECL_IMPORT
#endif
