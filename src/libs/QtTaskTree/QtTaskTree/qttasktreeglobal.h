// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTTASKTREEGLOBAL_H
#define QTTASKTREEGLOBAL_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#if defined(QTTASKTREE_LIBRARY)
#  define Q_TASKTREE_EXPORT Q_DECL_EXPORT
#elif defined(Q_TASKTREE_STATIC_LIBRARY)
#  define Q_TASKTREE_EXPORT
#else
#  define Q_TASKTREE_EXPORT Q_DECL_IMPORT
#endif

QT_END_NAMESPACE

#endif // QTTASKTREEGLOBAL_H
