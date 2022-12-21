// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

#if defined(RESOURCEEDITOR_LIBRARY)
#  define RESOURCE_EXPORT Q_DECL_EXPORT
#elif defined(RESOURCEEDITOR_STATIC_LIBRARY)
#  define RESOURCE_EXPORT
#else
#  define RESOURCE_EXPORT Q_DECL_IMPORT
#endif
