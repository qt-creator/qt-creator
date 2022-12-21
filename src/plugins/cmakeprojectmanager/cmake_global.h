// Copyright (C) 2016 Canonical Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(CMAKEPROJECTMANAGER_LIBRARY)
#  define CMAKE_EXPORT Q_DECL_EXPORT
#elif defined(CMAKEPROJECTMANAGER_STATIC_LIBRARY)
#  define CMAKE_EXPORT
#else
#  define CMAKE_EXPORT Q_DECL_IMPORT
#endif
