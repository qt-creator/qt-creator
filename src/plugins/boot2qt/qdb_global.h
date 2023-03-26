// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(BOOT2QT_LIBRARY)
#  define QDBPLUGINSHARED_EXPORT Q_DECL_EXPORT
#else
#  define QDBPLUGINSHARED_EXPORT Q_DECL_IMPORT
#endif
