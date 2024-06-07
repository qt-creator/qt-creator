// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TASKING_GLOBAL_H
#define TASKING_GLOBAL_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#if defined(TASKING_LIBRARY)
#  define TASKING_EXPORT Q_DECL_EXPORT
#elif defined(TASKING_STATIC_LIBRARY)
#  define TASKING_EXPORT
#else
#  define TASKING_EXPORT Q_DECL_IMPORT
#endif

QT_END_NAMESPACE

#endif // TASKING_GLOBAL_H
