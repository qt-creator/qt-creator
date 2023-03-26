// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(WEBASSEMBLY_LIBRARY)
#  define WEBASSEMBLYSHARED_EXPORT Q_DECL_EXPORT
#else
#  define WEBASSEMBLYSHARED_EXPORT Q_DECL_IMPORT
#endif
