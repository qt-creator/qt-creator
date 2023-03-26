// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(LANGUAGESERVERPROTOCOL_LIBRARY)
#  define LANGUAGESERVERPROTOCOL_EXPORT Q_DECL_EXPORT
#elif defined(LANGUAGESERVERPROTOCOL_STATIC_LIBRARY)
#  define LANGUAGESERVERPROTOCOL_EXPORT
#else
#  define LANGUAGESERVERPROTOCOL_EXPORT Q_DECL_IMPORT
#endif
