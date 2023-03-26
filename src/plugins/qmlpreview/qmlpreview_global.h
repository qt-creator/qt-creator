// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(QMLPREVIEW_LIBRARY)
#  define QMLPREVIEW_EXPORT Q_DECL_EXPORT
#elif defined(QMLPREVIEW_STATIC_LIBRARY)
#  define QMLPREVIEW_EXPORT
#else
#  define QMLPREVIEW_EXPORT Q_DECL_IMPORT
#endif
