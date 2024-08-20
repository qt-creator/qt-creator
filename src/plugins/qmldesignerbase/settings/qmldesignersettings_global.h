// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(QMLDESIGNERSETTINGS_LIBRARY)
#  define QMLDESIGNERSETTINGS_EXPORT Q_DECL_EXPORT
#elif defined(QMLDESIGNERSETTINGS_STATIC_LIBRARY)
#  define QMLDESIGNERSETTINGS_EXPORT
#else
#  define QMLDESIGNERSETTINGS_EXPORT Q_DECL_IMPORT
#endif
