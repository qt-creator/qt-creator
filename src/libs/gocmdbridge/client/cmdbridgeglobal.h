// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0+

#pragma once

#include <qglobal.h>

#if defined(CMDBRIDGECLIENT_LIBRARY)
#define QTCREATOR_CMDBRIDGE_EXPORT Q_DECL_EXPORT
#elif defined(CMDBRIDGECLIENT_STATIC_LIBRARY) // Abuse single files for manual tests
#define QTCREATOR_CMDBRIDGE_EXPORT
#else
#define QTCREATOR_CMDBRIDGE_EXPORT Q_DECL_IMPORT
#endif
