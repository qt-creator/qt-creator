// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

#if defined(ACPLIB_LIBRARY)
#  define ACPLIB_EXPORT Q_DECL_EXPORT
#elif defined(ACPLIB_STATIC_LIBRARY)
#  define ACPLIB_EXPORT
#else
#  define ACPLIB_EXPORT Q_DECL_IMPORT
#endif
