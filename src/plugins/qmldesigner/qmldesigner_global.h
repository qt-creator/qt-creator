// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(QMLDESIGNER_LIBRARY)
#define QMLDESIGNER_EXPORT Q_DECL_EXPORT
#elif defined(QMLDESIGNER_STATIC_LIBRARY)
#define QMLDESIGNER_EXPORT
#else
#define QMLDESIGNER_EXPORT Q_DECL_IMPORT
#endif
