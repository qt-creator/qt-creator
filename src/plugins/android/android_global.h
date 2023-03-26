// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

#if defined(ANDROID_LIBRARY)
#  define ANDROID_EXPORT Q_DECL_EXPORT
#elif defined(ANDROID_STATIC_LIBRARY)
#  define ANDROID_EXPORT
#else
#  define ANDROID_EXPORT Q_DECL_IMPORT
#endif
