// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

#if defined(DEVCONTAINER_LIBRARY)
#define DEVCONTAINER_EXPORT Q_DECL_EXPORT
#elif defined(DEVCONTAINER_STATIC_LIBRARY)
#define DEVCONTAINER_EXPORT
#else
#define DEVCONTAINER_EXPORT Q_DECL_IMPORT
#endif
