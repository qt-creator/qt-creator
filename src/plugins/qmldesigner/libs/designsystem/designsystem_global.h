// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(DESIGNSYSTEM_LIBRARY)
#  define DESIGNSYSTEM_EXPORT Q_DECL_EXPORT
#elif defined(DESIGNSYSTEM_STATIC_LIBRARY)
#  define DESIGNSYSTEM_EXPORT
#else
#  define DESIGNSYSTEM_EXPORT Q_DECL_IMPORT
#endif
