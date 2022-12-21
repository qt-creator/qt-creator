// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(DEBUGGER_LIBRARY)
#  define DEBUGGER_EXPORT Q_DECL_EXPORT
#elif defined(DEBUGGER_STATIC_LIBRARY) // Abuse single files for tests
#  define DEBUGGER_EXPORT
#else
#  define DEBUGGER_EXPORT Q_DECL_IMPORT
#endif
