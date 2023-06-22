// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

#if defined(SPINNER_LIBRARY)
#  define SPINNER_EXPORT Q_DECL_EXPORT
#elif defined(SPINNER_STATIC_LIBRARY)
#  define SPINNER_EXPORT
#else
#  define SPINNER_EXPORT Q_DECL_IMPORT
#endif
