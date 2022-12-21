// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(DIFFEDITOR_LIBRARY)
#  define DIFFEDITOR_EXPORT Q_DECL_EXPORT
#elif defined(DIFFEDITOR_STATIC_LIBRARY)
#  define DIFFEDITOR_EXPORT
#else
#  define DIFFEDITOR_EXPORT Q_DECL_IMPORT
#endif
