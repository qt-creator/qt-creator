// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#if defined(QMLDEBUG_LIBRARY)
#  define QMLDEBUG_EXPORT Q_DECL_EXPORT
#elif defined(QMLDEBUG_STATIC_LIBRARY)
#  define QMLDEBUG_EXPORT
#else
#  define QMLDEBUG_EXPORT Q_DECL_IMPORT
#endif
