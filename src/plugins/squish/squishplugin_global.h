// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef SQUISHPLUGIN_GLOBAL_H
#define SQUISHPLUGIN_GLOBAL_H

#include <QtGlobal>

#if defined(SQUISH_LIBRARY)
#  define SQUISHSHARED_EXPORT Q_DECL_EXPORT
#else
#  define SQUISHSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // SQUISHPLUGIN_GLOBAL_H

