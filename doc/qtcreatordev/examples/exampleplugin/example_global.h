// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

#if defined(EXAMPLE_LIBRARY)
#  define EXAMPLE_EXPORT Q_DECL_EXPORT
#else
#  define EXAMPLE_EXPORT Q_DECL_IMPORT
#endif
