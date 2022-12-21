// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(QMLJSEDITOR_LIBRARY)
#  define QMLJSEDITOR_EXPORT Q_DECL_EXPORT
#elif defined(QMLJSEDITOR_STATIC_LIBRARY)
#  define QMLJSEDITOR_EXPORT
#else
#  define QMLJSEDITOR_EXPORT Q_DECL_IMPORT
#endif
