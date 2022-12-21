// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

#if defined(VCSBASE_LIBRARY)
#  define VCSBASE_EXPORT Q_DECL_EXPORT
#elif defined(VCSBASE_STATIC_LIBRARY)
#  define VCSBASE_EXPORT
#else
#  define VCSBASE_EXPORT Q_DECL_IMPORT
#endif
