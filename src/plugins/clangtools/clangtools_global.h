// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(CLANGTOOLS_LIBRARY)
#  define CLANGTOOLS_EXPORT Q_DECL_EXPORT
#elif defined(CLANGTOOLS_STATIC_LIBRARY)
#  define CLANGTOOLS_EXPORT
#else
#  define CLANGTOOLS_EXPORT Q_DECL_IMPORT
#endif
