// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

#if defined(CORE_LIBRARY)
#  define CORE_EXPORT Q_DECL_EXPORT
#elif defined(CORE_STATIC_LIBRARY) // Abuse single files for manual tests
#  define CORE_EXPORT
#else
#  define CORE_EXPORT Q_DECL_IMPORT
#endif
