// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

#if defined(TASKING_LIBRARY)
#  define TASKING_EXPORT Q_DECL_EXPORT
#elif defined(TASKING_STATIC_LIBRARY)
#  define TASKING_EXPORT
#else
#  define TASKING_EXPORT Q_DECL_IMPORT
#endif
