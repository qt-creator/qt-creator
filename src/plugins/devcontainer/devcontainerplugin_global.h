// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(DEVCONTAINERPLUGIN_LIBRARY)
#define DEVCONTAINERPLUGIN_EXPORT Q_DECL_EXPORT
#elif defined(DEVCONTAINERPLUGIN_STATIC_LIBRARY)
#define DEVCONTAINERPLUGIN_EXPORT
#else
#define DEVCONTAINERPLUGIN_EXPORT Q_DECL_IMPORT
#endif
