// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(TRACING_LIBRARY)
#  define TRACING_EXPORT Q_DECL_EXPORT
#elif defined(TRACING_STATIC_LIBRARY)
#  define TRACING_EXPORT
#else
#  define TRACING_EXPORT Q_DECL_IMPORT
#endif
