// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(MODELEDITOR_LIBRARY)
#  define MODELEDITOR_EXPORT Q_DECL_EXPORT
#elif defined(MODELEDITOR_STATIC_LIBRARY)
#  define MODELEDITOR_EXPORT
#else
#  define MODELEDITOR_EXPORT Q_DECL_IMPORT
#endif
