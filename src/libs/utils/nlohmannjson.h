// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qcompilerdetection.h>

QT_WARNING_PUSH

#if defined(Q_CC_MSVC)
#  pragma system_header
#elif defined(Q_CC_GNU) || defined(Q_CC_CLANG)
#  pragma GCC system_header
#endif

#include <3rdparty/json/json.hpp>

QT_WARNING_POP
