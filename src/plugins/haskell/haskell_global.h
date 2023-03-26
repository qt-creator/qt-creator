// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(HASKELL_LIBRARY)
#  define HASKELLSHARED_EXPORT Q_DECL_EXPORT
#else
#  define HASKELLSHARED_EXPORT Q_DECL_IMPORT
#endif
