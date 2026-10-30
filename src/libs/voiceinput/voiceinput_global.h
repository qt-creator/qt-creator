// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(VOICEINPUT_LIBRARY)
#  define VOICEINPUT_EXPORT Q_DECL_EXPORT
#elif defined(VOICEINPUT_STATIC_LIBRARY)
#  define VOICEINPUT_EXPORT
#else
#  define VOICEINPUT_EXPORT Q_DECL_IMPORT
#endif
