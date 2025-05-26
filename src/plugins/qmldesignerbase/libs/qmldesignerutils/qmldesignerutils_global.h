// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(QMLDESIGNERUTILS_LIBRARY)
#define QMLDESIGNERUTILS_EXPORT Q_DECL_EXPORT
#elif defined(QMLDESIGNERUTILS_STATIC_LIBRARY)
#define QMLDESIGNERUTILS_EXPORT
#else
#define QMLDESIGNERUTILS_EXPORT Q_DECL_IMPORT
#endif
