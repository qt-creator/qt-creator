// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(NANOTRACE_LIBRARY)
#define NANOTRACE_EXPORT Q_DECL_EXPORT
#elif defined(NANOTRACE_STATIC_LIBRARY)
#define NANOTRACE_EXPORT
#else
#define NANOTRACE_EXPORT Q_DECL_IMPORT
#endif

#define NANOTRACESHARED_EXPORT NANOTRACE_EXPORT
