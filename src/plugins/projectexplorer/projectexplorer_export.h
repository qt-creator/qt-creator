// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

#if defined(PROJECTEXPLORER_LIBRARY)
#  define PROJECTEXPLORER_EXPORT Q_DECL_EXPORT
#elif defined(PROJECTEXPLORER_STATIC_LIBRARY)
#  define PROJECTEXPLORER_EXPORT
#else
#  define PROJECTEXPLORER_EXPORT Q_DECL_IMPORT
#endif
