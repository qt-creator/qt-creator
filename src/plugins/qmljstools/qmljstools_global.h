// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(QMLJSTOOLS_LIBRARY)
#  define QMLJSTOOLS_EXPORT Q_DECL_EXPORT
#elif defined(QMLJSTOOLS_STATIC_LIBRARY)
#  define QMLJSTOOLS_EXPORT
#else
#  define QMLJSTOOLS_EXPORT Q_DECL_IMPORT
#endif
