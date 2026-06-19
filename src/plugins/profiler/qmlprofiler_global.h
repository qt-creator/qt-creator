// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(QMLPROFILER_LIBRARY)
#  define QMLPROFILER_EXPORT Q_DECL_EXPORT
#elif defined(QMLPROFILER_STATIC_LIBRARY)
#  define QMLPROFILER_EXPORT
#else
#  define QMLPROFILER_EXPORT Q_DECL_IMPORT
#endif
