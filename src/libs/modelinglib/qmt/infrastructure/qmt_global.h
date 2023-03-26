// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtCore/QtGlobal>

#if defined(MODELING_LIBRARY)
#  define QMT_EXPORT Q_DECL_EXPORT
#elif defined(MODELING_STATIC_LIBRARY)
#  define QMT_EXPORT
#else
#  define QMT_EXPORT Q_DECL_IMPORT
#endif
